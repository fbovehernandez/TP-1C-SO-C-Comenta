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
        printf("Esta ejecutando %d\n", pcb->pid);   
        int resultado_ok = ejecutar_instruccion(instruccion, pcb);
        
        // free(instruccion); // double free
        
        if(resultado_ok == 1) {
            hay_interrupcion = 0;
            return;
        }

        pcb->program_counter++; // Ojo con esto! porque esta despues del return, osea q en el IO_GEN_SLEEP antes de desalojar hay que aumentar el pc
        // Podria ser una funcion directa -> Por ahora no es esencial

        if(hay_interrupcion == 1) {
            printf("Hubo una interrupcion.\n");
            desalojar(pcb, INTERRUPCION_QUANTUM, NULL);
            hay_interrupcion = 0;
            return;
        }
        
        /*
        recv(socket_memoria, &esperar_confirm, sizeof(int), MSG_WAITALL);
        
        printf("Esperar confirmacion: %d\n", esperar_confirm);

        if(esperar_confirm != 1) {
            return;
        }*/
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

    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = QUIERO_CANTIDAD_INSTRUCCIONES; // Podemos usar una constante por operación
    paquete->buffer = buffer;                                  // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void *a_enviar = malloc(buffer->size + sizeof(int) * 2);
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));

    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // printf("Pedi la cantidad de instrucciones del proceso %d.\n", pid);
    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) * 2, 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
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

    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = QUIERO_INSTRUCCION; // Podemos usar una constante por operación
    paquete->buffer = buffer;                       // Nuestro buffer de antes.

    void *a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); // siceof
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    // printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
    // free(pid_pc); -> ver si va aca
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

    free(paquete->buffer->stream);
    free(paquete->buffer);  
    free(paquete);

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
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);

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
    t_parametro* registro_direccion;
    void* registro1;
    int direccion_fisica;
    char* nombre_registro_dato;
    char* nombre_registro_dir;
    int tamanio_en_byte;
    int pagina;
    int esperar_confirm;

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
        uint32_t valor_en_mem;
         
        t_parametro *registro_datos = list_get(list_parametros, 0);
        t_parametro *registro_direccion = list_get(list_parametros, 1);
        nombre_registro_dato = registro_datos->nombre;
        nombre_registro_dir = registro_direccion->nombre; 

        void *registro_dato_mov_in = seleccionar_registro_cpu(nombre_registro_dato);

        es_registro_uint8_dato = es_de_8_bits(nombre_registro_dato);
        uint32_t* registro_direccion_1 = (uint32_t*) seleccionar_registro_cpu(nombre_registro_dir);
        // bool es_registro_uint8_direccion = es_de_8_bits(nombre_registro_dir); // not used

        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato);

        realizar_operacion(registro_direccion_1, tamanio_en_byte, pcb->pid, RECIBIR_DIRECCIONES);
        
        recv(socket_memoria, &valor_en_mem, sizeof(uint32_t), MSG_WAITALL);
        printf("Recibi el valor de memoria \n");

        printf("El valor de memoria es: %d\n", valor_en_mem);
        set(registro_dato_mov_in, valor_en_mem, es_registro_uint8_dato);
        break;
    case MOV_OUT: // MOV_OUT (Registro Dirección, Registro Datos)
        int respuesta_ok_mv;

        recibir_parametros_mov_out(list_parametros, &registro_direccion, &registro_datos, &nombre_registro_dato, &nombre_registro_dir);

        uint32_t* registro_dato_mov_out = (uint32_t*)seleccionar_registro_cpu(nombre_registro_dato);
        es_registro_uint8_dato = es_de_8_bits(nombre_registro_dato);
        
        uint32_t* registro_direccion_mov_out = (uint32_t*)seleccionar_registro_cpu(nombre_registro_dir); // este es la direccion logica

        tamanio_en_byte = tamanio_byte_registro(es_registro_uint8_dato);

        realizar_operacion(registro_direccion_mov_out, tamanio_en_byte, pcb->pid, ESCRIBIR_DATO_EN_MEM);

        recv(socket_memoria, &esperar_confirm, sizeof(int), MSG_WAITALL);
        
        printf("Esperar confirmacion: %d\n", esperar_confirm);
        break;
    case COPY_STRING: // COPY_STRING 1212
        t_parametro* tamanio_a_copiar_de_lista = list_get(list_parametros, 0);
        int tamanio_a_copiar = atoi(tamanio_a_copiar_de_lista->nombre);
        printf("Le hago atoi al tamanio a copiar\n");

        uint32_t* registro_SI = (uint32_t*)seleccionar_registro_cpu(pcb->registros->SI);
        uint32_t* registro_DI = (uint32_t*)seleccionar_registro_cpu(pcb->registros->DI);
        uint32_t* registro_auxiliar;

        realizar_operacion(registro_SI, tamanio_a_copiar, pcb->pid, RECIBIR_DIRECCIONES);
        recv(socket_memoria, &valor_en_mem, sizeof(uint32_t), MSG_WAITALL);
        printf("Recibi el valor de memoria \n");
        printf("El valor de memoria es: %d\n", valor_en_mem);
        set(registro_auxiliar, valor_en_mem, es_registro_uint8_dato);

        realizar_operacion(registro_DI, tamanio_a_copiar, pcb->pid, ESCRIBIR_DATO_EN_MEM);

        recv(socket_memoria, &esperar_confirm, sizeof(int), MSG_WAITALL);
        
        printf("Esperar confirmacion: %d\n", esperar_confirm);

        /*t_instruccion* instruccionMOVIN;
        instruccionMOVIN->nombre = MOV_IN;
        list_add(instruccionMOVIN->parametros, registro_auxiliar);
        list_add(instruccionMOVIN->parametros, registro_SI);
        ejecutar_instruccion(instruccionMOVIN, pcb);
        printf("valor del registro_auxiliar:%d", registro_auxiliar);
        void* reg_achicado = achicar_registro(registro_auxiliar, tamanio_a_copiar);
        t_instruccion* instruccionMOVOUT;
        instruccionMOVOUT->nombre = MOV_OUT;
        list_add(instruccionMOVOUT->parametros, registro_DI);
        list_add(instruccionMOVOUT->parametros, reg_achicado);*/


        /*int direccion_fisicaSI = traducir_direccion_logica_a_fisica(pcb->registros->SI, pcb->pid);
        int direccion_fisicaDI = traducir_direccion_logica_a_fisica(pcb->registros->DI, pcb->pid);
        printf("Tengo la direccionn fisica\n");
        enviar_buffer_copy_string(direccion_fisicaSI , direccion_fisicaDI, tamanio_a_copiar);
        */break;
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
        // recv(client_dispatch, &solicitud_unidades_trabajo, sizeof(int), MSG_WAITALL);
        // solicitud_dormirIO_kernel(interfazSeleccionada, unidadesDeTrabajo);

        // free(buffer->stream); -> Entiendo qeu esto ya lo libere en desalojar
        // free(buffer);

        printf("Hizo IO_GEN_SLEEP\n");
        return 1; // Retorna 1 si desalojo PCB...
        break;
    case IO_STDIN_READ:
    /* 
        t_parametro* interfaz = list_get(list_parametros,0);
        registro_direccion = list_get(list_parametros, 1);
        t_parametro *registro_tamanio = list_get(list_parametros, 2);

        char* nombre_registro_direccion = registro_direccion->nombre;
        char* nombre_registro_tamanio = registro_tamanio->nombre;

        uint32_t* registro_direccion_2 = (uint32_t*) seleccionar_registro_cpu(nombre_registro_direccion);
        uint32_t* registro_tamanio_1 = (uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio);

        direccion_fisica = traducir_direccion_logica_a_fisica(tamanio_pagina, *registro_direccion_2, pcb->pid); 

        pedir_lectura(interfaz->nombre, direccion_fisica, &registro_tamanio_1);

        desalojar(pcb, PEDIDO_LECTURA, buffer);
        break;
        */
    case EXIT_INSTRUCCION:
        // guardar_estado(pcb); -> No estoy seguro si esta es necesaria, pero de todas formas nos va a servir cuando se interrumpa por quantum
        desalojar(pcb, FIN_PROCESO, NULL); // Envio 0 ya que no me importa size
        break;
    case IO_STDOUT_WRITE:// IO_STDOUT_WRITE Int3 BX EAX
    /* 
        int devolucion_desalojo;
        t_parametro* parametro_interfaz = list_get(list_parametros,0);
        char* nombre_interfaz = parametro_interfaz->nombre;
        
        t_parametro* parametro_registro_direccion = list_get(list_parametros,1);
        char* nombre_registro_direccion_stdout = parametro_registro_direccion->nombre;
        
        t_parametro* parametro_registro_tamanio = list_get(list_parametros,2);
        char* nombre_registro_tamanio_stdout = parametro_registro_tamanio->nombre;

        uint32_t* registro_direccion11 = (uint32_t*) seleccionar_registro_cpu(nombre_registro_direccion_stdout);
        uint32_t* registro_tamanio1 = (uint32_t*) seleccionar_registro_cpu(nombre_registro_tamanio_stdout);
        
*        direccion_fisica = traducir_direccion_logica_a_fisica(tamanio_pagina, *registro_direccion11, pcb->pid);

        // CPU -> KERNEL -> MEMORIA -> ENTRADA SALIDA -> SEGFAULT
        enviar_kernel_stdout(nombre_interfaz, direccion_fisica, registro_tamanio1);
        recv(client_dispatch, &devolucion_desalojo, sizeof(int), MSG_WAITALL);
        
        if(devolucion_desalojo == 1) {
            printf("Hubo un error en la escritura\n");
            desalojar(pcb, ERROR_STDOUT, NULL);
            return 1;
        }
        
        desalojar(pcb, PEDIDO_ESCRITURA, buffer);
        
        break;
    case IO_FS_CREATE: // IO_FS_CREATE Int4 notas.txt
        
        t_parametro* primer_parametro = list_get(list_parametro,0);
        char* nombre_interfaz = primer_parametro->nombre;

        t_parametro* segundo_parametro = list_get(list_parametro,1);
        char* nombre_archivo = segundo_parametro->nombre; 
        
        enviar_buffer_fs_create(nombre_interaz,nombre_archivo);
        */
        break;
    case IO_FS_DELETE:
        /*
        t_parametro* primer_parametro = list_get(list_parametro,0);
        char* nombre_interfaz = primer_parametro->nombre;

        t_parametro* segundo_parametro = list_get(list_parametro,1);
        char* nombre_archivo = segundo_parametro->nombre; 
        
        enviar_buffer_fs_delete(nombre_interaz,nombre_archivo);
        */
        break;
    case IO_FS_TRUNCATE:
        /*
        t_parametro* primer_parametro = list_get(list_parametro,0);
        char* nombre_interfaz = primer_parametro->nombre;

        t_parametro* segundo_parametro = list_get(list_parametro,1);
        char* nombre_archivo = segundo_parametro->nombre; 
        
        enviar_buffer_fs_create(nombre_interaz,nombre_archivo);
        */
        break;
    case IO_FS_READ:
        break;
    case IO_FS_WRITE:
        break;
    default:
        printf("Error: No existe ese tipo de instruccion\n");
        printf("La instruccion recibida es: %d\n", nombreInstruccion);
        if (pcb->program_counter == 10) {
            imprimir_pcb(pcb); // Solo para ver.
        }
        desalojar(pcb, FIN_PROCESO, NULL);
        // Hasta que encontremos como hacer con el EXIT que recibe como E X I T
        break;
    }
    return 0;
}

