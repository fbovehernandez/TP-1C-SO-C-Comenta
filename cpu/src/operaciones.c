#include "../include/operaciones.h"
#include "../include/mmu.h"

// t_cantidad_instrucciones* cantidad_instrucciones;

int hay_interrupcion_quantum;
int hay_interrupcion_fin;
int hay_interrupcion_fin_usuario;

pthread_mutex_t mutex_interrupcion_quantum;
pthread_mutex_t mutex_interrupcion_fin;
pthread_mutex_t mutex_interrupcion_fin_usuario;

bool se_seteo_pc = false;

void ejecutar_pcb(t_pcb *pcb, int socket_memoria) {
    //int esperar_confirm;

    pedir_cantidad_instrucciones_a_memoria(pcb->pid, socket_memoria);
    int cantidad_instrucciones = recibir_cantidad_instrucciones(socket_memoria, pcb->pid);

    while(pcb->program_counter < cantidad_instrucciones) {
        // sem_wait(pedir_instruccion);
        log_info(logger_CPU, "PID: %d - FETCH - Program Counter: %d", pcb->pid, pcb->program_counter);

        pedir_instruccion_a_memoria(socket_memoria, pcb);
        t_instruccion* instruccion = recibir_instruccion(socket_memoria, pcb); // Se ejecuta la instruccion tambien
        // sem_post(pedir_instruccion);
        if (instruccion == NULL) {
            printf("Error al recibir instrucción.\n");
            return;
        }

        // if(instruccion->nombre == ERROR_INSTRUCCION) return;

        printf("Esta ejecutando %d\n", pcb->pid);   
        int resultado_io = ejecutar_instruccion(instruccion, pcb);
        
        instruccion_destruir(instruccion);
        
        // Esto no se si va, si ya esta activada la interrupcion no deberia scarme con el return antes???
        if (resultado_io == 1) {
            pthread_mutex_lock(&mutex_interrupcion_quantum);
            hay_interrupcion_quantum = 0;
            pthread_mutex_unlock(&mutex_interrupcion_quantum);
            
            pthread_mutex_lock(&mutex_interrupcion_fin);
            hay_interrupcion_fin = 0;
            pthread_mutex_unlock(&mutex_interrupcion_fin);

            pthread_mutex_lock(&mutex_interrupcion_fin_usuario);
            hay_interrupcion_fin_usuario = 0;
            pthread_mutex_unlock(&mutex_interrupcion_fin_usuario);
            
            return;
        }
        
        if(hay_interrupcion()) {
            hacer_interrupcion(pcb);
            return;
        } 
        
        // pcb->program_counter++; // Ojo con esto! porque esta despues del return, osea q en el IO_GEN_SLEEP antes de desalojar hay que aumentar el pc
        // Podria ser una funcion directa -> Por ahora no es esencial
        /*
        if(hay_interrupcion()) {
            hacer_interrupcion(pcb);
            return;
        }
        */
    }

}

void destruir_parametro(void* parametro_void) {
    t_parametro* parametro = (t_parametro*)parametro_void;
    if (parametro != NULL) {
        if (parametro->nombre != NULL) {
            free(parametro->nombre); // Liberar el nombre del parámetro
        }
        free(parametro);  // Liberar el parámetro en sí
    }
}

void instruccion_destruir(t_instruccion* instruccion) {
    if (instruccion != NULL) {
        if (instruccion->parametros != NULL && !list_is_empty(instruccion->parametros)) {
            for (int i = 0; i < instruccion->cantidad_parametros; i++) {
                t_parametro* parametro = list_get(instruccion->parametros, i);
                destruir_parametro(parametro);
            }    

            // list_clean_and_destroy_elements(instruccion->parametros, free);  // Destruir la lista de parámetros -> Sino hacer free
        }

        list_destroy(instruccion->parametros);  // Liberar la instrucción en sí
        free(instruccion);
    }
}

int hay_interrupcion() {
    return hay_interrupcion_quantum || hay_interrupcion_fin || hay_interrupcion_fin_usuario;
}

void hacer_interrupcion(t_pcb* pcb) {
    printf("Hubo una interrupcion.\n");
    if(hay_interrupcion_quantum) {
        printf("Hubo una interrupcion por fin de quantum %d\n", pcb->pid);
        desalojar(pcb, INTERRUPCION_QUANTUM, NULL);
        pthread_mutex_lock(&mutex_interrupcion_quantum);
        hay_interrupcion_quantum = 0;
        pthread_mutex_unlock(&mutex_interrupcion_quantum);
    } else if(hay_interrupcion_fin) {
        printf("Desalojado por fin de proceso %d\n",pcb->pid); // Creo que este esta al pedo mal
        desalojar(pcb, INTERRUPCION_FIN_PROCESO, NULL);
        pthread_mutex_lock(&mutex_interrupcion_fin);
        hay_interrupcion_fin = 0;
        pthread_mutex_unlock(&mutex_interrupcion_fin);
    }else{
        printf("Desalojado por gusto de usuario\n");
        desalojar(pcb, INTERRUPCION_FIN_USUARIO, NULL);
        pthread_mutex_lock(&mutex_interrupcion_fin_usuario);
        hay_interrupcion_fin_usuario = 0;
        pthread_mutex_unlock(&mutex_interrupcion_fin_usuario);
    }
}

void pedir_cantidad_instrucciones_a_memoria(int pid, int socket_memoria) {
    t_buffer *buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(int);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
    void *stream = buffer->stream;
    memcpy(stream + buffer->offset, &pid, sizeof(int));
    buffer->stream = stream;
    enviar_paquete(buffer, QUIERO_CANTIDAD_INSTRUCCIONES, socket_memoria);
}

int cantidad_instrucciones_deserializar(t_buffer *buffer) {
    // printf("Deserializa la cantidad de instrucciones.\n");
    int cantidad_instrucciones;

    void *stream = buffer->stream;
    memcpy(&cantidad_instrucciones, stream, sizeof(int));

    return cantidad_instrucciones;
}

// Manda a memoria el pcb, espera una instruccion y la devuelve
void pedir_instruccion_a_memoria(int socket_memoria, t_pcb *pcb) {
    // Recibimos cada parámetro
    t_solicitud_instruccion* pid_pc = malloc(sizeof(t_solicitud_instruccion)); //FREE --> Si
    pid_pc->pid = pcb->pid;
    pid_pc->pc = pcb->program_counter;

    t_buffer *buffer = llenar_buffer_solicitud_instruccion(pid_pc);
    enviar_paquete(buffer, QUIERO_INSTRUCCION, socket_memoria);
    free(pid_pc);
}

t_buffer *llenar_buffer_solicitud_instruccion(t_solicitud_instruccion *solicitud_instruccion) {
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 2;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream + buffer->offset, &solicitud_instruccion->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &solicitud_instruccion->pc, sizeof(int));

    buffer->stream = stream;

    return buffer;
}

