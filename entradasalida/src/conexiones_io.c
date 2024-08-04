#include "../include/conexiones_io.h"

int kernelfd;
int memoriafd;
t_log* logger_io;
char* nombre_io;
sem_t se_escribio_memoria;

//////////////////
///// KERNEL /////
//////////////////

int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake) {
    kernelfd = crear_conexion(IP_KERNEL, puerto_kernel, handshake);
    
    if (kernelfd == -1) {
        log_error(logger_io, "Error al conectar con Kernel\n");
        return -1;
    }

    printf("Conexion establecida con Kernel\n");
    
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

    return kernelfd;
}

void recibir_kernel(void* config_socket_io) { // FREE
    int still_running = 1;
    int test_conexion;
    
    t_config_socket_io* config_io_kernel = (t_config_socket_io*) config_socket_io;

    int socket_kernel_io = config_io_kernel->socket_io;
    t_config* config_io = config_io_kernel->config_io;

    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        test_conexion = 0;

        printf("Esperando paquete... en kernel\n");
        recv(socket_kernel_io, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de kernel: %d\n", paquete->codigo_operacion);

        recv(socket_kernel_io, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        printf("Recibi un tamanio\n");

        recv(socket_kernel_io, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);
        printf("Recibi un paquete con stream!\n");

        switch(paquete->codigo_operacion) {
            case DORMITE:
                t_pid_unidades_trabajo* pid_unidades_trabajo = serializar_unidades_trabajo(paquete->buffer);
                int pid = pid_unidades_trabajo->pid;
                int unidades_trabajo = pid_unidades_trabajo->unidades_trabajo;
                int tiempoUnidadesTrabajo = config_get_int_value(config_io,"TIEMPO_UNIDAD_TRABAJO");
                printf("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ\n");
                sleep(unidades_trabajo * tiempoUnidadesTrabajo / 1000);
                log_info(logger_io, "PID %d - Operacion: DORMIR_IO", pid);
                int termino_io = 1;
                send(socket_kernel_io, &termino_io, sizeof(int), 0);
                
                free(pid_unidades_trabajo);
                break;
            case LEETE:
                char valor_leido[100];

                int resultOk = 0;
                int terminoOk;
                
                t_pid_stdin* pid_stdin = deserializar_pid_stdin(paquete->buffer);
                send(socket_kernel_io, &resultOk, sizeof(int), 0);

                imprimir_datos_stdin(pid_stdin);
                
                // Leer valor
                printf("Ingrese lo que quiera guardar (hasta %d caracteres): \n", pid_stdin->registro_tamanio);

                scanf(" %[^\n]", valor_leido); // Sale en rojo, si...

                mandar_valor_a_memoria(valor_leido, pid_stdin);

                log_info(logger_io, "PID: %d - Operacion: LEER", pid_stdin->pid);

                recv(memoriafd, &terminoOk, sizeof(int), MSG_WAITALL);

                send(socket_kernel_io, &terminoOk, sizeof(int), 0);
                
                liberar_pid_stdin(pid_stdin);
                break;
            case CREAR_ARCHIVO:
                int termino_creacion_ok = 1; 

                usleep(dialfs->tiempo_unidad_trabajo * 1000);

                // Estaria bueno que tengamos un diccionario con key nombre archivo y puntero como atrbuto -> Si , o directamente el archivo y su bloque incial y la cantidad (facu)
                t_archivo_encolar* pedido_creacion = deserializar_pedido_creacion_destruccion(paquete->buffer); // Me olvide asi que por ahora sin PID

                log_info(logger_io, "PID: %d - Operacion: CREAR_ARCHIVO", pedido_creacion->pid);

                char path_completo[256];

                sprintf(path_completo, "%s/%s", dialfs->path_base, pedido_creacion->nombre_archivo);

                FILE* archivo;
                archivo = fopen(path_completo,"w"); 

                if (archivo == NULL) {
                    printf("Error al abrir el archivo.\n");
                } else {
                    printf("Se creo el archivo %s\n", path_completo);

                    // Escribir el bloque inicial y el tamaño del archivo
                    int bloque_inicial = first_block_free();
                    add_block_bitmap(bloque_inicial);
                    
                    // Agregar a la lista de archivos con un bloque inicial...
                    t_archivo* metadata_archivo = malloc(sizeof(t_archivo));
                    metadata_archivo->first_block = bloque_inicial;
                    metadata_archivo->name_file = pedido_creacion->nombre_archivo;
                    metadata_archivo->block_count = 1;
                    
                    dictionary_put(diccionario_archivos, pedido_creacion->nombre_archivo, metadata_archivo);

                    int tamanio_archivo = 0;
                    fprintf(archivo, "BLOQUE_INICIAL=%d\n", bloque_inicial);
                    fprintf(archivo, "TAMANIO_ARCHIVO=%d\n", tamanio_archivo);

                    log_info(logger_io, "PID: %d - Crear Archivo: %s", pedido_creacion->pid, pedido_creacion->nombre_archivo);

                    // Envio confirmacion de que salio todo ok..
                    send(socket_kernel_io, &termino_creacion_ok, sizeof(int), 0);
                }
    
                msync(bitmap_data, dialfs->block_count / 8, MS_SYNC);
                fclose(archivo); // txt_close_file(archivo);
                
                break;
            case ELIMINAR_ARCHIVO:
                int termino_delete_ok = 1;

                usleep(dialfs->tiempo_unidad_trabajo * 1000);

                t_archivo_encolar* pedido_destruccion = deserializar_pedido_creacion_destruccion(paquete->buffer);

                log_info(logger_io, "PID: %d - Operacion: ELIMINAR_ARCHIVO", pedido_destruccion->pid);

                char path_completo_eliminar[256];
                sprintf(path_completo_eliminar, "%s/%s", dialfs->path_base, pedido_destruccion->nombre_archivo);

                if (remove(path_completo_eliminar) == 0) {
                    printf("El archivo %s ha sido eliminado correctamente.\n", path_completo_eliminar);
                    liberar_bitmap(pedido_destruccion->nombre_archivo);
                    log_info(logger_io, "PID: %d - Eliminar Archivo: %s", pedido_destruccion->pid, pedido_destruccion->nombre_archivo);
                    send(socket_kernel_io, &termino_delete_ok, sizeof(int), 0);
                } else {
                    printf("Error al eliminar el archivo %s.\n", path_completo_eliminar);
                }
                
                break;
            case TRUNCAR_ARCHIVO:
                int fin_truncate_ok = 1;

                usleep(dialfs->tiempo_unidad_trabajo * 1000);

                t_pedido_truncate* pedido_truncate = deserializar_pedido_fs_truncate(paquete->buffer);

                log_info(logger_io, "PID: %d - Operacion: TRUNCAR_ARCHIVO", pedido_truncate->pid);

                printf("Namefile: %s\n", pedido_truncate->nombre_archivo);
                printf("Registro truncador: %d\n", pedido_truncate->registro_tamanio);

                log_info(logger_io, "PID: %d - Truncar Archivo: %s - Tamaño: %d", pedido_truncate->pid, pedido_truncate->nombre_archivo, pedido_truncate->registro_tamanio);

                gestionar_truncar(pedido_truncate->pid, pedido_truncate->nombre_archivo, pedido_truncate->registro_tamanio);

                send(socket_kernel_io, &fin_truncate_ok, sizeof(int), 0);
                break;
            case LEER_FS_MEMORIA:
                int termino_escritura_ok;
                int termino_ok_kernel = 1;

                usleep(dialfs->tiempo_unidad_trabajo * 1000);

                t_pedido_rw_encolar* pedido_escritura = deserializar_pedido_rw(paquete->buffer);

                log_info(logger_io, "PID: %d - Operacion: LEER_ARCHIVO", pedido_escritura->pid);

                imprimir_datos_pedido_escritura(pedido_escritura);

                // Aca lo que voy a hacer es leer del void* mepeado, lo voy a copiar a un buffer y lo voy a mandar a memoria asi lo guarda en el void* de mem
                void* buffer = malloc(pedido_escritura->registro_tamanio);

                int bytes_inicio_file = byte_de_primer_bloque(pedido_escritura->nombre_archivo);
                int bytes_movidos = bytes_inicio_file + pedido_escritura->registro_archivo;
                printf("Los bytes movidos son: %d\n", bytes_movidos);
                
                memcpy(buffer, bloques_data + bytes_movidos, pedido_escritura->registro_tamanio);

                printf("El buffer leido del fs y a copiar en mem es: %s\n", (char*)buffer);

                // Aca tengo que mandar el buffer a memoria
                mandar_valor_a_memoria_fs(pedido_escritura, buffer);

                log_info(logger_io, "PID: %d - Leer Archivo: %s - Tamaño a Leer: %d - Puntero Archivo: %d", pedido_escritura->pid, pedido_escritura->nombre_archivo, pedido_escritura->registro_tamanio, pedido_escritura->registro_archivo);

                // recv(memoriafd, &termino_escritura_ok, sizeof(int), MSG_WAITALL);

                // Uso esto y no recv porque literal no puedo recibir el valor por recv ya que lo toma el hilo
                sem_wait(&se_escribio_memoria);
                
                printf("el valor de termino_escritura_ok es %d\n", termino_escritura_ok);

                send(socket_kernel_io, &termino_ok_kernel, sizeof(int), 0);
                break;
            default:
                printf("Se rompio kernel!!!!\n");
                exit(-1);
                break;
        }

        liberar_paquete(paquete);        
    }
}

///////////////////
///// MEMORIA /////
///////////////////

int conectar_io_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz, int handshake) {
    memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria, handshake);
    printf("Conexion establecida con Memoria\n");
    
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
    free(io);
    return memoriafd;
}

