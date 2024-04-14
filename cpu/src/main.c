#include "../include/main.h"

int main(int argc, char* argv[]) {
    //int conexion;

    t_log* logger_CPU = iniciar_logger("cpu.log"); 
    t_config* config_CPU = iniciar_config("./cpu.config");
                            
    escuchar_dispatcher(config_CPU, logger_CPU);
    escuchar_interrupt(config_CPU, logger_CPU); 

    char* IP_MEMORIA = config_get_string_value(config_CPU, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config_CPU, "PUERTO_MEMORIA");

    int socket_memoria = conectar_memoria(IP_MEMORIA, puerto_memoria, logger_CPU);

    // Estaria bueno hacer esto generico (y en utils) pero tengo sueño
    handshake(socket_memoria);

    close(socket_memoria);

    return 0;
}
