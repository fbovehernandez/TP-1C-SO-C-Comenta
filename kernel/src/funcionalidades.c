#include "../include/funcionalidades.h"
#include "../include/conexiones.h"
#include "../include/sockets.h"

// Definicion de variables globales
int grado_multiprogramacion;
int quantum;
int contador_pid = 0;
int client_dispatch;

pthread_mutex_t mutex_estado_new;
pthread_mutex_t mutex_estado_ready;
pthread_mutex_t mutex_estado_exec;
sem_t sem_grado_multiprogramacion;
sem_t sem_hay_pcb_esperando_ready;
sem_t sem_hay_para_planificar;
sem_t sem_contador_quantum;

t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_blocked;
t_queue* cola_exec;
t_queue* cola_exit;

char* algoritmo_planificacion; // Tomamos en convencion que los algoritmos son "FIFO", "VRR" , "RR" (siempre en mayuscula)
t_log* logger_kernel;

void *interaccion_consola(t_sockets* sockets) {

    //char* respuesta_por_consola;
    int respuesta = 0;

    while (respuesta != 8)
    {
        printf("--------------- Consola interactiva Kernel ---------------\n");
        printf("Elija una opcion (numero)\n");
        printf("1- Ejecutar Script \n");
        printf("2- Iniciar proceso\n");
        printf("3- Finalizar proceso\n");
        printf("4- Iniciar planificacion\n");
        printf("5- Detener planificacion\n");
        printf("6- Listar procesos por estado\n");
        printf("7- Cambiar el grado de multiprogramacion\n");
        printf("8- Finalizar modulo\n");
        scanf("%d", &respuesta);

        switch (respuesta)
        {
        case 1:
            EJECUTAR_SCRIPT(/* pathing del conjunto de instrucciones*/);
            break;
        case 2:
            /*char path_ejecutable[100]; // chicos vean si pueden usar un getline aca, les dejo el problema je
            printf("Ingrese el path del script a ejecutar %s\n", path_ejecutable);
            scanf("%s", &path_ejecutable);
            INICIAR_PROCESO(path_ejecutable, sockets);*/ // Ver problemas con caracteres como _ o /

            INICIAR_PROCESO("/script_io_basico_1", sockets);

            // home/utnso/c-comenta/pruebas -> Esto tendria en memoria y lo uno con este que le mando -> Ver sockets como variable global
            break;
        case 3:
            FINALIZAR_PROCESO();
            break;
        case 4:
            INICIAR_PLANIFICACION(); // Esto depende del algoritmo vigente (FIFO, RR,VRR), se deben poder cambiar
            break;
        case 5:
            DETENER_PLANIFICACION();
            break;
        case 6:
            PROCESO_ESTADO();
            break;
        case 7:
            MULTIPROGRAMACION();
            break;
        case 8:
            printf("Finalizacion del modulo\n");
            exit(1);
            break;
        default:
            printf("No se encuentra el numero dentro de las opciones, porfavor elija una correcta\n");
            break;
        }
    } 

    return NULL;
}

void INICIAR_PROCESO(char* path_instrucciones, t_sockets* sockets)
{
    int pid_actual = obtener_siguiente_pid();
    printf("El pid del proceso es: %d\n", pid_actual);
    // Primero envio el path de instruccion a memoria y luego el PCB...
    enviar_path_a_memoria(path_instrucciones, sockets, pid_actual);

    sleep(3); // Sacar esto

    t_pcb *pcb = crear_nuevo_pcb(pid_actual); // Aca dentro recibo el set de instrucciones del path
    // en el enunciado dice "en caso de que el grado de multiprogramacion lo permita"

    encolar_a_new(pcb);
}

void* enviar_path_a_memoria(char* path_instrucciones, t_sockets* sockets, int pid) {
    // Mando PID para saber donde asociar las instrucciones
    t_path* pathNuevo = malloc(sizeof(t_path));

    pathNuevo->path = strdup(path_instrucciones); 
    pathNuevo->path_length = strlen(pathNuevo->path) + 1;
    pathNuevo->PID = pid;

    t_buffer* buffer = llenar_buffer_path(pathNuevo);

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = PATH; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0; 

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    log_info(logger_kernel, "Se armo un paquete con este PATH: %s\n", pathNuevo->path);

    // Por último enviamos
    send(sockets->socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);

    free(pathNuevo->path);
    free(pathNuevo);

    return NULL;
} 

