#include "../include/conexiones.h"

/* 
PUERTO_ESCUCHA_KERNEL_CPU=8010
IP_CPU=127.0.0.1
PUERTO_CPU_DISPATCH=8006
PUERTO_CPU_INTERRUPT=8007
IP_MEMORIA=127.0.0.1
PUERTO_ESCUCHA_KERNEL_MEMORIA=8009
PUERTO_MEMORIA=8002
ALGORITMO_PLANIFICACION=VRR
QUANTUM=2000
RECURSOS=[RA,RB,RC]
INSTANCIAS_RECURSOS=[1,2,1]
GRADO_MULTIPROGRAMACION=10
*/


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
    send(dispatcherfd, &message, sizeof(int), 0);
    return dispatcherfd;
}

int conectar_kernel_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_kernel) {
    int message_kernel = 7;
    int valor = 1;

    int memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria, valor);
    log_info(logger_kernel, "Conexion establecida con Memoria");

    send(memoriafd, &message_kernel, sizeof(int), 0); 

    // close(memoriafd); // (POSIBLE) no cierro el socket porque quiero reutilizar la conexion
    return memoriafd;
}

void* escuchar_IO(void* kernel_io) {
    t_config_kernel* kernel_struct_io = (t_config_kernel*) kernel_io;
    int socket_io = kernel_struct_io->socket;
    t_log* logger_io = kernel_struct_io->logger;

    while(1) {
        int socket_cliente = esperar_cliente(socket_io, logger_io);
    }
    
    return NULL;
}

