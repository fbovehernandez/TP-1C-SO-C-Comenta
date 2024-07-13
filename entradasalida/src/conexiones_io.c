#include "../include/conexiones_io.h"

int kernelfd;
int memoriafd;
t_log* logger_io;
char* nombre_io;

int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake) {
    // int message_io = 12; // nro de codop
    //int valor = 5; // handshake, 5 = I/O
    kernelfd = crear_conexion(IP_KERNEL, puerto_kernel, handshake);
    
    if (kernelfd == -1) {
        printf("AYUDAAAAAAAAAA ERROR CON KERNEEEEEEL\n");
        log_error(logger_io, "Error al conectar con Kernel\n");
        return -1;
    }

    log_info(logger_io, "Conexion establecida con Kernel\n");
    
    // send(kernelfd, &message_io, sizeof(int), 0); 

    int str_interfaz = strlen(nombre_interfaz) + 1;

    // Crear y configurar el buffer
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(int) + str_interfaz + sizeof(TipoInterfaz);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // Construir el paquete en el buffer
    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &str_interfaz, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, nombre_interfaz, str_interfaz);
    buffer->offset += str_interfaz;
    memcpy(stream + buffer->offset, &tipo_interfaz, sizeof(TipoInterfaz));
    buffer->offset += sizeof(TipoInterfaz);
    
    buffer->stream = stream;
    // Enviar el paquete al Kernel
    enviar_paquete(buffer, CONEXION_INTERFAZ, kernelfd);
    printf("ENVIO MI NOMBRE!!!\n");
    // Liberar memoria y retornar el descriptor de socket
    // free(buffer->stream);
    // free(buffer);

    return kernelfd;
}

/* 
void recibir_solicitud_kernel() {
    codigo_operacion cod_op;

    recv(socket_kernel_io, &cod_op, sizeof(int), 0);
    switch(cod_op) {
        case QUIERO_NOMBRE:
            send(socket_kernel_io, nombre_io, 100, 0); // esta bien pasado asi?
            break;
        default:
            break;
    }
}
*/

int conectar_io_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake) {
    // int message_io = 12; // nro de codop
    //int valor = 5; 
    
    memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria, handshake);
    log_info(logger_io, "Conexion establecida con Memoria\n");
    
    // send(kernelfd, &message_io, sizeof(int), 0); 

    int str_interfaz = strlen(nombre_interfaz) + 1;
    
    t_info_io* io = malloc(sizeof(int) + sizeof(TipoInterfaz) + str_interfaz);
    io->nombre_interfaz_largo = str_interfaz;
    io->tipo = tipo_interfaz;
    io->nombre_interfaz = nombre_interfaz;

    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) + str_interfaz + sizeof(TipoInterfaz); // sizeof(x2)
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;
    
    memcpy(stream + buffer->offset, &io->nombre_interfaz_largo, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, io->nombre_interfaz, io->nombre_interfaz_largo);
    buffer->offset += io->nombre_interfaz_largo;
    memcpy(stream + buffer->offset, &io->tipo, sizeof(TipoInterfaz));
    buffer->offset += sizeof(TipoInterfaz);

    buffer->stream = stream;
    enviar_paquete(buffer, CONEXION_INTERFAZ, memoriafd);
    return memoriafd;
}

void mandar_valor_a_memoria(char* valor, t_pid_stdin* pid_stdin) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    int largo_valor = string_length(valor);

    buffer->size = largo_valor + sizeof(int) * 4 + (sizeof(int) * pid_stdin->cantidad_paginas * 2); 

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // void* stream = buffer->stream;
    
    memcpy(buffer->stream + buffer->offset, &pid_stdin->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_stdin->registro_tamanio, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pid_stdin->cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &largo_valor, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, valor, largo_valor);
    buffer->offset += largo_valor;

    for(int i=0; i < pid_stdin->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pid_stdin->lista_direcciones, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }
    
    // Si usamos memoria dinámica para el nombre, y no la precisamos más, ya podemos liberarla:
    free(valor);
    printf("Le pide a mem GUARDAR_VALOR... o eso creo.\n");
    enviar_paquete(buffer, GUARDAR_VALOR, memoriafd);
}

