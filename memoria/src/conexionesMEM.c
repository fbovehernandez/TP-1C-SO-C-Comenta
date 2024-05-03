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

void* handle_cpu(void* socket) {
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

void* handle_kernel(void* socket) {
    int socket_kernel = (intptr_t)socket;
    // free(socket); 
    int resultOk = 0;
    send(socket_kernel, &resultOk, sizeof(int), 0);
    printf("Se conecto el kernel!\n");

    int cod_op;
    while(1) {
        cod_op = recibir_operacion(socket_kernel);
        switch(cod_op) {
            case 10:
                printf("Se recibio 10\n");
                break;
            case 7:
                printf("Se recibio 7\n");
                break;
            case 0:
                printf("Recibo el path");
                // recibir_peticion_de_kernel(socket_kernel);
            default:
                printf("Rompio todo?\n");
                // close(socket_kernel);
                return NULL;
        }
    }
    return NULL;
}

void* recibir_peticion_de_kernel(int socket_kernel) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    // Primero recibimos el codigo de operacion
    recv(socket_kernel, &(paquete->codigo_operacion), sizeof(uint8_t), 0);

    // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
    recv(socket_kernel, &(paquete->buffer->size), sizeof(uint32_t), 0);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, 0);

    // Ahora en función del código recibido procedemos a deserializar el resto
    switch(paquete->codigo_operacion) {
        case PATH:
            t_persona* persona = persona_serializar(paquete->buffer);
            ...
            // Hacemos lo que necesitemos con esta info
            // Y eventualmente liberamos memoria
            free(persona);
            ...
            break;
        ... // Evaluamos los demás casos según corresponda
    }

    // Liberamos memoria
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}