t_paquete* recibir_memoria(int socket_memoria) {
    while(1) {
        t_paquete *paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir instruccion...\n");

        recv(socket_memoria, &(paquete->codigo_operacion), sizeof(codigo_operacion), MSG_WAITALL);
        printf("Codigo de operacion: %s\n", string_operacion(paquete->codigo_operacion));

        recv(socket_memoria, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        // printf("paquete->buffer->size: %d\n", paquete->buffer->size);

        paquete->buffer->stream = malloc(paquete->buffer->size);
        // recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);
        return paquete;
    }  
}

int recibir_cantidad_instrucciones(int socket_memoria, int pid) {
    t_paquete *paquete = recibir_memoria(socket_memoria);
    int cantidad_instrucciones = 0;

    // Ahora en función del código recibido procedemos a deserializar el resto
    switch (paquete->codigo_operacion) {
        case ENVIO_CANTIDAD_INSTRUCCIONES:
            cantidad_instrucciones = cantidad_instrucciones_deserializar(paquete->buffer);
            printf("Recibi que la cantidad de instrucciones del proceso %d es %d.\n", pid, cantidad_instrucciones);
            break;
        default:
            printf("Error: Fallo!\n");
            break;
    }

    liberar_paquete(paquete);
    return cantidad_instrucciones;
}

t_instruccion* recibir_instruccion(int socket_memoria, t_pcb *pcb) {
    t_paquete* paquete = recibir_memoria(socket_memoria);
    t_instruccion *instruccion;
    // Ahora en función del código recibido procedemos a deserializar el resto
    switch (paquete->codigo_operacion) {
        case ENVIO_INSTRUCCION:
            instruccion = instruccion_deserializar(paquete->buffer);
            break;
        default:
            printf("Error: Fallo!\n");
            break;
        }
        
    // Liberamos memoria
    liberar_paquete(paquete);

    return instruccion; // Podria ser NULL si recibe mal el codop
}

t_instruccion *instruccion_deserializar(t_buffer *buffer) {
    t_instruccion *instruccion = malloc(sizeof(t_instruccion)); //
    instruccion->parametros = list_create();

    void *stream = buffer->stream;

    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(instruccion->nombre), stream, sizeof(TipoInstruccion));
    stream += sizeof(TipoInstruccion);
    memcpy(&(instruccion->cantidad_parametros), stream, sizeof(int));
    stream += sizeof(int);

    // printf("Recibimos la instruccion de enum %d con %d parametros.\n", instruccion->nombre, instruccion->cantidad_parametros);

    if (instruccion->cantidad_parametros == 0) {
        return instruccion;
    }

    for (int i = 0; i < instruccion->cantidad_parametros; i++) {
        t_parametro *parametro = malloc(sizeof(t_parametro));
        memcpy(&(parametro->length), stream, sizeof(int));
        // printf("Este es el largo del parametro %d que llego: %d\n", i, parametro->length);
        stream += sizeof(int);
        
        parametro->nombre = malloc(parametro->length + 1);
        memcpy(parametro->nombre, stream, parametro->length);
        parametro->nombre[parametro->length] = '\0';
        stream += parametro->length;

        list_add(instruccion->parametros, parametro); 
        // free(parametro->nombre);
    }

    return instruccion;
}

bool es_de_8_bits(char *registro) {
    return (strcmp(registro, "AX") == 0 || strcmp(registro, "BX") == 0 ||
            strcmp(registro, "CX") == 0 || strcmp(registro, "DX") == 0);
}

/*

    int traer_tamanio_pagina_mem(int socket_mem) {
        recv(socket_mem,&tamanio_pagina, sizeof(int), 0);
        return tamanio_pagina;
    }

    void crear_direccion_logica(int tamanio_pagina) {
        t_direccion_logica* direccion_logica;
        int tamanio_pagina = traer_tamanio_pagina_mem();
        direccion_logica->numero_página = floor(direccion_lógica / tamanio_pagina);
        direccion_logica->desplazamiento = direccion_logica - direccion_logica->numero_página * tamanio_pagina;
    }

    CANTIDAD_ENTRADAS_TLB=32
    ALGORITMO_TLB=FIFO

    void* crear_tlb() {
        t_tlb tlb;
        int cant_entradas = config_get_int_value(config_CPU, "CANTIDAD_ENTRADAS_TLB");
        char* algoritmo_tlb = config_get_string_value(config_CPU, "ALGORITMO_TLB");
        if(cant_entradas == 0){
            deshabilitar_tlb();
        }
        return;
    }

t_queue* TLB = queue_create();

//va en el main
algoritmo_reemplazo_TLB = algoritmo_reemplazo_to_int(config_get_string_value(config, "REEMPLAZO_TLB"));

long tiempoEnMilisecs() {
  struct timeval time;
  gettimeofday(&time, NULL);

  return time.tv_sec * 1000 + time.tv_usec / 1000;
}

int esta_en_TLB(int nro_pagina){
	int se_encontro = 0;	
	for(int i = 0; i < queue_size(TLB); i++){
		t_tlb* una_entrada = queue_pop(TLB);
		if (una_entrada->pagina == nro_pagina){
			una_entrada->timestamp = tiempoEnMilisecs();
			se_encontro = 1;
		}
		queue_push(TLB, una_entrada);
	}
    
    if(se_encontro == 1)  
        logger(socket_cpu,"PID: %s - TLB HIT - Pagina: %d",una_entrada->pid,nro_pagina );
    else 
        logger(socket_cpu,"PID: %s - TLB MISS - Pagina: %d",una_entrada->pid,nro_pagina);
    
    return se_encontro;
}

int numero_pagina(int direccion_logica){
	// el tamaño página se solicita a la memoria al comenzar la ejecución // falta hacerlo
	return direccion_logica / tamanio_pagina; // ---> la división ya retorna el entero truncado a la unidad
}

int desplazamiento_memoria(int direccion_logica, int nro_pagina){
	return direccion_logica - nro_pagina * tamanio_pagina;
}

*/

