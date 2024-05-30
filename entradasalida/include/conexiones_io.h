#ifndef CONEXIONES_IO_H
#define CONEXIONES_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include "instrucciones_io.h"

extern int socket_kernel_io;
extern t_log* logger_io;
extern char* nombre_io;

int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz);
void recibir_solicitud_kernel();
void recibir_kernel(t_config* config_io);
int serializar_unidades_trabajo(t_buffer* buffer);

#endif // CONEXIONES_IO_H