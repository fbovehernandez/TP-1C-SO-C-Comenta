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


/*void* handle_dispatch(void* socket_dispatch) {
    int socket_kernel = (intptr_t)socket_dispatch;
    // despues veremos que hace
    free(socket_dispatch);
    printf("Gestionando dispatcher\n");  
    int cod_op;
    
    while(1) {
        cod_op = recibir_operacion(socket_kernel);
        switch(cod_op) {
            case 1:
                recibir_pcb_de_kernel(socket_kernel);
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
}*/

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

    int client_dispatch = esperar_cliente(socket_cpu_escucha, datos->logger); // TODO -> ACA CREO OTRO HILO Y AHI VEO EL WHILE(1)

    int codop;
    while(client_dispatch != -1) {    
        // int client_dispatch = esperar_cliente(socket_cpu_escucha, datos->logger);
        codop = recibir_operacion(client_dispatch);
        switch(codop) {
            //Borramos el case 10: Estamos intentando con el envio_pcb
            case ENVIO_PCB:
                recibir_pcb_de_kernel(client_dispatch);
                break;
            default:
                printf("entro por default: %d\n", codop);
                return NULL;
        }
    }

    return NULL;
}

// Hace lo mismo que dispatch pero con interrupt (POR AHORA)
void* iniciar_servidor_interrupt(void* datos_interrupt) {
    t_config_cpu* datos = (t_config_cpu*) datos_interrupt;
    int socket_cpu_escucha = iniciar_servidor(datos->puerto_escucha); // Inicia el servidor, bind() y listen() y socket()
    int client_interrupt = esperar_cliente(socket_cpu_escucha, datos->logger); // TODO -> ACA CREO OTRO HILO Y AHI VEO EL WHILE(1)

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

void* recibir_pcb_de_kernel(int socket_kernel) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    //Ya sacamos el codigo de operacion en iniciar_servidor_dispatch, y es por eso que estamos en esta funcion.

    // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
    recv(socket_kernel, &(paquete->buffer->size), sizeof(paquete->buffer->size), 0);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, 0);

    // Ahora en función del código recibido procedemos a deserializar el resto
    t_pcb* pcb = pcb_deserializar(paquete->buffer);
    
    // Hacemos lo que necesitemos con esta info
    // Y eventualmente liberamos memoria
    free(pcb);
    
     // Evaluamos los demás casos según corresponda

    // Liberamos memoria
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);

    return 0;
}

//TO DO: Toda la funcion
t_pcb* pcb_deserializar(t_buffer* buffer) {
   /* t_pcb* pcb = malloc(sizeof(t_pcb));
    Registros* registros = malloc(sizeof(Registros));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(pcb->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->program_counter), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->quantum), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->estadoActual), stream, sizeof(enum Estado));
    stream += sizeof(enum Estado);
    memcpy(&(pcb->estadoAnterior), stream, sizeof(enum Estado));
    stream += sizeof(enum Estado);
    memcpy(&(pcb->registros), stream, sizeof(registros));
    stream += sizeof(registros);

    // Por último, para obtener el nombre, primero recibimos el tamaño y luego el texto en sí:
    memcpy(&(persona->nombre_length), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    persona->nombre = malloc(persona->nombre_length);
    memcpy(persona->nombre, stream, persona->nombre_length);*/
   
    return pcb;
}