void realizar_operacion(uint32_t* registro_direccion_1, int tamanio_en_byte, int pid, codigo_operacion codigo_operacion) {
    int respuesta_ok;

    int pagina = floor(*registro_direccion_1 / tamanio_pagina);
    int cantidad_paginas = cantidad_de_paginas_a_utilizar(*registro_direccion_1, tamanio_en_byte, pagina);

    enviar_primer_pagina(*registro_direccion_1, pid, tamanio_en_byte, cantidad_paginas, 0, codigo_operacion);
    recv(socket_memoria, &respuesta_ok, sizeof(uint32_t), MSG_WAITALL);
    
    enviar_direcciones_fisicas(pagina, pid, cantidad_paginas-1); 
}

void recibir_parametros_mov_out(t_list* list_parametros, t_parametro **registro_direccion, t_parametro **registro_datos, char** nombre_registro_dato, char** nombre_registro_dir) {
    *registro_direccion = list_get(list_parametros, 0);
    *registro_datos = list_get(list_parametros, 1);
    *nombre_registro_dato = (*registro_datos)->nombre;
    *nombre_registro_dir = (*registro_direccion)->nombre;
}

void enviar_primer_pagina(uint32_t direccion_logica, int pid, int tamanio_en_bytes, int cant_paginas, uint32_t valor, codigo_operacion cod_op) {
    int direccion_fisica = traducir_direccion_logica_a_fisica(direccion_logica, pid); 
    printf("direccion fisica que vamos a usar %d \n", direccion_fisica);

    mandar_direccion_fisica_a_mem(direccion_fisica, tamanio_en_bytes, cant_paginas, direccion_logica, valor, cod_op); 
    printf("se mando dir fisica a memoria %d : \n", direccion_fisica);
}  

