#ifndef FUNCIONALIDADES_H
#define FUNCIONALIDADES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/collections/queue.h>
#include <pthread.h>

typedef struct {

    // Registros de proposito gral.  (1 byte) 
    uint8_t   AX;
    uint8_t   BX; 
    uint8_t   CX;
    uint8_t   DX;

    // Registros de proposito gral. (4 bytes) 
    uint32_t  EAX;
    uint32_t  EBX; 
    uint32_t  ECX;
    uint32_t  EDX;

    uint32_t  SI; // Contiene la dirección lógica de memoria de origen desde donde se va a copiar un string.
    uint32_t  DI; // Contiene la dirección lógica de memoria de destino a donde se va a copiar un string.

} Registros;

enum Estado {
    NEW,
    READY,
    BLOCKED,
    EXEC,
    EXIT
};

typedef struct {
    int pid;
    int program_counter;
    int quantum;
    enum Estado estadoActual;
    enum Estado estadoAnterior;
    int socketProceso;
    Registros* registros;
} t_pcb;

// Los 2 de abajo no los usamos todavia
typedef struct {
    int socket_consola;
    int pid;
    int tabla_paginas;
    int tamanio_en_memoria;
} t_proceso;

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
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_hay_pcb_esperando_ready;

extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_blocked;
extern t_queue* cola_exec;
extern t_queue* cola_exit;

extern char* algoritmo_planificacion; // Tomamos en convencion que los algoritmos son "FIFO", "VRR" , "RR" (siempre en mayuscula)
extern t_log* logger_kernel;

// Funciones
void* interaccion_consola();
void encolar_a_new(t_pcb* pcb);
void* a_ready(); // Para hilo 
void pasar_a_ready(t_pcb *pcb);
t_pcb* crear_nuevo_pcb(int pid);
Registros* inicializar_registros_cpu();
void print_queue(enum Estado estado);
int obtener_siguiente_pid();
// t_queue* mostrar_cola(t_queue* cola);
void mostrar_pcb_proceso(t_pcb* pcb);

void EJECUTAR_SCRIPT();
void INICIAR_PROCESO();
void FINALIZAR_PROCESO();
void INICIAR_PLANIFICACION();
void DETENER_PLANIFICACION();
void PROCESO_ESTADO();
void MULTIPROGRAMACION();

#endif