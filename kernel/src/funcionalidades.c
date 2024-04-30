#include "../include/funcionalidades.h"

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

void *interaccion_consola(/*t_consola* todo_lo_del_main_que_queremos*/) {

    int respuesta;

    do
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
            INICIAR_PROCESO(); // Esto va a crear un pcb que representa al proceso, y va a ponerlo en la cola de new (despues un hilo lo pasa a ready?)
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
        case 8:
            printf("Finalizacion del modulo\n");
            exit(1);
            break;
        default:
            printf("No se encuentra el numero dentro de las opciones, porfavor elija una correcta\n");
            break;
        }

    } while (respuesta != 7);

    return NULL;
}

void INICIAR_PROCESO()
{
    int pid_actual = obtener_siguiente_pid();
    printf("El pid del proceso es: %d\n", pid_actual);

    t_pcb *pcb = crear_nuevo_pcb(pid_actual); // Aca dentro recibo el set de instrucciones del path
    // en el enunciado dice "en caso de que el grado de multiprogramacion lo permita"

    encolar_a_new(pcb);
    // print_queue(NEW);
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