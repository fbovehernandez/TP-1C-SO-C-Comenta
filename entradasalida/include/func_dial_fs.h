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

extern char* bitmap_data;
extern t_bitarray* bitmap;
extern void* bloques_data;

typedef struct {
    int block_size;
    int block_count;
    int retraso_compactacion;
    char* path_base;
} t_config_dialfs;

// void crear_archivos_iniciales(t_config_dialfs* dialfs_config);
t_config_dialfs* inicializar_file_system(t_config* config_io);
void crear_archivos_iniciales(t_config_dialfs* dialfs);  
