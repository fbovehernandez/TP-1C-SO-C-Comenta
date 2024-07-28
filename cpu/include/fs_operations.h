#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "conexionesCPU.h"
#include "operaciones.h"
#include "mmu.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

t_buffer* llenar_buffer_fs_create_delete(char* nombre_interfaz, char* nombre_archivo);
t_buffer* llenar_buffer_fs_truncate(char* nombre_interfaz_a_truncar, char* nombre_file_truncate, uint32_t registro_truncador);
t_buffer* llenar_buffer_fs_escritura_lectura(char* nombre_interfaz, char* nombre_archivo, uint32_t registro_direccion, uint32_t registro_tamanio, uint32_t registro_puntero, int cantidad_paginas, t_list* direcciones_fisicas);