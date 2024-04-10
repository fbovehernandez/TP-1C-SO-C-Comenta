#ifndef LOG_CONFIG_H
#define LOG_CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "../../utils/include/sockets.h" // No pude hacerlo de otra manera

t_config* iniciar_config(char* path);
t_log* iniciar_logger(char* path);
void terminar_programa(int conexion, t_log* logger, t_config* config);

#endif // LOG_CONFIG_H