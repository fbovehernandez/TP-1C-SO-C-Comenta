#include "../include/conexiones.h"

t_dictionary* diccionario_io;
t_list* lista_procesos;
t_list* lista_io;
sem_t sem_cola_io;
pthread_mutex_t mutex_lista_io;
pthread_mutex_t mutex_cola_io_generica;
t_sockets* sockets;

ptr_kernel* solicitar_datos(t_config* config_kernel){
    ptr_kernel* datos = malloc(sizeof(ptr_kernel)); //FREE?

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
        t_recurso* recurso = malloc(sizeof(t_recurso));
        recurso->instancias = atoi(instancias_recursos[i]);
        recurso->procesos_bloqueados = list_create();
        dictionary_put(datos->diccionario_recursos, recursos[i], recurso); //Mas tarde para sumar o restar deberemos castear a int instancias_recursos[i]
        //FREE?
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
    int resultOk = 0;
    
    printf("Esperando a la IO\n");
    
    int socket_cliente = accept(socket_escucha, NULL,  NULL);

    // corroborar_exito(socket_cliente, "aceptar el handshake.");

    if(socket_cliente == -1) {
        printf("No acepto el handshake.\n");
        return -1;
    }
    
    recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL);
    send(socket_cliente, &resultOk, sizeof(int), 0);

    if(handshake == 5) {
        pthread_t hilo_io;
        printf("Llego hasta crear el hilo generico\n");
        pthread_create(&hilo_io, NULL, (void*) handle_io_generica, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else if( handshake == 13) {
        pthread_t hilo_io;
        printf("llego hasta crear el hilo stdin\n");
        pthread_create(&hilo_io, NULL, (void*) handle_io_stdin, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else if(handshake == 15) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io_stdout, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else if(handshake == 17) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io_dialfs, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        printf("ENTRO POR ACA\n");
        close(socket_cliente);
        return -1;
    }

    return socket_cliente;
}

// Habria que ver de scar logica

void *handle_io_stdin(void *socket_io) {
    printf("llego hasta handle io stdin 1\n");    
    int socket = (intptr_t)socket_io;
    int termino_io;

    // Por que manda esto
    // int result = 0;
    // send(socket, &result, sizeof(int), 0);
    
    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    
    t_list_io* io;

    printf("codigo de op: %d\n", paquete->codigo_operacion); // Ahora no imprime esto
    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ: 
            printf("llego hasta handle io stdin 3\n");
            io = establecer_conexion(paquete->buffer, socket);
            // free(io);
            break;
        default:
            printf("Llega al default en handle_io_stdin.\n");
            return NULL;
    }

    t_pid_stdin* pid_stdin = malloc(sizeof(t_pid_stdin));

    while (true) {
        int size_dictionary = dictionary_size(diccionario_io);
        printf("\nSize del diccionario: %d\n", size_dictionary);

        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");
        
        // io_std *datos_stdin = malloc(sizeof(io_std)); -> No hace falta "aparentemente"
        io_std* datos_stdin; 
        
        pthread_mutex_lock(&mutex_cola_io_generica);

        if(list_is_empty(io->cola_blocked)) {
            printf("No hay procesos en la cola de bloqueados de la IO\n");
        } else {
           datos_stdin = list_remove(io->cola_blocked, 0); // FREE?
        }

        pthread_mutex_unlock(&mutex_cola_io_generica);

        pasar_a_blocked(datos_stdin->pcb);
        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE
        
        pid_stdin->pid = datos_stdin->pcb->pid;
        pid_stdin->cantidad_paginas = datos_stdin->cantidad_paginas;
        pid_stdin->lista_direcciones = datos_stdin->lista_direcciones;
        pid_stdin->registro_tamanio = datos_stdin->registro_tamanio;

        // mandar la io a memoria
        
        int respuesta_ok = ejecutar_io_stdin(socket, pid_stdin);

        printf("la respuesta ok es: %d\n", respuesta_ok);
        if (!respuesta_ok) {
            printf("Se ejecuto correctamente la IO...\n");
        
            // printf("Estado pcb: %d\n", datos_sleep->pcb->estadoActual);
            // datos_sleep->pcb->estadoActual = READY;
            // printf("Estado pcb : %d\n", datos_sleep->pcb->estadoActual);
            
            recv(socket, &termino_io, sizeof(int), MSG_WAITALL);
            
            printf("Termino io: %d\n", termino_io);
            if (termino_io == 1) { // El send de termino io envia 1.
                printf("Termino la IO\n");

                // Esto creo que no esta bien, pero no se como liberarlo...
                // t_pcb* pcb = datos_stdin->pcb;
                pasar_a_ready(datos_stdin->pcb);
                
                // free(datos_stdin->pcb->registros);
                // free(datos_stdin->pcb);
                // free(datos_stdin);
            }
        } else {
            printf("No se pudo ejecutar la IO\n");
            break;
        }
    }
    
    list_destroy(pid_stdin->lista_direcciones);
    free(pid_stdin);
    liberar_paquete(paquete);
    return NULL;
}

int ejecutar_io_stdin(int socket, t_pid_stdin* pid_stdin) {
    printf("Voy a ejecutar stdin!\n");
    t_buffer* buffer = malloc(sizeof(t_buffer)); 

    buffer->size =  sizeof(int) * 2 + sizeof(uint32_t) + pid_stdin->cantidad_paginas * 2 * sizeof(int); 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid_stdin->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_stdin->registro_tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &pid_stdin->cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int); 
    printf("la cant de paginas es: %d\n", pid_stdin->cantidad_paginas);

    for(int i=0; i < pid_stdin->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pid_stdin->lista_direcciones, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }

    printf("Va a hacer LEETE... o eso deberia.\n");
    enviar_paquete(buffer, LEETE, socket);

    int resultOk;

    recv(socket, &resultOk, sizeof(int), MSG_WAITALL);
    printf("el resultOk es: %d\n", resultOk);
    return resultOk; // Ojo que siempre retorna 0 y por ende ejecuta bien
}

