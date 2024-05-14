#include "../include/operaciones.h"

/*
void ejecutar_pcb(t_pcb* pcb){ // falta traer socket_memoria
    t_instruccion* instruccion;
    int cantidad_instrucciones = list_size(X); // -> Ver idea dictionario PID key y lista de valor

    while(pcb->pc < cantidad_instrucciones) {
        // Fetch // 
        pedir_instruccion_a_memoria(socket_memoria, pcb); // Que me lo cargue en el puntero
        instruccion = recibir(socket_memoria); // Aca agarro el nombre y los parametros
        pcb->pc++;
    }   
}
*/

typedef struct {
    uint8_t *registro8;
    uint32_t *registro32;
} RegistroSeleccionado;

typedef struct {
    TipoInstruccion nombre;
    t_list* parametros; // Cada una de las instrucciones
} t_instruccion;

/* 

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
}

*/
void get_registros_set() {
       
}

/*
void ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb) {
    // Se puede hacer con Strings también -> Enums
    log_info(logger, "PID: %d - Ejecutando: %s - Parametros: %s", pcb->pid, instruccion->nombre, list_iterate(instruccion->parametros, printf));
    // void list_iterate(t_list *, void(*closure)(void*));
        
    // Decode // -> Aca tendria que ver si va a memoria? implica devolucion a kernel?

    // Execute // -> Modelo de ejemplo basico
    
    char *nombreRegistro = (char*)instruccion->nombre; // Set AX 1
    t_list* list_parametros = instruccion->parametros

    t_registro* registro1 = seleccionar_registro(list_get(list_parametros,0), pcb);
    t_registro* registro2 = seleccionar_registro(list_get(list_parametros,1), pcb);
    t_registro* registro3 = seleccionar_registro(list_get(list_parametros,2), pcb);
    t_registro* registro4 = seleccionar_registro(list_get(list_parametros,3), pcb);
    t_registro* registro5 = seleccionar_registro(list_get(list_parametros,4), pcb);
    // Puede haber segmentation fault aca, estamos laburando con memoria a pleno, si se les ocurre alguna mejora o si me quieren putear haganlo (vamos, puedo putear)...

    switch (nombreRegistro) {
        case SET:
            get_registros_set();
            set(parametro[1], parametro[2]);
            break;
        case MOV_IN:
            get_registros_mov_in();
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
            printf("Error: No exxiste ese tipo de intruccion\n");
            break;
    }
    // Pongo log papra ver  si se  modifica
    log_info(logger, "PID: %d - Finalizando %s - Parametros: %s", pcb->pid, instruccion->nombre, list_iterate(instruccion->parametros, printf));
    // TO DO: Check interrupt // 
}
*/
/* 

t_registro* seleccionar_registro(char* nombreRegistro, t_pcb *pcb) {
    t_registro* registro;

    if (strcmp(nombreRegistro, "AX") == 0) {
        registro = &pcb->registros->AX;
    } else if (strcmp(nombreRegistro, "BX") == 0) {
        registro = &pcb->registros->BX;    
    } else if (strcmp(nombreRegistro, "CX") == 0) {
       registro = &pcb->registros->CX;
    } else if (strcmp(nombreRegistro, "DX") == 0) {
        registroSeleccionado->registro8 = &pcb->registros->DX;
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
    else if(todosSonDigitosDe(nombreRegistro)) {
        registro = atoi(nombreRegistro); 
    }else {
        // Registro no válido, se inicializan a NULL los punteros
        registro = NULL;
    }
    return registroSeleccionado;
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
// OPERACIONES DE CPU //
///////////////////////

void set(RegistroSeleccionado registro,(void*) valor) {
    memcpy(registro->registro8, valor, sizeof(uint8_t));
    memcpy(registro->registro32, valor, sizeof(uint32_t));
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
*/


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

