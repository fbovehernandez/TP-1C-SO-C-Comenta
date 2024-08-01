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

extern int kernelfd;
extern int memoriafd;
extern t_log* logger_io;
extern char* nombre_io;
extern sem_t se_escribio_memoria;

/////////////////////
///// FUNCIONES /////
/////////////////////

// KERNEL
int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake);
void recibir_kernel(void* config_io);

// MEMORIA
int conectar_io_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake);
void recibir_memoria(void* config_io);

// FUNCIONES DE FS 
void gestionar_truncar(int pid, char* nombre_archivo, int registro_tamanio);
void truncate_hacia_arriba(int pid, int ultimo_bloque, int registro_tamanio, t_archivo* file, char* nombre_archivo); 
void compactar(int pid, char* name_file, int tamanio_truncar);
int calcular_bloques_a_remover(int block_count_file, int tamanio_nuevo);
int obtener_tamanio_archivo(char* namefile);
void actualizar_file_metadata(char* namefile, int tamanio, int first_block);
void marcar_en_bitmap(int primer_bloque, int cantidad_bloques);
int calcular_bloques_necesarios(int bloques_actuales, int tamanio_nuevo);


// DESERIALIZACION|SERIALIZACION|ENVIO|RECEPCION DE PAQUETES
void mandar_valor_a_memoria(char* valor, t_pid_stdin* pid_dirfisica_tamanio);
void mandar_valor_a_memoria_fs(t_pedido_rw_encolar* pedido_fs, void* buffer);
t_pedido_rw_encolar* deserializar_pedido_rw(t_buffer* buffer);
t_solicitud_escritura_bloques* deserializar_solicitud_escritura_bloques(t_buffer* buffer);
t_archivo_encolar* deserializar_pedido_creacion_destruccion(t_buffer* buffer);
t_pedido_truncate* deserializar_pedido_fs_truncate(t_buffer* buffer);
t_pid_unidades_trabajo* serializar_unidades_trabajo(t_buffer* buffer);
t_pid_stdin* deserializar_pid_stdin(t_buffer* buffer);

// FUNCIONES DE AYUDA
void imprimir_datos_pedido_escritura(t_pedido_rw_encolar* pedido_escritura);
void mostrar_diccionario_no_vacio(t_dictionary* diccionario);
void mostrar_archivo(char* key, void* value);
int byte_de_primer_bloque(char* nombre_archivo);

// LIMPIEZA
void limpiar_bitmap(int ultimo_bloque, int bloques_a_remover);
void liberar_pid_stdin(t_pid_stdin* pid_stdin);

//////////////////////////////////////////////////
///// LOS DE ABAJO NO SE ENCUENTRAN EN EL .C /////
//////////////////////////////////////////////////

void recibir_solicitud_kernel();


#endif // CONEXIONES_IO_H