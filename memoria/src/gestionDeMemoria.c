#include "../include/gestionDeMemoria.h"

Memoria inicializarMemoria(t_config* config_memoria) {
    Memoria mem;

    int tamanioMemoria = config_get_int_value(config_memoria,"TAM_MEMORIA");
    int tamanioPagina  = config_get_int_value(config_memoria,"TAM_PAGINA");

    mem.tamanioMemoria = tamanioMemoria;
    mem.memoria = malloc(tamanioMemoria);
    
    // Inicializar la tabla de páginas
    int numPaginas = tamanioMemoria / tamanioPagina;

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

int recibir_solicitud_frame(int PID, int nroPagina) {
    t_list* tablaPaginas = dictionary_get(diccionario_tablas_paginas, string_itoa(PID)); // hacer ghlobal la tabla de paginas
    int frame = list_get(tablaPaginas, nroPagina);
    return frame;

    // Buscar en la tabla de páginas del PID la página nroPagina
    // Voy a considerar de que no puede recibir un frame que no existe
}