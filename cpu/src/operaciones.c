#include "../include/operaciones.h"

t_log *logger;
int hay_interrupcion;
// t_cantidad_instrucciones* cantidad_instrucciones;

void ejecutar_pcb(t_pcb *pcb, int socket_memoria) {
    //int esperar_confirm;

    pedir_cantidad_instrucciones_a_memoria(pcb->pid, socket_memoria);
    int cantidad_instrucciones = recibir_cantidad_instrucciones(socket_memoria, pcb->pid);

    while(pcb->program_counter < cantidad_instrucciones) {
        // sem_wait(pedir_instruccion);
        pedir_instruccion_a_memoria(socket_memoria, pcb);
        t_instruccion* instruccion = recibir_instruccion(socket_memoria, pcb); // Se ejecuta la instruccion tambien
        // sem_post(pedir_instruccion);

        if(instruccion->nombre == ERROR_INSTRUCCION) return;

        printf("Esta ejecutando %d\n", pcb->pid);   
        int resultado_ok = ejecutar_instruccion(instruccion, pcb);
        
        // free(instruccion); // double free
        
        pcb->program_counter++;
        
        if(resultado_ok == 1) {
            hay_interrupcion = 0;
            return;
        }

        // pcb->program_counter++; // Ojo con esto! porque esta despues del return, osea q en el IO_GEN_SLEEP antes de desalojar hay que aumentar el pc
        // Podria ser una funcion directa -> Por ahora no es esencial

        if(hay_interrupcion == 1) {
            printf("Hubo una interrupcion.\n");
            desalojar(pcb, INTERRUPCION_QUANTUM, NULL);
            hay_interrupcion = 0;
            return;
        }
    }

}

