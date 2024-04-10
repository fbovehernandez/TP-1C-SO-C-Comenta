#ifndef CONEXIONESCPU_H
#define CONEXIONESCPU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

void escuchar_dispatcher(t_config* config_CPU, t_log* logger_CPU);
void escuchar_interrupt(t_config* config_CPU, t_log* logger_CPU);

#endif // CONEXIONESCPU_H