int ejecutar_instruccion(t_instruccion *instruccion, t_pcb *pcb) {
    // log_info(logger, "PID: %d - Ejecutando: %d ", pcb->pid, instruccion->nombre);
    se_seteo_pc = false;
    TipoInstruccion nombreInstruccion = instruccion->nombre;
    t_list* list_parametros = instruccion->parametros;
    // printf("La instruccion es de numero %d y tiene %d parametros\n", instruccion->nombre, instruccion->cantidad_parametros);
    bool es_registro_uint8_dato;
    void* registro1;
    int direccion_fisica;
    char* nombre_registro_dato;
    char* nombre_registro_dir;
    size_t tamanio_en_byte;
    int esperar_confirm;
    int pagina;
    uint32_t default_value = 0;
    int cantidad_paginas;
    
    char* parametros_a_mostrar = obtener_lista_parametros(list_parametros);

    log_info(logger_CPU,"PID %d - Ejecutando %s %s",pcb->pid, pasar_instruccion_a_string(nombreInstruccion), parametros_a_mostrar);

    free(parametros_a_mostrar);

    switch (nombreInstruccion) // Ver la repeticion de logica... -> Abstraer
    {
    case SET:
        // Obtengo los datos de la lista de parametros
        t_parametro *registro_param = list_get(list_parametros, 0);
        registro1 = registro_param->nombre;

        t_parametro *valor_param = list_get(list_parametros, 1);

        char *valor_char = valor_param->nombre;
        int valor = atoi(valor_char);

        // Aca obtengo la direccion de memoria del registro
        void *registro_pcb1 = seleccionar_registro_cpu(registro1,pcb);

        // Aca obtengo si es de 8 bits o no y uso eso para setear, sino funciona el bool uso un int
        bool es_registro_uint8 = es_de_8_bits(registro1);
        set(registro_pcb1, valor, es_registro_uint8);

        /*printf("Cuando hace SET AX 1 queda asi el registro AX del CPU: %u\n", registros_cpu->AX);
        printf("Cuando hace SET BX 1 queda asi el registro BX del CPU: %u\n", registros_cpu->BX);*/
        break;
    case MOV_IN: // MOV_IN EDX SI
        t_parametro *registro_datos = list_get(list_parametros, 0);
        t_parametro *registro_direccion = list_get(list_parametros, 1);
        nombre_registro_dato = registro_datos->nombre;
        nombre_registro_dir = registro_direccion->nombre; 

        void *registro_dato_mov_in = seleccionar_registro_cpu(nombre_registro_dato,pcb);

        es_registro_uint8_dato = es_de_8_bits(nombre_registro_dato);
        uint32_t* registro_direccion_mov_in = (uint32_t*) seleccionar_registro_cpu(nombre_registro_dir,pcb);
        // bool es_registro_uint8_direccion = es_de_8_bits(nombre_registro_dir); // not used

        t_list* lista_bytes_lectura_mov_in = list_create();
        t_list* lista_direcciones_fisicas_mov_in = list_create(); 

        pagina = floor(*registro_direccion_mov_in / tamanio_pagina);
        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato);
        cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion_mov_in, tamanio_en_byte, pagina, lista_bytes_lectura_mov_in); // Cantidad de paginas + la primera
        
        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_lectura_mov_in, *registro_direccion_mov_in, pcb->pid, lista_direcciones_fisicas_mov_in, pagina);
        
        enviar_direcciones_fisicas(cantidad_paginas, lista_direcciones_fisicas_mov_in, &default_value, tamanio_en_byte, pcb->pid, LEER_DATO_MEMORIA);

        printf("Tamanio en byte: %ld\n", tamanio_en_byte);

        if(tamanio_en_byte == 1) {
            uint8_t* valor_en_mem_uint8 = malloc(tamanio_en_byte);
            recv(socket_memoria, valor_en_mem_uint8, tamanio_en_byte, MSG_WAITALL);
            uint8_t valor_recibido_8 = *(uint8_t*) valor_en_mem_uint8;
            printf("El valor recibido es: %d\n", valor_recibido_8);

            set_uint8(registro_dato_mov_in, valor_recibido_8);
            free(valor_en_mem_uint8);
        } else {
            uint32_t* valor_en_mem_uint32 = malloc(tamanio_en_byte);
            recv(socket_memoria, valor_en_mem_uint32, tamanio_en_byte, MSG_WAITALL);

            uint32_t valor_recibido_32 = *(uint32_t*) valor_en_mem_uint32;
            printf("El valor recibido es: %d\n", valor_recibido_32);

            set(registro_dato_mov_in, valor_recibido_32, es_registro_uint8_dato);
            free(valor_en_mem_uint32);
        }

        // printeo como qwueda AX Y EAX
        printf("Cuando hace MOV_IN AX SI queda asi el registro AX del CPU: %u\n", registros_cpu->AX);
        printf("Cuando hace MOV_IN EAX SI queda asi el registro EAX del CPU: %u\n", registros_cpu->EAX);

        list_destroy_and_destroy_elements(lista_bytes_lectura_mov_in, free);
        list_destroy_and_destroy_elements(lista_direcciones_fisicas_mov_in, free);

        break;
    case MOV_OUT: // MOV_OUT (Registro Dirección, Registro Datos)
        recibir_parametros_mov_out(list_parametros, &registro_direccion, &registro_datos, &nombre_registro_dato, &nombre_registro_dir);

        t_list* lista_bytes_lectura = list_create();
        t_list* lista_direcciones_fisicas_mov_out = list_create(); 

        uint32_t* registro_dato_mov_out = (uint32_t*)seleccionar_registro_cpu(nombre_registro_dato,pcb);
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_dato);
        bool registro_direccion_es_8_bits = es_de_8_bits(nombre_registro_dir);

        uint32_t* registro_direccion_mov_out = (uint32_t*)seleccionar_registro_cpu(nombre_registro_dir,pcb); // este es la direccion logica

        uint32_t var_register_mov_out;

        if(registro_direccion_es_8_bits) {
            var_register_mov_out = *(uint8_t*) registro_direccion_mov_out;
        } else {
            var_register_mov_out = *(uint32_t*) registro_direccion_mov_out;
        }
        
        pagina = floor(var_register_mov_out / tamanio_pagina);
        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato);

        cantidad_paginas = cantidad_de_paginas_a_utilizar(var_register_mov_out, tamanio_en_byte, pagina, lista_bytes_lectura); // Cantidad de paginas + la primera
        // imprimir lista de bytes
        // printf("Cantidad primera a copiar %d\n", *(int*)list_get(lista_bytes_lectura, 0));

        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_lectura, var_register_mov_out, pcb->pid, lista_direcciones_fisicas_mov_out, pagina);
        
        enviar_direcciones_fisicas(cantidad_paginas, lista_direcciones_fisicas_mov_out, (void*)registro_dato_mov_out, tamanio_en_byte, pcb->pid, ESCRIBIR_DATO_EN_MEM);
        // realizar_operacion(registro_direccion_mov_out, tamanio_en_byte, (void*)registro_dato_mov_out , 0, pcb->pid, ESCRIBIR_DATO_EN_MEM);

        recv(socket_memoria, &esperar_confirm, sizeof(int), MSG_WAITALL);

        //Acceso a espacio de usuario: PID: <PID> - Accion: <LEER / ESCRIBIR> - Direccion fisica: <DIRECCION_FISICA> - Tamaño <TAMAÑO A LEER / ESCRIBIR>
        
        // Este list_destroy elements funciona como quiere, ver si hay que liberar bien
        list_destroy_and_destroy_elements(lista_bytes_lectura, free);
        list_destroy_and_destroy_elements(lista_direcciones_fisicas_mov_out, free);

        break;
    case COPY_STRING:
        // int esperar_confirm_2;

        t_list* lista_bytes_lectura_cs = list_create();
        t_list* lista_direcciones_fisicas_cs = list_create(); 

        t_parametro* tamanio_a_copiar_de_lista = list_get(list_parametros, 0);
        int tamanio_a_copiar = atoi(tamanio_a_copiar_de_lista->nombre);

        uint32_t* registro_SI = (uint32_t*)seleccionar_registro_cpu("SI",pcb);
        uint32_t* registro_DI = (uint32_t*)seleccionar_registro_cpu("DI",pcb);

        void* valor_leido_cs = malloc(tamanio_a_copiar); // A mi no me importa si me traigo 50 bytes, quiero leer el tamanio que me pasaste

        pagina = floor(*registro_SI / tamanio_pagina);

        cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_SI, tamanio_a_copiar, pagina, lista_bytes_lectura_cs); // Cantidad de paginas + la primera

        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_lectura_cs, *registro_SI, pcb->pid, lista_direcciones_fisicas_cs, pagina);
        
        enviar_direcciones_fisicas(cantidad_paginas, lista_direcciones_fisicas_cs, &default_value, tamanio_a_copiar, pcb->pid, LEER_DATO_MEMORIA);
        // realizar_operacion(registro_direccion_mov_out, tamanio_en_byte, (void*)registro_dato_mov_out , 0, pcb->pid, ESCRIBIR_DATO_EN_MEM);

        recv(socket_memoria, valor_leido_cs, tamanio_a_copiar, MSG_WAITALL);
        // ((char*)valor_leido_cs)[tamanio_a_copiar] = '\0';
        // printf("El valor de memoria es: %s\n", (char*)valor_leido_cs);

        // Ahora hago lo mismo pero para escribir en DI
        t_list* lista_bytes_escritura_cs = list_create();
        t_list* lista_direcciones_fisicas_escritura_cs = list_create();

        pagina = floor(*registro_DI / tamanio_pagina);

        cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_DI, tamanio_a_copiar, pagina, lista_bytes_escritura_cs); // Cantidad de paginas + la primera

        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_escritura_cs, *registro_DI, pcb->pid, lista_direcciones_fisicas_escritura_cs, pagina);

        enviar_direcciones_fisicas(cantidad_paginas, lista_direcciones_fisicas_escritura_cs, valor_leido_cs, tamanio_a_copiar, pcb->pid, ESCRIBIR_DATO_EN_MEM);

        recv(socket_memoria, &esperar_confirm, sizeof(int), MSG_WAITALL);
        printf("Esperar confirmacion COPY STRING: %d\n", esperar_confirm);
        free(valor_leido_cs);
        
        list_destroy_and_destroy_elements(lista_bytes_lectura_cs, free);
        list_destroy_and_destroy_elements(lista_direcciones_fisicas_cs, free);
        break;
    case SUM: // SUM DESTINO ORIGEN
        t_parametro* registro_param1 = list_get(list_parametros, 0);
        char* registro_nombre = registro_param1->nombre;
        void* registro_destino = seleccionar_registro_cpu(registro_nombre,pcb);
        bool es_registro_uint8_destino = es_de_8_bits(registro_nombre);
        
        
        t_parametro* registro_param2 = list_get(list_parametros, 1); // valor_void = 2do registro
        char *registro2 = registro_param2->nombre;
        void* registro_origen = seleccionar_registro_cpu(registro2,pcb);
        bool es_registro_uint8_origen = es_de_8_bits(registro2);

        sum(registro_origen, registro_destino, es_registro_uint8_origen, es_registro_uint8_destino);
        printf("El registro %s tiene el valor %d\n", registro_nombre, *(uint32_t*)registro_destino);

        break;
    case SUB:
        t_parametro *registro_destino_param_resta = list_get(list_parametros, 0);
        char *registro_destino_resta = registro_destino_param_resta->nombre;
        void *registro_destino_pcb_resta = seleccionar_registro_cpu(registro_destino_resta,pcb);
        bool es_8_bits_destino_resta = es_de_8_bits(registro_destino_resta);
        
        t_parametro *registro_origen_param_resta = list_get(list_parametros, 1);
        char *registro_origen_resta = registro_origen_param_resta->nombre;
        void *registro_origen_pcb_resta = seleccionar_registro_cpu(registro_origen_resta,pcb);
        bool es_8_bits_origen_resta = es_de_8_bits(registro_origen_resta);
        

        sub(registro_origen_pcb_resta, registro_destino_pcb_resta, es_8_bits_origen_resta, es_8_bits_destino_resta);
        break;
    case JNZ:
        t_parametro *registro_parametro = list_get(list_parametros, 0);
        registro1 = registro_parametro->nombre;
        
        t_parametro *pc = list_get(list_parametros, 1);
        int nuevo_pc = atoi(pc->nombre);

        void *registro = seleccionar_registro_cpu(registro1, pcb);
        jnz(registro, nuevo_pc, pcb);
        break;
    case RESIZE:
        int devolucion_resize;
        t_parametro *registro_parametro_tamanio = list_get(list_parametros, 0);
        int nuevo_tamanio = atoi(registro_parametro_tamanio->nombre);

        enviar_tamanio_memoria(nuevo_tamanio, pcb->pid);
        recv(socket_memoria, &devolucion_resize, sizeof(int), MSG_WAITALL);

        if(devolucion_resize == 1) {
            log_info(logger_CPU, "OUT OF MEMORY");
            desalojar(pcb, OUT_OF_MEMORY, NULL);
            return 1;
        }

        printf("Se redimensiono la memoria\n"); 
        break;
    case WAIT: case SIGNAL:
        DesalojoCpu codigo = (nombreInstruccion == WAIT) ? WAIT_RECURSO : SIGNAL_RECURSO;
        t_parametro* parametro_recurso = list_get(list_parametros, 0);
        manejar_recursos(pcb, parametro_recurso, codigo);
        return 1;
        break; 
    case IO_GEN_SLEEP:
        t_parametro *interfaz1 = list_get(list_parametros, 0);
        char *interfazSeleccionada = interfaz1->nombre;

        printf("En la linea 476 tenemos esta interfaz: %s\n", interfazSeleccionada);

        t_parametro *unidades = list_get(list_parametros, 1);
        int unidadesDeTrabajo = atoi(unidades->nombre);

        t_buffer* buffer;
        buffer = llenar_buffer_dormir_IO(interfazSeleccionada, unidadesDeTrabajo);
        
        // el puntero al buffer de abajo YA ESTA SERIALIZADO
        
        // pcb->program_counter++;
        desalojar(pcb, DORMIR_INTERFAZ, buffer);
        printf("Hizo IO_GEN_SLEEP con el PID %d\n", pcb->pid);
        return 1; // Retorna 1 si desalojo PCB...
        break;
    case IO_STDIN_READ: 
        t_list* lista_bytes_stdin = list_create();
        t_list* lista_direcciones_fisicas_stdin = list_create();

        t_parametro* interfaz = list_get(list_parametros,0);
        registro_direccion = list_get(list_parametros, 1); // Cambiar, usa el de arriba, mejor nombre?
        t_parametro *registro_tamanio = list_get(list_parametros, 2);

        // int tamanio_a_copiar_en_mem = atoi(registro_tamanio->nombre);
       //  printf("El tamanio a copiar es: %d\n", tamanio_a_copiar_en_mem);

        char* nombre_registro_direccion = registro_direccion->nombre;
        char* nombre_registro_tamanio = registro_tamanio->nombre;

        void* registro_direccion_stdin = seleccionar_registro_cpu(nombre_registro_direccion,pcb);
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_tamanio);
        bool es_registro_uint8_dato_register = es_de_8_bits(nombre_registro_direccion);
        
        uint32_t* registro_tamanio_stdin = (uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio,pcb);

        uint32_t var_register;

        if(es_registro_uint8_dato_register) {
            var_register = *(uint8_t*) registro_direccion_stdin;
        } else {
            var_register = *(uint32_t*) registro_direccion_stdin;
        }

        uint32_t tamanio_a_leer;
        
        // Hacer lo mismo que arriba para el tamanio
        if(es_registro_uint8_dato) {
            tamanio_a_leer = *(uint8_t*) registro_tamanio_stdin;
        } else {
            tamanio_a_leer = *(uint32_t*) registro_tamanio_stdin;
        }
       
        printf("El valor de la direccion es: %d\n", var_register);
        
        int pagina = floor(var_register / tamanio_pagina);
        printf("la pagina es HOLIHOLI %d", pagina);

        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato); // Ojo que abajo no le paso el tam_byte, sino la cantidad que tiene dentro

        int cantidad_paginas = cantidad_de_paginas_a_utilizar(var_register, *registro_tamanio_stdin, pagina, lista_bytes_stdin); // Cantidad de paginas + la primera
        
        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_stdin, var_register, pcb->pid, lista_direcciones_fisicas_stdin, pagina);

        printf("\nAca va la lista de bytes del STDIN READ:\n");
        for(int i=0; i < list_size(lista_direcciones_fisicas_stdin); i++) {
            t_dir_fisica_tamanio* dir_stdin = list_get(lista_direcciones_fisicas_stdin, i);
            printf("Direccion fisica: %d\n", dir_stdin->direccion_fisica);
            printf("Bytes a leer: %d\n", dir_stdin->bytes_lectura);
            log_info(logger_CPU,"PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d",pcb->pid, dir_stdin->direccion_fisica, var_register);
        }

        t_buffer* buffer_lectura = llenar_buffer_stdio(interfaz->nombre, lista_direcciones_fisicas_stdin, *registro_tamanio_stdin, cantidad_paginas);
        
        // pcb->program_counter++;
        desalojar(pcb, PEDIDO_LECTURA, buffer_lectura);

        // free(buffer_lectura);
        list_destroy_and_destroy_elements(lista_bytes_stdin, free);
        list_destroy_and_destroy_elements(lista_direcciones_fisicas_stdin, free);
        return 1; // El return esta para que cuando se desaloje el pcb no siga ejecutando, si hace el break sigue pidiendo instrucciones, porfa no lo saquen o les va a romper
        break;
    case EXIT_INSTRUCCION:
        // guardar_estado(pcb); -> No estoy seguro si esta es necesaria, pero de todas formas nos va a servir cuando se interrumpa por quantum
        printf("Llego a instruccion EXIT\n");
        desalojar(pcb, FIN_PROCESO, NULL); // Envio 0 ya que no me importa size
        return 1;
        break;
    case IO_STDOUT_WRITE: // IO_STDOUT_WRITE Int3 BX EAX
        t_list* lista_bytes_stdout = list_create();
        t_list* lista_direcciones_fisicas_stdout = list_create();

        t_parametro* parametro_interfaz = list_get(list_parametros,0);
        char* nombre_interfaz = parametro_interfaz->nombre;
        
        t_parametro* parametro_registro_direccion = list_get(list_parametros,1);
        char* nombre_registro_direccion_stdout = parametro_registro_direccion->nombre;
        
        t_parametro* parametro_registro_tamanio = list_get(list_parametros,2);
        char* nombre_registro_tamanio_stdout = parametro_registro_tamanio->nombre;

        // int tamanio_a_leer_en_mem = atoi(nombre_registro_tamanio_stdout);
        // printf("El tamanio a leer en memoria es: %d\n", tamanio_a_leer_en_mem); // deberia decir 25

        void* registro_direccion11 = seleccionar_registro_cpu(nombre_registro_direccion_stdout,pcb);
        bool es_registro_uint8_dato_register_2 = es_de_8_bits(nombre_registro_direccion_stdout);

        uint32_t* registro_tamanio_stdout = (uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio_stdout,pcb);
        
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_tamanio_stdout);
        
        // direccion_fisica = traducir_direccion_logica_a_fisica(*registro_direccion11, pcb->pid);

        // CPU -> KERNEL -> MEMORIA -> ENTRADA SALIDA
        uint32_t valor_a_enviar;

        if(es_registro_uint8_dato_register_2){
            valor_a_enviar = *(uint8_t*) registro_direccion11;
        } else {
            valor_a_enviar = *(uint32_t*) registro_direccion11;
        }
        
        uint32_t tamanio_a_escribir;

        // Hacer lo mismo que arriba pero con el registro de tamanio
        if(es_registro_uint8_dato) {
            tamanio_a_escribir = *(uint8_t*) registro_tamanio_stdout;
        } else {
            tamanio_a_escribir = *(uint32_t*) registro_tamanio_stdout;
        }

        pagina = floor(valor_a_enviar / tamanio_pagina);
         
        printf("el valor a enviar es %d \n", valor_a_enviar);
        printf("la pagina es HOLIHOLI %d", pagina);

        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato); // Ojo que abajo no le paso el tam_byte, sino la cantidad que tiene dentro
                                                            // 11 25 1
        cantidad_paginas = cantidad_de_paginas_a_utilizar(valor_a_enviar, tamanio_a_escribir, pagina, lista_bytes_stdout); // Cantidad de paginas + la primera
        
        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_stdout, valor_a_enviar, pcb->pid, lista_direcciones_fisicas_stdout, pagina);
        
        printf("\nAca va la lista de bytes DEL STDOUT:\n");
        for(int i=0; i < list_size(lista_direcciones_fisicas_stdout); i++) {
            t_dir_fisica_tamanio* dir = list_get(lista_direcciones_fisicas_stdout, i);
            printf("Direccion fisica: %d\n", dir->direccion_fisica);
            printf("Bytes a leer: %d\n", dir->bytes_lectura);
            log_info(logger_CPU,"PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d", pcb->pid, dir->direccion_fisica,valor_a_enviar);
        }

        printf("\n");
        t_buffer* buffer_escritura = llenar_buffer_stdio(nombre_interfaz, lista_direcciones_fisicas_stdout, tamanio_a_escribir, cantidad_paginas);
        // pcb->program_counter++;

        desalojar(pcb, PEDIDO_ESCRITURA, buffer_escritura);
        
        list_destroy_and_destroy_elements(lista_bytes_stdout, free);
        list_destroy(lista_direcciones_fisicas_stdout);
        // free(buffer_escritura); se hace en desalojar
        return 1;
        break;
    /*
    case IO_FS_CREATE: case IO_FS_DELETE: // IO_FS_CREATE nombre_interfaz nombre_archivo
        t_parametro* interfaz_create = list_get(list_parametros, 0);
        char* nombre_interfaz_FS_CREATE = interfaz_create->nombre;

        t_parametro* name_file_create = list_get(list_parametros, 1);
        char* filename = name_file_create->nombre;

        codigo_operacion codigo_io = (nombreInstruccion == IO_FS_CREATE) ? FS_CREATE : FS_DELETE;
        t_buffer* buffer_io = llenar_buffer_fs_create_delete(nombre_interfaz_FS_CREATE, filename);
        desalojar(pcb, codigo_io, buffer_io);

        // Libero y desalojo...
        free(buffer_io);
        return -1;
        break;
    case IO_FS_TRUNCATE: // IO_FS_TRUNCATE interfaz archivo 
        t_parametro* interfaz_a_truncar = list_get(list_parametros, 0);
        char* nombre_interfaz_a_truncar = interfaz_create->nombre;

        t_parametro* parametro_file_truncate = list_get(list_parametros, 1);
        char* nombre_file_truncate = parametro_file_truncate->nombre;

        t_parametro* parametro_numero = list_get(list_parametros, 2);
        char* nombre_parametro_numero = parametro_numero->nombre;
        uint32_t registro_truncador = (uint32_t*) seleccionar_registro_cpu(nombre_parametro_numero);
        
        enviar_buffer_fs_truncate(nombre_interfaz_a_truncar,nombre_file_truncate,registro_truncador);
        
        return 1;
        break;
    case IO_FS_READ:
    case IO_FS_WRITE:
        t_parametro* primer_parametro2 = list_get(list_parametros, 0);
        char* nombre_interfaz2 = primer_parametro2->nombre;

        t_parametro* segundo_parametro2 = list_get(list_parametros, 1);
        char* nombre_archivo2 = segundo_parametro2->nombre;
        
        t_parametro* tercer_parametro = list_get(list_parametros, 2);
        char* nombre_registro_direccion2 = tercer_parametro->nombre; 
        uint32_t registro_direccion2 = *(uint32_t*) seleccionar_registro_cpu(nombre_registro_direccion2);
        
        t_parametro* cuarto_parametro = list_get(list_parametros, 3);
        char* nombre_registro_tamanio2 = cuarto_parametro->nombre; 
        uint32_t registro_tamanio2 = *(uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio2);

        t_parametro* quinto_parametro = list_get(list_parametros, 4);
        char* nombre_quinto_parametro = cuarto_parametro->nombre; 
        uint32_t registro_puntero_archivo = *(uint32_t*) seleccionar_registro_cpu(nombre_quinto_parametro);

        codigo_operacion codigo_escritura_lectura = nombreInstruccion == IO_FS_READ ? LECTURA_FS : ESCRITURA_FS;

        t_buffer* buffer_fs_lectura_escritura = llenar_buffer_fs_escritura_lectura(nombre_interfaz2,nombre_archivo2,registro_direccion2,registro_tamanio2,registro_puntero_archivo); 
        enviar_paquete(buffer_fs_lectura_escritura,codigo_escritura_lectura,client_dispatch);
        
        return 1;
        break;
    */
    default:
        printf("Error: No existe ese tipo de instruccion\n");
        // instruccion_destruir(instruccion);
        printf("La instruccion recibida es: %d\n", nombreInstruccion);
        desalojar(pcb, FIN_PROCESO, NULL);
        
        // Hasta que encontremos como hacer con el EXIT que recibe como E X I T
        break;
    }
    
    if(!se_seteo_pc) {
        pcb->program_counter++;
    }

    return 0;
}

