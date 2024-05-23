#include "../include/conexionesCPU.h"

int socket_memoria;

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
    // int message_cpu = 10;

    int memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria, valor);
    log_info(logger_CPU, "Conexion establecida con Memoria");
    
    // send(memoriafd, &message_cpu, sizeof(int), 0); // Me conecto y envio un mensaje a memoria

    sleep(3);
    return memoriafd;
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

// Es esta funcion la que se encarga de recibir el codigo de operacion y a partir de eso actuar? Tiene codop = recibirOperacion()!!!!!
void* iniciar_servidor_dispatch(void* datos_dispatch) {
    t_config_cpu* datos = (t_config_cpu*) datos_dispatch;
    int socket_cpu_escucha = iniciar_servidor(datos->puerto_escucha); // Inicia el servidor, bind() y listen() y socket()
    log_info(datos->logger, "Servidor iniciado, esperando conexiones!");

    int client_dispatch = esperar_cliente(socket_cpu_escucha, datos->logger); 
    log_info(datos->logger, "Esperando cliente...");

    // Aca recibo al cliente (este es mi while(1))
    recibir_cliente(client_dispatch);
    
    return NULL;
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

void cargar_registros(t_registros* registros_pcb) {
    registros_cpu->AX = registros_pcb->AX;
    registros_cpu->BX = registros_pcb->BX;
    registros_cpu->CX = registros_pcb->CX;
    registros_cpu->DX = registros_pcb->DX;

    registros_cpu->EAX = registros_pcb->EAX;
    registros_cpu->EBX = registros_pcb->EBX;
    registros_cpu->ECX = registros_pcb->ECX;
    registros_cpu->EDX = registros_pcb->EDX;

    registros_cpu->SI = registros_pcb->SI;
    registros_cpu->DI = registros_pcb->DI;
}

void recibir_cliente(int client_dispatch) {
    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        printf("Esperando recibir paquete\n");
        recv(client_dispatch, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion : %d\n", paquete->codigo_operacion);

        //recv(client_dispatch, &(paquete->buffer->size), sizeof(int), 0);
        recv(client_dispatch, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);

        //recv(client_dispatch, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(client_dispatch, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);
        // codigo_operacion = recibir_operacion(client_dispatch);

        switch (paquete->codigo_operacion) {
            case ENVIO_PCB:
                t_pcb* pcb = deserializar_pcb(paquete->buffer);
                cargar_registros(pcb->registros);
                imprimir_pcb(pcb);
                ejecutar_pcb(pcb, socket_memoria); // este ejecutar_pcb(pcb) seria el ejectuar_instrucciones(pcb)
                break;
            default:
                break;
        }

        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }
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

// Hace lo mismo que dispatch pero con interrupt (POR AHORA)
void* iniciar_servidor_interrupt(void* datos_interrupt) {
    t_config_cpu* datos = (t_config_cpu*) datos_interrupt;
    int socket_cpu_escucha = iniciar_servidor(datos->puerto_escucha); // Inicia el servidor, bind() y listen() y socket()
    int client_interrupt = esperar_cliente(socket_cpu_escucha, datos->logger); 
    int codop;

    while(client_interrupt != -1) {    
        // int client_dispatch = esperar_cliente(socket_cpu_escucha, datos->logger);
        codop = recibir_operacion(client_interrupt);
        switch(codop) {
            case 17:
                printf("Recibi 17\n");
                break;
            default:
                printf("entro por default con codop: %d\n", codop);
                return NULL;
        }
    }

    return NULL;
}

t_config_cpu* iniciar_datos(char* escucha_fd, t_log* logger_CPU) {
    // Iniciacion de datos

    t_config_cpu* cpu_server = malloc(sizeof(t_config_cpu));
    cpu_server->puerto_escucha = escucha_fd;
    cpu_server->logger = logger_CPU;

    return cpu_server;
}
