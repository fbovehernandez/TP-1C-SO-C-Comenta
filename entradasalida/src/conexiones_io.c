#include "../include/conexiones_io.h"

/*
void gestionar_STDOUT(t_config* config_io, t_log* logger_io) {
    // char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    char* ip_kernel = config_get_string_value(config_io, "IP_KERNEL_IO");
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL_IO");

    printf("IP_IO: %s\n", ip_kernel);
    printf("PUERTO_IO_STDOUT: %s\n", puerto_kernel);

    // El handshake de stdout es: 45
    int stdout_fd = crear_conexion(ip_kernel, puerto_kernel, 45 Handshake inventado);
    log_info(logger_io, "Conexion establecida con kernel desde IO STDOUT"); 
    sendMessage(stdout_fd);
    close(stdout_fd); // Si no quisiese que se desconecte, se puede comentar
}

void gestionar_STDIN(t_config *config_io){

}

void gestionar_GENERICA(t_config *config_io){

}

void gestionar_DIALFS(t_config *config_io) {

}
*/

int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io) {
    // char* IP_MEMORIA = config_get_string_value(config_kernel, "IP_MEMORIA");
    // char* puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
    int message_io = 12; //nro de codop
    int valor = 5; //handshake

    int kernelfd = crear_conexion(IP_KERNEL, puerto_kernel, valor);
    log_info(logger_io, "Conexion establecida con Kernel");

    send(kernelfd, &message_io, sizeof(int), 0); 

    close(kernelfd); // (POSIBLE) no cierro el socket porque quiero reutilizar la conexion
    return kernelfd;
}