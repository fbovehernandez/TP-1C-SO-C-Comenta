#include "../include/main.h"
// Include commons

int main(int argc, char* argv[]) {
    int quantum;
    uint32_t contador_pid;

    t_queue* cola_new;
    t_queue* cola_procesos_sus_ready;
    t_queue* cola_ready;
    t_queue* cola_io;
    t_queue* respuesta_interrupcion;

    cola_new = queue_create();
	cola_procesos_sus_ready = queue_create();
	cola_ready = queue_create();
	cola_io = queue_create();
	respuesta_interrupcion = queue_create();
    
    // Inicializo colas con queue_create;

    interaccion_consola(contador_pid); // -> Iniciar consola interactiva
    
    // int messagex = 10;

    t_config* config_kernel   = iniciar_config("./kernel.config");
    t_log* logger_kernel = iniciar_logger("kernel.log");
    ptr_kernel* datos_kernel = solicitar_datos(config_kernel);

    // Hilo 1 -> Hacer un hilo para gestionar la comunicacion con memoria?
    int socket_memoria_kernel = conectar_kernel_memoria(datos_kernel->ip_mem, datos_kernel->puerto_memoria, logger_kernel);

    // Hilo 2 -> Hacer un hilo para gestionar comunicacion con la cpu?
    int socket_cpu = conectar_kernel_cpu_dispatch(logger_kernel, datos_kernel->ip_cpu, datos_kernel->puerto_cpu_dispatch);
    // enviar_pcb(socket_cpu, logger_kernel, 1); // -> Enviar PCB a CPU

    // Hilo 3 -> Conexion con interfaz I/O
    int escucha_fd = iniciar_servidor(datos_kernel->puerto_io);
    log_info(logger_kernel, "Servidor iniciado, esperando conexiones!");

    // Creo el config para el hilo de I/O
    t_config_kernel* kernel_io = malloc(sizeof(t_config_kernel));
    kernel_io->puerto_escucha = datos_kernel->puerto_io;
    kernel_io->socket = escucha_fd;
    kernel_io->logger = logger_kernel;

    // Levanto hilo para escuchar peticiones I/O
    pthread_t hilo_io;
    pthread_create(&hilo_io, NULL, (void*) escuchar_IO, (void*) kernel_io);
    pthread_join(hilo_io, NULL);

    // Libero conexiones
    free(config_kernel);
    free(kernel_io);
    liberar_conexion(escucha_fd);   

    return 0;
}
