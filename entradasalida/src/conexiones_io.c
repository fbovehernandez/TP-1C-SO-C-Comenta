#include "../include/conexiones_io.h"

int socket_kernel_io;
t_log* logger_io;
char* nombre_io;

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

int conectar_io_kernel(char* IP_KERNEL, char* puerto_kernel, t_log* logger_io, char* nombre_interfaz, TipoInterfaz tipo_interfaz) {
    // int message_io = 12; // nro de codop
    int valor = 5; // handshake, 5 = I/O

    int kernelfd = crear_conexion(IP_KERNEL, puerto_kernel, valor);
    log_info(logger_io, "Conexion establecida con Kernel");
    
    // send(kernelfd, &message_io, sizeof(int), 0); 

    int str_interfaz = strlen(nombre_interfaz) + 1;
    
    t_info_io* io = malloc(sizeof(int) + str_interfaz);
    io->nombre_interfaz_largo = str_interfaz;
    io->nombre_interfaz = nombre_interfaz;
    io->tipo = tipo_interfaz;

    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_paquete* paquete = malloc(sizeof(t_paquete));

    buffer->size = sizeof(int) + str_interfaz + sizeof(int); // sizeof(x2)
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream+buffer->offset, &io->nombre_interfaz_largo, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &io->nombre_interfaz, str_interfaz);
    buffer->offset += str_interfaz;
    memcpy(stream + buffer->offset, &io->tipo, sizeof(int));
    buffer->offset += sizeof(int);

    buffer->stream = stream;

    paquete->codigo_operacion = CONEXION_INTERFAZ; // Podemos usar una constante por operación
    paquete->buffer = buffer;                                  // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void *a_enviar = malloc(buffer->size + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    send(kernelfd, a_enviar, buffer->size + sizeof(int), 0); 

    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);

    return kernelfd;
}

/* 
void recibir_solicitud_kernel() {
    codigo_operacion cod_op;

    recv(socket_kernel_io, &cod_op, sizeof(int), 0);
    switch(cod_op) {
        case QUIERO_NOMBRE:
            send(socket_kernel_io, nombre_io, 100, 0); // esta bien pasado asi?
            break;
        default:
            break;
    }
}
*/

void recibir_kernel(t_config* config_io) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    recv(socket_kernel_io, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
    printf("Recibi el codigo de operacion : %d\n", paquete->codigo_operacion);

    recv(socket_kernel_io, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
    paquete->buffer->stream = malloc(paquete->buffer->size);

    recv(socket_kernel_io, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

    switch(paquete->codigo_operacion) {
        case DORMITE:
            int unidadesDeTrabajo = serializar_unidades_trabajo(paquete->buffer);
            int tiempoUnidadesTrabajo = config_get_int_value(config_io,"TIEMPO_UNIDAD_TRABAJO");
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