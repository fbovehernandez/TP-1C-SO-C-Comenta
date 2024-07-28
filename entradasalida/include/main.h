#ifndef ENTRADA_SALIDA_H
#define ENTRADA_SALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "func_dial_fs.h"
#include "conexiones_io.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <readline/readline.h>
#include <readline/history.h>

void iniciar_interfaz_generica(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* puerto_kernel);
void iniciar_stdin(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria);
void iniciar_stdout(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria);
void iniciar_dialfs(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria);
void recibir_kernel_y_memoria(t_config_socket_io* config_kernel_io, t_config_socket_io* config_memoria_io);

void rearmar_diccionario_archivos(char* path_base);
bool hay_files_txt(char* path_base);
int asignar_bloques(int tamanio_file);
void recuperar_bitmap_y_bloques(char* filepath_bitmap, char* filepath_bloques);
#endif // MEMORIA_H