void recibir_kernel(t_config* config_io, int socket_kernel_io) {
    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        printf("Esperando paquete...\n");
        recv(socket_kernel_io, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de kernel: %d\n", paquete->codigo_operacion);

        recv(socket_kernel_io, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);

        recv(socket_kernel_io, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        switch(paquete->codigo_operacion) {
            case DORMITE:
                t_pid_unidades_trabajo* pid_unidades_trabajo = serializar_unidades_trabajo(paquete->buffer);
                int pid = pid_unidades_trabajo->pid;
                int unidades_trabajo = pid_unidades_trabajo->unidades_trabajo;
                int tiempoUnidadesTrabajo = config_get_int_value(config_io,"TIEMPO_UNIDAD_TRABAJO");
                printf("Unidades de trabajo: %d\n", unidades_trabajo);
                printf("Tiempo de unidades de trabajo: %d\n", tiempoUnidadesTrabajo);
                printf("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ\n");
                sleep(unidades_trabajo * tiempoUnidadesTrabajo);
                log_info(logger_io, "PID %d - Operacion: DORMIR_IO", pid);
                int termino_io = 1;
                send(socket_kernel_io, &termino_io, sizeof(int), 0);
                break;
            case LEETE: 
                int resultOk = 0;
                int terminoOk;
                
                t_pid_stdin* pid_stdin = deserializar_pid_stdin(paquete->buffer);
                send(socket_kernel_io, &resultOk, sizeof(int), 0);

                imprimir_datos_stdin(pid_stdin);
                printf("llego hasta LEETE\n");
                // Leer valor
                char *valor = malloc(pid_stdin->registro_tamanio);
                printf("Ingrese lo que quiera guardar (hasta %d caracteres): \n", pid_stdin->registro_tamanio);
                scanf("%s", valor); 
                // Mandarlo a memoria
                
                mandar_valor_a_memoria(valor, pid_stdin);
                log_info(logger_io, "PID: %d - Operacion: LEER", pid_stdin->pid);
                recv(memoriafd, &terminoOk, sizeof(int), MSG_WAITALL);
                printf("el terminoOk es: %d \n", terminoOk);
                send(socket_kernel_io, &terminoOk, sizeof(int), 0);
                free(pid_stdin);
                break;
            case CREAR_ARCHIVO:
                // Estaria bueno que tenngamos un diccionario con key nombre archiivo y puntero como  atrbuto
                t_fs_create_delete* pedido_creacion = deserializar_pedido_creacion_destruccion(paquete->buffer); 
                FILE* archivo;
                archivo = fopen(pedido_creacion->nombre_archivo,"w"); 

                if (archivo == NULL) {
                    printf("Error al abrir el archivo.\n");
                } else{
                    log_info(logger_io,"PID: %d - Crear Archivo: %s",pedido_creacion->pid,pedido_creacion->nombre_archivo);
                }
                
                break;
            case ELIMINAR_ARCHIVO:
            
                t_fs_create_delete* pedido_destruccion = deserializar_pedido_creacion_destruccion(paquete->buffer);
                // TODO: Hacemos busqueda en el diccionario y si lo encuentra, lo elimina sino, retorna
                
                log_info(logger_io,"PID: %d - Eliminar Archivo: %s",pedido_destruccion->pid,pedido_destruccion->nombre_archivo);
                txt_close_file(archivo); // Es de las commons, sino tenes el fclose de C
                
                break;
            default:
                printf("Se rompio kernel!!!!\n");
                exit(-1);
                break;
        }
        liberar_paquete(paquete);        
    }
}

void recibir_memoria(t_config* config_io, int socket_memoria) {
    printf("Voy a recibir memoria!\n");
    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        printf("Esperando paquete...\n");
        
        recv(socket_memoria, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de memoria: %d\n", paquete->codigo_operacion);

        recv(socket_memoria, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);

        recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        switch(paquete->codigo_operacion) {
            case RECIBI_VALOR_OK:
                printf("Recibi el valor ok\n");
                break;
            case ESCRIBITE:
                int tamanio, pid, terminoOk = 1;
                void* stream = paquete->buffer->stream;
                memcpy(&pid, stream, sizeof(int));
                stream += sizeof(int);
                memcpy(&tamanio, stream, sizeof(int));
                stream += sizeof(int);
                char* valor = malloc(tamanio);
                memcpy(valor, stream, tamanio);
                printf("\n\nEl valor leido de memoria es: %s \n\n", valor);
                log_info(logger_io, "PID: %d - Operacion: ESCRIBIR", pid);

                send(kernelfd, &terminoOk, sizeof(int), 0);
                break;
            default:
                printf("Se rompio memoria!!!!\n");
                exit(-1);
                break;
        }
        liberar_paquete(paquete);        
    }
    
}

t_pid_unidades_trabajo* serializar_unidades_trabajo(t_buffer* buffer) {
    t_pid_unidades_trabajo* pid_unidades_trabajo  = malloc(sizeof(t_pid_unidades_trabajo));
    void* stream = buffer->stream;
    memcpy(&(pid_unidades_trabajo->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pid_unidades_trabajo->unidades_trabajo), stream, sizeof(int));
    stream += sizeof(int);
    return pid_unidades_trabajo;
}

t_pid_stdin* deserializar_pid_stdin(t_buffer* buffer) {
    t_pid_stdin* pid_stdin = malloc(sizeof(t_pid_stdin)); 
    
    void* stream = buffer->stream;

    memcpy(&pid_stdin->pid, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&pid_stdin->registro_tamanio, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pid_stdin->cantidad_paginas, stream, sizeof(int));
    stream += sizeof(int);

    pid_stdin->lista_direcciones = malloc(pid_stdin->cantidad_paginas);
    pid_stdin->lista_direcciones = list_create();
    
    for(int i=0; i < pid_stdin->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio));
        memcpy(&dir_fisica_tam->direccion_fisica, stream, sizeof(int));
        stream += sizeof(int);
        memcpy(&dir_fisica_tam->bytes_lectura, stream, sizeof(int));
        stream += sizeof(int);
        list_add(pid_stdin->lista_direcciones, dir_fisica_tam);
        // free(dir_fisica_tam); //LIBERAR CUANDO SE DESCONECTE LA IO
    }
    printf("llego hasta deserializar pid stdin\n");
    return pid_stdin;
}

t_fs_create_delete* deserializar_pedido_creacion_destruccion(t_buffer* buffer){
    t_fs_create_delete* fs_create_delete = malloc(sizeof(t_fs_create_delete));
    
    void* stream = buffer->stream;

    memcpy(&fs_create_delete->pid, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&fs_create_delete->largo_archivo, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&fs_create_delete->nombre_archivo, stream,fs_create_delete->largo_archivo);
    
    return fs_create_delete;
}

void inicializar_file_system(t_config* config_io) {
    int block_size = config_get_int_value(config_io, "BLOCK_SIZE");
    int block_count = config_get_int_value(config_io, "BLOCK_COUNT");
    int retraso_compactacion = config_get_int_value(config_io, "RETRASO_COMPACTACION");
    char* path_base = config_get_string_value(config_io, "PATH_BASE_DIALFS");
    
    // Inicializar diccionario, bloques.dat, bitmap.dat y archivo metadata
}