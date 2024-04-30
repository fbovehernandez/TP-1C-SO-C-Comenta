#include "../include/main.h"

int main(int argc, char* argv[2]) {
    
    // Este archivo lo recibe por parametro
    config_io = iniciar_config(argv[2]);
    t_log* logger_io = iniciar_logger("entradasalida.log");
    
    if(config_io == NULL) {
        log_error(logger_io, "No se pudo leer el archivo de configuracion");
        return 1;
    }

    char* IP_KERNEL = config_get_string_value(config_io, "IP_KERNEL");
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL_IO");

    int socket_kernel_io = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io);     
    // close(socket_kernel_io);


// Falta manejar los distintos tipos de interfaces

    iniciar_interfaz_generica(argv[1]);
/*
    char* tipo_interfaz = config_get_string_value(config_io, "TIPO_INTERFAZ");
    printf("Tipo de interfaz: %s\n", tipo_interfaz);

    if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        gestionar_STDOUT(config_io, logger_io);
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        printf("STDIN - Falta gestionar\n");        
    } else if (strcmp(tipo_interfaz, "Genericas") == 0) {
        // gestionar_GENERICA(config_io);
    } else if (strcmp(tipo_interfaz, "DialFS") == 0) {
        // gestionar_DIALFS(config_io);
    } else {
        printf("No trates de hacer mi experiencia con operativos peor\n");
        // insert logica
    }
*/

    return 0;
}


/*

typedef struct {
    int pid;
    int program_counter;
    int quantum;
    enum Estado estadoActual;
    enum Estado estadoAnterior;
    int socketProceso;
    Registros registros;
} t_pcb;

t_paquete* paquete = malloc(sizeof(t_paquete));
paquete->buffer = malloc(sizeof(t_buffer));

// Primero recibimos el codigo de operacion
recv(unSocket, &(paquete->codigo_operacion), sizeof(uint8_t), 0);

// Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
recv(unSocket, &(paquete->buffer->size), sizeof(uint32_t), 0);
paquete->buffer->stream = malloc(paquete->buffer->size);
recv(unSocket, paquete->buffer->stream, paquete->buffer->size, 0);

// Ahora en función del código recibido procedemos a deserializar el resto
switch(paquete->codigo_operacion) {
    case PERSONA:
        t_persona* persona = persona_serializar(paquete->buffer);
        ...
        // Hacemos lo que necesitemos con esta info
        // Y eventualmente liberamos memoria
        free(persona);
        ...
        break;
    ... // Evaluamos los demás casos según corresponda
}

// Liberamos memoria
free(paquete->buffer->stream);
free(paquete->buffer);
free(paquete);



*/

void iniciar_interfaz_generica(char* nombreInterfaz) {
    char* tipo_interfaz = config_get_string_value(config_io, "TIPO_INTERFAZ");
    int tiempo_unidad_trabajo = config_get_int_value(config_io, "TIEMPO_UNIDAD_TRABAJO");
    // tiempo_unidad_trabajo: Tiempo que dura una unidad de trabajo en milisegundos
    // unidad_trabajo: Cantidad de unidades de trabajo que va a dormir segun la instruccion de CPU
    
    


    IO_GEN_SLEEP(unidad_trabajo,tiempo_unidad_trabajo); 
}

void IO_GEN_SLEEP(int unidad_trabajo, int tiempo_unidad_trabajo) {
    sleep(unidad_trabajo * tiempo_unidad_trabajo);
}