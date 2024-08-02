#include "../include/funcionalidades.h"

// Definicion de variables globales
int grado_multiprogramacion;
int quantum_config;
int contador_pid = 0;
int client_dispatch;
pthread_t escucha_consola;

int contador_aumento_instancias = 0;
int cantidad_de_waits_que_se_hicieron = 0;

sem_t sem_grado_multiprogramacion;
sem_t sem_hay_pcb_esperando_ready;
sem_t sem_hay_para_planificar;
sem_t sem_contador_quantum;
sem_t sem_planificadores;
// bool hay_proceso_en_exec;

char* algoritmo_planificacion; // Tomamos en convencion que los algoritmos son "FIFO", "VRR" , "RR" (siempre en mayuscula)
t_log* logger_kernel;
char* path_kernel;
ptr_kernel* datos_kernel;

t_temporal* timer;
int ms_transcurridos;
bool planificacion_pausada;

pthread_t hilo_detener_planificacion;

///////////////////
///// CONSOLA /////
///////////////////

void *interaccion_consola() {
    char *input;
    char path_ejecutable[256];
    int respuesta = 0;
    int valor;
    int pid_deseado_a_numero;

    while (respuesta != 8) {
        input = readline(
            "--------------- Consola interactiva Kernel ---------------\n"
            "Elija una opcion (numero):\n"
            "1- Ejecutar Script\n"
            "2- Iniciar proceso\n"
            "3- Finalizar proceso\n"
            "4- Iniciar planificacion\n"
            "5- Detener planificacion\n"
            "6- Listar procesos por estado\n"
            "7- Cambiar el grado de multiprogramacion\n"
            "Seleccione una opción: \n");
        
        if (input) {
            add_history(input);
            respuesta = atoi(input);
            free(input);

        switch (respuesta) {
            case 1:
                input = readline(">Ingrese el path del script a ejecutar: ");
                if (input) {
                    add_history(input);
                    strncpy(path_ejecutable, input, sizeof(path_ejecutable) - 1);
                    path_ejecutable[sizeof(path_ejecutable) - 1] = '\0';
                    free(input);
                    EJECUTAR_SCRIPT(path_ejecutable);
                }
                break;
            case 2:
                input = readline(">Ingrese el path del script a ejecutar: ");
                if (input) {
                    add_history(input);
                    strncpy(path_ejecutable, input, sizeof(path_ejecutable) - 1);
                    path_ejecutable[sizeof(path_ejecutable) - 1] = '\0';
                    free(input);
                    INICIAR_PROCESO(path_ejecutable);
                }
                break;
            case 3:
                input = readline(">Ingrese el pid del proceso que quiere finalizar: ");
                if (input) {
                    add_history(input);
                    pid_deseado_a_numero = atoi(input);
                    free(input);
                    FINALIZAR_PROCESO(pid_deseado_a_numero);
                }
                break;
            case 4:
                INICIAR_PLANIFICACION();
                break;
            case 5:
                DETENER_PLANIFICACION();
                break;
            case 6:
                PROCESO_ESTADO();
                break;
            case 7:
                input = readline(">Ingrese grado de multiprogramacion a cambiar: ");
                if (input) {
                    add_history(input);
                    valor = atoi(input);
                    free(input);
                    MULTIPROGRAMACION(valor);
                }
                break;
            case 8:
                exit(1);
                break;
            default:
                break;
            }
        }
    }                                   

    
    return NULL;
}

///// COMANDOS

void EJECUTAR_SCRIPT(char* path) {
    char* path_local_kernel = strdup(path_kernel);
    if (path_local_kernel == NULL) {
        printf("Error al duplicar path_kernel.\n");
        return;
    }

    // Calcular el tamaño necesario para path_completo
    size_t path_completo_len = strlen(path_local_kernel) + strlen(path) + 1; // +1 para el null terminator
    char* path_completo = malloc(path_completo_len);
    
    if (path_completo == NULL) {
        printf("Error al asignar memoria para path_completo.\n");
        free(path_local_kernel);
        return;
    }

    // Construir el path completo
    strcpy(path_completo, path_local_kernel);
    strcat(path_completo, path);

    // Abrir el archivo en modo de lectura (r)
    FILE* file = fopen(path_completo, "r");

    // Verificar si el archivo se abrió correctamente
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
    } else {
        aplicar_sobre_cada_linea_del_archivo(file, NULL, (void*) _ejecutarComando);
        fclose(file);
    }

    // Liberar la memoria
    free(path_local_kernel);
    free(path_completo);
}

/* 
Sin embargo, hay un problema en tu código. Estás asignando memoria para path_completo, pero luego estás asignando el resultado de 
strcat(path_local_kernel, path) a path_completo. Esto causa una fuga de memoria porque pierdes la referencia a 
la memoria que acabas de asignar a path_completo.
*/

void INICIAR_PROCESO(char* path_instrucciones) {
    if(planificacion_pausada) {
        printf("No podes crear proceso si esta pausada la plani\n");
        return;
    }

    sem_wait(&sem_planificadores);

    int pid_actual = obtener_siguiente_pid();
    printf("El pid del proceso es: %d\n", pid_actual);
    // Primero envio el path de instruccion a memoria y luego el PCB...
        
    enviar_path_a_memoria(path_instrucciones, sockets, pid_actual); 

    t_pcb *pcb = crear_nuevo_pcb(pid_actual); 
    
    sem_post(&sem_planificadores);
 
    encolar_a_new(pcb);
}

void FINALIZAR_PROCESO(int pid) {
    if (planificacion_pausada){
        printf("No podes finalizar proceso si esta pausada la plani\n");
        return;
    }
    
    t_queue* cola = encontrar_en_que_cola_esta(pid);
    
    if(cola == NULL) { // Esto quiere decir que esta en la cola exit
        printf("No se puede finalizar un proceso que esta en exit \n");
        return;
    }
    
    if(cola == cola_exec && !queue_is_empty(cola_exec)) {
        printf("El proceso se encuentra en ejecucion\n");
        int temp = INTERRUPCION_FIN_USUARIO;
        send(sockets->socket_int, &temp, sizeof(int), 0);
    } else {
        t_pcb* pcb = sacarDe(cola, pid);
        ejecutar_signal_de_recursos_bloqueados_por(pcb);
        pasar_a_exit(pcb, "INTERRUPTED_BY_USER");
    }
    
    // Ya se hace free en pasar a exit
}

void INICIAR_PLANIFICACION() {
    if(!planificacion_pausada) {
        printf("La planificacion no esta pausada\n");
        return;
    } 
    
    planificacion_pausada = false;
    printf("Reiniciando planificaciones\n"); 

    sem_post(&sem_planificadores);
}

