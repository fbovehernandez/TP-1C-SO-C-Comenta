#include "../include/main.h"

int main(int argc, char* argv[]) {
    int nuevo_mensaje = 10;

    t_log* logger_CPU = iniciar_logger("cpu.log"); 
    t_config* config_CPU = iniciar_config("./cpu.config");
    
    char* escucha_dispatch = config_get_string_value(config_CPU, "PUERTO_ESCUCHA_DISPATCH");
    char* escucha_interrupt = config_get_string_value(config_CPU, "PUERTO_ESCUCHA_INTERRUPT");
    char* ip_cpu = config_get_string_value(config_CPU, "IP_CPU");

    char* IP_MEMORIA = config_get_string_value(config_CPU, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config_CPU, "PUERTO_MEMORIA");

    // Aca lo conecto a memoria -> creo que esto SI deberia ser un hilo
    int socket_memoria = conectar_memoria(IP_MEMORIA, puerto_memoria, logger_CPU);
    
    sleep(5);
    send(socket_memoria, &nuevo_mensaje, sizeof(int), 0); // Me conecto y envio un mensaje a memoria
    // close(socket_memoria);

    //////////// ---

    t_config_cpu* datos_dispatch = malloc(sizeof(t_config_cpu));
    datos_dispatch->puerto_escucha = escucha_dispatch;
    datos_dispatch->logger = logger_CPU;

    // Aca levanto server dispatch
	pthread_t dispatch;
    pthread_create(&dispatch, NULL, (void*)iniciar_servidor_dispatch, (void*)datos_dispatch);
    pthread_join(dispatch, NULL);
	return 0;
}