#include "../include/main.h"

int main(int argc, char* argv[]) {
    int messagex = 10;
    t_config* config_kernel   = iniciar_config("./kernel.config");
    t_log* logger_kernel = iniciar_logger("kernel.log");

    char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    // conectar_kernel_cpu_dispatch(config_kernel, logger_kernel, IP_CPU); // agarra el puerto que escucha la CPU y se conecta a la CPU
    // conectar_kernel_cpu_interrupt(config_kernel, logger_kernel, IP_CPU);

    // Hilo 1
    // Utilizamos hilos para que no se trabe todo dentro del kernel
    char* IP_MEMORIA = config_get_string_value(config_kernel, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");

    int socket_memoria_kernel = conectar_kernel_memoria(IP_MEMORIA, puerto_memoria, logger_kernel);
    
    sleep(5);
    close(socket_memoria_kernel);

    // Hilo 2
    char* cpu_dispatch = config_get_string_value(config_kernel, "PUERTO_CPU_DISPATCH");
    char* cpu_interrupt = config_get_string_value(config_kernel, "PUERTO_CPU_INTERRUPT");

    int socket_cpu = conectar_kernel_cpu_dispatch(logger_kernel, IP_CPU, cpu_dispatch);
    // int socket_cpu2 = conectar_kernel_cpu_interrupt(logger_kernel, IP_CPU, cpu_interrupt);
    sleep(4);
    send(socket_cpu, &messagex, sizeof(int), 0);

    // Hilo 3
    char* puerto_escucha = config_get_string_value(config_kernel, "PUERTO_IO");

    int escucha_fd = iniciar_servidor(puerto_escucha);
    log_info(logger_kernel, "Servidor iniciado, esperando conexiones!");

    t_config_kernel* kernel_struct_io = malloc(sizeof(t_config_kernel));
    kernel_struct_io->socket = escucha_fd;
    kernel_struct_io->logger = logger_kernel;

    while(1) {
        int socket_cliente = esperar_cliente(escucha_fd, logger_kernel);
    }    

/*
int esperar_cliente(int socket_servidor, t_log *logger_cpu) {
    uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if(handshake == 1) {
		send(socket_cliente, &resultOk, sizeof(uint32_t), 0);
        log_info(logger_cpu, "Se conecto un cliente de dispatch!");
    } else {
        send(socket_cliente, &resultError, sizeof(uint32_t), 0);
    }
    
	return socket_cliente;
}
*/
    //close(socket_cpu);
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
    
    free(config_kernel);
    free(kernel_struct_io);
    // free(memory_struct_kernel);
    liberar_conexion(escucha_fd);   

    return 0;
}