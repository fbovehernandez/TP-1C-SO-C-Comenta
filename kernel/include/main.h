#ifndef KERNEL_MAIN_H_
#define KERNEL_MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "conexiones.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

/*
PID: Identificador del proceso (deberá ser un número entero, único en todo el sistema).
Program Counter: Número de la próxima instrucción a ejecutar.
Quantum: Unidad de tiempo utilizada por el algoritmo de planificación VRR.
Registros de la CPU: Estructura que contendrá los valores de los registros de uso general de la CPU.
*/

typedef struct {
    uint32_t  PC;

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

typedef struct {
    int pid;
    int program_counter;
    int quantum;
    Registro registro[11];
} PCB;

#endif // KERNEL_MAIN_H_