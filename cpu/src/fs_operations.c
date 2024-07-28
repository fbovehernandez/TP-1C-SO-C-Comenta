#include "../include/fs_operations.h"

t_buffer* llenar_buffer_fs_create_delete(char* nombre_interfaz, char* nombre_archivo){
    t_buffer *buffer = malloc(sizeof(t_buffer));
    
    int largo_nombre_interfaz = string_length(nombre_interfaz) + 1;  
    int largo_nombre_archivo = string_length(nombre_archivo) + 1; 

    buffer->size = sizeof(int) * 2 + largo_nombre_interfaz + largo_nombre_archivo; 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;
   
    memcpy(stream + buffer->offset, &largo_nombre_interfaz, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, nombre_interfaz, largo_nombre_interfaz);
    buffer->offset += largo_nombre_interfaz;
    memcpy(stream + buffer->offset, &largo_nombre_archivo, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, nombre_archivo, largo_nombre_archivo);
    buffer->offset += largo_nombre_archivo;

    return buffer;
}

t_buffer* llenar_buffer_fs_truncate(char* nombre_interfaz_a_truncar, char* nombre_file_truncate, uint32_t registro_truncador) {
    t_buffer *buffer = malloc(sizeof(t_buffer));

    int largo_nombre_interfaz = string_length(nombre_interfaz_a_truncar) + 1;
    int largo_nombre_archivo  = string_length(nombre_file_truncate) + 1; 
    
    buffer->size = sizeof(int) * 2 + largo_nombre_interfaz + largo_nombre_archivo + sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &largo_nombre_interfaz, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, nombre_interfaz_a_truncar, largo_nombre_interfaz);
    buffer->offset += largo_nombre_interfaz;
    memcpy(stream + buffer->offset, &largo_nombre_archivo, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, nombre_file_truncate, largo_nombre_archivo);
    buffer->offset += largo_nombre_archivo;
    memcpy(stream + buffer->offset, &registro_truncador, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    return buffer;
}

t_buffer* llenar_buffer_fs_escritura_lectura(char* nombre_interfaz, char* nombre_archivo, uint32_t registro_direccion, uint32_t registro_tamanio, uint32_t registro_puntero, int cantidad_paginas, t_list* direcciones_fisicas){ 
    t_buffer *buffer = malloc(sizeof(t_buffer));
    
    int largo_nombre_interfaz = string_length(nombre_interfaz) + 1;
    int largo_nombre_archivo  = string_length(nombre_archivo) + 1;

    int size_direcciones_fisicas = list_size(direcciones_fisicas);

    buffer->size = sizeof(int) * 3 + largo_nombre_interfaz + largo_nombre_archivo + sizeof(uint32_t) * 3 + (size_direcciones_fisicas * sizeof(int) * 2);

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &largo_nombre_interfaz, sizeof(int));
    buffer->offset += sizeof(int); 
    memcpy(stream + buffer->offset, nombre_interfaz, largo_nombre_interfaz);
    buffer->offset += largo_nombre_interfaz;
    memcpy(stream + buffer->offset, &largo_nombre_archivo, sizeof(int));
    buffer->offset += sizeof(int); 
    memcpy(stream + buffer->offset, nombre_archivo, largo_nombre_archivo);
    buffer->offset += largo_nombre_archivo;
    memcpy(stream + buffer->offset, &registro_direccion, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &registro_tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &registro_puntero, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int); 
    
    for(int i=0; i < cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(direcciones_fisicas, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }

    return buffer;
}