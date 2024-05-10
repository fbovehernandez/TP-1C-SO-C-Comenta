#ifndef FUNCIONALIDADES_H
#define FUNCIONALIDADES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "conexiones.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/collections/queue.h>
#include <pthread.h>

typedef struct {
    
} t_instrucciones;

//Me lleve t_pcb al utils, lo mismo que Registros y Estado

// Los 2 de abajo no los usamos todavia
typedef struct {
    int socket_consola;
    int pid;
    int tabla_paginas;
    int tamanio_en_memoria;
} t_proceso;

/*
typedef struct {
	t_link_element *head;
	int elements_count;
} t_list;
*/

//Si no podemos usar bien las variables globales, va a tener que pasar por consola
/*typedef struct {
    int contador_pid;
    int quantum;
    t_config* config_kernel;
    t_log* logger_kernel;
    int socket_memoria_kernel; // var global
    int socket_cpu; // var global
} t_consola;*/

extern int grado_multiprogramacion;
extern int quantum;
extern int contador_pid;  

extern pthread_mutex_t mutex_estado_new;
extern pthread_mutex_t mutex_estado_ready;
extern pthread_mutex_t mutex_estado_exec;
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_hay_pcb_esperando_ready;
extern sem_t sem_hay_para_planificar;
extern sem_t sem_contador_quantum;

extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_blocked;
extern t_queue* cola_exec;
extern t_queue* cola_exit;

extern char* algoritmo_planificacion; // Tomamos en convencion que los algoritmos son "FIFO", "VRR" , "RR" (siempre en mayuscula)
extern t_log* logger_kernel;

// Funciones
void* interaccion_consola(t_sockets* sockets);
void encolar_a_new(t_pcb* pcb);
void* a_ready(); // Para hilo 
void pasar_a_ready(t_pcb *pcb);
t_pcb* crear_nuevo_pcb(int pid);
void* contar_quantum(void* socket_CPU);
Registros* inicializar_registros_cpu();
void print_queue(enum Estado estado);
int obtener_siguiente_pid();
// t_queue* mostrar_cola(t_queue* cola);
void mostrar_pcb_proceso(t_pcb* pcb);
void* planificar_corto_plazo(void* sockets_kernel);
void* enviar_path_a_memoria(char* path, t_sockets* sockets, int pid);
void* enviar_pcb(t_pcb* pcb, int socket);
t_buffer* llenar_buffer_path(t_path* pathNuevo);
t_buffer* llenar_buffer_pcb(t_pcb* pcb);
t_pcb* proximo_a_ejecutar();
void* pasar_a_exec(t_pcb* pcb);
int esperar_cpu(t_pcb* pcb);

void EJECUTAR_SCRIPT();
void INICIAR_PROCESO(char* path, t_sockets* sockets);
void FINALIZAR_PROCESO();
void INICIAR_PLANIFICACION();
void DETENER_PLANIFICACION();
void PROCESO_ESTADO();
void MULTIPROGRAMACION();

#endif