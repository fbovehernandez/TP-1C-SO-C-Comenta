#include "../include/sockets.h"

// Ver esta... 
void receiveMessagex(int socket_fd) {
    char buffer[1024] = {0};
    size_t valread;
    
    valread = recv(socket_fd , buffer, 1024, 0);
    if (valread < 0) {
        perror("Failed to receive message");
        exit(1);
    }
    printf("%s\n",buffer );
}

void sendMessage(int socket_fd) {
    const char* message = "Hello, Kernel!";
    if (send(socket_fd, message, strlen(message), 0) < 0) {
        perror("Failed to send the message");
        exit(EXIT_FAILURE);
    }
}

// Saco funcion de serializacion!

int iniciar_servidor(char* puerto) {
    int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);
   
    socket_servidor = socket(servinfo->ai_family,
                    servinfo->ai_socktype,
                    servinfo->ai_protocol);
    
	// Tambien se podria hacer un bind y/o listen sin el int
    int bind_socket = bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	corroborar_exito(bind_socket, "hacer bind al socket.");

    int listen_socket = listen(socket_servidor, SOMAXCONN);

	corroborar_exito(listen_socket, "hacer listen al socket.");

	freeaddrinfo(servinfo);

    return socket_servidor;
}

void corroborar_exito(int variable, char* mensaje) {
	if(variable < 0)
	{
		printf("No se pudo %s", mensaje);
		exit(2); // Exit "2" devuelve el error puntual
	}
}

int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,
                    server_info->ai_socktype,
                    server_info->ai_protocol);
	
	corroborar_exito(socket_cliente, " crear el socket cliente.");

	// Ahora que tenemos el socket, vamos a conectarlo
	int conexion_cliente = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	corroborar_exito(conexion_cliente, " conectar al cliente.");

	freeaddrinfo(server_info);

	return socket_cliente;
}

// Saco funcion crear buffer! 

int esperar_conexion(int socket_servidor) {
    int socket_cliente;
    
    socket_cliente = accept(socket_servidor, NULL, NULL);
    return socket_cliente; // Devuelve el socket de conexion con el cliente que se conecto
}

// Saco algunas funciones que no se estaban usando para testear conexiones