void recibir_memoria(void* config_socket_io) {
    t_config_socket_io* config_io_memoria = (t_config_socket_io*) config_socket_io;

    int socket_memoria = config_io_memoria->socket_io;
    t_config* config_io = config_io_memoria->config_io;

    int still_running = 1;
    int test_conexion; 
    
    while(1) {
        printf("Esperando validar conexion... de memoria\n");  

        // recv(socket_memoria, &test_conexion, sizeof(int), MSG_WAITALL); de memoria
        // send(socket_memoria, &still_running, sizeof(int), 0);

        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        printf("Esperando paquete... en memoria\n");
        
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

                printf("el tamanio del dato a escribir es: %d", tamanio);

                char* valor = malloc(tamanio + 1); 

                memcpy(valor, stream, tamanio);
                
                int tamanio_valor_recibido = string_length(valor);
                printf("el tamanio del valor a escribir es %d\n", tamanio_valor_recibido);
                valor[tamanio] = '\0';
                printf("\n\nEl valor leido de memoria es: %s \n\n", valor);
                
                log_info(logger_io, "PID: %d - Operacion: ESCRIBIR", pid);
                free(valor);
                send(kernelfd, &terminoOk, sizeof(int), 0);
                break;
            case ESCRIBIR_BLOQUES:
                int termino_escritura_ok = 1;

                usleep(dialfs->tiempo_unidad_trabajo * 1000);

                // Ahora deserializo el buffer del char* con su tamanio y escribo en el void* mapeado y luego sincronizo la informacion con el file bloques.dat y al mismo tiempo actualizo el bitmap
                t_solicitud_escritura_bloques* solicitud_escritura = deserializar_solicitud_escritura_bloques(paquete->buffer);
                printf("EL TAMANIO A COPIAR ES: %d\n", solicitud_escritura->tamanio_a_copiar);
                printf("El valor a copiar es: %s\n", (char*)solicitud_escritura->valor_a_copiar);
                printf("El puntero archivo es: %d\n", solicitud_escritura->puntero_archivo);
                printf("El nombre del file es %s\n", solicitud_escritura->nombre_archivo);
                
                log_info(logger_io, "PID: %d - Operacion: ESCRIBIR_ARCHIVO", solicitud_escritura->pid);

                // Podria agregar en esta parte validacion por si se pasa del tamanio del file, pero como no tengo tiempo y supongo que tampoco va a pasar, no lo voy a hacer
                // Voy a hacer lo mismo que en memoria, y ponerle el \0 para leerlo bien y copiar eso en el file

                // char* registro_lectura_rw_string = malloc(solicitud_escritura->tamanio_a_copiar + 1); // Acordarse de hacer el free

                // memcpy(registro_lectura_rw_string, solicitud_escritura->valor_a_copiar, solicitud_escritura->tamanio_a_copiar);
                // registro_lectura_rw_string[solicitud_escritura->tamanio_a_copiar] = '\0';

                // printf("El registro pero bien leido es: %s\n", registro_lectura_rw_string);
                
                int bytes_inicio_file = byte_de_primer_bloque(solicitud_escritura->nombre_archivo);
                int bytes_movidos = bytes_inicio_file + solicitud_escritura->puntero_archivo;
                printf("Los bytes movidos son: %d\n", bytes_movidos);

                // Aca podria tomar el valor a copiar cortado por su tamanio
                memcpy(bloques_data + bytes_movidos, solicitud_escritura->valor_a_copiar, solicitud_escritura->tamanio_a_copiar);

                // Sincronizo los cambios con el archivo subyacente
                msync(bloques_data, dialfs->block_size * dialfs->block_count, MS_SYNC);

                log_info(logger_io, "PID: %d - Escribir Archivo: %s - Tamaño a Escribir: %d - Puntero Archivo: %d", solicitud_escritura->pid, solicitud_escritura->nombre_archivo, solicitud_escritura->tamanio_a_copiar, solicitud_escritura->puntero_archivo);

                // recv(socket_memoria, &termino_escritura_ok, sizeof(int), MSG_WAITALL);
                send(kernelfd, &termino_escritura_ok, sizeof(int), 0);
                break;
            case FIN_ESCRITURA_FS:
                int confirmacion_escritura_fs;

                // Aca voy a recibir el valor de termino escritura y voy a simplemente desbloquear el semaforo

                memcpy(&confirmacion_escritura_fs, paquete->buffer->stream, sizeof(int));

                printf("La confirmacion de escritura de memoria es: %d\n", confirmacion_escritura_fs);
                if(confirmacion_escritura_fs == 99) {
                    sem_post(&se_escribio_memoria);
                } else {
                    printf("Error en la escritura de memoria\n");
                }
                
                break;
            default:
                printf("Se rompio memoria!!!!\n");
                exit(-1);
                break;
        }

       liberar_paquete(paquete);        
    }
}

