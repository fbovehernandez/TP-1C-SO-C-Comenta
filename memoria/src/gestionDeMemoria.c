#include "../include/gestionDeMemoria.h"

Memoria inicializarMemoria(t_config* config_memoria) {
    Memoria mem;

    int tamanioMemoria = config_get_int_value(config_memoria,"TAM_MEMORIA");
    int tamanioPagina  = config_get_int_value(config_memoria,"TAM_MEMORIA");

    mem.tamanioMemoria = tamanioMemoria;
    mem.memoria = malloc(tamanioMemoria);
    
    // Inicializar la tabla de páginas
    int numPaginas = *(int*) tamanioMemoria / tamanioPagina;


    mem.tablaPaginas.numPaginas = numPaginas;
    mem.tablaPaginas.paginas = malloc(numPaginas * sizeof(Pagina));
    
    // NO creo que haya otra opcion que usar vectores para que la paginacion sea contigua
    for (int i = 0; i < numPaginas; i++) {
        mem.tablaPaginas.paginas[i].tamanio = tamanioPagina;
        mem.tablaPaginas.paginas[i].valido = true;
        mem.tablaPaginas.paginas[i].datos = NULL;
    }
    
    return mem;
}


