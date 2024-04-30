#ifndef CONEXIONES_H
#define CONEXIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

// Aca va el struct del PCB 
typedef struct {
    char* puerto_escucha;
    int socket;
    t_log* logger;
} t_config_kernel;

typedef struct {
    char* ip_cpu;
    char* ip_mem;
    char* puerto_memoria;
    char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;
    char* puerto_io;
    int quantum;
    int grado_multiprogramacion;   
    char* algoritmo_planificacion;
} ptr_kernel;

int conectar_kernel_memoria(char* ip, char* puerto, t_log* logger_kernel);
int conectar_kernel_cpu_dispatch(t_log* logger_kernel, char* ip, char* puerto);
int esperar_cliente(int socket_escucha, t_log* logger);
void* handle_io(void* socket);
int recibir_operacion(int socket_cliente);
void liberar_conexion(int socket_cliente);
// void escuchar_STDOUT(t_config* config_kernel, t_log* logger_kernel);
void* escuchar_IO(void* kernel_io);
ptr_kernel* solicitar_datos(t_config* config_kernel);

#endif // CONEXIONES_H