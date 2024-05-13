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

/* 
void ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb) {
    // TO-DO: FACU REVISA ESTO EN CONSOLA HERMOSO -> NO QUIERO MATI, NO ME OBLIGGUES
    // Se puede hacer con Strings también
    log_info(logger, "PID: %d - Ejecutando: %s - Parametros: %s", pcb->pid, instruccion->nombre, list_iterate(instruccion->parametros, printf));
    // void list_iterate(t_list *, void(*closure)(void*));
        
    // Decode // -> Aca tendria que ver si va a memoria? implica devolucion a kernel?

    // Execute // -> Modelo de ejemplo basico
    
    int registro = registroDe(instruccion->nombre); // Mati, sos un cabeza de zapallo, estas haciendo un switch con un registro (AX,BX)... no con el nombre de la instruccion
    switch (instruccion->nombre) {
        case "SET": // SET AX 1
            registro = instruccion -> parametros[1];
            break;
        case "MOV_IN": // MOV_IN EDX ECX
            // TODO
            break;
        case "MOV_OUT":
            break;
        case "SUM": // SUM AX 5
            pcb->(instruccion->parametros[0]) += instruccion -> parametros[1];
            break;
        case "SUB":
            pcb->(instruccion->parametros[0]) -= instruccion -> parametros[1];
            break;
        case "JNZ": // JNZ (variable_a_comparar) (salto_pc)
            if(instruccion->parametros[0] != 0) {
                pcb->pc = instruccion->parametros[1]; 
            }  
            break;
        case "RESIZE":
            break;
        case "COPY_STRING":
            //  TODO
            break;
        case "WAIT":
            break;
        case "SIGNAL":
            break;
        case "IO_GEN_SLEEP": // IO_GEN_SLEEP interfaz(1,2,3) unidades_de_trabajo
            dormirInterfaz(instruccion->parametros[0], instruccion->parametros[1]); // TODO en entrada salida
            break;
        case "IO_STDIN_READ":
            break;
        default:
            break;
    }
    */
    // TO DO: Check interrupt // 

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

/*
registroDe(char* registro){
    switch(registro){
        case "AX":
            return pcb->registros->AX;
            break;
        case "BX":
            return pcb->registros->BX;
        case "CX":
            return pcb->registros->CX;    
        case "DX":
            return pcb->registros->DX;
        case "EAX":
            return pcb->registros->EAX;
        case "EBX":
            return pcb->registros->EBX;
        case "ECX":
            return pcb->registros->ECX;    
        case "EDX":
            return pcb->registros->EDX;
        case "SI":
            return pcb->registros->SI;
        case "DI":
            return pcb->registros->DI;
    }
}
*/