void mandar_direccion_fisica_a_mem(int direccion_fisica, int tamanio_en_bytes, int cantidad_paginas, int direccion_logica, uint32_t valor, codigo_operacion cod_op) {
t_paquete *paquete = malloc(sizeof(t_paquete));
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 4 + sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(buffer->stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &tamanio_en_bytes, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &direccion_logica, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &valor, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
 
    paquete->codigo_operacion = cod_op; // Podemos usar una constante por operación
    
    paquete->buffer = buffer;

    void *a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); 
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
    
    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    // printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

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

    // Si usamos memoria dinámica para el nombre, y no la precisamos más, ya podemos liberarla:

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = PEDIDO_LECTURA; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(int) * 2);

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(client_dispatch, a_enviar, buffer->size + sizeof(int) * 2, 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(pedido_lectura->interfaz);
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void enviar_direcciones_fisicas(int pagina, int pid, int cant_paginas) {  
    int frame;
    // int termine_envio = 10; 
    
    for(int i = 0; i < cant_paginas; i++) {
        printf("Pido el marco de la pagina %d del proceso %d\n", pagina + 1, pid); // El uno es para que siempre pida la sig
        pedir_frame_a_memoria(pagina + 1, pid); 
 
        recv(socket_memoria, &frame, sizeof(int), MSG_WAITALL);

        int direccion_fisica = frame * tamanio_pagina + 0; // 0 es el offset

        printf("Direccion fisica a enviar y agregar en lista: %d\n", direccion_fisica);
        mandar_una_dir_fisica(direccion_fisica);
        pagina++;
    }

}

void mandar_una_dir_fisica(int direccion_fisica) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(buffer->stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);

    paquete->codigo_operacion = RECIBIR_DIR_FISICA; // Podemos usar una constante por operación
    
    paquete->buffer = buffer;

    void *a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); 
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
    
    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    // printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

