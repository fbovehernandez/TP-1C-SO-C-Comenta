#ifndef CONEXIONESCPU_H
#define CONEXIONESCPU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

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

int conectar_memoria(char* ip, char* puerto, t_log* logger_CPU);
int esperar_cliente(int socket_escucha, t_log* logger_CPU);
void* handle_dispatch(void* socket_dispatch);
void* handle_interrupt(void* socket_interrupt);
int recibir_operacion(int socket_client);
void* iniciar_servidor_dispatch(void* datos_dispatch);
void* iniciar_servidor_interrupt(void* datos_interrupt);
t_config_cpu* iniciar_datos(char* escucha_fd, t_log* logger_CPU);
t_pcb* pcb_deserializar(t_buffer* buffer);

#endif // CONEXIONESCPU_H