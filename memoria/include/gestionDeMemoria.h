#ifndef GESTIONDEMEMORIA_H
#define GESTIONDEMEMORIA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include "conexionesMEM.h"

typedef struct {
    bool valido; // Esto se hace con el bitmap, creo que es mas facil que lo tenga la pagina
    void *datos; // es el void* en si, no lo tiene que tener la pag
    int tamanio; // esto sale del config, tienen todas el mismo
} Pagina;

typedef struct {
    Pagina *paginas;
    int numPaginas; // es una lista (o un array), con los indices y el list_size sabes el tamaño
} TablaPaginas;

typedef struct {
    void *memoria; // Este Memoria no lo entiendo
    int tamanioMemoria;
    TablaPaginas tablaPaginas;
} Memoria;

#endif 
