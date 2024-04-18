#include "../include/conexionesMEM.h"

//Acepta el handshake del cliente, se podria hacer mas generico y que cada uno tenga un valor diferente
int esperar_cliente(int socket_servidor, t_log* logger_memoria)
{
	int handshake = 0;
	// int resultOk = 0;
	int resultError = -1;
	int socket_cliente = accept(socket_servidor, NULL, NULL);

    if(socket_cliente == -1) {
        return -1;
    }
    
	recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL); 
	if(handshake == 1) {
        pthread_t kernel_thread;
        pthread_create(&kernel_thread, NULL, (void*)handle_kernel, (void*)(intptr_t)socket_cliente);
        pthread_detach(kernel_thread);
    } else if(handshake == 2) {
        pthread_t cpu_thread;
        pthread_create(&cpu_thread, NULL, (void*)handle_cpu, (void*)(intptr_t)socket_cliente);
        pthread_detach(cpu_thread);
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        close(socket_cliente);
        return -1;
    }
	return socket_cliente;
}

void* handle_cpu(void* socket) {
    int socket_cpu = (intptr_t)socket;
    int resultOk = 0;
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
            default:
                printf("Rompio todo?\n");
                // close(socket_kernel);
                return NULL;
        }
    }
    return NULL;
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

// Posible futura abstraccion: atender(t_config_memoria* memory_struct, char* modulo_atendido)

void liberar_conexion(int socket_cliente) {
	close(socket_cliente);
}