#ifndef CONEXIONES_IO_H
#define CONEXIONES_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include "instrucciones_io.h"

extern int kernelfd;
extern int memoriafd;
extern t_log* logger_io;
extern char* nombre_io;

int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake);
int conectar_io_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake);
void recibir_solicitud_kernel();
void mandar_valor_a_memoria(char* valor, t_pid_stdin* pid_dirfisica_tamanio);
void recibir_kernel(t_config* config_io, int socket_kernel_io);
void recibir_memoria(t_config* config_io, int socket_memoria);
t_pid_unidades_trabajo* serializar_unidades_trabajo(t_buffer* buffer);
t_pid_stdin* deserializar_pid_stdin(t_buffer* buffer);
t_fs_create_delete* deserializar_pedido_creacion_destruccion(t_buffer* buffer);
void inicializar_file_system(t_config* config_io);

#endif // CONEXIONES_IO_H