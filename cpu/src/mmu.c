#include "../include/mmu.h"

int tamanio_pagina;
t_dictionary* tlb;

/*
typedef struct {
    int pid;
    int pagina;
    int marco;
    long timestamps;
} t_tlb;
*/

void inicializar_tlb(t_config* config_CPU) {
    //int cant_entradas = config_get_int_value(config_CPU, "CANTIDAD_ENTRADAS_TLB");
    tlb = dictionary_create();
}

/* 
bool estaEnLaTLB(int pid, int pagina) {
    
}
*/

/* 
void* crear_tlb() {
    t_tlb tlb;
    int cant_entradas = config_get_int_value(config_CPU, "CANTIDAD_ENTRADAS_TLB");
    char* algoritmo_tlb = config_get_string_value(config_CPU, "ALGORITMO_TLB");
    if(cant_entradas == 0) {
        deshabilitar_tlb();
    }
    return;
}

long tiempoEnMilisecs() {
  struct timeval time;
  gettimeofday(&time, NULL);

  return time.tv_sec * 1000 + time.tv_usec / 1000;
}

void guardar_en_TLB(int pid, int nro_pagina, int nro_marco) {
	t_tlb* nueva_entrada = malloc(sizeof(t_tlb));
	
    nueva_entrada->pid       = pid;
	nueva_entrada->pagina    = nro_pagina;
	nueva_entrada->marco     = nro_marco;
    nueva_entrada->timestamp = tiempoEnMilisecs();

	int cant_entradas   = config_get_int_value(config_CPU, "CANTIDAD_ENTRADAS_TLB");
    char* algoritmo_tlb = config_get_string_value(config_CPU, "ALGORITMO_TLB");

    switch(algoritmo_TLB) {
        case FIFO:
            queue_push(TLB, nueva_entrada);
            break;
        case LRU:
            algoritmo_LRU(nueva_entrada);
            break;
    }
}

void algoritmo_LRU(t_tlb* nueva_entrada) {
	t_tlb* auxiliar1 = malloc(sizeof(t_tlb));
	t_tlb* auxiliar2 = malloc(sizeof(t_tlb));

	int tamanio = queue_size(TLB);
	auxiliar1 = queue_pop(TLB);

	for(int i = 1; i < tamanio; i++) {
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

int esta_en_TLB(int nro_pagina) {
	int se_encontro = 0;	
	for(int i = 0; i < queue_size(TLB); i++) {
		t_tlb* una_entrada = queue_pop(TLB);
		if (una_entrada->pagina == nro_pagina) {
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

*/ 
// direccion_fisica = traducir_direccion_logica_a_fisica(tamanio_pagina, *registro_direccion11, pcb->pid);
int traducir_direccion_logica_a_fisica(uint32_t direccion_logica, int pid) { 
    t_direccion_logica* direccion_logica_a_crear = malloc(sizeof(t_direccion_logica)); 
    int frame;

    direccion_logica_a_crear->numero_pagina = floor(direccion_logica / tamanio_pagina);
    direccion_logica_a_crear->desplazamiento = direccion_logica - direccion_logica_a_crear->numero_pagina * tamanio_pagina;

    printf("Numero de pagina %d + Desplazamiento %d\n", direccion_logica_a_crear->numero_pagina, direccion_logica_a_crear->desplazamiento);

    pedir_frame_a_memoria(direccion_logica_a_crear->numero_pagina, pid); 
    
    recv(socket_memoria, &frame, sizeof(int), MSG_WAITALL);
    printf("Frame %d recibido de memoria con PID %d\n", frame, pid);
    
    int direccion_fisica = frame * tamanio_pagina + direccion_logica_a_crear->desplazamiento;
    
    free(direccion_logica_a_crear);

    return direccion_fisica;
}

void pedir_frame_a_memoria(int nro_pagina, int pid) {
    t_solicitud_frame* solicitud_frame = malloc(sizeof(t_solicitud_frame));
    t_buffer* buffer = malloc(sizeof(t_buffer));

    solicitud_frame->nro_pagina = nro_pagina;
    solicitud_frame->pid = pid;
    
    buffer->size = sizeof(int) + sizeof(int); 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &solicitud_frame->nro_pagina, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &solicitud_frame->pid, sizeof(int));
    buffer->offset += sizeof(int);

    buffer->stream = stream;

    printf("Pid de parametro %d\n", pid);
    printf("PID - %d\n", solicitud_frame->pid);
    enviar_paquete(buffer, QUIERO_FRAME, socket_memoria);
    free(solicitud_frame);
}

/* 
void vaciar_tlb() {
	while(queue_size(TLB) > 0){
		t_tlb* una_entrada = queue_pop(TLB);
		free(una_entrada);
	}
}
*/