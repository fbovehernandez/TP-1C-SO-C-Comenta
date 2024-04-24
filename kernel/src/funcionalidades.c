#include "../include/funcionalidades.h"

int interaccion_consola(uint32_t contador_pid) {
    
    int respuesta;

    do {
        printf("---------------Consola interactiva Kernel---------------\n");
        printf("Elija una opcion (numero)\n");
        printf("1- Ejecutar Script \n");
        printf("2- Iniciar proceso\n");
        printf("3- Finalizar proceso\n");
        printf("4- Iniciar planificacion\n");
        printf("5- Detener planificacion\n");
        printf("6- Listar procesos por estado\n");
        printf("7- Finalizar modulo\n");
        scanf("%d",respuesta);
        
        switch (respuesta) {
        case 1:
            EJECUTAR_SCRIPT(/* pathing del conjunto de instrucciones*/);
            break;
        case 2:
            INICIAR_PROCESO(contador_pid); // Esto va a crear un pcb que representa al proceso, y va a ponerlo en la cola de new (despues un hilo lo pasa a ready?)
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
            printf("Finalizacion del modulo\n");
            exit(1); 
            break;
        default:
            printf("No se encuentra el numero dentro de las opciones, porfavor elija una correcta\n");
            break;
        }

    } while(respuesta != 7); // Modificar respuesta si se agregan mas elementos en la consola
    return 0;
}

void INICIAR_PROCESO(uint32_t contador_pid) {
    // En algun punto voy a tener que sacar las instrucciones del path 
    uint32_t pid = obtener_nuevo_pid(contador_pid);
    crear_nuevo_pcb(pid);
    encolar_pcb_en_estado(pid, NEW);
}

void obtener_nuevo_pid(uint32_t contador_pid) { // Variable global
    (*contador_pid)++;
    return contador_pid;
}

// Aca se desarma el path y se obtienen las instrucciones y se le pasan a memoria para que esta lo guarde en su tabla de paginas de este proceso
void crear_nuevo_pcb(pid) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->program_counter = 0; // Deberia ser un num de instruccion de memoria?
    pcb->quantum = 0; // Esto lo saco del config
    pcb->instrucciones = 0; // Esto lo saco del PATH del script
    pcb->estadoActual = NEW;
    pcb->estadoAnterior = NEW;
    pcb->registros = iniciar_registros_cpu(); // como?
}

void encolar_pcb_en_estado(uint32_t pid, enum Estado estado) {
    t_pcb* pcb = obtener_pcb(pid);
    switch (estado) {
    case NEW:
        queue_push(cola_new, pcb);
        break;
    case READY:
        queue_push(cola_ready, pcb);
        break;
    case BLOCKED:
        queue_push(cola_io, pcb);
        break;
    case EXEC:
        queue_push(cola_procesos_sus_ready, pcb);
        break;
    case EXIT:
        // No se encola en ningun lado
        break;
    default:
        break;
    }
}