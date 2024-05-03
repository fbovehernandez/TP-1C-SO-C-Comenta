#include "../include/funcionalidades.h"
#include "../include/conexiones.h"

// Definicion de variables globales
int grado_multiprogramacion;
int quantum;
int contador_pid = 0;

pthread_mutex_t mutex_estado_new;
pthread_mutex_t mutex_estado_ready;
sem_t sem_grado_multiprogramacion;
sem_t sem_hay_pcb_esperando_ready;

t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_blocked;
t_queue* cola_exec;
t_queue* cola_exit;

char* algoritmo_planificacion; // Tomamos en convencion que los algoritmos son "FIFO", "VRR" , "RR" (siempre en mayuscula)
t_log* logger_kernel;

void *interaccion_consola(t_sockets* sockets) {

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
        scanf(&respuesta);

        switch (respuesta)
        {
        case 1:
            EJECUTAR_SCRIPT(/* pathing del conjunto de instrucciones*/);
            break;
        case 2:
            INICIAR_PROCESO("PATH", sockets); // Esto va a crear un pcb que representa al proceso, y va a ponerlo en la cola de new (despues un hilo lo pasa a ready?)
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

void INICIAR_PROCESO(char* path_secuencia_de_comandos, t_sockets* sockets)
{
    int pid_actual = obtener_siguiente_pid();
    printf("El pid del proceso es: %d\n", pid_actual);

    t_pcb *pcb = crear_nuevo_pcb(pid_actual); // Aca dentro recibo el set de instrucciones del path
    // en el enunciado dice "en caso de que el grado de multiprogramacion lo permita"

    encolar_a_new(pcb);

    enviar_path_a_memoria(path_secuencia_de_comandos, sockets);
    // print_queue(NEW);
}


void* enviar_path_a_memoria(char* path_secuencia_de_comandos, t_sockets* sockets) {
    t_path* pathNuevo;
    pathNuevo -> path = path_secuencia_de_comandos;
    pathNuevo -> path_length = sizeof(pathNuevo);
    t_buffer* buffer = llenar_buffer(pathNuevo);

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = PATH; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(uint8_t)) + sizeof(uint32_t);
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));

    offset += sizeof(uint8_t);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    log_info(logger_kernel, "Se armo un paquete con este PATH: %s\n", paquete->buffer->stream);

    // Por último enviamos
    send(sockets->socket_memoria, a_enviar, buffer->size + sizeof(uint8_t) + sizeof(uint32_t), 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
} 

typedef struct {
    int path_length;
    char* path;
} t_path;

t_buffer* llenar_buffer(t_path* pathNuevo) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    void* stream = malloc(sizeof(pathNuevo));

    buffer->size = pathNuevo -> path_length;                     

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream + buffer->offset, &pathNuevo->path_length, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(buffer->offset + stream, &pathNuevo->path, pathNuevo->path_length);
    // No tiene sentido seguir calculando el desplazamiento, ya ocupamos el buffer 
   
    buffer->stream = stream;
    // Si usamos memoria dinámica para el path, y no la precisamos más, ya podemos liberarla:
    free(pathNuevo -> path);
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
        pasar_a_ready(pcb);
        log_info(logger_kernel, "Cola Ready <COLA>: [<LISTA DE PIDS>]");
        
        // sem_post(&sem_hay_pcb_esperando_ready); 
    }
}

void pasar_a_ready(t_pcb *pcb)
{
    pthread_mutex_lock(&mutex_estado_ready);
    queue_push(cola_ready, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_ready);

    pcb->estadoActual = READY;
    pcb->estadoAnterior = NEW;

    sem_post(&hay_para_planificar);
    log_info(logger_kernel, "PID: %d - Estado Anterior: %d - Estado Actual: %d", pcb->pid, pcb->estadoAnterior, pcb->estadoActual);
}

// Ver
void printf_queue(t_queue *cola)
{

    t_queue *actual = cola;

    while (actual != NULL)
    {
        t_pcb *pcb = actual->elements->head->data;

        printf("PID: %d\n", pcb->pid);
        printf("Program Counter: %d\n", pcb->program_counter);
        // printf("Quantum: %d\n", pcb->quantum);
        // printf("Instrucciones: %d\n", pcb->instrucciones);
        printf("Estado Actual: %d\n", pcb->estadoActual);
        printf("Estado Anterior: %d\n", pcb->estadoAnterior);
        // Assuming 'Registros' is a struct that can be printed
        // printf("Registros: %s\n", pcb->registros->toString());

        actual->elements->head = actual->elements->head->next;
    }
}


