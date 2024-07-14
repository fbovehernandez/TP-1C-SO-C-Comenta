#include "../include/fs_operations.h"

t_buffer* llenar_buffer_fs_create_delete(char* nombre_interfaz,char* nombre_archivo){
    t_buffer *buffer = malloc(sizeof(t_buffer));
    
    int largo_nombre_interfaz = string_length(nombre_interfaz);  // + 1?????
    int largo_nombre_archivo = string_length(nombre_archivo);  // +1 ????????

    buffer->size = sizeof(int) * 2 + largo_nombre_interfaz + largo_nombre_archivo; // Revisar esto AYUDAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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