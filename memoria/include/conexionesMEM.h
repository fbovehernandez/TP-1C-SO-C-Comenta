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
extern int socket_cpu;

typedef struct {
    int socket;
    t_log* logger;
} t_config_memoria;

typedef struct {
    int cantidad;
} t_cantidad_instrucciones;

int min(int a, int b);
void recortar_tamanio(int tamanio, char* pid_string, int cant_bytes_uso);
int resize_memory(void* stream);
int buscar_frame_libre();
void marcar_frame_en_tabla_paginas(t_list* tabla_paginas, int frame);
char* instruccion_a_string(TipoInstruccion tipo);
int validar_out_of_memory(int tamanio);
int contar_frames_libres(/* bit map es global*/);
int recibir_operacion(int socket_cliente);
void* handle_kernel(void* socket); //
void desconectar_io_de_diccionario(char* nombre_interfaz);
void* handle_cpu(void* socket);//
void liberar_conexion(int socket_cliente);
int esperar_cliente(int, t_log*); //
int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io);
void imprimir_path(t_path* path);//
t_path* deserializar_path(t_buffer* buffer);//
t_instruccion* build_instruccion(char*);//
t_solicitud_instruccion* deserializar_solicitud_instruccion(t_buffer* buffer);//
void enviar_instruccion(t_solicitud_instruccion* solicitud_cpu,int socket_cpu);//
char* agrupar_path(t_path* path);//
void imprimir_path(t_path* path);
void crear_estructuras(char* path_completo, int pid);//
void imprimir_diccionario(); //
void print_parametros(t_parametro* parametro); //
void print_instruccion(t_instruccion* instruccion);//
void print_instrucciones(char* key, t_list* lista_instrucciones);//
void _agregar_instruccion_a_diccionario(char* pid_char, char* linea);
TipoInstruccion pasar_a_enum(char* nombre);//
//t_cantidad_instrucciones* deserializar_cantidad(t_buffer* buffer);
int deserializar_pid(t_buffer* buffer);
void enviar_cantidad_instrucciones_pedidas(char* pid, int socket_cpu);
int buscar_frame(t_solicitud_frame* pedido_frame);
t_solicitud_frame* deserializar_solicitud_frame(t_buffer* buffer);
int cantidad_frames_proceso(char* pid_string);
bool check_same_page(int tamanio, int cant_bytes_uso);
void asignar_tamanio(int tamanio, int cant_bytes_uso, char* pid_string);
void printear_paginas(char* pid_char);
void printear_bitmap();
void* handle_io_stdin(void* socket);
void* handle_io_stdout(void* socket);
t_pedido* desearializar_pedido_escritura(t_buffer* buffer);
char* leer(int tamanio, int direccion_fisica);
t_buffer* llenar_buffer_stdout(char* valor);
void recibir_resto_direcciones(t_list* lista_direcciones);
int bytes_usables_por_pagina(int direccion_logica);
void quiero_frame(t_buffer* buffer);
void realizar_operacion(int pid, tipo_operacion operacion, t_list* direcciones_restantes, void* user_space_aux, void* registro_escritura, void* registro_lectura);
void interaccion_user_space(int pid,tipo_operacion operacion, int df_actual, void* user_space_aux, int tam_escrito_anterior, int tamanio, void* registro_escritura, void* registro_lectura);
t_direccion_fisica_escritura* deserializar_direccion_fisica_escritura(t_buffer* buffer);
t_direccion_fisica* deserializar_direccion_fisica_lectura(t_buffer* buffer);
t_direccion_fisica* armar_dir_lectura(t_direccion_fisica_escritura* df);
void agregar_interfaz_en_el_diccionario(t_paquete* paquete, int socket);
t_pedido_memoria* deserializar_direccion_fisica(t_buffer* buffer, t_list* direcciones_restantes);
t_escritura_stdin* deserializar_escritura_stdin(void* stream);
void imprimir_datos_stdin_escritura(t_escritura_stdin* escritura);
t_pid_stdout* desearializar_pid_stdout(t_buffer* buffer);
void enviar_valor_leido_a_io(int pid, int socket_io, char* valor, int tamanio);
void* handle_io_dialfs(void* socket);
t_memoria_fs_escritura_lectura* deserializar_escritura_lectura(t_buffer* buffer);
char* string_tipo_operacion(tipo_operacion operacion);
void* handle_io_dialfs(void* socket);
t_pedido_rw_encolar* deserializar_pedido_lectura_escritura_mem(t_buffer* buffer);
void imprimir_datos_pedido_lectura(t_pedido_rw_encolar* pedido);
void mandar_lectura_a_fs(void* registro_lectura, t_pedido_rw_encolar* pedido_fs, int size_registro, int socket_io);


// FREE
void destruir_paginas(t_proceso_paginas* proceso);
void eliminar_instrucciones(int pid);
void liberar_modulo_memoria();
void liberar_ios();
void limpiar_bitmap(int pid_a_liberar);
void liberar_modulo_memoria();
void liberar_tablas_paginas();
void liberar_cola_recursos(t_list* procesos_bloqueados);
void liberar_estructuras_proceso(int pid);

#endif // CONEXIONES_H