///////////////////////////
///// FUNCIONES DE FS /////
///////////////////////////

void gestionar_truncar(int pid, char* nombre_archivo, int registro_tamanio) {
    // Detail, ver si con el get y la modificacion del valor luego no hace falta hacer el put
    t_archivo* file = dictionary_get(diccionario_archivos, nombre_archivo);

    int ultimo_bloque = file->block_count + file->first_block -1;
    int tamanio_actual = obtener_tamanio_archivo(nombre_archivo);

    // Posibles problemas not tested -> Mismo tamanio para truncar hacia abajo, y si trunco para abajo en 0
    if(registro_tamanio < tamanio_actual) {
        int bloques_a_remover = calcular_bloques_a_remover(file->block_count, registro_tamanio);

        limpiar_bitmap(ultimo_bloque, bloques_a_remover);

        actualizar_file_metadata(nombre_archivo, registro_tamanio, file->first_block);

        printf("Bloques a remover: %d\n", bloques_a_remover);

        file->block_count -= bloques_a_remover;

        printf("Bloques actuales luego de remover: %d\n", file->block_count);

        // dictionary_put(diccionario_archivos, nombre_archivo, file);
    } else if (registro_tamanio >= tamanio_actual) {
        truncate_hacia_arriba(pid, ultimo_bloque, registro_tamanio, file, nombre_archivo);
        // dictionary_put(diccionario_archivos, nombre_archivo, file);
    } 

    mostrar_diccionario_no_vacio(diccionario_archivos);
    msync(bitmap_data, dialfs->block_count / 8, MS_SYNC);
}

