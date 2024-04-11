#include "../include/main.h"

int main(int argc, char* argv[]) {
    // decir_hola("una Interfaz de Entrada/Salida");
    
    t_config* config_io = iniciar_config("./entradasalida.config");
    t_log* logger_io = iniciar_logger("entradasalida.log");

    // Esto es solo para probar y practicar sockets, supongo que esto despues va con hilos

    char* ip_io = config_get_string_value(config_io, "IP_KERNEL_IO");
    // char* puerto_io = config_get_string_value(config_io, "PUERTO_KERNEL_IO");
    conectar_kernel_io(config_io, logger_io, ip_io);
    
    return 0;
}
