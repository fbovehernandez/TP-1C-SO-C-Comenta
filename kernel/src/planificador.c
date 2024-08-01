#include "../include/funcionalidades.h"

void change_status(t_pcb* pcb, Estado new_status) {
    pcb->estadoAnterior = pcb -> estadoActual;
    pcb->estadoActual   = new_status;

    char* estado_actual   = pasar_a_string_estado(pcb->estadoActual); 
    char* estado_anterior = pasar_a_string_estado(pcb->estadoAnterior);

    log_info(logger_kernel, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->pid, estado_anterior, estado_actual);
}


void pasar_a_ready(t_pcb *pcb) {
    if (es_VRR() && leQuedaTiempoDeQuantum(pcb) && quantum_config != pcb->quantum) {
        pasar_a_ready_plus(pcb);
    } else {
        if(es_VRR()) {
            volver_a_settear_quantum(pcb);
        }

        pasar_a_ready_normal(pcb);
    }
    
    sem_post(&sem_hay_para_planificar);
    // sem_post(&sem_planificadores); // Mati: Puede que este semaforo este mejor arriba que el otro semaforo
    // cantidad_bloqueados--;
}

void pasar_a_ready_normal(t_pcb* pcb) {
    change_status(pcb, READY);
    
    pthread_mutex_lock(&mutex_estado_ready);
    queue_push(cola_ready, pcb);
    char* pids = obtener_pid_de(cola_ready);
    log_info(logger_kernel,"Cola Ready: %s\n", pids);
    pthread_mutex_unlock(&mutex_estado_ready);
    
    free(pids);
}
/*
void pasar_a_exec(t_pcb* pcb) {
    change_status(pcb, EXEC);
    
    memcpy(pcb_exec, pcb, sizeof(t_pcb));
    memcpy(pcb_exec->registros, pcb->registros, sizeof(t_registros));
    // pcb_exec = pcb;
    
    enviar_pcb(pcb, client_dispatch, ENVIO_PCB, NULL);

    liberar_pcb_estructura(pcb);
    // esperar_cpu();
}*/

void pasar_a_exec(t_pcb* pcb) {
    if(pcb == NULL){
        printf("Es el ultimo o estas metiendo cualquier cosa");
        return;
    }    
        
    change_status(pcb, EXEC);
    // hay_proceso_en_exec = true;

    pthread_mutex_lock(&mutex_estado_exec);
    queue_push(cola_exec, pcb);
    pthread_mutex_unlock(&mutex_estado_exec);
    
    
    printf("PID que se va a ejecutar: %d\n", pcb->pid);
    enviar_pcb(pcb, client_dispatch, ENVIO_PCB, NULL);
    
    // Liberar la estructura del PCB original
    // liberar_pcb_estructura(pcb); -> VOY A VER ESTO
}

void pasar_a_blocked(t_pcb* pcb) {
    change_status(pcb, BLOCKED);
    
    pthread_mutex_lock(&mutex_estado_blocked);
    queue_push(cola_blocked, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_blocked);
}

void pasar_a_exit(t_pcb* pcb, char* motivo_exit) {
    // printf("La planificacion permite pasar a exit \n");
    // cantidad_bloqueados++;
    // sem_wait(&sem_planificadores);
    // printf("Si.\n");
    change_status(pcb, EXIT);
    log_info(logger_kernel, "Finaliza el proceso %d - Motivo: %s", pcb->pid, motivo_exit);
  
    if(pcb->estadoAnterior != NEW) {
        sem_post(&sem_grado_multiprogramacion);
    }

    // liberar_pcb_de_recursos(pcb->pid);
    // liberar_pcb_de_io(pcb->pid); -------------> PARA CUANDO ESTA EN LA IO
    // liberar_pcb((void*)pcb);
    // liberar_recurso_de_pcb(pcb->pid);
    // printf("\nVoy a ejecutar signal de recursos bloqueados por !!!!\n\n");
    // ejecutar_signal_de_recursos_bloqueados_por(pcb);
    //printf("Va a liberar recurso de pcb %d que lo retiene\n", pcb->pid);

    if(pcb->estadoAnterior == BLOCKED) {
        liberar_pcb_de_io(pcb->pid);
    }

    enviar_eliminacion_pcb_a_memoria(pcb->pid);
    liberar_pcb_estructura(pcb);

    printf("Llego a liberar la estructura del pcb\n");

    // sem_post(&sem_planificadores);
    // cantidad_bloqueados--;
}

void pasar_a_ready_plus(t_pcb* pcb){
    change_status(pcb, READY_PLUS);
    
    pthread_mutex_lock(&mutex_estado_ready_plus);
    queue_push(cola_ready_plus, pcb);
    char* pids = obtener_pid_de(cola_ready_plus);
    log_info(logger_kernel,"Cola Ready Prioridad: %s\n", pids);
    pthread_mutex_unlock(&mutex_estado_ready_plus);
    
    free(pids);
}

char* obtener_pid_de(t_queue* cola){ //  OBLIGATORIO: Usar el mutex de la cola correspondiente antes de mandarlo
    t_queue* colaAux = queue_create(); 
    char* pids = string_new();

    while(!queue_is_empty(cola)) {
        t_pcb* pcbAux1 = queue_pop(cola);
        string_append_with_format(&pids, "%d ", pcbAux1->pid); 
        queue_push(colaAux, pcbAux1);
    }

    // Restaurar los elementos en la cola original
    while(!queue_is_empty(colaAux)) {
        t_pcb* pcbAux2 = queue_pop(colaAux);
        queue_push(cola, pcbAux2);
    }
         
    queue_destroy(colaAux); 
    return pids;
}
 
char* pasar_a_string_estado(Estado estado){
    switch (estado) {
        case READY:
            return "READY";
            break;
        case NEW:
            return "NEW";
            break;
        case BLOCKED:
            return "BLOCKED";
            break;
        case READY_PLUS:
            return "READY_PLUS";
            break;
        case EXEC:
            return "EXEC";
            break;
        case EXIT:
            return "EXIT";
            break;
        default:
            log_error(logger_kernel,"Invalid state");
            exit(-1);
            break;
    }
} 