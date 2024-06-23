#include "../include/conexionesMEM.h"

pthread_t cpu_thread;
pthread_t kernel_thread;
pthread_t io_stdin_thread;
pthread_t io_stdout_thread;
pthread_mutex_t mutex_diccionario_instrucciones;
t_config* config_memoria;
t_dictionary* diccionario_instrucciones;
t_dictionary* diccionario_tablas_paginas;
char* path_config;
int tamanio_pagina;
int tamanio_memoria;
void* espacio_usuario;
int cant_frames;
//sem_t* hay_valores_para_leer;
t_bitarray* bitmap;
int socket_cpu;

//Acepta el handshake del cliente, se podria hacer mas generico y que cada uno tenga un valor diferente
int esperar_cliente(int socket_servidor, t_log* logger_memoria) {
	int handshake = 0;
	int resultError = -1;
	int socket_cliente = accept(socket_servidor, NULL, NULL);

    if(socket_cliente == -1) {
        return -1;
    }
    
	recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL); 
	if(handshake == 1) {
        // pthread_t kernel_thread;
        pthread_create(&kernel_thread, NULL, (void*)handle_kernel, (void*)(intptr_t)socket_cliente);
        
    } else if(handshake == 2) {
        // pthread_t cpu_thread;
        pthread_create(&cpu_thread, NULL, (void*)handle_cpu, (void*)(intptr_t)socket_cliente);
    } else if(handshake == 91) {
        // pthread_t cpu_thread;
        pthread_create(&io_stdin_thread, NULL, (void*)handle_io_stdin, (void*)(intptr_t)socket_cliente); // Ver
    } else if(handshake == 79) {
        // pthread_t cpu_thread;
        pthread_create(&io_stdout_thread, NULL, (void*)handle_io_stdout, (void*)(intptr_t)socket_cliente); // Ver
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

bool check_same_page(int tamanio, int cant_bytes_uso) { 
    return tamanio > cant_bytes_uso - tamanio_pagina && tamanio < cant_bytes_uso;
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
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    
    while(1) {
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
        uint32_t registro;
        void* user_space_aux;

        bool es_uint8;
        int direccion_fisica;
        
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
                quiero_frame(paquete->buffer); // Recibe mal pid y pagina
                break;
            case QUIERO_INSTRUCCION:
                t_solicitud_instruccion* solicitud_cpu = deserializar_solicitud_instruccion(paquete->buffer);
                usleep(1000 * 1000); // Harcodeado nashe para probar (Con retardo)
                enviar_instruccion(solicitud_cpu, socket_cpu);
                free(solicitud_cpu);
                break;
            case QUIERO_CANTIDAD_INSTRUCCIONES:
                //t_cantidad_instrucciones* cantidad_instrucciones = deserializar_cantidad(paquete->buffer);
                int pid_int = deserializar_pid(paquete->buffer);
                char* pid_string = malloc(sizeof(int));
                pid_string = string_itoa(pid_int);
                printf("La CPU me pide la cantidad de instrucciones del proceso con pid %s.\n", pid_string);
                // enviar_cantidad_parametros(socket_cpu);
                enviar_cantidad_instrucciones_pedidas(pid_string, socket_cpu);
                // free(cantidad_instrucciones);
                free(pid_string);
                break;
            case RECIBIR_DIRECCIONES:
                user_space_aux = espacio_usuario;

                t_list* direcciones_restantes = list_create(); 
                t_direccion_fisica* datos_lectura = deserializar_direccion_fisica_lectura(paquete->buffer);
                
                printf("dir_fisica->tamanio : %d\n", datos_lectura->tamanio);
                void* registro_lectura = malloc(datos_lectura->tamanio);

                int respuesta_ok_l = 3;
                
                send(socket_cpu, &respuesta_ok_l, sizeof(uint32_t), 0);
            
                realizar_operacion(LECTURA, direcciones_restantes, user_space_aux, datos_lectura, NULL, registro_lectura);
                
                printf("El valor leido es: %s\n", (char*)registro_lectura);

                // uint32_t* ptr = (uint32_t*)registro_lectura;
                // printf("El valor leido es: %d\n", *ptr);

                printf("tamanio : %d\n", datos_lectura->tamanio);
                send(socket_cpu, registro_lectura, datos_lectura->tamanio, 0);
                
                free(datos_lectura);
                free(registro_lectura);
                break;
            case ESCRIBIR_DATO_EN_MEM: 
                user_space_aux = espacio_usuario;
                int respuesta_ok_mv = 2;
                confirm_finish = 1;

                // recibir_datos_escritura();
                t_list* direcciones_restantes_mov_out = list_create();

                t_direccion_fisica_escritura* df_escritura_general = deserializar_direccion_fisica_escritura(paquete->buffer);

                t_direccion_fisica* datos_escritura = malloc(sizeof(t_direccion_fisica));
                datos_escritura = armar_dir_lectura(df_escritura_general);

                printf("la direccion fisica recibida es %d\n", datos_escritura->direccion_fisica);

                send(socket_cpu, &respuesta_ok_mv, sizeof(int), 0);

                void* registro_escritura = df_escritura_general->valor_a_escribir;
                printf("El valor a escribir es: %s\n", (char*)registro_escritura); // Prueba con un uint32_t, romperia si length es > 0
                // printf("El valor a escribir es: %d\n", *(uint32_t*)registro_escritura); // Prueba con un uint32_t, romperia si length es > 0

                realizar_operacion(ESCRITURA, direcciones_restantes_mov_out, user_space_aux, datos_escritura, registro_escritura, NULL);
                send(socket_cpu, &confirm_finish, sizeof(uint32_t), 0);

                free(datos_escritura);
                free(df_escritura_general->valor_a_escribir);
                free(df_escritura_general);
                break;
            default:
                printf("No reconozco ese cod-op...\n"); 
                return NULL;
        }
    }

    free(paquete->buffer->stream); // Por alguna razon no le gusta esto
    free(paquete->buffer);
    free(paquete);
    return NULL;
}