// Falta implementar esta
t_buffer* llenar_buffer_path(t_path* pathNuevo) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 2 + pathNuevo->path_length;                    
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream; // No memoria?

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream+buffer->offset, &pathNuevo->PID, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pathNuevo->path_length, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, pathNuevo->path, pathNuevo->path_length);
    // No tiene sentido seguir calculando el desplazamiento, ya ocupamos el buffer 
   
    buffer->stream = stream;
    // Si usamos memoria dinámica para el path, y no la precisamos más, ya podemos liberarla:

    // free(pathNuevo->path); -> Si yo hago el free esto rompe, porque?
    // free(pathNuevo);

    return(buffer);
}


int obtener_siguiente_pid()
{
    contador_pid++;
    return contador_pid;
}

void encolar_a_new(t_pcb *pcb)
{
    // Bloquear el acceso a la cola de procesos
    pthread_mutex_lock(&mutex_estado_new);
    // Sacar el proceso de la cola
    queue_push(cola_new, (void *)pcb);
    // Desbloquear el acceso a la cola de procesos
    pthread_mutex_unlock(&mutex_estado_new);

    log_info(logger_kernel, "Se crea el proceso: %d en NEW", pcb->pid);
    // Incrementar el semáforo para indicar que hay un nuevo proceso
    sem_post(&sem_hay_pcb_esperando_ready);
}

void* a_ready() {
    while (1) {
        // Esperar a que haya un proceso en la cola -> Esto espera a que literalment haya uno
        sem_wait(&sem_hay_pcb_esperando_ready); 
        log_info(logger_kernel, "Hay proceso esperando en la cola de NEW!\n");

        // Esperar a que el grado de multiprogramación lo permita
        sem_wait(&sem_grado_multiprogramacion); 
        log_info(logger_kernel, "Grado de multiprogramación permite agregar proceso a ready\n");

        pthread_mutex_lock(&mutex_estado_new);
        t_pcb *pcb = queue_pop(cola_new);
        pthread_mutex_unlock(&mutex_estado_new);

        // Pasar el proceso a ready
        pasar_a_ready(pcb, NEW);
        
        // sem_post(&sem_hay_pcb_esperando_ready); 
    }
}

void pasar_a_ready(t_pcb *pcb, Estado estadoAnterior) {
    pcb->estadoAnterior = estadoAnterior;
    change_status(pcb, READY);

    pthread_mutex_lock(&mutex_estado_ready);
    queue_push(cola_ready, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_ready);

    log_info(logger_kernel, "PID: %d - Estado Anterior: %d - Estado Actual: %d", pcb->pid, pcb->estadoAnterior, pcb->estadoActual);
    sem_post(&sem_hay_para_planificar);
}

void* planificar_corto_plazo(void* sockets_necesarios) { // Ver el cambio por tipo void....
    pthread_t hilo_quantum; // uff las malas practicas

    t_sockets* sockets = (t_sockets*)sockets_necesarios;
    int contexto_devolucion = 0;
    t_pcb *pcb = malloc(sizeof(t_pcb));

    int socket_CPU = sockets->socket_cpu;
    int socket_INT = sockets->socket_int; 
    
    while(1) {
        sem_wait(&sem_hay_para_planificar); 
        pcb = proximo_a_ejecutar();
        pasar_a_exec(pcb);
        enviar_pcb(pcb, socket_CPU, ENVIO_PCB); // Serializar antes de enviar 

        // FIFO ya esta hecho con lo de arriba
        if(strcmp(algoritmo_planificacion,"RR") == 0) {
            pthread_create(&hilo_quantum, NULL, (void*)contar_quantum, (void*)(intptr_t)socket_INT);

            sem_post(&sem_contador_quantum);
        }

        esperar_cpu(pcb); 

        if(strcmp(algoritmo_planificacion,"RR") == 0) { // Plantear una mejor forma de hacer esto
            pthread_cancel(hilo_quantum);
        }

        // Y sino sigue sin crear ni eliminar el hilo...
    }
}