void DETENER_PLANIFICACION() {
    if(planificacion_pausada) {
        printf("La planificacion ya esta detenida\n");
        return;
    }

    planificacion_pausada = true;
    printf("Deteniendo planificaciones\n");   

    pthread_create(&hilo_detener_planificacion, NULL, detener_planificaciones, NULL); 
    pthread_join(hilo_detener_planificacion, NULL);
}

void PROCESO_ESTADO() {
    printf("--------Listado de procesos--------\n");
    
    printf("Procesos en NEW\n");
    mostrar_cola_con_mutex(cola_new, &mutex_estado_new);
    
    printf("Procesos en READY\n");
    mostrar_cola_con_mutex(cola_ready, &mutex_estado_ready);

    printf("Procesos en READY+\n");
    mostrar_cola_con_mutex(cola_ready_plus, &mutex_estado_ready_plus);
    
    printf("Procesos en BLOCKED\n");
    mostrar_cola_con_mutex(cola_blocked, &mutex_estado_blocked);

    printf("Proceso en EXEC\n");
    mostrar_cola_con_mutex(cola_exec, &mutex_estado_exec);
}

void MULTIPROGRAMACION(int valor) {
    if(grado_multiprogramacion < valor) {
        for(int i=0; i < valor-grado_multiprogramacion; i++) {
            sem_post(&sem_grado_multiprogramacion);
        }
    } else if(grado_multiprogramacion > valor) {
        for(int i=0; i < grado_multiprogramacion-valor; i++) {
            sem_wait(&sem_grado_multiprogramacion);
        }
    }
    grado_multiprogramacion = valor;
}

////////////////////////////////////////////////
///// FUNCIONES COMPLEMENTARIAS A COMANDOS /////
////////////////////////////////////////////////

void _ejecutarComando(void* _, char* linea_leida) {
    char comando[20]; 
    char parametro[20]; 
    int parametroAux;

    if (sscanf(linea_leida, "%s %s", comando, parametro) == 2) {
        printf("Palabra 1: %s, Palabra 2: %s\n", comando, parametro);
        if(strcmp(comando, "INICIAR_PROCESO") == 0) {
            INICIAR_PROCESO(parametro);
        } else if(strcmp(comando, "FINALIZAR_PROCESO") == 0) {
            parametroAux = atoi(parametro);
            FINALIZAR_PROCESO(parametroAux);
        } else if(strcmp(comando, "MULTIPROGRAMACION") == 0) {
            parametroAux = atoi(parametro);
            MULTIPROGRAMACION(parametroAux);
        }
    } else {
        sscanf(linea_leida, "%s", comando);
        printf("Palabra única: %s\n", comando);
        if(strcmp(comando, "INICIAR_PLANIFICACION") == 0) {
            INICIAR_PLANIFICACION();
        } else if(strcmp(comando, "DETENER_PLANIFICACION") == 0) {
            DETENER_PLANIFICACION();
        } else if(strcmp(comando, "PROCESO_ESTADO") == 0) {
            PROCESO_ESTADO();
        } 
    }
}

void* detener_planificaciones() {
    sem_wait(&sem_planificadores);
    return NULL;
}

/////////////////////////
///// PLANIFICACION /////
/////////////////////////

void *planificar_corto_plazo(void *sockets_necesarios) {
    pthread_t hilo_quantum;
    int valor_del_sem = 0;

    t_sockets *sockets = (t_sockets *)sockets_necesarios;
    t_pcb *pcb;

    int socket_CPU = sockets->socket_cpu;
    int socket_INT = sockets->socket_int;

    while (1) {
        printf("Esperando a que haya un proceso para planificar\n");
        sem_wait(&sem_hay_para_planificar);
     
        sem_wait(&sem_planificadores);

        pthread_mutex_lock(&no_hay_nadie_en_cpu);
        pcb = proximo_a_ejecutar();
        pasar_a_exec(pcb);
        
        pthread_mutex_unlock(&no_hay_nadie_en_cpu);
        
        if (es_RR()) {
            pthread_create(&hilo_quantum, NULL, (void*) esperar_RR, (void *)pcb);
            sem_post(&sem_contador_quantum);
        } else if (es_VRR()) {
            pthread_create(&hilo_quantum, NULL, esperar_VRR, (void *)pcb);
            sem_post(&sem_contador_quantum);
        }

        sem_post(&sem_planificadores);
       
        
        esperar_cpu();
    
        
        if (es_VRR_RR()) {
            pthread_cancel(hilo_quantum);
            pthread_join(hilo_quantum, NULL); // Deberia liberar los recursos del cancel
        }
    }
}

t_pcb *proximo_a_ejecutar() {
    t_pcb *pcb = NULL;
    
    if (!queue_is_empty(cola_prioritarios_por_signal)) {
        pthread_mutex_lock(&mutex_prioritario_por_signal);
        pcb = queue_pop(cola_prioritarios_por_signal);
        pthread_mutex_unlock(&mutex_prioritario_por_signal);
    } else if (!queue_is_empty(cola_ready_plus)){
        pthread_mutex_lock(&mutex_estado_ready_plus);
        pcb = queue_pop(cola_ready_plus);
        pthread_mutex_unlock(&mutex_estado_ready_plus);
    } else if (!queue_is_empty(cola_ready)) {
        pthread_mutex_lock(&mutex_estado_ready);
        pcb = queue_pop(cola_ready);
        pthread_mutex_unlock(&mutex_estado_ready);
    } else {
        log_error(logger_kernel,"No hay procesos para ejecutar.");
        exit(1);
    }
    
    return pcb;
}

t_pcb* crear_nuevo_pcb(int pid){
    t_pcb *pcb = malloc(sizeof(t_pcb));
    t_registros* registros_pcb = malloc(sizeof(t_registros)); 

    pcb->pid = pid;
    pcb->program_counter = 0; // Deberia ser un num de instruccion de memoria?
    pcb->quantum = quantum_config;   // Esto lo saco del config
    pcb->estadoActual = NEW;
    pcb->estadoAnterior = NEW;
    pcb->registros = inicializar_registros_cpu(registros_pcb);

    return pcb;
}

int obtener_siguiente_pid() {
    contador_pid++;
    return contador_pid;
}

void encolar_a_new(t_pcb *pcb) {
    sem_wait(&sem_planificadores);

    pthread_mutex_lock(&mutex_estado_new);
    queue_push(cola_new, pcb);
    pthread_mutex_unlock(&mutex_estado_new);

    log_info(logger_kernel, "Se crea el proceso: %d en NEW", pcb->pid);
    printf("Hay proceso esperando en la cola de NEW!\n");

    sem_post(&sem_planificadores);
    sem_post(&sem_hay_pcb_esperando_ready);
}

