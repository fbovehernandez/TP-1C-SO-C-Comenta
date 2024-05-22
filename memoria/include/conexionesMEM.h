#ifndef CONEXIONESMEM_H
#define CONEXIONESMEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

extern pthread_t kernel_thread;
extern pthread_t cpu_thread;
extern t_config* config_memoria;
extern t_dictionary* diccionario_instrucciones;

typedef struct {
    int socket;
    t_log* logger;
} t_config_memoria;

char* instruccion_a_string(TipoInstruccion tipo);
int recibir_operacion(int socket_cliente);
void* handle_kernel(void* socket); //
void* handle_cpu(void* socket);//
void liberar_conexion(int socket_cliente);
int esperar_cliente(int, t_log*); //
int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io);
void imprimir_path(t_path* path);//
t_path* deserializar_path(t_buffer* buffer);//
t_instruccion* build_instruccion(char*);//
t_solicitud_instruccion* deserializar_solicitud(t_buffer* buffer);//
void enviar_instruccion(t_solicitud_instruccion* solicitud_cpu,int socket_cpu);//
char* agrupar_path(t_path* path);//
void imprimir_path(t_path* path);
void crear_estructuras(char* path_completo, int pid);//
void imprimir_diccionario(); //
void print_parametros(t_parametro* parametro); //
void print_instruccion(t_instruccion* instruccion);//
void print_instrucciones(char* key, t_list* lista_instrucciones);//
TipoInstruccion pasar_a_enum(char* nombre);//

#endif // CONEXIONES_H