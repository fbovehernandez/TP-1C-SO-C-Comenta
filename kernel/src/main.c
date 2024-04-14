#include "../include/main.h"

int main(int argc, char* argv[]) {

    t_config* config_kernel   = iniciar_config("./kernel.config");
    t_log* logger_kernel = iniciar_logger("kernel.log"); 

    char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    conectar_kernel_cpu_dispatch(config_kernel, logger_kernel, IP_CPU); // agarra el puerto que escucha la CPU y se conecta a la CPU
    conectar_kernel_cpu_interrupt(config_kernel, logger_kernel, IP_CPU);

    char* IP_MEMORIA = config_get_string_value(config_kernel, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");

    conectar_kernel_memoria(IP_MEMORIA, puerto_memoria, logger_kernel);
    
    // escuchar_conexiones(config_kernel, logger_kernel);
    // escuchar_STDOUT(config_kernel, logger_kernel);
    return 0;
}