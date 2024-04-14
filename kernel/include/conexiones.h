#ifndef CONEXIONES_H
#define CONEXIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

void conectar_kernel_cpu_dispatch(t_config* config_kernel, t_log* logger_kernel, char* ip);
void conectar_kernel_cpu_interrupt(t_config* config_kernel, t_log* logger_kernel, char* ip);
int conectar_kernel_memoria(char* ip, char* puerto, t_log* logger_kernel);
// void escuchar_STDOUT(t_config* config_kernel, t_log* logger_kernel);

#endif // CONEXIONES_H