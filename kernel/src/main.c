#include "../include/main.h"

int main(int argc, char* argv[]) {
    
    // Inicializo colas con queue_create
    cola_new     = queue_create(); 
    cola_ready   = queue_create();
    cola_blocked = queue_create();
    cola_exec    = queue_create();
    cola_exit    = queue_create();
    
    t_config* config_kernel   = iniciar_config("./kernel.config");
    logger_kernel = iniciar_logger("kernel.log");
    ptr_kernel* datos_kernel = solicitar_datos(config_kernel);

    quantum = datos_kernel->quantum;
    grado_multiprogramacion = datos_kernel->grado_multiprogramacion;
    algoritmo_planificacion = datos_kernel->algoritmo_planificacion;

    pthread_mutex_init(&mutex_estado_new, NULL);
    pthread_mutex_init(&mutex_estado_ready, NULL);
    sem_init(&sem_hay_pcb_esperando_ready, 0, 0);
    sem_init(&sem_grado_multiprogramacion, 0, grado_multiprogramacion); // No testeado

    // Hilo 1 -> Hacer un hilo para gestionar la comunicacion con memoria?
    int socket_memoria_kernel = conectar_kernel_memoria(datos_kernel->ip_mem, datos_kernel->puerto_memoria, logger_kernel);

    // Hilo 2 -> Hacer un hilo para gestionar comunicacion con la cpu?
    int socket_cpu = conectar_kernel_cpu_dispatch(logger_kernel, datos_kernel->ip_cpu, datos_kernel->puerto_cpu_dispatch);

    pthread_t pasar_a_ready;
	pthread_create(&pasar_a_ready, NULL, a_ready, NULL);

    pthread_t planificador_corto_plazo;
    // pthread_create(&planificador_corto_plazo, NULL, (void*)planificar_corto_plazo, NULL); // Definir plani a corto plazo

    /////////////// ---------- ///////////////
    // Hilo 3 -> Conexion con interfaz I/O
    int escucha_fd = iniciar_servidor(datos_kernel->puerto_io);
    log_info(logger_kernel, "Servidor iniciado, esperando conexiones!");

    // Creo el config para el hilo de I/O
    t_config_kernel* kernel_io = malloc(sizeof(t_config_kernel));
    kernel_io->puerto_escucha = datos_kernel->puerto_io;
    kernel_io->socket = escucha_fd;
    kernel_io->logger = logger_kernel;

    t_sockets* sockets = malloc(sizeof(t_sockets));
    sockets->socket_cpu = socket_cpu;
    sockets->socket_memoria = socket_memoria_kernel;

    // Levanto hilo para escuchar peticiones I/O
    pthread_t hilo_io;
    // pthread_create(&hilo_io, NULL, (void*) escuchar_IO, (void*) kernel_io); PARA PROBAR

    // Hilo que guía la ejecución de procesos
    pthread_t hilo_procesos;
    // pthread_create(&hilo_procesos, NULL, (void*) escuchar_IO, (void*) kernel_io);

    pthread_t socket_escucha_consola;
    pthread_create(&socket_escucha_consola, NULL, (void*) interaccion_consola, sockets); 

    pthread_join(socket_escucha_consola, NULL);
    pthread_join(hilo_io, NULL);

    // Libero conexiones
    free(config_kernel);
    free(kernel_io);
    liberar_conexion(escucha_fd);   

    return 0;
}
