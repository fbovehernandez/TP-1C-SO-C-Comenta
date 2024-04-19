#include "../include/conexionesCPU.h"

/* 

PUERTO_KERNEL=8010
IP_KERNEL=127.0.0.1
PUERTO_ESCUCHA_DISPATCH=8006
PUERTO_ESCUCHA_INTERRUPT=8007
IP_MEMORIA=127.0.0.1
PUERTO_MEMORIA=8002
CANTIDAD_ENTRADAS_TLB=32
ALGORITMO_TLB=FIFO

*/
int conectar_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_CPU) {
    int valor = 2;
    int message_cpu = 10;

    int memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria, valor);
    log_info(logger_CPU, "Conexion establecida con Memoria");
    
    send(memoriafd, &message_cpu, sizeof(int), 0); // Me conecto y envio un mensaje a memoria

    sleep(5);
    return memoriafd;
}


void* handle_dispatch(void* socket_dispatch) {
    int socket_kernel = (intptr_t)socket_dispatch;
    // despues veremos que hace
    free(socket_dispatch);
    printf("Gestionando dispatcher\n");  
    int cod_op;
    
    while(1) {
        cod_op = recibir_operacion(socket_kernel);
        switch(cod_op) {
            case 10:
                printf("Se recibio 10 en dispatcher\n");
                break;
            case 7:
                printf("Se recibio 7 en dispatcher\n");
                break;
            default:
                printf("Rompio todo en dispatcher?\n");
                close(socket_kernel);
                return NULL;
        }
    }
    return NULL;
}

void* handle_interrupt(void* socket) {
    printf("Todavia no hacemos nada");
    return NULL;
}

// Modificar para que me devuelva un struct con los sockets y el sv_type
int esperar_cliente(int socket_servidor, t_log *logger_cpu) {
    uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if(handshake == 1) {
		send(socket_cliente, &resultOk, sizeof(uint32_t), 0);
        log_info(logger_cpu, "Se conecto un cliente de dispatch!");
    } else {
        send(socket_cliente, &resultError, sizeof(uint32_t), 0);
    }
    
	return socket_cliente;
}

void* iniciar_servidor_dispatch(void* datos_dispatch) {
    t_config_cpu* datos = (t_config_cpu*) datos_dispatch;
    int socket_cpu_escucha = iniciar_servidor(datos->puerto_escucha); // Inicia el servidor, bind() y listen() y socket()
    // chicos, les parece bien que se quede esperando a nuevos clientes?
    int client_dispatch = esperar_cliente(socket_cpu_escucha, datos->logger); // TODO -> ACA CREO OTRO HILO Y AHI VVEO EL WHILE(1)

    int codop;
    while(client_dispatch != -1) {    
        // int client_dispatch = esperar_cliente(socket_cpu_escucha, datos->logger);
        codop = recibir_operacion(client_dispatch);
        switch(codop) {
            case 10:
                printf("Recibi 10\n");
                break;
            default:
                printf("entro por default con codop: %d\n", codop);
                return NULL;
        }
    }

    return NULL;
}