#include "../include/sockets.h"

sem_t sem_memoria_instruccion;
t_registros* registros_cpu;

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
	if(variable < 0) {
		printf("No se pudo %s\n", mensaje);
		exit(2); // Exit "2" devuelve el error puntual
	}
}

int crear_conexion(char *ip, char* puerto, int valor) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	uint32_t handshake = valor;
	uint32_t result;

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

	corroborar_exito(conexion_cliente, "conectar al cliente.");

	// Envio handshake
	send(socket_cliente, &handshake, sizeof(int), 0);
	recv(socket_cliente, &result, sizeof(int), MSG_WAITALL); 

	// Hacer con logger
	
	if(!result)
		printf("conexion exitosa");
	else
		printf("conexion fallida");

	freeaddrinfo(server_info);

	return socket_cliente;
}

// Saco funcion crear buffer! 

int esperar_conexion(int socket_servidor) {
    int socket_cliente;
    
    socket_cliente = accept(socket_servidor, NULL, NULL);
    return socket_cliente; // Devuelve el socket de conexion con el cliente que se conecto
}

void liberar_conexion(int socket) {
	close(socket);
}

t_registros* inicializar_registros_cpu(t_registros* registro_pcb) {

    registro_pcb->AX = 0;
    registro_pcb->BX = 0;
    registro_pcb->CX = 0;
    registro_pcb->DX = 0;

    registro_pcb->EAX = 0;
    registro_pcb->EBX = 0;
    registro_pcb->ECX = 0;
    registro_pcb->EDX = 0;

    registro_pcb->SI = 0;
    registro_pcb->DI = 0;

    return registro_pcb;
}

void* enviar_pcb(t_pcb* pcb, int socket, codigo_operacion cod_op) {
    printf("Este es el pid del pcb a mandar: %d\n", pcb->pid);
    t_buffer* buffer = llenar_buffer_pcb(pcb);

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = cod_op; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    printf("Paquete enviado!");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
    return 0;
}

t_pcb* deserializar_pcb(t_buffer* buffer) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->registros = malloc(sizeof(t_registros)); // Acordarse de liberar memoria

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(pcb->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->program_counter), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->quantum), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->estadoActual), stream, sizeof(Estado));
    stream += sizeof(Estado);
    memcpy(&(pcb->estadoAnterior), stream, sizeof(Estado));
    stream += sizeof(Estado);
    // Deserializo los registros ...

    memcpy(&(pcb->registros->AX), stream, sizeof(uint8_t));
    stream += sizeof(uint8_t);
    memcpy(&(pcb->registros->BX), stream, sizeof(uint8_t));
    stream += sizeof(uint8_t);
    memcpy(&(pcb->registros->CX), stream, sizeof(uint8_t));
    stream += sizeof(uint8_t);
    memcpy(&(pcb->registros->DX), stream, sizeof(uint8_t));
    stream += sizeof(uint8_t);

    memcpy(&(pcb->registros->EAX), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pcb->registros->EBX), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pcb->registros->ECX), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pcb->registros->EDX), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    memcpy(&(pcb->registros->SI), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pcb->registros->DI), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
   
    return pcb;
}

t_buffer* llenar_buffer_pcb(t_pcb* pcb) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    printf("Llenando buffer...");

    buffer->size = sizeof(int) * 3 
                + sizeof(Estado) * 2
                + sizeof(uint8_t) * 4
                + sizeof(uint32_t) * 6;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream; // No memoria?

    memcpy(stream + buffer->offset, &pcb->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pcb->program_counter, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pcb->quantum, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pcb->estadoActual, sizeof(Estado));
    buffer->offset += sizeof(Estado);
    memcpy(stream + buffer->offset, &pcb->estadoAnterior, sizeof(Estado));
    buffer->offset += sizeof(Estado);
    // Serializo los registros...

    memcpy(stream + buffer->offset, &pcb->registros->AX, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);
    memcpy(stream + buffer->offset, &pcb->registros->BX, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);
    memcpy(stream + buffer->offset, &pcb->registros->CX, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);
    memcpy(stream + buffer->offset, &pcb->registros->DX, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);

    memcpy(stream + buffer->offset, &pcb->registros->EAX, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &pcb->registros->EBX, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &pcb->registros->ECX, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &pcb->registros->EDX, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(stream + buffer->offset, &pcb->registros->SI, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &pcb->registros->DI, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    buffer->stream = stream;

    printf("Buffer llenado...");
    return buffer;
}

/* 
void paquete(int conexion) {
	// Ahora toca lo divertido!
	char* leido;
	t_paquete* paquete;

	// Leemos y esta vez agregamos las lineas al paquete
	paquete = crear_paquete();

	while(1) {
		leido = readline("> ");

		if(strcmp(leido, "") == 0) 
		{
			break;
		}

		agregar_a_paquete(paquete, leido, strlen(leido) + 1);

		free(leido);
	}

	enviar_paquete(paquete, conexion);
	
	eliminar_paquete(paquete);
}
*/