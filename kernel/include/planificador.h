#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

void pasar_a_ready(t_pcb *pcb, Estado estadoAnterior);
void pasar_a_exec(t_pcb* pcb);
void pasar_a_blocked(t_pcb* pcb, Estado estadoAnterior);
void pasar_a_exit(t_pcb* pcb);
void change_status(t_pcb* pcb, Estado new_status);

#endif