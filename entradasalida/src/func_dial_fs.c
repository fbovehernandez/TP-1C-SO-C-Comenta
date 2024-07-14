#include "../include/func_dial_fs.h"

t_bitarray* bitmap;
char* bitmap_data; 
void* bloques_data;

t_config_dialfs* inicializar_file_system(t_config* config_io) {
    t_config_dialfs* dialfs_config = malloc(sizeof(t_config_dialfs));
    
    int block_size = config_get_int_value(config_io, "BLOCK_SIZE");
    int block_count = config_get_int_value(config_io, "BLOCK_COUNT");
    int retraso_compactacion = config_get_int_value(config_io, "RETRASO_COMPACTACION");
    char* path_base = config_get_string_value(config_io, "PATH_BASE_DIALFS");

    dialfs_config->block_size           = block_size;
    dialfs_config->block_count          = block_count;
    dialfs_config->retraso_compactacion = retraso_compactacion;
    dialfs_config->path_base            = path_base;
    
    return dialfs_config;
}

void crear_archivos_iniciales(t_config_dialfs* dialfs_config) {
    char filepath_bitmap[256]; 
    char filepath_bloques[256]; 

    snprintf(filepath_bloques, sizeof(filepath_bloques), "%s/bloques.dat", dialfs_config->path_base); // copia el path_base y lo pega directo, esto es par acuando levante otra io
    snprintf(filepath_bitmap, sizeof(filepath_bitmap), "%s/bitmap.dat", dialfs_config->path_base); // copia el path_base y lo pega directo, esto es par acuando levante otra io
    
    int fd_bitmap = open(filepath_bitmap, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    
    int tamanio_bitmap = dialfs_config->block_count / 8;
    
    if (ftruncate(fd_bitmap,tamanio_bitmap) == -1) {
        perror("Error al truncar archivo de bitmap");
        exit(EXIT_FAILURE);
    }

    bitmap_data = mmap(NULL, tamanio_bitmap , PROT_READ | PROT_WRITE, MAP_SHARED, fd_bitmap, 0); // 0 es el offset, me traigo todo el file

    // Crear un t_bitarray con la memoria mapeada y establecer todos los bits a cero
    bitmap = bitarray_create_with_mode(bitmap_data, tamanio_bitmap, LSB_FIRST);

    for (size_t i = 0; i < tamanio_bitmap; i++) {
        bitarray_clean_bit(bitmap, i); // Esto los limpia y lo pone en 0, es decr, disponible
    }

    // Sincronizar los cambios con el archivo bitmap.dat
    msync(bitmap_data, tamanio_bitmap, MS_SYNC);

    int fd_bloques = open(filepath_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if(ftruncate(fd_bloques, dialfs_config->block_size * dialfs_config->block_count) == -1) {
        perror("Error al truncar archivo de bloques");
        exit(EXIT_FAILURE);
    }

    bloques_data = mmap(NULL, dialfs_config->block_size * dialfs_config->block_count, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bloques, 0);
    memset(bloques_data, 0, dialfs_config->block_size * dialfs_config->block_count);

    /* Escritura de prueba para ver los bytes con el hexdump */ 
    char* data = "Hola, mundo!";
    size_t data_size = strlen(data); // +1 para el carácter nulo al final no se agrega pero si es importante cuando voy a leer del archivo (si en algun momento lo hago)

    // Copia los datos en la memoria mapeada
    memcpy(bloques_data, data, data_size);

    /* Fin escritura de prueba */

    msync(bloques_data, dialfs_config->block_size * dialfs_config->block_count, MS_SYNC);

    // Todo esto iria en la funcion que se encargue de liberarlo
    // bitarray_destroy(bitmap);
    // munmap(bitmap_data, block_count / 8);
    // munmap(bloques_data, block_size * block_count);

    close(fd_bloques);
    close(fd_bitmap);
}
