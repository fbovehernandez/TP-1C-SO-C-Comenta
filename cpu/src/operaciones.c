#include "../include/operaciones.h"

t_log* logger;
t_cantidad_instrucciones* cantidad_instrucciones;

void ejecutar_pcb(t_pcb* pcb, int socket_memoria) {
    pedir_cantidad_instrucciones_a_memoria(socket_memoria);
    recibir(socket_memoria, pcb); // recibir cantidad de instrucciones

    while(pcb->estadoActual < cantidad_instrucciones) { 
        pedir_instruccion_a_memoria(socket_memoria, pcb);
        //sem_wait(&sem_memori, pcbtr 
        recibir(socket_memoria,pcb); // recibir cada instruccion
        pcb->program_counter++;
    }
}

void pedir_cantidad_instrucciones_a_memoria(int socket_memoria) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_cantidad_instrucciones* cantidad = malloc(sizeof(t_cantidad_instrucciones));

    buffer->size = sizeof(int);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;
   
    buffer->stream = stream;

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = CANTIDAD_INSTRUCCIONES; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));

    offset += sizeof(uint8_t);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int), 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}       

t_cantidad_instrucciones* cantidad_instrucciones_serializar(t_buffer* buffer) {
    t_cantidad_instrucciones* cantidad_instrucciones = malloc(sizeof(t_cantidad_instrucciones));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(cantidad_instrucciones->cantidad), stream, sizeof(int));
    stream += sizeof(int);

    return cantidad_instrucciones;
}

// Manda a memoria el pcb, espera una instruccion y la devuelve
void pedir_instruccion_a_memoria(int socket_memoria, t_pcb* pcb) {
    // Recibimos cada parámetro
    t_solicitud_instruccion* pid_pc = malloc(sizeof(t_solicitud_instruccion));
    pid_pc->pid = pcb->pid;
    pid_pc->pc = pcb->program_counter;
    
    t_buffer* buffer = llenar_buffer_solicitud_instruccion(pid_pc);

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = QUIERO_INSTRUCCION; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;
    
    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); //siceof
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

t_buffer* llenar_buffer_solicitud_instruccion(t_solicitud_instruccion* solicitud_instruccion) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    
    buffer->size = sizeof(int) * 2 ;                    
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream+buffer->offset, &solicitud_instruccion->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &solicitud_instruccion->pc, sizeof(int));
   
    buffer->stream = stream;

    return buffer;
}

void recibir(int socket_memoria,t_pcb* pcb) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    // Primero recibimos el codigo de operacion
    printf("Esperando recibir instruccion...\n");

    recv(socket_memoria, &(paquete->codigo_operacion), sizeof(codigo_operacion), MSG_WAITALL);
    printf("codigo de operacion: %d\n", paquete->codigo_operacion); 
 
    recv(socket_memoria, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

    paquete->buffer->stream = malloc(paquete->buffer->size);
    //recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);
    recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

    // Ahora en función del código recibido procedemos a deserializar el resto
    switch(paquete->codigo_operacion) {
        case ENVIO_INSTRUCCION:
            t_instruccion* instruccion = malloc(sizeof(t_instruccion));
            instruccion = instruccion_deserializar(paquete->buffer);
            ejecutar_instruccion(instruccion, pcb);
            free(instruccion);
            break;
        case CANTIDAD:
            cantidad_instrucciones = cantidad_instrucciones_serializar(paquete->buffer);
            break;
        default:
            printf("Error: Fallo!\n");
            break;
    }

    // Liberamos memoria
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

t_instruccion* instruccion_deserializar(t_buffer* buffer) {
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    instruccion->parametros = list_create();

    void* stream = buffer->stream;

    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(instruccion->nombre), stream, sizeof(TipoInstruccion));
    stream += sizeof(TipoInstruccion);
    memcpy(&(instruccion->cantidad_parametros), stream, sizeof(int));
    stream += sizeof(int);
    
    printf("Recibimos la instruccion de enum %d con %d parametros.\n", instruccion->nombre, instruccion->cantidad_parametros);

    if(instruccion->cantidad_parametros == 0) {
        return instruccion;
    }
    
    for(int i=0; i<instruccion->cantidad_parametros; i++) {
        t_parametro* parametro = malloc(sizeof(t_parametro));
        memcpy(&(parametro->length), stream, sizeof(int));
        printf("Este es el largo del parametro %d que llego: %d\n", i, parametro->length);
        stream += sizeof(int);
        
        parametro->nombre = malloc(sizeof(char) * parametro->length + 1);
        memcpy(parametro->nombre, stream, sizeof(char) * parametro->length);
        
        parametro->nombre[parametro->length] = '\0';
        stream += parametro->length;
        printf("Parametro: %s\n", parametro->nombre);

        list_add(instruccion->parametros, parametro);
        // free(parametro->nombre);
    }

    return instruccion;
}

void ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb) {
 
    // log_info(logger, "PID: %d - Ejecutando: %d ", pcb->pid, instruccion->nombre);
    
    TipoInstruccion nombreInstruccion = instruccion->nombre; 
    t_list* list_parametros = instruccion->parametros;

    printf("La instruccion es de numero %d y tiene %d parametros\n", instruccion->nombre, instruccion->cantidad_parametros);

    switch(nombreInstruccion) {
        case SET: 
            // Obtengo los datos de la lista de parametros
            t_parametro* registro_param = list_get(list_parametros, 0);
            char* registro1 = registro_param->nombre;
            
            t_parametro* valor_param = list_get(list_parametros, 1);

            char* valor_char = valor_param->nombre;
            int valor = atoi(valor_char);

            // Aca obtengo la direccion de memoria del registro
            void* registro_pcb1 = seleccionar_registro_cpu(registro1); 

            // Aca obtengo si es de 8 bits o no y uso eso para setear, sino funciona el bool uso un int
            bool es_registro_uint8 = es_de_8_bits(registro1);
            set(registro_pcb1, valor, es_registro_uint8); 

            printf("Cuando hace SET AX 1 queda asi el registro AX del CPU: %u\n", registros_cpu->AX);
            printf("Cuando hace SET BX 1 queda asi el registro BX del CPU: %u\n", registros_cpu->BX);
            sleep(5);
            break;
        case MOV_IN:
            break;
        case MOV_OUT:
            break;
        case SUM: 
            /* 
            char* registro1 = (char*)list_get(list_parametros, 0);
            void* valor_void = (char*)list_get(list_parametros, 1); 
            void* registro_pcb1 = seleccionar_registro(registro1, pcb); 
            
            int es_registro_uint8 = es_de_8bits(registro1);
            sum(registro_pcb1, valor_void, es_registro_uint8, pcb);
            break;

            char* registro1 = list_get(list_parametros, 0);
            char* registro2 = list_get(list_parametros, 1);

            void* registroDestino = seleccionar_registro(registro1, pcb);
            void* registroOrigen = seleccionar_registro(registro2, pcb);

            bool es_registro_uint8 = es_de_8_bits(registro1);
            sum(registroDestino, registroOrigen, es_registro_uint8, pcb);
            */
        case SUB:
        /*
            char* registro1 = list_get(list_parametros, 0);
            char* registro2 = list_get(list_parametros, 1);

            void* registroDestino = seleccionar_registro(registro1, pcb);
            void* registroOrigen = seleccionar_registro(registro2, pcb);

            bool es_registro_uint8 = es_de_8_bits(registro1);
            sub(registroDestino, registroOrigen, es_registro_uint8, pcb);
        */
            break;
        
        case JNZ:
        /*
            char* registro1 = list_get(list_parametros, 0);
            char* nuevo_pc = list_get(list_parametros, 1);

            void* registro = seleccion_registro(registro1, pcb);
            
            jnz(registro1, nuevo_pc, pcb);
        */     
            break;
    
        case RESIZE:
            break;
        case COPY_STRING:
            break;
        case WAIT:
            break;
        case SIGNAL:
            break;
        case IO_GEN_SLEEP:
            break;
        case IO_STDIN_READ:
            break;
        case EXIT_INSTRUCCION:
            // guardar_estado(pcb); -> No estoy seguro si esta es necesaria, pero de todas formas nos va a servir cuando se interrumpa por quantum
            setear_registros_cpu();
            //enviar_fin_programa(pcb); // No hecha, seria enviarle la kernel para que haga lo suyo
            break;
        default:
            printf("Error: No existe ese tipo de intruccion\n");
            break;
    }

    // log_info(logger, "PID: %d - Finalizando %d", pcb->pid, instruccion->nombre);
    // TO DO: Check interrupt // 
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

void* seleccionar_registro_cpu(char* nombreRegistro) { 

    if (strcmp(nombreRegistro, "AX") == 0) {
        return &registros_cpu->AX;
    } else if (strcmp(nombreRegistro, "BX") == 0) {
        return &registros_cpu->BX;    
    } else if (strcmp(nombreRegistro, "CX") == 0) {
       return &registros_cpu->CX;
    } else if (strcmp(nombreRegistro, "DX") == 0) {
        return &registros_cpu->DX;
    } else if (strcmp(nombreRegistro, "SI") == 0) {
        return &registros_cpu->SI;
    } else if (strcmp(nombreRegistro, "DI") == 0) {
        return &registros_cpu->DI;
    } else if (strcmp(nombreRegistro, "EAX") == 0) {
        return &registros_cpu->EAX;  
    } else if (strcmp(nombreRegistro, "EBX") == 0) {
        return &registros_cpu->EBX;
    } else if (strcmp(nombreRegistro, "ECX") == 0) {
        return &registros_cpu->ECX;
    } else if (strcmp(nombreRegistro, "EDX") == 0) {
        return &registros_cpu->EDX;
    }

    return NULL;
    
}

/* 
void* seleccionar_registro(char* nombreRegistro, t_pcb *pcb) { 

    if (strcmp(nombreRegistro, "AX") == 0) {
        return &pcb->registros->AX;
    } else if (strcmp(nombreRegistro, "BX") == 0) {
        return &pcb->registros->BX;    
    } else if (strcmp(nombreRegistro, "CX") == 0) {
       return &pcb->registros->CX;
    } else if (strcmp(nombreRegistro, "DX") == 0) {
        return &pcb->registros->DX;
    } else if (strcmp(nombreRegistro, "SI") == 0) {
        return &pcb->registros->SI;
    } else if (strcmp(nombreRegistro, "DI") == 0) {
        return &pcb->registros->DI;
    } else if (strcmp(nombreRegistro, "EAX") == 0) {
        return &pcb->registros->EAX;  
    } else if (strcmp(nombreRegistro, "EBX") == 0) {
        return &pcb->registros->EBX;
    } else if (strcmp(nombreRegistro, "ECX") == 0) {
        return &pcb->registros->ECX;
    } else if (strcmp(nombreRegistro, "EDX") == 0) {
        return &pcb->registros->EDX;
    }

    return NULL;
}
*/

bool es_de_8_bits(char* registro) {
    return (strcmp(registro, "AX") == 0 || strcmp(registro, "BX") == 0 ||
            strcmp(registro, "CX") == 0 || strcmp(registro, "DX") == 0);
}

///////////////////////////////
//////////  SET  //////////////
///////////////////////////////

void set(void* registro, uint32_t valor, bool es_8_bits) {
    //registro = &valor;
    if (es_8_bits) {
        *(uint8_t*)registro = (uint8_t)valor;  // NO va a haber ninguno registro de 8 bits que pase el limite
    } else {
        *(uint32_t*)registro = valor;
    }
}

///////////////////////////////
//////////  SUM  //////////////
/////////////////////////////// 

/*
void sum(void* registroDestino, void* registroOrigen, bool es_8_bits, t_pcb* pcb){
    void* registro2; 
    if(!(todosSonDigitosDe(valor))) {         
        registro2 = seleccionar_registro(valor, pcb); // Podria ser int* ???? 
        if (es_8_bits) {
            *(uint8_t*)registro += (uint8_t)valor; // NO va a haber ninguno registro de 8 bits que pase el limite
        } else {
            *(uint32_t*)registro += (uint32_t) valor;
        }
    } else {
        int registro2 = (int *) valor;
    }
    if(es_8_bits) {
        *(uint8_t*)registroDestino += *(uint8_t*)registroOrigen;
    } else {
        *(uint32_t*)registroDestino += *(uint32_t*)registroOrigen;
    }
}
*/
/*

///////////////////////////////
//////////  SUB  //////////////
///////////////////////////////

void sub(void* registroDestino, void* registroOrigen, bool es_8_bits, t_pcb* pcb) {
    if(es_8_bits) {
        *(uint8_t*)registroDestino -= *(uint8_t*)registroOrigen;
    } else {
        *(uint32_t*)registroDestino -= *(uint32_t*)registroOrigen;
    }
}

///////////////////////////////
//////////  JNZ  //////////////
///////////////////////////////

void jnz(void* registro, int valor, pcb){
    if(*(void*)registro == 0) {
        pcb->program_counter = valor;
    }
}
*/

////////////////////////////////////////
//////////  IO_GEN_SLEEP  //////////////
////////////////////////////////////////


/* 
void io_gen_sleep(char* nombre_interfaz, char* unidades_de_trabajo) {
    // TO DO : Esta instrucción solicita al Kernel que se envíe a una interfaz de I/O a que realice un 
    // sleep por una cantidad de unidades de trabajo.

}
*/
