#include "../include/conexionesCPU.h"

/* 

PUERTO_KERNEL=8010
IP_KERNEL=127.0.0.1
PUERTO_ESCUCHA_DISPATCH=8006
PUERTO_ESCUCHA_INTERRUPT=8007
IP_MEMORIA=127.0.0.1
PUERTO_MEMORIA=8002
CANTIDAD_ENTRADAS_TLB=32
ALGORITMO_TLB=FIFO

*/

void escuchar_dispatcher(t_config* config_CPU, t_log* logger_CPU) {
    char* escucha_dispatcher = config_get_string_value(config_CPU, "PUERTO_ESCUCHA_DISPATCH"); // 8006
    int dispatcherfd = iniciar_servidor(escucha_dispatcher);
    log_info(logger_CPU, "Servidor iniciado, esperando conexion de Dispatcher");
    printf("Escucha dispatcher: %s\n", escucha_dispatcher);
    int kernel_dispatcher = esperar_conexion(dispatcherfd);

    receiveMessagex(kernel_dispatcher);
    close(kernel_dispatcher);
}

void escuchar_interrupt(t_config* config_CPU, t_log* logger_CPU) {
    char* escucha_interrupt = config_get_string_value(config_CPU, "PUERTO_ESCUCHA_INTERRUPT"); // 8007
    int interruptfd = iniciar_servidor(escucha_interrupt);
    log_info(logger_CPU, "Servidor iniciado, esperando conexion de Interrupt");
    printf("Escucha interrupt: %s\n", escucha_interrupt);
    int kernel_interrupt = esperar_conexion(interruptfd);

    receiveMessagex(kernel_interrupt);
    close(kernel_interrupt);
}

/* 
int conectar_kernel_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_kernel) {
    // char* IP_MEMORIA = config_get_string_value(config_kernel, "IP_MEMORIA");
    // char* puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");

    printf("IP_MEMORIA: %s\n", IP_MEMORIA);
    printf("PUERTO_MEMORIA: %s\n", puerto_memoria);

    int memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria);
    log_info(logger_kernel, "Conexion establecida con Memoria");

    sendMessage(memoriafd);

    close(memoriafd);
    return memoriafd;
}*/

int conectar_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_CPU) {
    // char* IP_MEMORIA = config_get_string_value(config_CPU, "IP_MEMORIA");
    // char* puerto_memoria = config_get_string_value(config_CPU, "PUERTO_MEMORIA");

    printf("IP_MEMORIA: %s\n", IP_MEMORIA);
    printf("PUERTO_MEMORIA: %s\n", puerto_memoria);

    int memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria);
    log_info(logger_CPU, "Conexion establecida con Memoria");

    sendMessage(memoriafd);

    close(memoriafd);
    return memoriafd;
}