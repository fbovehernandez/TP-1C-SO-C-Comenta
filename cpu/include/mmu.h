#ifndef MMU_H
#define MMU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include "conexionesCPU.h"

extern t_log* logger_CPU;
extern t_list* tlb;

/////////////////////
///// FUNCIONES /////
/////////////////////

// void inicializar_tlb();
int buscar_frame_en_TLB(int pid, int pagina);
void agregar_frame_en_TLB(int pid, int pagina, int frame);
void eliminar_victima_TLB();
void* _timestamp_menor_de_entrada(t_entrada_tlb* entrada1, t_entrada_tlb* entrada2);
long tiempoEnMilisecs();
int traducir_direccion_logica_a_fisica(uint32_t direccion_logica, int pid);
void pedir_frame_a_memoria(int nro_pagina, int pid);
void vaciar_tlb();
void imprimir_tlb();

//void* crear_tlb();
//void guardar_en_TLB(int pid, int nro_pagina, int nro_marco);
//void algoritmo_LRU(t_tlb* nueva_entrada);
//int esta_en_TLB(int nro_pagina);
// int desplazamiento_memoria(int direccion_logica, int nro_pagina);

#endif