void pedir_cantidad_instrucciones_a_memoria(int pid, int socket_memoria) {
    t_buffer *buffer = malloc(sizeof(t_buffer));
    // t_cantidad_instrucciones* cantidad = malloc(sizeof(t_cantidad_instrucciones));
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
    t_solicitud_instruccion *pid_pc = malloc(sizeof(t_solicitud_instruccion));
    pid_pc->pid = pcb->pid;
    pid_pc->pc = pcb->program_counter;

    t_buffer *buffer = llenar_buffer_solicitud_instruccion(pid_pc);
    enviar_paquete(buffer, QUIERO_INSTRUCCION, socket_memoria);
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
        printf("Codigo de operacion: %d\n", paquete->codigo_operacion);

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
    t_instruccion *instruccion = malloc(sizeof(t_instruccion));
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

        parametro->nombre = malloc(sizeof(char) * parametro->length + 1);
        memcpy(parametro->nombre, stream, sizeof(char) * parametro->length);

        parametro->nombre[parametro->length] = '\0';
        stream += parametro->length;
        // printf("Parametro: %s\n", parametro->nombre);

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
    typedef struct {
        int pid;
        int pagina;
        int marco;
        long timestamps;
    } t_tlb;

    typedef struct {
        int numero_pagina;
        int desplazamiento;
    } t_direccion_logica;

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

void guardar_en_TLB(int pid, int nro_pagina, int nro_marco) {
	t_tlb* nueva_entrada = malloc(sizeof(t_tlb));
	
    nueva_entrada->pid = pid;
	nueva_entrada->pagina = nro_pagina;
	nueva_entrada->marco = nro_marco;
    nueva_entrada->timestamp = tiempoEnMilisecs();

	int cant_entradas = config_get_int_value(config_CPU, "CANTIDAD_ENTRADAS_TLB");
    char* algoritmo_tlb = config_get_string_value(config_CPU, "ALGORITMO_TLB");

    switch(algoritmo_TLB)
    {
        case FIFO:
            queue_push(TLB, nueva_entrada);
            break;
        case LRU:
            algoritmo_LRU(nueva_entrada);
            break;
    }
}

void algoritmo_LRU(t_tlb* nueva_entrada){
	t_tlb* auxiliar1 = malloc(sizeof(t_tlb));
	t_tlb* auxiliar2 = malloc(sizeof(t_tlb));

	int tamanio = queue_size(TLB);
	auxiliar1 = queue_pop(TLB);

	for(int i = 1; i < tamanio; i++){
		auxiliar2 = queue_pop(TLB);

		if(auxiliar1->timestamp > auxiliar2->timestamp) {
			queue_push(TLB, auxiliar1);
			auxiliar1 = auxiliar2;
		}
        // Si el aux1 es más nuevo en la TLB que el aux2, pushea el auxiliar 1 y le asigna el más viejo, o sea auxiliar 2. 
        // De esta forma, en auxiliar 1 nos queda la entrada que esta hace mas tiempo en TLB para volver a comparar
		else
			queue_push(TLB, auxiliar2);
	}
	
	queue_push(TLB, nueva_entrada);
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

void vaciar_tlb(){
	while(queue_size(TLB) > 0){
		t_tlb* una_entrada = queue_pop(TLB);
		free(una_entrada);
	}
}

*/

int ejecutar_instruccion(t_instruccion *instruccion, t_pcb *pcb) {
    // log_info(logger, "PID: %d - Ejecutando: %d ", pcb->pid, instruccion->nombre);
    TipoInstruccion nombreInstruccion = instruccion->nombre;
    t_list *list_parametros = instruccion->parametros;
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
        void *registro_pcb1 = seleccionar_registro_cpu(registro1);

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

        void *registro_dato_mov_in = seleccionar_registro_cpu(nombre_registro_dato);

        es_registro_uint8_dato = es_de_8_bits(nombre_registro_dato);
        uint32_t* registro_direccion_mov_in = (uint32_t*) seleccionar_registro_cpu(nombre_registro_dir);
        // bool es_registro_uint8_direccion = es_de_8_bits(nombre_registro_dir); // not used

        t_list* lista_bytes_lectura_mov_in = list_create();
        t_list* lista_direcciones_fisicas_mov_in = list_create(); 

        pagina = floor(*registro_direccion_mov_in / tamanio_pagina);
        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato);
        cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion_mov_in, tamanio_en_byte, pagina, lista_bytes_lectura_mov_in); // Cantidad de paginas + la primera
        
        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_lectura_mov_in, *registro_direccion_mov_in, pcb->pid, lista_direcciones_fisicas_mov_in, pagina);
        
        enviar_direcciones_fisicas(cantidad_paginas, lista_direcciones_fisicas_mov_in, &default_value, tamanio_en_byte, pcb->pid, LEER_DATO_MEMORIA);

        void* valor_en_mem = malloc(tamanio_en_byte);

        recv(socket_memoria, valor_en_mem, tamanio_en_byte, MSG_WAITALL); // Deberia leer el tamanio 
        
        printf("Recibi el valor de memoria \n");

        uint32_t valor_recibido = *(uint32_t*) valor_en_mem;

        printf("El valor recibido es: %d\n", valor_recibido);

        set(registro_dato_mov_in, valor_recibido, es_registro_uint8_dato);

        free(valor_en_mem);
        break;
    case MOV_OUT: // MOV_OUT (Registro Dirección, Registro Datos)
        recibir_parametros_mov_out(list_parametros, &registro_direccion, &registro_datos, &nombre_registro_dato, &nombre_registro_dir);

        t_list* lista_bytes_lectura = list_create();
        t_list* lista_direcciones_fisicas_mov_out = list_create(); 

        uint32_t* registro_dato_mov_out = (uint32_t*)seleccionar_registro_cpu(nombre_registro_dato);
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_dato);
        
        uint32_t* registro_direccion_mov_out = (uint32_t*)seleccionar_registro_cpu(nombre_registro_dir); // este es la direccion logica

        pagina = floor(*registro_direccion_mov_out / tamanio_pagina);
        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato);

        cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion_mov_out, tamanio_en_byte, pagina, lista_bytes_lectura); // Cantidad de paginas + la primera
        // imprimir lista de bytes
        // printf("Cantidad primera a copiar %d\n", *(int*)list_get(lista_bytes_lectura, 0));

        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_lectura, *registro_direccion_mov_out, pcb->pid, lista_direcciones_fisicas_mov_out, pagina);
        
        enviar_direcciones_fisicas(cantidad_paginas, lista_direcciones_fisicas_mov_out, (void*)registro_dato_mov_out, tamanio_en_byte, pcb->pid, ESCRIBIR_DATO_EN_MEM);
        // realizar_operacion(registro_direccion_mov_out, tamanio_en_byte, (void*)registro_dato_mov_out , 0, pcb->pid, ESCRIBIR_DATO_EN_MEM);

        recv(socket_memoria, &esperar_confirm, sizeof(int), MSG_WAITALL);
        
        printf("Esperar confirmacion MOV OUT: %d\n", esperar_confirm);
        break;
    case COPY_STRING:
        // int esperar_confirm_2;

        t_list* lista_bytes_lectura_cs = list_create();
        t_list* lista_direcciones_fisicas_cs = list_create(); 

        t_parametro* tamanio_a_copiar_de_lista = list_get(list_parametros, 0);
        int tamanio_a_copiar = atoi(tamanio_a_copiar_de_lista->nombre);

        uint32_t* registro_SI = (uint32_t*)seleccionar_registro_cpu("SI");
        uint32_t* registro_DI = (uint32_t*)seleccionar_registro_cpu("DI");

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
        
        break;
    case SUM:
        t_parametro *registro_param1 = list_get(list_parametros, 0);
        registro1 = registro_param1->nombre;

        t_parametro* registro_param2 = list_get(list_parametros, 1); // valor_void = 2do registro
        char *registro2 = registro_param2->nombre;

        void* registro_origen = seleccionar_registro_cpu(registro1);
        bool es_registro_uint8_origen = es_de_8_bits(registro1);
        void* registro_destino = seleccionar_registro_cpu(registro2);
        bool es_registro_uint8_destino = es_de_8_bits(registro2);

        sum(registro_origen, registro_destino, es_registro_uint8_origen, es_registro_uint8_destino);
        break;
    case SUB:
        t_parametro *registro_origen_param_resta = list_get(list_parametros, 0);
        char *registro_origen_resta = registro_origen_param_resta->nombre;

        t_parametro *registro_destino_param_resta = list_get(list_parametros, 1);
        char *registro_destino_resta = registro_destino_param_resta->nombre;

        void *registro_origen_pcb_resta = seleccionar_registro_cpu(registro_origen_resta);
        void *registro_destino_pcb_resta = seleccionar_registro_cpu(registro_destino_resta);

        bool es_8_bits_origen_resta = es_de_8_bits(registro_origen_resta);
        bool es_8_bits_destino_resta = es_de_8_bits(registro_destino_resta);

        sub(registro_origen_pcb_resta, registro_destino_pcb_resta, es_8_bits_origen_resta, es_8_bits_destino_resta);
        break;
    case JNZ:
        t_parametro *registro_parametro = list_get(list_parametros, 0);
        registro1 = registro_parametro->nombre;
        
        t_parametro *pc = list_get(list_parametros, 1);
        int nuevo_pc = atoi(pc->nombre);

        void *registro = seleccionar_registro_cpu(registro1);
        jnz(registro, nuevo_pc, pcb);
        break;
    case RESIZE:
        int devolucion_resize;
        t_parametro *registro_parametro_tamanio = list_get(list_parametros, 0);
        int nuevo_tamanio = atoi(registro_parametro_tamanio->nombre);

        enviar_tamanio_memoria(nuevo_tamanio, pcb->pid);
        recv(socket_memoria, &devolucion_resize, sizeof(int), MSG_WAITALL);

        if(devolucion_resize == 1) {
            printf("Che pibe, te quedaste sin memoria\n");
            // desalojar(pcb, OUT_OF_MEMORY, NULL);
            return 1;
        }

        printf("Se redimensiono la memoria\n"); 
        break;
    case WAIT:
        //Manda a kernel a asignar una instancia del recurso pasado por parametro
        t_parametro* parametro_wait = list_get(list_parametros, 0);
        manejar_recursos(pcb, parametro_wait, WAIT_RECURSO);
        break;
    case SIGNAL:
        //Manda a kernel a liberar una instancia del recurso pasado por parametro
        manejar_recursos(pcb, list_get(list_parametros, 0), SIGNAL_RECURSO);
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
        
        pcb->program_counter++;
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

        char* nombre_registro_direccion = registro_direccion->nombre;
        char* nombre_registro_tamanio = registro_tamanio->nombre;

        uint32_t* registro_direccion_stdin = (uint32_t*) seleccionar_registro_cpu(nombre_registro_direccion);
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_tamanio);

        uint32_t* registro_tamanio_stdin = (uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio);
        
        int pagina = floor(*registro_direccion_stdin / tamanio_pagina);
        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato); //Ojo que abajo no le paso el tam_byte, sino la cantidad que tiene dentro

        int cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion_stdin, tamanio_en_byte, pagina, lista_bytes_stdin); // Cantidad de paginas + la primera
        
        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_stdin, *registro_direccion_stdin, pcb->pid, lista_direcciones_fisicas_stdin, pagina);

        t_buffer* buffer_lectura = llenar_buffer_stdio(interfaz->nombre, lista_direcciones_fisicas_stdin, *registro_tamanio_stdin, cantidad_paginas);
        desalojar(pcb, PEDIDO_LECTURA, buffer_lectura);

        free(buffer_lectura);
        // return 1; // El return esta para que cuando se desaloje el pcb no siga ejecutando, si hace el break sigue pidiendo instrucciones, porfa no lo saquen o les va a romper
        break;
    case EXIT_INSTRUCCION:
        // guardar_estado(pcb); -> No estoy seguro si esta es necesaria, pero de todas formas nos va a servir cuando se interrumpa por quantum
        printf("Llego a instruccion EXIT\n");
        desalojar(pcb, FIN_PROCESO, NULL); // Envio 0 ya que no me importa size
        break;
    case IO_STDOUT_WRITE:// IO_STDOUT_WRITE Int3 BX EAX
        t_list* lista_bytes_stdout = list_create();
        t_list* lista_direcciones_fisicas_stdout = list_create();
        t_parametro* parametro_interfaz = list_get(list_parametros,0);
        char* nombre_interfaz = parametro_interfaz->nombre;
        
        t_parametro* parametro_registro_direccion = list_get(list_parametros,1);
        char* nombre_registro_direccion_stdout = parametro_registro_direccion->nombre;
        
        t_parametro* parametro_registro_tamanio = list_get(list_parametros,2);
        char* nombre_registro_tamanio_stdout = parametro_registro_tamanio->nombre;

        uint32_t* registro_direccion11 = (uint32_t*) seleccionar_registro_cpu(nombre_registro_direccion_stdout);
        uint32_t* registro_tamanio_stdout = (uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio_stdout);
        
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_tamanio_stdout);
        
        direccion_fisica = traducir_direccion_logica_a_fisica(*registro_direccion11, pcb->pid);

        // CPU -> KERNEL -> MEMORIA -> ENTRADA SALIDA
    
        pagina = floor(*registro_direccion11 / tamanio_pagina);
        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato); //Ojo que abajo no le paso el tam_byte, sino la cantidad que tiene dentro

        cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion11, tamanio_en_byte, pagina, lista_bytes_stdout); // Cantidad de paginas + la primera
        
        cargar_direcciones_tamanio(cantidad_paginas, lista_bytes_stdout, *registro_direccion11, pcb->pid, lista_direcciones_fisicas_stdout, pagina);

        t_buffer* buffer_escritura = llenar_buffer_stdio(nombre_interfaz, lista_direcciones_fisicas_stdout, *registro_tamanio_stdout, cantidad_paginas);
        desalojar(pcb, PEDIDO_ESCRITURA, buffer_escritura);

        free(buffer_escritura);
        break;
    
    case IO_FS_CREATE: // IO_FS_CREATE nombre_interfaz nombre_arch
    case IO_FS_DELETE: // IO_FS_DELETE nombre_interfaz nombre_arch
  
        t_parametro* primer_parametro1 = list_get(list_parametros, 0);
        char* nombre_interfaz1 = primer_parametro1->nombre;

        t_parametro* segundo_parametro1 = list_get(list_parametros, 1);
        char* nombre_archivo = segundo_parametro1->nombre; 

        codigo_operacion codigo = nombreInstruccion == IO_FS_CREATE ? FS_CREATE : FS_DELETE
        enviar_buffer_fs_create_delete(nombre_interfaz1, nombre_archivo,codigo);
  
        break;
    case IO_FS_READ:
    case IO_FS_WRITE:
        t_parametro* primer_parametro2 = list_get(list_parametros, 0);
        char* nombre_interfaz2 = primer_parametro2->nombre;

        t_parametro* segundo_parametro2 = list_get(list_parametros, 1);
        char* nombre_archivo = segundo_parametro2->nombre;
        
        t_parametro* tercer_parametro = list_get(list_parametros, 2);
        char* nombre_registro_direccion = tercer_parametro->nombre; 
        
        t_parametro* cuarto_parametro = list_get(list_parametros, 3);
        char* nombre_registro_tamanio = cuarto_parametro->nombre; 
        
        codigo_operacion codigo = nombreInstruccion == IO_FS_READ ? LECTURA_FS : ESCRITURA_FS;

        enviar_buffer_fs_escritura_lectura(); 
        
        break;
    
    default:
        printf("Error: No existe ese tipo de instruccion\n");
        printf("La instruccion recibida es: %d\n", nombreInstruccion);
        desalojar(pcb, FIN_PROCESO, NULL);
        // Hasta que encontremos como hacer con el EXIT que recibe como E X I T
        break;
    }
    return 0;
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
    int frame;

    // Aca cargo la primera
    t_dir_fisica_tamanio *dir_fisica_tamanio = malloc(sizeof(t_dir_fisica_tamanio));
    printf("Direccion logica: %d\n", direccion_logica);

    dir_fisica_tamanio->direccion_fisica = traducir_direccion_logica_a_fisica(direccion_logica, pid);
    dir_fisica_tamanio->bytes_lectura = *(int*)list_get(lista_bytes_lectura, 0); 
    printf("tamanio guardado: %d\n", dir_fisica_tamanio->bytes_lectura);
            
    list_add(direcciones_fisicas, dir_fisica_tamanio);

    // Aca cargo el resto
    for(int i = 0; i < cantidad_paginas-1; i++) {
        printf("Iteracion %d\n", i);
        printf("Pido el marco de la pagina %d del proceso %d\n", pagina+1, pid); // El uno es para que siempre pida la sig
        pedir_frame_a_memoria(pagina+1, pid); 
 
        recv(socket_memoria, &frame, sizeof(int), MSG_WAITALL);
        printf("el frame es:%d\n", frame);

        int direccion_fisica = frame * tamanio_pagina + 0; // 0 es el offset

        t_dir_fisica_tamanio *dir_fisica_tamanio = malloc(sizeof(t_dir_fisica_tamanio));
        dir_fisica_tamanio->direccion_fisica = direccion_fisica;
        dir_fisica_tamanio->bytes_lectura = *(int*)list_get(lista_bytes_lectura, i+1); 

        list_add(direcciones_fisicas, dir_fisica_tamanio);

        pagina++;
    }
}

