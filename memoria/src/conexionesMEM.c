#include "../include/conexionesMEM.h"

pthread_t cpu_thread;
pthread_t kernel_thread;
pthread_t io_stdin_thread;
pthread_t io_stdout_thread;
pthread_mutex_t mutex_diccionario_instrucciones;
pthread_mutex_t mutex_diccionario_io;
t_config* config_memoria;
t_dictionary* diccionario_instrucciones;
t_dictionary* diccionario_tablas_paginas;
t_dictionary* diccionario_io;
char* path_config;
int tamanio_pagina;
int tamanio_memoria;
void* espacio_usuario;
int cant_frames;
int tiempo_retardo;
//sem_t* hay_valores_para_leer;
t_bitarray* bitmap;
int socket_cpu;
t_log* logger_memoria;

//Acepta el handshake del cliente, se podria hacer mas generico y que cada uno tengadiferente
int esperar_cliente(int socket_servidor, t_log* logger_memoria) {
	int handshake = 0;
	int resultError = -1;
	int socket_cliente = accept(socket_servidor, NULL, NULL);

    if(socket_cliente == -1) {
        return -1;
    }
    
	recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL); 
    printf("recibi este handshake: %d\n", handshake);

	if(handshake == 1) {
        // pthread_t kernel_thread;
        pthread_create(&kernel_thread, NULL, (void*)handle_kernel, (void*)(intptr_t)socket_cliente);
    } else if(handshake == 2) {
        // pthread_t cpu_thread;
        pthread_create(&cpu_thread, NULL, (void*)handle_cpu, (void*)(intptr_t)socket_cliente);
    } else if(handshake == 91) {
        // pthread_t cpu_thread;
        pthread_create(&io_stdin_thread, NULL, (void*)handle_io_stdin, (void*)(intptr_t)socket_cliente); // Ver
        printf("Se creo el hilo STDIN\n");
    } else if(handshake == 79) {
        // pthread_t cpu_thread;
        pthread_create(&io_stdout_thread, NULL, (void*)handle_io_stdout, (void*)(intptr_t)socket_cliente); // Ver
        printf("Se creo el hilo STDOUT\n");
    } else if(handshake == 81) { 
        pthread_create(&io_stdout_thread, NULL, (void*)handle_io_dialfs, (void*)(intptr_t)socket_cliente);
        printf("Se creo el hilo DIALFS\n");
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        close(socket_cliente);
        return -1;
    }

	return socket_cliente;
}


// Ejemplo de esto numerico considerando un tam_pagina = 4bytes
/*

Tamanio total en cantidad de bytes x frames = 12 bytes
Tamanio solicitado = 10 bytes

10 DEBE ser > a 12 - 4 (tam_pagina) = 8 y 10 < 12 (total bytes x frame)

*/

