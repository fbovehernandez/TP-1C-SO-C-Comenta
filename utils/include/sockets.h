#ifndef SOCKET_H
#define SOCKET_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/config.h>
#include <assert.h>
#include <pthread.h>

typedef struct {
    int PID;
    int path_length;
    char* path;
} t_path;

typedef struct {
    int pid;
    int pc;
} t_solicitud_instruccion;

typedef enum {
    INTERRUPCION_QUANTUM,
    IO_BLOCKED,
    WAIT_RECURSO,
    FIN_PROCESO
} desalojo_cpu;

// Para que usar op_code si tenemos codigo_operacion? (O al revés, qué rompimos?)
typedef enum {
	MENSAJE,
	PAQUETE
} op_code;

typedef enum {
    PATH,
	ENVIO_PCB,
    QUIERO_INSTRUCCION
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

} Registros;

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
    // Registros* registros;
} t_pcb;

typedef struct {
    TipoInstruccion nombre;
    t_list* parametros; // Cada una de las instrucciones
    int cantidad_parametros;
} t_instruccion;


typedef enum {
    SET,  
    MOV_IN,
    MOV_OUT,
    SUM,
    SUB,
    JNZ,
    RESIZE,
    COPY_STRING,,
    // ...
    WAIT,
    SIGNAL,
    IO_GEN_SLEEP,
    IO_STDIN_READ
} TipoInstruccion;

int iniciar_servidor(char*);
int esperar_conexion(int);
void corroborar_exito(int, char*);
void sendMessage(int socket_fd);
void receiveMessagex(int socket_fd);
int crear_conexion(char *ip, char* puerto, int valor);

#endif // SOCKET_H