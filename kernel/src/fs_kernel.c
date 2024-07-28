#include "../include/fs_kernel.h"

t_pedido_fs_create_delete* deserializar_pedido_fs_create_delete(t_buffer* buffer) {
    t_pedido_fs_create_delete* pedido_fs = malloc(sizeof(t_pedido_fs_create_delete));

    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6;
    
    memcpy(&(pedido_fs->longitud_nombre_interfaz), stream, sizeof(int));
    stream += sizeof(int);
    
    pedido_fs->nombre_interfaz = malloc(pedido_fs->longitud_nombre_interfaz);
    memcpy(pedido_fs->nombre_interfaz, stream, pedido_fs->longitud_nombre_interfaz);
    stream += pedido_fs->longitud_nombre_interfaz;
    
    memcpy(&(pedido_fs->longitud_nombre_archivo), stream, sizeof(int));
    stream += sizeof(int);
    
    pedido_fs->nombre_archivo = malloc(pedido_fs->longitud_nombre_archivo);
    memcpy(pedido_fs->nombre_archivo, stream, pedido_fs->longitud_nombre_archivo);
    stream += pedido_fs->longitud_nombre_archivo;
    
    printf("Nombre archivo recibido  %s\n", pedido_fs->nombre_archivo);
    printf("Nombre interfaz recibido  %s\n", pedido_fs->nombre_interfaz);
    
   // free(pedido_fs->nombre_interfaz);
   // free(pedido_fs->nombre_archivo);

    return pedido_fs;    
}

t_fs_truncate* deserializar_pedido_fs_truncate(t_buffer* buffer) {
    t_fs_truncate* pedido_fs = malloc(sizeof(t_fs_truncate));

    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6;
    
    memcpy(&(pedido_fs->largo_nombre_interfaz), stream, sizeof(int));
    stream += sizeof(int);
    
    pedido_fs->nombre_interfaz = malloc(pedido_fs->largo_nombre_interfaz);
    memcpy(pedido_fs->nombre_interfaz, stream, pedido_fs->largo_nombre_interfaz);
    stream += pedido_fs->largo_nombre_interfaz;
    
    memcpy(&(pedido_fs->largo_nombre_archivo), stream, sizeof(int));
    stream += sizeof(int);
    
    pedido_fs->nombre_archivo = malloc(pedido_fs->largo_nombre_archivo);
    memcpy(pedido_fs->nombre_archivo, stream, pedido_fs->largo_nombre_archivo);
    stream += pedido_fs->largo_nombre_archivo;
    
    memcpy(&(pedido_fs->registro_tamanio), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    
    printf("Nombre archivo recibido  %s\n", pedido_fs->nombre_archivo);
    printf("Nombre interfaz recibido  %s\n", pedido_fs->nombre_interfaz);
    
    //free(pedido_fs->nombre_interfaz);
    //free(pedido_fs->nombre_archivo);

    return pedido_fs;
}

t_pedido_fs_escritura_lectura* deserializar_pedido_fs_escritura_lectura(t_buffer* buffer) {
    t_pedido_fs_escritura_lectura* pedido_fs = malloc(sizeof(t_pedido_fs_escritura_lectura));

    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6;

    memcpy(&(pedido_fs->longitud_nombre_interfaz), stream, sizeof(int));
    stream += sizeof(int);

    pedido_fs->nombre_interfaz = malloc(pedido_fs->longitud_nombre_interfaz);
    memcpy(pedido_fs->nombre_interfaz, stream, pedido_fs->longitud_nombre_interfaz);
    stream += pedido_fs->longitud_nombre_interfaz;

    printf("Nombre interfaz: %s\n", pedido_fs->nombre_interfaz);

    memcpy(&pedido_fs->largo_archivo, stream, sizeof(int));
    stream += sizeof(int);

    pedido_fs->nombre_archivo = malloc(pedido_fs->largo_archivo);
    memcpy(pedido_fs->nombre_archivo, stream, pedido_fs->largo_archivo);
    stream += pedido_fs->largo_archivo;

    memcpy(&pedido_fs->registro_tamanio, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pedido_fs->registro_tamanio, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pedido_fs->registro_archivo, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    memcpy(&(pedido_fs->cantidad_paginas), stream, sizeof(int));
    stream += sizeof(int);

    printf("Cantidad de paginas: %d\n", pedido_fs->cantidad_paginas);

    pedido_fs->lista_dir_tamanio = list_create();

    // Deserializar lista con dir fisica y tamanio en bytes a leer segun la cant de pags

    for(int i = 0; i < pedido_fs->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio));
        memcpy(&(dir_fisica_tam->direccion_fisica), stream, sizeof(int));
        stream += sizeof(int);
        memcpy(&(dir_fisica_tam->bytes_lectura), stream, sizeof(int));
        stream += sizeof(int);

        list_add(pedido_fs->lista_dir_tamanio, dir_fisica_tam);
    }   

    return pedido_fs;
} 