void* handle_io_dialfs(void* socket) {
    int socket_io = (intptr_t) socket;
    int resultOk = 0;
    printf("Llega a handle_io_fs\n");
    send(socket_io, &resultOk, sizeof(int), 0);

    t_paquete* paquete_inicial = inicializarIO_recibirPaquete(socket_io);
    agregar_interfaz_en_el_diccionario(paquete_inicial, socket_io);

    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete de IO\n");
        //recv(socket_cpu, &(paquete->codigo_operacion), sizeof(int), 0);
        //sem_wait(&sem_memoria_instruccion);
        recv(socket_io, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de IO: %d\n", paquete->codigo_operacion);

        // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
        //recv(socket_cpu, &(paquete->buffer->size), sizeof(int), 0);
        recv(socket_io, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        //recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(socket_io, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        void* stream = paquete->buffer->stream;

        switch(paquete->codigo_operacion) { 
            case ESCRIBIR_MEM_FS:                  
                // Aca voy a deserializar todo lo que me manda el fs y lo voy a escribir en memoria
                t_memoria_fs_escritura_lectura* escritura_lectura = deserializar_escritura_lectura_fs(paquete->buffer);
                /* 
                    void* registro_escritura = pedido_operacion->valor_a_escribir;

                realizar_operacion(pedido_operacion->pid, ESCRITURA, direcciones_restantes_escritura, user_space_aux, registro_escritura, NULL); 
                // printf("Registro escrito como int %d\n", *((uint32_t*)registro_escritura));

                send(socket_cpu, &confirm_finish, sizeof(uint32_t), 0);
                */
                break;
            default:
                printf("Rompio todo?\n");
                return NULL;
            }
    }

    liberar_paquete(paquete_inicial);
    return NULL;
}

bool check_same_page(int tamanio, int cant_bytes_uso) { 
    // en RESIZE 960 devuelve false 
    return tamanio > (cant_bytes_uso - tamanio_pagina) && tamanio < cant_bytes_uso;
}

void* handle_cpu(void* socket) { // Aca va a pasar algo parecido a lo que pasa en handle_kernel, se va a recibir peticiones de cpu y se va a hacer algo con ellas (iternado switch)
    socket_cpu = (intptr_t)socket;
    // free(socket); 
    int resultOk = 0;
    int confirm_finish = 1;
    // Envio confirmacion de handshake!
    send(socket_cpu, &resultOk, sizeof(int), 0);
    printf("Se conecto un el cpu!\n");

    send(socket_cpu, &tamanio_pagina, sizeof(int), MSG_WAITALL);
    
    while(1) {
        
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));
        
        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete de CPU\n");
        //recv(socket_cpu, &(paquete->codigo_operacion), sizeof(int), 0);
        //sem_wait(&sem_memoria_instruccion);
        recv(socket_cpu, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de CPU: %d\n", paquete->codigo_operacion);

        // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
        //recv(socket_cpu, &(paquete->buffer->size), sizeof(int), 0);
        recv(socket_cpu, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        //recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        // void* stream = paquete->buffer->stream;
        void* user_space_aux;

        // Evaluamos el codigo de operacion
        switch(paquete->codigo_operacion) { 
             case RESIZE_MEMORIA: 
                // int tamanio, pid;
                int devolucion_resize_ok = -1;

                devolucion_resize_ok = resize_memory(paquete->buffer->stream);
                send(socket_cpu, &devolucion_resize_ok, sizeof(int), 0);
                break;
            case QUIERO_FRAME:
                // Aca abstraigo para recibir el frame
                quiero_frame(paquete->buffer); 
                break;
            case QUIERO_INSTRUCCION:
                t_solicitud_instruccion* solicitud_cpu = deserializar_solicitud_instruccion(paquete->buffer);
                usleep(tiempo_retardo * 1000); // Harcodeado nashe para probar (Con retardo)
                enviar_instruccion(solicitud_cpu, socket_cpu);
                free(solicitud_cpu);
                break;
            case QUIERO_CANTIDAD_INSTRUCCIONES:
                //t_cantidad_instrucciones* cantidad_instrucciones = deserializar_cantidad(paquete->buffer);
                int pid_int = deserializar_pid(paquete->buffer);
                char* pid_string = string_itoa(pid_int);
                printf("La CPU me pide la cantidad de instrucciones del proceso con pid %s.\n", pid_string);
                // enviar_cantidad_parametros(socket_cpu);
                enviar_cantidad_instrucciones_pedidas(pid_string, socket_cpu);
                // free(cantidad_instrucciones);
                free(pid_string);
                break;
            case LEER_DATO_MEMORIA:
                user_space_aux = espacio_usuario;

                t_list* direcciones_restantes_mov_in = list_create(); 
                t_pedido_memoria* pedido_op = deserializar_direccion_fisica(paquete->buffer, direcciones_restantes_mov_in); // Le puse pedido_op para no repertir nombre, seria la operacion

                // Seteos por las dudas
                // pedido_op->valor_a_escribir = NULL;
                
                printf("dir_fisica->tamanio total calculado : %d\n", pedido_op->length_valor);

                void* registro_lectura = malloc(pedido_op->length_valor);
            
                realizar_operacion(pedido_op->pid, LECTURA, direcciones_restantes_mov_in, user_space_aux, NULL, registro_lectura);

                // log_info("Contenido completo de registro_completo: %s", (char*)registro_completo);
                send(socket_cpu, registro_lectura, pedido_op->length_valor, 0);
                
                list_destroy(direcciones_restantes_mov_in);

                free(pedido_op->valor_a_escribir);
                free(pedido_op);
                free(registro_lectura);
                break;
            case ESCRIBIR_DATO_EN_MEM: 
                user_space_aux = espacio_usuario;
                confirm_finish = 1;

                // recibir_datos_escritura();
                t_list* direcciones_restantes_escritura = list_create();

                t_pedido_memoria* pedido_operacion = deserializar_direccion_fisica(paquete->buffer, direcciones_restantes_escritura);

                void* registro_escritura = pedido_operacion->valor_a_escribir;

                realizar_operacion(pedido_operacion->pid, ESCRITURA, direcciones_restantes_escritura, user_space_aux, registro_escritura, NULL); 
                // printf("Registro escrito como int %d\n", *((uint32_t*)registro_escritura));

                send(socket_cpu, &confirm_finish, sizeof(uint32_t), 0);
                
            
                list_destroy(direcciones_restantes_escritura);            
                free(pedido_operacion->valor_a_escribir);
                free(pedido_operacion);
                             
                break;
            default:
                printf("No reconozco ese cod-op...\n"); 
                // liberar_paquete(paquete);  // -> Comento y libero despues de cada iteracion
                return NULL;
        }

        liberar_paquete(paquete);
    }

    return NULL;
}


void realizar_operacion(int pid, tipo_operacion operacion, t_list* direcciones_restantes, void* user_space_aux, void* registro_escritura, void* registro_lectura) {

    // printf("Antes de entrar al for, necesito %d paginas\n", df->cantidad_paginas);
    int tamanio_anterior = 0;

    while(list_size(direcciones_restantes) > 0) { 
        t_dir_fisica_tamanio* dir_fisica_tam = list_remove(direcciones_restantes, 0);
        printf("La direccion fisica es: %d\n", dir_fisica_tam->direccion_fisica);
        printf("El tamanio es: %d\n", dir_fisica_tam->bytes_lectura);
        // printf("el registro escritura es : %s \n", (char*)registro_escritura);

        interaccion_user_space(pid, operacion, dir_fisica_tam->direccion_fisica, user_space_aux, tamanio_anterior, dir_fisica_tam->bytes_lectura, registro_escritura, registro_lectura);
        tamanio_anterior += dir_fisica_tam->bytes_lectura;

        free(dir_fisica_tam);
    }
}

void interaccion_user_space(int pid, tipo_operacion operacion, int df_actual, void* user_space_aux, int tam_escrito_anterior, int tamanio, void* registro_escritura, void* registro_lectura) {
    if(operacion == ESCRITURA) {
        memcpy(user_space_aux + df_actual, registro_escritura + tam_escrito_anterior, tamanio);
        printf("Escritura: df_actual=%d, tam_escrito_anterior=%d, tamanio=%d\n", df_actual, tam_escrito_anterior, tamanio);
        printf("Contenido escrito: %.*d\n", tamanio, *(uint8_t*)(user_space_aux + df_actual));
        log_info(logger_memoria, "Contenido escrito:  %.*d", tamanio, *(uint8_t*)(user_space_aux + df_actual));
        log_info(logger_memoria, "El PID del contenido escrito es: %d", pid);
    } else { // == LECTURA
        memcpy(registro_lectura + tam_escrito_anterior, (user_space_aux + df_actual), tamanio);
        printf("Lectura: df_actual=%d, tam_escrito_anterior=%d, tamanio=%d\n", df_actual, tam_escrito_anterior, tamanio);
        printf("Contenido leido: %.*d\n", tamanio, *(uint8_t*)(registro_lectura + tam_escrito_anterior));
    }
}                                                                                                                                 
     
char* string_tipo_operacion(tipo_operacion operacion){
    switch(operacion){
        case ESCRITURA: return "ESCRITURA";
        case LECTURA: return "LECTURA";
        default:
            log_error(logger_memoria,"Rompio algo, llego un tipo operacion null");
            exit(1);
    }
}

void quiero_frame(t_buffer* buffer) {
    t_solicitud_frame* pedido_frame = deserializar_solicitud_frame(buffer);
    log_info(logger_memoria, "El pid que me pide frame es: %d\n", pedido_frame->pid);

    int frame = buscar_frame(pedido_frame);

    /* 
    if(frame == -1) {
        int frame_libre = buscar_frame_libre();

        if (frame_libre != -1) {  // Si se encontró un frame libre
            bitarray_set_bit(bitmap, frame_libre);
        }
        send(socket_cpu, &frame_libre, sizeof(int), 0);
    }
    */

    send(socket_cpu, &frame, sizeof(int), 0);
    
    free(pedido_frame);
}

t_pedido_memoria* deserializar_direccion_fisica(t_buffer* buffer, t_list* direcciones_restantes) {
    t_pedido_memoria* datos_operacion = malloc(sizeof(t_pedido_memoria));

    void* stream = buffer->stream;

    memcpy(&datos_operacion->pid, stream, sizeof(int));
    stream += sizeof(int);

    memcpy(&datos_operacion->cantidad_paginas, stream, sizeof(int));
    stream += sizeof(int);
    
    memcpy(&datos_operacion->length_valor, stream, sizeof(int));
    stream += sizeof(int);

    datos_operacion->valor_a_escribir = malloc(datos_operacion->length_valor);
    memcpy(datos_operacion->valor_a_escribir, stream, datos_operacion->length_valor);
    stream += datos_operacion->length_valor;
   // printf("el valor a escribir en deserializar dir fisica es %d \n", datos_operacion->valor_a_escribir);

    printf("cant paginas: %d\n", datos_operacion->cantidad_paginas);
    for(int i = 0; i < datos_operacion->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio)); //HECHO EN TEORIA
        memcpy(&dir_fisica_tam->direccion_fisica, stream, sizeof(int));
        stream += sizeof(int);
        printf("Direccion fisica que llega desde CPU: %d\n", dir_fisica_tam->direccion_fisica);
        memcpy(&dir_fisica_tam->bytes_lectura, stream, sizeof(int));
        stream += sizeof(int);
        printf("Bytes de lectura: %d\n", dir_fisica_tam->bytes_lectura);

        list_add(direcciones_restantes, dir_fisica_tam);
    }

    return datos_operacion;
}

int resize_memory(void* stream) {
    int tamanio, pid;

    memcpy(&tamanio, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&pid, stream, sizeof(int));
    // Luego verifico si tengo espacio suficiente en memoria
    int out_of_memory = validar_out_of_memory(tamanio);

    if(out_of_memory) {
        printf("PID: %d - No hay espacio suficiente en memoria\n", pid);
        return out_of_memory; // 1
    }
    
    char* pid_char = string_itoa(pid);
    int cant_bytes_uso = cantidad_frames_proceso(pid_char);
    
    // esta funcion se fija si la cant de bytes que pidio (tamanio) no implica aumentar o recortar la cant de frames que le dimos
    bool same_page = check_same_page(tamanio, cant_bytes_uso);

    if(same_page) {
       printf("El proceso ya tiene una pagina con el tamanio solicitado\n");
    } else if(tamanio > cant_bytes_uso) {
        log_info(logger_memoria,"PID: %d - Tamaño Actual: %d - Tamaño a Ampliar: %d",pid,cant_bytes_uso,tamanio);
        asignar_tamanio(tamanio, cant_bytes_uso, pid_char);
    } else {
        log_info(logger_memoria,"PID: %d - Tamaño Actual: %d - Tamaño a Reducir: %d",pid,cant_bytes_uso,tamanio);
        recortar_tamanio(tamanio, pid_char, cant_bytes_uso);
    }
    
    printear_paginas(pid_char);
    printear_bitmap();
    free(pid_char);

    return 0; //hasta aca, la op salio bien :)
}