void* a_ready() {
    while (1) {
        sem_wait(&sem_hay_pcb_esperando_ready);
        sem_wait(&sem_grado_multiprogramacion);

        printf("Grado de multiprogramación permite agregar proceso a ready\n");

        pthread_mutex_lock(&mutex_estado_new);
        t_pcb *pcb = queue_pop(cola_new);
        pthread_mutex_unlock(&mutex_estado_new);

        sem_wait(&sem_planificadores);
        pasar_a_ready(pcb);
        sem_post(&sem_planificadores);
    }
}

bool es_VRR_RR() {
    return es_RR() || es_VRR();
}

bool es_RR() {
    return strcmp(algoritmo_planificacion, "RR") == 0;
}

bool es_VRR() {
    return strcmp(algoritmo_planificacion, "VRR") == 0;
}

void* esperar_VRR(void* pcb) {
    t_pcb* pcb_nuevo = (t_pcb*) pcb;
    timer = temporal_create(); // Crearlo ya empieza a contar
    esperar_RR(pcb);
    // temporal_destroy(timer);

    /*int socket_Int = (intptr_t)socket_Int;
    sem_wait(&sem_contador_quantum);*/

    // posible send de interrupcion
    return NULL;
}

void* esperar_RR(void* pcb) {
    /* Esperar_cpu_RR */
    t_pcb* pcb_nuevo = (t_pcb*) pcb;
    int quantum = pcb_nuevo->quantum;
    int socket_Int = sockets->socket_int;
    
    sem_wait(&sem_contador_quantum);
    codigo_operacion interrupcion_cpu = INTERRUPCION_CPU;
    usleep(quantum * 1000);

    send(socket_Int, &interrupcion_cpu, sizeof(codigo_operacion), 0);
    
    printf("Envie interrupcion para PID %d despues de %d\n", pcb_nuevo->pid, pcb_nuevo->quantum);
    return NULL;
}

int leQuedaTiempoDeQuantum(t_pcb *pcb) {
    return pcb->quantum > 0;
}

void volver_a_settear_quantum(t_pcb* pcb) {
    pcb->quantum = quantum_config;
}

t_queue* encontrar_en_que_cola_esta(int pid) {
    if(esta_en_cola_pid(cola_new, pid, &mutex_estado_new)) {
        printf("Esta en cola new\n");
        return cola_new;
    } else if(esta_en_cola_pid(cola_ready, pid, &mutex_estado_ready)) {
        printf("Esta en cola ready\n");
        sem_wait(&sem_hay_para_planificar);
        return cola_ready;
    } else if(esta_en_cola_pid(cola_blocked, pid, &mutex_estado_blocked)) {
        printf("Esta en cola blocked\n");
        return cola_blocked; // analizar si hay que sacarlo tmb de la cola de procesos bloqueados de una io
    } else if(esta_en_cola_pid(cola_ready_plus, pid, &mutex_estado_ready_plus)){
        printf("Esta en cola ready PLUS\n");
        return cola_ready_plus;
    } else if(esta_en_cola_pid(cola_exec, pid, &mutex_estado_exec)) {
        printf("Esta en cola exec\n");
        return cola_exec;
    } else {
        printf("Esta en cola exit\n");
        return NULL;
    }
}

int esta_en_cola_pid(t_queue* cola, int pid, pthread_mutex_t* mutex) {
    t_queue* colaAux = queue_create();
    int esta_el_pid = 0;
    
    pthread_mutex_lock(mutex); // Se que es una zona critica grande pero podria afectar negativamente si lo  pongo cada vez que hago push - pop
    while (!queue_is_empty(cola)) {
        t_pcb* pcbAux = queue_pop(cola);
        
        if (pcbAux->pid != pid) {
            queue_push(colaAux, pcbAux);
        } else {   
            esta_el_pid = 1;
            queue_push(colaAux, pcbAux);
        }
    }

    // Restauro los elementos en la cola original
    while (!queue_is_empty(colaAux)) {
        queue_push(cola, queue_pop(colaAux));
    }
    pthread_mutex_unlock(mutex);
    
    queue_destroy(colaAux);
    
    return esta_el_pid;
}

/*
Detener planificación: Este mensaje se encargará de pausar la planificación de corto y largo plazo. El proceso que se encuentra en ejecución NO es desalojado, 
pero una vez que salga de EXEC se va a pausar el manejo de su motivo de desalojo. De la misma forma, los procesos bloqueados van a pausar su transición a la 
cola de Ready.
*/

void mostrar_cola_con_mutex(t_queue* cola,pthread_mutex_t* mutex){
    if(!queue_is_empty(cola)){
        pthread_mutex_lock(mutex);
        mostrar_cola(cola);
        pthread_mutex_unlock(mutex);  
    }else{
        printf("Cola vacia\n");
    }
      
}

void mostrar_cola(t_queue* cola) {
    t_pcb* aux;
    t_queue* cola_aux = queue_create(); 

    while (!queue_is_empty(cola)){
        aux = queue_pop(cola);
        imprimir_pcb(aux);
        queue_push(cola_aux, aux);
    }

    while (!queue_is_empty(cola_aux)){
        aux = queue_pop(cola_aux);
        queue_push(cola, aux);
    }

    queue_destroy(cola_aux); 
}

void mostrar_pcb_proceso(t_pcb* pcb) {
    printf("PID: %d\n", pcb->pid);
    printf("Program Counter: %d\n", pcb->program_counter);
    printf("Estado Actual: %d\n", pcb->estadoActual);
    printf("Estado Anterior: %d\n", pcb->estadoAnterior);
    if(strcmp(algoritmo_planificacion, "FIFO") == 0) {
        printf("Quantum: %d\n", pcb->quantum);
    }
}

///////////////
///// CPU /////
///////////////

// TO DO: Esto bloquearia la planificacion hasta que la cpu termine de ejecutar y me devuelva el contexto.
// Tiene que recibir causa de desalojo y contexto

