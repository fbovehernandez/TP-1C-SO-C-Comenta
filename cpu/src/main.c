#include "../include/main.h"

int main(int argc, char* argv[]) {
    //int conexion;

    t_log* logger_CPU = iniciar_logger("cpu.log"); 
    t_config* config_CPU = iniciar_config("./cpu.config");
                            
    escuchar_dispatcher(config_CPU, logger_CPU);
    escuchar_interrupt(config_CPU, logger_CPU); 
    return 0;
}
