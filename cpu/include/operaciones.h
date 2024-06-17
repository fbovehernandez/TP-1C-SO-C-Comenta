#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include "conexionesCPU.h"
#include "mmu.h"

extern int hay_interrupcion;
extern t_log* logger;

typedef struct {
    int cantidad;
} t_cantidad_instrucciones;

int cantidad_de_paginas_a_leer(uint32_t direccion_logica, int tamanio_en_bytes, int pagina);
int tamanio_byte_registro(bool es_registro_uint8);
int recibir_resultado_resize();
void enviar_tamanio_memoria(int nuevo_tamanio, int pid);
void ejecutar_pcb(t_pcb* pcb, int socket_memoria);
void pedir_cantidad_instrucciones_a_memoria(int pid, int socket_memoria);
void enviar_direcciones_fisicas(int pagina, int cant_paginas, int pid, uint32_t *registro_direccion_1, bool es_registro_uint8_dato);
void escribir_direccion_fisica_a_mem(int direccion_fisica, bool es_registro_uint8_dato, uint32_t valor);
int check_interrupt(t_pcb* pcb);
int cantidad_instrucciones_deserializar(t_buffer *buffer);
void pedir_instruccion_a_memoria(int socket_memoria, t_pcb *pcb);
t_buffer* llenar_buffer_solicitud_instruccion(t_solicitud_instruccion* solicitud_instruccion);
t_paquete* recibir_memoria(int socket_memoria);
int recibir_cantidad_instrucciones(int socket_memoria, int pid);
t_instruccion* recibir_instruccion(int socket_memoria, t_pcb *pcb);
t_instruccion* instruccion_deserializar(t_buffer* buffer);
int ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb);
int enviar_primer_pagina(uint32_t direccion_logica, int pid, bool es_registro_uint8_dato);
void desalojar(t_pcb* pcb, DesalojoCpu motivo, t_buffer* add);
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
void mandar_direccion_fisica_a_mem(int direccion_fisica, bool es_de_8_bits, codigo_operacion codigo_operacion);
//void sum(void* registro,void* valor, bool es_8_bits, t_pcb* pcb);
void* sacarParametroParaConseguirRegistro(t_list* list_parametros,bool es_registro_uint8);
t_buffer* llenar_buffer_copy_string(int direccion_fisica_SI, int direccion_fisica_DI, int tamanio);
void pedir_todos_los_marcos(int pagina, int cant_paginas, int pid, uint32_t *registro_direccion_1, bool es_registro_uint8_dato);
void enviar_buffer_copy_string(int direccion_fisica_SI, int direccion_fisica_DI, int tamanio);
void manejar_recursos(t_pcb* pcb, t_parametro* recurso, DesalojoCpu codigo);
void pedir_lectura(char* interfaz, int direccion_fisica, uint32_t* registro_tamanio);
void enviar_kernel_stdout(char* nombre_interfaz, int direccion_fisica, uint32_t tamanio);
t_buffer* llenar_buffer_stdout(int direccion_fisica,char* nombre_interfaz, uint32_t tamanio);

#endif 