void encolar_fs_truncate(t_pcb* pcb,  t_fs_truncate* pedido_fs, t_list_io* interfaz) {
    datos_operacion* datos_op_fs = malloc(sizeof(datos_operacion));

    t_pedido_truncate* pedido_a_encolar = malloc(sizeof(t_archivo_encolar));
    pedido_a_encolar->largo_nombre_archivo = pedido_fs->largo_nombre_archivo;
    pedido_a_encolar->nombre_archivo = strdup(pedido_fs->nombre_archivo);
    pedido_a_encolar->registro_tamanio = pedido_fs->registro_tamanio;

    printf("Voy a pushear lo siguiente - Namefile: %s\n", pedido_a_encolar->nombre_archivo);
    printf("Voy a pushear lo siguiente - Namelength: %d\n", pedido_a_encolar->largo_nombre_archivo);
    printf("Voy a pushear lo siguiente - TamRegistro: %d\n", pedido_a_encolar->registro_tamanio);

    datos_op_fs->tipo_operacion = TRUNCAR_ARCHIVO;
    datos_op_fs->pcb = pcb;
    datos_op_fs->puntero_operacion = pedido_a_encolar;
    
    // Ver bien esta sincronizacion
    pthread_mutex_lock(&mutex_cola_fs);
    list_add(interfaz->cola_blocked, datos_op_fs);
    
    sem_post(interfaz->semaforo_cola_procesos_blocked);
    printf("Desbloquee al sem de fs...\n");
    pthread_mutex_unlock(&mutex_cola_fs);

}

void encolar_fs_create_delete(codigo_operacion operacion, t_pcb* pcb, t_pedido_fs_create_delete* pedido_fs, t_list_io* interfaz) {
    datos_operacion* datos_op_fs = malloc(sizeof(datos_operacion));

    t_archivo_encolar* pedido_a_encolar = malloc(sizeof(t_archivo_encolar));
    pedido_a_encolar->largo_archivo = pedido_fs->longitud_nombre_archivo;
    pedido_a_encolar->nombre_archivo = strdup(pedido_fs->nombre_archivo);

    datos_op_fs->tipo_operacion = operacion;
    datos_op_fs->pcb = pcb;
    datos_op_fs->puntero_operacion = pedido_a_encolar;
    
    // Ver bien esta sincronizacion
    pthread_mutex_lock(&mutex_cola_fs);
    list_add(interfaz->cola_blocked, datos_op_fs);
    
    sem_post(interfaz->semaforo_cola_procesos_blocked);
    printf("Desbloquee al sem de fs...\n");
    pthread_mutex_unlock(&mutex_cola_fs);
}