char* pasar_instruccion_a_string(TipoInstruccion nombreInstruccion) {
    switch(nombreInstruccion) {
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
        case ERROR_INSTRUCCION: return "ERROR_INSTRUCCION";
        default: return "UNKNOWN_INSTRUCTION";
    }
}
// Suponiendo que t_list es una lista de punteros a t_parametro
// y que hay funciones list_size y list_get definidas para manejar esta lista.

char* obtener_lista_parametros(t_list* list_parametros) {
    int total_length = 1;  // Empezamos en 1 para incluir el terminador nulo.
    int list_length = list_size(list_parametros);

    // Calcular la longitud total necesaria.
    for (int i = 0; i < list_length; i++) {
        t_parametro* parametro = list_get(list_parametros, i);
        total_length += strlen(parametro->nombre) + 1;  // Incluyendo el espacio y el terminador nulo.
    }

    char* argumentos_a_mostrar = malloc(total_length);

    argumentos_a_mostrar[0] = '\0';  // Inicializar la cadena vacía.

    for (int i = 0; i < list_length; i++) {
        t_parametro* parametro = list_get(list_parametros, i);
        strcat(argumentos_a_mostrar, parametro->nombre);
        if (i < list_length - 1) {
            strcat(argumentos_a_mostrar, " ");
        }
    }

    return argumentos_a_mostrar;
}

