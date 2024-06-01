#include "../include/conexiones.h"

t_dictionary* diccionario_io;
t_list* lista_io;
sem_t* sem_cola_io;
sem_t mutex_lista_io;

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

    recv(socket_cliente, &handshake, sizeof(int), 0);
    if(handshake == 5) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io_generica, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        close(socket_cliente);
        return -1;
    }

    return socket_cliente;
}

    // Habria que ver de scar logica
void handle_STD_IN(void* socket) {

}

void* handle_io_generica(void* socket) {
    int socket_io = (intptr_t) socket;
    int resultOk = 0;
    send(socket_io, &resultOk, sizeof(int), 0);
    printf("Conexion establecida con I/O\n");

    // Recibo el cliente i/o, con todos sus datos
    t_queue* cola_io = queue_create();
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
            io->nombreInterfaz = malloc(interfaz->nombre_interfaz_largo);

            io->socket = socket_io;
            strcpy(io->nombreInterfaz, interfaz->nombre_interfaz);
            io->tipoInterfaz = interfaz->tipo;
            io->cola_blocked = cola_io;
            io->semaforo_cola_procesos_blocked = sem_cola_io;
            
            // Ver que sea asi -> agrego a la lista global de io
            sem_wait(&mutex_lista_io);
            list_add(lista_io, io);
            sem_post(&mutex_lista_io);

            break;
        default:
            printf("Llega al default.");
            return NULL;
    }
        
    while(true) {
        sem_wait(io->semaforo_cola_procesos_blocked);
        t_pcb* pcb = queue_pop(io->cola_procesos_blocked);
        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola)
        
        int respuesta_ok = ejecutar_io(io->socket);
        if (respuesta_ok) {
            printf("Se ejecuto correctamente la IO\n");
            pcb->estadoActual = READY;
            printf("Pasar a ready\n");
            // pasar_a_ready(); // poner funcion en socket.h
        } else {
            printf("No se pudo ejecutar la IO\n");
            break;
        }
    }

        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
        return NULL;

}

int ejecutar_io(int socket_io) {
    // Armado del paquete para enviar a io y que duerma
    int unidades_trabajo;
    t_buffer* buffer = malloc(sizeof(t_buffer)); // Creo que no hace falta reservar memoria
    buffer->size = sizeof(int);

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
    
    memcpy(buffer->stream + buffer->offset, &unidades_trabajo, sizeof(int));

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = DORMITE; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    // hay que poner el socket kernel globallll
    send(socket_io, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    printf("Paquete enviado!");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
    return 0;
}

t_info_io* deserializar_interfaz(t_buffer* buffer) {
    t_info_io* interfaz = malloc(sizeof(t_info_io));

    printf("Deserializando interfaz\n");

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(interfaz->nombre_interfaz_largo), stream, sizeof(int));
    stream += sizeof(int);

    printf("Este es el largo que deserializa: %d\n", interfaz->nombre_interfaz_largo);
    
    memcpy(&(interfaz->tipo), stream, sizeof(TipoInterfaz));
    stream += sizeof(TipoInterfaz);

    interfaz->nombre_interfaz = malloc(interfaz->nombre_interfaz_largo);
    memcpy(interfaz->nombre_interfaz, stream, interfaz->nombre_interfaz_largo);  

    printf("Este es el nombre interfaz que deserializa: %s\n", interfaz->nombre_interfaz);

    return interfaz;
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

/* 
void mostrar_elem_diccionario(char* nombre_interfaz) {
    t_conjuntoInterfaz* interfaz = dictionary_get(diccionario_io, nombre_interfaz);
    printf("Nombre Interfaz: %s\n", nombre_interfaz);
    printf("Tipo Interfaz (enum): %d\n", interfaz->tipoInterfaz);
    printf("Socket: %d\n", interfaz->socket);
}*/