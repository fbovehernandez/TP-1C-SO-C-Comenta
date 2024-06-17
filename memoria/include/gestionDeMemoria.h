#ifndef GESTIONDEMEMORIA_H
#define GESTIONDEMEMORIA_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

typedef struct {
    bool valido;
    void *datos; 
    int tamanio;
} Pagina;

typedef struct {
    Pagina *paginas;
    int numPaginas;
} TablaPaginas;

typedef struct {
    void *memoria;
    int tamanioMemoria;
    TablaPaginas tablaPaginas;
} Memoria;

#endif 