t_paquete *recibir_cpu() {
    while (1) {
        t_paquete *paquete = malloc(sizeof(t_paquete));
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

// TO DO: Esto bloquearia la planificacion hasta que la cpu termine de ejecutar y me devuelva el contexto. 
// Tiene que recibir causa de desalojo y contexto

void esperar_cpu() { // Evaluar la idea de que esto sea otro hilo...
    DesalojoCpu devolucion_cpu;
    
    t_paquete* package = recibir_cpu(); // pcb y codigo de operacion (devolucion_cpu)

    devolucion_cpu = package->codigo_operacion;

    if(!queue_is_empty(cola_exec)) {
        pthread_mutex_lock(&mutex_estado_exec);
        t_pcb* pcb_anterior = queue_pop(cola_exec);
        printf("el pid del pcb anterior es: %d\n", pcb_anterior->pid);
        pthread_mutex_unlock(&mutex_estado_exec);

        liberar_pcb_estructura(pcb_anterior);
    }
    
    // Deserializo antes el pcb, me ahorro cierta logica y puedo hacer send si es IO_BLOCKED
    t_pcb* pcb = deserializar_pcb(package->buffer);

    printf("pid del pcb despues de desalojar: %d\n", pcb->pid);

    if (es_VRR()) {
        temporal_stop(timer); // Detenemos el temporizador

        int64_t tiempo_transcurrido = temporal_gettime(timer);  // Obtenemos el tiempo transcurrido en milisegundos
        temporal_destroy(timer);
   

        int64_t tiempo_restante = max(0, min(quantum_config, pcb->quantum - tiempo_transcurrido));  // Fede scarpa orgulloso
        printf("Tiempo restante: %ld ms\n", tiempo_restante); // jejejej

        // Ajustamos el quantum, asegurándonos de que no sea menor a 0
        pcb->quantum = tiempo_restante;
        printf("El quantum restante del proceso con PID %d es: %d\n", pcb->pid, pcb->quantum);
    }
      
    sem_wait(&sem_planificadores);
 
    switch (devolucion_cpu) {
        case ERROR_STDOUT: case ERROR_STDIN:
            ejecutar_signal_de_recursos_bloqueados_por(pcb);
            pasar_a_exit(pcb, "INVALID_INTERFACE");
            break;
        case OUT_OF_MEMORY:
            ejecutar_signal_de_recursos_bloqueados_por(pcb);
            pasar_a_exit(pcb, "OUT OF MEMORY");
            break;
        case INTERRUPCION_FIN_USUARIO:
            ejecutar_signal_de_recursos_bloqueados_por(pcb);
            pasar_a_exit(pcb, "INTERRUPTED_BY_USER");
            break;
        case INTERRUPCION_QUANTUM:
            log_info(logger_kernel, "PID: %d - Desalojado por fin de Quantum", pcb->pid);
            pasar_a_ready(pcb);
            break;
        case DORMIR_INTERFAZ:         
            t_operacion_io* operacion_io = deserializar_io(package->buffer);
            dormir_io(operacion_io, pcb);        
            break;
        case WAIT_RECURSO: case SIGNAL_RECURSO:
            t_parametro* recurso = deserializar_parametro(package->buffer);
            printf("el nombre del recurso es %s\n", recurso->nombre);  
    
            wait_signal_recurso(pcb, recurso->nombre, devolucion_cpu);
            
            free(recurso->nombre); //FREE? A LO ULTIMO
            free(recurso);
            break;
        case FIN_PROCESO:
            // pcb = deserializar_pcb(package->buffer); 
            // ejecutar_signal_de_recursos_bloqueados_por(pcb);
            pasar_a_exit(pcb, "SUCCESS");
            //liberar_memoria(pcb->pid); // Por ahora esto seria simplemente decirle a memoria que elimine el PID del diccionario
            //change_status(pcb, EXIT); 
            //sem_post(&sem_grado_multiprogramacion); // Esto deberia liberar un grado de memoria para que acceda un proceso
            // free(pcb);
            break;
        case PEDIDO_LECTURA:
            t_pedido* pedido_lectura = deserializar_pedido(package->buffer);
    
            encolar_datos_std(pcb, pedido_lectura);
            
            // liberar_pedido_escritura_lectura(pedido_lectura);
            break;
        case PEDIDO_ESCRITURA:
            t_pedido* pedido_escritura = deserializar_pedido(package->buffer); // llega bien el nombre de la interfaz
            printf("NOMBRE PEDIDO ESCRITURA: %s", pedido_escritura->interfaz);
            encolar_datos_std(pcb, pedido_escritura);


            // liberar_pedido_escritura_lectura(pedido_escritura);
            break;
        case FS_DELETE: case FS_CREATE:
            t_pedido_fs_create_delete* pedido_fs = deserializar_pedido_fs_create_delete(package->buffer);

            t_list_io* interfaz_crear_destruir = io_esta_en_diccionario(pcb, pedido_fs->nombre_interfaz);
            
            if (interfaz_crear_destruir != NULL) {
                // codigo_operacion operacion = (package->codigo_operacion == FS_CREATE) ? CREAR_ARCHIVO : ELIMINAR_ARCHIVO;
                codigo_operacion operacion = (devolucion_cpu == FS_CREATE) ? CREAR_ARCHIVO : ELIMINAR_ARCHIVO;

                // Esto se hace en la conexion, aca tiene que encolar el pedido
                encolar_fs_create_delete(operacion, pcb, pedido_fs, interfaz_crear_destruir);
                // enviar_buffer_fs(interfaz_crear_destruir->socket, pcb->pid, pedido_fs->longitud_nombre_archivo, pedido_fs->nombre_archivo, operacion);
                
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, pedido_fs->nombre_interfaz);
            }
            
            // free(pedido_fs);
            // Liberar lista...
            break;
        case FS_TRUNCATE:
            t_fs_truncate* pedido_fs_truncate = deserializar_pedido_fs_truncate(package->buffer);
            t_list_io* interfaz_truncate = io_esta_en_diccionario(pcb, pedido_fs_truncate->nombre_interfaz);

            if (interfaz_truncate != NULL) {
                // Esto se hace en la conexion, aca tiene que encolar el pedido
                encolar_fs_truncate(pcb, pedido_fs_truncate, interfaz_truncate);
                // enviar_buffer_fs(interfaz_truncate->socket, pcb->pid, pedido_fs_truncate->longitud_nombre_archivo, pedido_fs_truncate->nombre_archivo, TRUNCAR_ARCHIVO);
                printf("Operacion TRUNCATE\n");
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, pedido_fs_truncate->nombre_interfaz);
            }

            // free(pedido_fs_truncate);
            break;

        case ESCRITURA_FS:
        case LECTURA_FS:
            t_pedido_fs_escritura_lectura* fs_read_write = deserializar_pedido_fs_escritura_lectura(package->buffer);
            t_list_io* interfaz = io_esta_en_diccionario(pcb, fs_read_write->nombre_interfaz);

            printf("Registro archivo: %d\n", fs_read_write->registro_archivo);

            if (interfaz != NULL) {
                // codigo_operacion operacion = (package->codigo_operacion == ESCRITURA_FS) ? ESCRIBIR_FS_MEMORIA : LEER_FS_MEMORIA;
                codigo_operacion operacion = (devolucion_cpu == ESCRITURA_FS) ? ESCRIBIR_FS_MEMORIA : LEER_FS_MEMORIA;
                // Esto se hace en la conexion, aca tiene que encolar el pedido
                encolar_fs_read_write(operacion, pcb, fs_read_write, interfaz);

                // enviar_buffer_fs_escritura_lectura(pcb->pid,fs_read_write->largo_archivo,fs_read_write->nombre_archivo,fs_read_write->registro_direccion,fs_read_write->registro_tamanio,fs_read_write->registro_archivo,operacion);
                printf("Operacion: LECTURA_FS\n");
                // HASTA ACA LLEGA
                
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, fs_read_write->nombre_interfaz);
            }
            
            break;
        default:
            printf("Llego un codigo de operacion inexistente o rompio algo\n");
            exit(-1);
            break;
    }

    sem_post(&sem_planificadores);
    liberar_paquete(package);
}


