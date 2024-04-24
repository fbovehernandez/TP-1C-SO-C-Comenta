#ifndef FUNCIONALIDADES_H
#define FUNCIONALIDADES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<semaphore.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/collections/queue.h>
typedef struct {

    // Registros de proposito gral  (1 byte) 
    uint8_t   AX;
    uint8_t   BX; 
    uint8_t   CX;
    uint8_t   DX;

    // Registros de proposito gral (4 bytes) 
    uint32_t  EAX;
    uint32_t  EBX; 
    uint32_t  ECX;
    uint32_t  EDX;

    uint32_t  SI; // Contiene la dirección lógica de memoria de origen desde donde se va a copiar un string.
    uint32_t  DI; // Contiene la dirección lógica de memoria de destino a donde se va a copiar un string.
} Registro;

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
    int instrucciones;
    enum Estado estadoActual;
    // enum Estado estadoDeFinalizacion;
    enum Estado estadoAnterior;
    // enum Estado procesoBloqueadoOTerminado;
    int socketProceso;
    Registro registros;
} t_pcb;

typedef struct {
    // Es un struct o que es???
} t_proceso;

t_pcb *crear_pcb(uint32_t pid);

sem_t mutex_cola_new;
sem_t mutex_cola_sus_ready;
sem_t mutex_cola_ready;
sem_t cantidad_listas_para_estar_en_ready;
sem_t binario_flag_interrupt;
sem_t mutex_cola_blocked;
sem_t signal_a_io;
sem_t dispositivo_de_io;
sem_t mutex_cola_sus_ready;
sem_t binario_planificador_corto;
sem_t mutex_respuesta_interrupt;

t_queue* cola_new;
t_queue* cola_procesos_sus_ready;
t_queue* cola_ready;
t_queue* cola_io;
t_queue* respuesta_interrupcion;

#endif