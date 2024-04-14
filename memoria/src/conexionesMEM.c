#include "../include/conexionesMEM.h"

void* manejar_conexion_kernel(void* config_memoria) {
    t_config_memoria* config_memory = (t_config_memoria*)config_memoria;

    while(1) {
        int kernel_memoria = esperar_conexion(config_memory->socket);
        log_info(config_memory->logger, "Conexion establecida con kernel");
        receiveMessagex(kernel_memoria); // Recibo mensjae -> Cambiar por handshake
        close(kernel_memoria);
    }
}

// En algun punto supongo que deberia pasar esta funcion a otro .c
void* manejar_conexion_cpu(void* config_memoria) {
    t_config_memoria* config_memory = (t_config_memoria*)config_memoria;

    while(1) {
        int cpu_memoria = esperar_conexion(config_memory->socket);
        log_info(config_memory->logger, "Conexion establecida con cpu");
        receiveMessagex(cpu_memoria); // Recibo mensjae -> Cambiar por handshake
        close(cpu_memoria);
    }
}