/* 
int bytes_usables_por_pagina(int direccion_logica) {
    int offset = direccion_logica % tamanio_pagina;
    int devolucion_lectura =  tamanio_pagina - offset;
    return devolucion_lectura;
}
*/

/* NOTA FACU -> ESTO HABRIA QUE VER DE OPTIMIZARLO MAS, LO HICE ASI PARA QUE FUNCIONE, PERO CAMBIANDO UN PAR DE COSAS EN LA TRADUCCION CREO QUE
           SE PUEDE HACER MEJOR Y AHORRAR UNA BANDA DE CODIGO (PARA DESPUES)                    */
void cargar_direcciones_tamanio(int cantidad_paginas, t_list* lista_bytes_lectura, uint32_t direccion_logica, int pid, t_list* direcciones_fisicas, int pagina) {
    // Aca cargo la primera
    t_dir_fisica_tamanio *dir_fisica_tamanio = malloc(sizeof(t_dir_fisica_tamanio)); 
    log_info(logger_CPU, "Direccion logica: %d\n", direccion_logica);

    dir_fisica_tamanio->direccion_fisica = traducir_direccion_logica_a_fisica(direccion_logica, pid);
    dir_fisica_tamanio->bytes_lectura = *(int*)list_get(lista_bytes_lectura, 0); 
    log_info(logger_CPU, "direccion fisica: %d", dir_fisica_tamanio->direccion_fisica);
    log_info(logger_CPU, "tamanio guardado: %d", dir_fisica_tamanio->bytes_lectura);
            
    list_add(direcciones_fisicas, dir_fisica_tamanio); 

    int frame;
    // Aca cargo el resto

    for(int i = 0; i < cantidad_paginas - 1; i++) {
        printf("Iteracion %d\n", i);
        
        log_info(logger_CPU, "HELP Pido el marco de la pagina %d del proceso %d", pagina + 1 + i, pid); // El uno es para que siempre pida la sig
        
        frame = buscar_frame_en_TLB(pid, pagina + 1 + i);

        if(frame == -1) {
            printf("ENTRO POR ACA\n");
            pedir_frame_a_memoria(pagina + 1 + i, pid);
            printf("pedi el frame de la pagina : %d\n", pagina + 1 + i);
            
            recv(socket_memoria, &frame, sizeof(int), MSG_WAITALL);
        }
        
        printf("el frame es %d", frame);

        int direccion_fisica = frame * tamanio_pagina + 0; // 0 es el offset

        t_dir_fisica_tamanio *dir_fisica_tamanio_2 = malloc(sizeof(t_dir_fisica_tamanio));
        dir_fisica_tamanio_2->direccion_fisica = direccion_fisica;
        dir_fisica_tamanio_2->bytes_lectura = *(int*)list_get(lista_bytes_lectura, i+1); 

        printf("direccion fisica: %d\n", dir_fisica_tamanio_2->direccion_fisica);
        printf("tamanio guardado: %d\n", dir_fisica_tamanio_2->bytes_lectura);

        list_add(direcciones_fisicas, dir_fisica_tamanio_2); 

        // pagina++;
    }
}

