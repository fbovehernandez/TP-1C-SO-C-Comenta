#include "../include/conexionesMEM.h"

void iniciar_servidor_memoria(t_config* config_memoria, t_log* logger) {
    char* puerto_memoria = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");
    int memoriafd = iniciar_servidor(puerto_memoria);
    log_info(logger, "Servidor iniciado, esperando conexion de KERNEL");
    int kernel_memoria = esperar_conexion(memoriafd);
    receiveMessagex(kernel_memoria);
    close(kernel_memoria);
}