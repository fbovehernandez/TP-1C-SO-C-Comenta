#include "../include/sockets.h"

sem_t sem_cargo_instrucciones;
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
	
	corroborar_exito(socket_cliente, "crear el socket cliente.\n");

	// Ahora que tenemos el socket, vamos a conectarlo
	int conexion_cliente = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	corroborar_exito(conexion_cliente, "conectar al cliente.\n");

	// Envio handshake
	send(socket_cliente, &handshake, sizeof(int), 0);
	recv(socket_cliente, &result, sizeof(int), MSG_WAITALL); 
    
    printf("recibo %d como result\n", result);
	// Hacer con logger
	
	if(!result)
		printf("conexion exitosa\n");
	else
		printf("conexion fallida\n");

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

void imprimir_pcb(t_pcb* pcb) {
    printf("El pid es: %d\n", pcb->pid);
    printf("El program counter es: %d\n", pcb->program_counter);
    printf("El quantum es: %d\n", pcb->quantum);
    // printf("El estado actual es %s : ", pcb->estadoActual);
    // printf("El estado anterior es %s : ", pcb->estadoAnterior);
    printf("Registro AX: %u\n", pcb->registros->AX);
    printf("Registro BX: %u\n", pcb->registros->BX);
    printf("Registro CX: %u\n", pcb->registros->CX);
    printf("Registro DX: %u\n", pcb->registros->DX);

    printf("Registro EAX: %u\n", pcb->registros->EAX);
    printf("Registro EBX: %u\n", pcb->registros->EBX);
    printf("Registro ECX: %u\n", pcb->registros->ECX);
    printf("Registro EDX: %u\n", pcb->registros->EDX);

    printf("Registro SI: %u\n", pcb->registros->SI);
    printf("Registro DI: %u\n", pcb->registros->DI);
}
/* 
t_operacion_io* operacion_io = malloc(sizeof(t_operacion_io));

        void* stream = buffer->stream;

        // Deserializamos los campos que tenemos en el buffer
        memcpy(&(operacion_io->unidadesDeTrabajo), stream, sizeof(int));
        stream += sizeof(int);

        memcpy(&(operacion_io->nombre_interfaz_largo), stream, sizeof(int));
        stream += sizeof(int);

        operacion_io->nombre_interfaz = malloc(operacion_io->nombre_interfaz_largo); 

        memcpy(operacion_io->nombre_interfaz, stream, operacion_io->nombre_interfaz_largo);
    
        printf("Interfaz: %s\n", operacion_io->nombre_interfaz);
        printf("Unidades de trabajo: %d\n", operacion_io->unidadesDeTrabajo);

*/
void* enviar_pcb(t_pcb* pcb, int socket, codigo_operacion cod_op, t_buffer* datos_add) {
    printf("Este es el pid del pcb a mandar: %d\n", pcb->pid);
    t_buffer* buffer = llenar_buffer_pcb(pcb);

    size_t total_size = buffer->size + (datos_add ? datos_add->size : 0);

    void* stream = malloc(total_size);
    int offset_add = 0;

    memcpy(stream + offset_add, buffer->stream, buffer->size); // antes decia buffer->size en el parametro 3
    offset_add += buffer->size;

    if (datos_add) {
        memcpy(stream + offset_add, datos_add->stream, datos_add->size);
        offset_add += datos_add->size;
    }

    buffer->size = total_size;
    buffer->stream = stream;

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
    printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    // free(datos_add->stream); // Libero esto aca -> Ver de liberal en algo lado
    // free(datos_add);
    liberar_paquete(paquete);
    // Creo que no hace falta el free stream que apunta a lo mismo que paquete->buffer->stream
    return 0;
}

