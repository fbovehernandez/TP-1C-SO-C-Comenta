#include "../include/operaciones.h"

t_log* logger;

void ejecutar_pcb(t_pcb* pcb, int socket_memoria) {
    // Va a terminar cuando llegue a EXIT
    while(pcb->program_counter < 10) { // Por ahora esto esta medio hecho para el culo, pero la idea es que yo de algun lado tengo que frenar
    // si ponemos pcb->estadoActual == 4
    // Por ahora veamos solo con 1
        // FETCH //
        pedir_instruccion_a_memoria(socket_memoria, pcb);
        //sem_wait(&sem_memoria_instruccion);
        recibir_instruccion(socket_memoria, pcb);
        
        pcb->program_counter++;
    }
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

void recibir_instruccion(int socket_memoria, t_pcb* pcb) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    t_instruccion* instruccion = malloc(sizeof(t_instruccion));

    // Primero recibimos el codigo de operacion
    printf("Esperando recibir instruccion...\n");

    //int bytes_recibidos = recv(socket_memoria, &(paquete->codigo_operacion), sizeof(codigo_operacion), 0);
    recv(socket_memoria, &(paquete->codigo_operacion), sizeof(codigo_operacion), MSG_WAITALL);
    printf("codigo de operacion: %d\n", paquete->codigo_operacion); // Veo codigo a ver que onda

    // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
    //recv(socket_memoria, &(paquete->buffer->size), sizeof(uint32_t), 0);
    //recv(socket_memoria, &(paquete->buffer->size), sizeof(int), 0);
    
    recv(socket_memoria, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

    paquete->buffer->stream = malloc(paquete->buffer->size);
    //recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);
    recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

    // Ahora en función del código recibido procedemos a deserializar el resto
    switch(paquete->codigo_operacion) {
        case ENVIO_INSTRUCCION:
            printf("SE RECIBIO ALGO \n");
            instruccion = instruccion_deserializar(paquete->buffer);
            //ejecutar_instruccion(instruccion, pcb);
            // free(instruccion);
            break;
        default:
            printf("Error: Fallo!\n");
            break;
    }

    // Liberamos memoria
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
    free(instruccion);
}

/*
t_list de la instruccion guarda una lista de parametros

typedef struct {
    char* nombre;
    int length;
} t_parametro;
*/

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
        
        //memcpy(&(parametro->length), stream, sizeof(int));
        //stream += sizeof(int);
        parametro->nombre = malloc(sizeof(char) * parametro->length + 1);
        memcpy(parametro->nombre, stream, sizeof(char) * parametro->length);
        
        /*
        parametro->nombre = malloc(parametro->length);
        memcpy(&(parametro->nombre), stream, parametro->length);
        */
        parametro->nombre[parametro->length] = '\0';
        stream += parametro->length;
        printf("Parametro: %s\n", parametro->nombre);

        list_add(instruccion->parametros, parametro);
        //free(parametro->nombre);
    }

    return instruccion;
}

void ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb) {
 
    // log_info(logger, "PID: %d - Ejecutando: %d ", pcb->pid, instruccion->nombre);
    
    TipoInstruccion nombreInstruccion = instruccion->nombre; // Set AX 1
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
            uint32_t* registro_pcb1 = seleccionar_registro(registro1, pcb); 

            // Aca obtengo si es de 8 bits o no y uso eso para setear, sino funciona el bool uso un int
            bool es_registro_uint8 = es_de_8_bits(registro1);
            set(registro_pcb1, valor, es_registro_uint8); 
            printf("Cuando hace SET AX 1 queda asi el registro AX del pcb: %u\n", pcb->registros->AX);
            break;
        case MOV_IN:
            //mov_in(parametros...);
            break;
        case MOV_OUT:
            break;
        case SUM: //entiendo que no pueden sumarse dos registros de tipos distintos, tipo un uint8 y un uint32
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
        default:
            printf("Error: No existe ese tipo de intruccion\n");
            break;
    }
    // Pongo log papra ver  si se  modifica
    // log_info(logger, "PID: %d - Finalizando %d", pcb->pid, instruccion->nombre);
    // TO DO: Check interrupt // 
}


///////////////////////////////
////  SELECCIONAR REGISTRO ////
///////////////////////////////
void* seleccionar_registro(char* nombreRegistro, t_pcb *pcb) { //BIEN MATI // bien mati

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

/*
bool todosSonDigitosDe(char* valor) {
   while (*valor) {
        if (!isdigit(*valor)) {
            return false;
        }
        valor++;
    }
    return true;
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

// Serializa el pcb y se lo manda a Memoria, que recibe el pcb con codigo PCB y manda una instruccion
void pedir_instruccion_a_memoria(int socket_memoria, t_pcb* pcb) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 3 // pid, pc, quantum
                + sizeof(enum Estado) * 2 // estados

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(stream + offset, &pcb->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + offset, &pcb->pc, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + offset, &pcb->quantum, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + offset, &pcb->estadoInicial, sizeof(enum Estado));
    buffer->offset += sizeof(enum Estado);
    memcpy(stream + offset, &pcb->estadoFinal, sizeof(enum Estado));
    buffer->offset += sizeof(int);

    buffer->stream = stream;
    
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = PCB; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(enum codigo_operacion));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(enum codigo_operacion));

    offset += sizeof(enum codigo_operacion);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(enum codigo_operacion), 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}
*/

/*
t_instruccion* deserializar_instruccion(int socket_memoria) {
    t_instruccion* instruccion;
    
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    // Primero recibimos el codigo de operacion
    recv(socket_memoria, &(paquete->codigo_operacion), sizeof(enum codigo_operacion), 0);

    // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
    recv(socket_memoria, &(paquete->buffer->size), sizeof(int), 0);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);

    // Ahora en función del código recibido procedemos a deserializar el resto
    switch(paquete->codigo_operacion) {
        case INSTRUCCION:
            instruccion = instruccion_serializar(paquete->buffer);
            return instruccion;
            // Hacemos lo que necesitemos con esta info
            // Y eventualmente liberamos memoria
            break;
        default:
            printf("Tenia que deserializar la instruccion, no se que paso.");
            break;
    }

    // Liberamos memoria
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);

    return instruccion;
}*/
/*
t_instruccion* instruccion_serializar(t_buffer) {
    return instruccion;
}*/

/*
()iUna vez recibido el PCntB del kernel, se pide la direccion de memoria de dicho
}proceso para pedirle luego la direccion de memoria a Memoria.
*/

/*
typedef struct {
	char* nombre;
	t_list* parametros; // lista de subinstrucciones
} t_instruccion;

// Por que subinstrucciones?

typedef struct {
		t_link_element *head;
		int elements_count;
} t_list;
*/

/*
AVERIGUAR como se relaciona el PATH del config.memoria con el pcb o la instruccion.

tendriamos que hacer un archivo en memoria que tenga las instrucciones, agarrar la instruccion
de interes, hacer un send a la CPU y un recv desde la CPU
*/

// Program counter del pcb va a Memoria para pedir la instruccion (el numero de instruccion a buscar relativo al proceso en ejecucion)
// Tendremos que buscar el program counter en la cola de procesos a ejecutar?-> no es el del 


