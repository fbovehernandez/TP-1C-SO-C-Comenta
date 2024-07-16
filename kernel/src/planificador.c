#include "../include/funcionalidades.h"

void change_status(t_pcb* pcb, Estado new_status) {
    pcb->estadoAnterior = pcb -> estadoActual;
    pcb->estadoActual   = new_status;

    char* estado_actual   = pasar_a_string_estado(pcb->estadoActual); 
    char* estado_anterior = pasar_a_string_estado(pcb->estadoAnterior);

    log_info(logger_kernel, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->pid, estado_anterior, estado_actual);
}

void pasar_a_ready(t_pcb *pcb) {
    if (es_VRR() && leQuedaTiempoDeQuantum(pcb)) {
        pasar_a_ready_plus(pcb);
    } else if(es_VRR()) {
        volver_a_settear_quantum(pcb);
    } else {
        pasar_a_ready_normal(pcb);
    }
    
    sem_post(&sem_hay_para_planificar);
}

void pasar_a_ready_normal(t_pcb* pcb) {
    change_status(pcb, READY);
    
    pthread_mutex_lock(&mutex_estado_ready);
    queue_push(cola_ready, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_ready);

    /*
    pthread_mutex_lock(&mutex_estado_ready);
    pids = obtenerPidsDe(&cola_ready);
    loggear_pids_de(pids);
    pthread_mutex_unlock(&mutex_estado_ready);
    */
}

void pasar_a_exec(t_pcb* pcb) {
    change_status(pcb, EXEC);
    //pthread_mutex_lock(&mutex_estado_exec);
    //queue_push(cola_exec, (void *)pcb);
    //pthread_mutex_unlock(&mutex_estado_exec);
    // pcb_exec = pcb;
    enviar_pcb(pcb, client_dispatch, ENVIO_PCB, NULL);
    //esperar_cpu();
}

void pasar_a_blocked(t_pcb* pcb) {
    change_status(pcb, BLOCKED);
    
    pthread_mutex_lock(&mutex_estado_blocked);
    queue_push(cola_blocked, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_blocked);
}

void pasar_a_exit(t_pcb* pcb) {
    change_status(pcb, EXIT);
    // liberar_memoria(pcb->pid);
    sem_post(&sem_grado_multiprogramacion);
}

void pasar_a_ready_plus(t_pcb* pcb){
    change_status(pcb, READY_PLUS);
    
    pthread_mutex_lock(&mutex_estado_ready_plus);
    queue_push(cola_ready_plus, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_ready_plus);

    pthread_mutex_lock(&mutex_estado_ready_plus);
    char* pids = obtener_pid_de(cola_ready_plus);
    log_info(logger_kernel,"Cola Ready Prioridad: %s\n", pids);
    pthread_mutex_unlock(&mutex_estado_ready_plus);
}

char* obtener_pid_de(t_queue* cola){
    t_queue* colaAux = queue_create(); 
    t_pcb* pcbAux;
    char* pids = string_new();

    while(!queue_is_empty(cola)){
        pcbAux = queue_pop(cola);
        string_append_with_format(&pids, "%d ", pcbAux->pid); 
        queue_push(colaAux, pcbAux);
    }

    // Restaurar los elementos en la cola original
    while(!queue_is_empty(colaAux)){
        pcbAux = queue_pop(colaAux);
        queue_push(cola, pcbAux);
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
