#include "../include/operaciones.h"

void ejecutar_pcb(t_pcb* pcb,int socket_memoria) {
    // Va a terminar cuando llegue a EXIT
    while(1) {
        t_instruccion* instruccion = malloc(sizeof(t_instruccion));
        // FETCH //
        pedir_instruccion_a_memoria(socket_memoria, pcb);
        recibir_instruccion(socket_memoria, pcb);
        
        pcb->program_counter++;
    }
}

// Manda a memoria el pcb, espera una instruccion y la devuelve
void* pedir_instruccion_a_memoria(int socket_memoria, t_pcb* pcb) {
    // Recibimos cada parámetro
    t_instruccion* instruccion;

    t_solicitud_instruccion* pid_pc;
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
    send(socket, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
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

typedef struct {
    uint8_t *registro8;
    uint32_t *registro32;
} RegistroSeleccionado;

void* recibir_instruccion(int socket_memoria, int pcb) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));

    // Primero recibimos el codigo de operacion
    recv(socket_memoria, &(paquete->codigo_operacion), sizeof(uint8_t), 0);

    // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
    recv(socket_memoria, &(paquete->buffer->size), sizeof(uint32_t), 0);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);


    // Ahora en función del código recibido procedemos a deserializar el resto
    switch(paquete->codigo_operacion) {
        case ENVIO_INSTRUCCION:
            t_instruccion* instruccion = instruccion_serializar(paquete->buffer);
            ejecutar_instruccion(instruccion, pcb);
            /*// DECODE //
            switch(instruccion->nombre) {
                case SET:
                    // TO DO: SET
                    break;
                case SUM:
                    break;
                
                default:
                    printf("La instruccion no estaba en la lista de instrucciones\n");
                    break;
            }*/
            free(instruccion);
            break;
    }

    // Liberamos memoria
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
    /*
    // Recibimos el nombre de la instrucción
    recv(socket_memoria, &(instruccion->nombre), sizeof(TipoInstruccion), 0);

    // Recibimos la cantidad de parámetros
    int cantidad_parametros;
    recv(socket_memoria, &cantidad_parametros, sizeof(int), 0);

    instruccion->parametros = list_create();

    // Recibimos la instruccion y deserializamos
    // TO DO 
    for (int i = 0; i < cantidad_parametros; i++) {
        void* longitud_parametro;
        recv(socket_memoria, &longitud_parametro, sizeof(int), 0);
        char* parametro = malloc(longitud_parametro);
        recv(socket_memoria, parametro, longitud_parametro, 0);
        list_add(instruccion->parametros, parametro);
    }
    */
}

/*
t_list de la instruccion guarda una lista de parametros

typedef struct {
    char* nombre;
    int length;
} t_parametro;
*/

t_instruccion* instruccion_serializar(t_buffer* buffer) {
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(instruccion->nombre), stream, sizeof(TipoInstruccion));
    stream += sizeof(TipoInstruccion);
    memcpy(&(instruccion->cantidad_parametros), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(instruccion->parametros), stream, sizeof(t_parametro) * instruccion->cantidad_parametros);
    stream += sizeof(t_parametro) * instruccion->cantidad_parametros;
    
    for(int i=0; i<instruccion->cantidad_parametros; i++) {
        char* parametro = list_get(instruccion->parametros, i);
        memcpy(&(parametro), stream, list_size(instruccion->parametros));
        stream += list_size(instruccion->parametros) ;
    }
    
    return instruccion;
}

/*
 // ESTO ES EN EL LADO DE LA MEMORIA
void enviar_instruccion(int socket_cpu, t_instruccion* instruccion) {
    // Primero enviamos el nombre de la instrucción
    send(socket_cpu, &(instruccion->nombre), sizeof(TipoInstruccion), 0);

    // Luego enviamos la cantidad de parámetros
    int cantidad_parametros = list_size(instruccion->parametros);
    send(socket_cpu, &cantidad_parametros, sizeof(int), 0);

    // Enviamos cada parámetro
    for (int i = 0; i < cantidad_parametros; i++) {
        char* parametro = list_get(instruccion->parametros, i);
        int longitud_parametro = strlen(parametro) + 1; // Incluimos el '\0'
        send(socket_cpu, &longitud_parametro, sizeof(int), 0); // Enviamos la longitud del parámetro
        send(socket_cpu, parametro, longitud_parametro, 0); // Enviamos el parámetro en sí
    }
}*/

/*typedef struct {
    TipoInstruccion nombre;
    t_list* parametros; // Cada una de las instrucciones
    int cantidad_parametros;
} t_instruccion;*/

void ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb) {
    // Se puede hacer con Strings también -> Enums
    //log para mostrar las instrucciones
    log_info(logger, "PID: %d - Ejecutando: %s - Parametros: %s", pcb->pid, instruccion->nombre, list_iterate(instruccion->parametros, printf));
    // void list_iterate(t_list *, void(*closure)(void*));
        
    // Decode // -> Aca tendria que ver si va a memoria? implica devolucion a kernel?

    // Execute // -> Modelo de ejemplo basico
    // SEGUIR CON ESTOOO
    TipoInstruccion nombreInstruccion = instruccion->nombre; // Set AX 1
    t_list* list_parametros = instruccion->parametros;

    // TO DO: Seleccionar registro tiene que recibir un enum TipoInstruccion
    // Esta bien que el switch sea segun el nombre de la instruccion, pero se tiene que seleccionar el registro
    // segun la instruccion, imaginate un resize o un exit, no reciben un registro por parametro.
    // char* registro = seleccionar_registro(nombreInstruccion, pcb);
    switch(nombreInstruccion) {
        case SET:

        /* 
        typedef struct {
        char* nombre;
        int tipo; // 0 para uint8_t, 1 para uint32_t
} t_registro_info;

    t_registro_info registros_info[] = {
        {"AX", 0},
        {"BX", 0},
        // ...
        {"EAX", 1},
        {"EBX", 1},
        // ...
};

    int obtener_tipo_registro(char* nombre) {
        for (int i = 0; i < sizeof(registros_info) / sizeof(t_registro_info); i++) {
            if (strcmp(nombre, registros_info[i].nombre) == 0) {
             return registros_info[i].tipo;
        }
    }
    return -1; // No se encontró el registro
}
    char* registro1 = list_get(list_parametros, 0);
    int tipo_registro1 = obtener_tipo_registro(registro1);
    if (tipo_registro1 == 0) {
        // registro1 es un uint8_t -> casteo a uint_8 
    } else if (tipo_registro1 == 1) {
    // registro1 es un uint32_t // casteo uint32_t
    } else {
    // No se reconoció el registro
    }
        */
            char* registro1 = list_get(list_parametros, 0); 
                                                            // El tema es q se repeteria un poco de logica porque lo vas a tratar igual en mo            // svalorca habria que ver de hacer poner la funcion para que pueda castear el tipo y me lo devuelva con su tipo (nose tdvia mb como)
            puntero_pcb = seleccionar_registro(registro1,pcb);
            set(puntero_pcb, registro2);
            break;
        case MOV_IN:
            mov_in(parametros...);
            break;
        case MOV_OUT:
            break;
        case SUM:
            sum(registro,valor);
            break;
        case SUB:
            sub(registro,valor);
            break;
        case JNZ:
            jnz(registro,valor)
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
    log_info(logger, "PID: %d - Finalizando %s - Parametros: %s", pcb->pid, instruccion->nombre, list_iterate(instruccion->parametros, printf));
    // TO DO: Check interrupt // 
}

void* set(t_instruccion* instruccion) {
    char* registro1 = list_get(instruccion->parametros, 0);
    instruccion->parametros->head = instruccion->parametros[instruccion->parametros->elements_count - 1];
}

// Revisar, como hacemos esto????
t_registros* seleccionar_registro(char* nombreRegistro, t_pcb *pcb) {
    t_registros* registro = NULL;

    if (strcmp(nombreRegistro, "AX") == 0) {
        registro = &pcb->registros->AX;
    } else if (strcmp(nombreRegistro, "BX") == 0) {
        registro = &pcb->registros->BX;    
    } else if (strcmp(nombreRegistro, "CX") == 0) {
       registro = &pcb->registros->CX;
    } else if (strcmp(nombreRegistro, "DX") == 0) {
        registro = &pcb->registros->DX;
    } else if (strcmp(nombreRegistro, "SI") == 0) {
        registro = &pcb->registros->SI;
    } else if (strcmp(nombreRegistro, "DI") == 0) {
        registro = &pcb->registros->DI;
    } else if (strcmp(nombreRegistro, "EAX") == 0) {
        registro = &pcb->registros->EAX;  
    } else if (strcmp(nombreRegistro, "EBX") == 0) {
        registro = &pcb->registros->EBX;
    } else if (strcmp(nombreRegistro, "ECX") == 0) {
        registro = &pcb->registros->ECX;
    } else if (strcmp(nombreRegistro, "EDX") == 0) {
        registro = &pcb->registros->EDX;
    }else if(todosSonDigitosDe(nombreRegistro)) {
        registro = atoi(nombreRegistro); 
    }
    return registro;   
}

int todosSonDigitosDe(char *nombreRegistro) { //con libreria #include <ctype.h>
    while(*nombreRegistro != '\0'){
        if(!isdigit(*nombreRegistro)) {
            return 0;
        }
        nombreRegistro++; // No sabemos si anda pero ahi esta la idea 
    }
    return 1;
}


/////////////////////////

/*
registttr
void set int valor(t_pcb* pcb, char* registro, char* valor) { //SET AX 1
    int valor_atoi = atoi(valor);
    RegistroSeleccionado registro_seleccionado;

    //Hay problemas por todos lados. TO DO: Todo.

    if(es_registro_uint8(registro)){
        memcpy(pcb->registros->AX, valor, sizeof(uint8_t));
    } else {
        memcpy(pcb->registros->AX, valor, sizeof(uint32_t));
    }
}

*/

*////////////////////////
void set int valor(t_pcb* pcb, char* registro, char* valor) { //SET AX 1
    int valor_atoi = atoi(valor);
    RegistroSeleccionado registro_seleccionado;

    //Hay problemas por todos lados. TO DO: Todo.

    if(es_registro_uint8(registro)){
        memcpy(pcb->registros->AX, valor, sizeof(uint8_t));
    } else {
        memcpy(pcb->registros->AX, valor, sizeof(uint32_t));
    }
}

bool es_registro_uint8(registro) {
    return (strcmp(registro, "AX" ==0) || strcmp(registro, "BX" ==0) || strcmp(registro, "CX" ==0) || strcmp(registro, "CX" ==0));
}


void sum(RegistroSeleccionado registro,(void*) valor){
    *(registro->registro8) += *((uint32_t *)valor);
    *(registro->registro32) += *((uint32_t *)valor);
}

void sub(RegistroSelecccionado registro,(void*) valor) {
    *(registro->registro8) -= *((uint8_t *)valor);
    *(registro->registro32) -= *((uint32_t *)valor);   
}

void jnz(RegistroSelecccionado registro,(void*) valor){
    if(registro -> registro8 == 0 || registro -> registro32 == 0){
        pc = valor;
    }
}


/* 
void dormirInterfaz(char* nombre_interfaz, char* unidades_de_trabajo) {
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

