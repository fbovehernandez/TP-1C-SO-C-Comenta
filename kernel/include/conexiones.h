#ifndef CONEXIONES_H
#define CONEXIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include "planificador.h"

extern t_dictionary* diccionario_io;
extern t_list* lista_io;
extern t_list* lista_procesos;
extern sem_t sem_cola_io;
extern pthread_mutex_t mutex_lista_io;
extern pthread_mutex_t mutex_cola_io_generica;
extern pthread_mutex_t mutex_cola_io_stdout;
extern pthread_mutex_t mutex_cola_io_stdin;

extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_blocked;
// extern t_pcb* pcb_exec;
extern t_queue* cola_exec;
extern t_queue* cola_exit;
extern t_queue* cola_prioritarios_por_signal;
extern t_queue* cola_ready_plus;

extern pthread_mutex_t mutex_estado_new;
extern pthread_mutex_t mutex_estado_ready;
extern pthread_mutex_t mutex_estado_exec;
extern pthread_mutex_t mutex_estado_blocked;
extern pthread_mutex_t mutex_estado_ready_plus;
extern pthread_mutex_t mutex_prioritario_por_signal;
extern pthread_mutex_t no_hay_nadie_en_cpu;


// Aca va el struct del PCB 
typedef struct {
    char* puerto_escucha;
    int socket;
    t_log* logger;
} t_config_kernel;

int conectar_kernel_memoria(char* ip, char* puerto, t_log* logger_kernel);
int conectar_kernel_cpu_dispatch(t_log* logger_kernel, char* ip, char* puerto);
int conectar_kernel_cpu_interrupt(t_log* logger_kernel,char* IP_CPU,char* puerto_cpu_dispatch);
int esperar_cliente(int socket_escucha, t_log* logger);
void* handle_io_stdout(void* socket);
void* handle_io_stdin(void* socket);
void* handle_io_dialfs(void* socket);
int ejecutar_io_stdin(int socket, t_pid_stdin* pid_stdin);
int ejecutar_io_stdout(t_pid_stdout* pid_stdout);
void* handle_io_generica(void* socket);
int ejecutar_io_generica(int socket_io, t_pid_unidades_trabajo* pid_unidades_trabajo);
int recibir_operacion(int socket_cliente);
void liberar_conexion(int socket_cliente);
// void escuchar_STDOUT(t_config* config_kernel, t_log* logger_kernel);
void* escuchar_IO(void* kernel_io);
ptr_kernel* solicitar_datos(t_config* config_kernel);
void solicitar_nombre_io(int socket);
t_info_io* deserializar_interfaz(t_buffer* buffer);
t_list_io* establecer_conexion(t_buffer *buffer, int socket_io);
void mostrar_elem_diccionario(char* nombre_interfaz);
t_pcb* sacarDe(t_queue* cola, int pid);
pthread_mutex_t* obtener_mutex_de(t_queue* cola);
void liberarIOyPaquete(t_paquete *paquete, t_list_io *io);
void liberar_io(t_list_io* io);
void liberar_arrays_recurso(char** recursos, char** instancias_recursos);
t_pcb* sacarDe(t_queue* cola, int pid);
pthread_mutex_t* obtener_mutex_de(t_queue* cola);
#endif // CONEXIONES_H