#ifndef CONEXIONES_IO_H
#define CONEXIONES_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "func_dial_fs.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include <commons/txt.h>
#include "instrucciones_io.h"

extern int kernelfd;
extern int memoriafd;
extern t_log* logger_io;
extern char* nombre_io;

int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake);
int conectar_io_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake);
void recibir_solicitud_kernel();
void mandar_valor_a_memoria(char* valor, t_pid_stdin* pid_dirfisica_tamanio);
void recibir_kernel(void* config_io);
void recibir_memoria(void* config_io);
t_pid_unidades_trabajo* serializar_unidades_trabajo(t_buffer* buffer);
t_pid_stdin* deserializar_pid_stdin(t_buffer* buffer);
t_archivo_encolar* deserializar_pedido_creacion_destruccion(t_buffer* buffer);
t_pedido_truncate* deserializar_pedido_fs_truncate(t_buffer* buffer);
int calcular_bloques_necesarios(int bloques_actuales, int tamanio_nuevo);
void gestionar_truncar(char* nombre_archivo, int registro_tamanio);
int obtener_tamanio_archivo(char* namefile);
void actualizar_file_metadata(char* namefile, int tamanio, int first_block);
void marcar_en_bitmap(int primer_bloque, int cantidad_bloques);
void compactar(char* name_file, int tamanio_truncar);
int calcular_bloques_a_remover(int block_count_file, int tamanio_nuevo);
void limpiar_bitmap(int ultimo_bloque, int bloques_a_remover);
void mostrar_diccionario_no_vacio(t_dictionary* diccionario);
void mostrar_archivo(char* key, void* value);
void truncate_hacia_arriba(int ultimo_bloque, int registro_tamanio, t_archivo* file, char* nombre_archivo); 
t_pedido_rw_encolar* deserializar_pedido_rw(t_buffer* buffer);
void imprimir_datos_pedido_escritura(t_pedido_rw_encolar* pedido_escritura);
void liberar_pid_stdin(t_pid_stdin* pid_stdin);

#endif // CONEXIONES_IO_H