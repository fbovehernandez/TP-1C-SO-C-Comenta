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

void pasar_a_ready(t_pcb *pcb);
void pasar_a_exec(t_pcb* pcb);
void pasar_a_blocked(t_pcb* pcb);
void pasar_a_exit(t_pcb* pcb);
void change_status(t_pcb* pcb, Estado new_status);
void pasar_a_ready_plus(t_pcb* pcb);
void pasar_a_ready_normal(t_pcb* pcb);
char* obtener_pid_de(t_queue* cola);
char* pasar_a_string_estado(Estado estado);

#endif