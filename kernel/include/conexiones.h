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

// Aca va el struct del PCB 
typedef struct {
    char* puerto_escucha;
    int socket;
    t_log* logger;
} t_config_kernel;

typedef struct {
    int socket_memoria;
    int socket_cpu;
    int socket_int;
} t_sockets;

typedef struct {
    char* ip_cpu;
    char* ip_mem;
    char* puerto_memoria;
    char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;
    char* puerto_io;
    int quantum;
    int grado_multiprogramacion;   
    char* algoritmo_planificacion;
    t_dictionary* diccionario_recursos;
} ptr_kernel;

typedef struct {
    int instancias;
    t_queue* procesos_bloqueados;
} t_recurso;

int conectar_kernel_memoria(char* ip, char* puerto, t_log* logger_kernel);
int conectar_kernel_cpu_dispatch(t_log* logger_kernel, char* ip, char* puerto);
int conectar_kernel_cpu_interrupt(t_log* logger_kernel,char* IP_CPU,char* puerto_cpu_dispatch);
int esperar_cliente(int socket_escucha, t_log* logger);
void* handle_io_stdout(void* socket);
void* handle_io_stdin(void* socket);
int ejecutar_io_stdin(int socket, t_pid_stdin* pid_stdin);
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
void liberarIOyPaquete(t_paquete *paquete, t_list_io *io);
void liberar_io(t_list_io* io);

#endif // CONEXIONES_H