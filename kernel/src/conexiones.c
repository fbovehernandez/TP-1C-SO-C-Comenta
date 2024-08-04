#include "../include/conexiones.h"

t_dictionary* diccionario_io;
t_list* lista_procesos;
t_list* lista_io;
sem_t sem_cola_io;
pthread_mutex_t mutex_lista_io;
pthread_mutex_t mutex_cola_io_generica;
pthread_mutex_t mutex_cola_io_stdout;
pthread_mutex_t mutex_cola_io_stdin;
t_sockets* sockets;

pthread_mutex_t mutex_estado_new;
pthread_mutex_t mutex_estado_ready;
pthread_mutex_t mutex_estado_exec;
pthread_mutex_t mutex_cola_fs;
pthread_mutex_t no_hay_nadie_en_cpu;
pthread_mutex_t mutex_estado_blocked;
pthread_mutex_t mutex_estado_ready_plus;
pthread_mutex_t mutex_prioritario_por_signal;

t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_blocked;
t_queue* cola_exec;
// t_pcb* pcb_exec;
t_queue* cola_prioritarios_por_signal;
t_queue* cola_ready_plus;

///////////////////
///// GENERAL /////
///////////////////

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
        t_recurso* recurso = malloc(sizeof(t_recurso));
        recurso->instancias = atoi(instancias_recursos[i]);
        recurso->procesos_bloqueados = list_create();
        recurso->procesos_que_lo_retienen = list_create();
        dictionary_put(datos->diccionario_recursos, recursos[i], recurso); //Mas tarde para sumar o restar deberemos castear a int instancias_recursos[i]
    }
    /* // REVISAR
    for(int i=0; i<string_array_size(recursos); i++){
        printf("Recurso: %s\n", recursos[i]);
        printf("Diccionario en instancias recursos: %d\n", ((t_recurso*)(dictionary_get(datos->diccionario_recursos, recursos[i])))->instancias);  
    }
    */
    liberar_arrays_recurso(recursos, instancias_recursos);

    return datos;
}

