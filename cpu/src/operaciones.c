/* 
void ejecutar_pcb(t_pcb* pcb){
    t_instruccion* instruccion;
    int cantidad_instrucciones = list_size(X); // -> Ver idea dictionario PID key y lista de valor

    while(pcb->pc < cantidad_instrucciones) {
        // Fetch // 
        instruccion_code = pedir_instruccion_a_memoria(socket_memoria, pcb->pc); // Que me lo cargue en el puntero
        instruccion = deserializar_instruccion(); // Aca agarro el nombre y los parametros

        log_info(logger, "PID: %d - Ejecutando: %s - %s", pcb->pid, instruccion_actual_t->nombre, parametros_a_emitir);

        // Decode // -> Aca tendria que ver si va a memoria? implica devolucion a kernel?

        // Execute // -> Modelo de ejemplo basico
        switch (instruccion->name)
        {
        case SET:
            // TODO
            break;
        case MOV_IN:
            // TODO
            break;


        default:
            break;
        }

        // Check interrupt // 

        pcb->pc++;
    }
}
*/
/*
Una vez recibido el PCB del kernel, se pide la direccion de memoria de dicho
proceso para pedirle luego la direccion de memoria a Memoria.
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


//NO es void: ¿es un enum? ¿struct? ¿Que es una instruccion? Hay que usar dictionary?
void pedir_instruccion_a_memoria(int pid, int program_counter) {
    
}