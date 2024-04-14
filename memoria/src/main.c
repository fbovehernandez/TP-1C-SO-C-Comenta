#include "../include/main.h"

int main(int argc, char* argv[]) {
    pthread_t conexion_kernel, conexion_cpu;

    t_config* config_memoria = iniciar_config("./memoria.config");
    t_log* logger_memoria = iniciar_logger("memoria.log");

    char* puerto_kernel = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");

    int escucha_fd = iniciar_servidor(puerto_kernel);
    log_info(logger_memoria, "Servidor iniciado, esperando conexion de kernel");

    t_config_memoria* memory_struct = malloc(sizeof(t_config_memoria));
    memory_struct->socket = escucha_fd;
    memory_struct->logger = logger_memoria;

    pthread_create(&conexion_kernel, NULL, (void*)manejar_conexion_kernel , (void*)memory_struct);
    pthread_create(&conexion_cpu, NULL, (void*)manejar_conexion_cpu , (void*)memory_struct); // Ver si hay problemas con el pasar el msimo struct
    
    // Aca lo dejo como un join porque no se en que momento el server deberia dejar de escuchar peticiones
    pthread_join(conexion_kernel, NULL);
    pthread_join(conexion_cpu, NULL);
    // pthread_detach(conexion_kernel);
    free(config_memoria);
    return 0;
}