void truncate_hacia_arriba(int pid, int ultimo_bloque, int registro_tamanio, t_archivo* file, char* nombre_archivo) {
    int bloques_libres_consecutivos = 0;
    int bloques_necesarios_add = calcular_bloques_necesarios(file->block_count, registro_tamanio);

    for (int i = ultimo_bloque + 1; i < (ultimo_bloque + 1 + bloques_necesarios_add); i++) {
        if (i >= dialfs->block_count) {
            log_error(logger_io, "No hay bloques disponibles para truncar el archivo %s", nombre_archivo);
            break;
        }
        
        if (bitarray_test_bit(bitmap, i) == 0) { // Si el bloque está libre
            bloques_libres_consecutivos++;
        } 

    }

    if (bloques_libres_consecutivos == bloques_necesarios_add) {

        for (int i = ultimo_bloque + 1; i < (ultimo_bloque + 1 + bloques_necesarios_add); i++) {
            bitarray_set_bit(bitmap, i);
        }
       
        file->block_count += bloques_necesarios_add;

        // Modificar el tamanio del metadata coincidente
        actualizar_file_metadata(nombre_archivo, registro_tamanio, file->first_block);
    } else {
        printf("Entro para compactar\n");
        compactar(pid, nombre_archivo, registro_tamanio);
    }
}

