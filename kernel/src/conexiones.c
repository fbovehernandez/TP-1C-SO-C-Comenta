#include "../include/conexiones.h"

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
    int socket_cliente = accept(socket_escucha, NULL,  NULL);

    if(socket_cliente == -1) {
        return -1;
    }

    recv(socket_cliente, &handshake, sizeof(int), 0);
    if(handshake == 5) {
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*) handle_io, (void*)(intptr_t) socket_cliente);
        pthread_detach(hilo_io);
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        close(socket_cliente);
        return -1;
    }

    return socket_cliente;
}

void* handle_io(void* socket) {
    int socket_io = (intptr_t) socket;
    int resultOk = 0;
    send(socket_io, &resultOk, sizeof(int), 0);
    printf("Conexion establecida con I/O\n");

    int cod_op;
    while(socket_io != -1) {
        cod_op = recibir_operacion(socket_io);
        switch (cod_op) {
        case 12:
            printf("Soy el kernel y recibi un mensaje de I/0\n");
            break;
        default:
            printf("Llega al default.");
            return NULL;
        }
    }
    return NULL;
}

int conectar_kernel_cpu_dispatch(t_log* logger_kernel, char* IP_CPU, char* puerto_cpu_dispatch) {
    int valor = 1;
    int message = 10;
    int dispatcherfd = crear_conexion(IP_CPU, puerto_cpu_dispatch, valor);
    log_info(logger_kernel, "Conexion establecida con Dispatcher");
    // send(dispatcherfd, &message, sizeof(int), 0);
    return dispatcherfd;
}

int conectar_kernel_memoria(char* ip_memoria, char* puerto_memoria, t_log* logger_kernel) {
    int message_kernel = 7;
    int valor = 1;

    int memoriafd = crear_conexion(ip_memoria, puerto_memoria, valor);
    log_info(logger_kernel, "Conexion establecida con Memoria");

    // send(memoriafd, &message_kernel, sizeof(int), 0); 

    // close(memoriafd); // (POSIBLE) no cierro el socket porque quiero reutilizar la conexion
    return memoriafd;
}

void* escuchar_IO(void* kernel_io) {
    t_config_kernel* kernel_struct_io = (void*) kernel_io;

    // int socket_io = kernel_struct_io->socket;
    // t_log* logger_io = kernel_struct_io->logger;

    while(1) {
        int socket_cliente = esperar_cliente(kernel_struct_io->socket, kernel_struct_io->logger);
    }
    
    return NULL;
}

