#ifndef FUNCIONALIDADES_H
#define FUNCIONALIDADES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <readline/readline.h>
#include <semaphore.h>
#include "conexiones.h"
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/collections/queue.h>
#include <pthread.h>

extern pthread_t escucha_consola;

extern int client_dispatch;
extern t_temporal* timer;
extern int ms_transcurridos;

extern pthread_t hilo_detener_planificacion;
extern int cantidad_bloqueados;

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
extern int quantum_config;
extern int contador_pid;  

extern pthread_mutex_t mutex_estado_new;
extern pthread_mutex_t mutex_estado_ready;
extern pthread_mutex_t mutex_estado_exec;
extern pthread_mutex_t mutex_estado_blocked;
extern pthread_mutex_t mutex_estado_ready_plus;
extern pthread_mutex_t mutex_prioritario_por_signal;
extern pthread_mutex_t no_hay_nadie_en_cpu;

extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_hay_pcb_esperando_ready;
extern sem_t sem_hay_para_planificar;
extern sem_t sem_contador_quantum;
extern sem_t sem_planificadores;

extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_blocked;
extern t_pcb* pcb_exec;
extern t_queue* cola_exit;
extern t_queue* cola_prioritarios_por_signal;
extern t_queue* cola_ready_plus;

extern char* algoritmo_planificacion; // Tomamos en convencion que los algoritmos son "FIFO", "VRR" , "RR" (siempre en mayuscula)
extern t_log* logger_kernel;
extern char* path_kernel;
extern ptr_kernel* datos_kernel;

extern bool planificacion_pausada; 

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
void enviar_path_a_memoria(char* path, t_sockets* sockets, int pid);
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
void _ejecutarComando(void* _, char* linea_leida);
void *esperar_RR(t_pcb* sockets_Int) ;
void* detener_planificaciones();

t_parametro* deserializar_parametro(t_buffer* buffer);
t_pedido* deserializar_pedido(t_buffer* buffer);
void wait_signal_recurso(t_pcb* pcb, char* recurso,DesalojoCpu desalojo);
void ejecutar_signal_recurso(t_recurso* recurso,t_pcb* pcb);
void ejecutar_wait_recurso(t_recurso* recurso,t_pcb* pcb,char* key_nombre_recurso);
char* pasar_string_desalojo_recurso(DesalojoCpu desalojoCpu);
void _imprimir_recurso(char* nombre, void* element);
void imprimir_diccionario_recursos();
// t_pedido_escritura* deserializar_pedido_escritura(t_buffer* buffer);
void mandar_a_escribir_a_memoria(char* nombre_interfaz, int direccion_fisica, uint32_t tamanio);
void encolar_datos_std(t_pcb* pcb, t_pedido* pedido);

t_pedido_fs_create_delete* deserializar_pedido_fs_create_delete(t_buffer* buffer);
void mandar_pedido_fs();
t_list_io* io_esta_en_diccionario(t_pcb* pcb, char* interfaz_nombre);
t_buffer* llenar_buffer_stdout(int direccion_fisica,char* nombre_interfaz, uint32_t tamanio);
t_buffer* llenar_buffer_nombre_archivo_pid(int pid,int largo_archivo,char* nombre_archivo);
void enviar_buffer_fs(int socket_io,int pid,int longitud_nombre_archivo,char* nombre_archivo, codigo_operacion codigo_operacion);
t_pedido_fs_escritura_lectura* deserializar_pedido_fs_escritura_lectura(t_buffer* buffer);

t_buffer* llenar_buffer_nombre_archivo_pid(int pid,int largo_archivo,char* nombre_archivo);
t_buffer* llenar_buffer_fs_escritura_lectura(int pid,int socket,int largo_archivo,char* nombre_archivo,uint32_t registro_direccion,uint32_t registro_tamanio,uint32_t registro_archivo);
void enviar_buffer_fs_escritura_lectura(int pid,int socket,int largo_archivo,char* nombre_archivo,uint32_t registro_direccion,uint32_t registro_tamanio,uint32_t registro_archivo,codigo_operacion codigo);
// void imprimir_datos_stdin(io_stdin* datos_stdin);
t_pedido_fs_truncate* deserializar_fs_truncate(t_buffer* buffer);
t_buffer* llenar_buffer_fs_truncate(int pid,int largo_archivo,char* nombre_archivo,uint32_t truncador);
bool es_VRR_RR();
bool es_RR();
bool es_VRR();
void *esperar_VRR(t_pcb* pcb);
void *esperar_RR(t_pcb* pcb);
void volver_a_settear_quantum(t_pcb* pcb);
int max(int num1, int num2);
int leQuedaTiempoDeQuantum(t_pcb *pcb);
void mostrar_cola(t_queue* cola);
void ejecutarComando(char* linea_leida);
void mostrar_cola_con_mutex(t_queue* cola,pthread_mutex_t* mutex);
pthread_mutex_t* obtener_mutex_de(t_queue* cola);


t_queue* encontrar_en_que_cola_esta(int pid);
int esta_en_cola_pid(t_queue* cola, int pid, pthread_mutex_t* mutex);
t_pcb* sacarDe(t_queue* cola, int pid);
int queue_find(t_queue* cola, int pid);
t_pcb* sacarPCBDeDondeEste(int pid);
pthread_mutex_t* obtener_mutex_de(t_queue* cola);

/* FUNCIONES PARA LOS MEMCHECK Y LIBERAR MALLOCS :D */
void liberar_pedido_escritura_lectura(t_pedido* pedido_escritura_lectura);
void liberar_recursos(t_dictionary* recursos);
void finalizar_kernel();
void enviar_eliminacion_pcb_a_memoria(int pid);
void* liberar_pcb(void* pcb);
void liberar_estructura_sockets();
void liberar_cola_recursos(t_list* procesos_bloqueados);
void liberar_datos_kernel();
void liberar_ios();
void liberar_pcb_de_io(int pid);
void liberar_pcb_de_recursos(int pid);
void liberar_pcb_normal(t_pcb* pcb);
void finalzar_cpu();
void finalizar_memoria();

void EJECUTAR_SCRIPT(char* path);
void INICIAR_PROCESO(char* path);
void FINALIZAR_PROCESO(int pid);
void INICIAR_PLANIFICACION();
void DETENER_PLANIFICACION();
void PROCESO_ESTADO();
void MULTIPROGRAMACION(int valor);

#endif