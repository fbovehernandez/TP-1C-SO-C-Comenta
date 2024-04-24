#include "../include/funcionalidades.h"


uint32_t obtener_pid(uint32_t* contador_pid) {
    (*contador_pid)++;
    return contador_pid;
}

void INICIAR_PROCESO(int contador_pid) {
    uint32_t pid = obtener_pid(contador_pid); 
    t_pcb* pcb_process = crear_pcb(pid);
    encolar_proceso_en_new(pid);
}


t_pcb *crear_pcb(uint32_t pid) {
    t_pcb *pcb = malloc(sizeof(*pcb));
    
    pcb->pid = pid;
    pcb->instrucciones = NULL;
    pcb->program_counter = 0; // Esto es el numero de instruccion? no la direccion logica?
    pcb->quantum = 0; // Esto lo traigo despues del config
    pcb->registros = registros_cpu_create();
    
    pcb->estadoActual = NEW;
    pcb->estadoAnterior = NEW;

    return pcb;
}

void* escuchar_consola(int socket_kernel_escucha, t_config* config){
	while(1){

		pthread_t thread_type;
		int cliente_fd = esperar_cliente(socket_kernel_escucha);

		void* funcion_recibir_cliente(void* cliente_fd){
			log_info(logger, "Conectado con una consola");
			recibir_cliente(cliente_fd, config);// post recibiendo perdemos las cosas
			return NULL;
		}
		pthread_create(&thread_type, NULL, funcion_recibir_cliente, (void*) &cliente_fd);
	}
    
    return NULL;
}

t_proceso* recibir_cliente(void* input, t_config* config, t_log* logger) {
    t_pcb* nuevo_pcb = malloc(sizeof(t_pcb)); // OJO con que no nos pase q se va el espacio de mem al salir del contexto de la llamada
	//printf("voy a recibir el pcb de consola\n");
	int* cliente_fd = (int *) input;
	void* buffer_instrucciones;
	int paquete = recibir_operacion(*cliente_fd);
    log_info(logger, "Recibidas instrucciones de la consola");
    buffer_instrucciones = recibir_instrucciones(*cliente_fd);
//----------------------Planificador a largo plazo---- agrega a cola de new
    sem_wait(&mutex_cola_new);
    crear_header(proximo_pid, buffer_instrucciones, config, nuevo_pcb, estimacion_inicial);
    log_info(logger, "Creado PCB correspondiente");
    nuevo_pcb->socketProceso = *cliente_fd;
    queue_push(cola_new, (void*) nuevo_pcb);
    proximo_pid++;
    log_info(logger, "Agregado a la cola de nuevo");
    sem_post(&mutex_cola_new);
    sem_post(&cantidad_listas_para_estar_en_ready);
	return NULL;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_pcb_de_cpu(void* nada, t_log* logger){
	uint32_t finalizar = 101;

	while(1){
		//printf("espero a recibir op\n");
		long io[2];
		int dato;
		int fin_proc = FINALIZAR_PROCESO;
		int rta;
		t_pcb* proceso_recibido = malloc(sizeof(t_pcb));
		int codigo_de_paquete = recibir_operacion(conexion_CPU_dispatch);
		log_info(logger, "Operación recibida: %d", codigo_de_paquete);
		switch(codigo_de_paquete) {
			case OPERACION_IO:
				dato = recibir_operacion(conexion_CPU_dispatch);
				recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

				io[0] = (long)dato;
				io[1] = currentTimeMillis();
				char* string_pid = string_itoa(proceso_recibido->pid);
				proceso_recibido->real_actual = io[1] - proceso_recibido->timestamp_inicio_exe;
				log_info(logger, "vengo a hacer io: %d, mi nuevo real actual es: %ld", proceso_recibido->pid, proceso_recibido->real_actual);
				sem_wait(&mutex_cola_suspendido);

				dictionary_put(pid, string_pid, (void*) io);
				queue_push(cola_io, proceso_recibido);
				sem_post(&mutex_cola_suspendido);

				dictionary_put(estado_proceso, string_pid, "blocked");
				sem_post(&signal_a_io);
				sem_post(&binario_plani_corto);//aca esta el error ahora esta distito
				//printf("recibir intento planificar\n");
				break;

			case OPERACION_EXIT:
				recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

				send(conexion_memoria, &fin_proc, sizeof(uint32_t), 0);  //MANDAR A MEMORIA QUE FINALIZA EL PROCESO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				send(conexion_memoria, &proceso_recibido->tabla_paginas, sizeof(uint32_t), 0);  
				send(conexion_memoria, &proceso_recibido->tamanio_en_memoria, sizeof(uint32_t), 0); 
				recv(conexion_memoria, &rta, sizeof(uint32_t), MSG_WAITALL); 
				
				send(proceso_recibido->socket_consola, &finalizar, sizeof(uint32_t), 0); //aviso a la consola que termino su proceso
				log_info(logger, "finalizo el proceso: %d", proceso_recibido->pid);
				liberar_pcb(proceso_recibido);
				sem_post(&sem_contador_multiprogramacion);
				sem_post(&binario_plani_corto);
				break;

			case RESPUESTA_INTERRUPT:
				flag_respuesta_a_interrupcion = 1;
				proceso_recibido = malloc(sizeof(t_pcb));
				recibir_pcb(conexion_CPU_dispatch, proceso_recibido);

				log_info(logger, "El proceso %d fue interrumpido y su est es originalmente:%ld", proceso_recibido->pid, proceso_recibido->estimacion_siguiente);

				proceso_recibido->real_actual += currentTimeMillis() - proceso_recibido->timestamp_inicio_exe;
				sem_wait(&mutex_respuesta_interrupt);
				queue_push(respuesta_interrupcion, (void*) proceso_recibido);
				sem_post(&mutex_respuesta_interrupt);
				sem_post(&binario_flag_interrupt);

				break;
			case CPU_LIBRE:
				//printf("caso vacio\n");
				flag_respuesta_a_interrupcion = 2;
				sem_post(&binario_flag_interrupt);

				break;
			}
	}
 return NULL;
}