void enviar_direcciones_fisicas(int cantidad_paginas, t_list* direcciones_fisicas, void* registro_dato_mov_out, int tamanio_valor, int pid, codigo_operacion cod_op) {
    t_buffer* buffer = serializar_direcciones_fisicas(cantidad_paginas, direcciones_fisicas, registro_dato_mov_out, tamanio_valor, pid);
    enviar_paquete(buffer, cod_op, socket_memoria);
}

t_buffer* serializar_direcciones_fisicas(int cantidad_paginas, t_list* direcciones_fisicas, void* registro_dato_mov_out, int tamanio_valor, int pid) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    int cant_elementos_lista = list_size(direcciones_fisicas);
    log_info(logger_CPU, "el tamanio de la lista de direcciones fisicas de enviar direcciones fisicas es %d \n", cant_elementos_lista);

    buffer->size = sizeof(int) * 4 + (cant_elementos_lista * sizeof(t_dir_fisica_tamanio)) + tamanio_valor;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &pid, sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(stream + buffer->offset, &cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &tamanio_valor, sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(stream + buffer->offset, registro_dato_mov_out, tamanio_valor);
    buffer->offset += tamanio_valor;

    printf("VOY A IMPRIMIR LAS DIRECCIONES FISICAS\n");
    for(int i=0; i < cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(direcciones_fisicas, i);
        printf("Direccion fisica: %d\n", dir_fisica_tam->direccion_fisica);
        printf("Bytes a leer: %d\n", dir_fisica_tam->bytes_lectura);

        memcpy(stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
        free(dir_fisica_tam);
    }
    list_destroy(direcciones_fisicas);

    buffer->stream = stream;

    return buffer;
}

void recibir_parametros_mov_out(t_list* list_parametros, t_parametro **registro_direccion, t_parametro **registro_datos, char** nombre_registro_dato, char** nombre_registro_dir) {
    *registro_direccion = list_get(list_parametros, 0);
    *registro_datos = list_get(list_parametros, 1);
    *nombre_registro_dato = (*registro_datos)->nombre;
    *nombre_registro_dir = (*registro_direccion)->nombre;
}

int tamanio_byte_registro(bool es_registro_uint8) {
    return es_registro_uint8 ? 1 : 4;
}


int primer_byte_anterior_a_dir_logica(uint32_t direccion_logica, int tamanio_pagina) {
    // Dir logica en 198, con un tam pagina de 32, el primer byte seria 192 
    // Dir logica en 224, con un tam pagina de 32, el primer byte seria 224
    // Dir logica en 221, con un tam pagina de 32, el primer byte seria 192
    // Dir logica 16, con un tam pagina de 32, el primer byte seria 0
    
    int offset_movido = direccion_logica % tamanio_pagina; // 11 % 16 = 11 ✔ - 198 % 32 = 6 ✔ 16 % 16 = 0 ✔
    printf("El offset movido dentro del calculo del primer byte es %d\n", offset_movido);

    int primer_byte = direccion_logica - offset_movido; // 16 - 16 = 0 ✔ - 198 - 6 = 192 ✔ - 16 - 0 = 16 ✔ 
    return primer_byte;
}

/***** ACLARACION *******/
int cantidad_de_paginas_a_utilizar(uint32_t direccion_logica, int tamanio_en_bytes, int pagina, t_list* lista_bytes_lectura) { // Dir logica 5 tamanio 17
    int cantidad_paginas = 1; // (5)

   // imprimir la dir logica y el tam de una pagina
    printf("Direccion logica: %d\n", direccion_logica);
    printf("Tamanio pagina: %d\n", tamanio_pagina);

    int primer_byte_anterior = primer_byte_anterior_a_dir_logica(direccion_logica, tamanio_pagina); // 0 ✔ - 192 ✔
    int offset_movido = direccion_logica - primer_byte_anterior; // 16 - 0 = 16 ✔ - 198 - 192 = 6 ✔
    printf("el offset movido es %d", offset_movido);
    /* 
    if(direccion_logica > tamanio_pagina) {
        offset_movido = direccion_logica % tamanio_pagina; // 198 224 = abs(dir_logica - tam_pagina * pagina)  = abs(-26) = 26 primer byte pagina en la que esta
        printf("el offset es %d", offset_movido);
    } else {
        int numero = direccion_logica / tamanio_pagina;
        offset_movido = direccion_logica - tamanio_pagina * numero;
        printf("el offset es %d", offset_movido);
    }
    */

    while(1) { 
        int posible_lectura = tamanio_pagina - offset_movido;  // 3
        printf("tamanio en bytes: %d\n", tamanio_en_bytes); // 17 -> 14 -> 10 -> 6 -> 2
        printf("Posible lectura: %d\n", posible_lectura); // 3 -> 4 -> 4 -> 4
        int sobrante_de_pagina = tamanio_en_bytes - posible_lectura; 
        printf("Sobrante de pagina: %d\n", sobrante_de_pagina); 

        if(sobrante_de_pagina <= 0) {
            int* tam_bytes = malloc(sizeof(int));
            *tam_bytes = tamanio_en_bytes; 
            // int* tam_bytes = tamanio_en_bytes; ???
            printf("Tamanio LECTURA: %d\n", *tam_bytes); // 17 -> 14 -> 10 -> 6 -> 2
            list_add(lista_bytes_lectura, tam_bytes); //VER_SI_HAY_FREE
            break;
        }
        
        // Si sobra espacio en la pagina, se agrega a la lista de bytes a leer
        int* tam_posible_lectura = malloc(sizeof(int)); //VER_SI_HAY_FREE
        *tam_posible_lectura = posible_lectura; 

        printf("Tamanio posible lectura: %d\n", *tam_posible_lectura); // 3 -> 4 -> 4 -> 4
        list_add(lista_bytes_lectura, tam_posible_lectura); 
        
        cantidad_paginas++; // 2 (8) -> 3 (12) -> 4 (16) -> 5 (20)
        offset_movido = 0;
        
        tamanio_en_bytes = sobrante_de_pagina; // 14 -> 10 -> 6 -> 2
    } 

    return cantidad_paginas;
}

void enviar_tamanio_memoria(int nuevo_tamanio, int pid) {
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 2; 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &nuevo_tamanio, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pid, sizeof(int));

    enviar_paquete(buffer, RESIZE_MEMORIA, socket_memoria); 
}

