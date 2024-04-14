#include "../include/conexiones_io.h"

void gestionar_STDOUT(t_config* config_io, t_log* logger_io) {
    // char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    char* ip_kernel = config_get_string_value(config_io, "IP_KERNEL_IO");
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL_IO");

    printf("IP_IO: %s\n", ip_kernel);
    printf("PUERTO_IO_STDOUT: %s\n", puerto_kernel);

    int stdout_fd = crear_conexion(ip_kernel, puerto_kernel);
    log_info(logger_io, "Conexion establecida con kernel desde IO STDOUT"); 
    sendMessage(stdout_fd);
    close(stdout_fd); // Si no quiesese que se desconecte, se puede comentar
}