void recortar_tamanio(int tamanio, char* pid_string, int cant_bytes_uso) {
    int tamanio_a_compactar = cant_bytes_uso - tamanio;
    printf("Tamanio a compactar: %d\n", tamanio_a_compactar);
    
    // consideramos que no hay que sacarle mas paginas en caso de que sobren bytes menores al tamanio de una pagina. PREGUNTAR
    while(tamanio_a_compactar >= tamanio_pagina) { 
        /************  Si por un byte sobrante (o varias menos al tam_pagina) tengo que sacar toda otra pagina entonces hago "tamanio_a_compactar > 0" *************/
        t_proceso_paginas* proceso_pagina = dictionary_get(diccionario_tablas_paginas, pid_string);
        t_pagina* pagina = list_remove(proceso_pagina->tabla_paginas, list_size(proceso_pagina->tabla_paginas) - 1);

        if (pagina != NULL) {  // Marco el frame como libre y le resto un frame
            proceso_pagina->cantidad_frames--;
            bitarray_clean_bit(bitmap, pagina->frame); 
            free(pagina);
        }

        tamanio_a_compactar -= tamanio_pagina;
    }
}

void printear_paginas(char* pid_char) {
    t_proceso_paginas* proceso_pagina = dictionary_get(diccionario_tablas_paginas, pid_char);
    t_list* tabla_paginas = proceso_pagina->tabla_paginas;

    for(int i = 0; i < list_size(tabla_paginas); i++) { // Obviemos por ahora el list_iterate
        t_pagina* pagina = list_get(tabla_paginas, i);
        printf("Pagina: %d\n", pagina->numero_pagina);
        printf("Frame: %d\n", pagina->frame);
    }
}

// Funcion expermiental (con limite en 10), porque sino me imprime una locura de bits (tam max)
void printear_bitmap() {
    for(int i = 0; i < 10; i++) {
        printf("El bit es : %d\n", bitarray_test_bit(bitmap, i));
    }
}

// Caso tengo 12 me pide 17 con paginas de 4. Hago 17 - 12 = 5. 5 < 4. Entonces busco 1 frame libre.
void asignar_tamanio(int tamanio, int cant_bytes_uso, char* pid_string) {
    while(tamanio - cant_bytes_uso > 0) { // <= 0
        int frame_libre = buscar_frame_libre();

        if (frame_libre != -1) {  // Si se encontró un frame libre
            bitarray_set_bit(bitmap, frame_libre); 

            t_proceso_paginas* proceso_pagina = dictionary_get(diccionario_tablas_paginas, pid_string);
            proceso_pagina->cantidad_frames++;
            marcar_frame_en_tabla_paginas(proceso_pagina->tabla_paginas, frame_libre);
        }

        tamanio -= tamanio_pagina;
    }
}

void marcar_frame_en_tabla_paginas(t_list* tabla_paginas, int frame) {
    t_pagina* pagina = malloc(sizeof(t_pagina));
    pagina->numero_pagina = list_size(tabla_paginas); 
    pagina->frame = frame;
    list_add(tabla_paginas, pagina); //VER_SI_FUNCIONA
}

int buscar_frame_libre() {
    // Tomo la cant_frames ya que no quiero tomar los bits adicionales que me sobren del bitmap 
    for (size_t i = 0; i < cant_frames; i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            return i;  // Devuelve el índice del primer frame libre encontrado
        }
    }

    return -1;  // No hay frames libres
}

int cantidad_frames_proceso(char* pid_string) {
    t_proceso_paginas* proceso_pagina = dictionary_get(diccionario_tablas_paginas, pid_string);
    return proceso_pagina->cantidad_frames * tamanio_pagina;
}

int validar_out_of_memory(int tamanio) { // Me esta pidiendo mas de lo que tengo /= Te pasaste del ultimo byte disponible
    int frames_necesarios = tamanio / tamanio_pagina; // 8
    int frames_disponibles = contar_frames_libres(); 

    if(frames_disponibles < frames_necesarios) {
        // printf("No hay espacio suficiente en memoria\n");
        return 1; // 1 es out of memory
        // send(socket_cpu, &out_of_memory, sizeof(int), 0);
    }
    return 0;
}

int contar_frames_libres(/* bit map es global*/) {
    int free_frames = 0;
    // size_t max_bits = bitarray_get_max_bit(bitmap);

    for (size_t i = 0; i < cant_frames; i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            free_frames++;
        }
    }

    return free_frames;
}

int buscar_frame(t_solicitud_frame* pedido_frame) {
    printf("Busco el frame con la pagina %d\n", pedido_frame->nro_pagina);
    log_info(logger_memoria, "BUSCO EL FRAME CON LA PAGINA: %d", pedido_frame->nro_pagina);
    log_info(logger_memoria, "BUSCO EL FRAME CORRESPONDIENTE AL PID: %d", pedido_frame->pid);

    char* pid_string = string_itoa(pedido_frame->pid);

    printf("El pid despues de la conversion es: %s\n", pid_string);

    t_proceso_paginas* proceso_paginas = dictionary_get(diccionario_tablas_paginas, pid_string); // el diccionario tiene como key el pid y la lista de pags asociado a ese pid

    free(pid_string);

    if(proceso_paginas == NULL) {
        printf("No existe el proceso con pid %d o rompio otra cosa\n", pedido_frame->pid);
        log_info(logger_memoria, "No existe el proceso con pid %d o rompio otra cosa", pedido_frame->pid);
        return -1; // Si no existe el proceso (pid) en la memoria (diccionario_tablas_paginas
    }

    t_list* tabla_paginas = proceso_paginas->tabla_paginas;

    log_info(logger_memoria, "La cantidad de entradas que tiene la memoria es: %d", list_size(proceso_paginas->tabla_paginas));
    
    for(int i=0;i < list_size(tabla_paginas); i++) {
        t_pagina* pagina = list_get(tabla_paginas, i);

        printf("La pagina es: %d\n", pagina->numero_pagina);
        printf("El frame es: %d\n", pagina->frame);

        if(pagina->numero_pagina == pedido_frame->nro_pagina){
            // int numero_frame = *(int*) list_get(tabla_paginas, pedido_frame->nro_pagina);
            log_info(logger_memoria, "el numero de frame es %d", pagina->frame);
            return pagina->frame;
        }
    }

    return -1; // Si no lo encontre el frame
}

t_solicitud_frame* deserializar_solicitud_frame(t_buffer* buffer) {
    t_solicitud_frame* solicitud_frame = malloc(sizeof(t_solicitud_frame));

    void* stream = buffer->stream;

    memcpy(&solicitud_frame->nro_pagina, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&solicitud_frame->pid, stream, sizeof(int));
    stream += sizeof(int);

    return solicitud_frame;
}