/*
    IO_STDIN_READ: Cadena de comunicaccion
    CPU                     -->             KERNEL                                    -->       IO                                                       --> Memoria 
    Traduce dir fisica a     | 1- Coincide la interfaz con                             |   Ingresar por Teclado un valor con tam maximo registro_tamanio | En la direccionn fisica copia el valor mandado por la io con el tamanio registro tamanio
    partir de un registro    | la operacion?                                           |   y ese valor se lo  manda a memoria para que lo guarde         | 
    direccion                | 2- Se agrega el pcb, dir_fisica y reg_tam a la cola de  |   cod: GUARDAR_VALOR                                            |
                            | blocked de esa io                                       |   datos: direccion_fisica                                       |
    cod:   PEDIDO_LECTURA    | 3-Sino EXIT                       -                      |          registro_tamanio                                       |
    datos: nombre_interfaz   | cod: LEETE                                              |          valor_leido                                            |
        direccion_fisica  | datos: direccion_fisica                                 |
        registro_tamanio  |        registro_tamanio                                 |
*/

///////////////////////
///// FILE SYSTEM /////
///////////////////////

void enviar_buffer_fs(int socket_io,int pid,int longitud_nombre_archivo,char* nombre_archivo, codigo_operacion codigo_operacion) {
    t_buffer* buffer = llenar_buffer_nombre_archivo_pid(pid,longitud_nombre_archivo,nombre_archivo);
    enviar_paquete(buffer,codigo_operacion,socket_io);
} 

t_buffer* llenar_buffer_nombre_archivo_pid(int pid,int largo_archivo,char* nombre_archivo){
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) + sizeof(largo_archivo) + sizeof(int);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &(largo_archivo), sizeof(int)); 
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &nombre_archivo, largo_archivo);
    
    buffer->stream = stream;

    free(nombre_archivo);
    return buffer;
}

////////////////////////
///// STDIN|STDOUT /////
////////////////////////

void encolar_datos_std(t_pcb* pcb, t_pedido* pedido) {
    t_list_io* interfaz;
    interfaz = io_esta_en_diccionario(pcb, pedido->interfaz);

    if(interfaz != NULL) {
        int socket_io = interfaz->socket; // lo voy a necesitar?

        io_std* datos_std = malloc(sizeof(io_std));

        datos_std->lista_direcciones = list_create();

        for(int i = 0; i < list_size(pedido->lista_dir_tamanio); i++) {
            t_dir_fisica_tamanio* original = list_get(pedido->lista_dir_tamanio, i);
            t_dir_fisica_tamanio* copia = malloc(sizeof(t_dir_fisica_tamanio));
            copia->direccion_fisica = original->direccion_fisica;
            copia->bytes_lectura = original->bytes_lectura;
            list_add(datos_std->lista_direcciones, copia);
        }
    
        // datos_std->lista_direcciones = pedido->lista_dir_tamanio;
        datos_std->registro_tamanio  = pedido->registro_tamanio;
        datos_std->cantidad_paginas  = pedido->cantidad_paginas;
        datos_std->pcb               = pcb;
              
        printf("\nAca va la lista de bytes DEL STDOUT/STDIN ANTES DE ENCOLAR\n");
        for(int i=0; i < list_size(datos_std->lista_direcciones); i++) {
            t_dir_fisica_tamanio* dir = list_get(datos_std->lista_direcciones, i);
            printf("Direccion fisica: %d\n", dir->direccion_fisica);
            printf("Bytes a leer: %d\n", dir->bytes_lectura);
        }

        printf("datos_std->registro_tamanio = %d\n", datos_std->registro_tamanio);
        printf("datos_std->cantidad_paginas = %d\n", datos_std->cantidad_paginas);
        printf("datos_std->pcb->pid = %d\n", datos_std->pcb->pid);
        
        // imprimir_datos_stdin();
        if(interfaz->TipoInterfaz == STDOUT) {
            pthread_mutex_lock(&mutex_cola_io_stdout); // cambiar nombre_mutex
            list_add(interfaz->cola_blocked, datos_std); // VER_SI_HAY_FREE
            pthread_mutex_unlock(&mutex_cola_io_stdout);
        } else {
            pthread_mutex_lock(&mutex_cola_io_stdin); // cambiar nombre_mutex
            list_add(interfaz->cola_blocked, datos_std); // VER_SI_HAY_FREE
            pthread_mutex_unlock(&mutex_cola_io_stdin);
        }

        pasar_a_blocked(pcb);

        log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->pid, interfaz->nombreInterfaz);   

        // use lo que estaba, antes decia sem_post al mutex, por eso, era un binario o eso entendi
        pthread_mutex_lock(&mutex_lista_io);
        sem_post(interfaz->semaforo_cola_procesos_blocked);
        pthread_mutex_unlock(&mutex_lista_io);
    } 

    liberar_pedido(pedido);
} 

void liberar_pedido(t_pedido* pedido) {
    // Libera cada elemento de la lista lista_dir_tamanio
    for(int i = 0; i < list_size(pedido->lista_dir_tamanio); i++) {
        t_dir_fisica_tamanio* dir = list_get(pedido->lista_dir_tamanio, i);
        free(dir);
    }

    list_destroy(pedido->lista_dir_tamanio);

    // Libera la interfaz y el pedido
    free(pedido->interfaz);
    free(pedido);
}

////////////////////
///// GENERICA /////
////////////////////

void dormir_io(t_operacion_io* io, t_pcb* pcb) {
    // VER_SI_HAY_FREE

    // bool existe_io = dictionary_has_key(diccionario_io, operacion_io->interfaz);
    t_list_io* elemento_encontrado = io_esta_en_diccionario(pcb, io->nombre_interfaz);

    // INVALID_INTERFACE aparece en io_esta_en_diccionario

    // t_list_io* elemento_encontrado = validar_io(io, pcb); 
    // Aca tambien deberia cambiar su estado a blocked o Exit respectivamnete
    // capaz estaba mal el nombre y al ser null no lo agarraba y el print rompia
    if(elemento_encontrado != NULL) { // resultado_okey
        io_gen_sleep* datos_sleep = malloc(sizeof(io_gen_sleep)); //VER_SI_HAY_FREE

        datos_sleep->unidad_trabajo = io->unidadesDeTrabajo;
        datos_sleep->pcb = pcb;
                
        pasar_a_blocked(pcb);

        pthread_mutex_lock(&mutex_cola_io_generica);
        list_add(elemento_encontrado->cola_blocked, datos_sleep);
        pthread_mutex_unlock(&mutex_cola_io_generica);

        log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->pid, io->nombre_interfaz);  

        // use lo que estaba, antes decia sem_post al mutex, por eso, era un binario o eso entendi
        pthread_mutex_lock(&mutex_lista_io);
        sem_post(elemento_encontrado->semaforo_cola_procesos_blocked);
        pthread_mutex_unlock(&mutex_lista_io);
        free(io->nombre_interfaz);
        free(io);
    } else {
        printf("ES NULL LA IO\n");
        free(io->nombre_interfaz);
        free(io);
        return;
    }
}