t_direccion_fisica* armar_dir_lectura(t_direccion_fisica_escritura* df) {
    t_direccion_fisica* df_lectura = malloc(sizeof(t_direccion_fisica));

    df_lectura->direccion_fisica = df->datos_direccion->direccion_fisica;
    df_lectura->tamanio = df->datos_direccion->tamanio;
    df_lectura->cantidad_paginas = df->datos_direccion->cantidad_paginas;
    df_lectura->direccion_logica = df->datos_direccion->direccion_logica;

    return df_lectura;
}

/**** 
 
Pequeños detalles de esta implemenetacion :  la funcion recibe 2 punteros, ya que tanto es MOV-IN, como en MOV-OUT, funcionan de forma diferente estos punteros.
De todas formas se usan solo cuando se tienen que usar, ya que en la llamada se les pasa NULL.
El 0 en el tercer parametro del a primera llamada a interaccion_user_space es para que la prmera vez simplemente no se mueva de su lugar
Ahora dir fisica es una misma estructura, solo que en el MOV-IN no se usa el apartado del valor. No se si esta es la mejor idea, pero la otra era
hacer un send y recv + para el valor, y me parecio que era mas paja.
 
******/

void realizar_operacion(tipo_operacion operacion, t_list* direcciones_restantes, void* user_space_aux, t_direccion_fisica* df, void* registro_escritura, void* registro_lectura) {

    printf("Antes de entrar al for, necesito %d paginas\n", df->cantidad_paginas);
    /***** Ver existe algun forma de simplificar esto *****/
    if(df->cantidad_paginas > 1) {
        for(int i = 0; i < (df->cantidad_paginas-1) * 2; i++) {
            printf("Entro al for por %d vez\n", i+1);
            recibir_resto_direcciones(direcciones_restantes);
        }
    }
    /****                                             *****/

    int df_actual = df->direccion_fisica;
    int indice_direccion = 0;
    
    int cant_bytes_usables = bytes_usables_por_pagina(df->direccion_logica); // Esto podria enviarlo CPU   

    int tam_a_escribir = min(cant_bytes_usables, df->tamanio); 
    interaccion_user_space(operacion, df_actual, user_space_aux, 0, tam_a_escribir, registro_escritura, registro_lectura);
    // memcpy((user_space_aux + df_actual), registro, tam_a_escribir);

    int resto_usable = df->tamanio - cant_bytes_usables;

    while(resto_usable > 0) {
        int* df_ptr = (int*)list_get(direcciones_restantes, indice_direccion);  
        df_actual = *df_ptr;

        int tam_escrito_anterior = tam_a_escribir;
        int cant_bytes_usables = tamanio_pagina;

        user_space_aux = espacio_usuario; // Con esto lo devuelvo inicio al ptr_aux
        
        tam_a_escribir = min(cant_bytes_usables, resto_usable); 

        interaccion_user_space(operacion, df_actual, user_space_aux, tam_escrito_anterior, tam_a_escribir, registro_escritura, registro_lectura);
        // memcpy((user_space_aux + df_actual), registro + tam_escrito_anterior, tam_a_escribir); -> Escritura

        resto_usable -= cant_bytes_usables;
        list_remove(direcciones_restantes, indice_direccion);

        // aca quiero obtener la proxima direccion de la lista inmediata y por eso aumento el indice
        indice_direccion++;
    }
}

