#ifndef FUNCIONALIDADES_H
#define FUNCIONALIDADES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "conexiones.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/collections/queue.h>
#include <pthread.h>

extern int client_dispatch;

typedef struct {
    
} t_instrucciones;

// Me lleve t_pcb al utils, lo mismo que Registros y Estado

// Los 2 de abajo no los usamos todavia
typedef struct {
    int socket_consola;
    int pid;
    int tabla_paginas;
    int tamanio_en_memoria;
} t_proceso;

/*
typedef struct {
	t_link_element *head;
	int elements_count;
} t_list;
*/

//Si no podemos usar bien las variables globales, va a tener que por consola
/*typedef struct {
    int contador_pid;
    int quantum;
    t_config* config_kernel;
    t_log* logger_kernel;
    int socket_memoria_kernel; // var global
    int socket_cpu; // var global
} t_consola;*/

extern int grado_multiprogramacion;
extern int quantum;
extern int contador_pid;  

extern t_sockets* sockets;

extern pthread_mutex_t mutex_estado_new;
extern pthread_mutex_t mutex_estado_ready;
extern pthread_mutex_t mutex_estado_exec;
extern pthread_mutex_t mutex_estado_blocked;
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_hay_pcb_esperando_ready;
extern sem_t sem_hay_para_planificar;
extern pthread_mutex_t no_hay_nadie_en_cpu;
extern sem_t sem_contador_quantum;

extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_blocked;
// extern t_queue* cola_exec;
extern t_pcb* pcb_exec;
extern t_queue* cola_exit;
extern t_queue* cola_prioritarios_por_signal;

extern char* algoritmo_planificacion; // Tomamos en convencion que los algoritmos son "FIFO", "VRR" , "RR" (siempre en mayuscula)
extern t_log* logger_kernel;
extern char* path_kernel;
extern ptr_kernel* datos_kernel;

// Funciones
void* interaccion_consola();
void encolar_a_new(t_pcb* pcb);
void* a_ready(); // Para hilo 
t_pcb* crear_nuevo_pcb(int pid);
void* contar_quantum(void* socket_CPU);
t_registros* inicializar_registros_cpu();
void print_queue(Estado estado);
int obtener_siguiente_pid();
// t_queue* mostrar_cola(t_queue* cola);
void mostrar_pcb_proceso(t_pcb* pcb);
void* planificar_corto_plazo(void* sockets_kernel);
void* enviar_path_a_memoria(char* path, t_sockets* sockets, int pid);
t_buffer* llenar_buffer_path(t_path* pathNuevo);
t_pcb* proximo_a_ejecutar();
void esperar_cpu();
void mandar_datos_io(char* interfaz_nombre, uint32_t registro_direccion, uint32_t registro_tamanio);
t_operacion_io* deserializar_io(t_buffer* buffer);
t_paquete* recibir_cpu();
// void liberar_memoria(t_pcb* pcb, int pid);
void dormir_io(t_operacion_io* operacion_io, t_pcb* pcb);
void hilo_dormir_io(t_operacion_io* operacion_io);
void change_status(t_pcb* pcb, Estado new_status);
bool match_nombre(char* interfaz);
t_list_io* validar_io(t_operacion_io* operacion_io, t_pcb* pcb);
void ejecutarComando(char* linea_leida);
void *esperar_RR(void *sockets_Int) ;
// t_pcb* sacarDe(t_queue* cola, int pid);
// t_queue* encontrar_en_que_cola_esta(int pid);
// int queue_find(t_queue* cola, int pid);
// t_pcb* sacarPCBDeDondeEste(int pid);

t_parametro* deserializar_parametro(t_buffer* buffer);
t_pedido* deserializar_pedido(t_buffer* buffer);
void wait_recurso(t_pcb* pcb, char* recurso);
void signal_recurso(t_pcb* pcb, char* recurso);
// t_pedido_escritura* deserializar_pedido_escritura(t_buffer* buffer);
void mandar_a_escribir_a_memoria(char* nombre_interfaz, int direccion_fisica, uint32_t tamanio);
t_buffer* llenar_buffer_stdout(int direccion_fisica,char* nombre_interfaz, int tamanio);
void encolar_datos_std(t_pcb* pcb, t_pedido* pedido);
t_pedido_fs_create_delete* deserializar_pedido_fs_create(t_buffer* buffer);
void mandar_pedido_fs();
t_list_io* io_esta_en_diccionario(t_pcb* pcb, char* interfaz_nombre);
t_buffer* llenar_buffer_nombre_archivo_pid(int pid,int largo_archivo,char* nombre_archivo);
void enviar_buffer_fs(int socket_io,int pid,int longitud_nombre_archivo,char* nombre_archivo, codigo_operacion codigo_operacion);
// void imprimir_datos_stdin(io_stdin* datos_stdin);

void EJECUTAR_SCRIPT(char* path);
void INICIAR_PROCESO(char* path);
void FINALIZAR_PROCESO(int pid);
void INICIAR_PLANIFICACION();
void DETENER_PLANIFICACION();
void PROCESO_ESTADO();
void MULTIPROGRAMACION(int valor);

#endif