// Ver error
void* handle_kernel(void* socket) {
    int socket_kernel = (intptr_t)socket;

    int resultOk = 0;
    send(socket_kernel, &resultOk, sizeof(int), 0);
    printf("Se conecto el kernel!\n");

    char* path_completo;

    while(1) {
        void* user_space_aux;

        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete de kernel\n");
        //recv(socket_kernel, &(paquete->codigo_operacion), sizeof(int), 0);
        recv(socket_kernel, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de kernel: %d\n", paquete->codigo_operacion);

        // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
        //recv(socket_kernel, &(paquete->buffer->size), sizeof(int), 0);
        recv(socket_kernel, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        //recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        // Ahora en función del código recibido procedemos a deserializar el resto
        switch(paquete->codigo_operacion) {
            case PATH:
                // Ver si no es mejor recibir primero el cod-op y adentro recibimos el resto (Cambiaria la forma de serializar, ya no habira)
                t_path* path = deserializar_path(paquete->buffer);
                imprimir_path(path);
                path_completo = agrupar_path(path); // Ojo con el SEG_FAULT
                printf("CUIDADO QUE IMPRIMO EL PATH: %s\n", path_completo);

                crear_estructuras(path_completo, path->PID);
                free(path_completo);
                int se_crearon_estructuras = 1;
                send(socket_kernel, &se_crearon_estructuras, sizeof(int), 0);

                printf("Cuidado que voy a imprimir el diccionario...\n");
                pthread_mutex_lock(&mutex_diccionario_instrucciones);
                imprimir_diccionario(); // Imprime todo, sin PID especifico...
                pthread_mutex_unlock(&mutex_diccionario_instrucciones);
            
                free(path->path);
                free(path);
                break;
            case ESCRIBIR_STDOUT:
                int respuesta_conexion;
                int validacion_conexion = 1;

                t_pid_stdout* pid_stdout = desearializar_pid_stdout(paquete->buffer); // bien

                printf("\nEste es el nombre de la interfaz: %s\n", pid_stdout->nombre_interfaz);

                pthread_mutex_lock(&mutex_diccionario_io);
                socket_estructurado* socket_io = dictionary_get(diccionario_io, pid_stdout->nombre_interfaz);
                pthread_mutex_unlock(&mutex_diccionario_io);

                send(socket_io->socket, &validacion_conexion, sizeof(int), 0);
                int result_io = recv(socket_io->socket, &respuesta_conexion, sizeof(int), 0);
                printf("resultado recv: %d\n", result_io);

                int result_kernel = 0;
                
                if(result_io == 0) {
                    result_kernel = -1;
                    desconectar_io_de_diccionario(pid_stdout->nombre_interfaz);
                } else {
                    user_space_aux = espacio_usuario;
                
                    printf("dir_fisica->tamanio total calculado : %d\n", pid_stdout->registro_tamanio);

                    void* registro_lectura = malloc(pid_stdout->registro_tamanio);
                    log_info(logger_memoria, "El tamanio del valor leido* es: %d\n", pid_stdout->registro_tamanio);

                    realizar_operacion(pid_stdout->pid, LECTURA, pid_stdout->lista_direcciones, user_space_aux, NULL, registro_lectura);
                    log_info(logger_memoria, "el valor del registro lectura es :%s", (char*)registro_lectura);
                    
                    char* registro_string = malloc(pid_stdout->registro_tamanio + 1);
                    memcpy(registro_string, registro_lectura, pid_stdout->registro_tamanio);
                    registro_string[pid_stdout->registro_tamanio] = '\0';
                    
                    printf("El valor leido para char* es: %s\n", registro_string);

                    printf("El pid que mandaremos a la io es %d\n", pid_stdout->pid);
                    printf("El socket es %d\n", socket_io->socket);

                    enviar_valor_leido_a_io(pid_stdout->pid, socket_io->socket, registro_string, pid_stdout->registro_tamanio);
                    
                    free(registro_string);
                    free(registro_lectura);
                }

                send(socket_kernel, &result_kernel, sizeof(int), 0);

                list_destroy(pid_stdout->lista_direcciones);
                free(pid_stdout->nombre_interfaz);
                free(pid_stdout);
                break;
            case ESCRIBIR_FS_MEMORIA: // Lee en memoria y envia a fs
                int validacion_conexion_fs = 1;
                int respuesta_conexion_fs;

                t_pedido_rw_encolar* pedido_write = deserializar_pedido_lectura_escritura_mem(paquete->buffer);

                imprimir_datos_pedido_lectura(pedido_write);

                sleep(5);

                user_space_aux = espacio_usuario;

                void* registro_lectura_rw = malloc(pedido_write->registro_tamanio);

                realizar_operacion(pedido_write->pid, LECTURA, pedido_write->lista_dir_tamanio, user_space_aux, NULL, registro_lectura_rw);

                printf("El valor leido para char* es: %s\n", (char*)registro_lectura_rw);

                int size_registro = strlen((char*)registro_lectura_rw) + 1;

                pthread_mutex_lock(&mutex_diccionario_io);
                socket_estructurado* socket_io_fs = dictionary_get(diccionario_io, pedido_write->nombre_interfaz);
                pthread_mutex_unlock(&mutex_diccionario_io);
                
                printf("Comparacion entre los 2 sockets: %d - %d\n", socket_io_fs->socket, pedido_write->socket_io);

                send(socket_io_fs->socket, &validacion_conexion_fs, sizeof(int), 0);
                int result_io_fs = recv(socket_io_fs->socket, &respuesta_conexion_fs, sizeof(int), 0);

                printf("resultado recv: %d\n", result_io_fs);

                int result_kernel_fs = 0;

                if(result_io_fs == 0) {
                    int result_kernel = -1;
                    desconectar_io_de_diccionario(pedido_write->nombre_archivo); // Tengo mis dudas de si se agrega al diccionario pero creo que s
                } else {
                    printf("VOY A MANDAR A FS\n");  
                    mandar_lectura_a_fs(registro_lectura_rw, pedido_write, size_registro, socket_io_fs->socket);
                }

                send(socket_kernel, &result_kernel_fs, sizeof(int), 0);
                // mandar_lectura_a_fs(registro_lectura, pedido_write->socket_io);

                // Una vez leido, tiene que enviar a fs toda la info para que esta lo puedo copiar el en archivo, pero no sin antes validar la conexion
                
                free(pedido_write);
                free(registro_lectura_rw);
                break;
            case LIBERAR_PROCESO:
                printf("\nLlega a liberar_proceso\n\n");
                int pid_a_liberar;
                memcpy(&pid_a_liberar, paquete->buffer->stream, sizeof(int));
                liberar_estructuras_proceso(pid_a_liberar);
                // eliminar_instrucciones(pid_a_liberar);
                limpiar_bitmap(pid_a_liberar);

                break;
            default:
                printf("Rompio kernel.\n");
                liberar_modulo_memoria();
    
                exit(-1);
                break;
        }

        // Liberamos memoria
        liberar_paquete(paquete);
    }

    return NULL;
}

void mandar_lectura_a_fs(void* registro_lectura, t_pedido_rw_encolar* pedido_fs, int size_registro, int socket_io) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 2 + size_registro + sizeof(uint32_t) + pedido_fs->largo_archivo;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &size_registro, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, registro_lectura, size_registro);
    buffer->offset += size_registro;
    memcpy(stream + buffer->offset, &pedido_fs->registro_archivo, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &pedido_fs->largo_archivo, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, pedido_fs->nombre_archivo, pedido_fs->largo_archivo);
    buffer->offset += pedido_fs->largo_archivo;

    enviar_paquete(buffer, ESCRIBIR_BLOQUES, socket_io);
}

void desconectar_io_de_diccionario(char* nombre_interfaz) {
    printf("Se desconecto la IO\n");

    pthread_mutex_lock(&mutex_diccionario_io);
    socket_estructurado* socket_a_remover = dictionary_remove(diccionario_io, nombre_interfaz);
    pthread_mutex_unlock(&mutex_diccionario_io);

    // Ojo! Aca tambien hay que liberar todo lo que no se usa cuando la io se termina
    close(socket_a_remover->socket);
    free(socket_a_remover);
}

void imprimir_datos_pedido_lectura(t_pedido_rw_encolar* pedido) {
    printf("El largo de la interfaz es: %d\n", pedido->largo_interfaz);

    printf("INTERFACE NAME IS: %s\n", pedido->nombre_interfaz);

    printf("El largo del archivo es: %d\n", pedido->largo_archivo);
    printf("El nombre del archivo es: %s\n", pedido->nombre_archivo);
    printf("El registro direccion es: %d\n", pedido->registro_direccion);
    printf("El registro tamanio es: %d\n", pedido->registro_tamanio);
    printf("El registro archivo es: %d\n", pedido->registro_archivo);
    printf("La cantidad de paginas es: %d\n", pedido->cantidad_paginas);
    printf("El pid es: %d\n", pedido->pid);

    for(int i = 0; i < pedido->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(pedido->lista_dir_tamanio, i);
        printf("La direccion fisica es: %d\n", dir_fisica_tam->direccion_fisica);
        printf("El tamanio es: %d\n", dir_fisica_tam->bytes_lectura);
    }

    printf("El socket de io es: %d\n", pedido->socket_io);
    printf("El socket de memoria es: %d\n", pedido->socket_memoria);
}