void* handle_io_stdout(void* socket_io) {
    printf("llego hasta handle io stdout 1\n");    
    int socket = (intptr_t)socket_io;
    
    /*int result = 0;
    send(socket, &result, sizeof(int), 0);*/

    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;
    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            printf("llego hasta handle io stdout 2\n");
            io = establecer_conexion(paquete->buffer, socket);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    // ESTO NO ES LO MISMO QUE STDIN
    t_pid_stdout* pid_stdout = malloc(sizeof(t_pid_stdout)); //FREE?

    while (true) {
        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");
        
        /////////////////// hasta aca se mantiene

        io_std *datos_stdout = malloc(sizeof(io_std)); //FREE?

        pthread_mutex_lock(&mutex_cola_io_generica);
        if(list_is_empty(io->cola_blocked)) {
            printf("No hay procesos en la cola de bloqueados de la IO\n");
        } else {
            datos_stdout = list_remove(io->cola_blocked, 0);
        }
        pthread_mutex_unlock(&mutex_cola_io_generica);

        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE

        pasar_a_blocked(datos_stdout->pcb);

        pid_stdout->pid = datos_stdout->pcb->pid;
        pid_stdout->cantidad_paginas = datos_stdout->cantidad_paginas;
        pid_stdout->lista_direcciones = datos_stdout->lista_direcciones;
        pid_stdout->registro_tamanio = datos_stdout->registro_tamanio;
        pid_stdout->nombre_interfaz = malloc(string_length(io->nombreInterfaz)); //FREE?
        pid_stdout->nombre_interfaz = io->nombreInterfaz;
        printf("\nLa io conectada tiene nombre %s\n\n", io->nombreInterfaz);
        pid_stdout->largo_interfaz = string_length(io->nombreInterfaz) + 1;

        int respuesta_ok = ejecutar_io_stdout(pid_stdout);

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
                pasar_a_ready(datos_stdout->pcb);
            }
        } else {
            printf("No se pudo ejecutar la IO\n");
            break;
        }
    }

    free(io->semaforo_cola_procesos_blocked);
    free(io->nombreInterfaz);
    free(io);
    liberar_paquete(paquete);
    return NULL;
}

