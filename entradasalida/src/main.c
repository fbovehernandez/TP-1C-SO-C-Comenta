#include "../include/main.h"

int main(int argc, char* argv[1]) {
    
    // Este archivo lo recibe por parametro
    t_config* config_io = iniciar_config(argv[1]);
    t_log* logger_io = iniciar_logger("entradasalida.log");

    char* IP_KERNEL = config_get_string_value(config_io, "IP_KERNEL");
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL_IO");
    
    if(config_io == NULL) {
        log_error(logger_io, "No se pudo leer el archivo de configuracion");
        return 1;
    }

    int socket_kernel_io = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io);     
    // close(socket_kernel_io);

/*
    char* tipo_interfaz = config_get_string_value(config_io, "TIPO_INTERFAZ");
    printf("Tipo de interfaz: %s\n", tipo_interfaz);

    if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        gestionar_STDOUT(config_io, logger_io);
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        printf("STDIN - Falta gestionar\n");        
    } else if (strcmp(tipo_interfaz, "Genericas") == 0) {
        // gestionar_GENERICA(config_io);
    } else if (strcmp(tipo_interfaz, "DialFS") == 0) {
        // gestionar_DIALFS(config_io);
    } else {
        printf("No trates de hacer mi experiencia con operativos peor\n");
        // insert logica
    }
*/

    return 0;
}