t_pedido_rw_encolar* deserializar_pedido_lectura_escritura_mem(t_buffer* buffer) {
    t_pedido_rw_encolar* pedido_fs = malloc(sizeof(t_pedido_rw_encolar));

    void* stream = buffer->stream;

    memcpy(&pedido_fs->largo_interfaz, stream, sizeof(int));
    stream += sizeof(int);
    pedido_fs->nombre_interfaz = malloc(pedido_fs->largo_interfaz);
    memcpy(pedido_fs->nombre_interfaz, stream, pedido_fs->largo_interfaz);
    stream += pedido_fs->largo_interfaz;

    memcpy(&pedido_fs->largo_archivo, stream, sizeof(int));
    stream += sizeof(int);
    pedido_fs->nombre_archivo = malloc(pedido_fs->largo_archivo);
    memcpy(pedido_fs->nombre_archivo, stream, pedido_fs->largo_archivo);
    stream += pedido_fs->largo_archivo;
    memcpy(&pedido_fs->registro_direccion, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pedido_fs->registro_tamanio, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pedido_fs->registro_archivo, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&pedido_fs->cantidad_paginas, stream, sizeof(int));
    stream += sizeof(int);

    pedido_fs->lista_dir_tamanio = list_create();

    for(int i=0; i < pedido_fs->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio));
        memcpy(&(dir_fisica_tam->direccion_fisica), stream, sizeof(int));
        stream += sizeof(int);
        memcpy(&(dir_fisica_tam->bytes_lectura), stream, sizeof(int));
        stream += sizeof(int);
        list_add(pedido_fs->lista_dir_tamanio, dir_fisica_tam);
    }
    
    memcpy(&pedido_fs->socket_io, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&pedido_fs->socket_memoria, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&pedido_fs->pid, stream, sizeof(int));

    return pedido_fs;
}

/* 
t_list* instrucciones = list_create();

    t_proceso_paginas* proceso_paginas = malloc(sizeof(t_proceso_paginas));
*/
void liberar_estructuras_proceso(int pid) {
    /*char pid_char[4];
    sprintf(pid_char, "%d", pid);*/
    char* pid_char = string_itoa(pid);

    pthread_mutex_lock(&mutex_diccionario_instrucciones);

    t_list* instrucciones = dictionary_get(diccionario_instrucciones, pid_char);

    if (instrucciones != NULL) {
        for (int i = 0; i < list_size(instrucciones); i++) {
            t_instruccion* instruccion = list_get(instrucciones, i);

            for (int j = 0; j < list_size(instruccion->parametros); j++) {
                t_parametro* parametro = list_get(instruccion->parametros, j);
                free(parametro->nombre);
                free(parametro);
            }

            list_destroy(instruccion->parametros);
            free(instruccion);
        }
    }   

    list_destroy(instrucciones);
    dictionary_remove(diccionario_instrucciones, pid_char);

    pthread_mutex_unlock(&mutex_diccionario_instrucciones);
    free(pid_char);
}

void eliminar_instrucciones(int pid) {
    /*
    void _destruir_parametro(t_parametro* parametro) {
        free(parametro->nombre);
        free(parametro);
    }

    void _destruir_instruccion(t_instruccion* instruccion) {
        list_destroy_and_destroy_elements(instruccion->parametros, (void*)_destruir_parametro);
    }

    void _destruir_instrucciones(t_list* lista_instrucciones) {
        list_destroy_and_destroy_elements(lista_instrucciones, (void*)_destruir_instruccion);
    }

    dictionary_remove_and_destroy(diccionario_instrucciones, string_itoa(pid), (void*)_destruir_instrucciones);
    */
}

void limpiar_bitmap(int pid_a_liberar) {
    char* pid_string = string_itoa(pid_a_liberar);
    
    t_proceso_paginas* proceso_pagina = dictionary_remove(diccionario_tablas_paginas, pid_string);
    t_list* tabla_paginas = proceso_pagina->tabla_paginas;

    log_info(logger_memoria, "PID: %d - Accion: DESTRUIR - Tamaño: %d", pid_a_liberar, list_size(proceso_pagina ->tabla_paginas));    

    for(int i = 0; i < list_size(tabla_paginas); i++) {
        t_pagina* pagina = list_get(tabla_paginas, i);
        bitarray_clean_bit(bitmap, pagina->frame);
    }

    destruir_paginas(proceso_pagina);
    free(pid_string);
   // free(proceso_pagina);
}

void liberar_modulo_memoria() {
    /*bitarray_destroy(bitmap);
    liberar_tablas_paginas();
    liberar_ios();*/
}

void _liberar_pagina(t_pagina* pagina) {
    free(pagina);
}
    
void destruir_paginas(t_proceso_paginas* proceso) {
    
    for(int i = 0; i < list_size(proceso->tabla_paginas); i++) {
        t_pagina* pagina = list_get(proceso->tabla_paginas, i);
        free(pagina);
    }
    
    // Luego, liberamos la lista tabla_paginas
    list_destroy(proceso->tabla_paginas);
    free(proceso);
}

void liberar_tablas_paginas() {
    dictionary_destroy_and_destroy_elements(diccionario_tablas_paginas, (void*)destruir_paginas);
}

void liberar_ios() {

}

void enviar_valor_leido_a_io(int pid, int socket_io, char* valor, int tamanio) {
    printf("\nLlega a enviar_valor con: PID %d, socket %d, valor %s, tamanio %d\n", pid, socket_io, valor, tamanio); // ACA LLEGA MAL
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 2 + tamanio;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;
    
    memcpy(stream + buffer->offset, &pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &tamanio, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, valor, tamanio);
    // buffer->stream += tamanio;

    enviar_paquete(buffer, ESCRIBITE, socket_io);
}

void* handle_io_stdout(void* socket) {
    int socket_io = (intptr_t) socket;
    int resultOk = 0;
    printf("Llega a handle_io_stdout\n");
    send(socket_io, &resultOk, sizeof(int), 0);

    // Recibir nombre de la IO

    printf("Se conecto una io oojoo!\n");
    //void* user_space_aux;

    t_paquete* paquete_inicial = inicializarIO_recibirPaquete(socket_io);
    agregar_interfaz_en_el_diccionario(paquete_inicial, socket_io);

    liberar_paquete(paquete_inicial);
    return NULL;
}