void desalojar(t_pcb* pcb, DesalojoCpu motivo, t_buffer* datos_adicionales) {
    if(motivo  != INTERRUPCION_QUANTUM) {
        pcb->program_counter++;   
    }

    guardar_estado(pcb);
    setear_registros_cpu(); 
    enviar_pcb(pcb, client_dispatch, motivo, datos_adicionales);
}

void solicitud_dormirIO_kernel(char* interfaz, int unidades) {
    t_buffer* buffer = llenar_buffer_dormir_IO(interfaz, unidades);
    enviar_paquete(buffer, DORMIR_INTERFAZ, client_dispatch);
}

t_buffer* llenar_buffer_dormir_IO(char* interfaz, int unidades) {
    int length_interfaz = string_length(interfaz) + 1;

    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_operacion_io* io = malloc(sizeof(t_operacion_io));

    io->nombre_interfaz_largo = length_interfaz;
    io->nombre_interfaz = malloc(length_interfaz);
    strcpy(io->nombre_interfaz, interfaz);
    io->unidadesDeTrabajo = unidades;
    printf("Este es el nombre de la Interfaz segun operaciones.c: %s\n", io->nombre_interfaz);

    buffer->size = sizeof(int) * 2 + length_interfaz;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &io->unidadesDeTrabajo, sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &io->nombre_interfaz_largo, sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, io->nombre_interfaz, io->nombre_interfaz_largo);
    printf("nombre interfaz lalala %s\n",  io->nombre_interfaz);
    // buffer->offset += io->nombre_interfaz_largo;
    
    free(io->nombre_interfaz);
    free(io);
    return buffer;
}

/* 
void enviar_fin_programa(t_pcb *pcb) {
    enviar_pcb(pcb, client_dispatch, FIN_PROCESO, NULL, 0);
}
*/

void guardar_estado(t_pcb *pcb) {
    pcb->registros->AX = registros_cpu->AX;
    printf("Guardo el AX: %d\n", pcb->registros->AX);
    pcb->registros->BX = registros_cpu->BX;
    printf("Guardo el BX: %d\n", pcb->registros->BX);
    pcb->registros->CX = registros_cpu->CX;
    pcb->registros->DX = registros_cpu->DX;
    pcb->registros->EAX = registros_cpu->EAX;
    pcb->registros->EBX = registros_cpu->EBX;
    pcb->registros->ECX = registros_cpu->ECX;
    pcb->registros->EDX = registros_cpu->EDX;
    pcb->registros->SI = registros_cpu->SI;
    pcb->registros->DI = registros_cpu->DI;
}

// Deberia estar en otro lado
void setear_registros_cpu() {
    registros_cpu->AX = 0;
    registros_cpu->BX = 0;
    registros_cpu->CX = 0;
    registros_cpu->DX = 0;

    registros_cpu->EAX = 0;
    registros_cpu->EBX = 0;
    registros_cpu->ECX = 0;
    registros_cpu->EDX = 0;

    registros_cpu->SI = 0;
    registros_cpu->DI = 0;
}

void *seleccionar_registro_cpu(char *nombreRegistro, t_pcb* pcb) {
    if (strcmp(nombreRegistro, "AX") == 0)
    {
        return &registros_cpu->AX;
    }
    else if (strcmp(nombreRegistro, "BX") == 0)
    {
        return &registros_cpu->BX;
    }
    else if (strcmp(nombreRegistro, "CX") == 0)
    {
        return &registros_cpu->CX;
    }
    else if (strcmp(nombreRegistro, "DX") == 0)
    {
        return &registros_cpu->DX;
    }
    else if (strcmp(nombreRegistro, "SI") == 0)
    {
        return &registros_cpu->SI;
    }
    else if (strcmp(nombreRegistro, "DI") == 0)
    {
        return &registros_cpu->DI;
    }
    else if (strcmp(nombreRegistro, "EAX") == 0)
    {
        return &registros_cpu->EAX;
    }
    else if (strcmp(nombreRegistro, "EBX") == 0)
    {
        return &registros_cpu->EBX;
    }
    else if (strcmp(nombreRegistro, "ECX") == 0)
    {
        return &registros_cpu->ECX;
    }
    else if (strcmp(nombreRegistro, "EDX") == 0)
    {
        return &registros_cpu->EDX;
    }
    else if (strcmp(nombreRegistro,"PC") == 0){
        se_seteo_pc = true;
        return &pcb->program_counter;
    }

    return NULL;
}

///////////////////////////
/////  INSTRUCCIONES //////
///////////////////////////

// Esta funcion transforma ese uint8 en un uint32;

void set(void *registro, uint32_t valor, bool es_8_bits) {
    // registro = &valor;

    if (es_8_bits) {
        *(uint8_t *)registro = (uint8_t)valor; // NO va a haber ninguno registro de 8 bits que pase el limite
    } else {
        *(uint32_t *)registro = valor;
    }
}

// NO SACAR ESTA FUNCION POR MAS QUE SSEA UNA LINEA
void set_uint8(void *registro, uint8_t valor) {
    *(uint8_t *)registro = (uint8_t)valor; 
}

void sub(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino) {
    uint32_t resultado_resta;

    if (es_8_bits_origen && es_8_bits_destino) {
        resultado_resta = *(uint8_t *)registroDestino - *(uint8_t *)registroOrigen;
    } else if (es_8_bits_origen) {
        resultado_resta = *(uint32_t *)registroDestino - *(uint8_t *)registroOrigen;
    } else if (es_8_bits_destino) {
        resultado_resta = *(uint8_t *)registroDestino - *(uint32_t *)registroOrigen;
    } else {
        resultado_resta = *(uint32_t *)registroDestino - *(uint32_t *)registroOrigen;
    }

    set(registroDestino, resultado_resta, es_8_bits_destino);
}

