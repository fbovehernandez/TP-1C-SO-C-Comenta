#include "../include/conexiones.h"

t_dictionary* diccionario_io;
t_list* lista_procesos;
t_list* lista_io;
sem_t sem_cola_io;
pthread_mutex_t mutex_lista_io;
pthread_mutex_t mutex_cola_io_generica;

ptr_kernel* solicitar_datos(t_config* config_kernel){
    ptr_kernel* datos = malloc(sizeof(ptr_kernel));

    datos->ip_cpu                   = config_get_string_value(config_kernel, "IP_CPU");
    datos->ip_mem                   = config_get_string_value(config_kernel, "IP_MEMORIA");
    datos->puerto_memoria           = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
    datos->puerto_cpu_dispatch      = config_get_string_value(config_kernel, "PUERTO_CPU_DISPATCH");
    datos->puerto_cpu_interrupt     = config_get_string_value(config_kernel, "PUERTO_CPU_INTERRUPT");
    datos->puerto_io                = config_get_string_value(config_kernel, "PUERTO_IO");
    datos->quantum                  = config_get_int_value(config_kernel, "QUANTUM");
    datos->grado_multiprogramacion  = config_get_int_value(config_kernel, "GRADO_MULTIPROGRAMACION");
    datos->algoritmo_planificacion  = config_get_string_value(config_kernel, "ALGORITMO_PLANIFICACION");

    char** recursos            = config_get_array_value(config_kernel, "RECURSOS");
    char** instancias_recursos = config_get_array_value(config_kernel, "INSTANCIAS_RECURSOS");

    datos->diccionario_recursos = dictionary_create();
    
    for(int i=0; i<string_array_size(recursos); i++) {
        t_recurso *recurso = malloc(sizeof(t_recurso));
        recurso->instancias = atoi(instancias_recursos[i]);
        recurso->procesos_bloqueados = queue_create();
        dictionary_put(datos->diccionario_recursos, recursos[i], recurso); //Mas tarde para sumar o restar deberemos castear a int instancias_recursos[i]
        /*printf("Recurso: %s\n", recursos[i]);
        printf("Diccionario en instancias recursos: %d\n", ((t_recurso*)(dictionary_get(datos->diccionario_recursos, recursos[i])))->instancias);    */
    }

    for(int i=0; i<string_array_size(recursos); i++){
        printf("Recurso: %s\n", recursos[i]);
        printf("Diccionario en instancias recursos: %d\n", ((t_recurso*)(dictionary_get(datos->diccionario_recursos, recursos[i])))->instancias);  
    }
    return datos;
}

int esperar_cliente(int socket_escucha, t_log* logger) {
    int handshake = 0;
    int resultError = -1;
    printf("Esperando a la IO\n");
    
    int socket_cliente = accept(socket_escucha, NULL,  NULL);

    // corroborar_exito(socket_cliente, "aceptar el handshake.");

    if(socket_cliente == -1) {
        printf("No acepto el handshake.\n");
        return -1;
    }

    recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL);
    if(handshake == 5) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io_generica, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else if( handshake == 13) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io_stdin, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } if( handshake == 15) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io_stdout, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        close(socket_cliente);
        return -1;
    }

    return socket_cliente;
}

// Habria que ver de scar logica

