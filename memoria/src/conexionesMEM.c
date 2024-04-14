#include "../include/conexionesMEM.h"

void* manejar_conexion_kernel(void* config_memoria) {
    t_config_memoria* config_memory = (t_config_memoria*)config_memoria;
    char* buffer = malloc(10);

    while(1) {
        int kernel_memoria = esperar_conexion(config_memory->socket);
        log_info(config_memory->logger, "Conexion establecida con kernel");
        
        recv(kernel_memoria, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, "KERNEL") == 0) {
            send(kernel_memoria, "MEMORIA", 7, 0); // len("MEMORIA")
        } else {
            send(kernel_memoria, "ERROR", 6, 0); // len("ERROR")
        }
        
        close(kernel_memoria);
    }
}

// En algun punto supongo que deberia pasar esta funcion a otro .c
void* manejar_conexion_cpu(void* config_memoria) {
    t_config_memoria* config_memory = (t_config_memoria*)config_memoria;
    char* buffer = malloc(10);

    while(1) {
        int cpu_memoria = esperar_conexion(config_memory->socket);
        // La idea aca seria implementar algo tipo, bueno me das tu cod-op y veo que operacion realizo si en handshake dio bien
        log_info(config_memory->logger, "Conexion establecida con cpu");

        recv(cpu_memoria, buffer, sizeof(buffer), 0); 
        if(strcmp(buffer, "CPU") == 0) {
            send(cpu_memoria, "MEMORIA", 7, 0); // len("MEMORIA")
        } else {
            send(cpu_memoria, "ERROR", 6, 0); // len("ERROR")
        }
        close(cpu_memoria);
    }
}