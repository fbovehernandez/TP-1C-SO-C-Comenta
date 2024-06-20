#ifndef MMU_H
#define MMU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include "conexionesCPU.h"

extern t_dictionary* tlb;

void inicializar_tlb(t_config* config_CPU);

//void* crear_tlb();
long tiempoEnMilisecs() ;
void guardar_en_TLB(int pid, int nro_pagina, int nro_marco);
void algoritmo_LRU(t_tlb* nueva_entrada);
int esta_en_TLB(int nro_pagina);
int traducir_direccion_logica_a_fisica(uint32_t direccion_logica, int pid);
int desplazamiento_memoria(int direccion_logica, int nro_pagina);
void vaciar_tlb();
void pedir_frame_a_memoria(int nro_pagina, int pid);

#endif