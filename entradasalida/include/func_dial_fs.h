#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "conexiones_io.h"
#include <commons/bitarray.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>

extern char* bitmap_data;
extern t_bitarray* bitmap;
extern void* bloques_data;
extern t_config_dialfs* dialfs;
extern t_dictionary* diccionario_archivos;

// void crear_archivos_iniciales(t_config_dialfs* dialfs_config);
t_config_dialfs* inicializar_file_system(t_config* config_io);
void crear_archivos_iniciales(char* filepath_bitmap, char* filepath_bloques);
int first_block_free();
void add_block_bitmap(int bloque);
void liberar_bitmap(char* nombre_archivo);