void enviar_direcciones_fisicas(int cantidad_paginas, t_list* direcciones_fisicas, void* registro_dato_mov_out, int tamanio_valor, int pid, codigo_operacion cod_op) {
    t_buffer* buffer = serializar_direcciones_fisicas(cantidad_paginas, direcciones_fisicas, registro_dato_mov_out, tamanio_valor, pid);
    enviar_paquete(buffer, cod_op, socket_memoria);
}

t_buffer* serializar_direcciones_fisicas(int cantidad_paginas, t_list* direcciones_fisicas, void* registro_dato_mov_out, int tamanio_valor, int pid) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    int cant_elementos_lista = list_size(direcciones_fisicas);

    buffer->size = sizeof(int) * 4 + (cant_elementos_lista * sizeof(t_dir_fisica_tamanio)) + tamanio_valor;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &pid, sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(stream + buffer->offset, &cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);

    for(int i=0; i < cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(direcciones_fisicas, i);
        memcpy(stream + buffer->offset, &dir_fisica_tam->direccion_fisica, sizeof(int));
        buffer->offset += sizeof(int);
        memcpy(stream + buffer->offset, &dir_fisica_tam->bytes_lectura, sizeof(int));
        buffer->offset += sizeof(int);
    }

    memcpy(stream + buffer->offset, &tamanio_valor, sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(stream + buffer->offset, registro_dato_mov_out, tamanio_valor);
    
    buffer->stream = stream;

    return buffer;
}

