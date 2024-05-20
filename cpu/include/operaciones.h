#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

void ejecutar_pcb(t_pcb* pcb, int socket_memoria);
void pedir_instruccion_a_memoria(int socket_memoria, t_pcb* pcb);
t_buffer* llenar_buffer_solicitud_instruccion(t_solicitud_instruccion* solicitud_instruccion);
void recibir_instruccion(int socket_memoria, t_pcb* pcb);
t_instruccion* instruccion_deserializar(t_buffer* buffer);
void ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb);
void* seleccionar_registro(char* nombreRegistro, t_pcb *pcb);
//bool todosSonDigitosDe(char* valor);
bool es_de_8_bits(char* registro);
void set(void* registro, uint32_t valor, bool es_8_bits);
//void sum(void* registro,void* valor, bool es_8_bits, t_pcb* pcb);

extern t_log* logger;

#endif 