void* contar_quantum(void* sockets_CPU) {
    int socket_CPU = (intptr_t)sockets_CPU;
    sem_wait(&sem_contador_quantum);
    sleep(quantum);

    int interrupcion_cpu = INTERRUPCION_QUANTUM;
    send(socket_CPU, &interrupcion_cpu, sizeof(int), 0);
    return NULL;
}


t_pcb* proximo_a_ejecutar() { // Esta pensando solo para FIFO y RR
    int ready_vacio = 0;
    t_pcb* pcb = NULL;

    pthread_mutex_lock(&mutex_estado_ready); 
    ready_vacio = queue_is_empty(cola_ready);
    if(!ready_vacio) {
        pcb = queue_pop(cola_ready); 
    } else {
        printf("No hay procesos para ejecutar\n");
    }
    pthread_mutex_unlock(&mutex_estado_ready);
    
    return pcb; // NO deberia devolver null
}

// TO DO: Esto bloquearia la planificacion hasta que la cpu termine de ejecutar y me devuelva el contexto. 
// Tiene que recibir causa de desalojo y contexto

t_paquete* recibir_cpu() {
    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        printf("Esperando recibir paquete\n");
        recv(client_dispatch, &(paquete->codigo_operacion), sizeof(int), MSG_WAITALL);
        printf("Recibi el codigo de operacion : %d\n", paquete->codigo_operacion);

        recv(client_dispatch, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        paquete->buffer->stream = malloc(paquete->buffer->size);

        recv(client_dispatch, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);
        return paquete;
    }
}

void esperar_cpu(t_pcb* pcb) {
    t_paquete* package = malloc(sizeof(t_pcb));

    DesalojoCpu devolucion_cpu;
    
    package = recibir_cpu(); // pcb y codigo de operacion (devolucion_cpu)
    devolucion_cpu = package->codigo_operacion;

    switch (devolucion_cpu) {
        case INTERRUPCION_QUANTUM:
            pcb = deserializar_pcb(package->buffer);
            pasar_a_ready(pcb, EXEC);
            break;
        case DORMIR_INTERFAZ:
            t_operacion_io* operacion_io = deserializar_io(package->buffer);
            dormir_io(operacion_io);
            break;
        case FIN_PROCESO:
            pcb = deserializar_pcb(package->buffer); 
            //liberar_memoria(); // Por ahora esto seria simplemente decirle a memoria que elimine el PID del diccionario
            change_status(pcb, EXIT); 
            sem_post(&sem_grado_multiprogramacion); // Esto deberia liberar un grado de memoria para que acceda un proceso
            free(pcb);
            break;
        default:
            printf("Llego a default de la 333 en funcionalidades.c\n");
            break;
    }
    
    free(package); 
}

t_operacion_io* deserializar_io(t_buffer* buffer) {
    t_operacion_io* operacion_io = malloc(sizeof(t_operacion_io));

    void* stream = buffer->stream;

    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(operacion_io->unidadesDeTrabajo), stream, sizeof(int));
    stream += sizeof(int);

    memcpy(&(operacion_io->length_interfaz), stream, sizeof(int));
    stream += sizeof(int);

    operacion_io->interfaz = malloc(operacion_io->length_interfaz); 

    memcpy(operacion_io->interfaz, stream, operacion_io->length_interfaz);
    
    printf("Interfaz: %s\n", operacion_io->interfaz);
    printf("Unidades de trabajo: %d\n", operacion_io->unidadesDeTrabajo);
    return operacion_io;
}

/* 
void dormir_io(t_operacion_io* operacion_io) {
    pthread_t dormir_interfaz;
    pthread_create(&dormir_interfaz, NULL, (void*) hilo_dormir_io, operacion_io); 
}
*/

