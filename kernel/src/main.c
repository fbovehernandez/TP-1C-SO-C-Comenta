#include "../include/main.h"
#include <commons/collections/queue.h>

uint32_t contador_pid = 0; // No se si podria ir en el .h


/* -> Creo proceso con pcb y obtengo su PID
    void iniciar_proceso() {
    uint_32 pid = obtener_pid();
    t_pcb* = crear_pcb(pid);
}
*/

// Creacion del pcb es la que esta definida en funcionalidades
int interaccion_consola(uint32_t contador_pid) {
    
    int respuesta;
    do {
        printf("---------------Consola interactiva Kernel---------------\n");
        printf("Elija una opcion (numero)\n");
        printf("1- Ejecutar Script \n");
        printf("2- Iniciar proceso\n");
        printf("3- Finalizar proceso\n");
        printf("4- Iniciar planificacion\n");
        printf("5- Detener planificacion\n");
        printf("6- Listar procesos por estado\n");
        printf("7- Finalizar modulo\n");
        scanf("%d",respuesta);
        switch (respuesta) {
        case 1:
            EJECUTAR_SCRIPT(/* pathing del conjunto de instrucciones*/);
            break;
        case 2:
            INICIAR_PROCESO(contador_pid); // Esto va a crear un pcb que representa al proceso, y va a ponerlo en la cola de new (despues un hilo lo pasa a ready?)
            break;
        case 3:
            FINALIZAR_PROCESO();
            break;
        case 4:
            INICIAR_PLANIFICACION(); // Esto depende del algoritmo vigente (FIFO, RR,VRR), se deben poder cambiar
            break;
        case 5:
            DETENER_PLANIFICACION(); 
            break;
        case 6:
            PROCESO_ESTADO();
            break;
        case 7:
            printf("Finalizacion del modulo\n");
            exit(1); 
            break;
        default:
            printf("No se encuentra el numero dentro de las opciones, porfavor elija una correcta\n");
            break;
        }

    } while(respuesta != 7); // Modificar respuesta si se agregan mas elementos en la consola
    return 0;
}

int main(int argc, char* argv[]) {
    int contador_pid = 0;

    // Consola interactiva -> un hilo?
    t_dictionary* pid = dictionary_create();
	t_dictionary* estado_proceso = dictionary_create();

    int proximo_pid = 0;
	int resultOk = 0;

	sem_init(&mutex_cola_new, 0, 1); // 1 da el paso al que tenga 0 como tercer parametro
	sem_init(&mutex_cola_sus_ready, 0, 1);
	sem_init(&mutex_cola_ready, 0, 1);
	sem_init(&cantidad_listas_para_estar_en_ready, 0, 0);
	sem_init(&binario_flag_interrupt, 0, 0);
	sem_init(&mutex_cola_blocked, 0, 1);
	sem_init(&signal_a_io, 0, 0);
	sem_init(&dispositivo_de_io, 0, 1);
	sem_init(&mutex_cola_sus_ready, 0, 0);
	sem_init(&binario_planificador_corto, 0, 0);
	sem_init(&mutex_respuesta_interrupt, 0, 1);

	cola_new = queue_create();
	cola_procesos_sus_ready = queue_create();
	cola_ready = queue_create();
	cola_io = queue_create();
	respuesta_interrupcion = queue_create();

    int quantum;
    // int messagex = 10;

    t_config* config_kernel   = iniciar_config("./kernel.config");
    t_log* logger_kernel = iniciar_logger("kernel.log");
    ptr_kernel* datos_kernel = solicitar_datos(config_kernel);

    // Hilo 1 -> Hacer un hilo para gestionar la comunicacion con memoria?
    int socket_memoria_kernel = conectar_kernel_memoria(datos_kernel->ip_mem, datos_kernel->puerto_memoria, logger_kernel);

    // Hilo 2 -> Hacer un hilo para gestionar comunicacion con la cpu?
    int socket_cpu = conectar_kernel_cpu_dispatch(logger_kernel, datos_kernel->ip_cpu, datos_kernel->puerto_cpu_dispatch);
    
    // crear_pcb()
    // enviar_pcb(socket_cpu, logger_kernel, 1); // -> Enviar PCB a CPU

    pthread_t socket_escucha_consola;
	pthread_create(&socket_escucha_consola, NULL, _f_aux_escucha_consola, (void*) &socket_kernel_escucha);
    
    pthread_t recibir_procesos_por_cpu;
	pthread_create(&recibir_procesos_por_cpu, NULL, (void*)recibir_pcb_de_cpu, NULL);

	pthread_create(&pasar_a_ready, NULL, funcion_pasar_a_ready, NULL);

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
