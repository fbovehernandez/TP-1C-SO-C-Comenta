#include "../include/main.h"

int main(int argc, char* argv[]) {

    // Creo los configs y logs   
    pthread_mutex_init(&mutex_diccionario_instrucciones, NULL);
    config_memoria = iniciar_config("./memoria.config");
    t_log* logger_memoria = iniciar_logger("memoria.log");

    char* puerto_escucha = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");
    path_config = config_get_string_value(config_memoria, "PATH_INSTRUCCIONES");

    // Aca creo el espacio de usuario con el tam de la memoria
    tamanio_memoria = config_get_int_value(config_memoria, "TAM_MEMORIA");
    tamanio_pagina = config_get_int_value(config_memoria, "TAM_PAGINA");

    espacio_usuario = malloc(tamanio_memoria);
	memset(espacio_usuario, 0, tamanio_memoria); // seteo memoria en 0
    
    cant_frames = tamanio_memoria / tamanio_pagina;

    size_t bitarray_size_in_bytes = (cant_frames + 7 / 8);
    char *bitarray_memory = malloc(bitarray_size_in_bytes);

    // Aca hay que tener cuidado con los bits adicionales cuando se recorre el bityarray
    bitmap = bitarray_create_with_mode(bitarray_memory , bitarray_size_in_bytes, LSB_FIRST);

    size_t max_bits = bitarray_get_max_bit(bitmap); // retorna la cantidad total de bits, + los adicionales
    for (size_t i = 0; i < max_bits; i++) {
        bitarray_clean_bit(bitmap, i); // Esto los limpia y lo pone en 0, es decr, disponible
    }

    diccionario_tablas_paginas = dictionary_create();
    diccionario_instrucciones = dictionary_create();

    // t_list* lista_valores_a_leer = list_create();

    // Levanto el servidor y devuelvo el socket de escucha
    int escucha_fd = iniciar_servidor(puerto_escucha);
    log_info(logger_memoria, "Servidor iniciado, esperando conexiones!");

    t_config_memoria* memory_struct_cpu = malloc(sizeof(t_config_memoria));
    memory_struct_cpu->socket = escucha_fd;
    memory_struct_cpu->logger = logger_memoria;

    esperar_cliente(escucha_fd, logger_memoria);
    esperar_cliente(escucha_fd, logger_memoria); // Comente esto porque no se si es necesario
    
    // Hago los join aca para que no se cierre el hilo principal ->  TODO: Ver mejor implementacion o volver al while(1)
    pthread_join(cpu_thread, NULL); 
    pthread_join(io_stdin_thread, NULL); 
    pthread_join(io_stdout_thread, NULL); 
    pthread_join(kernel_thread, NULL);

    // Libero conexiones 
    free(config_memoria);
    free(memory_struct_cpu);
    liberar_conexion(escucha_fd);
    // liberar_conexion(cliente_kernel);

    return 0;
}