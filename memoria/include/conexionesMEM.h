#ifndef CONEXIONESMEM_H
#define CONEXIONESMEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

extern pthread_t kernel_thread;
extern pthread_t cpu_thread;
typedef struct {
    int socket;
    t_log* logger;
} t_config_memoria;

int recibir_operacion(int socket_cliente);
void* handle_kernel(void* socket);
void* handle_cpu(void* socket);
void liberar_conexion(int socket_cliente);
int esperar_cliente(int, t_log*);
int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io);
void imprimir_path(t_path* path);
t_path* deserializar_path(t_buffer* buffer);

#endif // CONEXIONES_H