// Otro hilo para recibir de cpu? o uso solo recv?
void* planificar_corto_plazo(t_sockets* sockets) {
    // Esto no considera todavia la planificacion por quantum (desalojo) -> Como hago?
    // Si quantum se mide en segundos, yo podria enviar una interrupcion despues de 
    // contar los segundos (arrancaria a contar haciendo un post en la cpu cuando
    // arranque a ejecutar o reciba el pcb? bloqueando el hilo que cuenta con wait)

    t_pcb *pcb;

    int socket_CPU = sockets -> socket_cpu;
    
    while(1) {
        sem_wait(&hay_para_planificar); 
        pcb = proximo_a_ejecutar();
        
        pasar_a_exec(pcb);
        enviar_pcb(pcb, socket_CPU); // Serializar antes de enviar

        // TO DO: Recibir respuesta de la CPU y todo lo que le sigue
        int causa_desalojo = esperar_cpu(); // Esto bloquearia la planificacion hasta que la cpu termine de ejecutar y me devuelva el contexto
        // Tiene que recibir causa_desalojo y contexto_ejecucion
    }

    // Aca se deberia planificar el proceso que esta en la cola de ready
    // Se deberia sacar de la cola de ready y ponerlo en la cola de exec
    // Se deberia ejecutar el proceso
    // Se deberia poner en la cola de exit
    // No se como sera la implementacion de quantum (otro hilo?)
}

void* pasar_a_exec(t_pcb* pcb) {
    pthread_mutex_lock(&mutex_estado_exec);
    queue_push(cola_exec, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_exec);

    pcb->estadoActual = EXEC;
    pcb->estadoAnterior = READY;

    log_info(logger_kernel, "PID: %d - Estado Anterior: %d - Estado Actual: %d", pcb->pid, pcb->estadoAnterior, pcb->estadoActual);
}

void* enviar_pcb(t_pcb* pcb, int socket) {
    t_buffer* buffer = llenar_buffer(pcb);

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = ENVIO_PCB; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(codigo_operacion) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(codigo_operacion));

    offset += sizeof(codigo_operacion);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket, a_enviar, buffer->size + sizeof(codigo_operacion) + sizeof(int), 0);
}

t_buffer* llenar_buffer(t_pcb* pcb) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 4 
                + sizeof(enum Estado) * 2
                + sizeof(Registros);

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &pcb->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pcb->program_counter, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pcb->quantum, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pcb->socketProceso, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &pcb->estadoActual, sizeof(enum Estado));
    buffer->offset += sizeof(enum Estado);
    memcpy(stream + buffer->offset, &pcb->estadoAnterior, sizeof(enum Estado));
    buffer->offset += sizeof(enum Estado);
    memcpy(stream + buffer->offset, &pcb->registros, sizeof(Registros));
    buffer->offset += sizeof(Registros);

    buffer->stream = stream;

    return buffer;
}

t_pcb* proximo_a_ejecutar() {
    t_pcb* pcb = NULL;
    
    if(strcmp("FIFO",algoritmo_planificacion) == 0) {
        pthread_mutex_lock(&mutex_estado_ready);
        pcb = queue_pop(cola_ready);
        pthread_mutex_unlock(&mutex_estado_ready);
    } else if (strcmp("RR",algoritmo_planificacion) == 0){
        // Logica RR
    } else{
        // Logica VRR
    }

    return pcb;
}

// Aca se desarma el path y se obtienen las instrucciones y se le pasan a memoria para que esta lo guarde en su tabla de paginas de este proceso
t_pcb *crear_nuevo_pcb(int pid)
{
    t_pcb *pcb = malloc(sizeof(t_pcb));

    pcb->pid = pid;
    pcb->program_counter = 0; // Deberia ser un num de instruccion de memoria?
    pcb->quantum = quantum;   // Esto lo saco del config
    pcb->estadoActual = NEW;
    pcb->estadoAnterior = NEW;
    pcb->registros = inicializar_registros_cpu();

    return pcb;
}

Registros *inicializar_registros_cpu()
{
    Registros *registro_cpu = malloc(sizeof(Registros));

    registro_cpu->AX = 0;
    registro_cpu->BX = 0;
    registro_cpu->CX = 0;
    registro_cpu->DX = 0;

    registro_cpu->EAX = 0;
    registro_cpu->EBX = 0;
    registro_cpu->ECX = 0;
    registro_cpu->EDX = 0;

    registro_cpu->SI = 0;
    registro_cpu->DI = 0;

    return registro_cpu;
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

typedef struct {
    int pid;
    int program_counter;
    int quantum;
    enum Estado estadoActual;
    enum Estado estadoAnterior;
    Registros* registros;
} t_pcb;


*/
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