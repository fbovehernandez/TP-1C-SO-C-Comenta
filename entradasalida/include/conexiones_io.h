#ifndef CONEXIONES_IO_H
#define CONEXIONES_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

void conectar_kernel_io(t_config* config_io, t_log* logger_io, char* IP_IO);

#endif // CONEXIONES_IO_H