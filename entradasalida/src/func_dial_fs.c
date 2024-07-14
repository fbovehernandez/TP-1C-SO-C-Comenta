#include "../include/func_dial_fs.h"

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

/*
void crear_archivos_iniciales(t_config_dialfs* dialfs_config) {
    int fd_bitmap = open("bitmap.dat", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    ftruncate(fd_bitmap, block_count / 8);
    char* bitmap_data = mmap(NULL, block_count / 8, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bitmap, 0);

    // Crear un t_bitarray con la memoria mapeada y establecer todos los bits a cero
    bitmap = bitarray_create_with_mode(bitmap_data, block_count / 8, MSB_FIRST);
    bitarray_clean_range(bitmap, 0, block_count);

    // Sincronizar los cambios con el archivo bitmap.dat
    msync(bitmap_data, block_count / 8, MS_SYNC);

    // bitarray_destroy(bitmap);
    // munmap(bitmap_data, block_count / 8);
    close(fd_bitmap);
}
*/