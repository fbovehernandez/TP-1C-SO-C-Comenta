#ifndef FUNCIONALIDADES_H
#define FUNCIONALIDADES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<semaphore.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
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
    int instrucciones;
    enum Estado estadoActual;
    enum Estado estadoAnterior;
    Registros registros;
} t_pcb;

#endif