void compactar(int pid, char* name_file, int tamanio_truncar) {
    log_info(logger_io, "PID: %d - Inicio Compactación.", pid);
    usleep(dialfs->retraso_compactacion * 1000);

    // Obtener datos del namefile a truncar

    t_archivo* file_a_truncar = dictionary_get(diccionario_archivos, name_file);

    // Hacer una copia de bloques.dat
    void* bloques_copia = malloc(dialfs->block_size * dialfs->block_count);
    memcpy(bloques_copia, bloques_data, dialfs->block_size * dialfs->block_count);

    // Limpiar todos los bits del bitmap
    for (int i = 0; i < dialfs->block_count; i++) {
        bitarray_clean_bit(bitmap, i);
    }

    // Aca uso una lista de keys para poder iterar sobre cada una en el file
    t_list* keys = dictionary_keys(diccionario_archivos);

    // Recorrer el diccionario de archivos
    for(int i = 0; i < list_size(keys); i++) {
        char* key = list_get(keys, i);

        t_archivo* file = dictionary_get(diccionario_archivos, key);
        int old_file_first_block = file->first_block;
        
        // Esto lo voy a usar porque sino se ve que tengo problemas para comparar los strings
        char* name_file = malloc(strlen(file->name_file) + 1);
        strcpy(name_file, file->name_file);

        // Si el archivo no es el que se va a truncar- Detail, ver de usar algo que no sea tan rancio como la misma key adentro del diccionario
        printf("Voy a comparar los strings %s y %s\n", file_a_truncar->name_file, name_file);
        if (strcmp(file_a_truncar->name_file, name_file) != 0) {
            // En funcion de la cantidad de bloques que tenga, agregarlos al bitmap, desde el primer bloque disponbile segun bitmap
            file->first_block = first_block_free();

            marcar_en_bitmap(file->first_block, file->block_count);
            int tamfile = obtener_tamanio_archivo(key);

            // Not tested
            // Copiar los datos del bloque antiguo al nuevo, el new_block_data apunta a una region de memoria que se encuentra en bloques_data, por ende modifica el archivo original
            void* old_block_data = bloques_copia + old_file_first_block * dialfs->block_size;
            void* new_block_data = bloques_data + file->first_block * dialfs->block_size;
            memcpy(new_block_data, old_block_data, tamfile); // Podira ser el tam del archivo --> VER

            // Actualizar el diccionario de archivos
            // dictionary_put(diccionario_archivos, key, file);

            // Actualizar el file de metadata con el nuevo first block
            // int tamfile = obtener_tamanio_archivo(key);
            printf("Tamanio del archivo %s: %d\n", key, tamfile);

            actualizar_file_metadata(key, tamfile, file->first_block); // Key es el namefile del archivo
        }
    }

    printf("Primera etapa de diccionario\n");
    mostrar_diccionario_no_vacio(diccionario_archivos);

    int tamanio_actual = obtener_tamanio_archivo(name_file); // Podria usar tambien el file_truncate->name_file

    // Posibles problemas not tested -> Mismo tamanio para truncar hacia abajo, y si trunco para abajo en 0
    if(tamanio_truncar < tamanio_actual) {
        int bloques_a_remover = calcular_bloques_a_remover(file_a_truncar->block_count, tamanio_truncar);
        // limpiar_bitmap(ultimo_bloque, bloques_a_remover); -> Esto es lo que cambie con al de arriba, ya que se supone que el bitmap arranca limpio
        
        int anterior_first_block = file_a_truncar->first_block;

        file_a_truncar->first_block = first_block_free();
        actualizar_file_metadata(name_file, tamanio_truncar, file_a_truncar->first_block);

        file_a_truncar->block_count -= bloques_a_remover;
        marcar_en_bitmap(file_a_truncar->first_block, file_a_truncar->block_count);

        // Tambien ver la parte de reeescribir los datos!
        void* old_block_data = bloques_copia + anterior_first_block * dialfs->block_size;
        void* new_block_data = bloques_data + file_a_truncar->first_block * dialfs->block_size;
        // Aca para el memcpy hay que tener cuidado porque no quiero copiar todo el archivo, sino solo el tamanio del nuevo file, aca si es obligatorio que sea el tamanio
        memcpy(new_block_data, old_block_data, tamanio_actual); 

        // dictionary_put(diccionario_archivos, nombre_archivo, file);
    } else if (tamanio_truncar >= tamanio_actual) {
        // No voy a usar el truncate_hacia_arriba porque aca necesito hacer otras cosas
        int anterior_first_block = file_a_truncar->first_block;
        file_a_truncar->first_block = first_block_free();

        int bloques_necesarios_add = calcular_bloques_necesarios(file_a_truncar->block_count, tamanio_truncar);
        int bloques_totales = file_a_truncar->block_count + bloques_necesarios_add;
        printf("Bloques totales a agregar: %d\n", bloques_totales);

        // Como no evaluo casos en los que se pase del tamanio total del fs, no deberia tener problemas con el bitmap
        for(int i = file_a_truncar->first_block; i < (file_a_truncar->first_block + bloques_totales); i++) {
            bitarray_set_bit(bitmap, i);
        }
       
        file_a_truncar->block_count += bloques_necesarios_add;

        // Aca hago lo mismo que hacia antes para copiar la informacion
        void* old_block_data = bloques_copia + anterior_first_block * dialfs->block_size;
        void* new_block_data = bloques_data + file_a_truncar->first_block * dialfs->block_size;
        memcpy(new_block_data, old_block_data, tamanio_actual); // Podira ser el tam del archivo --> VER

        // Modificar el tamanio del metadata coincidente
        actualizar_file_metadata(name_file, tamanio_truncar, file_a_truncar->first_block);
    }
    
    printf("Segunda etapa de diccionario\n");
    mostrar_diccionario_no_vacio(diccionario_archivos);

    // Sincronizar los cambios con el archivo original

    msync(bloques_data, dialfs->block_size * dialfs->block_count, MS_SYNC);
    msync(bitmap_data, dialfs->block_count / 8, MS_SYNC);

    log_info(logger_io, "PID: %d - Fin Compactación.", pid);

    // Liberar la copia de bloques.dat
    free(bloques_copia);
}

