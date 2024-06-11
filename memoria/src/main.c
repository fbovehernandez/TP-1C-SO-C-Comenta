#include "../include/main.h"

int main(int argc, char *argv[]) {

    // Creo los configs y logs
    config_memoria = iniciar_config("./memoria.config");
    t_log *logger_memoria = iniciar_logger("memoria.log");

    char *puerto_escucha = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");
    path_config = config_get_string_value(config_memoria, "PATH_INSTRUCCIONES");

    // Aca creo el espacio de usuario con el tam de la memoria
    tamanio_memoria = config_get_int_value(config_memoria, "TAM_MEMORIA");
    
    tamanio_pagina = config_get_int_value(config_memoria, "TAM_PAGINA");

    espacio_usuario = malloc(tamanio_memoria);
	memset(espacio_usuario, 0, tamanio_memoria); // seteo memoria en 0

    int cant_frames = tamanio_memoria / tamanio_pagina;
    //t_bitarray* bitmap = bitarray_create_with_mode(user_space, cant_frames, MSB_FIRST);

    diccionario_instrucciones = dictionary_create();
    diccionario_tablas_paginas = dictionary_create();

    // Levanto el servidor y devuelvo el socket de escucha
    int escucha_fd = iniciar_servidor(puerto_escucha);
    log_info(logger_memoria, "Servidor iniciado, esperando conexiones!");

    t_config_memoria *memory_struct_cpu = malloc(sizeof(t_config_memoria));
    memory_struct_cpu->socket = escucha_fd;
    memory_struct_cpu->logger = logger_memoria;

    esperar_cliente(escucha_fd, logger_memoria);
    esperar_cliente(escucha_fd, logger_memoria);

    // Hago los join aca para que no se cierre el hilo principal ->  TODO: Ver mejor implementacion o volver al while(1)
    pthread_join(cpu_thread, NULL);
    pthread_join(kernel_thread, NULL);

    // Libero conexiones
    free(config_memoria);
    free(memory_struct_cpu);
    liberar_conexion(escucha_fd);
    // liberar_conexion(cliente_kernel);

    return 0;
}