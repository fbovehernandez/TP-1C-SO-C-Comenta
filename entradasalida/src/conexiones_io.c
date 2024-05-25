#include "../include/conexiones_io.h"

int socket_kernel_io;
t_log* logger_io;

/*
void gestionar_STDOUT(t_config* config_io, t_log* logger_io) {
    // char* IP_CPU = config_get_string_value(config_kernel, "IP_CPU");
    char* ip_kernel = config_get_string_value(config_io, "IP_KERNEL_IO");
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL_IO");

    printf("IP_IO: %s\n", ip_kernel);
    printf("PUERTO_IO_STDOUT: %s\n", puerto_kernel);

    // El handshake de stdout es: 45
    int stdout_fd = crear_conexion(ip_kernel, puerto_kernel, 45 Handshake inventado);
    log_info(logger_io, "Conexion establecida con kernel desde IO STDOUT"); 
    sendMessage(stdout_fd);
    close(stdout_fd); // Si no quisiese que se desconecte, se puede comentar
}

void gestionar_STDIN(t_config *config_io){

}

void gestionar_GENERICA(t_config *config_io){

}

void gestionar_DIALFS(t_config *config_io) {

}
*/
typedef struct {
    int nombre_interfaz_largo;
    char* nombre_interfaz;
} t_info_io;

int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io, char* nombre_interfaz) {
    int message_io = 12; // nro de codop
    int valor = 5; //handshake, 5 = I/O

    int kernelfd = crear_conexion(IP_KERNEL, puerto_kernel, valor);
    log_info(logger_io, "Conexion establecida con Kernel");
    
    t_info_io* io = malloc(sizeof(int) + string_length(nombre_interfaz));
    // Hay que mandarle el nombre de la IO. Ver estructura t_info_io
    int str_interfaz = strlen(nombre_interfaz);
    send(socket_kernel_io, &str_interfaz, sizeof(int), 0); 

    // close(kernelfd); // (POSIBLE) no cierro el socket porque quiero reutilizar la conexion
    return kernelfd;
}

void recibir_solicitud_kernel() {
    codigo_operacion cod_op;
    recv(socket_kernel_io, &cod_op, sizeof(int), 0);
    switch(cod_op) {
        case QUIERO_NOMBRE:
            send(socket_kernel_io, nombre_io, 100, 0); // esta bien pasado asi?
            break;
        case DORMITE:
            //Podria estar en recibir_kernel(), asi no tenemos que mandar un paquete vacio en QUIERO_NOMBRE.
            t_paquete* paquete = malloc(sizeof(t_paquete));
            paquete->buffer = malloc(sizeof(t_buffer));

            // Primero recibimos el codigo de operacion
            recv(socket_kernel_io, &(paquete->codigo_operacion), sizeof(int), 0);
            // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
            recv(socket_kernel_io, &(paquete->buffer->size), sizeof(uint32_t), 0);
            paquete->buffer->stream = malloc(paquete->buffer->size);
            recv(socket_kernel_io, paquete->buffer->stream, paquete->buffer->size, 0);

            int unidadesDeTrabajo = serializar_unidades_trabajo(paquete->buffer);
            
            // tiempoUnidadesTrabajo esta en el config
            sleep(unidadesDeTrabajo * tiempoUnidadesTrabajo);
        default:
            break;
    }
}

int serializar_unidades_trabajo(t_buffer* buffer) {
    int unidades_de_trabajo;
    memcpy(&unidades_de_trabajo, buffer->stream, sizeof(int));
    return unidades_de_trabajo;
}