int ejecutar_io_stdout(t_pid_stdout* pid_stdout) {
    // Mandar todo a memoria
    printf("Voy a ejecutar stdout!\n");
    printf("\n\nEL PID QUE SE VA A MANDAR A MEMORIA ES: %d\n\n", pid_stdout->pid);

    t_buffer* buffer = malloc(sizeof(t_buffer)); 

    buffer->size = sizeof(int) * 3 + sizeof(uint32_t) + pid_stdout->cantidad_paginas * 2 * sizeof(int) + pid_stdout->largo_interfaz; 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid_stdout->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_stdout->registro_tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &pid_stdout->cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    printf("\nEl largo de la interfaz es: %d\n", pid_stdout->largo_interfaz);
    memcpy(buffer->stream + buffer->offset, &pid_stdout->largo_interfaz, sizeof(int)); 
    buffer->offset += sizeof(int);
    printf("\n\nLa interfaz a mandar es: %s\n\n", pid_stdout->nombre_interfaz);
    memcpy(buffer->stream + buffer->offset, pid_stdout->nombre_interfaz, pid_stdout->largo_interfaz);
    buffer->offset += pid_stdout->largo_interfaz;
    
    for(int i=0; i < pid_stdout->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pid_stdout->lista_direcciones, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }
    
    enviar_paquete(buffer, ESCRIBIR_STDOUT, sockets->socket_memoria); //ver socket

    int resultOk;

    recv(sockets->socket_memoria, &resultOk, sizeof(int), MSG_WAITALL);

    return resultOk;
}

void* handle_io_dialfs(void* socket_io) {
    int socket = (intptr_t)socket_io;

    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;

    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            printf("Vamos a establecer conexion con dialfs!!!\n");
            io = establecer_conexion(paquete->buffer, socket);
            // free(io);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    while(true) {
        // compl, no es espera activa
    }

    liberar_paquete(paquete);
    return NULL;
}

