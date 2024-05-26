#ifndef ENTRADA_SALIDA_H
#define ENTRADA_SALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "conexiones_io.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <readline/readline.h>
#include <readline/history.h>

void iniciar_interfaz_generica(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* puerto_kernel);

#endif // MEMORIA_H
