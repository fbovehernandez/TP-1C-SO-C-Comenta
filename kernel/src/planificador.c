#include "../include/funcionalidades.h"

void change_status(t_pcb* pcb, Estado new_status) {
    pcb->estadoAnterior = pcb -> estadoActual;
    pcb->estadoActual   = new_status;
    log_info(logger_kernel, "PID: %d - Estado Anterior: %d - Estado Actual: %d", pcb->pid, pcb->estadoAnterior, pcb->estadoActual);
}

void pasar_a_ready(t_pcb *pcb) {
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
    
    sem_post(&sem_hay_para_planificar);
}

void pasar_a_exec(t_pcb* pcb) {
    change_status(pcb, EXEC);
    //pthread_mutex_lock(&mutex_estado_exec);
    //queue_push(cola_exec, (void *)pcb);
    //pthread_mutex_unlock(&mutex_estado_exec);
    // pcb_exec = pcb;
    enviar_pcb(pcb, client_dispatch, ENVIO_PCB, NULL);
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

/*
int* obtenerPidsDe(t_queue* cola){
    t_queue colaAux = NULL;
    t_pcb* pcbAux;
    int* pids = NULL; 
    int indice = 0; 

    while(cola != NULL){
        pcbAux = queue_pop(&cola);
        pids[indice] = pcbAux->pid;
        indice++;
        queue_push(colaAux,pcbAux);
    }

    cola = colaAux;
    return pids;
}
*/