void dormir_io(t_operacion_io* operacion_io) {

    // bool existe_io = dictionary_has_key(diccionario_io, operacion_io->interfaz);
    int resultado_io = validar_io(operacion_io);

    if(resultado_io) {
        t_conjuntoInterfaz* data_interfaz = dictionary_get(diccionario_io, operacion_io->interfaz);

        t_paquete* paquete = malloc(sizeof(t_paquete));
        t_buffer* buffer = malloc(sizeof(t_buffer));

        buffer->size = sizeof(int);

        buffer->offset = 0;
        buffer->stream = malloc(buffer->size);

        memcpy(buffer->stream + buffer->offset, &, sizeof(uint32_t));
        buffer->offset += sizeof(uint32_t);
        memcpy(buffer->stream + buffer->offset, &persona, sizeof(uint8_t));
        buffer->offset += sizeof(uint8_t);
        
        paquete->codigo_operacion = DORMITE; // Podemos usar una constante por operación
        paquete->buffer = buffer; // Nuestro buffer de antes.

        // Armamos el stream a enviar
        void* a_enviar = malloc(buffer->size + sizeof(int));
        int offset = 0;

        memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
        offset += sizeof(int);
        memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
        offset += sizeof(int);
        memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

        // Por último enviamos
        // send(&elemento, a_enviar, buffer->size + sizeof(int), 0); -> COMENTO POR AHORA

        // No nos olvidamos de liberar la memoria que ya no usaremos
        free(a_enviar);
        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }
}


void change_status(t_pcb* pcb, Estado new_status) {
    pcb->estadoAnterior = pcb -> estadoActual;
    pcb->estadoActual   = new_status;
}

void* pasar_a_exec(t_pcb* pcb) {
    change_status(pcb, EXEC);

    pthread_mutex_lock(&mutex_estado_exec);
    queue_push(cola_exec, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_exec);

    log_info(logger_kernel, "PID: %d - Estado Anterior en exec: %d - Estado Actual: %d", pcb->pid, pcb->estadoAnterior, pcb->estadoActual);

    return NULL;
}

// Esta podria ser la funcion generica y pasarle el codOP por parametro

// Aca se desarma el path y se obtienen las instrucciones y se le pasan a memoria para que esta lo guarde en su tabla de paginas de este proceso
t_pcb *crear_nuevo_pcb(int pid){
    t_pcb *pcb = malloc(sizeof(t_pcb));
    t_registros* registros_pcb = malloc(sizeof(t_registros));

    pcb->pid = pid;
    pcb->program_counter = 0; // Deberia ser un num de instruccion de memoria?
    pcb->quantum = quantum;   // Esto lo saco del config
    pcb->estadoActual = NEW;
    pcb->estadoAnterior = NEW;
    pcb->registros = inicializar_registros_cpu(registros_pcb);

    return pcb;
}

/* 
void* recibir_peticion_de_cpu() {
    // recibiremos la instruccion
}

typedef struct {    
    char* instruccion; 
    int instruccion_length;  
} t_peticion_io;

typedef struct {
    uint32_t size; // Tamaño del payload
    uint32_t offset; // Desplazamiento dentro del payload
    void* stream; // Payload
} t_buffer;

void* enviar_peticion_a_io(instruccion) {
    t_peticion_io peticion = {
        instruccion = ;
        instruccion_length = instruccion.length();
    }
    
    serializar_peticion(peticion);
}

void serializar_peticion(t_peticion_io* peticion){
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(uint32_t) * 3 // DNI, Pasaporte y longitud del nombre
                + sizeof(uint8_t) // Edad
                + peticion.instruccion_length; // La longitud del string nombre.
                                        // Le habíamos sumado 1 para enviar tambien el caracter centinela '\0'.
                                        // Esto se podría obviar, pero entonces deberíamos agregar el centinela en el receptor.

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    // Para la instruccion primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream + offset, &peticion.instruccion_length, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + offset, peticion.instruccion, peticion.instruccion_length);
    // No tiene sentido seguir calculando el desplazamiento, ya ocupamos el buffer completo

    buffer->stream = stream;

    // Si usamos memoria dinámica para el nombre, y no la precisamos más, ya podemos liberarla:
    free(peticion->instruccion);
}
*/
/*
void* escuchar_consola(int socket_kernel_escucha, t_config* config){
    while(1) {

        pthread_t thread_type;
        int cliente_fd = esperar_cliente(socket_kernel_escucha);

        void* funcion_recibir_cliente(void* cliente_fd){
            log_info(_kernellogger_kernel, "Conectado con una consola");
            recibir_cliente(cliente_fd, config);// post recibiendo perdemos las cosas
            return NULL;
        }
        pthread_create(&thread_type, NULL, funcion_recibir_cliente, (void*) &cliente_fd);
    }

    return NULL;
}
*/


