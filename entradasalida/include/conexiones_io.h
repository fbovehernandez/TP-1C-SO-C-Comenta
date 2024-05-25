#ifndef CONEXIONES_IO_H
#define CONEXIONES_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

extern int socket_kernel_io;

int conectar_io_kernel(char* ip, char* puerto, t_log* logger);

#endif // CONEXIONES_IO_H