// Descomente esto pero tiene varios errores que no vi
void* handle_io_stdin(void* socket) {
    int respuesta_conexion;
    int validacion_conexion = 1;

    int socket_io = (intptr_t)socket;
    // free(socket); 
    int resultOk = 0;
    // Envio confirmacion de handshake!
    send(socket_io, &resultOk, sizeof(int), 0);
    printf("holu holu mande resultado ok porque se conecto el io\n");

    // Recibir nombre de la IO
    t_paquete* paquete_inicial = inicializarIO_recibirPaquete(socket_io);
    agregar_interfaz_en_el_diccionario(paquete_inicial, socket_io);

    t_info_io* interfaz_de_buffer = deserializar_interfaz(paquete_inicial->buffer);

    printf("Se conecto una io!\n");

    void* user_space_aux = espacio_usuario;

    // send(socket_io, &tamanio_pagina, sizeof(int), MSG_WAITALL);

    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete de IO\n");
        //recv(socket_cpu, &(paquete->codigo_operacion), sizeof(int), 0);
        //sem_wait(&sem_memoria_instruccion);
        recv(socket_io, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de IO: %d\n", paquete->codigo_operacion);

        // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
        //recv(socket_cpu, &(paquete->buffer->size), sizeof(int), 0);
        recv(socket_io, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        //recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(socket_io, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        void* stream = paquete->buffer->stream;

        switch(paquete->codigo_operacion) { 
            case GUARDAR_VALOR:                  
                t_escritura_stdin* escritura_stdin = deserializar_escritura_stdin(stream); // PRINTF

                imprimir_datos_stdin_escritura(escritura_stdin);
                
                printf("voy a dormir para probar serializacion\n"); 
                
                user_space_aux = espacio_usuario;

                // recibir_datos_escritura();

                char* registro_escritura = escritura_stdin->valor;

                printf("el valor de GUARDAR_VALOR es %s\n", registro_escritura);
                printf("el tamanio de registro escritura es %d", string_length(registro_escritura));
                log_info(logger_memoria, "\nel valor a guardar en memoria llegado desde STDIN es: %s\n\n", registro_escritura);

                realizar_operacion(escritura_stdin->pid_stdin->pid, ESCRITURA, escritura_stdin->pid_stdin->lista_direcciones, user_space_aux, registro_escritura, NULL); 
                
                // printf("\n\nRegistro escrito como char* %s\n\n", ((char*)registro_escritura));
                
                int terminoOk = 1;
                // send(socket_cpu, &confirm_finish, sizeof(uint32_t), 0);
                // printf("el valor guardado en dir fisica por STDIN: %s\n", registro_escritura);
                send(socket_io, &terminoOk, sizeof(int), 0);
                // void* primera_direccion = list_get(escritura_stdin->pid_stdin->lista_direcciones, 0);
                // log_info(logger_memoria,"PID %d - Accion: ESCRIBIR - Direccion fisica: %d - Tamanio %d", escritura_stdin->pid_stdin->pid, *(int*)primera_direccion, escritura_stdin->valor_length);
                list_destroy(escritura_stdin->pid_stdin->lista_direcciones);
                free(escritura_stdin->pid_stdin);
                free(escritura_stdin->valor); 
                free(escritura_stdin);
                break;
            default:
                printf("Rompio todo?\n");
                return NULL;
            }
        liberar_paquete(paquete);
    }
    
    // Liberamos memoria
    free(interfaz_de_buffer->nombre_interfaz);
    free(interfaz_de_buffer);
    liberar_paquete(paquete_inicial);
    return NULL;
}

void imprimir_datos_stdin_escritura(t_escritura_stdin* escritura)  {
    printf("PID: %d\n", escritura->pid_stdin->pid);
    printf("Tamanio registro: %d\n", escritura->pid_stdin->registro_tamanio);
    printf("Cantidad paginas: %d\n", escritura->pid_stdin->cantidad_paginas);
    printf("Valor length: %d\n", escritura->valor_length);
    printf("Valor: %s\n", escritura->valor);

    printf("Lista direcciones: \n");

    for(int i = 0; i < list_size(escritura->pid_stdin->lista_direcciones); i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(escritura->pid_stdin->lista_direcciones, i);
        printf("Direccion fisica: %d\n", dir_fisica_tam->direccion_fisica);
        printf("Tamanio: %d\n", dir_fisica_tam->bytes_lectura);
    }
}
   
/*
typedef struct {
    t_list* lista_direcciones;
    int registro_tamanio;
    int cantidad_paginas;
    int pid;
} t_pid_stdin;

typedef struct {
    t_pid_stdin* pid_stdin;
    int valor_length;
    char* valor;
} t_escritura_stdin;
*/

t_escritura_stdin* deserializar_escritura_stdin(void* stream) {
    t_escritura_stdin* escritura_stdin = malloc(sizeof(t_escritura_stdin));
    escritura_stdin->pid_stdin = malloc(sizeof(t_pid_stdin));

    memcpy(&escritura_stdin->pid_stdin->pid, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&escritura_stdin->pid_stdin->registro_tamanio, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&escritura_stdin->pid_stdin->cantidad_paginas, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&escritura_stdin->valor_length, stream, sizeof(int)); // Ojo con el "&" aca
    stream += sizeof(int);

    escritura_stdin->valor = malloc(escritura_stdin->valor_length); // +1 posible
    memcpy(escritura_stdin->valor, stream, escritura_stdin->valor_length);
    stream += escritura_stdin->valor_length;

    escritura_stdin->pid_stdin->lista_direcciones = list_create();
    
    for(int i=0; i < escritura_stdin->pid_stdin->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio)); 
        memcpy(&dir_fisica_tam->direccion_fisica, stream, sizeof(int));
        stream += sizeof(int);
        memcpy(&dir_fisica_tam->bytes_lectura, stream, sizeof(int));
        stream += sizeof(int);

        list_add(escritura_stdin->pid_stdin->lista_direcciones, dir_fisica_tam); //VER_S
        // list_add(direcciones_restantes, dir_fisica_tam);
    }
    
    return escritura_stdin;
}

void imprimir_diccionario() {
    // t_list* instrucciones = dictionary_elements(diccionario_instrucciones);
    dictionary_iterator(diccionario_instrucciones, (void*)print_instrucciones);  
}

void print_parametros(t_parametro* parametro) {
    printf("Parametro: %s \n", parametro->nombre);
    printf("Parametro length: %d \n", parametro->length);
}

void print_instruccion(t_instruccion* instruccion) {
    printf("Nombre instruccion: %s\n", instruccion_a_string(instruccion->nombre)); // Ver esto
    printf("Cantidad parametros: %d\n", instruccion->cantidad_parametros);
    list_iterate(instruccion->parametros, (void*)print_parametros);
}

void print_instrucciones(char* pid, t_list* lista_instrucciones) {
    // closure(element->key, element->data);
    printf("El PID del proceso a mostrar es: %s\n", pid);
    list_iterate(lista_instrucciones, (void*)print_instruccion); 
}

char* instruccion_a_string(TipoInstruccion tipo) {
    switch (tipo) {
        case SET: return "SET";
        case MOV_IN: return "MOV_IN";
        case MOV_OUT: return "MOV_OUT";
        case SUM: return "SUM";
        case SUB: return "SUB";
        case JNZ: return "JNZ";
        case RESIZE: return "RESIZE";
        case COPY_STRING: return "COPY_STRING";
        case WAIT: return "WAIT";
        case SIGNAL: return "SIGNAL";
        case IO_GEN_SLEEP: return "IO_GEN_SLEEP";
        case IO_STDIN_READ: return "IO_STDIN_READ";
        case IO_STDOUT_WRITE: return "IO_STDOUT_WRITE";
        case IO_FS_CREATE: return "IO_FS_CREATE";
        case IO_FS_DELETE: return "IO_FS_DELETE";
        case IO_FS_TRUNCATE: return "IO_FS_TRUNCATE";
        case IO_FS_WRITE: return "IO_FS_WRITE";
        case IO_FS_READ: return "IO_FS_READ";
        case EXIT_INSTRUCCION: return "EXIT";
        default: return "UNKNOWN";
    }
}

void crear_estructuras(char* path_completo, int pid) {
    FILE* file = fopen(path_completo, "r");
    if(file == NULL) {
        printf("No se pudo abrir el archivo\n");
        return;
    }

    t_list* instrucciones = list_create();

    t_proceso_paginas* proceso_paginas = malloc(sizeof(t_proceso_paginas));
    t_list* tabla_pagina = list_create();

    proceso_paginas->tabla_paginas = tabla_pagina;
    proceso_paginas->cantidad_frames = 0;

    log_info(logger_memoria,"PID: %d - Accion: CREAR - Tamaño: %d", pid, list_size(proceso_paginas->tabla_paginas));
    // aca podria ir el log con el list_size(instrucciones)

    /*
    char* pid_char = malloc(sizeof(char) * 4); // Un char es de 1B, un int es de 4B
    sprintf(pid_char, "%d", pid);
    */
    char* pid_char = string_itoa(pid);
    
    dictionary_put(diccionario_tablas_paginas, pid_char, proceso_paginas); // VER_SI_HAY_FREE
    
    pthread_mutex_lock(&mutex_diccionario_instrucciones);
    dictionary_put(diccionario_instrucciones, pid_char, instrucciones);
    pthread_mutex_unlock(&mutex_diccionario_instrucciones);

    aplicar_sobre_cada_linea_del_archivo(file, pid_char, (void*) _agregar_instruccion_a_diccionario);

    free(pid_char);
    fclose(file);
}

void _agregar_instruccion_a_diccionario(char* pid_char, char* linea) {
    t_list* instrucciones = dictionary_get(diccionario_instrucciones, pid_char);
    t_instruccion* instruccion = build_instruccion(linea);
    
    list_add(instrucciones, instruccion);
}

