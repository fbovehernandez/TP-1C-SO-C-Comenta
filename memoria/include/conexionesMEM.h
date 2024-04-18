#ifndef CONEXIONESMEM_H
#define CONEXIONESMEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

typedef struct {
    int socket;
    t_log* logger;
} t_config_memoria;

int recibir_operacion(int socket_cliente);
void* handle_kernel(void* socket);
void* handle_cpu(void* socket);
void liberar_conexion(int socket_cliente);
int esperar_cliente(int, t_log*);

#endif // CONEXIONES_H