void *handle_io_stdin(void *socket_io) {
    int socket = (intptr_t)socket_io;

    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;
    
    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            io = establecer_conexion(paquete->buffer, socket);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }    

    t_pid_stdin* pid_stdin = malloc(sizeof(t_pid_stdin));

    while (true) {
        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");

        pthread_mutex_lock(&mutex_cola_io_generica);
        io_stdin *datos_stdin = queue_pop(io->cola_blocked);
        pthread_mutex_unlock(&mutex_cola_io_generica);

        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE

        pid_stdin->pid = datos_stdin->pcb->pid;
        pid_stdin->cantidad_paginas = datos_stdin->cantidad_paginas;
        pid_stdin->lista_direcciones = datos_stdin->lista_direcciones;
        pid_stdin->registro_tamanio = datos_stdin->registro_tamanio;
        
        int respuesta_ok = ejecutar_io_stdin(io->socket, pid_stdin);

        if (!respuesta_ok) {
            printf("Se ejecuto correctamente la IO...\n");
            // printf("Estado pcb: %d\n", datos_sleep->pcb->estadoActual);
            // datos_sleep->pcb->estadoActual = READY;
            // printf("Estado pcb : %d\n", datos_sleep->pcb->estadoActual);
            int termino_io;
            recv(socket, &termino_io, sizeof(int), MSG_WAITALL);

            printf("Termino io: %d\n", termino_io);
            if (termino_io == 1) { // El send de termino io envia 1.
                printf("Termino la IO\n");
                pasar_a_ready(datos_stdin->pcb);
            }
        } else {
            printf("No se pudo ejecutar la IO\n");
            break;
        }
    }

    free(pid_stdin);
    liberar_paquete(paquete);
    return NULL;
}

int ejecutar_io_stdin(int socket, t_pid_stdin* pid_stdin) {
    t_buffer* buffer = malloc(sizeof(t_buffer)); 

    buffer->size = sizeof(int) * 3 + pid_stdin->cantidad_paginas;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid_stdin->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_stdin->registro_tamanio, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_stdin->cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    for(int i=0; i < pid_stdin->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pid_stdin->lista_direcciones, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }

    enviar_paquete(buffer, LEETE, socket);

    return 0;
}

