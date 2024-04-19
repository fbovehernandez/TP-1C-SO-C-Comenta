#ifndef CONEXIONES_H
#define CONEXIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

int conectar_kernel_memoria(char* ip, char* puerto, t_log* logger_kernel);
int conectar_kernel_cpu_dispatch(t_log* logger_kernel, char* ip, char* puerto);
int esperar_cliente(int socket_escucha, t_log* logger);
void* handle_io(void* socket);
int recibir_operacion(int socket_cliente);
void liberar_conexion(int socket_cliente);
// void escuchar_STDOUT(t_config* config_kernel, t_log* logger_kernel);

#endif // CONEXIONES_H