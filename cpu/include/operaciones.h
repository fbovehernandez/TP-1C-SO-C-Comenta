#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include "conexionesCPU.h"

extern int hay_interrupcion;

typedef struct {
    int cantidad;
} t_cantidad_instrucciones;

void ejecutar_pcb(t_pcb* pcb, int socket_memoria);
void pedir_cantidad_instrucciones_a_memoria(int pid, int socket_memoria);
int cantidad_instrucciones_deserializar(t_buffer *buffer);
void pedir_instruccion_a_memoria(int socket_memoria, t_pcb *pcb);
t_buffer* llenar_buffer_solicitud_instruccion(t_solicitud_instruccion* solicitud_instruccion);
void recibir(int socket_memoria, t_pcb *pcb);
t_instruccion* instruccion_deserializar(t_buffer* buffer);
void ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb);
void desalojar(t_pcb* pcb, DesalojoCpu motivo, void* add, int size);
void solicitud_dormirIO_kernel(char* interfaz, int unidades);
t_buffer* llenar_buffer_dormir_IO(char* interfaz, int unidades);
void enviar_fin_programa(t_pcb *pcb);
void guardar_estado(t_pcb *pcb);
void setear_registros_cpu();
void* seleccionar_registro_cpu(char* nombreRegistro);
bool es_de_8_bits(char* registro);
void set(void *registro, uint32_t valor, bool es_8_bits);
void sub(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino);
void sum(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino);
void jnz(void* registro, int valor, t_pcb* pcb);
bool sonTodosDigitosDe(char *palabra);
//void sum(void* registro,void* valor, bool es_8_bits, t_pcb* pcb);

extern t_log* logger;

#endif 