int calcular_bloques_a_remover(int block_count_file, int tamanio_nuevo) {
    int bloques_a_remover = 0;
    int primer_byte_ultimo_bloque = dialfs->block_size * (block_count_file - 1);

    while (tamanio_nuevo <= primer_byte_ultimo_bloque) {
        bloques_a_remover++;
        block_count_file--;
        primer_byte_ultimo_bloque = (block_count_file -1) * dialfs->block_size;
    }

    return bloques_a_remover;
}

int obtener_tamanio_archivo(char* namefile) {
    char path_completo[256];
    sprintf(path_completo, "%s/%s", dialfs->path_base, namefile);

    FILE* metadata_a_mod;
    metadata_a_mod = fopen(path_completo,"r");

    if (metadata_a_mod == NULL) {
        printf("Error al abrir el archivo.\n");
    } else {
        // Esto esta en revision, pero deberia descartarme la primera linea que es el first_block para ir directo al tamanio
        char discard[256];
        fgets(discard, sizeof(discard), metadata_a_mod);

        int tamanio;
        fscanf(metadata_a_mod, "TAMANIO_ARCHIVO=%d\n", &tamanio);
        fclose(metadata_a_mod);
        return tamanio;
    }
    return -1;
}

void actualizar_file_metadata(char* namefile, int tamanio, int first_block) {
    // Actualizar el file de metadata con el nuevo first block
    char path_completo[256];
    sprintf(path_completo, "%s/%s", dialfs->path_base, namefile);

    FILE* metadata_a_mod;
    metadata_a_mod = fopen(path_completo,"w");

    if (metadata_a_mod == NULL) {
        printf("Error al abrir el archivo.\n");
    } else {
        printf("Se truncó el archivo %s\n", path_completo);
        fprintf(metadata_a_mod, "BLOQUE_INICIAL=%d\n", first_block);
        fprintf(metadata_a_mod, "TAMANIO_ARCHIVO=%d\n", tamanio);
        fflush(metadata_a_mod); 
    }
}

void marcar_en_bitmap(int primer_bloque, int cantidad_bloques) {
    for (int i = primer_bloque; i < primer_bloque + cantidad_bloques; i++) {
        bitarray_set_bit(bitmap, i);
    }
}

int calcular_bloques_necesarios(int bloques_actuales, int tamanio_nuevo) {
    int bloques_necesarios = tamanio_nuevo / dialfs->block_size; 

    if (tamanio_nuevo % dialfs->block_size != 0) {
        bloques_necesarios++; // Agrego el bloque adicional
    }

    return bloques_necesarios - bloques_actuales;
}

/////////////////////////////////////////////////////////////////////
///// DESERIALIZACION|SERIALIZACION|ENVIO|RECEPCION DE PAQUETES /////
/////////////////////////////////////////////////////////////////////

void mandar_valor_a_memoria(char* valor, t_pid_stdin* pid_stdin) {
    // Cuando lo mandamos a memoria, no deberiamos decirle que seguimos vivos?

    t_buffer* buffer = malloc(sizeof(t_buffer));

    int largo_valor = string_length(valor) + 1;

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
    enviar_paquete(buffer, GUARDAR_VALOR, memoriafd);
}