TipoInstruccion pasar_a_enum(char* nombre) {
    if (strcmp(nombre, "SET") == 0) {
        return SET;
    } else if (strcmp(nombre, "MOV_IN") == 0) {
        return MOV_IN;    
    } else if (strcmp(nombre, "MOV_OUT") == 0) {
       return MOV_OUT;
    } else if (strcmp(nombre, "SUM") == 0) {
        return SUM;
    } else if (strcmp(nombre, "SUB") == 0) {
        return SUB;
    } else if (strcmp(nombre, "JNZ") == 0) {
        return JNZ;
    } else if (strcmp(nombre, "RESIZE") == 0) {
        return RESIZE;  
    } else if (strcmp(nombre, "COPY_STRING") == 0) { 
        return COPY_STRING; 
    } else if (strcmp(nombre, "WAIT") == 0) {
        return WAIT;
    } else if (strcmp(nombre, "SIGNAL") == 0){
        return SIGNAL;
    } else if (strcmp(nombre, "IO_GEN_SLEEP") == 0) {
        return IO_GEN_SLEEP;
    } else if (strcmp(nombre, "IO_STDIN_READ") == 0) {
        return IO_STDIN_READ;
    } else if (strcmp(nombre, "IO_STDOUT_WRITE") == 0) {
        return IO_STDOUT_WRITE;
    } else if (strcmp(nombre, "IO_FS_CREATE") == 0) {
        return IO_FS_CREATE;
    } else if (strcmp(nombre, "IO_FS_DELETE") == 0) {
        return IO_FS_DELETE;
    } else if (strcmp(nombre, "IO_FS_TRUNCATE") == 0) {
        return IO_FS_TRUNCATE;
    } else if (strcmp(nombre, "IO_FS_WRITE") == 0) {
        return IO_FS_WRITE;
    } else if (strcmp(nombre, "IO_FS_READ") == 0) {
        return IO_FS_READ;
    } else if (strcmp(nombre, "EXIT") == 0) {
        return EXIT_INSTRUCCION;
    }
    // printf("\n\nRompe aca en pasarAEnum\n\n");
    // printf("Salio mal la lectura de instruccion.\n");
    return ERROR_INSTRUCCION; // Ver esto
}

//Hay que construir bien la instruccion
t_instruccion* build_instruccion(char* line) {
    t_instruccion* instruccion = malloc(sizeof(t_instruccion)); // SET AX 1

    char* nombre_instruccion = strtok(line, " \n"); // char* nombre_instruccion = strtok(linea_leida, " \n");
    
    nombre_instruccion = strdup(nombre_instruccion); 

    // printf("Nombre instruccion: %s\n", nombre_instruccion);

    instruccion->nombre = pasar_a_enum(nombre_instruccion);
    // printf("Nombre instruccion: %s\n", instruccion_a_string(instruccion->nombre));

    free(nombre_instruccion); // Cualquier cosa ver free
    instruccion->parametros = list_create();

    char* arg = strtok(NULL, " \n");
    
    if(instruccion->nombre == EXIT_INSTRUCCION) { // Esta ultima impresion podria ser por el EXIT final que no tiene argumentos
        printf("Estoy en EXIT\n");
        instruccion->cantidad_parametros = 0;
        return instruccion;
    }

    while(arg != NULL) {
        t_parametro* parametro = malloc(sizeof(t_parametro)); // Aaca falta un free

        parametro->nombre = strdup(arg);
        parametro->length = string_length(arg);

        // printf("Parametro: %s\n", parametro->nombre);
        list_add(instruccion->parametros, parametro);
        arg = strtok(NULL, " \n");
        // free(parametro);
    }
    
    instruccion->cantidad_parametros = list_size(instruccion->parametros);

    return instruccion;
}

// Posible mem leak, fix para el futuro
char* agrupar_path(t_path* path) { 
    char* pathConfig_local = strdup(path_config);
    // char* path_completo = malloc(strlen(pathConfig_local) + path->path_length + 1); // 69  + 20  + 1 
    size_t path_completo_size = strlen(pathConfig_local) + path->path_length + 1;
    char* path_completo = malloc(path_completo_size);
    if (path_completo == NULL) {
        // Manejo de errores en la asignación de memoria
        fprintf(stderr, "Error al asignar memoria para path_completo\n");
        free(pathConfig_local);  // Libera memoria de pathConfig_local si path_completo no pudo ser asignado
        return NULL;
    }
    // Concatena pathConfig_local con path->path en path_completo.
    strcpy(path_completo, pathConfig_local);
    strcat(path_completo, path->path);
    printf("Path completo: %s\n", path_completo);

    free(pathConfig_local);
    return path_completo;
}

t_path* deserializar_path(t_buffer* buffer) {
    t_path* path = malloc(sizeof(t_path));
    
    void* stream = buffer->stream;

    memcpy(&(path->PID), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(path->path_length), stream, sizeof(int));
    stream += sizeof(int);
    
    // Reservo memoria para el path
    path->path = malloc(path->path_length);
    memcpy(path->path, stream, path->path_length);
    path->path[path->path_length - 1] = '\0'; // Es necesario?

    // La posta es que toque un par de cosas en lo de seralizar con mem dinamica,
    // pero enrealidad fui viendo que era diferente del de la catedra je, asi
    // que no se como funciona, pero funciona :)

    return path;
}

void imprimir_path(t_path* path) {
    printf("PID: %d\n", path->PID);
    printf("Path length: %d\n", path->path_length);
    printf("Path: %s\n", path->path);
}

t_solicitud_instruccion* deserializar_solicitud_instruccion(t_buffer* buffer) {
    t_solicitud_instruccion* instruccion = malloc(sizeof(t_solicitud_instruccion));
    printf("Deserializando solicitud\n");
    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(instruccion->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(instruccion->pc), stream, sizeof(int));
    return instruccion;
}

/*
typedef struct {
    int pid;
    int pc;
} t_solicitud_instruccion;
*/

void enviar_instruccion(t_solicitud_instruccion* solicitud_cpu, int socket_cpu) {
    int pid = solicitud_cpu->pid;
    int pc = solicitud_cpu->pc;

    printf("PID: %d\n", pid);
    printf("PC: %d\n", pc);

    //char* pid_char = malloc(20);
    //char* pid_char = malloc(sizeof(char) * 4); // El pid es un int, los ints ocupan 4 bytes y cada char ocupa un 1 byte 
    //sprintf(pid_char, "%d", pid);

    char* pid_char = string_itoa(pid);

    printf("PID en char*: %s\n", pid_char);

    t_list* instrucciones = dictionary_get(diccionario_instrucciones, pid_char);

    free(pid_char); //String_itoa hace malloc?

    t_instruccion* instruccion = list_get(instrucciones, pc);
    printf("Instruccion a enviar: %s\n", instruccion_a_string(instruccion->nombre));
    
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(TipoInstruccion) + sizeof(int);

    char* nombre_instruccion_string = instruccion_a_string(instruccion->nombre);
    if(strcmp(nombre_instruccion_string, "EXIT_INSTRUCCION") != 0) {
        for(int i=0; i<instruccion->cantidad_parametros; i++) {      
            t_parametro* parametro1 = list_get(instruccion->parametros, i);
            buffer->size += sizeof(int) + parametro1->length;
        }
    }

    buffer->offset = 0;                                       
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &instruccion->nombre, sizeof(TipoInstruccion));
    buffer->offset += sizeof(TipoInstruccion);
    memcpy(stream + buffer->offset, &instruccion->cantidad_parametros, sizeof(int));  // Copiar cantidad_parametros
    buffer->offset += sizeof(int);

    if(strcmp(nombre_instruccion_string, "EXIT_INSTRUCCION") != 0) {
        for(int i = 0; i < instruccion->cantidad_parametros; i++) {
            // t_parametro* parametro2 = malloc(sizeof(t_parametro));
            t_parametro* parametro2 = list_get(instruccion->parametros, i);
            // parametro2 = list_get(instruccion->parametros, i);

            printf("Este es el nombre del parametro %d que agarro de la lista: %s\n", i, parametro2->nombre);

            /*uint32_t param_length = parametro2->length;
            memcpy(stream + buffer->offset, &param_length, sizeof(uint32_t));
            buffer->offset += sizeof(uint32_t); 
            memcpy(stream + buffer->offset, parametro2->nombre, param_length);
            buffer->offset += param_length;
            */

            memcpy(stream + buffer->offset, &(parametro2->length), sizeof(int));
            buffer->offset += sizeof(int); 
            memcpy(stream + buffer->offset, parametro2->nombre, parametro2->length);
            buffer->offset += parametro2->length;

            // free(parametro2->nombre);
        }
    }

    buffer->stream = stream;
    enviar_paquete(buffer,ENVIO_INSTRUCCION,socket_cpu);
}

