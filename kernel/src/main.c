#include "../include/main.h"

int main(int argc, char* argv[]) {
    int messagex = 10;
    
    // Implementar ptr para estos valores...
    t_config* config_kernel   = iniciar_config("./kernel.config");
    t_log* logger_kernel = iniciar_logger("kernel.log");

    char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");

    // Hilo 1 -> Hacer un hilo para gestionar la comunicacion con memoria?
    char* IP_MEMORIA = config_get_string_value(config_kernel, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");

    int socket_memoria_kernel = conectar_kernel_memoria(IP_MEMORIA, puerto_memoria, logger_kernel);
    
    sleep(5);// -> Pequeño retardo para probarlo 
    // close(socket_memoria_kernel); -> Cerrar mas abajo

    // Hilo 2 -> Hacer un hilo para gestionar comunicacion con la cpu?
    char* cpu_dispatch = config_get_string_value(config_kernel, "PUERTO_CPU_DISPATCH");
    char* cpu_interrupt = config_get_string_value(config_kernel, "PUERTO_CPU_INTERRUPT");

    int socket_cpu = conectar_kernel_cpu_dispatch(logger_kernel, IP_CPU, cpu_dispatch);
    
    sleep(4); // Idem arriba
    send(socket_cpu, &messagex, sizeof(int), 0);

    // Hilo 3 -> Un hilo x cada interfaz
    char* puerto_escucha = config_get_string_value(config_kernel, "PUERTO_IO");

    int escucha_fd = iniciar_servidor(puerto_escucha);
    log_info(logger_kernel, "Servidor iniciado, esperando conexiones!");

    t_config_kernel* kernel_struct_io = malloc(sizeof(t_config_kernel));
    kernel_struct_io->socket = escucha_fd;
    kernel_struct_io->logger = logger_kernel;

    pthread_t hilo_io;
    pthread_create(&hilo_io, NULL, (void*) escuchar_IO, (void*) kernel_struct_io);
    pthread_join(hilo_io, NULL);

    free(config_kernel);
    free(kernel_struct_io);
    liberar_conexion(escucha_fd);   

    return 0;
}