/*  Al recibir una petición de I/O de parte de la CPU primero se deberá validar que exista y esté
conectada la interfaz solicitada, en caso contrario, se deberá enviar el proceso a EXIT.
En caso de que la interfaz exista y esté conectada, se deberá validar que la interfaz admite la
operación solicitada, en caso de que no sea así, se deberá enviar el proceso a EXIT. */



// Esta podria ser la funcion generica y pasarle el codOP por parametro

// Funcion para crear un nuevo PCB.

////////////////////
///// RECURSOS /////
////////////////////

void wait_signal_recurso(t_pcb* pcb, char* key_nombre_recurso, DesalojoCpu desalojoCpu){ 
    printf("Hace %s con el pid %d en recurso %s\n", pasar_string_desalojo_recurso(desalojoCpu), pcb->pid, key_nombre_recurso);
    if(dictionary_has_key(datos_kernel->diccionario_recursos, key_nombre_recurso)) {
        t_recurso* recurso_obtenido = dictionary_get(datos_kernel->diccionario_recursos, key_nombre_recurso);
        if(desalojoCpu == WAIT_RECURSO) {
            int esta_bloqueado = ejecutar_wait_recurso(recurso_obtenido, pcb);

            if(esta_bloqueado) {
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, key_nombre_recurso);
            }
        } else {
            ejecutar_signal_recurso(recurso_obtenido, pcb, false);
        }
    } else {
        pasar_a_exit(pcb, "INVALID_RESOURCE");
    }
    imprimir_diccionario_recursos();
}

int ejecutar_wait_recurso(t_recurso* recurso_obtenido, t_pcb* pcb) {
    if(recurso_obtenido->instancias > 0) {        
        recurso_obtenido->instancias--;
        char* pid_string = string_itoa(pcb->pid);
        list_add(recurso_obtenido->procesos_que_lo_retienen, pid_string);
        pasar_a_ready(pcb);
        return 0;    
    } else {
        pasar_a_blocked(pcb);
        list_add(recurso_obtenido->procesos_bloqueados, pcb);
        return 1;
    }
}

void ejecutar_signal_recurso(t_recurso* recurso_obtenido, t_pcb* pcb, bool esta_para_finalizar) {
    char* pid_string = string_itoa(pcb->pid);
     
    if(esta_el_pid_en_cola_de_procesos_que_retienen(pid_string, recurso_obtenido)) {
        recurso_obtenido->instancias++;
        bool encontro_algo_la_lista = list_remove_element(recurso_obtenido->procesos_que_lo_retienen, pid_string);

        if(!esta_para_finalizar) {
            pthread_mutex_lock(&mutex_prioritario_por_signal);
            queue_push(cola_prioritarios_por_signal, pcb);
            pthread_mutex_unlock(&mutex_prioritario_por_signal);
            sem_post(&sem_hay_para_planificar);
        }
    }

    if(!list_is_empty(recurso_obtenido->procesos_bloqueados)) {
        t_pcb* proceso_liberado = list_remove(recurso_obtenido->procesos_bloqueados, 0);
        t_pcb* pcb_sacado = sacarDe(cola_blocked, proceso_liberado->pid);
        // char* pid_liberado = string_itoa(proceso_liberado->pid);
        /*list_add(recurso_obtenido->procesos_que_lo_retienen, pid_liberado);
        proceso_liberado->program_counter--;
        pasar_a_ready(proceso_liberado);*/

        ejecutar_wait_recurso(recurso_obtenido, pcb_sacado);
    }

    free(pid_string);
}

void ejecutar_signal_de_recursos_bloqueados_por(t_pcb* pcb) {
    t_list* elementos = dictionary_elements(datos_kernel->diccionario_recursos);
    char* pid_string = string_itoa(pcb->pid);
    for(int i=0; i<list_size(elementos); i++) {
        t_recurso* recurso = list_get(elementos, i);
        for(int j=0;j<list_size(recurso->procesos_que_lo_retienen); j++) {
            char* pid = list_get(recurso->procesos_que_lo_retienen, j);
            if(strcmp(pid, pid_string) == 0 ) { // 0 es verdadero en strcmp
                log_info(logger_kernel, "Entra a ejecutar signal recurso del EXIT el proceso: %d\n", pcb->pid);
                ejecutar_signal_recurso(recurso, pcb, true);
                break;
            }
        }

        for(int k=0; k<list_size(recurso->procesos_bloqueados); k++) {
           t_pcb* pcb_bloqueado = list_get(recurso->procesos_bloqueados, k);
           if(pcb_bloqueado->pid == pcb->pid) {
                list_remove(recurso->procesos_bloqueados, k);
                sacarDe(cola_blocked, pcb_bloqueado->pid);
                break;
           }
        }
    }

    free(pid_string);
    list_destroy(elementos);
}


char* pasar_string_desalojo_recurso(DesalojoCpu desalojoCpu){
    switch (desalojoCpu) {
    case WAIT_RECURSO:
        return "WAIT";
        break;
    case SIGNAL_RECURSO:
        return "SIGNAL";
        break;
    default:
        return "ERROR";
        break;
    }
}

void _imprimir_recurso(char* nombre, void* element) {
    t_recurso* recurso = (t_recurso*) element;
    printf("El recurso %s tiene %d instancias y %d procesos bloqueados.\n", nombre, recurso->instancias, list_size(recurso->procesos_bloqueados));
}

void imprimir_diccionario_recursos() {
    printf("\n");
    dictionary_iterator(datos_kernel->diccionario_recursos, (void*)_imprimir_recurso);
}  

bool esta_el_pid_en_cola_de_procesos_que_retienen(char* pid, t_recurso* recurso_obtenido) {
    bool seEncuentra = false;
    
    for(int i=0;i<list_size(recurso_obtenido->procesos_que_lo_retienen);i++) {
        char* pid_recibido = list_get(recurso_obtenido->procesos_que_lo_retienen, i);
        if(strcmp(pid_recibido, pid) == 0) {    
            seEncuentra = true;
        } 
    }

    return seEncuentra;
}

