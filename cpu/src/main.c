#include "../include/main.h"

int main(int argc, char* argv[]) {
    logger_CPU    = iniciar_logger("cpu.log"); 
    
    char* input = readline(">Ingrese el config (con ./): ");
    config_CPU = iniciar_config(input);
    free(input);


    char* escucha_dispatch  = config_get_string_value(config_CPU, "PUERTO_ESCUCHA_DISPATCH");
    char* escucha_interrupt = config_get_string_value(config_CPU, "PUERTO_ESCUCHA_INTERRUPT");
    //config_get_string_value(config_CPU, "IP_CPU");

    char* IP_MEMORIA     = config_get_string_value(config_CPU, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config_CPU, "PUERTO_MEMORIA");

    registros_cpu = malloc(sizeof(t_registros)); 
    registros_cpu = inicializar_registros_cpu(registros_cpu);

    tlb = list_create();

    // Aca lo conecto a memoria 
    socket_memoria = conectar_memoria(IP_MEMORIA, puerto_memoria, logger_CPU);
    
    // send(socket_memoria, &nuevo_mensaje, sizeof(int), 0); // Me conecto y envio un mensaje a memoria
    // close(socket_memoria);

    //////////// -> Inicializa los datos y se los pasa a los hilos
    t_config_cpu* cpu_interrupt = iniciar_datos(escucha_interrupt, logger_CPU);
    t_config_cpu* cpu_dispatch  = iniciar_datos(escucha_dispatch, logger_CPU);

    printf("Puerto escucha dispatch: %s\n", cpu_dispatch->puerto_escucha);

    // Aca levanto server dispatch e interrupt
	pthread_t dispatch;  
    pthread_t interrupt;

    // Creacion de hilos, con sus respectivas funciones
    pthread_create(&dispatch, NULL, (void*)iniciar_servidor_dispatch, (void*)(cpu_dispatch));
    pthread_create(&interrupt, NULL, (void*)iniciar_servidor_interrupt, (void*)(cpu_interrupt));

    pthread_join(interrupt, NULL);
    pthread_join(dispatch, NULL);

    free(cpu_interrupt->puerto_escucha);
    log_destroy(cpu_interrupt->logger);
    free(cpu_interrupt);

    free(cpu_dispatch->puerto_escucha);
    log_destroy(cpu_dispatch->logger);
    free(cpu_dispatch);

    free(escucha_dispatch);
    free(escucha_interrupt);
    free(IP_MEMORIA);
    free(puerto_memoria);
    free(registros_cpu);

    config_destroy(config_CPU);
    log_destroy(logger_CPU);

	return 0;
}
