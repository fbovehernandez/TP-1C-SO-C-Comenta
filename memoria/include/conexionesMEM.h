#ifndef CONEXIONESMEM_H
#define CONEXIONESMEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

void iniciar_servidor_memoria(t_config* config_memoria, t_log* logger);

#endif // CONEXIONES_H