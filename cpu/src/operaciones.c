#include "../include/operaciones.h"

void cicloDeInstruccion(t_pcb* pcb){
    t_instruccion* instruccion;
    
    fetch(pcb);
    decode(pcb);
    execute(pcb);
}

/*
La primera etapa del ciclo consiste en buscar la próxima instrucción a ejecutar. 
En este trabajo práctico cada instrucción deberá ser pedida al módulo Memoria utilizando el Program Counter 
(también llamado Instruction Pointer) que representa el número de instrucción a buscar relativo al proceso en ejecución. 
Al finalizar el ciclo, este último deberá ser actualizado (sumarle 1) si corresponde.
*/

/*
Una vez recibido el PCB del kernel, se pide la direccion de memoria de dicho
proceso para pedirle luego la direccion de memoria a Memoria.
*/

/*
typedef struct {
	char* nombre;
	t_list* parametros; //lista de subinstrucciones
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

t_instruccion* fetch(t_pcb* pcb) {
    // Program counter del pcb va a Memoria para pedir la instruccion (el numero de instruccion a buscar relativo al proceso en ejecucion)
    // Tendremos que buscar el program counter en la cola de procesos a ejecutar?-> no es el del 

    t_instruccion *instruccion;
    instruccion = malloc(sizeof(t_instruccion));
    instruccion = pedir_instruccion_a_memoria(pcb);

    (pcb->program_counter)++;

//     return instruccion;
}

void decode(t_pcb* pcb) {
    /*
    Esta etapa consiste en interpretar qué instrucción es la que se va a ejecutar 
    y si la misma requiere de una traducción de dirección lógica a dirección física.
    
    
    
    
    */
}

void execute(t_pcb* pcb) {

}

//NO es void: ¿es un enum? ¿struct? ¿Que es una instruccion? Hay que usar dictionary?
void pedir_instruccion_a_memoria(int pid, int program_counter) {
    
}

// SET, SUM, SUB, JNZ e IO_GEN_SLEEP


/////////////////////////////////////
// EJECUTAR INSTRUCCIONES DE JOACO //
/////////////////////////////////////
/*
void ejecutar_instrucciones(t_pcb* pcb) {
	int cantidad_instrucciones = list_size(pcb->instrucciones);
	t_instruccion* instruccion_actual_t;

	while(pcb->pc < cantidad_instrucciones) {
		///////////
		// Fetch //
		///////////
		instruccion_actual_t = list_get(pcb->instrucciones, pcb->pc);
		pcb->pc++;

		t_list* parametros_actuales = instruccion_actual_t->parametros;

		char* parametros_a_emitir = obtener_parametros_a_emitir(parametros_actuales);
		log_info(logger, "PID: %d - Ejecutando: %s - %s", pcb->pid, instruccion_actual_t->nombre, parametros_a_emitir); //log obligatorio
		free(parametros_a_emitir);

		////////////
		// Decode //
		////////////
		t_msj_kernel_cpu instruccion_actual = instruccion_a_enum(instruccion_actual_t);

		/////////////
		// Execute //
		/////////////
		switch(instruccion_actual) {
			case SET:
				set_registro(pcb, list_get(parametros_actuales, 0), list_get(parametros_actuales, 1));
				break;

			case MOV_IN: case MOV_OUT: case F_READ: case F_WRITE:
				
                // COMENTADO
				MOV_IN lee de Memoria y escribe en registro
				MOV_OUT lee el registro y escribe en Memoria

				F_READ lee del archivo y escribe en Memoria
				F_WRITE lee de Memoria y escribe en archivo
				// FIN DE COMENTADO

				// OBTENCION DE DATOS
				int indice = numero_de_parametro_de_direccion_logica(instruccion_actual);
				int direccion_logica = atoi(list_get(parametros_actuales, indice));

				t_datos_mmu datos_mmu = mmu(pcb, direccion_logica);

				int cantidad_de_bytes = obtener_cantidad_de_bytes(instruccion_actual, parametros_actuales);

				// EVALUACION SEG_FAULT
				if(datos_mmu.desplazamiento_segmento + cantidad_de_bytes > datos_mmu.tamanio_segmento) {
					log_info(logger, "PID: %d - Error SEG_FAULT - Segmento: %d - Offset: %d - Tamaño: %d", pcb->pid, datos_mmu.numero_segmento, datos_mmu.desplazamiento_segmento, datos_mmu.tamanio_segmento); //log obligatorio

					enviar_pcb_a_kernel(pcb, EXIT_CON_SEG_FAULT, parametros_actuales);
					return;
				}
				else {
					//Atentos a la mayor (ya no... xd) villereada de todos los tiempos!!

				    // EJECUCION INSTRUCCION
					switch(instruccion_actual) {
						case MOV_IN:
							ejecutar_mov_in(pcb, datos_mmu, obtener_registro(instruccion_actual, parametros_actuales));
							break;
						case MOV_OUT:
							ejecutar_mov_out(pcb, datos_mmu, obtener_registro(instruccion_actual, parametros_actuales));
							break;
						default:
							ejecutar_fread_o_fwrite(pcb, datos_mmu.direccion_fisica, instruccion_actual, parametros_actuales);
							return;
					}
				}

				break;

			case IO: case F_OPEN: case F_CLOSE: case F_SEEK: case F_TRUNCATE: case WAIT:
			case SIGNAL: case CREATE_SEGMENT: case DELETE_SEGMENT: case YIELD: case EXIT:
				enviar_pcb_a_kernel(pcb, instruccion_actual, parametros_actuales);
				return;

			default:
				log_error(my_logger, "Error en el decode de la instruccion");
				//exit(EXIT_FAILURE);
				break;
		}
	}
}
*/

void set_registro(t_pcb* pcb, char* registro, char* valor) {
	if(!strcmp(registro, "AX")) {
		memcpy(pcb->registros_cpu.AX, valor, 4 * sizeof(char));
	}
	else if(!strcmp(registro, "BX")) {
		memcpy(pcb->registros_cpu.BX, valor, 4 * sizeof(char));
	}
	else if(!strcmp(registro, "CX")) {
		memcpy(pcb->registros_cpu.CX, valor, 4 * sizeof(char));
	}
	else if(!strcmp(registro, "DX")) {
		memcpy(pcb->registros_cpu.DX, valor, 4 * sizeof(char));
	}
	else if(!strcmp(registro, "EAX")) {
		memcpy(pcb->registros_cpu.EAX, valor, 8 * sizeof(char));
	}
	else if(!strcmp(registro, "EBX")) {
		memcpy(pcb->registros_cpu.EBX, valor, 8 * sizeof(char));
	}
	else if(!strcmp(registro, "ECX")) {
		memcpy(pcb->registros_cpu.ECX, valor, 8 * sizeof(char));
	}
	else if(!strcmp(registro, "EDX")) {
		memcpy(pcb->registros_cpu.EDX, valor, 8 * sizeof(char));
	}
	else if(!strcmp(registro, "RAX")) {
		memcpy(pcb->registros_cpu.RAX, valor, 16 * sizeof(char));
	}
	else if(!strcmp(registro, "RBX")) {
		memcpy(pcb->registros_cpu.RBX, valor, 16 * sizeof(char));
	}
	else if(!strcmp(registro, "RCX")) {
		memcpy(pcb->registros_cpu.RCX, valor, 16 * sizeof(char));
	}
	else if(!strcmp(registro, "RDX")) {
		memcpy(pcb->registros_cpu.RDX, valor, 16 * sizeof(char));
	}

	usleep(lectura_de_config.RETARDO_INSTRUCCION * 1000); //Porque me lo dan en milisegundos
	return;
}