/* 
void realizar_operacion(uint32_t* registro_direccion_1, int tamanio_en_byte, void* valor_a_escribir, uint32_t length_valor, int pid, codigo_operacion codigo_operacion) {
    int respuesta_ok;

    int pagina = floor(*registro_direccion_1 / tamanio_pagina);
    int cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion_1, tamanio_en_byte, pagina);
    enviar_primer_pagina(*registro_direccion_1, pid, tamanio_en_byte, cantidad_paginas, valor_a_escribir, length_valor, codigo_operacion);
    recv(socket_memoria, &respuesta_ok, sizeof(uint32_t), MSG_WAITALL);
    printf("la respuesta ok es:%d\n", respuesta_ok);
    
    printf("Ya recibi la primera, ahora va el resto y son %d\n", cantidad_paginas);
    enviar_direcciones_fisicas_2(pagina, pid, cantidad_paginas-1); 
}
*/

void recibir_parametros_mov_out(t_list* list_parametros, t_parametro **registro_direccion, t_parametro **registro_datos, char** nombre_registro_dato, char** nombre_registro_dir) {
    *registro_direccion = list_get(list_parametros, 0);
    *registro_datos = list_get(list_parametros, 1);
    *nombre_registro_dato = (*registro_datos)->nombre;
    *nombre_registro_dir = (*registro_direccion)->nombre;
}

