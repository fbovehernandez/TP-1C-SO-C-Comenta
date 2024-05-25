#include "../include/main.h"

int main(int argc, char* argv[2]) {
    
    // Este archivo lo recibe por parametro
    config_io = iniciar_config(argv[2]);
    logger_io = iniciar_logger("entradasalida.log"); // Declararla en el .h
    
    if(config_io == NULL) {
        log_error(logger_io, "No se pudo leer el archivo de configuracion");
        return 1;
    }
    nombre_io = argc;

    char* tipo_interfaz = config_get_string_value(config_io, "TIPO_INTERFAZ"); // TOdos
    char* IP_KERNEL = config_get_string_value(config_io, "IP_KERNEL"); // Todos
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL_IO"); // Todos

    int socket_kernel_io = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io);     
    // close(socket_kernel_io);

    // Falta manejar los distintos tipos de interfaces
    // iniciar_interfaz_generica(argv[1]);

    printf("Tipo de interfaz: %s\n", tipo_interfaz);

    if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        // gestionar_STDOUT(config_io, logger_io); 
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        // printf("STDIN - Falta gestionar\n");        
    } else if (strcmp(tipo_interfaz, "Genericas") == 0) {
            iniciar_interfaz_generica(config_io, IP_KERNEL, puerto_kernel);
    } else if (strcmp(tipo_interfaz, "DialFS") == 0) {
        // gestionar_DIALFS(config_io);
    } else {
        printf("No trates de hacer mi experiencia con operativos peor\n");
        // insert logica
    }

    return 0;
}

void iniciar_interfaz_generica(char* nombreInterfaz, char* IP_KERNEL, char* puerto_kernel) {
    // Saco el unico dato que tiene esta interfaz
    int tiempo_unidad_trabajo = config_get_int_value(config_io, "TIEMPO_UNIDAD_TRABAJO");
    conectar_io_kernel(IP_KERNEL, puerto_kernel, logger, nombre_interfaz); // falta logger..
    recibir_kernel();

    // IO_GEN_SLEEP(unidad_trabajo,tiempo_unidad_trabajo); 
}

void IO_GEN_SLEEP(int unidad_trabajo, int tiempo_unidad_trabajo) {
    sleep(unidad_trabajo * tiempo_unidad_trabajo);
}


// tiempo_unidad_trabajo: Tiempo que dura una unidad de trabajo en milisegundos
// unidad_trabajo: Cantidad de unidades de trabajo que va a dormir segun la instruccion de CPU