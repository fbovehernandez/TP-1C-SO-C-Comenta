#include "../include/conexiones.h"

/* 
PUERTO_ESCUCHA_KERNEL_CPU=8010
IP_CPU=127.0.0.1
PUERTO_CPU_DISPATCH=8006
PUERTO_CPU_INTERRUPT=8007
IP_MEMORIA=127.0.0.1
PUERTO_ESCUCHA_KERNEL_MEMORIA=8009
PUERTO_MEMORIA=8002
ALGORITMO_PLANIFICACION=VRR
QUANTUM=2000
RECURSOS=[RA,RB,RC]
INSTANCIAS_RECURSOS=[1,2,1]
GRADO_MULTIPROGRAMACION=10
*/

void conectar_kernel_cpu_dispatch(t_config* config_kernel, t_log* logger_kernel, char* IP_CPU) {
    // char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    char* puerto_dispatcher = config_get_string_value(config_kernel, "PUERTO_CPU_DISPATCH");

    printf("IP_CPU: %s\n", IP_CPU);
    printf("PUERTO_CPU_DISPATCH: %s\n", puerto_dispatcher);

    int dispatcherfd = crear_conexion(IP_CPU, puerto_dispatcher);
    log_info(logger_kernel, "Conexion establecida con Dispatcher");
    sendMessage(dispatcherfd);
    close(dispatcherfd);
} 


void conectar_kernel_cpu_interrupt(t_config* config_kernel, t_log* logger_kernel, char* IP_CPU) {
    // char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    char* puerto_interrupt = config_get_string_value(config_kernel, "PUERTO_CPU_INTERRUPT");

    printf("IP_CPU: %s\n", IP_CPU);
    printf("PUERTO_CPU_INTERRUPT: %s\n", puerto_interrupt);

    int interruptfd = crear_conexion(IP_CPU, puerto_interrupt);
    log_info(logger_kernel, "Conexion establecida con Interrupt");
    sendMessage(interruptfd);
    close(interruptfd);
}

void conectar_kernel_memoria(t_config* config_kernel, t_log* logger_kernel) {
    char* IP_MEMORIA = config_get_string_value(config_kernel, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");

    printf("IP_MEMORIA: %s\n", IP_MEMORIA);
    printf("PUERTO_MEMORIA: %s\n", puerto_memoria);

    int memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria);
    log_info(logger_kernel, "Conexion establecida con Memoria");
    sendMessage(memoriafd);
    close(memoriafd);
}

void escuchar_IO(t_config* config_kernel, t_log* logger_kernel) {
    char* escucha_IO = config_get_string_value(config_kernel, "PUERTO_IO"); // 8009
    int kernelfd = iniciar_servidor(escucha_IO);
    log_info(logger_kernel, "Servidor iniciado, esperando conexion de I/O");
    printf("Escucha I/O: %s\n", escucha_IO);
    int io_fd = esperar_conexion(kernelfd);

    receiveMessagex(io_fd);
    close(io_fd);
}