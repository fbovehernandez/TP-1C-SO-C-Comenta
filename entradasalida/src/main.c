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
    char* IP_MEMORIA = config_get_string_value(config_io, "IP_MEMORIA"); // Todos
    char* puerto_memoria = config_get_string_value(config_io, "PUERTO_MEMORIA"); // Todos

    // int socket_kernel_io = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombre_io);     
    // close(socket_kernel_io);

    // Falta manejar los distintos tipos de interfaces
    printf("Tipo de interfaz: %s\n", tipo_interfaz);

    if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        iniciar_stdout(nombre_io, config_io, IP_KERNEL,IP_MEMORIA, puerto_kernel, puerto_memoria);
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        iniciar_stdin(nombre_io, config_io, IP_KERNEL, IP_MEMORIA, puerto_kernel, puerto_memoria) ;      
    } else if (strcmp(tipo_interfaz, "GENERICA") == 0) {
        iniciar_interfaz_generica(nombre_io, config_io, IP_KERNEL, puerto_kernel);
    } else if (strcmp(tipo_interfaz, "DialFS") == 0) {
        iniciar_dialfs(nombre_io,config_io,IP_KERNEL,IP_MEMORIA,puerto_kernel,puerto_memoria);
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
    kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, GENERICA, 5); 
    
    t_config_socket_io* config_generica_io = malloc(sizeof(t_config_socket_io)); //FREE?
    config_generica_io->config_io = config_io;
    config_generica_io->socket_io = kernelfd;
    
    recibir_kernel((void*) config_generica_io); 
}

// tiempo_unidad_trabajo: Tiempo que dura una unidad de trabajo en milisegundos
// unidad_trabajo: Cantidad de unidades de trabajo que va a dormir segun la instruccion de CPU

void iniciar_stdin(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria) {
    kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, STDIN, 13); 
    memoriafd = conectar_io_memoria(IP_MEMORIA, puerto_memoria, logger_io, nombreInterfaz, STDIN, 91);

    t_config_socket_io* config_stdin_io = malloc(sizeof(t_config_socket_io)); //FREE?
    config_stdin_io->config_io = config_io;
    config_stdin_io->socket_io = kernelfd;
    
    recibir_kernel((void*) config_stdin_io);
}

void iniciar_stdout(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria) {
    kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, STDOUT, 15); 
    memoriafd = conectar_io_memoria(IP_MEMORIA, puerto_memoria, logger_io, nombreInterfaz, STDOUT, 79);

    t_config_socket_io* config_stdout_io = malloc(sizeof(t_config_socket_io)); //FREE?
    config_stdout_io->config_io = config_io;
    config_stdout_io->socket_io = memoriafd;

    // recibir_kernel(config_io, kernelfd);
    recibir_memoria((void*) config_stdout_io);
} 

void iniciar_dialfs(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria) {
    t_config_dialfs* dialfs = inicializar_file_system(config_io);
    crear_archivos_iniciales(dialfs);

    kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, DIALFS, 17); 
    memoriafd = conectar_io_memoria(IP_MEMORIA, puerto_memoria, logger_io, nombreInterfaz, DIALFS, 81);

    t_config_socket_io* config_kernel_io = malloc(sizeof(t_config_socket_io)); //FREE?
    config_kernel_io->config_io = config_io;
    config_kernel_io->socket_io = kernelfd;

    t_config_socket_io* config_memoria_io = malloc(sizeof(t_config_socket_io)); //FREE?
    config_memoria_io->config_io = config_io;
    config_memoria_io->socket_io = memoriafd;

    recibir_kernel_y_memoria(config_kernel_io, config_memoria_io);
    // recibir_memoria(config_io, memoriafd); TO DO: CONECTAR CON MEMORIA DESDE MEMORIA
}

void recibir_kernel_y_memoria(t_config_socket_io* config_kernel_io, t_config_socket_io* config_memoria_io) {
    pthread_t hilo_kernel;
    pthread_t hilo_memoria;
    
    pthread_create(&hilo_kernel, NULL, (void*) recibir_kernel, (void*) config_kernel_io);
    pthread_create(&hilo_memoria, NULL, (void*) recibir_memoria, (void*) config_memoria_io);

    // Uso los join porque sino se me corta el main
    pthread_join(hilo_kernel, NULL);
    pthread_join(hilo_memoria, NULL);
}


