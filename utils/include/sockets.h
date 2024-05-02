#ifndef SOCKET_H
#define SOCKET_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/config.h>
#include <assert.h>
#include <pthread.h>

typedef enum {
	MENSAJE,
	PAQUETE
} op_code;

typedef struct {
	int size;
	uint32_t offset;
	void* stream;
} t_buffer;

typedef struct {
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef enum {
    PATH
} codigo_operacion;

int iniciar_servidor(char*);
int esperar_conexion(int);
void corroborar_exito(int, char*);
void sendMessage(int socket_fd);
void receiveMessagex(int socket_fd);
int crear_conexion(char *ip, char* puerto, int valor);

#endif // SOCKET_H