/*
void* FINALIZAR_PROCESO(t_pcb* pcb) {
    queue_push(cola_exit, pcb);
    liberar_pcb(pcb_a_eliminar);
}

void* liberar_pcb(t_pcb* pcb) {
    free(pcb->instrucciones);
    free(pcb);
}
*/

void EJECUTAR_SCRIPT() {
    // Aca se deberia recibir el path del script y desarmarlo para obtener las instrucciones
    // Luego se le pasan a memoria para que esta las guarde en su tabla de paginas
}

void FINALIZAR_PROCESO() {
}

void INICIAR_PLANIFICACION() {
}

void DETENER_PLANIFICACION() {
}


void MULTIPROGRAMACION() {
    /* 
    int nuevo_grado_multiprogramacion;
    printf("Ingrese el nuevo grado de multiprogramacion\n");
    scanf(&nuevo_grado_multiprogramacion);
    grado_multiprogramacion = nuevo_grado_multiprogramacion;
    */
}

void PROCESO_ESTADO() {

}

/* 
void PROCESO_ESTADO()
{
    enum Estado estadoNuevo;
    printf("Ingrese el estado en mayusculas que quiere listar\n");
    scanf(&estadoNuevo);

    printf("--------Listado de procesos--------\n");
    switch (estadoNuevo)
    {
    case NEW:
        cola_new = mostrarCola(cola_new);
        break;
    case READY:
        cola_ready = mostrarCola(cola_ready);
        break;
    case BLOCKED:
        cola_blocked = mostrarCola(cola_blocked);
        break;
    case EXEC:
        cola_exec = mostrarCola(cola_exec);
        break;
    case EXIT:
        cola_new = mostrar_cola(cola_new);
        break;
    }
    // No es necesario default porque ya te rompe el enum
}

t_queue* mostrar_cola(t_queue* cola)
{
    t_pcb *aux;
    t_queue *cola_aux = NULL;

    while (cola->elements->head != NULL)
    {
        aux = queue_pop(&cola);
        mostrar_pcb_proceso(aux);
        queue_push(&cola_aux, aux);
    }

    return cola_aux;
}
*/
void mostrar_pcb_proceso(t_pcb* pcb)
{
    printf("PID: %d\n", pcb->pid);
    printf("Program Counter: %d\n", pcb->program_counter);
    // printf("Instrucciones: %d\n", pcb->instrucciones);
    printf("Estado Actual: %d\n", pcb->estadoActual);
    printf("Estado Anterior: %d\n", pcb->estadoAnterior);

    // TODO: Revisar mostrar los registros por pantalla y probarlo

    if(strcmp(algoritmo_planificacion, "FIFO") == 0) {
    printf("Quantum: %d\n", pcb->quantum);
    }
}

/*
void* serializar_instrucciones(t_list* instrucciones, int size) {
    void* stream = malloc(size);
    size_t size_payload = size - sizeof(t_msj_kernel_consola) - sizeof(size_t);
    int desplazamiento = 0;

    t_msj_kernel_consola op_code = LIST_INSTRUCCIONES;
    memcpy(stream + desplazamiento, &op_code, sizeof(op_code));
    desplazamiento += sizeof(op_code);

    memcpy(stream + desplazamiento, &size_payload, sizeof(size_t));
    desplazamiento += sizeof(size_t);

    for(int i = 0; i < list_size(instrucciones); i++) {
		t_instruccion* instruccion = list_get(instrucciones, i);

		size_t largo_nombre = strlen(instruccion->nombre) + 1;
		memcpy(stream + desplazamiento, &largo_nombre, sizeof(size_t));
		desplazamiento += sizeof(size_t);

		memcpy(stream + desplazamiento, instruccion->nombre, largo_nombre);
		desplazamiento += largo_nombre;

		t_list* parametros = instruccion->parametros;
		for(int j = 0; j < list_size(parametros); j++) {
			char* parametro = list_get(parametros, j);

			size_t largo_nombre = strlen(parametro) + 1;
			memcpy(stream + desplazamiento, &largo_nombre, sizeof(size_t));
			desplazamiento += sizeof(size_t);

			memcpy(stream + desplazamiento, parametro, largo_nombre);
			desplazamiento += largo_nombre;
	    }
	}

    return stream;
}*/