void* handle_io_stdout(void* socket) {
    int socket_io = (intptr_t) socket;
    int resultOk = 0;
    send(socket_io, &resultOk, sizeof(int), 0);
    printf("Conexion establecida con I/O\n");

    t_list_io* io = malloc(sizeof(t_list_io)); 

    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    printf("Esperando recibir paquete de IO\n");
    recv(socket_io, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);

    printf("Recibi el codigo de operacion de IO: %d\n", paquete->codigo_operacion);

    recv(socket_io, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

    paquete->buffer->stream = malloc(paquete->buffer->size);

    recv(socket_io, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            t_info_io* interfaz = deserializar_interfaz(paquete->buffer);
            io->socket = socket_io;
            io->TipoInterfaz = interfaz->tipo;
            io->nombreInterfaz = interfaz->nombre_interfaz;
            io->cola_blocked = queue_create();
            sem_init(io->semaforo_cola_procesos_blocked, 0, 0);
            
            dictionary_put(diccionario_io, interfaz->nombre_interfaz, (void*) io);
            mostrar_elem_diccionario(interfaz->nombre_interfaz);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    free(io->semaforo_cola_procesos_blocked);
    free(io->nombreInterfaz);
    free(io);
    liberar_paquete(paquete);
    return NULL;
}

void *handle_io_generica(void *socket_io) {
    int socket = (intptr_t)socket_io;
    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;
    switch (paquete->codigo_operacion) {
    case CONEXION_INTERFAZ:
        io = establecer_conexion(paquete->buffer, socket);
        free(io);
        break;
    default:
        printf("Llega al default.");
        return NULL;
    }

    t_pid_unidades_trabajo* pid_unidades_trabajo = malloc(sizeof(t_pid_unidades_trabajo));

    while (true) {
        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");

        pthread_mutex_lock(&mutex_cola_io_generica);
        io_gen_sleep *datos_sleep = queue_pop(io->cola_blocked);
        pthread_mutex_unlock(&mutex_cola_io_generica);
        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE
        pid_unidades_trabajo->pid = datos_sleep->pcb->pid;
        pid_unidades_trabajo->unidades_trabajo = datos_sleep->unidad_trabajo;

        int respuesta_ok = ejecutar_io_generica(io->socket, pid_unidades_trabajo);
        if (!respuesta_ok) {
            printf("Se ejecuto correctamente la IO...\n");
            // printf("Estado pcb: %d\n", datos_sleep->pcb->estadoActual);
            // datos_sleep->pcb->estadoActual = READY;
            // printf("Estado pcb : %d\n", datos_sleep->pcb->estadoActual);
            int termino_io;
            recv(socket, &termino_io, sizeof(int), MSG_WAITALL);

            printf("Termino io: %d\n", termino_io);
            if (termino_io == 1) { // El send de termino io envia 1.
                printf("Termino la IO\n");
                pasar_a_ready(datos_sleep->pcb);
            }
        } else {
            printf("No se pudo ejecutar");
        }    
    }
    free(pid_unidades_trabajo);
    liberarIOyPaquete(paquete, io);
    
    return NULL;
}

int ejecutar_io_generica(int socket_io, t_pid_unidades_trabajo* pid_unidades_trabajo) {
    t_buffer *buffer = malloc(sizeof(t_buffer)); // Creo que no hace falta reservar memoria
    buffer->size = sizeof(int) * 2;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid_unidades_trabajo->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_unidades_trabajo->unidades_trabajo, sizeof(int));

    enviar_paquete(buffer, DORMITE, socket_io);
    return 0;
}

int conectar_kernel_cpu_interrupt(t_log* logger_kernel, char* IP_CPU, char* puerto_interrupt) {
    int valor = 3;
    int interruptfd = crear_conexion(IP_CPU, puerto_interrupt, valor);
    log_info(logger_kernel, "Conexion establecida con Interrupt");
    // send(dispatcherfd, &message, sizeof(int), 0);
    return interruptfd;   
}

int conectar_kernel_cpu_dispatch(t_log* logger_kernel, char* IP_CPU, char* puerto_cpu_dispatch) {
    int valor = 1;
    // int message = 10;
    int dispatcherfd = crear_conexion(IP_CPU, puerto_cpu_dispatch, valor);
    log_info(logger_kernel, "Conexion establecida con Dispatcher");
    // send(dispatcherfd, &message, sizeof(int), 0);
    return dispatcherfd;
}

int conectar_kernel_memoria(char* ip_memoria, char* puerto_memoria, t_log* logger_kernel) {
    int valor = 1;

    int memoriafd = crear_conexion(ip_memoria, puerto_memoria, valor);
    log_info(logger_kernel, "Conexion establecida con Memoria");

    // send(memoriafd, &message_kernel, sizeof(int), 0); 

    // close(memoriafd); // (POSIBLE) no cierro el socket porque quiero reutilizar la conexion
    return memoriafd;
}

void* escuchar_IO(void* kernel_io) { //kernel_io es el socket del kernel
    t_config_kernel* kernel_struct_io = (void*) kernel_io;

    int socket_io = kernel_struct_io->socket;
    t_log* logger_kernel = kernel_struct_io->logger;

    while(1) {
        int socket_cliente = esperar_cliente(socket_io, logger_kernel);
    }
    
    return NULL;
}

void mostrar_elem_diccionario(char* nombre_interfaz) {
    t_list_io* interfaz = dictionary_get(diccionario_io, nombre_interfaz);
    printf("Nombre Interfaz: %s\n", nombre_interfaz);
    printf("Tipo Interfaz (enum): %d\n", interfaz->TipoInterfaz);
    printf("Socket: %d\n", interfaz->socket);
}

void liberarIOyPaquete(t_paquete *paquete, t_list_io *io) {
    liberar_io(io);
    liberar_paquete(paquete);
}

void liberar_io(t_list_io* io) {
    free(io->semaforo_cola_procesos_blocked);
    free(io->nombreInterfaz);
    free(io);
}

t_list_io* establecer_conexion(t_buffer *buffer, int socket_io) {
    t_list_io *io = malloc(sizeof(t_list_io));
    t_info_io *interfaz = deserializar_interfaz(buffer); 

    io->socket         = socket_io;
    io->TipoInterfaz   = interfaz->tipo;
    io->nombreInterfaz = interfaz->nombre_interfaz;
    io->cola_blocked   = queue_create();
    
    sem_init(io->semaforo_cola_procesos_blocked, 0, 0);
    dictionary_put(diccionario_io, interfaz->nombre_interfaz, (void *)io);
    mostrar_elem_diccionario(interfaz->nombre_interfaz);
    
    return io;
}