int esperar_cliente(int socket_escucha, t_log* logger) {
    int handshake = 0;
    int resultError = -1;
    int resultOk = 0;
        
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
        pthread_create(&hilo_io, NULL, (void*) handle_io_generica, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else if( handshake == 13) {
        pthread_t hilo_io;
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
        printf("El handshake es invalido\n");
        close(socket_cliente);
        return -1;
    }

    return socket_cliente;
}

int conectar_kernel_cpu_interrupt(t_log* logger_kernel, char* IP_CPU, char* puerto_interrupt) {
    int valor = 3;
    int interruptfd = crear_conexion(IP_CPU, puerto_interrupt, valor);
    printf("Conexion establecida con Interrupt\n");
    // send(dispatcherfd, &message, sizeof(int), 0);
    return interruptfd;   
}

int conectar_kernel_cpu_dispatch(t_log* logger_kernel, char* IP_CPU, char* puerto_cpu_dispatch) {
    int valor = 1;
    // int message = 10;
    int dispatcherfd = crear_conexion(IP_CPU, puerto_cpu_dispatch, valor);
    printf("Conexion establecida con Dispatcher\n");
    // send(dispatcherfd, &message, sizeof(int), 0);
    return dispatcherfd;
}

int conectar_kernel_memoria(char* ip_memoria, char* puerto_memoria, t_log* logger_kernel) {
    int valor = 1;

    int memoriafd = crear_conexion(ip_memoria, puerto_memoria, valor);
    printf("Conexion establecida con Memoria\n");

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

t_list_io* establecer_conexion(t_buffer *buffer, int socket_io) {
    t_list_io *io = malloc(sizeof(t_list_io)); //FREE? PARA LO ULTIMO
    t_info_io *interfaz = deserializar_interfaz(buffer); 
    // io->nombreInterfaz = malloc(string_length(interfaz->nombre_interfaz));

    io->socket         = socket_io;
    io->TipoInterfaz   = interfaz->tipo;
    io->cola_blocked   = list_create();
    
    // Esta memoria la liberamos cuando la io se desconecte
    io->semaforo_cola_procesos_blocked = malloc(sizeof(sem_t));
    sem_init(io->semaforo_cola_procesos_blocked, 0, 0);
    
    io->nombreInterfaz = strdup(interfaz->nombre_interfaz); // Asignación del nombre
    
    dictionary_put(diccionario_io, interfaz->nombre_interfaz, (void*)io);
    printf("\n el nombre de la interfaz conectada es: %s\n\n", interfaz->nombre_interfaz);
    mostrar_elem_diccionario(interfaz->nombre_interfaz);

    free(interfaz->nombre_interfaz);
    free(interfaz);
    
    return io;
}

/////////////////////////
///// PLANIFICACION /////
/////////////////////////

t_pcb* sacarDe(t_queue* cola, int pid){
    t_pcb* pcb;
    t_queue* colaAux = queue_create();
    
    pthread_mutex_t* mutex = obtener_mutex_de(cola);

    pthread_mutex_lock(mutex);
    while(!queue_is_empty(cola)){
        t_pcb* pcbAux = queue_pop(cola);
        if(pcbAux->pid != pid){
            queue_push(colaAux,pcbAux);
        } else {
            pcb = pcbAux;
        }
    }

    while(!queue_is_empty(colaAux)){
        queue_push(cola, queue_pop(colaAux));
    }
    pthread_mutex_unlock(mutex);

    queue_destroy(colaAux);

    return pcb;
}

pthread_mutex_t* obtener_mutex_de(t_queue* cola){
    if(cola == cola_exec){
        return &mutex_estado_exec;
    }
    if(cola == cola_new) {
        return &mutex_estado_new;
    }
    else if(cola == cola_blocked)  {
        return &mutex_estado_blocked;
    }
    else if(cola == cola_ready) {
        return &mutex_estado_ready;
    }
    else if(cola == cola_prioritarios_por_signal){
        return &mutex_prioritario_por_signal;
    }
    else if(cola == cola_ready_plus){
        return &mutex_estado_ready_plus;
    } 
    else {
        printf("Error cola invalida");
        exit(1);
    }
}

// Habria que ver de scar logica

void pasar_a_exit_procesos_bloqueados(t_list* bloqueados) {
    t_list* IOs = dictionary_elements(diccionario_io);

    for(int i=0; i<dictionary_size(diccionario_io); i++) {
        t_list_io* IO = list_get(IOs, i);

        for(int j=0; j<list_size(IO->cola_blocked); j++) {
            int pid_obtenido;

            if(IO->TipoInterfaz == GENERICA) {
                io_gen_sleep* dato_sleep = list_get(IO->cola_blocked, j);
                pid_obtenido = dato_sleep->pcb->pid;
            } else if(IO->TipoInterfaz == STDIN || IO->TipoInterfaz == STDOUT) {
                io_std* dato_std = list_get(IO->cola_blocked, j);
                pid_obtenido = dato_std->pcb->pid;
            } else {
                datos_operacion* dato_fs = list_get(IO->cola_blocked, j);
                pid_obtenido = dato_fs->pcb->pid;
            }

            t_pcb* pcb = sacarDe(cola_blocked, pid_obtenido);
            pasar_a_exit(pcb, "DESCONEXION_IO");
            // Aca se considera liberar todo los io_gen_sleep, io_std y datos_operacion.
        } 

        free(IO->nombreInterfaz);
        free(IO);
    }
    list_destroy(IOs);
}

/////////////////
///// STDIN /////
/////////////////

void *handle_io_stdin(void *socket_io) {
    int respuesta_conexion;
    int validacion_conexion = 1; 
    int socket = (intptr_t)socket_io;
    int termino_io;

    // Por que manda esto
    // int result = 0;
    // send(socket, &result, sizeof(int), 0);
    
    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    
    printf("Se conecto una io stdin\n");

    t_list_io* io;

    printf("codigo de op: %d\n", paquete->codigo_operacion);
    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ: 
            io = establecer_conexion(paquete->buffer, socket);
            break;
        default:
            printf("Llega al default en handle_io_stdin.\n");
            return NULL;
    }

    while (true) {
        int size_dictionary = dictionary_size(diccionario_io);

        sem_wait(io->semaforo_cola_procesos_blocked);

        t_pid_stdin* pid_stdin = malloc(sizeof(t_pid_stdin));
        
        // io_std *datos_stdin = malloc(sizeof(io_std)); -> No hace falta "aparentemente"
        io_std* datos_stdin; 
        
        pthread_mutex_lock(&mutex_cola_io_generica);

        if(list_is_empty(io->cola_blocked)) {
            printf("No hay procesos en la cola de bloqueados de la IO\n");
            pthread_mutex_unlock(&mutex_cola_io_generica);
        } else {
            datos_stdin = list_remove(io->cola_blocked, 0); // FREE?
            pthread_mutex_unlock(&mutex_cola_io_generica);
            
            // pasar_a_blocked(datos_stdin->pcb);

            // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE
            
            /*send(socket, &validacion_conexion, sizeof(int), 0);
            int result = recv(socket, &respuesta_conexion, sizeof(int), 0);
            printf("resultado recv: %d\n", result);

            if(result == 0) {
                printf("Se desconecto la IO\n");
                sacarDe(cola_blocked, datos_stdin->pcb->pid);
                pasar_a_exit(datos_stdin->pcb, "IO_DISCONNECTED");
                t_list_io* interfaz_a_liberar = dictionary_remove(diccionario_io, io->nombreInterfaz);

                // Ojo! Aca tambien hay que liberar todo lo que no se usa cuando la io se termina
                liberar_io(interfaz_a_liberar);
                free(pid_stdin);        
                liberar_datos_std(datos_stdin);
                liberar_paquete(paquete);
                
                return NULL;
            }*/

            pid_stdin->pid = datos_stdin->pcb->pid;
            pid_stdin->cantidad_paginas = datos_stdin->cantidad_paginas;
            pid_stdin->lista_direcciones = datos_stdin->lista_direcciones;
            pid_stdin->registro_tamanio = datos_stdin->registro_tamanio;

            // mandar la io a memoria
            
            int respuesta_ok = ejecutar_io_stdin(socket, pid_stdin);

            if (!respuesta_ok) {
                printf("Se ejecuto correctamente la IO...\n");
            
                // printf("Estado pcb: %d\n", datos_sleep->pcb->estadoActual);
                // datos_sleep->pcb->estadoActual = READY;
                // printf("Estado pcb : %d\n", datos_sleep->pcb->estadoActual);
                
                recv(socket, &termino_io, sizeof(int), MSG_WAITALL);
                
                // printf("Termino io: %d\n", termino_io);
                if (termino_io == 1) { // El send de termino io envia 1.
                    printf("Termino la IO\n");

                    // Esto creo que no esta bien, pero no se como liberarlo...
                    // t_pcb* pcb = datos_stdin->pcb;
                    t_pcb* pcb = sacarDe(cola_blocked, datos_stdin->pcb->pid);
                 
                    sem_wait(&sem_planificadores);
                    pasar_a_ready(pcb); 
                    sem_post(&sem_planificadores);
                  
                    
                    // free(datos_stdin->pcb->registros);
                    // free(datos_stdin->pcb);
                    // free(datos_stdin);
                }
            } else {
                printf("No se pudo ejecutar la IO\n");
                break;
            }

            // liberar_datos_std(datos_stdin);
            list_destroy(datos_stdin->lista_direcciones);

            // free(datos_std->pcb); -> No lo libero porque es el mismo que le hacemos malloc cuando desalojamos...
            free(datos_stdin);
            free(pid_stdin);
            // list_destroy(pid_stdin->lista_direcciones);
        }
    }
    
    // free(pid_stdin);
    liberar_paquete(paquete);
    return NULL;
}

int ejecutar_io_stdin(int socket, t_pid_stdin* pid_stdin) {
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

    for(int i=0; i < pid_stdin->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pid_stdin->lista_direcciones, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
        free(dir_fisica_tam);
    }
    
    //
    enviar_paquete(buffer, LEETE, socket);

    int resultOk;

    recv(socket, &resultOk, sizeof(int), MSG_WAITALL);
    
    return resultOk; // Ojo que siempre retorna 0 y por ende ejecuta bien
}

//////////////////
///// STDOUT /////
//////////////////

void* handle_io_stdout(void* socket_io) {
    int respuesta_conexion;
    int validacion_conexion = 1;

    int socket = (intptr_t)socket_io;
    
    /*int result = 0;
    send(socket, &result, sizeof(int), 0);*/

    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;

    printf("Se conecto una io stdout\n");
    
    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            io = establecer_conexion(paquete->buffer, socket);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    while (true) {
        sem_wait(io->semaforo_cola_procesos_blocked);
        
        // io_std *datos_stdout = malloc(sizeof(io_std)); //FREE?
        io_std *datos_stdout;

        pthread_mutex_lock(&mutex_cola_io_generica);
        if(list_is_empty(io->cola_blocked)) {
            printf("No hay procesos en la cola de bloqueados de la IO\n");
        } else {
            datos_stdout = list_remove(io->cola_blocked, 0);
        }
        pthread_mutex_unlock(&mutex_cola_io_generica);

        /*send(sockets->socket_memoria, &validacion_conexion, sizeof(int), 0);
        int result = recv(sockets->socket_memoria, &respuesta_conexion, sizeof(int), 0);
        printf("resultado recv: %d\n", result);*/

        /*if(result == 0) {
            printf("Se desconecto la IO\n");
            sacarDe(cola_blocked, datos_stdout->pcb->pid);
            pasar_a_exit(datos_stdout->pcb, "IO_DISCONNECTED");
            t_list_io* interfaz_a_liberar = dictionary_remove(diccionario_io, io->nombreInterfaz);

            // Ojo! Aca tambien hay que liberar todo lo que no se usa cuando la io se termina
            liberar_io(interfaz_a_liberar);

            liberar_datos_std(datos_stdout);
            liberar_paquete(paquete);
            
            return NULL;
        }*/

        t_pid_stdout* pid_stdout = malloc(sizeof(t_pid_stdout));
        
        pid_stdout->pid = datos_stdout->pcb->pid;
        pid_stdout->cantidad_paginas = datos_stdout->cantidad_paginas;
        pid_stdout->lista_direcciones = datos_stdout->lista_direcciones;
        pid_stdout->registro_tamanio = datos_stdout->registro_tamanio;
        // pid_stdout->nombre_interfaz = malloc(string_length(io->nombreInterfaz));
        pid_stdout->nombre_interfaz = io->nombreInterfaz;
        pid_stdout->largo_interfaz = string_length(io->nombreInterfaz) + 1;
        
        int respuesta_ok = ejecutar_io_stdout(pid_stdout);

        if (!respuesta_ok) {
            printf("Se ejecuto correctamente la IO...\n");
        
            // printf("Estado pcb: %d\n", datos_sleep->pcb->estadoActual);
            // datos_sleep->pcb->estadoActual = READY;
            // printf("Estado pcb : %d\n", datos_sleep->pcb->estadoActual);
            
            int termino_io;
            recv(socket, &termino_io, sizeof(int), MSG_WAITALL);

            // printf("Termino io: %d\n", termino_io);

            if (termino_io == 1) { // El send de termino io envia 1.                printf("Termino la IO\n");
                sem_wait(&sem_planificadores);
                t_pcb* pcb = sacarDe(cola_blocked, datos_stdout->pcb->pid);
                pasar_a_ready(pcb); 
                sem_post(&sem_planificadores);
               
            }
        } else {
            printf("No se pudo ejecutar la IO\n");
            // La IO se desconecto.
            printf("Se desconecto la IO\n");
            sacarDe(cola_blocked, datos_stdout->pcb->pid);
            pasar_a_exit(datos_stdout->pcb, "IO_DISCONNECTED");
            t_list_io* interfaz_a_liberar = dictionary_remove(diccionario_io, io->nombreInterfaz);

            // Ojo! Aca tambien hay que liberar todo lo que no se usa cuando la io se termina
            liberar_io(interfaz_a_liberar);

            // liberar_datos_std(datos_stdout);
            liberar_paquete(paquete);
                
            return NULL;
            break;
        }

        // liberar_datos_std(datos_stdout);
        list_destroy(datos_stdout->lista_direcciones);

        // free(datos_std->pcb); -> No lo libero porque es el mismo que le hacemos malloc cuando desalojamos...
        free(datos_stdout);
        // free(pid_stdout->nombre_interfaz);
        free(pid_stdout);
    }

    free(io->semaforo_cola_procesos_blocked);
    free(io->nombreInterfaz);
    free(io);
    // free(pid_stdout); // QUE_NO_ROMPA

    liberar_paquete(paquete);
    return NULL;
}

int ejecutar_io_stdout(t_pid_stdout* pid_stdout) {
    // Mandar todo a memoria
    
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
    memcpy(buffer->stream + buffer->offset, &pid_stdout->largo_interfaz, sizeof(int)); 
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, pid_stdout->nombre_interfaz, pid_stdout->largo_interfaz);
    buffer->offset += pid_stdout->largo_interfaz;
    // free(pid_stdout->nombre_interfaz);
    
    for(int i=0; i < pid_stdout->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pid_stdout->lista_direcciones, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
        free(dir_fisica_tam);
    }
    
    // list_destroy(pid_stdout->lista_direcciones);
    // free(pid_stdout); lo voy a poner arriba
    
    enviar_paquete(buffer, ESCRIBIR_STDOUT, sockets->socket_memoria); //ver socket

    int resultOk;

    recv(sockets->socket_memoria, &resultOk, sizeof(int), MSG_WAITALL);

    return resultOk;
}

