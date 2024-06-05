#include "../include/funcionalidades.h"

void change_status(t_pcb* pcb, Estado new_status) {
    pcb->estadoAnterior = pcb -> estadoActual;
    pcb->estadoActual   = new_status;
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

void pasar_a_exec(t_pcb* pcb) {
    change_status(pcb, EXEC);

    pthread_mutex_lock(&mutex_estado_exec);
    queue_push(cola_exec, (void *)pcb);
    pthread_mutex_unlock(&mutex_estado_exec);

    log_info(logger_kernel, "PID: %d - Estado Anterior en exec: %d - Estado Actual: %d", pcb->pid, pcb->estadoAnterior, pcb->estadoActual);
}

void pasar_a_blocked(t_pcb* pcb, Estado estadoAnterior) {
    pcb->estadoAnterior = estadoAnterior;
    change_status(pcb, BLOCKED);

    log_info(logger_kernel, "PID: %d - Estado Anterior: %d - Estado Actual: %d", pcb->pid, pcb->estadoAnterior, pcb->estadoActual);
}

void pasar_a_exit(t_pcb* pcb) {
    change_status(pcb, EXIT);
    liberar_memoria(pcb);
    // liberar_pcb(pcb);
    sem_post(&sem_grado_multiprogramacion);
}