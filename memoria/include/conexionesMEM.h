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

void* manejar_conexion_kernel(void* config_memoria);
void* manejar_conexion_cpu(void* config_memoria);

#endif // CONEXIONES_H