////////////////////
///// LIMPIEZA /////
////////////////////

// Las 2 funciones de abajo quedaron viejas, pero no las sacamos por miedo

void liberar_recursos(t_dictionary* recursos) {
    // Iterar sobre los elementos del diccionario y liberar cada recurso
    void liberar_recurso(char* clave, void* recurso) {
        t_recurso* recurso_sacado = (t_recurso*)recurso;
        list_destroy(recurso_sacado->procesos_bloqueados); // Liberar la lista de procesos bloqueados
        list_destroy(recurso_sacado->procesos_que_lo_retienen);
        free(recurso_sacado); // Liberar el recurso
    }

    dictionary_iterator(recursos, (void*)liberar_recurso);
    dictionary_destroy(recursos); // Liberar el diccionario en sí
}

void liberar_ios() {
    void _liberar_io(t_list_io* io) {
        free(io->nombreInterfaz);
        list_destroy_and_destroy_elements(io->cola_blocked, (void*)liberar_pcb);
        free(io);
    }
    
    if(diccionario_io != NULL){
        dictionary_destroy_and_destroy_elements(diccionario_io, (void*)_liberar_io);
    }
}

/*
Cosas para liberar un proceso:
Kernel:
planificador.c:
void pasar_a_exit(t_pcb* pcb) {
    change_status(pcb, EXIT);
    liberar_memoria(pcb->pid);
    sem_post(&sem_grado_multiprogramacion);
}
*/

void liberar_pcb(t_pcb* pcb) {
    enviar_eliminacion_pcb_a_memoria(pcb->pid);
    if (pcb != NULL) {
        // Primero, liberamos cualquier memoria que haya sido asignada a la subestructura t_registros
        if (pcb->registros != NULL) {
            free(pcb->registros);
            pcb->registros = NULL;  // Asegúrate de que el puntero no apunte a memoria liberada
        }

        // Luego, liberamos la memoria de la propia estructura t_pcb
        free(pcb);
    }

    // t_pcb* pcb = (t_pcb*)pcb_void; // Convertimos de void* a t_pcb*
    // enviar_eliminacion_pcb_a_memoria(pcb->pid);
    // liberar_pcb_estructura(pcb);
}

void liberar_pcb_de_recursos(int pid) {
    if(dictionary_is_empty(datos_kernel->diccionario_recursos)) return;
        
    t_list* elementos = dictionary_elements(datos_kernel->diccionario_recursos);

    for(int i=0; i<list_size(elementos); i++) {
        t_recurso* recurso = list_get(elementos, i);

        for(int j=0; j<list_size(recurso->procesos_bloqueados); j++){
            t_pcb* pcb = list_get(recurso->procesos_bloqueados, j);
            if(pcb->pid == pid) {
                t_pcb* pcb_removido = list_remove(recurso->procesos_bloqueados, j); // MEMORY_LEAK?
                recurso->instancias++;
                // liberar_pcb_estructura(pcb_removido);
            }
        }
    }
}


/*
void liberar_pcb_de_recursos(int pid) {
    bool tiene_pid_en_recurso(t_pcb* pcb) {
        return pcb->pid == pid; // este pid no lo toma
    }

    void _buscar_pid_entre_recursos(t_recurso* recurso) {
        if(!list_is_empty(recurso->procesos_bloqueados)) {
            if(list_filter(recurso->procesos_bloqueados, (void*)tiene_pid_en_recurso)) {
                recurso->instancias++;
            }
        
            list_remove_by_condition(recurso->procesos_bloqueados, (void*)tiene_pid_en_recurso);
        }
    }

    dictionary_iterator(datos_kernel->diccionario_recursos, (void*)_buscar_pid_entre_recursos);
}
*/

void liberar_recurso_de_pcb(int pid) {
    char* pid_string = string_itoa(pid);
    t_list* elementos = dictionary_elements(datos_kernel->diccionario_recursos);
    
    for(int i=0; i<list_size(elementos); i++) {
        t_recurso* recurso = list_get(elementos, i);

        for(int j=0; j<list_size(recurso->procesos_que_lo_retienen); j++){
            char* pid_obtenido = list_get(recurso->procesos_que_lo_retienen, j);
            if(pid_obtenido == pid_string) {
                char* pid_removido = list_remove(recurso->procesos_que_lo_retienen, j); 
            }
        }
    }
    free(pid_string);
}

void liberar_pcb_de_io (int pid) { //NO EXISTE en el .h y NO se utiliza
    t_list* IOs = dictionary_elements(diccionario_io);

    for(int i=0; i<list_size(IOs); i++) {
        t_list_io* IO = list_get(IOs, i);

        for(int j=0; j<list_size(IO->cola_blocked); j++) {
            int pid_obtenido;

            if(IO->TipoInterfaz == GENERICA) {
                io_gen_sleep* dato_sleep = list_get(IO->cola_blocked, j);
                pid_obtenido = dato_sleep->pcb->pid;
                free(dato_sleep);
            } else if(IO->TipoInterfaz == STDIN || IO->TipoInterfaz == STDOUT) {
                io_std* dato_std = list_get(IO->cola_blocked, j);
                pid_obtenido = dato_std->pcb->pid;
                liberar_datos_std(dato_std);
            } else {
                datos_operacion* dato_fs = list_get(IO->cola_blocked, j);
                pid_obtenido = dato_fs->pcb->pid;
                liberar_fs_puntero(dato_fs);
            }

            if(pid_obtenido == pid) {
                list_remove(IO->cola_blocked, j);
                break;
            }
        }
    }
    list_destroy(IOs);
}

void liberar_pedido_escritura_lectura(t_pedido* pedido_escritura_lectura) {
    free(pedido_escritura_lectura->interfaz);
    // list_clean_and_destroy_elements(pedido_escritura_lectura->lista_dir_tamanio,);
    free(pedido_escritura_lectura);
}


/////////////////////////////////////////////////////////////////////
///// SERIALIZACION|DESERIALIZACION|ENVIO|RECEPCION DE PAQUETES /////
/////////////////////////////////////////////////////////////////////