void encolar_fs_read_write(codigo_operacion cod_op, t_pcb* pcb, t_pedido_fs_escritura_lectura* fs_read_write, t_list_io* interfaz) {
    // Funcion para encolar todos los datos necesarios para la lectura y escritura de un archivo sin la validacion de la io 

    datos_operacion* datos_op_fs = malloc(sizeof(datos_operacion));
    
    t_pedido_rw_encolar* fs_rw = malloc(sizeof(t_pedido_rw_encolar));
    int socket_io = interfaz->socket;
    
    fs_rw->largo_archivo = fs_read_write->largo_archivo;
    fs_rw->nombre_archivo = fs_read_write->nombre_archivo;
    fs_rw->registro_direccion = fs_read_write->registro_direccion;
    fs_rw->registro_tamanio = fs_read_write->registro_tamanio;
    fs_rw->registro_archivo = fs_read_write->registro_archivo;
    fs_rw->cantidad_paginas = fs_read_write->cantidad_paginas; 

    printf("Socket io: %d\n", socket_io);
    printf("Socket memoria: %d\n", sockets->socket_memoria);

    fs_rw->socket_io = socket_io;
    fs_rw->socket_memoria = sockets->socket_memoria;
   
    fs_rw->lista_dir_tamanio = list_create();

    // Deserializo la lista y la agrego al pedido a encolar fs_rw

    for(int i = 0; i < fs_rw->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(fs_read_write->lista_dir_tamanio, i);
        t_dir_fisica_tamanio* dir_fisica_tam_encolar = malloc(sizeof(t_dir_fisica_tamanio));
        dir_fisica_tam_encolar->direccion_fisica = dir_fisica_tam->direccion_fisica;
        dir_fisica_tam_encolar->bytes_lectura = dir_fisica_tam->bytes_lectura;

        list_add(fs_rw->lista_dir_tamanio, dir_fisica_tam_encolar);
    }

    datos_op_fs->tipo_operacion = cod_op;
    printf("Tipo operacion: %d\n", datos_op_fs->tipo_operacion);
    datos_op_fs->pcb = pcb;
    printf("pcb: %d\n", datos_op_fs->pcb->pid);
    datos_op_fs->puntero_operacion = fs_rw;
    
    pthread_mutex_lock(&mutex_cola_fs);
    list_add(interfaz->cola_blocked, datos_op_fs);
    pthread_mutex_unlock(&mutex_cola_fs);

    sem_post(interfaz->semaforo_cola_procesos_blocked);

    // free(fs_rw->nombre_archivo);
    // free(fs_rw);

    // liberar_pedido_fs_escritura_lectura(fs_read_write);
}

int ejecutar_io_dialfs_CREATE_DELETE(int socket, t_archivo_encolar* archivo_encolar, codigo_operacion operacion) {
    // Aca voy a enviar a la io toda la informacion necesria para que pueda crear o borrar un archivo
    t_buffer *buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(int) + archivo_encolar->largo_archivo;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &archivo_encolar->largo_archivo, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, archivo_encolar->nombre_archivo, archivo_encolar->largo_archivo);

    enviar_paquete(buffer, operacion, socket);

    return 0;
}

int ejecutar_io_dialfs_TRUNCATE(int socket, t_pedido_truncate* pedido_truncate) {
    // Aca voy a enviar a la io toda la informacion necesria para que pueda truncar un archivo
    t_buffer *buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(int) + pedido_truncate->largo_nombre_archivo + sizeof(uint32_t);

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pedido_truncate->largo_nombre_archivo, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, pedido_truncate->nombre_archivo, pedido_truncate->largo_nombre_archivo);
    buffer->offset += pedido_truncate->largo_nombre_archivo;
    memcpy(buffer->stream + buffer->offset, &pedido_truncate->registro_tamanio, sizeof(uint32_t));

    enviar_paquete(buffer, TRUNCAR_ARCHIVO, socket);

    return 0;

}

