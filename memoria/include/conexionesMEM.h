#ifndef CONEXIONESMEM_H
#define CONEXIONESMEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"

extern pthread_t kernel_thread;
extern pthread_t cpu_thread;
extern pthread_t kernel_thread;
extern pthread_t io_stdin_thread;
extern pthread_t io_stdout_thread;
extern pthread_mutex_t mutex_diccionario_instrucciones;
extern pthread_mutex_t mutex_diccionario_io;
extern pthread_t cpu_thread;
extern t_config* config_memoria;
extern t_dictionary* diccionario_instrucciones;
extern char* path_config;
extern t_dictionary* diccionario_tablas_paginas;
extern t_dictionary* diccionario_io;
extern sem_t* hay_valores_para_leer;
extern t_log* logger_memoria;

extern int tamanio_pagina;
extern int tamanio_memoria;
extern void* espacio_usuario;
extern int cant_frames;
extern t_bitarray* bitmap;

extern int tiempo_retardo;
extern int io_desconectada;
extern int socket_cpu;

typedef struct {
    int socket;
    t_log* logger;
} t_config_memoria;

typedef struct {
    int cantidad;
} t_cantidad_instrucciones;


/////////////////////
///// FUNCIONES /////
/////////////////////

// GENERAL
int esperar_cliente(int, t_log*);
void crear_estructuras(char* path_completo, int pid);
void _agregar_instruccion_a_diccionario(char* pid_char, char* linea);
void agregar_interfaz_en_el_diccionario(t_paquete* paquete, int socket);
t_instruccion* build_instruccion(char*);

// CPU
void* handle_cpu(void* socket);
// *quiero frame*
void quiero_frame(t_buffer* buffer);
int buscar_frame(t_solicitud_frame* pedido_frame);
// *resize*
int resize_memory(void* stream);
void recortar_tamanio(int tamanio, char* pid_string, int cant_bytes_uso);
void asignar_tamanio(int tamanio, int cant_bytes_uso, char* pid_string);
void marcar_frame_en_tabla_paginas(t_list* tabla_paginas, int frame);
int buscar_frame_libre();
int cantidad_frames_proceso(char* pid_string);
int validar_out_of_memory(int tamanio, int pid);
int contar_frames_libres(/* bit map es global*/);

// KERNEL
void* handle_kernel(void* socket); 

// FILE SYSTEM
void* handle_io_dialfs(void* socket);

// LECTURA|ESCRITURA
void realizar_operacion(int pid, tipo_operacion operacion, t_list* direcciones_restantes, void* user_space_aux, void* registro_escritura, void* registro_lectura);
void interaccion_user_space(int pid,tipo_operacion operacion, int df_actual, void* user_space_aux, int tam_escrito_anterior, int tamanio, void* registro_escritura, void* registro_lectura);

// FUNCIONES DE AYUDA
char* agrupar_path(t_path* path);
void imprimir_path(t_path* path);
bool check_same_page(int tamanio, int cant_bytes_uso);
void printear_paginas(char* pid_char);
void printear_bitmap();
void imprimir_datos_pedido_lectura(t_pedido_rw_encolar* pedido);
void imprimir_datos_stdin_escritura(t_escritura_stdin* escritura);
void imprimir_diccionario(); 
void imprimir_datos_escritura_fs(t_escritura_memoria_fs* escritura_lectura);
void print_parametros(t_parametro* parametro);
void print_instruccion(t_instruccion* instruccion);
void print_instrucciones(char* key, t_list* lista_instrucciones);
char* instruccion_a_string(TipoInstruccion tipo);
TipoInstruccion pasar_a_enum(char* nombre);

// ENTRADAS SALIDAS
void* handle_io_stdout(void* socket);
void* handle_io_stdin(void* socket);
void desconectar_io_de_diccionario(char* nombre_interfaz);
void enviar_valor_leido_a_io(int pid, int socket_io, char* valor, int tamanio);


// DESERIALIZACION|SERIALIZACION|ENVIO|RECEPCION DE PAQUETES 
t_pedido_memoria* deserializar_direccion_fisica(t_buffer* buffer, t_list* direcciones_restantes);
t_solicitud_frame* deserializar_solicitud_frame(t_buffer* buffer);
t_pedido_rw_encolar* deserializar_pedido_lectura_escritura_mem(t_buffer* buffer);
t_escritura_stdin* deserializar_escritura_stdin(void* stream);
t_buffer* llenar_buffer_stdout(char* valor);
t_path* deserializar_path(t_buffer* buffer);
t_solicitud_instruccion* deserializar_solicitud_instruccion(t_buffer* buffer);
void enviar_instruccion(t_solicitud_instruccion* solicitud_cpu,int socket_cpu);
void enviar_cantidad_instrucciones_pedidas(char* pid, int socket_cpu);
void mandar_lectura_a_fs(int pid, void* registro_lectura, t_pedido_rw_encolar* pedido_fs, uint32_t size_registro, int socket_io);
int deserializar_pid(t_buffer* buffer);
t_escritura_memoria_fs* deserializar_escritura_lectura_fs(t_buffer* buffer);
t_memoria_fs_escritura_lectura* deserializar_escritura_lectura(t_buffer* buffer);
t_pid_stdout* desearializar_pid_stdout(t_buffer* buffer);
void enviar_confirmacion_escritura_fs(int socket_io);

// LIMPIEZA
void liberar_estructuras_proceso(int pid);
void limpiar_bitmap(int pid_a_liberar);
void liberar_modulo_memoria(); //no hace nada todavia
void destruir_paginas(t_proceso_paginas* proceso);

// NO SE USA
char* leer(int tamanio, int direccion_fisica);
char* string_tipo_operacion(tipo_operacion operacion);

//////////////////////////////////////////////////
///// LOS DE ABAJO NO SE ENCUENTRAN EN EL .C /////
//////////////////////////////////////////////////

int min(int a, int b);
int recibir_operacion(int socket_cliente);
void liberar_conexion(int socket_cliente);
int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io);
void imprimir_path(t_path* path);
//t_cantidad_instrucciones* deserializar_cantidad(t_buffer* buffer);
t_pedido* desearializar_pedido_escritura(t_buffer* buffer);
void recibir_resto_direcciones(t_list* lista_direcciones);
int bytes_usables_por_pagina(int direccion_logica);
t_direccion_fisica_escritura* deserializar_direccion_fisica_escritura(t_buffer* buffer);
t_direccion_fisica* deserializar_direccion_fisica_lectura(t_buffer* buffer);
t_direccion_fisica* armar_dir_lectura(t_direccion_fisica_escritura* df);


// FREE
void eliminar_instrucciones(int pid);
void liberar_ios();
void liberar_tablas_paginas();
void liberar_cola_recursos(t_list* procesos_bloqueados);

#endif // CONEXIONES_H




