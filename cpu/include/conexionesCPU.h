#ifndef CONEXIONESCPU_H
#define CONEXIONESCPU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include "operaciones.h"

extern int socket_memoria;
extern int client_dispatch;
extern int tamanio_pagina;
extern t_config* config_CPU;

typedef struct {
    char* puerto_escucha;
    t_log* logger;
} t_config_cpu;

typedef struct {
    char* puerto_escucha_dispatch;
    char* puerto_escuchar_interrupt;
    char* ip_cpu;
    char* ip_memoria;
    char* puerto_memoria;
} ptrdata_cpu;

int conectar_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_CPU);
int esperar_cliente(int socket_escucha, t_log* logger_CPU);
void* iniciar_servidor_dispatch(void* datos_dispatch);
void cargar_registros_en_cpu(t_registros* registros_pcb);
void recibir_cliente();
void* iniciar_servidor_interrupt(void* datos_interrupt);
void recibir_cliente_interrupt(int client_interrupt);
t_config_cpu* iniciar_datos(char* escucha_fd, t_log* logger_CPU);

//Todo esto donde esta?
void* handle_dispatch(void* client_dispatch);
void* handle_interrupt(void* socket_interrupt);
int recibir_operacion(int socket_client);
void* recibir_pcb_de_kernel(int socket_kernel);
t_pcb* deserializar_pcb(t_buffer* buffer); //sockets
int recibir_interrupcion(t_buffer* buffer);

#endif // CONEXIONESCPU_H