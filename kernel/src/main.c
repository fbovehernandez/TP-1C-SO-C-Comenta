#include "../include/main.h"

int main(int argc, char* argv[]) {
    planificacion_pausada = false;
    // Inicializo colas con queue_create
    cola_new                     = queue_create(); 
    cola_ready                   = queue_create();
    cola_blocked                 = queue_create();
    cola_exit                    = queue_create();
    cola_prioritarios_por_signal = queue_create();
    cola_ready_plus              = queue_create();

    
    t_config* config_kernel   = iniciar_config("./kernel.config");
    logger_kernel = iniciar_logger("kernel.log");
    datos_kernel = solicitar_datos(config_kernel);
    path_kernel = "/home/utnso/tp-2024-1c-Sofa-Cama/kernel/scripts-comandos"; // hardcodeado nashe

    quantum_config = datos_kernel->quantum;
    grado_multiprogramacion = datos_kernel->grado_multiprogramacion;
    algoritmo_planificacion = datos_kernel->algoritmo_planificacion;
    diccionario_io = dictionary_create();

    // Lista global para manejo de I/O
    // lista_io = list_create();
    lista_procesos = list_create();

    pthread_mutex_init(&mutex_estado_new, NULL);
    pthread_mutex_init(&mutex_estado_blocked, NULL);
    pthread_mutex_init(&mutex_estado_ready, NULL);
    sem_init(&sem_contador_quantum, 0, 0);
    sem_init(&sem_hay_para_planificar, 0, 0);
    sem_init(&sem_hay_pcb_esperando_ready, 0, 0);
    sem_init(&sem_grado_multiprogramacion, 0, grado_multiprogramacion); // No testeado
    sem_init(&sem_memoria_instruccion, 0, 0);
    sem_init(&sem_cargo_instrucciones, 0 ,0);
    sem_init(&sem_planificadores,0,1);
    pthread_mutex_init(&mutex_lista_io, NULL);
    pthread_mutex_init(&mutex_cola_io_generica, NULL);
    pthread_mutex_init(&no_hay_nadie_en_cpu, NULL);

    pcb_exec = crear_nuevo_pcb(0);
    
    // Hilo 1 -> Hacer un hilo para gestionar la comunicacion con memoria?
    int socket_memoria_kernel = conectar_kernel_memoria(datos_kernel->ip_mem, datos_kernel->puerto_memoria, logger_kernel);

    // Hilo 2 -> Hacer un hilo para gestionar comunicacion con la cpu?
    int socket_cpu = conectar_kernel_cpu_dispatch(logger_kernel, datos_kernel->ip_cpu, datos_kernel->puerto_cpu_dispatch);
    client_dispatch = socket_cpu;
    
    // Hilo 3 -> Hacer un hilo para interrupt
    int socket_interrupt = conectar_kernel_cpu_interrupt(logger_kernel, datos_kernel->ip_cpu, datos_kernel->puerto_cpu_interrupt);

    sockets = malloc(sizeof(t_sockets));
    sockets->socket_cpu = socket_cpu;
    sockets->socket_memoria = socket_memoria_kernel;
    sockets->socket_int = socket_interrupt;

    pthread_t pasar_a_ready;
	pthread_create(&pasar_a_ready, NULL, (void*)a_ready, NULL);

    pthread_t planificador_corto_plazo;
    pthread_create(&planificador_corto_plazo, NULL, (void*)planificar_corto_plazo, (void*)sockets); 

    /////////////// ---------- ///////////////
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

    pthread_t socket_escucha_consola;
    pthread_create(&socket_escucha_consola, NULL, (void*) interaccion_consola, NULL); 

    pthread_join(socket_escucha_consola, NULL);
    pthread_join(pasar_a_ready, NULL);
    pthread_join(planificador_corto_plazo, NULL);
    pthread_join(hilo_io, NULL);

    // Libero conexiones
    // free(pcb_exec);
    free(config_kernel);
    free(kernel_io);
    liberar_conexion(escucha_fd);   

    return 0;
}