void *handle_io_generica(void *socket_io) {
    int socket = (intptr_t)socket_io;
    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;

    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            printf("Vamos a establecer conexion generica!!!\n");
            io = establecer_conexion(paquete->buffer, socket);
            // free(io);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    t_pid_unidades_trabajo* pid_unidades_trabajo = malloc(sizeof(t_pid_unidades_trabajo));

    while (true) {
        printf("Esperando semaforo\n");
        
        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");

        pthread_mutex_lock(&mutex_cola_io_generica);
        io_gen_sleep *datos_sleep = list_remove(io->cola_blocked, 0);
        pthread_mutex_unlock(&mutex_cola_io_generica);
        
        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE
        pid_unidades_trabajo->pid = datos_sleep->pcb->pid;
        pid_unidades_trabajo->unidades_trabajo = datos_sleep->unidad_trabajo;
        printf("La cola nos saca esto: PID %d, UT %d\n", datos_sleep->pcb->pid, datos_sleep->unidad_trabajo);

        int respuesta_ok = ejecutar_io_generica(io->socket, pid_unidades_trabajo);
        
        if (!respuesta_ok) {
            printf("Se ejecuto correctamente la IO... que le mandamos el pid %d\n", pid_unidades_trabajo->pid);
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
    // liberarIOyPaquete(paquete, io);
    liberar_paquete(paquete);
    
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
    printf("Nombre Interfaz que queremos: %s\n", nombre_interfaz);
    printf("Nombre Interfaz de Dictionary: %s\n", interfaz->nombreInterfaz);
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
    t_list_io *io = malloc(sizeof(t_list_io)); //FREE?
    t_info_io *interfaz = deserializar_interfaz(buffer); 
    io->nombreInterfaz = malloc(string_length(interfaz->nombre_interfaz));
    printf("\n\nllego hasta establecer conexion\n\n");

    io->socket         = socket_io;
    io->TipoInterfaz   = interfaz->tipo;
    // io->nombreInterfaz = interfaz->nombre_interfaz;
    io->cola_blocked   = list_create();
    
    // Esta memoria la liberamos cuando la io se desconecte
    io->semaforo_cola_procesos_blocked = malloc(sizeof(sem_t)); //FREE?
    sem_init(io->semaforo_cola_procesos_blocked, 0, 0);
    
    io->nombreInterfaz = strdup(interfaz->nombre_interfaz); // Asignación del nombre
    
    dictionary_put(diccionario_io, interfaz->nombre_interfaz, (void*)io);
    printf("\n\n el nombre de la interfaz es: %s\n\n", interfaz->nombre_interfaz);
    mostrar_elem_diccionario(interfaz->nombre_interfaz);

    free(interfaz->nombre_interfaz);
    free(interfaz);
    
    return io;
}

/* 

case IO_STDIN_READ: 

        t_list* lista_bytes_stdin = list_create();
        t_list* lista_direcciones_fisicas_stdin = list_create();

        t_parametro* interfaz = list_get(list_parametros,0);
        registro_direccion = list_get(list_parametros, 1); // Cambiar, usa el de arriba, mejor nombre?
        t_parametro *registro_tamanio = list_get(list_parametros, 2);

        char* nombre_registro_direccion = registro_direccion->nombre;
        char* nombre_registro_tamanio = registro_tamanio->nombre;

        uint32_t* registro_direccion_stdin = (uint32_t*) seleccionar_registro_cpu(nombre_registro_direccion);
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_tamanio);

        uint32_t* registro_tamanio_stdin = (uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio);
        
        int pagina = floor(*registro_direccion_stdin / tamanio_pagina);
        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato); //Ojo que abajo no le paso el tam_byte, sino la cantidad que tiene dentro

        int cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion_stdin, tamanio_en_byte, pagina, lista_bytes_stdin); // Cantidad de paginas + la primera
        
        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_stdin, *registro_direccion_stdin, pcb->pid, lista_direcciones_fisicas_stdin, pagina);

        t_buffer* buffer_lectura = llenar_buffer_stdio(interfaz->nombre, lista_direcciones_fisicas_stdin, *registro_tamanio_stdin, cantidad_paginas);
        
        pcb->program_counter++;
        desalojar(pcb, PEDIDO_LECTURA, buffer_lectura);

        free(buffer_lectura);
        return 1; // El return esta para que cuando se desaloje el pcb no siga ejecutando, si hace el break sigue pidiendo instrucciones, porfa no lo saquen o les va a romper
        break;

*/
#include "../include/conexiones.h"

t_dictionary* diccionario_io;
t_list* lista_procesos;
t_list* lista_io;
sem_t sem_cola_io;
pthread_mutex_t mutex_lista_io;
pthread_mutex_t mutex_cola_io_generica;
t_sockets* sockets;

ptr_kernel* solicitar_datos(t_config* config_kernel){
    ptr_kernel* datos = malloc(sizeof(ptr_kernel)); //FREE?

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
        t_recurso* recurso = malloc(sizeof(t_recurso));
        recurso->instancias = atoi(instancias_recursos[i]);
        recurso->procesos_bloqueados = list_create();
        dictionary_put(datos->diccionario_recursos, recursos[i], recurso); //Mas tarde para sumar o restar deberemos castear a int instancias_recursos[i]
        //FREE?
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
    int resultOk = 0;
    
    printf("Esperando a la IO\n");
    
    int socket_cliente = accept(socket_escucha, NULL,  NULL);

    // corroborar_exito(socket_cliente, "aceptar el handshake.");

    if(socket_cliente == -1) {
        printf("No acepto el handshake.\n");
        return -1;
    }
    
    recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL);
    send(socket_cliente, &resultOk, sizeof(int), 0);

    if(handshake == 5) {
        pthread_t hilo_io;
        printf("Llego hasta crear el hilo generico\n");
        pthread_create(&hilo_io, NULL, (void*) handle_io_generica, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else if( handshake == 13) {
        pthread_t hilo_io;
        printf("llego hasta crear el hilo stdin\n");
        pthread_create(&hilo_io, NULL, (void*) handle_io_stdin, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else if(handshake == 15) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io_stdout, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else if(handshake == 17) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io_dialfs, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        printf("ENTRO POR ACA\n");
        close(socket_cliente);
        return -1;
    }

    return socket_cliente;
}

// Habria que ver de scar logica

void *handle_io_stdin(void *socket_io) {
    printf("llego hasta handle io stdin 1\n");    
    int socket = (intptr_t)socket_io;
    int termino_io;

    // Por que manda esto
    // int result = 0;
    // send(socket, &result, sizeof(int), 0);
    
    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    
    t_list_io* io;

    printf("codigo de op: %d\n", paquete->codigo_operacion); // Ahora no imprime esto
    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ: 
            printf("llego hasta handle io stdin 3\n");
            io = establecer_conexion(paquete->buffer, socket);
            // free(io);
            break;
        default:
            printf("Llega al default en handle_io_stdin.\n");
            return NULL;
    }

    t_pid_stdin* pid_stdin = malloc(sizeof(t_pid_stdin));

    while (true) {
        int size_dictionary = dictionary_size(diccionario_io);
        printf("\nSize del diccionario: %d\n", size_dictionary);

        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");
        
        // io_std *datos_stdin = malloc(sizeof(io_std)); -> No hace falta "aparentemente"
        io_std* datos_stdin; 
        
        pthread_mutex_lock(&mutex_cola_io_generica);

        if(list_is_empty(io->cola_blocked)) {
            printf("No hay procesos en la cola de bloqueados de la IO\n");
        } else {
           datos_stdin = list_remove(io->cola_blocked, 0); // FREE?
        }

        pthread_mutex_unlock(&mutex_cola_io_generica);

        pasar_a_blocked(datos_stdin->pcb);
        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE
        
        pid_stdin->pid = datos_stdin->pcb->pid;
        pid_stdin->cantidad_paginas = datos_stdin->cantidad_paginas;
        pid_stdin->lista_direcciones = datos_stdin->lista_direcciones;
        pid_stdin->registro_tamanio = datos_stdin->registro_tamanio;

        // mandar la io a memoria
        
        int respuesta_ok = ejecutar_io_stdin(socket, pid_stdin);

        printf("la respuesta ok es: %d\n", respuesta_ok);
        if (!respuesta_ok) {
            printf("Se ejecuto correctamente la IO...\n");
        
            // printf("Estado pcb: %d\n", datos_sleep->pcb->estadoActual);
            // datos_sleep->pcb->estadoActual = READY;
            // printf("Estado pcb : %d\n", datos_sleep->pcb->estadoActual);
            
            recv(socket, &termino_io, sizeof(int), MSG_WAITALL);
            
            printf("Termino io: %d\n", termino_io);
            if (termino_io == 1) { // El send de termino io envia 1.
                printf("Termino la IO\n");

                // Esto creo que no esta bien, pero no se como liberarlo...
                // t_pcb* pcb = datos_stdin->pcb;
                pasar_a_ready(datos_stdin->pcb);
                
                // free(datos_stdin->pcb->registros);
                // free(datos_stdin->pcb);
                // free(datos_stdin);
            }
        } else {
            printf("No se pudo ejecutar la IO\n");
            break;
        }
    }
    
    list_destroy(pid_stdin->lista_direcciones);
    free(pid_stdin);
    liberar_paquete(paquete);
    return NULL;
}

int ejecutar_io_stdin(int socket, t_pid_stdin* pid_stdin) {
    printf("Voy a ejecutar stdin!\n");
    t_buffer* buffer = malloc(sizeof(t_buffer)); 

    buffer->size =  sizeof(int) * 2 + sizeof(uint32_t) + pid_stdin->cantidad_paginas * 2 * sizeof(int); 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid_stdin->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_stdin->registro_tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &pid_stdin->cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int); 
    printf("la cant de paginas es: %d\n", pid_stdin->cantidad_paginas);

    for(int i=0; i < pid_stdin->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pid_stdin->lista_direcciones, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }

    printf("Va a hacer LEETE... o eso deberia.\n");
    enviar_paquete(buffer, LEETE, socket);

    int resultOk;

    recv(socket, &resultOk, sizeof(int), MSG_WAITALL);
    printf("el resultOk es: %d\n", resultOk);
    return resultOk; // Ojo que siempre retorna 0 y por ende ejecuta bien
}

void* handle_io_stdout(void* socket_io) {
    printf("llego hasta handle io stdout 1\n");    
    int socket = (intptr_t)socket_io;
    
    /*int result = 0;
    send(socket, &result, sizeof(int), 0);*/

    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;
    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            printf("llego hasta handle io stdout 2\n");
            io = establecer_conexion(paquete->buffer, socket);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    // ESTO NO ES LO MISMO QUE STDIN
    t_pid_stdout* pid_stdout = malloc(sizeof(t_pid_stdout)); //FREE?

    while (true) {
        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");
        
        /////////////////// hasta aca se mantiene

        io_std *datos_stdout = malloc(sizeof(io_std)); //FREE?

        pthread_mutex_lock(&mutex_cola_io_generica);
        if(list_is_empty(io->cola_blocked)) {
            printf("No hay procesos en la cola de bloqueados de la IO\n");
        } else {
            datos_stdout = list_remove(io->cola_blocked, 0);
        }
        pthread_mutex_unlock(&mutex_cola_io_generica);

        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE

        pasar_a_blocked(datos_stdout->pcb);

        pid_stdout->pid = datos_stdout->pcb->pid;
        pid_stdout->cantidad_paginas = datos_stdout->cantidad_paginas;
        pid_stdout->lista_direcciones = datos_stdout->lista_direcciones;
        pid_stdout->registro_tamanio = datos_stdout->registro_tamanio;
        pid_stdout->nombre_interfaz = malloc(string_length(io->nombreInterfaz)); //FREE?
        pid_stdout->nombre_interfaz = io->nombreInterfaz;
        printf("\nLa io conectada tiene nombre %s\n\n", io->nombreInterfaz);
        pid_stdout->largo_interfaz = string_length(io->nombreInterfaz) + 1;

        int respuesta_ok = ejecutar_io_stdout(pid_stdout);

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
                pasar_a_ready(datos_stdout->pcb);
            }
        } else {
            printf("No se pudo ejecutar la IO\n");
            break;
        }
    }

    free(io->semaforo_cola_procesos_blocked);
    free(io->nombreInterfaz);
    free(io);
    liberar_paquete(paquete);
    return NULL;
}

