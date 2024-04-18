#ifndef KERNEL_MAIN_H_
#define KERNEL_MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "conexiones.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

typedef struct {
    int socket;
    t_log* logger;
} t_config_kernel;

#endif // KERNEL_MAIN_H_