/*
void enviar_cantidad_parametros(int socket_cpu){
    t_list* cantidad_instrucciones = dictionary_size(diccionario_instrucciones);

    printf("cant de elementos %d\n", cantidad_instrucciones);
    
    t_paquete* paquete = malloc(sizeof(t_paquete));
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->stream = malloc(buffer->size);
    void* stream = buffer->stream;

    buffer->stream = stream;

    paquete->codigo_operacion = CANTIDAD; 
    paquete->buffer = buffer;

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(codigo_operacion) + sizeof(int));
    //void* a_enviar = malloc(buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(codigo_operacion));
    offset += sizeof(codigo_operacion);
    
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
    
    send(socket_cpu, a_enviar, buffer->size + sizeof(codigo_operacion) + sizeof(int), 0);

    // Liberamos memoria
    free(a_enviar);
    liberar_paquete(paquete);
}
*/

void enviar_cantidad_instrucciones_pedidas(char* pid, int socket_cpu) {
    /* 
    if(dictionary_has_key(diccionario_instrucciones, pid)) {
        printf("El diccionario tiene el pid %s\n", pid);
    }
    */

    pthread_mutex_lock(&mutex_diccionario_instrucciones);
    t_list* instrucciones = dictionary_get(diccionario_instrucciones, pid);
    pthread_mutex_unlock(&mutex_diccionario_instrucciones);
    
    int cantidad_instrucciones;
    
    if(instrucciones == NULL) {
        cantidad_instrucciones = 0;
        printf("No se encontraron las instrucciones de %s\n", pid);
    } else {
        cantidad_instrucciones = list_size(instrucciones);
    }

    //printf("Cantidad instrucciones del proceso %s: %d\n", pid, instrucciones->elements_count);
    printf("PID: %s\n", pid);

    printf("Cant de instrucciones del proceso %s: %d\n", pid, cantidad_instrucciones);
    
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int);
    buffer->offset = 0;

    buffer->stream = malloc(buffer->size);
    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &cantidad_instrucciones, sizeof(int));

    buffer->stream = stream;
    enviar_paquete(buffer,ENVIO_CANTIDAD_INSTRUCCIONES,socket_cpu);
}

int deserializar_pid(t_buffer* buffer) {
    int pid;

    void* stream = buffer->stream;
    memcpy(&pid, stream, sizeof(int));

    return pid;
}


char* leer(int tamanio, int direccion_fisica) {
    char* valor_leido;
    void* user_space_aux = espacio_usuario;
    memcpy(&valor_leido, user_space_aux + direccion_fisica, tamanio);
    return valor_leido;
}

void mostrar_en_io(int socket_io, char* valor_leido) {
    t_buffer* buffer = llenar_buffer_stdout(valor_leido);
    enviar_paquete(buffer,MOSTRAR,socket_io);
}

t_buffer* llenar_buffer_stdout(char* valor) {
    t_buffer* buffer = malloc(sizeof(t_buffer)); 

    buffer->size = string_length(valor);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &valor, string_length(valor));
    buffer->offset += string_length(valor);

    buffer->stream = stream;

    return buffer;
}

t_pid_stdout* desearializar_pid_stdout(t_buffer* buffer){
    t_pid_stdout* pedido_escritura = malloc(sizeof(t_pid_stdout)); 
    // sizeof(int) * 3 + sizeof(uint32_t) + pid_stdout->cantidad_paginas * 2 * sizeof(int) + pid_stdout->largo_interfaz

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(pedido_escritura->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_escritura->registro_tamanio), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pedido_escritura->cantidad_paginas), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_escritura->largo_interfaz), stream, sizeof(int));
    stream += sizeof(int);
    printf("el largo de la interfaz es: %d\n", pedido_escritura->largo_interfaz);
    pedido_escritura->nombre_interfaz = malloc(pedido_escritura->largo_interfaz); 
    memcpy(pedido_escritura->nombre_interfaz, stream, pedido_escritura->largo_interfaz);
    stream += pedido_escritura->largo_interfaz;
    printf("El nombre de la interfaz en deserializar stdout es: %s\n", pedido_escritura->nombre_interfaz);

    pedido_escritura->lista_direcciones = list_create();

    for(int i=0; i < pedido_escritura->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio)); 
        memcpy(&(dir_fisica_tam->direccion_fisica), stream, sizeof(int));
        stream += sizeof(int);
        printf("\n\nLa direccion fisica recibida de memoria: %d\n\n", dir_fisica_tam->direccion_fisica);
        printf("\n\nel pid recibido de memoria: %d\n\n", pedido_escritura->pid);
        memcpy(&(dir_fisica_tam->bytes_lectura), stream, sizeof(int));
        stream += sizeof(int);
        printf("\n\nLos bytes recibidos de memoria: %d\n\n", dir_fisica_tam->bytes_lectura);
        list_add(pedido_escritura->lista_direcciones, dir_fisica_tam);
    }
 
    return pedido_escritura; 
    // VER FREE
}

void agregar_interfaz_en_el_diccionario(t_paquete* paquete, int socket) {
    t_info_io* interfaz = deserializar_interfaz(paquete->buffer);
    printf("\nSe conecto la interfaz con nombre %s\n", interfaz->nombre_interfaz);
    printf("La interfaz tiene socket %d\n", socket);
    socket_estructurado* socket_io = malloc(sizeof(int));
    socket_io->socket = socket;
    pthread_mutex_lock(&mutex_diccionario_io);
    dictionary_put(diccionario_io, interfaz->nombre_interfaz, socket_io); //VER_SI_HAY_FREE se hace al final del proceso
    pthread_mutex_unlock(&mutex_diccionario_io);
    
    pthread_mutex_lock(&mutex_diccionario_io); // decile a osfi que me siga
    socket_estructurado* socket_sacado = dictionary_get(diccionario_io, interfaz->nombre_interfaz);
    pthread_mutex_unlock(&mutex_diccionario_io);
    printf("El socket del diccionario: %d\n", socket_sacado->socket);
}

t_memoria_fs_escritura_lectura* deserializar_escritura_lectura(t_buffer* buffer){
    t_memoria_fs_escritura_lectura* pedido_escritura_lectura = malloc(sizeof(t_memoria_fs_escritura_lectura)); //FREE?

    void* stream = buffer->stream;
    
    memcpy(&(pedido_escritura_lectura->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_escritura_lectura->socket), stream, sizeof(int));
    stream += sizeof(int);

    memcpy(&(pedido_escritura_lectura->longitud_nombre_interfaz), stream, sizeof(int));
    stream += sizeof(int);
    memcpy((pedido_escritura_lectura->nombre_interfaz), stream, pedido_escritura_lectura->longitud_nombre_interfaz);
    stream += pedido_escritura_lectura->longitud_nombre_interfaz;
    
    memcpy(&(pedido_escritura_lectura->largo_archivo), stream, sizeof(int));
    stream += sizeof(int);
    memcpy((pedido_escritura_lectura->nombre_archivo), stream, pedido_escritura_lectura->largo_archivo);
    stream += pedido_escritura_lectura->largo_archivo;
    
    memcpy(&(pedido_escritura_lectura->registro_direccion), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pedido_escritura_lectura->registro_archivo), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pedido_escritura_lectura->registro_archivo), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    return pedido_escritura_lectura;
}