void* handle_io_dialfs(void* socket_io) {
    int termino_io, respuesta_conexion;
    int validacion_conexion = 1;

    int socket = (intptr_t)socket_io;

    t_paquete *paquete = inicializarIO_recibirPaquete(socket);
    t_list_io* io;

    switch (paquete->codigo_operacion) {
        case CONEXION_INTERFAZ:
            printf("Vamos a establecer conexion con dialfs!!!\n");
            io = establecer_conexion(paquete->buffer, socket);
            // free(io);
            break;
        default:
            printf("Llega al default.");
            return NULL;
    }

    datos_operacion* datos_op = malloc(sizeof(datos_operacion));

    while (true) {
        sem_wait(io->semaforo_cola_procesos_blocked);

        printf("Llega al sem y mutex\n");
        
        pthread_mutex_lock(&mutex_cola_fs);
        if(list_is_empty(io->cola_blocked)) {
            printf("No hay procesos en la cola de bloqueados de la IO\n");
        } else {
            datos_op = (datos_operacion*) list_remove(io->cola_blocked, 0);
        }
        pthread_mutex_unlock(&mutex_cola_fs);

        pasar_a_blocked(datos_op->pcb);

        // Chequeo conexion de la io, sino desconecto y envio proceso a exit (no se desconectan io mientras tenga procesos en la cola) -> NO BORREN ESTE
        
        send(socket, &validacion_conexion, sizeof(int), 0);
        int result = recv(socket, &respuesta_conexion, sizeof(int), 0);
        printf("resultado recv: %d\n", result);

        if(result == 0) {
            printf("Se desconecto la IO\n");
            pasar_a_exit(datos_op->pcb, "DESCONEXION_IO");
            dictionary_remove(diccionario_io, io->nombreInterfaz);

            // Ojo! Aca tambien hay que liberar todo lo que no se usa cuando la io se termina
            free(datos_op->puntero_operacion);
            free(datos_op);
            liberar_paquete(paquete);
            
            return NULL;
        }

        printf("Codigo de operacion: %d\n", datos_op->tipo_operacion);
        switch (datos_op->tipo_operacion) {
            case ELIMINAR_ARCHIVO:
            case CREAR_ARCHIVO:
                t_archivo_encolar* archivo_encolar = (t_archivo_encolar*) datos_op->puntero_operacion;
                printf("Vamos a encolar el archivo %s\n", archivo_encolar->nombre_archivo);
       
                int respuesta_ok = ejecutar_io_dialfs_CREATE_DELETE(socket, archivo_encolar, datos_op->tipo_operacion);
                printf("la respuesta ok es: %d\n", respuesta_ok);

                if (!respuesta_ok) {
                    printf("Se ejecuto correctamente la IO...\n");
        
                    recv(socket, &termino_io, sizeof(int), MSG_WAITALL);
                    
                    printf("Termino io: %d\n", termino_io);
                    if (termino_io == 1) { // El send de termino io envia 1.
                        printf("Termino la IO\n");
                        pasar_a_ready(datos_op->pcb);
                    }
                } else {
                    printf("No se pudo ejecutar la IO\n");
                    break; // Ver si es necesario este break
                }

                break;
            case TRUNCAR_ARCHIVO : // Voy a repetir un poco de logica aca, despues abstraigo
                t_pedido_truncate* pedido_truncate = (t_pedido_truncate*) datos_op->puntero_operacion;
                printf("Vamos a encolar el archivo %s\n", pedido_truncate->nombre_archivo);
       
                int respuesta_ok_truncate = ejecutar_io_dialfs_TRUNCATE(socket, pedido_truncate);
                printf("la respuesta ok es: %d\n", respuesta_ok);

                if (!respuesta_ok_truncate) {
                    printf("Se ejecuto correctamente la IO...\n");
        
                    recv(socket, &termino_io, sizeof(int), MSG_WAITALL);
                    
                    printf("Termino io: %d\n", termino_io);
                    if (termino_io == 1) { // El send de termino io envia 1.
                        printf("Termino la IO\n");
                        pasar_a_ready(datos_op->pcb);
                    }
                } else {
                    printf("No se pudo ejecutar la IO\n");
                    break; // Ver si es necesario este break
                }
                break;
            case ESCRIBIR_FS_MEMORIA: // Write  
            case LEER_FS_MEMORIA: // Read
                int termino_io_rw;
                
                printf("Llega a leer o escribir en memoria\n");

                t_pedido_rw_encolar* rw_encolar = (t_pedido_rw_encolar*) datos_op->puntero_operacion;
                
                imprimir_datos_rw(rw_encolar);

                sleep(10); 

                int respuesta_ok_rw = ejecutar_io_dialfs_READ_WRITE(rw_encolar, datos_op->tipo_operacion, socket);

                printf("la respuesta ok es: %d\n", respuesta_ok_rw);

                if (!respuesta_ok_rw) {
                    printf("Se ejecuto correctamente la IO...\n");
        
                    recv(socket, &termino_io_rw, sizeof(int), MSG_WAITALL);
                    
                    printf("Termino io: %d\n", termino_io_rw);
                    if (termino_io_rw == 1) { // El send de termino io envia 1.
                        printf("Termino la IO\n");
                        pasar_a_ready(datos_op->pcb);
                    }
                } else {
                    printf("No se pudo ejecutar la IO\n");
                    break; // Ver si es necesario este break
                }

                break;

            default:
                printf("Llega al default en handle_io_dialfs.\n");
                return NULL;
                break;
        }
    }

    free(datos_op->puntero_operacion);
    free(datos_op);
    liberar_paquete(paquete);
    return NULL;
}