void mandar_valor_a_memoria_fs(t_pedido_rw_encolar* pedido_fs, void* buffer) {
    t_buffer* buffer_memoria = malloc(sizeof(t_buffer));

    int tamanio_buffer = pedido_fs->registro_tamanio; // Tamanio del buffer

    buffer_memoria->size = tamanio_buffer + sizeof(uint32_t) + sizeof(int) * 3 + (sizeof(int) * pedido_fs->cantidad_paginas * 2); 

    buffer_memoria->offset = 0;
    buffer_memoria->stream = malloc(buffer_memoria->size);
    
    memcpy(buffer_memoria->stream + buffer_memoria->offset, &pedido_fs->pid, sizeof(int));
    buffer_memoria->offset += sizeof(int);
    memcpy(buffer_memoria->stream + buffer_memoria->offset, &pedido_fs->registro_tamanio, sizeof(int));
    buffer_memoria->offset += sizeof(uint32_t);
    memcpy(buffer_memoria->stream + buffer_memoria->offset, &pedido_fs->cantidad_paginas, sizeof(int));
    buffer_memoria->offset += sizeof(int);
    memcpy(buffer_memoria->stream + buffer_memoria->offset, &tamanio_buffer, sizeof(int));
    buffer_memoria->offset += sizeof(int);
    memcpy(buffer_memoria->stream + buffer_memoria->offset, buffer, tamanio_buffer);
    buffer_memoria->offset += tamanio_buffer;

    for(int i=0; i < pedido_fs->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pedido_fs->lista_dir_tamanio, i);
        memcpy(buffer_memoria->stream + buffer_memoria->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer_memoria->offset += sizeof(int);
        memcpy(buffer_memoria->stream + buffer_memoria->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer_memoria->offset += sizeof(int);
    }
        
    enviar_paquete(buffer_memoria, ESCRIBIR_MEM_FS, memoriafd);
}

t_pedido_rw_encolar* deserializar_pedido_rw(t_buffer* buffer) {
    t_pedido_rw_encolar* pedido_rw = malloc(sizeof(t_pedido_rw_encolar));

    void* stream = buffer->stream;
    int offset = 0;

    memcpy(&pedido_rw->largo_interfaz, stream + offset, sizeof(int));
    offset += sizeof(int);
    pedido_rw->nombre_interfaz = malloc(pedido_rw->largo_interfaz);
    memcpy(pedido_rw->nombre_interfaz, stream + offset, pedido_rw->largo_interfaz);
    offset += pedido_rw->largo_interfaz;

    memcpy(&pedido_rw->largo_archivo, stream + offset, sizeof(int));
    offset += sizeof(int);
    pedido_rw->nombre_archivo = malloc(pedido_rw->largo_archivo);
    memcpy(pedido_rw->nombre_archivo, stream + offset, pedido_rw->largo_archivo);
    offset += pedido_rw->largo_archivo;

    memcpy(&pedido_rw->registro_direccion, stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&pedido_rw->registro_tamanio, stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&pedido_rw->registro_archivo, stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Recibo cantidad de paginas y la lista
    memcpy(&pedido_rw->cantidad_paginas, stream + offset, sizeof(int));
    offset += sizeof(int);

    pedido_rw->lista_dir_tamanio = list_create();

    for(int i = 0; i < pedido_rw->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio));
        memcpy(&dir_fisica_tam->direccion_fisica, stream + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&dir_fisica_tam->bytes_lectura, stream + offset, sizeof(int));
        offset += sizeof(int);

        list_add(pedido_rw->lista_dir_tamanio, dir_fisica_tam);
    }

    // Aca deserializo tambien los sockets
    memcpy(&pedido_rw->socket_io, stream + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&pedido_rw->socket_memoria, stream + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&pedido_rw->pid, stream + offset, sizeof(int));
    offset += sizeof(int);

    return pedido_rw;
}

