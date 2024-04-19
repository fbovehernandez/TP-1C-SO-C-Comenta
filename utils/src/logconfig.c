#include "../include/logconfig.h"

t_config* iniciar_config(char* path) {
    t_config* nuevo_config;
	if((nuevo_config = config_create(path)) == NULL) {
		printf("%s", "config not created");
		exit(1);
	}

	return nuevo_config;
}

t_log* iniciar_logger(char* path) {
	t_log* nuevo_logger = log_create(path, "LOGGER", true, LOG_LEVEL_INFO);
	if(nuevo_logger == NULL) {
		perror("Hay un error!");
		exit(EXIT_FAILURE);
	}
	
	return nuevo_logger;
}

void terminar_programa(int conexion, t_log* logger, t_config* config) {
	log_destroy(logger);
	config_destroy(config);
	close(conexion);
}

int recibir_operacion(int socket_client) {
    int cod_op;
    if(recv(socket_client, &cod_op, sizeof(int), MSG_WAITALL) != 0) {
        return cod_op;
    } else {
        close(socket_client);
        return -1;
    }
}