/* 
void enviar_primer_pagina(uint32_t direccion_logica, int pid, int tamanio_en_bytes, int cant_paginas, void* valor_a_escribir, uint32_t length_valor, codigo_operacion cod_op) {
    int direccion_fisica = traducir_direccion_logica_a_fisica(direccion_logica, pid); 

    mandar_direccion_fisica_a_mem(direccion_fisica, tamanio_en_bytes, cant_paginas, direccion_logica, valor_a_escribir, length_valor, cod_op); 
    printf("se mando la direccion fisica a memoria %d : \n", direccion_fisica);
}  
*/

/* 
t_buffer* serializar_escritura(int direccion_fisica, int tamanio_en_bytes, int cantidad_paginas, int direccion_logica, void* valor, uint32_t length_valor) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    if(length_valor > 0) {
        buffer->size = sizeof(int) * 4 + sizeof(uint32_t) + length_valor;
    } else {
        buffer->size = sizeof(int) * 4 + sizeof(uint32_t) + tamanio_en_bytes;
    }

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &tamanio_en_bytes, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &direccion_logica, sizeof(int));
    buffer->offset += sizeof(int);

    printf("String a enviar atnes de serializar: %s\n", (char*)valor);
    printf("Largo de la cadena a enviar: %d\n", length_valor);

    memcpy(stream + buffer->offset, &length_valor, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    if(length_valor > 0) {
        memcpy(stream + buffer->offset, valor, length_valor);
    } else {
        memcpy(stream + buffer->offset, valor, tamanio_en_bytes);
    } 

    return buffer;
}
*/

