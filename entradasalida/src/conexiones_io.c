#include "../include/conexiones_io.h"

void conectar_kernel_io(t_config* config_io, t_log* logger_io, char* IP_IO) {
    // char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL_IO");

    printf("IP_IO: %s\n", IP_IO);
    printf("PUERTO_IO_INTERRUPT: %s\n", puerto_kernel);

    int io_kernel_fd = crear_conexion(IP_IO, puerto_kernel);
    log_info(logger_io, "Conexion establecida con KERNEL");
    sendMessage(io_kernel_fd);
    close(io_kernel_fd);
}