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

void escuchar_dispatcher(t_config* config_CPU, t_log* logger_CPU);
void escuchar_interrupt(t_config* config_CPU, t_log* logger_CPU);
int conectar_memoria(char* ip, char* puerto, t_log* logger_CPU);
int esperar_clientes_cpu(int socket_escucha_dispatch, int socket_escucha_interrupt, t_log* logger_CPU);
int esperar_cliente(int socket_escucha, t_log* logger_CPU);
void* handle_dispatch(void* socket_dispatch);
void* handle_interrupt(void* socket_interrupt);
int recibir_operacion(int socket_client);
void* iniciar_servidor_dispatch(void* datos_dispatch);

#endif // CONEXIONESCPU_H