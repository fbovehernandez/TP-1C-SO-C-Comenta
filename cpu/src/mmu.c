#include "../include/mmu.h"

int tamanio_pagina;
t_log* logger_CPU;
t_list* tlb;

int traducir_direccion_logica_a_fisica(uint32_t direccion_logica, int pid) { 
    t_direccion_logica* direccion_logica_a_crear = malloc(sizeof(t_direccion_logica)); 

    direccion_logica_a_crear->numero_pagina = floor(direccion_logica / tamanio_pagina);
    direccion_logica_a_crear->desplazamiento = direccion_logica - direccion_logica_a_crear->numero_pagina * tamanio_pagina;

    printf("Numero de pagina %d + Desplazamiento %d\n", direccion_logica_a_crear->numero_pagina, direccion_logica_a_crear->desplazamiento);

    int frame = buscar_frame_en_TLB(pid, direccion_logica_a_crear->numero_pagina);
    
    if(frame == -1) { // Caso de que no encuentre el marco
        pedir_frame_a_memoria(direccion_logica_a_crear->numero_pagina, pid); 
        recv(socket_memoria, &frame, sizeof(int), MSG_WAITALL);
        log_info(logger_CPU, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pid, direccion_logica_a_crear->numero_pagina, frame);
        agregar_frame_en_TLB(pid, direccion_logica_a_crear->numero_pagina, frame);
    }

    // pedir_frame_a_memoria(direccion_logica_a_crear->numero_pagina, pid); 
    // imprimir_tlb();
    // recv(socket_memoria, &frame, sizeof(int), MSG_WAITALL);
    
    printf("Frame %d recibido de memoria con PID %d\n", frame, pid);
    
    int direccion_fisica = frame * tamanio_pagina + direccion_logica_a_crear->desplazamiento;
    
    free(direccion_logica_a_crear);

    return direccion_fisica;
}

int buscar_frame_en_TLB(int pid, int pagina) {
    for(int i=0; i<list_size(tlb); i++){
        t_entrada_tlb* entrada = list_get(tlb, i);
        if(entrada->pid == pid && entrada->pagina == pagina) {
            log_info(logger_CPU, "PID: %d - TLB HIT - Pagina: %d", pid, pagina);
            entrada->timestamps = tiempoEnMilisecs();
            return entrada->marco;
        }
    }
    log_info(logger_CPU, "PID: %d - TLB MISS - Pagina: %d", pid, pagina);
    return -1;
}

void agregar_frame_en_TLB(int pid, int pagina, int frame) {
    t_entrada_tlb* entrada = malloc(sizeof(t_entrada_tlb));
    
    entrada->pid = pid;
    entrada->pagina = pagina;
    entrada->marco = frame;
    entrada->timestamps = tiempoEnMilisecs();

    int cantidad_entradas = config_get_int_value(config_CPU, "CANTIDAD_ENTRADAS_TLB");
    printf("La cantidad de entradas es %d\n", cantidad_entradas);

    if(cantidad_entradas > 0) { // Esta habilitada la TLB?
        if(list_size(tlb) == cantidad_entradas) { // Esta completa la TLB ??
            eliminar_victima_TLB();
        }
        list_add(tlb, entrada);
    } else {
        free(entrada);
    }

    imprimir_tlb();
    // Si la cantidad de entradas es 0, no hace nada
}

void eliminar_victima_TLB() {
    char* algoritmo_tlb = config_get_string_value(config_CPU, "ALGORITMO_TLB");
    t_entrada_tlb* entrada_victima; // NO se le hace malloc porque ya le hicimos malloc cuando entro en la lista

    if(strcmp(algoritmo_tlb, "FIFO") == 0) {
        entrada_victima = list_get(tlb, 0);
    } else { // ES LRU
        entrada_victima = (t_entrada_tlb*) list_get_minimum(tlb, (void*) _timestamp_menor_de_entrada);
    }

    printf("TLB elimina victima - PID: %d - Pagina: %d", entrada_victima->pid, entrada_victima->pagina);
    
    list_remove_element(tlb, entrada_victima);
    free(entrada_victima);
    // free(algoritmo_tlb);
}


void* _timestamp_menor_de_entrada(t_entrada_tlb* entrada1, t_entrada_tlb* entrada2) {
    return entrada1->timestamps <= entrada2->timestamps ? entrada1 : entrada2;
}

long tiempoEnMilisecs() {
    struct timeval time;
    gettimeofday(&time, NULL);

    return time.tv_sec * 1000 + time.tv_usec / 1000;
}
/*
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

void vaciar_tlb() {
	
}

void imprimir_tlb() {
    printf("\nVoy a imprimir el TLB \n");
    for(int i=0;i < list_size(tlb);i++){
        t_entrada_tlb* entrada = list_get(tlb,i);
        printf("Pid: %d\n", entrada->pid);
        printf("La pagina es: %d\n", entrada->pagina);
        printf("El marco: %d\n", entrada->marco);
        printf("El timestamp: %ld\n\n", entrada->timestamps);
    }
    printf("TLB mostrada :D \n\n");
}