t_solicitud_escritura_bloques* deserializar_solicitud_escritura_bloques(t_buffer* buffer) {
    // Aca deserializo el char* y el tamanio del char*
    
    t_solicitud_escritura_bloques* solicitud_escritura = malloc(sizeof(t_solicitud_escritura_bloques));

    void* stream = buffer->stream;

    memcpy(&solicitud_escritura->pid, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&solicitud_escritura->tamanio_a_copiar, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    solicitud_escritura->valor_a_copiar = malloc(solicitud_escritura->tamanio_a_copiar);
    memcpy(solicitud_escritura->valor_a_copiar, stream, solicitud_escritura->tamanio_a_copiar);
    stream += solicitud_escritura->tamanio_a_copiar;
    // Agrego tambien el puntero archivo
    memcpy(&solicitud_escritura->puntero_archivo, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&solicitud_escritura->largo_archivo, stream, sizeof(int));
    stream += sizeof(int);
    solicitud_escritura->nombre_archivo = malloc(solicitud_escritura->largo_archivo);
    memcpy(solicitud_escritura->nombre_archivo, stream, solicitud_escritura->largo_archivo);
    stream += solicitud_escritura->largo_archivo;

    return solicitud_escritura;
}

t_archivo_encolar* deserializar_pedido_creacion_destruccion(t_buffer* buffer) {
    t_archivo_encolar* archivo_a_usar = malloc(sizeof(t_archivo_encolar));
    
    void* stream = buffer->stream;

    // memcpy(&fs_create_delete->pid, stream, sizeof(int));
    // stream += sizeof(int);
    memcpy(&archivo_a_usar->largo_archivo, stream, sizeof(int));
    stream += sizeof(int);
    archivo_a_usar->nombre_archivo = malloc(archivo_a_usar->largo_archivo);
    memcpy(archivo_a_usar->nombre_archivo, stream, archivo_a_usar->largo_archivo);
    stream += archivo_a_usar->largo_archivo;
    memcpy(&archivo_a_usar->pid, stream, sizeof(int));
    stream += sizeof(int);

    return archivo_a_usar;
}

t_pedido_truncate* deserializar_pedido_fs_truncate(t_buffer* buffer) {
    t_pedido_truncate* pedido_truncate = malloc(sizeof(t_pedido_truncate));
    void* stream = buffer->stream;

    memcpy(&pedido_truncate->largo_nombre_archivo, stream, sizeof(int));
    stream += sizeof(int);
    pedido_truncate->nombre_archivo = malloc(pedido_truncate->largo_nombre_archivo);
    memcpy(pedido_truncate->nombre_archivo, stream, pedido_truncate->largo_nombre_archivo);
    stream += pedido_truncate->largo_nombre_archivo;
    memcpy(&pedido_truncate->registro_tamanio, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pedido_truncate->pid, stream, sizeof(int));
    stream += sizeof(int);

    return pedido_truncate;
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

    // pid_stdin->lista_direcciones = malloc(pid_stdin->cantidad_paginas);
    pid_stdin->lista_direcciones = list_create();
    
    for(int i=0; i < pid_stdin->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio));
        memcpy(&dir_fisica_tam->direccion_fisica, stream, sizeof(int));
        stream += sizeof(int);
        memcpy(&dir_fisica_tam->bytes_lectura, stream, sizeof(int));
        stream += sizeof(int);
        list_add(pid_stdin->lista_direcciones, dir_fisica_tam);
        // free(dir_fisica_tam); // LIBERAR CUANDO SE DESCONECTE LA IO
    }
    return pid_stdin;
}

//////////////////////////////
///// FUNCIONES DE AYUDA /////
//////////////////////////////

void imprimir_datos_pedido_escritura(t_pedido_rw_encolar* pedido_escritura) {
    printf("Nombre archivo: %s\n", pedido_escritura->nombre_archivo);
    printf("Registro direccion: %d\n", pedido_escritura->registro_direccion);
    printf("Registro tamanio: %d\n", pedido_escritura->registro_tamanio);
    printf("Registro archivo: %d\n", pedido_escritura->registro_archivo);
    printf("Cantidad de paginas: %d\n", pedido_escritura->cantidad_paginas);

    for(int i = 0; i < pedido_escritura->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pedido_escritura->lista_dir_tamanio, i);
        printf("Direccion fisica: %d\n", dir_fisica_tam->direccion_fisica);
        printf("Bytes lectura: %d\n", dir_fisica_tam->bytes_lectura);
    }
}

void mostrar_diccionario_no_vacio(t_dictionary* diccionario) {
    if (dictionary_size(diccionario) > 0) {
        dictionary_iterator(diccionario, (void*)mostrar_archivo);
    } else {
        printf("El diccionario está vacío.\n");
    }
}

void mostrar_archivo(char* key, void* value) {
    t_archivo* archivo = (t_archivo*) value;
    printf("Primer bloque: %d\n", archivo->first_block);
    printf("Cantidad de bloques: %d\n", archivo->block_count);
    printf("Nombre del archivo: %s\n", archivo->name_file);
}

int byte_de_primer_bloque(char* nombre_archivo) {
    t_archivo* file = dictionary_get(diccionario_archivos, nombre_archivo);
    return file->first_block * dialfs->block_size;
}

//
////////////////////
///// LIMPIEZA /////
////////////////////


// Es parecida a la otra pero sin remover la entrada del diccionario
void limpiar_bitmap(int ultimo_bloque, int bloques_a_remover) {
    for (int i = ultimo_bloque; i > (ultimo_bloque - bloques_a_remover); i--) {
        bitarray_clean_bit(bitmap, i);
    }
}

void liberar_pid_stdin(t_pid_stdin* pid_stdin) {
    for(int i = 0; i < list_size(pid_stdin->lista_direcciones); i++) {
        t_dir_fisica_tamanio* elemento = list_get(pid_stdin->lista_direcciones, i);
        free(elemento);
    }

    list_destroy(pid_stdin->lista_direcciones);
    
    free(pid_stdin);
}