void imprimir_datos_rw(t_pedido_rw_encolar* rw_encolar) {
    printf("Largo archivo: %d\n", rw_encolar->largo_archivo);
    printf("Nombre archivo: %s\n", rw_encolar->nombre_archivo);
    printf("Registro direccion: %d\n", rw_encolar->registro_direccion);
    printf("Registro tamanio: %d\n", rw_encolar->registro_tamanio);
    printf("Registro archivo: %d\n", rw_encolar->registro_archivo);
    printf("Cantidad de paginas: %d\n", rw_encolar->cantidad_paginas);

    for(int i = 0; i < rw_encolar->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(rw_encolar->lista_dir_tamanio, i);
        printf("Direccion fisica: %d\n", dir_fisica_tam->direccion_fisica);
        printf("Bytes a leer: %d\n", dir_fisica_tam->bytes_lectura);
    }

    printf("Socket io: %d\n", rw_encolar->socket_io);
    printf("Socket memoria: %d\n", rw_encolar->socket_memoria);
}

/* typedef struct{
    int largo_archivo;
    char* nombre_archivo;
    uint32_t registro_direccion;
    uint32_t registro_tamanio;
    uint32_t registro_archivo;
} t_pedido_rw_encolar; */

int ejecutar_io_dialfs_READ_WRITE(t_pedido_rw_encolar* rw_encolar, codigo_operacion operacion, int socket_io) {
    // Aca voy a enviar a la io toda la informacion necesria para que pueda leer o escribir un archivo
    t_buffer *buffer = malloc(sizeof(t_buffer));

    // Calculo el list size de la lista y lo agrego al size del buffer
    int list_size = rw_encolar->cantidad_paginas * sizeof(int) * 2;

    buffer->size = sizeof(uint32_t) * 3 + rw_encolar->largo_archivo + sizeof(int) * 4 + list_size;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &rw_encolar->largo_archivo, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, rw_encolar->nombre_archivo, rw_encolar->largo_archivo);
    buffer->offset += rw_encolar->largo_archivo;
    memcpy(buffer->stream + buffer->offset, &rw_encolar->registro_direccion, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &rw_encolar->registro_tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &rw_encolar->registro_archivo, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    // Agrego la cantidad de paginas a leer o escribir y la lista
    memcpy(buffer->stream + buffer->offset, &rw_encolar->cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);

    for(int i = 0; i < rw_encolar->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(rw_encolar->lista_dir_tamanio, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }

    printf("Socket io: %d\n", rw_encolar->socket_io);

    memcpy(buffer->stream + buffer->offset, &rw_encolar->socket_io, sizeof(int));
    buffer->offset += sizeof(int);

    printf("Socket memoria: %d\n", rw_encolar->socket_memoria);

    memcpy(buffer->stream + buffer->offset, &rw_encolar->socket_memoria, sizeof(int));
    buffer->offset += sizeof(int);

    if(operacion == LEER_FS_MEMORIA) {
        enviar_paquete(buffer, operacion, socket_io);
    } else {
        enviar_paquete(buffer, operacion, sockets->socket_memoria);
    }

    // enviar_paquete(buffer, operacion, socket_io);

    return 0;
}


// READ -> A LA IO
// WRITE -> A MEM