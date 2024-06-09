#include "../include/mmu.h"

void* crear_tlb() {
    t_tlb tlb;
    int cant_entradas = config_get_int_value(config_CPU, "CANTIDAD_ENTRADAS_TLB");
    char* algoritmo_tlb = config_get_string_value(config_CPU, "ALGORITMO_TLB");
    if(cant_entradas == 0){
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

/*            -------------------> POSIBLE SOLUCION FACU <-------------------
int traducir_direccion_logica_a_fisica_2(int tamanio_pagina, int direccion_logica) {
    t_direccion_logica* direccion_logica_a_crear;
    direccion_logica_a_crear->numero_página = floor(direccion_logica / tamanio_pagina);
    direccion_logica_a_crear->desplazamiento = direccion_logica - direccion_logica->numero_página * tamanio_pagina;
    
    // Ahora si, yo le pido el frame a la memoria y calculo la dir fisica que me da el acceso directo al void* (falta implementar pedir frame a mem)
    int frame = pedir_frame_a_memoria(direccion_logica_a_crear->numero_pagina);
    int direccion_fisica = frame * tamanio_pagina + direccion_logica_a_crear->desplazamiento;

    return direccion_fisica;
}

int pedir_frame_a_memoria(int nro_pagina) { -> Reajustar
    send(socket_mem, &nro_pagina, sizeof(int), 0);
    recv(socket_mem, &frame, sizeof(int), 0);
    return frame;
}
*/


// Solucion 1 - Dir fisica con la pagina (?
int traducir_direccion_logica_a_fisica(int tamanio_pagina, int direccion_logica) {
    t_direccion_logica* direccion_logica_a_crear;
    direccion_logica_a_crear->numero_página = floor(direccion_logica / tamanio_pagina);
    direccion_logica_a_crear->desplazamiento = direccion_logica - direccion_logica->numero_página * tamanio_pagina;
    // yo creo que esta es la dir fisica pero en funcion de la pagina, no del frame
    int direccion_fisica = direccion_logica_a_crear->numero_pagina * tamanio_pagina + direccion_logica_a_crear->desplazamiento;

    return direccion_fisica;
}

int traer_tamanio_pagina_mem(int socket_mem) {
    recv(socket_mem,&tamanio_pagina, sizeof(int), 0);
    return tamanio_pagina;
}

int numero_pagina(int direccion_logica) {
	// el tamaño página se solicita a la memoria al comenzar la ejecución // falta hacerlo
	return direccion_logica / tamanio_pagina; // ---> la división ya retorna el entero truncado a la unidad
}

int desplazamiento_memoria(int direccion_logica, int nro_pagina) {
	return direccion_logica - nro_pagina * tamanio_pagina;
}

void vaciar_tlb() {
	while(queue_size(TLB) > 0){
		t_tlb* una_entrada = queue_pop(TLB);
		free(una_entrada);
	}
}