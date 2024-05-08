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

#endif 