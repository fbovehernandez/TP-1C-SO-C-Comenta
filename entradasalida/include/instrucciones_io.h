#ifndef INSTRUCCIONES_IO_H
#define INSTRUCCIONES_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include "conexiones_io.h"

void io_gen_sleep_func(int unidad_trabajo, int tiempo_unidad_trabajo);

#endif