void sum(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino) {
    //uint32_t valor_origen;
    //uint32_t valor_destino;
    uint32_t resultado_suma;
    /* 
    if(es_8_bits_destino && es_8_bits_origen) {
        resultado_suma = *(uint8_t *)registroOrigen + *(uint8_t *)registroDestino;
        set(registroDestino, resultado_suma, es_8_bits_destino);
    } else {
        resultado_suma = *(uint32_t *)registroOrigen + *(uint32_t *)registroDestino;
        set(registroDestino, resultado_suma, es_8_bits_destino);
    }
    
    */
    if (es_8_bits_origen && es_8_bits_destino) {
        resultado_suma = *(uint8_t *)registroOrigen + *(uint8_t *)registroDestino;
    } else if (es_8_bits_origen) {
        resultado_suma = *(uint8_t *)registroOrigen + *(uint32_t *)registroDestino;
    } else if (es_8_bits_destino) {
        resultado_suma = *(uint32_t *)registroOrigen + *(uint8_t *)registroDestino;
    } else {
        resultado_suma = *(uint32_t *)registroOrigen + *(uint32_t *)registroDestino;
    }

    set(registroDestino, resultado_suma, es_8_bits_destino);
    
        /*
    // Obtener el valor del registro origen
    if (es_8_bits_origen) {
        valor_origen = *(uint8_t *)registroOrigen;
    } else {
        valor_origen = *(uint32_t *)registroOrigen;
    }

    // Obtener el valor del registro destino
    if (es_8_bits_destino) {
        valor_destino = *(uint8_t *)registroDestino;
    } else {
        valor_destino = *(uint32_t *)registroDestino;
    }

    // Realizar la suma
    uint32_t resultado_suma = valor_origen + valor_destino;

     // Asegurarse de que el resultado no cause desbordamiento
    if (resultado_suma < valor_origen) {
        // Manejo de desbordamiento, si es necesario
        // Por ejemplo, puedes establecer un flag, lanzar una excepción, etc.
    }

    // Actualizar el registro destino con el resultado usando la función set
    set(registroDestino, resultado_suma, es_8_bits_destino);*/
}

void jnz(void* registro, int valor, t_pcb* pcb) {
    if(*(int*) registro != 0) {
        pcb->program_counter = valor;
    }
}

void manejar_recursos(t_pcb* pcb, t_parametro* recurso, DesalojoCpu codigo) {
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) + recurso->length + 1 ;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &recurso->length, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, recurso->nombre, recurso->length);
    
    printf("el nombre del rec es %s\n", recurso->nombre);
    printf("el length del rec es %d\n", recurso->length);

    buffer->stream = stream;    
    desalojar(pcb, codigo, buffer);
}

void enviar_buffer_copy_string(int direccion_fisica_SI, int direccion_fisica_DI, int tamanio) {
    t_buffer* buffer = llenar_buffer_copy_string(direccion_fisica_SI,direccion_fisica_DI,tamanio);
    enviar_paquete(buffer, COPY_STRING_MEMORIA ,socket_memoria);
}

t_buffer* llenar_buffer_copy_string(int direccion_fisica_SI, int direccion_fisica_DI, int tamanio){ 
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 3;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &direccion_fisica_SI, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &direccion_fisica_DI, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &tamanio, sizeof(int));
    buffer->offset += sizeof(int);
    
    buffer->stream = stream;

    return buffer;
}

void enviar_kernel_stdout(char* nombre_interfaz, int direccion_fisica, uint32_t tamanio){
    t_buffer* buffer = llenar_buffer_stdout(direccion_fisica, nombre_interfaz, tamanio);
    enviar_paquete(buffer, PEDIDO_ESCRITURA, client_dispatch);
}

t_buffer* llenar_buffer_stdout(int direccion_fisica, char* nombre_interfaz, uint32_t tamanio){
    t_buffer *buffer = malloc(sizeof(t_buffer));
    int largo_nombre = string_length(nombre_interfaz);

    buffer->size = sizeof(int) + sizeof(largo_nombre) + sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;
    printf("la direccion fisica de llenar buffer stdout es: %d \n", direccion_fisica);
    printf("la tamanio de llenar buffer stdout es: %d \n", direccion_fisica);
    memcpy(stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &(largo_nombre), sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, nombre_interfaz, largo_nombre);
    buffer->stream = stream;

    free(nombre_interfaz);

    return buffer;
}


/****
 NOTA -> Ya lo puse en un issue pero igual lo pongo tambien por aca asi lo ven. Cuando arregle el ultimo problema de serializacion del manejo del recurso, lo hice igual que aca, 
 hay memory leak pero es la unica forma que encontre por ahora de que funcione, sino se serializaba mal y no se mandaba bien el buffer. Con el stream solo no hay problema de mem leak, porque 
 no se pierde la referencia, pero no funciona nose porque. Esto pasa con todas los buffers que se mandan adicional al PCB (creo que hasta ahora son 3 o 4)
 *****/


t_buffer* llenar_buffer_stdio(char* interfaz, t_list* direcciones_fisicas, uint32_t tamanio_a_copiar, int cantidad_paginas) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    int largo_interfaz = string_length(interfaz) + 1;
    printf("Largo de la interfaz: %d\n", largo_interfaz);

    int size_direcciones_fisicas = list_size(direcciones_fisicas); 

    buffer->size = sizeof(int) * 2+ sizeof(uint32_t) + (size_direcciones_fisicas * sizeof(int) * 2) + largo_interfaz; 

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // Ver posible problema de serializacion como paso la otra vez que lo adjuntamos al pcb
    // void* stream = buffer->stream;

    memcpy(buffer->stream + buffer->offset, &cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);

    // Serializacion de la lista -> Ahora con la lista es una incognita si esto funciona bien inclusive con el mem leak, una cagada
    for(int i=0; i < cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(direcciones_fisicas, i);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(buffer->stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);

        free(dir_fisica_tam);
    }

    memcpy(buffer->stream + buffer->offset, &tamanio_a_copiar, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    printf("el tamanio a copiar es %d", tamanio_a_copiar);

    memcpy(buffer->stream + buffer->offset, &largo_interfaz, sizeof(int));
    buffer->offset += sizeof(int);
    
    memcpy(buffer->stream + buffer->offset, interfaz, largo_interfaz);

    // buffer->stream = stream;

    return buffer;
}

t_buffer* llenar_buffer_fs_escritura_lectura(char* nombre_interfaz,char* nombre_archivo,uint32_t registro_direccion,uint32_t registro_tamanio,uint32_t registro_archivo){ 
    uint32_t largo_nombre_interfaz = string_length(nombre_interfaz) + 1;
    uint32_t largo_nombre_archivo  = string_length(nombre_archivo) + 1;
    int size = sizeof(int) * 2 + largo_nombre_interfaz + largo_nombre_archivo + sizeof(uint32_t) * 3;
    
    t_buffer* buffer  = buffer_create(size);

    buffer_add_uint32(buffer,largo_nombre_interfaz);
    buffer_add_string(buffer,largo_nombre_interfaz,nombre_interfaz);
    
    buffer_add_uint32(buffer,largo_nombre_archivo);
    buffer_add_string(buffer,largo_nombre_archivo,nombre_archivo);

    buffer_add_uint32(buffer,registro_direccion);
    buffer_add_uint32(buffer,registro_tamanio);
    buffer_add_uint32(buffer,registro_archivo);

    return buffer;
}

void enviar_buffer_fs_truncate(char* nombre_interfaz_a_truncar,char* nombre_file_truncate,uint32_t registro_truncador){
    t_buffer* buffer = llenar_buffer_fs_truncate(nombre_interfaz_a_truncar,nombre_file_truncate,registro_truncador);
    enviar_paquete(buffer,FS_TRUNCATE_KERNEL,client_dispatch);
}

t_buffer* llenar_buffer_fs_truncate(char* nombre_interfaz_a_truncar,char* nombre_file_truncate,uint32_t registro_truncador){
    int largo_nombre_interfaz = nombre_interfaz_a_truncar + 1;
    int largo_nombre_archivo  = nombre_file_truncate + 1; 
    
    int size = sizeof(uint32_t) + largo_nombre_archivo + largo_nombre_interfaz + sizeof(int) * 2;

    t_buffer* buffer = buffer_create(size);

    buffer_add_int(buffer,largo_nombre_interfaz);
    buffer_add_string(buffer,nombre_interfaz_a_truncar,largo_nombre_interfaz);
    buffer_add_int(buffer,largo_nombre_archivo);
    buffer_add_string(buffer,nombre_file_truncate,largo_nombre_archivo);
    buffer_add_uint32(buffer,registro_truncador);

    return buffer;
}
/*
typedef struct {
    char* nombre;
    int length;
} t_parametro;
*/ 

void liberar_parametro(t_parametro* parametro){
    free(parametro->nombre);
    free(parametro);
}


