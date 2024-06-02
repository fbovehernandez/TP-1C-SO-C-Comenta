#ifndef SOCKET_H
#define SOCKET_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <assert.h>
#include <pthread.h>

extern sem_t sem_memoria_instruccion;

typedef enum {
    GENERICA, 
    STDIN,
    STDOUT,
    DIALFS
} TipoInterfaz;
typedef struct {
    int PID;
    int path_length;
    char* path;
} t_path;

typedef struct { // usa io para pasarlo al kernel
    int nombre_interfaz_largo;
    TipoInterfaz tipo;
    char* nombre_interfaz;
} t_info_io;

typedef struct {
    t_info_io* info;
    int unidadesDeTrabajo;
} t_operacion_io;
// Lo usamos en kernel para recibir cpu

typedef struct {
    int socket;
    char* nombreInterfaz; // Aaca tenia un dato_io que le sque
    TipoInterfaz TipoInterfaz;
    t_queue* cola_blocked;
    sem_t* semaforo_cola_procesos_blocked;
} t_list_io; // para el diccionario

typedef struct {
    int pid;
    int pc;
} t_solicitud_instruccion;

typedef enum {
    INTERRUPCION_QUANTUM,
    IO_BLOCKED,
    DORMIR_INTERFAZ,
    LEER_INTERFAZ,
    WAIT_RECURSO,
    FIN_PROCESO
} DesalojoCpu;

typedef struct {
    int pid;
    DesalojoCpu motivoDesalojo;
} t_desalojo;

// Para que usar op_code si tenemos codigo_operacion? (O al revés, qué rompimos?)
typedef enum {
	MENSAJE,
	PAQUETE
} op_code;

typedef enum {
    PATH,
	ENVIO_PCB,
    QUIERO_INSTRUCCION,
    ENVIO_INSTRUCCION,
    QUIERO_CANTIDAD_INSTRUCCIONES,
    ENVIO_CANTIDAD_INSTRUCCIONES,
    INTERRUPCION_CPU,
    QUIERO_NOMBRE,
    DORMITE,
    CONEXION_INTERFAZ
} codigo_operacion;

typedef struct {
	int size;
	uint32_t offset;
	void* stream;
} t_buffer;

typedef struct {
	codigo_operacion codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct {

    // Registros de proposito gral.  (1 byte) 
    uint8_t   AX;
    uint8_t   BX; 
    uint8_t   CX;
    uint8_t   DX;

    // Registros de proposito gral. (4 bytes) 
    uint32_t  EAX;
    uint32_t  EBX; 
    uint32_t  ECX;
    uint32_t  EDX;

    uint32_t  SI; // Contiene la dirección lógica de memoria de origen desde donde se va a copiar un string.
    uint32_t  DI; // Contiene la dirección lógica de memoria de destino a donde se va a copiar un string.

} t_registros;

extern t_registros* registros_cpu; // Por ahora la pongo aca, despues hay que sacarla para que solo la use la CPU

typedef enum {
    NEW,
    READY,
    BLOCKED,
    EXEC,
    EXIT
} Estado;

typedef struct {
    int pid;
    int program_counter;
    int quantum;
    Estado estadoActual;
    Estado estadoAnterior;
    t_registros* registros;
} t_pcb;

typedef struct {
    int ut;
    t_pcb* pcb;
} io_gen_sleep;

typedef enum {
    SET,  
    MOV_IN,
    MOV_OUT,
    SUM,
    SUB,
    JNZ,
    RESIZE,
    COPY_STRING,
    WAIT,
    SIGNAL,
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    EXIT_INSTRUCCION,
    ERROR_INSTRUCCION
} TipoInstruccion;

typedef struct {
    TipoInstruccion nombre;
    int cantidad_parametros;
    t_list* parametros; // Cada una de las instrucciones
} t_instruccion;

typedef struct {
    char* nombre;
    int length;
} t_parametro;

void* enviar_pcb(t_pcb* pcb, int socket, codigo_operacion cod_op, void* buffer, int size);
int iniciar_servidor(char*);
int esperar_conexion(int);
void corroborar_exito(int, char*);
void sendMessage(int socket_fd);
void receiveMessagex(int socket_fd);
int crear_conexion(char *ip, char* puerto, int valor);
t_registros* inicializar_registros_cpu(t_registros* registro_pcb);
t_pcb* deserializar_pcb(t_buffer* buffer);
t_buffer* llenar_buffer_pcb(t_pcb* pcb);

#endif // SOCKET_H