/*
t_buffer* serializar_lectura(int direccion_fisica, int tamanio_en_bytes, int cantidad_paginas, int direccion_logica) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 4 + sizeof(codigo_operacion);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &tamanio_en_bytes, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &direccion_logica, sizeof(int));
    buffer->offset += sizeof(int);

    return buffer;
}
 */

/* 
void mandar_direccion_fisica_a_mem(int direccion_fisica, int tamanio_en_bytes, int cantidad_paginas, int direccion_logica, void* valor, uint32_t length_valor, codigo_operacion cod_op) {
    t_buffer* buffer;
    if(cod_op == ESCRIBIR_DATO_EN_MEM) {
        buffer = serializar_escritura(direccion_fisica, tamanio_en_bytes, cantidad_paginas, direccion_logica, valor, length_valor);
    } else {
        buffer = serializar_lectura(direccion_fisica, tamanio_en_bytes, cantidad_paginas, direccion_logica);
    }
    enviar_paquete(buffer, cod_op, socket_memoria);
}
*/

/* 
void pedir_lectura(char* interfaz, int direccion_fisica, uint32_t* registro_tamanio) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    int largo_interfaz = string_length(interfaz) + 1;
    t_pedido_lectura* pedido_lectura = malloc(sizeof(t_pedido_lectura));

    buffer->size = sizeof(int) + sizeof(uint32_t) + largo_interfaz;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
    int offset = 0;

    void* stream = buffer->stream;

    memcpy(stream + offset, &pedido_lectura->registro_direccion, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + offset, &pedido_lectura->registro_tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream + offset, &largo_interfaz, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + offset, pedido_lectura->interfaz, largo_interfaz);
    // No tiene sentido seguir calculando el desplazamiento, ya ocupamos el buffer completo

    buffer->stream = stream;
    enviar_paquete(buffer,PEDIDO_LECTURA, client_dispatch);
    free(pedido_lectura);
}
*/

