#include "../include/main.h"

int main(int argc, char* argv[]) {
    
    t_config* config_memoria = iniciar_config("./memoria.config");
    t_log* logger_memoria = iniciar_logger("memoria.log");

    // Esto es solo para probar y practicar sockets, supongo que esto despues va con hilos
    iniciar_servidor_memoria(config_memoria, logger_memoria);

    return 0;
}
