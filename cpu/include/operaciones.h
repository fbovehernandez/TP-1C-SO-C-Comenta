#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

void cicloDeInstruccion(t_pcb* pcb);
void fetch(t_pcb* pcb);
void decode(t_pcb* pcb);
void execute(t_pcb* pcb);

void ejectuar_pcb(t_pcb* pcb);
t_instruccion* pedir_instruccion_a_memoria(int socket_memoria, t_pcb* pcb);

extern t_log* logger;

#endif 