void enviar_path_a_memoria(char* path_instrucciones, t_sockets* sockets, int pid) {
// Mando PID para saber donde asociar las instrucciones
    int se_crearon_estructuras = 0;
    t_path* pathNuevo = malloc(sizeof(t_path));

    pathNuevo->path = strdup(path_instrucciones); 
    pathNuevo->path_length = strlen(pathNuevo->path) + 1;
    pathNuevo->PID = pid;

    t_buffer* buffer = llenar_buffer_path(pathNuevo);

    enviar_paquete(buffer, PATH, sockets->socket_memoria);
    recv(sockets->socket_memoria, &se_crearon_estructuras, sizeof(int), MSG_WAITALL);

    free(pathNuevo->path);
    free(pathNuevo);
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

t_pedido* deserializar_pedido(t_buffer* buffer) {
    t_pedido* pedido = malloc(sizeof(t_pedido));

    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6; 

    memcpy(&(pedido->cantidad_paginas), stream, sizeof(int));
    stream += sizeof(int);
    printf("Cantidad de paginas: %d\n", pedido->cantidad_paginas);

    pedido->lista_dir_tamanio = list_create();
    // Deserializar lista con dir fisica y tamanio en bytes a leer segun la cant de pags
    for(int i = 0; i < pedido->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio)); //VER_SI_HAY_FREE
        memcpy(&(dir_fisica_tam->direccion_fisica), stream, sizeof(int));
        stream += sizeof(int);
        memcpy(&(dir_fisica_tam->bytes_lectura), stream, sizeof(int));
        stream += sizeof(int);

        list_add(pedido->lista_dir_tamanio, dir_fisica_tam);
    }

    memcpy(&(pedido->registro_tamanio), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    printf("Registro tamanio: %d\n", pedido->registro_tamanio);

    memcpy(&(pedido->length_interfaz), stream, sizeof(int));
    stream += sizeof(int);

    // Se recibe mal el length
    printf("Length interfaz: %d\n", pedido->length_interfaz);

    pedido->interfaz = malloc(pedido->length_interfaz);
    memcpy(pedido->interfaz, stream, pedido->length_interfaz);

    // Insertar /0 y leer

    pedido->interfaz[pedido->length_interfaz] = '\0';
    printf("Interfaz: %s\n", pedido->interfaz);

    return pedido;
}

t_operacion_io* deserializar_io(t_buffer* buffer) {
    t_operacion_io* operacion_io = malloc(sizeof(t_operacion_io)); 

    // Lo siguiente es el tamaño del pcb
    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6;

    // Deserializamos los campos que tenemos en el buffer
    // stream += sizeof(t_pcb);
    
    memcpy(&(operacion_io->unidadesDeTrabajo), stream, sizeof(int));
    stream += sizeof(int);

    memcpy(&(operacion_io->nombre_interfaz_largo), stream, sizeof(int));
    stream += sizeof(int);

    operacion_io->nombre_interfaz = malloc(operacion_io->nombre_interfaz_largo); 

    memcpy(operacion_io->nombre_interfaz, stream, operacion_io->nombre_interfaz_largo);

    printf("Interfaz (funcionalidades): %s\n", operacion_io->nombre_interfaz);
    printf("Unidades de trabajo (unidades): %d\n", operacion_io->unidadesDeTrabajo);

    return operacion_io;
}

t_parametro* deserializar_parametro(t_buffer* buffer) {
    int largo_parametro;
    
    t_parametro* parametro = malloc(sizeof(t_parametro)); // sizeof(t_parametro) //FREE?

    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6;

    if (stream >= buffer->stream + buffer->size) {
        printf("No hay nada en el buffer\n");
        free(parametro->nombre);
        free(parametro);
        return NULL;
    }

    memcpy(&largo_parametro, stream, sizeof(int));
    stream += sizeof(int);

    parametro->length = largo_parametro;
    parametro->nombre = malloc(parametro->length); //FREE?

    memcpy(parametro->nombre, stream, parametro->length);

    parametro->nombre[parametro->length] = '\0';

    printf("el nombre del parametro es %s\n", parametro->nombre);

    return parametro; // free(parametro) y free(parametro->nombre)
}

void mandar_a_escribir_a_memoria(char* nombre_interfaz, int direccion_fisica, uint32_t tamanio){
    t_buffer* buffer = llenar_buffer_stdout(direccion_fisica, nombre_interfaz, tamanio);
    enviar_paquete(buffer, ESCRIBIR_STDOUT, sockets->socket_memoria);
}

t_buffer* llenar_buffer_stdout(int direccion_fisica,char* nombre_interfaz, uint32_t tamanio){
    t_buffer *buffer = malloc(sizeof(t_buffer));

    int largo_interfaz = string_length(nombre_interfaz);

    buffer->size = sizeof(int) + largo_interfaz + sizeof(uint32_t); 
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(stream + buffer->offset, &(largo_interfaz), sizeof(int)); // sizeof(largo_interfaz) ????
    buffer->offset += sizeof(int);
    
    memcpy(stream + buffer->offset, nombre_interfaz, largo_interfaz);
    
    buffer->stream = stream;

    free(nombre_interfaz);

    return buffer;
}

void enviar_eliminacion_pcb_a_memoria(int pid) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(int);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid, sizeof(int));
    enviar_paquete(buffer, LIBERAR_PROCESO, sockets->socket_memoria);
    // Problema con memoria, hay que ir a buscar sus frames y marcarlos como libres y debe ser una comunicacion directa de Kernel con Memoria
}

///////////////////////////
///// FUNCIONES AYUDA /////
///////////////////////////

int max(int num1, int num2) {
    return num1 > num2 ? num1 : num2;
}

t_list_io* io_esta_en_diccionario(t_pcb* pcb, char* interfaz_nombre) {
    if(dictionary_has_key(diccionario_io, interfaz_nombre)) {
        t_list_io* interfaz = dictionary_get(diccionario_io, interfaz_nombre);
        if(strcmp(interfaz_nombre, interfaz->nombreInterfaz) == 0) {
            printf("Son iguales los nombres\n");
        } else {
            printf("No son iguales los nombres\n");
        }
        return interfaz;
    } else {
        pasar_a_exit(pcb, "INVALID_INTERFACE");
        return NULL;
    }
}

t_list_io* validar_io(t_operacion_io* io, t_pcb* pcb) {
    /*
    bool match_nombre(void nodo_lista) {
        return strcmp(((t_list_io*)nodo_lista)->nombreInterfaz, io->nombre_interfaz) == 0;
    };
    */
    
    pthread_mutex_lock(&mutex_lista_io);
    // t_list_io* elemento_encontrado = list_find(lista_io, match_nombre); // Aca deberia buscar la interfaz en la lista de io
    t_list_io* elemento_encontrado = dictionary_get(diccionario_io, io->nombre_interfaz);

    if(elemento_encontrado == NULL || elemento_encontrado->TipoInterfaz != GENERICA) { // Deberia ser global
        pthread_mutex_unlock(&mutex_lista_io);
        printf("No existe la io o no admite operacion\n");
    } else { 
        pasar_a_blocked(pcb);
        pthread_mutex_unlock(&mutex_lista_io);
        return elemento_encontrado;    
    }
    
    return NULL;
}


///////////////////////////////////////
////////// NO SE ESTA USANDO //////////
///////////////////////////////////////

