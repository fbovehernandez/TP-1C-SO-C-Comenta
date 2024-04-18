#include "../include/main.h"

int main(int argc, char* argv[]) {
    int messagex = 10;
    t_config* config_kernel   = iniciar_config("./kernel.config");
    t_log* logger_kernel = iniciar_logger("kernel.log"); 

    char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    // conectar_kernel_cpu_dispatch(config_kernel, logger_kernel, IP_CPU); // agarra el puerto que escucha la CPU y se conecta a la CPU
    // conectar_kernel_cpu_interrupt(config_kernel, logger_kernel, IP_CPU);

    // Hilo 1
    char* IP_MEMORIA = config_get_string_value(config_kernel, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");

    int socket_memoria_kernel = conectar_kernel_memoria(IP_MEMORIA, puerto_memoria, logger_kernel);
    
    sleep(5);
    close(socket_memoria_kernel);

    // Hilo 2
    char* cpu_dispatch = config_get_string_value(config_kernel, "PUERTO_CPU_DISPATCH");
    char* cpu_interrupt = config_get_string_value(config_kernel, "PUERTO_CPU_INTERRUPT");

    int socket_cpu = conectar_kernel_cpu_dispatch(logger_kernel, IP_CPU, cpu_dispatch);
    sleep(4);
    send(socket_cpu, &messagex, sizeof(int), 0);
    
    // close(socket_cpu);
    // Hilo servidor -> Hilo 3
    /* 
    pthread_t servidor;
    t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
    args->log = logger;
    args->fd = cliente_socket;
    args->server_name = server_name;
    pthread_create(&servidor, NULL, (void*) procesar_conexion_servidor, (void*) args);
    pthread_detach(servidor);
    */

    //  Hilo cliente (conexion con la cpu)
    /* 
    pthread_t cliente;
    t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
    args->log = logger;
    args->fd = cliente_socket;
    args->server_name = server_name;
    pthread_create(&cliente, NULL, (void*) procesar_conexion_cliente, (void*) args);
    pthread_detach(cliente);
    */
    // escuchar_conexiones(config_kernel, logger_kernel);
    // escuchar_STDOUT(config_kernel, logger_kernel);
    return 0;
}