////////////////////
///// GENERICA /////
////////////////////

void *handle_io_generica(void *socket_io) {
    int respuesta_conexion;
    int validacion_conexion = 1;

    int socket = (intptr_t)socket_io;
    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;

    printf("Se conecto una io generica\n");

    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            io = establecer_conexion(paquete->buffer, socket);
            // free(io);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }
    
    while (true) {
        t_pid_unidades_trabajo* pid_unidades_trabajo = malloc(sizeof(t_pid_unidades_trabajo));
        
        sem_wait(io->semaforo_cola_procesos_blocked);

        pthread_mutex_lock(&mutex_cola_io_generica);
        io_gen_sleep *datos_sleep = list_remove(io->cola_blocked, 0);
        pthread_mutex_unlock(&mutex_cola_io_generica);
        
        pid_unidades_trabajo->pid = datos_sleep->pcb->pid;
        pid_unidades_trabajo->unidades_trabajo = datos_sleep->unidad_trabajo;
        
        // printf("LO IMPRIMO DESPUES DE SACARLO DE LA COLA\n");
        // imprimir_pcb(datos_sleep->pcb);
        
        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE
        // hay que agregar un send recv de validacion en cada io. Esto es lo que use en fs

        /*send(socket, &validacion_conexion, sizeof(int), 0);
        int result = recv(socket, &respuesta_conexion, sizeof(int), 0);
        printf("resultado recv: %d\n", result);

        if(result == 0) {
            printf("Se desconecto la IO\n");
            sacarDe(cola_blocked, datos_sleep->pcb->pid);
            pasar_a_exit(datos_sleep->pcb, "IO_DISCONNECTED");
            t_list_io* interfaz_a_liberar = dictionary_remove(diccionario_io, io->nombreInterfaz);

            // Ojo! Aca tambien hay que liberar todo lo que no se usa cuando la io se termina
            liberar_io(interfaz_a_liberar);

            free(pid_unidades_trabajo);

            free(datos_sleep);
            liberar_paquete(paquete);
            
            return NULL;
        }*/
        
        int respuesta_ok = ejecutar_io_generica(io->socket, pid_unidades_trabajo);
        
        if (!respuesta_ok) {
            printf("Se ejecuto correctamente la IO...");
            // printf("Estado pcb: %d\n", datos_sleep->pcb->estadoActual);
            // datos_sleep->pcb->estadoActual = READY;
            // printf("Estado pcb : %d\n", datos_sleep->pcb->estadoActual);
            int termino_io;
        
            recv(socket, &termino_io, sizeof(int), MSG_WAITALL);
            if (termino_io == 1) { // El send de termino io envia 1.
                printf("Termino io: %d\n", termino_io);

                // pthread_mutex_lock(&mutex_estado_blocked); 
                sem_wait(&sem_planificadores);
                t_pcb* pcb = sacarDe(cola_blocked, datos_sleep->pcb->pid);
                pasar_a_ready(pcb); 
                sem_post(&sem_planificadores);
                // pthread_mutex_unlock(&mutex_estado_blocked);
            }
        }else {
            printf("No se encontró el PCB con PID %d en la cola de bloqueados.\n", datos_sleep->pcb->pid);
        }
        
            if(datos_sleep->pcb != NULL) {
                // free(datos_sleep->pcb->registros);
                // free(datos_sleep->pcb);
                free(datos_sleep);
            }

            free(pid_unidades_trabajo);
    }   

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

////////////////////
///// LIMPIEZA /////
////////////////////

void liberar_datos_std(io_std* datos_std) {
    // Liberar una por una las direcciones
    for(int i=0; i < datos_std->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(datos_std->lista_direcciones, i);
        free(dir_fisica_tam);
    }

    list_destroy(datos_std->lista_direcciones);

    // free(datos_std->pcb); -> No lo libero porque es el mismo que le hacemos malloc cuando desalojamos...
    free(datos_std);
}

void liberar_fs_puntero(datos_operacion* dato_fs) {
    if(dato_fs->tipo_operacion == TRUNCAR_ARCHIVO) {
        // Aca lo casteo al tipo correcto y lo libero
        t_pedido_fs_truncate* fs_truncate = (t_pedido_fs_truncate*) dato_fs->puntero_operacion;
        free(fs_truncate->nombre_archivo);
        free(fs_truncate->nombre_interfaz);
        free(fs_truncate);
    } else if(dato_fs->tipo_operacion == CREAR_ARCHIVO || dato_fs->tipo_operacion == ELIMINAR_ARCHIVO) {
        t_archivo_encolar* pedido_a_encolar = (t_archivo_encolar*) dato_fs->puntero_operacion;
        free(pedido_a_encolar->nombre_archivo);
        free(pedido_a_encolar);        
    } else {
        t_pedido_rw_encolar* fs_rw = (t_pedido_rw_encolar*) dato_fs->puntero_operacion;
        // Libero todo l que tiene la lista una por una...
        for(int i=0; i<list_size(fs_rw->lista_dir_tamanio); i++) {
            t_dir_fisica_tamanio* dir_tamanio = list_get(fs_rw->lista_dir_tamanio, i);
            free(dir_tamanio);
        }

        list_destroy(fs_rw->lista_dir_tamanio);
        
        free(fs_rw->nombre_archivo);
    }
    
    free(dato_fs);
}

void liberarIOyPaquete(t_paquete *paquete, t_list_io *io) {
    liberar_io(io);
    liberar_paquete(paquete);
}

void liberar_io(t_list_io* io) {
    free(io->semaforo_cola_procesos_blocked);
    free(io->nombreInterfaz);
    close(io->socket);
    pasar_a_exit_procesos_bloqueados(io->cola_blocked);
    list_destroy(io->cola_blocked);
    free(io);
}

void liberar_arrays_recurso(char** recursos,char** instancias_recursos){
    for (int i = 0; recursos[i] != NULL; i++) {
        free(recursos[i]);
    }

    free(recursos);
    
    for (int i = 0; instancias_recursos[i] != NULL; i++) {
        free(instancias_recursos[i]);
    }
    
    free(instancias_recursos);
}

