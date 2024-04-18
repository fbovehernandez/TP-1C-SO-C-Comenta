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
            printf("Soy el I/O y recibi un mensaje\n");
            break;
        default:
            printf("Llega al default.");
            return NULL;
        }
    }
    return NULL;
}

// Puede ir al utils/src/sockets.c
int recibir_operacion(int socket_client) {
    int cod_op;
    if(recv(socket_client, &cod_op, sizeof(int), MSG_WAITALL) != 0) {
        return cod_op;
    } else {
        close(socket_client);
        return -1;
    }
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
    // char* IP_MEMORIA = config_get_string_value(config_kernel, "IP_MEMORIA");
    // char* puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
    int message_kernel = 7;
    int valor = 1;

    int memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria, valor);
    log_info(logger_kernel, "Conexion establecida con Memoria");

    send(memoriafd, &message_kernel, sizeof(int), 0); 

    // close(memoriafd); // (POSIBLE) no cierro el socket porque quiero reutilizar la conexion
    return memoriafd;
}

/*
void handshake(int socket) {
    send(socket, 1, 6, 0); // len("kernel")
    recv(socket, buffer, sizeof(buffer), 0);
    printf("Handshake: %s\n", buffer);
    free(buffer);
} */

/*int server_escuchar(t_log* logger, char* server_name, int server_socket) {
    int cliente_socket = esperar_cliente(logger, server_name, server_socket);

    if (cliente_socket != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = server_name;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}
*/
/* 
void escuchar_conexiones(t_config* config_kernel, t_log* logger_kernel) {
    char* escuchar_io = config_get_string_value(config_kernel, "PUERTO_IO"); // 8009
    int kernelfd = iniciar_servidor(escuchar_io);
    log_info(logger_kernel, "Servidor iniciado, esperando conexion de I/O stdout");
    printf("Escucha I/O: %s\n", escuchar_io);
    int iostdout_fd = esperar_conexiones_io(kernelfd);

    receiveMessagex(iostdout_fd);
    close(iostdout_fd);
}
*/

/* 

typedef struct {
    t_log* log;
    int fd;
    char* server_name;
} t_procesar_conexion_args;

static void procesar_conexion_cliente(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
    t_log* logger = args->log;
    int cliente_socket = args->fd;
    char* server_name = args->server_name;
    free(args);
    op_code cop;
    while (cliente_socket != -1) {

        if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            log_info(logger, "Desconectado!");
            return;
        }
        switch (cop) {
            case DEBUG:
                log_info(logger, "debug");
                break;

            case INICIAR_PLANIFICACION:
            {
                // LOGICA DE INICIAR_PLANIFICACION
            }
            case DETENER_PLANIFICACION:
            {

            }

            // Errores
            case -1:
                log_error(logger, "Cliente desconectado de %s...", server_name);
                return;
            default:
                log_error(logger, "Algo anduvo mal en el server de %s", server_name);
                return;
        }
    }

    log_warning(logger, "El cliente se desconecto de %s server", server_name);
    return;
    */
// }