void interaccion_user_space(tipo_operacion operacion, int df_actual, void* user_space_aux, int tam_escrito_anterior, int tamanio, void* registro_escritura, void* registro_lectura) {
    if(operacion == ESCRITURA) {
        memcpy((user_space_aux + df_actual), registro_escritura + tam_escrito_anterior, tamanio);
    } else { // == LECTURA
        memcpy(registro_lectura + tam_escrito_anterior, (user_space_aux + df_actual), tamanio);
    }
}

void quiero_frame(t_buffer* buffer) {
    t_solicitud_frame* pedido_frame = deserializar_solicitud_frame(buffer);

    int frame = buscar_frame(pedido_frame);

    send(socket_cpu, &frame, sizeof(int), 0); 
    free(pedido_frame);
}

t_direccion_fisica_escritura* deserializar_direccion_fisica_escritura(t_buffer* buffer) {
    t_direccion_fisica_escritura* datos_escritura = malloc(sizeof(t_direccion_fisica_escritura));

    void* stream = buffer->stream;

    datos_escritura->datos_direccion = malloc(sizeof(t_direccion_fisica));

    memcpy(&datos_escritura->datos_direccion->direccion_fisica, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&datos_escritura->datos_direccion->tamanio, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&datos_escritura->datos_direccion->cantidad_paginas, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&datos_escritura->datos_direccion->direccion_logica, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&datos_escritura->length_valor, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    printf("El length_valor en medio de la deserializacion es: %d\n", datos_escritura->length_valor);
    if(datos_escritura->length_valor > 0) {
        datos_escritura->valor_a_escribir = malloc(datos_escritura->length_valor);
        memcpy(datos_escritura->valor_a_escribir, stream, datos_escritura->length_valor);
        printf("El valor a escribir justo despues de serializar es: %s\n", (char*)datos_escritura->valor_a_escribir);
    } else {
        datos_escritura->valor_a_escribir = malloc(datos_escritura->datos_direccion->tamanio);
        memcpy(datos_escritura->valor_a_escribir, stream, datos_escritura->datos_direccion->tamanio);
    }

    buffer->stream = stream;

    return datos_escritura;
}

t_direccion_fisica* deserializar_direccion_fisica_lectura(t_buffer* buffer) {
    t_direccion_fisica* datos_lectura = malloc(sizeof(t_direccion_fisica));

    void* stream = buffer->stream;

    memcpy(&datos_lectura->direccion_fisica, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&datos_lectura->tamanio, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&datos_lectura->cantidad_paginas, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&datos_lectura->direccion_logica, stream, sizeof(int));
    stream += sizeof(int);
    
    buffer->stream = stream;

    return datos_lectura;
}

int min(int a, int b) {
    return (a < b) ? a : b;
}

// Esta funcion podria cambiar en un futuro, si yo ya envio esa cantidad a memoria, que seria lo logico
int bytes_usables_por_pagina(int direccion_logica) {
    int offset = direccion_logica % tamanio_pagina;
    int dev_lectura =  tamanio_pagina - offset;
    return dev_lectura;
}

void recibir_resto_direcciones(t_list* lista_direcciones_mv) {
    
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete de CPU\n");
        recv(socket_cpu, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de CPU: %d\n", paquete->codigo_operacion);

        recv(socket_cpu, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        
        paquete->buffer->stream = malloc(paquete->buffer->size);
            
        recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        void* stream = paquete->buffer->stream;

        int direccion_fisica;
        // int mandameSiguienteDireccion = 1;
        int* cast_dir_fisica;
        
        switch(paquete->codigo_operacion) {
            case RECIBIR_DIR_FISICA :   
                    memcpy(&direccion_fisica, paquete->buffer->stream, sizeof(int));
                    stream += sizeof(int);

                    cast_dir_fisica = malloc(sizeof(int)); // Ver free
                    *cast_dir_fisica = direccion_fisica;

                    printf("La direccion fisica es: %d\n", direccion_fisica);

                    list_add(lista_direcciones_mv, cast_dir_fisica); // Despues habria que hacerle todos los free correspondientes
                    int* ultimo_valor = list_get(lista_direcciones_mv, list_size(lista_direcciones_mv) - 1);
                    printf("Agregue la direccion en la lista y es : %d\n", *ultimo_valor);

                    free(cast_dir_fisica);
                    // send(socket_cpu, &mandameSiguienteDireccion, sizeof(int), 0);
                break;
            case QUIERO_FRAME :
                quiero_frame(paquete->buffer);
                break;
            default :
                printf("No recibio ni quiero frame ni quiero dirrecion\n");
        }
        
        // Liberamos memoria
        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
}

/* 
t_direccion_fisica* deserializar_direccion_fisica(t_buffer* buffer) {
    t_direccion_fisica* dir_fisica = malloc(sizeof(t_direccion_fisica));

    void* stream = buffer->stream;

    memcpy(&dir_fisica->direccion_fisica, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&dir_fisica->tamanio, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&dir_fisica->cantidad_paginas, stream, sizeof(int));
    stream += sizeof(int);
    
    memcpy(&dir_fisica->direccion_logica, stream, sizeof(int));
    stream += sizeof(int);

    buffer->stream = stream;

    return dir_fisica;
}
*/

int resize_memory(void* stream) {
    int tamanio, pid;

    memcpy(&tamanio, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&pid, stream, sizeof(int));

    printf("El tamanio que me pide el CPU es : %d\n", tamanio);
    printf("El pid es :  %d\n", pid);

    // Luego verifico si tengo espacio suficiente en memoria
    int out_of_memory = validar_out_of_memory(tamanio);
    
    if(out_of_memory == 1) {
        printf("No hay espacio suficiente en memoria\n");
        return 1; // 1 es out of memory
        // send(socket_cpu, &out_of_memory, sizeof(int), 0);
    } 
                
    char* pid_char = string_itoa(pid);
    int cant_bytes_uso = cantidad_frames_proceso(pid_char);
    
    // esta funcion se fija si la cant de bytes que pidio (tamanio) no implica aumentar o recortar la cant de frames que le dimos
    bool same_page = check_same_page(tamanio, cant_bytes_uso);

    if(same_page) {
        printf("El proceso ya tiene una pagina con el tamanio solicitado\n");
    } else if(tamanio > cant_bytes_uso) {
        printf("Voy a asignar tamanio\n");
        asignar_tamanio(tamanio, cant_bytes_uso, pid_char);
    } else {
        printf("Voy a recortar tamanio");
        recortar_tamanio(tamanio, pid_char, cant_bytes_uso);
    }
    
    printear_paginas(pid_char);
    printear_bitmap();
    free(pid_char);

    return 0; // Si llego hasta aca, la op salio bien :)
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
    list_add(tabla_paginas, pagina);
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
    int frames_necesarios = tamanio / tamanio_pagina;
    int frames_disponibles = contar_frames_libres();
    if(frames_disponibles < frames_necesarios) {
        return 1;
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
    bool _es_la_pagina_buscada(t_pagina* pagina) {
        return pagina->numero_pagina == pedido_frame->nro_pagina; 
    }

    printf("Busco el frame con la pagina %d\n", pedido_frame->nro_pagina);
    t_proceso_paginas* proceso_paginas = dictionary_get(diccionario_tablas_paginas, string_itoa(pedido_frame->pid)); // el diccionario tiene como key el pid y la lista de pags asociado a ese pid
    t_list* tabla_paginas = proceso_paginas->tabla_paginas;
    
    if(list_find(tabla_paginas, (void*) _es_la_pagina_buscada) == NULL) return -1;
    
    int numero_frame = *(int*) list_get(tabla_paginas, pedido_frame->nro_pagina);
    printf("el marco es: %d\n", numero_frame);
    
    return numero_frame;
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
            case PATH:                                       // Ver si no es mejor recibir primero el cod-op y adentro recibimos el resto (Cambiaria la forma de serializar, ya no habira)
                t_path* path = deserializar_path(paquete->buffer);
                imprimir_path(path);
                path_completo = agrupar_path(path); // Ojo con el SEG_FAULT
                printf("CUIDADO QUE IMPRIMO EL PATH: %s\n", path_completo);

                crear_estructuras(path_completo, path->PID);
                
                /*
                printf("Antes del sem_post\n");
                sem_post(&sem_cargo_instrucciones); // CREO que va aca
                printf("Despues del sem_post\n");
                */
                printf("Cuidado que voy a imprimir el diccionario...\n");

                pthread_mutex_lock(&mutex_diccionario_instrucciones);
                imprimir_diccionario(); // Imprime todo, sin PID especifico...
                pthread_mutex_unlock(&mutex_diccionario_instrucciones);

                free(path);
                break;
            case ESCRIBIR_STDOUT:
                t_pedido_escritura* pedido_escritura = desearializar_pedido_escritura(paquete->buffer);
                //char* valor_leido = leer(pedido_escritura->tamanio, pedido_escritura->direccion_fisica);
                //send(socket_interfaz, &valor_leido, sizeof(char*), 0); // ver como sacar el socket de la interfaz
                //list_add(lista_valores_a_leer, valor_leido);
                //sem_post(hay_valores_para_leer);
                /*if(dictionary_has_key(diccionario_io, pedido_escritura->nombre_interfaz)) { // hacer diccionario de ios en memoria
                    t_list_io* interfaz = malloc(sizeof(t_list_io));
                    interfaz = dictionary_get(diccionario_io, nombre_interfaz);
                    socket_io = interfaz->socket;
                    mostrar_en_io(socket_io, valor_leido);
                    free(interfaz);
                } else {
                    printf("No se conecto dicha interfaz con memoria\n");
                    exit(1);
                }*/
                free(pedido_escritura->nombre_interfaz);
                break;
            default: 
                break;
        }

        // Liberamos memoria
        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }

    return NULL;
}

void* handle_io_stdout(void* socket) {
    int socket_io = *(int*) socket;
    int resultOk = 0;
    send(socket_io, &resultOk, sizeof(int), 0);

    // Recibir nombre de la IO

    printf("Se conecto una io oojoo!\n");
    //void* user_space_aux;

    send(socket_io, &tamanio_pagina, sizeof(int), MSG_WAITALL);
    void* user_space_aux = espacio_usuario;

    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete de IO\n");
        recv(socket_io, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion de IO: %d\n", paquete->codigo_operacion);

        // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
        //recv(socket_cpu, &(paquete->buffer->size), sizeof(int), 0);
        recv(socket_io, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        //recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(socket_io, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        switch(paquete->codigo_operacion) { 
            case GUARDAR_VALOR:  
                int registro_direccion;    
                int largo_valor;

                // Deserializamos tamanios que tenemos en el buffer
                memcpy(&(registro_direccion), paquete->buffer->stream, sizeof(int));
                paquete->buffer->stream += sizeof(int);
                memcpy(&(largo_valor), paquete->buffer->stream, sizeof(int));
                paquete->buffer->stream += sizeof(int);
                char* valor = malloc(largo_valor);
                memcpy(valor, paquete->buffer->stream, largo_valor);

                // guardamos el valor en la direccion fisica
                memcpy((uint32_t*) (user_space_aux + registro_direccion), &valor, sizeof(uint32_t));
                uint32_t mostrar_valor = *((uint32_t*)(user_space_aux + registro_direccion));
                printf("el valor guardado en dir fisica por STDIN: %d\n", mostrar_valor);
                free(valor);
                // hacerlo
                break;
            default:
                printf("Rompio todo?\n");
                return NULL;
        }

        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }
    // Liberamos memoria
    return NULL;
} 

void* handle_io_stdin(void* socket) {
    int socket_io = *(int*) socket;
    // free(socket); 
    int resultOk = 0;
    // Envio confirmacion de handshake!
    send(socket_io, &resultOk, sizeof(int), 0);

    // Recibir nombre de la IO

    printf("Se conecto una io oojoo!\n");
    void* user_space_aux = espacio_usuario;

    send(socket_io, &tamanio_pagina, sizeof(int), MSG_WAITALL);

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
                int registro_direccion;    
                int largo_valor;

                // Deserializamos tamanios que tenemos en el buffer
                memcpy(&(registro_direccion), stream, sizeof(int));
                stream += sizeof(int);
                memcpy(&(largo_valor), stream, sizeof(int));
                stream += sizeof(int);
                char* valor = malloc(largo_valor);
                memcpy(valor, stream, largo_valor);

                // guardamos el valor en la direccion fisica
                memcpy((uint32_t*) (user_space_aux + registro_direccion), &valor, sizeof(uint32_t));
                uint32_t mostrar_valor = *((uint32_t*)(user_space_aux + registro_direccion));
                printf("el valor guardado en dir fisica por STDIN: %d\n", mostrar_valor);
                free(valor);
                break;
            default:
                printf("Rompio todo?\n");
                return NULL;
        }

        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }
    // Liberamos memoria
    return NULL;
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
        case EXIT_INSTRUCCION: return "EXIT_INSTRUCCION";
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

    char* line = NULL;
    //size_t bufsize = 0;
    size_t bufsize = 0; // Tamaño inicial del buffer
    
    while ((getline(&line, &bufsize, file)) != -1) {
        printf("Esta es la linea: %s\n",line);
        if(strcmp(line, "\n") != 0) {
            //printf("Esta es la linea: %s\n", line);
            t_instruccion* instruccion = build_instruccion(line);
            // printf("Instruccion creada %s\n" , instruccion_a_string(instruccion->nombre));
            list_add(instrucciones, instruccion);
            // free(instruccion);
        }
    }
    
    //char* pid_char = malloc(20); // sizeof(int) + 2 VER
    char* pid_char = malloc(sizeof(char) * 4); // Un char es de 1B, un int es de 4B
    sprintf(pid_char, "%d", pid);
    
    // char* pid_char = pid + '0'; 
    // pid_char = (char*)pid;
    // char *pidchar = (char *)&pid;
    
    dictionary_put(diccionario_tablas_paginas, pid_char, proceso_paginas);
    
    pthread_mutex_lock(&mutex_diccionario_instrucciones);
    dictionary_put(diccionario_instrucciones, pid_char, instrucciones);
    pthread_mutex_unlock(&mutex_diccionario_instrucciones);

    // ->  
    free(pid_char);
    free(line);
    fclose(file);
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

    instruccion->parametros = list_create();

    char* arg = strtok(NULL, " \n");
    
    if(instruccion->nombre == EXIT_INSTRUCCION) { // Esta ultima impresion podria ser por el EXIT final que no tiene argumentos
        printf("Estoy en EXIT\n");
        instruccion->cantidad_parametros = 0;
        return instruccion;
    }

    while(arg != NULL) {
        t_parametro* parametro = malloc(sizeof(int) + string_length(strdup(arg)) * sizeof(char)); // Aaca falta un free

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
    // char* pathConfig = config_get_string_value(config_memoria, "PATH_INSTRUCCIONES");
    char* pathConfig_local = strdup(path_config);
    char* path_completo = malloc(strlen(pathConfig_local) + path->path_length); // 69  + 20  + 1
    path_completo = strcat(pathConfig_local, path->path);
    printf("Path completo: %s\n", path_completo);
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
    char* pid_char = malloc(sizeof(char) * 4); // El pid es un int, los ints ocupan 4 bytes y cada char ocupa un 1 byte
    sprintf(pid_char, "%d", pid);

    printf("PID en char*: %s\n", pid_char);

    t_list* instrucciones = dictionary_get(diccionario_instrucciones, pid_char);

    //free(pid_char); // vamos a necesitar esta variable?

    t_instruccion* instruccion = list_get(instrucciones, pc);
    printf("Instruccion a enviar: %s\n", instruccion_a_string(instruccion->nombre));
    
    t_paquete* paquete = malloc(sizeof(t_paquete));
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(TipoInstruccion) + sizeof(int);

    char* nombre_instruccion_string = instruccion_a_string(instruccion->nombre);
    if(strcmp(nombre_instruccion_string, "EXIT_INSTRUCCION") != 0) {
        for(int i=0; i<instruccion->cantidad_parametros; i++) {
            // t_parametro* parametro1 = malloc(sizeof(t_parametro));
            t_parametro* parametro1 = list_get(instruccion->parametros, i);
            //buffer->size += sizeof(uint32_t) + parametro1->length;
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

    paquete->codigo_operacion = ENVIO_INSTRUCCION; 
    paquete->buffer = buffer;

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(codigo_operacion) + sizeof(int));
    //void* a_enviar = malloc(buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(codigo_operacion));
    offset += sizeof(codigo_operacion);
    
    /*memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);*/
    
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    //send(socket_cpu, a_enviar, buffer->size + sizeof(uint8_t) + sizeof(uint32_t), 0);
    //sem_post(&sem_memoria_instruccion);

    send(socket_cpu, a_enviar, buffer->size + sizeof(codigo_operacion) + sizeof(int), 0);

    // Liberamos memoria
    // free(instruccion);
    free(pid_char);
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
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
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
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
    
    t_paquete* paquete = malloc(sizeof(t_paquete));
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int);
    buffer->offset = 0;

    buffer->stream = malloc(buffer->size);
    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &cantidad_instrucciones, sizeof(int));

    buffer->stream = stream;

    paquete->codigo_operacion = ENVIO_CANTIDAD_INSTRUCCIONES; 
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
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

/*
t_cantidad_instrucciones* deserializar_cantidad(t_buffer* buffer) {
    t_cantidad_instrucciones* cantidad_instrucciones = malloc(sizeof(t_cantidad_instrucciones));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(cantidad_instrucciones->cantidad), stream, sizeof(int));
    stream += sizeof(int);

    return cantidad_instrucciones;
}
*/

int deserializar_pid(t_buffer* buffer) {
    int pid;

    void* stream = buffer->stream;
    memcpy(&pid, stream, sizeof(int));

    return pid;
}


t_copy_string* deserializarCopyString(t_buffer* buffer) {
    t_copy_string* copy_string = malloc(sizeof(t_copy_string));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(copy_string->direccionFisicaSI), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(copy_string->direccionFisicaDI), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(copy_string->tamanio), stream, sizeof(int));
    stream += sizeof(int);
    
    return copy_string;
}

char* leer(int tamanio, int direccion_fisica) {
    char* valor_leido;
    void* user_space_aux = espacio_usuario;
    memcpy(&valor_leido, user_space_aux + direccion_fisica, tamanio);
    return valor_leido;
}

void mostrar_en_io(int socket_io, char* valor_leido) {
    t_buffer* buffer;
    buffer = llenar_buffer_stdout(valor_leido);
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = MOSTRAR; 
    paquete->buffer = buffer;

    void* a_enviar = malloc(buffer->size + string_length(valor_leido));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    send(socket_io, a_enviar, buffer->size + string_length(valor_leido), 0);

    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
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

t_pedido_escritura* desearializar_pedido_escritura(t_buffer* buffer){
    t_pedido_escritura* pedido_escritura = malloc(sizeof(pedido_escritura));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(pedido_escritura->direccion_fisica), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_escritura->tamanio), stream, sizeof(int));
    stream += sizeof(int);
    int longitud_nombre_interfaz = string_length(pedido_escritura->nombre_interfaz);
    memcpy(&(pedido_escritura->nombre_interfaz), stream, sizeof(longitud_nombre_interfaz));

    return pedido_escritura;
}
