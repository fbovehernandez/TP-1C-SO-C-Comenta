#include "../include/conexionesMEM.h"

pthread_t cpu_thread;
pthread_t kernel_thread;

//Acepta el handshake del cliente, se podria hacer mas generico y que cada uno tenga un valor diferente
int esperar_cliente(int socket_servidor, t_log* logger_memoria)
{
	int handshake = 0;
	int resultError = -1;
	int socket_cliente = accept(socket_servidor, NULL, NULL);

    if(socket_cliente == -1) {
        return -1;
    }
    
	recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL); 
	if(handshake == 1) {
        // pthread_t kernel_thread;
        pthread_create(&kernel_thread, NULL, (void*)handle_kernel, (void*)(intptr_t)socket_cliente);
        
    } else if(handshake == 2) {
        // pthread_t cpu_thread;
        pthread_create(&cpu_thread, NULL, (void*)handle_cpu, (void*)(intptr_t)socket_cliente);
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        close(socket_cliente);
        return -1;
    }

	return socket_cliente;
}

void* handle_cpu(void* socket) { // Aca va a pasar algo parecido a lo que pasa en handle_kernel, se va a recibir peticiones de cpu y se va a hacer algo con ellas (iternado switch)
    int socket_cpu = (intptr_t)socket;
    // free(socket); 
    int resultOk = 0;
    // Envio confirmacion de handshake!
    send(socket_cpu, &resultOk, sizeof(int), 0);
    printf("Se conecto un el cpu!\n");

    int cod_op;
    while(socket_cpu != -1) {
        cod_op = recibir_operacion(socket_cpu);
        switch(cod_op) {
            case 10:
                printf("Se recibio 10\n"); 
                break;
            case 7:
                printf("Se recibio 7\n");
                break;
            default:
                printf("Rompio todo?\n");
                // close(socket_cpu); NO cierro conexion
                return NULL;
        }
    }
    return NULL;
}

// Ver error
void* handle_kernel(void* socket) {
    int socket_kernel = (intptr_t)socket;
    // free(socket); 
    int resultOk = 0;
    send(socket_kernel, &resultOk, sizeof(int), 0);
    printf("Se conecto el kernel!\n");

    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete\n");
        recv(socket_kernel, &(paquete->codigo_operacion), sizeof(int), 0);
        printf("Recibi el codigo de operacion : %d\n", paquete->codigo_operacion);

        // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
        recv(socket_kernel, &(paquete->buffer->size), sizeof(int), 0);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, 0);

        // Ahora en función del código recibido procedemos a deserializar el resto
        switch(paquete->codigo_operacion) {
            case PATH:
                t_path* path = deserializar_path(paquete->buffer);
                imprimir_path(path);
                // crear_estructuras();
                break;
            default:
                break;
        }

        // Liberamos memoria
        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }

    return NULL;
}

t_path* deserializar_path(t_buffer* buffer) {
    t_path* path = malloc(sizeof(t_path));
    
    void* stream = buffer->stream;

    memcpy(&(path->PID), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(path->path_length), stream, sizeof(int));
    stream += sizeof(int);
    
    // Reservo memoria para el path
    path->path = malloc(path->path_length);
    memcpy(path->path, stream, path->path_length);
    path->path[path->path_length - 1] = '\0'; // Es necesario?

    // La posta es que toque un par de cosas en lo de seralizar con mem dinamica,
    // pero enrealidad fui viendo que era diferente del de la catedra je, asi
    // que no se como funciona, pero funciona :)

    return path;

}

void imprimir_path(t_path* path) {
    printf("PID: %d\n", path->PID);
    printf("Path length: %d\n", path->path_length);
    printf("Path: %s\n", path->path);
}
