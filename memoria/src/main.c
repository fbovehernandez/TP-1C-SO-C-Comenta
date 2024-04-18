#include "../include/main.h"

int main(int argc, char* argv[]) {

    // Creo los configs y logs
    t_config* config_memoria = iniciar_config("./memoria.config");
    t_log* logger_memoria = iniciar_logger("memoria.log");

    char* puerto_escucha = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");

    // Levanto el servidor y devuelvo el socket de escucha
    int escucha_fd = iniciar_servidor(puerto_escucha);

    log_info(logger_memoria, "Servidor iniciado, esperando conexiones!");

    // conecto y recibo handshake de cpu
    // int cliente_cpu = esperar_cliente(escucha_fd, logger_memoria, "CPU"); 

    t_config_memoria* memory_struct_cpu = malloc(sizeof(t_config_memoria));
    memory_struct_cpu->socket = escucha_fd;
    memory_struct_cpu->logger = logger_memoria;

    while(1) {
        int socket_cliente = esperar_cliente(escucha_fd, logger_memoria);
    }
   
    /* 
    int cliente_kernel = esperar_cliente(escucha_fd, logger_memoria, "KERNEL"); // Aca va el handshake con el Kernel
    
    t_config_memoria* memory_struct_kernel = malloc(sizeof(t_config_memoria));
    memory_struct_kernel->socket = cliente_kernel;
    memory_struct_kernel->logger = logger_memoria;

    pthread_create(&escucha_kernel, NULL, (void*)atender_kernel , (void*)memory_struct_kernel);
    
    pthread_join(escucha_kernel, NULL);
    */
   
    // pthread_join(escucha_cpu, NULL);

    // Libero conexiones 
    free(config_memoria);
    free(memory_struct_cpu);
    // free(memory_struct_kernel);
    liberar_conexion(escucha_fd);
    // liberar_conexion(cliente_kernel);

    return 0;
}