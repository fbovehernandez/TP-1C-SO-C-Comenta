#include "../include/main.h"

int main(int argc, char* argv[]) {

    t_config* config_kernel   = iniciar_config("./kernel.config");
    t_log* logger_kernel = iniciar_logger("kernel.log"); 

    char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    conectar_kernel_cpu_dispatch(config_kernel, logger_kernel, IP_CPU); // agarra el puerto que escucha la CPU y se conecta a la CPU
    conectar_kernel_cpu_interrupt(config_kernel, logger_kernel, IP_CPU);
    conectar_kernel_memoria(config_kernel, logger_kernel);

    escuchar_IO(config_kernel, logger_kernel);
    return 0;
}