int tamanio_byte_registro(bool es_registro_uint8) {
    return es_registro_uint8 ? 1 : 4;
}

int cantidad_de_paginas_a_utilizar(uint32_t direccion_logica, int tamanio_en_bytes, int pagina) { 
    int cantidad_paginas = 1;
    
    int offset = direccion_logica - pagina * tamanio_pagina;
    
    while(1) { // Hay una situacion en que haga mas de 2 iteraciones? -> tam pag 2b registro 4b -> frame 2 frame 3 frame 4
        int posible_lectura = tamanio_pagina - offset + 1; // El +1 es porque tiene que leer de 0 a 32, son 33 bytes
        int sobrante_de_pagina = tamanio_en_bytes - posible_lectura;
        
        if(sobrante_de_pagina <= 0) {
            break;
        }

        cantidad_paginas++; // Aca ya se que me va a llevar mas de una page
        offset = 0;
        
        tamanio_en_bytes = sobrante_de_pagina;
    } 

    return cantidad_paginas;
}


void enviar_tamanio_memoria(int nuevo_tamanio, int pid) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 2; 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &nuevo_tamanio, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pid, sizeof(int));

    paquete->codigo_operacion = RESIZE_MEMORIA; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    void *a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); 
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);

    // Liberamos memoria
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

/* 
void mandar_direccion_fisica_a_mem(int direccion_fisica, int tamanio_en_bytes, int cantidad_paginas, uint32_t direccion_logica) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 3 + sizeof(uint32_t);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(buffer->stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &tamanio_en_bytes, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &cantidad_paginas, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &direccion_logica, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
 
    paquete->codigo_operacion = RECIBIR_DIRECCIONES; // Podemos usar una constante por operación
    
    paquete->buffer = buffer;

    void *a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); 
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
    
    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    // printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}
*/