int ejecutar_io_stdout(t_pid_stdout* pid_stdout) {
    // Mandar todo a memoria
    printf("Voy a ejecutar stdout!\n");
    printf("\n\nEL PID QUE SE VA A MANDAR A MEMORIA ES: %d\n\n", pid_stdout->pid);

    t_buffer* buffer = malloc(sizeof(t_buffer)); 

    buffer->size = sizeof(int) * 3 + sizeof(uint32_t) + pid_stdout->cantidad_paginas * 2 * sizeof(int) + pid_stdout->largo_interfaz; 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid_stdout->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_stdout->registro_tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &pid_stdout->cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    printf("\nEl largo de la interfaz es: %d\n", pid_stdout->largo_interfaz);
    memcpy(buffer->stream + buffer->offset, &pid_stdout->largo_interfaz, sizeof(int)); 
    buffer->offset += sizeof(int);
    printf("\n\nLa interfaz a mandar es: %s\n\n", pid_stdout->nombre_interfaz);
    memcpy(buffer->stream + buffer->offset, pid_stdout->nombre_interfaz, pid_stdout->largo_interfaz);
    buffer->offset += pid_stdout->largo_interfaz;
    
    for(int i=0; i < pid_stdout->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pid_stdout->lista_direcciones, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }
    
    enviar_paquete(buffer, ESCRIBIR_STDOUT, sockets->socket_memoria); //ver socket

    int resultOk;

    recv(sockets->socket_memoria, &resultOk, sizeof(int), MSG_WAITALL);

    return resultOk;
}

