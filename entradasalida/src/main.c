#include "../include/main.h"

//./bin/entradasalida generica.config Interfaz1
//  Bin entradasalida es 0
//   es 1
//  config es 2

int main(int argc, char* argv[2]) {
    
    // Este archivo lo recibe por parametro
    t_config* config_io = iniciar_config(argv[1]);
    logger_io = iniciar_logger("entradasalida.log"); // Declararla en el .h
    
    if(config_io == NULL) {
        log_error(logger_io, "No se pudo leer el archivo de configuracion");
        return 1;
    }
    
    nombre_io = strdup(argv[2]);

    printf("Este es el nombre de la io: %s\n", nombre_io);
    printf("Este es el nombre del config: %s\n", argv[1]);

    char* tipo_interfaz = config_get_string_value(config_io, "TIPO_INTERFAZ"); // TOdos
    char* IP_KERNEL = config_get_string_value(config_io, "IP_KERNEL"); // Todos
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL"); // Todos

    // int socket_kernel_io = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombre_io);     
    // close(socket_kernel_io);

    // Falta manejar los distintos tipos de interfaces
    printf("Tipo de interfaz: %s\n", tipo_interfaz);

    if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        // gestionar_STDOUT(config_io, logger_io); 
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        // printf("STDIN - Falta gestionar\n");        
    } else if (strcmp(tipo_interfaz, "GENERICA") == 0) {
            iniciar_interfaz_generica(nombre_io, config_io, IP_KERNEL, puerto_kernel);
    } else if (strcmp(tipo_interfaz, "DialFS") == 0) {
        // gestionar_DIALFS(config_io);
    } else {
        printf("No trates de hacer mi experiencia con operativos peor\n");
        // insert logica
    }

    free(nombre_io);
    return 0;
}

void iniciar_interfaz_generica(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* puerto_kernel) {
    // Saco el unico dato que tiene esta interfaz
    int tiempo_unidad_trabajo = config_get_int_value(config_io, "TIEMPO_UNIDAD_TRABAJO");
    printf("Tiempo de unidad de trabajo: %d\n", tiempo_unidad_trabajo);
    int kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, GENERICA); 
    recibir_kernel(config_io, kernelfd); 
}


// tiempo_unidad_trabajo: Tiempo que dura una unidad de trabajo en milisegundos
// unidad_trabajo: Cantidad de unidades de trabajo que va a dormir segun la instruccion de CPU