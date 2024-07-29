#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include "funcionalidades.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

t_pedido_fs_create_delete* deserializar_pedido_fs_create_delete(t_buffer* buffer);
t_fs_truncate* deserializar_pedido_fs_truncate(t_buffer* buffer);
t_pedido_fs_escritura_lectura* deserializar_pedido_fs_escritura_lectura(t_buffer* buffer);
void encolar_fs_truncate(t_pcb* pcb,  t_fs_truncate* pedido_fs, t_list_io* interfaz);
void encolar_fs_create_delete(codigo_operacion operacion, t_pcb* pcb, t_pedido_fs_create_delete* pedido_fs, t_list_io* interfaz);
void encolar_fs_read_write(codigo_operacion cod_op, t_pcb* pcb, t_pedido_fs_escritura_lectura* fs_read_write, t_list_io* interfaz);
int ejecutar_io_dialfs_CREATE_DELETE(int socket, t_archivo_encolar* archivo_encolar, codigo_operacion operacion, int pid);
int ejecutar_io_dialfs_TRUNCATE(int socket, t_pedido_truncate* pedido_truncate, int pid);
int ejecutar_io_dialfs_READ_WRITE(t_pedido_rw_encolar* rw_encolar, codigo_operacion operacion, int socket_, int pid);
void imprimir_datos_rw(t_pedido_rw_encolar* rw_encolar);
#endif // KERNEL_FS_H_