void desalojar(t_pcb* pcb, DesalojoCpu motivo, t_buffer* datos_adicionales) {
    guardar_estado(pcb);
    setear_registros_cpu(); 
    enviar_pcb(pcb, client_dispatch, motivo, datos_adicionales);
}

void solicitud_dormirIO_kernel(char* interfaz, int unidades) {
    t_buffer* buffer; // Creo que no hace falta reservar memoria
    buffer = llenar_buffer_dormir_IO(interfaz, unidades);
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = DORMIR_INTERFAZ; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar   
    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    // hay que poner el socket kernel globallll
    send(client_dispatch, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    // printf("Paquete enviado!");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

/*
void* sacarParametroParaConseguirRegistro(t_list* list_parametros, bool esDe8bits, int nroParametro){
    t_parametro *parametro_registro_direccion = list_get(list_parametros, nroParametro);
    char* nombre_registro1 = parametro_registro_direccion->nombre;
    esDe8bits = es_de_8_bits(nombre_registro1);
    return seleccionar_registro_cpu(nombre_registro1);
}
*/


/*
void solicitud_lecturaIO_kernel(char* interfaz,void* valorRegistroDestino,void* valorRegistroTamanio){
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer = llenar_buffer_leer_IO(interfaz, valorRegistroDestino,valorRegistroTamanio);
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = LEER_INTERFAZ; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    // hay que poner el socket kernel globallll
    send(client_dispatch, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    printf("Paquete enviado!");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}
*/
// ver
/*
typedef struct {
    int nombre_interfaz_largo;
    char* nombre_interfaz;
    int unidadesDeTrabajo;
} t_operacion_io; 
*/


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

/*
t_buffer* llenar_buffer_leer_IO(interfaz, valorRegistroDestino,valorRegistroTamanio){
    return NULL; // TODO
}
*/

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


// Lo mismo que sum pero negativo
/*void sub(void* registroOrigen, void* registroDestino, bool es_8_bits, t_pcb* pcb) {
   if (es_8_bits) {
        *(uint8_t *)registroOrigen -= *(uint8_t *)registroDestino; // NO va a haber ninguno registro de 8 bits que pase el limite
    } else {
        *(uint32_t *)registroOrigen -= *(uint32_t *)registroDestino;
    }
}*/

void jnz(void* registro, int valor, t_pcb* pcb) {
    if (*(int*) registro != 0) {
        pcb->program_counter = valor;
    }
}

/*
bool sonTodosDigitosDe(char *palabra) {
    while (*palabra)
    {
        if (!isdigit(*palabra))
        {
            return false;
        }
        palabra++;
    }
    return true;
}
*/

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
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = COPY_STRING_MEMORIA; 
    paquete->buffer = buffer; 
    printf("llego a enviar buffer copy");

    // Armamos el stream a enviar   
    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
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
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = PEDIDO_ESCRITURA; 
    paquete->buffer = buffer; 
    printf("llego a enviar buffer stdout\n");
    int largo = string_length(nombre_interfaz);

    // Armamos el stream a enviar   
    void* a_enviar = malloc(buffer->size + sizeof(int) * 2 + sizeof(largo));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(client_dispatch, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
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
/*
void enviar_buffer_fs_create(char* nombre_interaz,char* nombre_archivo){
    t_buffer* buffer;
    buffer = llenar_buffer_fs_create(nombre_interfaz,nombre_archivo);
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = FS_CREATE; 
    paquete->buffer = buffer; 
    printf("llego a enviar buffer stdout\n");
    int largo = string_length(nombre_interfaz);

    // Armamos el stream a enviar   
    void* a_enviar = malloc(buffer->size + sizeof(int) * 2 + sizeof(largo));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(soccket_kernel, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}


t_buffer* llenar_buffer_fs_create(char* nombre_interfaz,char* nombre_archivo){
    t_buffer *buffer = malloc(sizeof(t_buffer));
    int largo_nombre_interfaz = string_length(nombre_interfaz);
    int largo_nombre_archivo = string_length(nombre_archivo);

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

*/  