t_pcb* deserializar_pcb(t_buffer* buffer) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->registros = malloc(sizeof(t_registros)); // Acordarse de liberar memoria

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer

    // Ojo con esto, aca usamos directamente el buffer...
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

    printf("Llenando buffer...\n");

    buffer->size = sizeof(int) * 3 
                + sizeof(Estado) * 2
                + sizeof(uint8_t) * 4
                + sizeof(uint32_t) * 6;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pcb->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pcb->program_counter, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pcb->quantum, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->stream + buffer->offset, &pcb->estadoActual, sizeof(Estado));
    buffer->offset += sizeof(Estado);
    memcpy(buffer->stream + buffer->offset, &pcb->estadoAnterior, sizeof(Estado));
    buffer->offset += sizeof(Estado);
    // Serializo los registros...

    memcpy(buffer->stream + buffer->offset, &pcb->registros->AX, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);
    memcpy(buffer->stream + buffer->offset, &pcb->registros->BX, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);
    memcpy(buffer->stream + buffer->offset, &pcb->registros->CX, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);
    memcpy(buffer->stream + buffer->offset, &pcb->registros->DX, sizeof(uint8_t));
    buffer->offset += sizeof(uint8_t);

    memcpy(buffer->stream + buffer->offset, &pcb->registros->EAX, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &pcb->registros->EBX, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &pcb->registros->ECX, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &pcb->registros->EDX, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &pcb->registros->SI, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, &pcb->registros->DI, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    printf("Buffer llenado...\n");
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

void liberar_paquete(t_paquete* paquete) {
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void liberar_paquete_y_a_enviar(t_paquete* paquete,void* a_enviar) {
    free(a_enviar);
    liberar_paquete(paquete);
}

void enviar_paquete(t_buffer* buffer, codigo_operacion codigo, int socket) {
    t_paquete* paquete = malloc(sizeof(t_paquete));

    printf("\nEl socket de la io a la q se va a enviar el paquete es: %d\n", socket);

    paquete->codigo_operacion = codigo;
    paquete->buffer = buffer; 

    void* a_enviar = malloc(buffer->size + sizeof(int) * 2);
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    printf("\nCodigo de operacion: %d\n", codigo);
    send(socket, a_enviar, buffer->size + sizeof(int) * 2, 0);
    printf("Paquete enviado!\n");

    liberar_paquete_y_a_enviar(paquete, a_enviar);
}

t_info_io *deserializar_interfaz(t_buffer *buffer) {
    t_info_io *interfaz = malloc(sizeof(t_info_io));
    void *stream = buffer->stream;
    
    memcpy(&(interfaz->nombre_interfaz_largo), stream, sizeof(int));
    stream += sizeof(int);

    interfaz->nombre_interfaz = malloc(interfaz->nombre_interfaz_largo);

    memcpy(interfaz->nombre_interfaz, stream, interfaz->nombre_interfaz_largo);
    stream += interfaz->nombre_interfaz_largo;
    memcpy(&(interfaz->tipo), stream, sizeof(TipoInterfaz));
    stream += sizeof(TipoInterfaz);

    return interfaz;
}

t_paquete* inicializarIO_recibirPaquete(int socket) {
    printf("Conexion establecida con I/O en sockets...\n");

    t_paquete *paquete = malloc(sizeof(t_paquete));
    if (!paquete) {
        perror("No hay paquete\n");
        return NULL;
    }
    paquete->buffer = malloc(sizeof(t_buffer));
    if (!paquete->buffer) {
        perror("Error\n");
        free(paquete);
        return NULL;
    }
    printf("Esperando recibir paquete de IO\n");
    
    recv(socket, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
    printf("Recibi el codigo de operacion de IO: %d\n", paquete->codigo_operacion);
    
    recv(socket, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

    printf("Recibe el size de IO: %d\n", paquete->buffer->size);

    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);
    printf("Recibi el buffer de IO\n");

    if (!paquete->buffer->stream) {
        perror("Error allocating memory for paquete buffer stream\n");
        free(paquete->buffer);
        free(paquete);
        return NULL;
    }
    
    return paquete;
}

void imprimir_datos_stdin(t_pid_stdin* datos_stdin) {
    printf("Cantidad de paginas: %d\n", datos_stdin->cantidad_paginas);
    printf("Registro tamanio: %d\n", datos_stdin->registro_tamanio);
    printf("Cantidad de direcciones: %d\n", list_size(datos_stdin->lista_direcciones));
    for(int i = 0; i < list_size(datos_stdin->lista_direcciones); i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = list_get(datos_stdin->lista_direcciones, i);
        printf("Direccion fisica: %d\n", dir_fisica_tam->direccion_fisica);
        printf("Tamanio: %d\n", dir_fisica_tam->bytes_lectura);
    }
}

// Lo uso para evitar el error de EXIT en cuatro lineas distintas al final
void aplicar_sobre_cada_linea_del_archivo(FILE* file, void* datos_extra, void(*closure)(void*, void*)) {
    char* line = NULL;
    char* aux_line = string_new();
    size_t bufsize = 0;

    while((getline(&line, &bufsize, file)) != -1) {
        printf("Esta es la linea: %s\n",line);
        if(string_ends_with(line, "\n")) {
            closure(datos_extra, line);
        } else {
            string_append(&aux_line, line);
        }
    }

    if(!string_is_empty(aux_line)) {
        closure(datos_extra, aux_line);
    }

    free(line);
    free(aux_line);
}

t_buffer *buffer_create(int size){
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer -> size = size;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
    return buffer;
}

// Crea un buffer vacío de tamaño size y offset 0
t_buffer* buffer_create(int size) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = size;
    buffer->offset = 0;
    buffer->stream = malloc(size);
    return buffer;
}

// Libera la memoria asociada al buffer
void buffer_destroy(t_buffer *buffer) {
    free(buffer->stream);
    free(buffer);
}

// Agrega un stream al buffer en la posición actual y avanza el offset
void buffer_add(t_buffer *buffer, void *data, int size) {
    memcpy((uint8_t *)buffer->stream + buffer->offset, data, size);
    buffer->offset += size;
}

// Guarda size bytes del principio del buffer en la dirección data y avanza el offset
void buffer_read(t_buffer *buffer, void *data, int size) {
    memcpy(data, (uint8_t *)buffer->stream + buffer->offset, size);
    buffer->offset += size;
}

// Agrega un uint32_t al buffer
void buffer_add_uint32(t_buffer *buffer, uint32_t data) {
    buffer_add(buffer, &data, sizeof(uint32_t));
}

// Lee un uint32_t del buffer y avanza el offset
uint32_t buffer_read_uint32(t_buffer *buffer) {
    uint32_t data;
    buffer_read(buffer, &data, sizeof(uint32_t));
    return data;
}

// Agrega un uint8_t al buffer
void buffer_add_uint8(t_buffer *buffer, uint8_t data) {
    buffer_add(buffer, &data, sizeof(uint8_t));
}

// Lee un uint8_t del buffer y avanza el offset
uint8_t buffer_read_uint8(t_buffer *buffer) {
    uint8_t data;
    buffer_read(buffer, &data, sizeof(uint8_t));
    return data;
}

// Agrega string al buffer con un uint32_t adelante indicando su longitud
void buffer_add_string(t_buffer *buffer, uint32_t length, char *string) {
    buffer_add_uint32(buffer, length);
    buffer_add(buffer, string, length);
}

// Lee un string y su longitud del buffer y avanza el offset
char *buffer_read_string(t_buffer *buffer, uint32_t *length) {
    *length = buffer_read_uint32(buffer);
    char *string = malloc(*length);
    buffer_read(buffer, string, *length);
    return string;
}

void buffer_add_int(t_buffer *buffer, int data) {
    buffer_add(buffer, &data, sizeof(int));
}

// Lee un int del buffer y avanza el offset
int buffer_read_int(t_buffer *buffer) {
    int data;
    buffer_read(buffer, &data, sizeof(int));
    return data;
}