/* 
void enviar_direcciones_fisicas_2(int pagina, int pid, int cant_paginas) {  
    int frame;
    // int termine_envio = 10; 
    
    for(int i = 0; i < cant_paginas; i++) {
        printf("Iteracion %d\n", i);
        printf("Pido el marco de la pagina %d del proceso %d\n", pagina + 1, pid); // El uno es para que siempre pida la sig
        pedir_frame_a_memoria(pagina + 1, pid); 
 
        recv(socket_memoria, &frame, sizeof(int), MSG_WAITALL);
        printf("el frame es:%d\n", frame);

        int direccion_fisica = frame * tamanio_pagina + 0; // 0 es el offset

        printf("Direccion fisica a enviar y agregar en lista: %d\n", direccion_fisica);
        mandar_una_dir_fisica(direccion_fisica);
        pagina++;
    }

    printf("Termine de enviar las direcciones fisicas\n");
}
*/

/* 
void mandar_una_dir_fisica(int direccion_fisica) {
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(buffer->stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);

    enviar_paquete(buffer, RECIBIR_DIR_FISICA, socket_memoria);
}
*/

int tamanio_byte_registro(bool es_registro_uint8) {
    return es_registro_uint8 ? 1 : 4;
}

/***** ACLARACION *******/
int cantidad_de_paginas_a_utilizar(uint32_t direccion_logica, int tamanio_en_bytes, int pagina, t_list* lista_bytes_lectura) { // Dir logica 5 tamanio 17
    int cantidad_paginas = 1; // (5)
    
    int offset_movido = direccion_logica - pagina * tamanio_pagina; // 5 dir logica -> 16 tam pagina EAX = 4 BYTES
    
    while(1) { 
        int posible_lectura = tamanio_pagina - offset_movido;  // 16 -5  = 11;

        int sobrante_de_pagina = tamanio_en_bytes - posible_lectura; // 4 -11 = -7
        
        if(sobrante_de_pagina <= 0) {
            int* tam_bytes = malloc(sizeof(int));
            *tam_bytes = tamanio_en_bytes; 
            printf("Tamanio LECTURA: %d\n", *tam_bytes); // 17 -> 14 -> 10 -> 6 -> 2
            list_add(lista_bytes_lectura, tam_bytes);
            break;
        }
        
        // Si sobra espacio en la pagina, se agrega a la lista de bytes a leer
        int* tam_posible_lectura = malloc(sizeof(int));
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

void *seleccionar_registro_cpu(char *nombreRegistro) {

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

void sub(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino) {
    uint32_t valor_origen;
    uint32_t valor_destino;

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

    // Realizar la resta
    uint32_t resultado = valor_destino - valor_origen;

    // Actualizar el registro destino con el resultado usando la función set
    set(registroDestino, resultado, es_8_bits_destino);
}

void sum(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino) {
    uint32_t valor_origen;
    uint32_t valor_destino;

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

    // Actualizar el registro destino con el resultado usando la función set
    set(registroDestino, resultado_suma, es_8_bits_destino);
}

void jnz(void* registro, int valor, t_pcb* pcb) {
    if (*(int*) registro != 0) {
        pcb->program_counter = valor;
    }
}

void manejar_recursos(t_pcb* pcb, t_parametro* recurso, DesalojoCpu codigo) {
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) + recurso->length + 1 ;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // void *stream = buffer->stream;

    memcpy(buffer->stream + buffer->offset, &recurso->length, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, recurso->nombre, recurso->length);
    
    printf("el nombre del rec es %s\n", recurso->nombre);
    printf("el length del rec es %d\n", recurso->length);

    // buffer->stream = stream;    
    desalojar(pcb, codigo, buffer);
}

void enviar_buffer_copy_string(int direccion_fisica_SI, int direccion_fisica_DI, int tamanio) {
    t_buffer* buffer;
    buffer = llenar_buffer_copy_string(direccion_fisica_SI,direccion_fisica_DI,tamanio);
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
    t_buffer* buffer;
    buffer = llenar_buffer_stdout(direccion_fisica, nombre_interfaz, tamanio);
    enviar_paquete(buffer, PEDIDO_ESCRITURA, client_dispatch);
}

t_buffer* llenar_buffer_stdout(int direccion_fisica, char* nombre_interfaz, uint32_t tamanio){
    t_buffer *buffer = malloc(sizeof(t_buffer));
    int largo_nombre = string_length(nombre_interfaz);

    buffer->size = sizeof(int) + sizeof(largo_nombre) + sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

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

void enviar_buffer_fs_create_delete(char* nombre_interfaz,char* nombre_archivo,codigo_operacion codigo){
    t_buffer* buffer;
    buffer = llenar_buffer_fs_create_delete(nombre_interfaz,nombre_archivo);
    enviar_paquete(buffer,codigo,client_dispatch);
}


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

    buffer->size = sizeof(int) * 2 + sizeof(uint32_t) + (size_direcciones_fisicas * sizeof(int) * 2) + largo_interfaz;

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
    }

    memcpy(buffer->stream + buffer->offset, &tamanio_a_copiar, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &largo_interfaz, sizeof(int));
    buffer->offset += sizeof(int);
    
    memcpy(buffer->stream + buffer->offset, interfaz, largo_interfaz);

    // buffer->stream = stream;

    return buffer;
}

/* 
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
    printf("nombre interfaz lalala %s",  io->nombre_interfaz);
    // buffer->offset += io->nombre_interfaz_largo;
    
    free(io->nombre_interfaz);
    free(io);
    return buffer;
}

*/
