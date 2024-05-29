#include "../include/conexionesCPU.h"

int socket_memoria;
int client_dispatch; //socket_kernel

int conectar_memoria(char* IP_MEMORIA, char* puerto_memoria, t_log* logger_CPU) {
    int valor = 2;
    // int message_cpu = 10;

    int memoriafd = crear_conexion(IP_MEMORIA, puerto_memoria, valor);
    log_info(logger_CPU, "Conexion establecida con Memoria");
    
    // send(memoriafd, &message_cpu, sizeof(int), 0); // Me conecto y envio un mensaje a memoria

    sleep(3);
    return memoriafd;
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
        log_info(logger_cpu, "Se conecto un cliente de dispatch!\n");
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

    client_dispatch = esperar_cliente(socket_cpu_escucha, datos->logger); 
    log_info(datos->logger, "Esperando cliente...");

    // Aca recibo al cliente (este es mi while(1))
    recibir_cliente();
    
    return NULL;
}

void cargar_registros_en_cpu(t_registros* registros_pcb) {
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

void recibir_cliente() { // Se supone que desde aca se conecta el kernel
    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        printf("Esperando recibir paquete (por dispatch)\n");
        recv(client_dispatch, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion por dispatch: %d\n", paquete->codigo_operacion);

        //recv(client_dispatch, &(paquete->buffer->size), sizeof(int), 0);
        recv(client_dispatch, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);

        //recv(client_dispatch, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(client_dispatch, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);
        // codigo_operacion = recibir_operacion(client_dispatch);

        switch (paquete->codigo_operacion) {
            case ENVIO_PCB:
                t_pcb* pcb = deserializar_pcb(paquete->buffer);
                cargar_registros_en_cpu(pcb->registros);
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

    recibir_cliente_interrupt(client_interrupt);
    return NULL;
}

void recibir_cliente_interrupt(int client_interrupt) {
    codigo_operacion cod_op;
    while(1) {
        printf("Esperando recibir paquete (interrupt)\n");
        recv(client_interrupt, &cod_op, sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion por interrupt: %d\n", cod_op);

        // Posible if si no hay mas codigos de operacion para interrumpir
        switch (cod_op) {
            case INTERRUPCION_CPU:
                hay_interrupcion = 1;
                break;         
            default:
                break;
        }
    }
}

t_config_cpu* iniciar_datos(char* escucha_fd, t_log* logger_CPU) {
    // Iniciacion de datos

    t_config_cpu* cpu_server = malloc(sizeof(t_config_cpu));
    cpu_server->puerto_escucha = escucha_fd;
    cpu_server->logger = logger_CPU;

    return cpu_server;
}
