#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../utils/include/sockets.h" 
#include "../../utils/include/logconfig.h"
#include <commons/string.h>
#include "conexionesCPU.h"
#include "mmu.h"
#include "fs_operations.h"

extern int hay_interrupcion_fin;
extern int hay_interrupcion_quantum;

extern pthread_mutex_t mutex_interrupcion_quantum;
extern pthread_mutex_t mutex_interrupcion_fin;

typedef struct {
    int cantidad;
} t_cantidad_instrucciones;

// EJECUCION PROCESO
void ejecutar_pcb(t_pcb* pcb, int socket_memoria);
int ejecutar_instruccion(t_instruccion* instruccion,t_pcb* pcb);
char* obtener_lista_parametros(t_list* list_parametros);
char* pasar_instruccion_a_string(TipoInstruccion nombreInstruccion);
void hacer_interrupcion(t_pcb* pcb);
int hay_interrupcion();

// INSTRUCCIONES
void set(void *registro, uint32_t valor, bool es_8_bits);
void set_uint8(void* registro_dato_mov_in, uint8_t valor_recibido_8);
void sub(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino);
void sum(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino);
void jnz(void* registro, int valor, t_pcb* pcb);

// FUNCIONES COMPLEMENTARIAS DE INSTRUCCIONES
void* seleccionar_registro_cpu(char* nombreRegistro,t_pcb* pcb);
int cantidad_de_paginas_a_utilizar(uint32_t direccion_logica, int tamanio_en_bytes, int pagina, t_list* lista_bytes_lectura);
int primer_byte_anterior_a_dir_logica(uint32_t direccion_logica, int tamanio_pagina);
int tamanio_byte_registro(bool es_registro_uint8);
bool es_de_8_bits(char* registro);
void enviar_direcciones_fisicas(int cantidad_paginas, t_list* direcciones_fisicas, void* registro_dato_mov_out, int tamanio_valor, int pid, codigo_operacion cod_op);
void cargar_direcciones_tamanio(int cantidad_paginas, t_list* lista_bytes_lectura, uint32_t direccion_logica, int pid, t_list* direcciones_fisicas, int pagina);
void recibir_parametros_mov_out(t_list* list_parametros, t_parametro **registro_direccion, t_parametro **registro_datos, char** nombre_registro_dato, char** nombre_registro_dir);
void enviar_tamanio_memoria(int nuevo_tamanio, int pid);
t_buffer* llenar_buffer_dormir_IO(char* interfaz, int unidades);
void manejar_recursos(t_pcb* pcb, t_parametro* recurso, DesalojoCpu codigo);
t_buffer* llenar_buffer_stdio(char* interfaz, t_list* direcciones_fisicas, uint32_t tamanio_a_copiar, int cantidad_paginas);
t_buffer* llenar_buffer_fs_create_delete(char* nombre_interfaz,char* nombre_archivo);
t_buffer* llenar_buffer_fs_truncate(char* nombre_interfaz_a_truncar,char* nombre_file_truncate,uint32_t registro_truncador);


// MEMORIA
t_paquete* recibir_memoria(int socket_memoria);

// KERNEL
void desalojar(t_pcb* pcb, DesalojoCpu motivo, t_buffer* add);
void guardar_estado(t_pcb *pcb);
void setear_registros_cpu();

// DESERIALIZACION|SERIALIZACION|ENVIO|RECEPCION DE PAQUETES
void pedir_cantidad_instrucciones_a_memoria(int pid, int socket_memoria);
int recibir_cantidad_instrucciones(int socket_memoria, int pid);
int cantidad_instrucciones_deserializar(t_buffer *buffer);
void pedir_instruccion_a_memoria(int socket_memoria, t_pcb *pcb);
t_buffer* llenar_buffer_solicitud_instruccion(t_solicitud_instruccion* solicitud_instruccion);
t_instruccion* recibir_instruccion(int socket_memoria, t_pcb *pcb);
t_instruccion* instruccion_deserializar(t_buffer* buffer);
t_buffer* serializar_direcciones_fisicas(int cantidad_paginas, t_list* direcciones_fisicas, void* registro_dato_mov_out, int tamanio_valor, int pid);

// LIMPIEZA
void instruccion_destruir(t_instruccion *instruccion);
void destruir_parametro(void* parametro_void);

// NO SE UTILIZAN
void solicitud_dormirIO_kernel(char* interfaz, int unidades); 
t_buffer* llenar_buffer_stdout(int direccion_fisica,char* nombre_interfaz, uint32_t tamanio);
void enviar_kernel_stdout(char* nombre_interfaz, int direccion_fisica, uint32_t tamanio);
t_buffer* llenar_buffer_copy_string(int direccion_fisica_SI, int direccion_fisica_DI, int tamanio);
void enviar_buffer_copy_string(int direccion_fisica_SI, int direccion_fisica_DI, int tamanio);
void liberar_parametro(t_parametro* parametro);

//////////////////////////////////////////////////
///// LOS DE ABAJO NO SE ENCUENTRAN EN EL .C /////
//////////////////////////////////////////////////

int recibir_resultado_resize();
int check_interrupt(t_pcb* pcb);
void enviar_primer_pagina(uint32_t direccion_logica, int pid, int tamanio_en_bytes, int cant_paginas, void* valor_a_escribir, uint32_t length_valor, codigo_operacion cod_op);
void enviar_fin_programa(t_pcb *pcb);
bool sonTodosDigitosDe(char *palabra);
void mandar_direccion_fisica_a_mem(int direccion_fisica, int tamanio_en_bytes, int cantidad_paginas, int direccion_logica, void* valor, uint32_t length_valor, codigo_operacion cod_op);
void* sacarParametroParaConseguirRegistro(t_list* list_parametros,bool es_registro_uint8);
void pedir_todos_los_marcos(int pagina, int cant_paginas, int pid, uint32_t *registro_direccion_1, bool es_registro_uint8_dato);
void mandar_una_dir_fisica(int direccion_fisica);
void realizar_operacion(int pid,uint32_t* registro_direccion_1, int tamanio_en_byte, void* valor_a_escribir, uint32_t length_valor, codigo_operacion codigo_operacion);
t_buffer* serializar_lectura(int direccion_fisica, int tamanio_en_bytes, int cantidad_paginas, int direccion_logica);
t_buffer* serializar_escritura(int direccion_fisica, int tamanio_en_bytes, int cantidad_paginas, int direccion_logica, void* valor, uint32_t length_valor);
void enviar_buffer_fs_create_delete(char* nombre_interfaz,char* nombre_archivo,codigo_operacion codigo);
char* obtener_direcciones_fisicas(t_list* lista_direcciones_fisicas);


#endif 