void* handle_io_dialfs(void* socket_io) {
    int socket = (intptr_t)socket_io;

    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;

    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            printf("Vamos a establecer conexion con dialfs!!!\n");
            io = establecer_conexion(paquete->buffer, socket);
            // free(io);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    while(true) {
        // compl, no es espera activa
    }

    liberar_paquete(paquete);
    return NULL;
}

void *handle_io_generica(void *socket_io) {
    int socket = (intptr_t)socket_io;
    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;

    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            printf("Vamos a establecer conexion generica!!!\n");
            io = establecer_conexion(paquete->buffer, socket);
            // free(io);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    t_pid_unidades_trabajo* pid_unidades_trabajo = malloc(sizeof(t_pid_unidades_trabajo));

    while (true) {
        printf("Esperando semaforo\n");
        
        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");

        pthread_mutex_lock(&mutex_cola_io_generica);
        io_gen_sleep *datos_sleep = list_remove(io->cola_blocked, 0);
        pthread_mutex_unlock(&mutex_cola_io_generica);
        
        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE
        pid_unidades_trabajo->pid = datos_sleep->pcb->pid;
        pid_unidades_trabajo->unidades_trabajo = datos_sleep->unidad_trabajo;
        printf("La cola nos saca esto: PID %d, UT %d\n", datos_sleep->pcb->pid, datos_sleep->unidad_trabajo);

        int respuesta_ok = ejecutar_io_generica(io->socket, pid_unidades_trabajo);
        
        if (!respuesta_ok) {
            printf("Se ejecuto correctamente la IO... que le mandamos el pid %d\n", pid_unidades_trabajo->pid);
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
    // liberarIOyPaquete(paquete, io);
    liberar_paquete(paquete);
    
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
    printf("Nombre Interfaz que queremos: %s\n", nombre_interfaz);
    printf("Nombre Interfaz de Dictionary: %s\n", interfaz->nombreInterfaz);
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
    t_list_io *io = malloc(sizeof(t_list_io)); //FREE?
    t_info_io *interfaz = deserializar_interfaz(buffer); 
    io->nombreInterfaz = malloc(string_length(interfaz->nombre_interfaz));
    printf("\n\nllego hasta establecer conexion\n\n");

    io->socket         = socket_io;
    io->TipoInterfaz   = interfaz->tipo;
    // io->nombreInterfaz = interfaz->nombre_interfaz;
    io->cola_blocked   = list_create();
    
    // Esta memoria la liberamos cuando la io se desconecte
    io->semaforo_cola_procesos_blocked = malloc(sizeof(sem_t)); //FREE?
    sem_init(io->semaforo_cola_procesos_blocked, 0, 0);
    
    io->nombreInterfaz = strdup(interfaz->nombre_interfaz); // Asignación del nombre
    
    dictionary_put(diccionario_io, interfaz->nombre_interfaz, (void*)io);
    printf("\n\n el nombre de la interfaz es: %s\n\n", interfaz->nombre_interfaz);
    mostrar_elem_diccionario(interfaz->nombre_interfaz);

    free(interfaz->nombre_interfaz);
    free(interfaz);
    
    return io;
}

/* 

case IO_STDIN_READ: 

        t_list* lista_bytes_stdin = list_create();
        t_list* lista_direcciones_fisicas_stdin = list_create();

        t_parametro* interfaz = list_get(list_parametros,0);
        registro_direccion = list_get(list_parametros, 1); // Cambiar, usa el de arriba, mejor nombre?
        t_parametro *registro_tamanio = list_get(list_parametros, 2);

        char* nombre_registro_direccion = registro_direccion->nombre;
        char* nombre_registro_tamanio = registro_tamanio->nombre;

        uint32_t* registro_direccion_stdin = (uint32_t*) seleccionar_registro_cpu(nombre_registro_direccion);
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_tamanio);

        uint32_t* registro_tamanio_stdin = (uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio);
        
        int pagina = floor(*registro_direccion_stdin / tamanio_pagina);
        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato); //Ojo que abajo no le paso el tam_byte, sino la cantidad que tiene dentro

        int cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion_stdin, tamanio_en_byte, pagina, lista_bytes_stdin); // Cantidad de paginas + la primera
        
        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_stdin, *registro_direccion_stdin, pcb->pid, lista_direcciones_fisicas_stdin, pagina);

        t_buffer* buffer_lectura = llenar_buffer_stdio(interfaz->nombre, lista_direcciones_fisicas_stdin, *registro_tamanio_stdin, cantidad_paginas);
        
        pcb->program_counter++;
        desalojar(pcb, PEDIDO_LECTURA, buffer_lectura);

        free(buffer_lectura);
        return 1; // El return esta para que cuando se desaloje el pcb no siga ejecutando, si hace el break sigue pidiendo instrucciones, porfa no lo saquen o les va a romper
        break;

*/
