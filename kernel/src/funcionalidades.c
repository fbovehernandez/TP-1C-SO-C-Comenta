#include "../include/funcionalidades.h"

// Definicion de variables globales
int grado_multiprogramacion;
int quantum_config;
int contador_pid = 0;
int client_dispatch;
pthread_t escucha_consola;
int cantidad_bloqueados;
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
    int pid_actual = obtener_siguiente_pid();
    printf("El pid del proceso es: %d\n", pid_actual);
    // Primero envio el path de instruccion a memoria y luego el PCB...

    if(planificacion_pausada) {
        printf("No podes crear proceso si esta pausada la plani\n");
        return;
    }
    
    printf("La  planificacion permite iniciar proceso? \n");
    cantidad_bloqueados++;
    sem_wait(&sem_planificadores);
    
    printf("Si.\n");    
    
    enviar_path_a_memoria(path_instrucciones, sockets, pid_actual); 

    t_pcb *pcb = crear_nuevo_pcb(pid_actual); 
    
    sem_post(&sem_planificadores);
    cantidad_bloqueados--;
    encolar_a_new(pcb);
}

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

int obtener_siguiente_pid() {
    contador_pid++;
    return contador_pid;
}

void encolar_a_new(t_pcb *pcb) {
    printf("Esperando a que la planificacion se des-pause \n");
    cantidad_bloqueados++;
    sem_wait(&sem_planificadores); // Semaforo detener planificacion
    printf("Planificacion continua\n");
    
    pthread_mutex_lock(&mutex_estado_new);
    queue_push(cola_new, pcb);
    pthread_mutex_unlock(&mutex_estado_new);

    log_info(logger_kernel, "Se crea el proceso: %d en NEW", pcb->pid);
    sem_post(&sem_planificadores);
    cantidad_bloqueados--;
    printf ("Hay proceso esperando en la cola de NEW!\n");
    sem_post(&sem_hay_pcb_esperando_ready); // Incrementar el semáforo para indicar que hay un nuevo proceso
    
}

void* a_ready() {
    while (1) {
        // Esperar a que haya un proceso en la cola -> Esto espera a que literalment haya uno
        sem_wait(&sem_hay_pcb_esperando_ready); 
        sem_wait(&sem_grado_multiprogramacion); 
        printf("Grado de multiprogramación permite agregar proceso a ready\n");

        pthread_mutex_lock(&mutex_estado_new);
        t_pcb *pcb = queue_pop(cola_new);
        pthread_mutex_unlock(&mutex_estado_new);
        
        // Este ya lo traba new
        pasar_a_ready(pcb);
    }
}

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
        printf("Hay un proceso para planificar\n");

        cantidad_bloqueados++;
        sem_wait(&sem_planificadores);

        pthread_mutex_lock(&no_hay_nadie_en_cpu);
        pcb = proximo_a_ejecutar();
        pasar_a_exec(pcb);
        
        pthread_mutex_unlock(&no_hay_nadie_en_cpu);
        
        if (es_RR()) {
            printf("\nES RR... A dormir el hilo!\n\n");
            pthread_create(&hilo_quantum, NULL, (void*) esperar_RR, (void *)pcb);
            sem_post(&sem_contador_quantum);
        } else if (es_VRR()) {
            pthread_create(&hilo_quantum, NULL, esperar_VRR, (void *)pcb);
            sem_post(&sem_contador_quantum);
        }

        sem_post(&sem_planificadores);
        cantidad_bloqueados--;
        
        esperar_cpu();
    
        
        if (es_VRR_RR()) {
            pthread_cancel(hilo_quantum);
            pthread_join(hilo_quantum, NULL); // Deberia liberar los recursos del cancel
        }
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
    
    printf("ESTOY ENVIANDO LA INTERRUPCION DESPUES DEL SLEEP\n");

    send(socket_Int, &interrupcion_cpu, sizeof(codigo_operacion), 0);
    
    char* log_message = string_from_format("Envie interrupcion para PID %d despues de %d", pcb_nuevo->pid, pcb_nuevo->quantum);
    log_info(logger_kernel, "%s", log_message);
    free(log_message);
    
    return NULL;
}

void volver_a_settear_quantum(t_pcb* pcb) {
    pcb->quantum = quantum_config;
}

int max(int num1, int num2) {
    return num1 > num2 ? num1 : num2;
}

int leQuedaTiempoDeQuantum(t_pcb *pcb) {
    return pcb->quantum > 0;
}

t_pcb *proximo_a_ejecutar() {
    t_pcb *pcb = NULL;
    
    printf("Llega a proximo a ejecutar\n");

    if (!queue_is_empty(cola_prioritarios_por_signal)) {
        printf("Llega a prioritario por signal\n");
        pthread_mutex_lock(&mutex_prioritario_por_signal);
        pcb = queue_pop(cola_prioritarios_por_signal);
        pthread_mutex_unlock(&mutex_prioritario_por_signal);
    } else if (!queue_is_empty(cola_ready_plus)){
        printf("Llega a ready plus\n");
        pthread_mutex_lock(&mutex_estado_ready_plus);
        pcb = queue_pop(cola_ready_plus);
        pthread_mutex_unlock(&mutex_estado_ready_plus);
        log_info(logger_kernel, "Proximo PID de la cola Ready Plus: %d", pcb->pid);
    } else if (!queue_is_empty(cola_ready)) {
        printf("Llega a ready\n");
        pthread_mutex_lock(&mutex_estado_ready);
        pcb = queue_pop(cola_ready);
        pthread_mutex_unlock(&mutex_estado_ready);
    } else {
        log_error(logger_kernel,"No hay procesos para ejecutar.");
        exit(1);
    }
    
    return pcb;
}

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
        printf("el pid del pcb anterior es:%d", pcb_anterior->pid);
        pthread_mutex_unlock(&mutex_estado_exec);

        liberar_pcb_estructura(pcb_anterior);
    }
    
    // Deserializo antes el pcb, me ahorro cierta logica y puedo hacer send si es IO_BLOCKED
    t_pcb* pcb = deserializar_pcb(package->buffer);

    printf("pid del pcb despues de desalojar: %d\n", pcb->pid);

    // hay_proceso_en_exec = false;

    if (es_VRR()) {
        temporal_stop(timer); // Detenemos el temporizador

        int64_t tiempo_transcurrido = temporal_gettime(timer);  // Obtenemos el tiempo transcurrido en milisegundos
        temporal_destroy(timer);
       //  printf("Tiempo transcurrido: %ld ms\n", tiempo_transcurrido);

        int64_t tiempo_restante = max(0, min(quantum_config, pcb->quantum - tiempo_transcurrido));  // Fede scarpa orgulloso
        printf("Tiempo restante: %ld ms\n", tiempo_restante); // jejejej

        // Ajustamos el quantum, asegurándonos de que no sea menor a 0
        pcb->quantum = tiempo_restante;
        log_info(logger_kernel, "El quantum restante del proceso con PID %d es: %d", pcb->pid, pcb->quantum);
    }
    
    printf("Planificacion recibe el desalojo pero... ¿esta pausada?\n");
    cantidad_bloqueados++;
    sem_wait(&sem_planificadores);
    printf("No esta pausada.\n");
 
    switch (devolucion_cpu) {
        case ERROR_STDOUT: case ERROR_STDIN:
            pasar_a_exit(pcb, "INVALID_INTERFACE");
            break;
        case OUT_OF_MEMORY:
            pasar_a_exit(pcb, "OUT OF MEMORY");
            break;
        case INTERRUPCION_FIN_USUARIO:
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
            printf("\nLlega al final de WAIT_RECURSO SIGNAL_RECURSO\n\n");
            break;
        case FIN_PROCESO:
            // pcb = deserializar_pcb(package->buffer); 
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
            // log_info(logger_kernel,"PID: %d - Bloqueado por - %s", pcb->pid, pedido_escritura->interfaz);

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
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, pedido_fs_truncate->nombre_interfaz);
            }

            // free(pedido_fs_truncate);
            break;

        case ESCRITURA_FS:
        case LECTURA_FS:
            t_pedido_fs_escritura_lectura* fs_read_write = deserializar_pedido_fs_escritura_lectura(package->buffer);
            t_list_io* interfaz = io_esta_en_diccionario(pcb, fs_read_write->nombre_interfaz);

            if (interfaz != NULL) {
                // codigo_operacion operacion = (package->codigo_operacion == ESCRITURA_FS) ? ESCRIBIR_FS_MEMORIA : LEER_FS_MEMORIA;
                codigo_operacion operacion = (devolucion_cpu == ESCRITURA_FS) ? ESCRIBIR_FS_MEMORIA : LEER_FS_MEMORIA;
                // Esto se hace en la conexion, aca tiene que encolar el pedido
                encolar_fs_read_write(operacion, pcb, fs_read_write, interfaz);

                // enviar_buffer_fs_escritura_lectura(pcb->pid,fs_read_write->largo_archivo,fs_read_write->nombre_archivo,fs_read_write->registro_direccion,fs_read_write->registro_tamanio,fs_read_write->registro_archivo,operacion);
                
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, fs_read_write->nombre_interfaz);
            }
            
            break;
        default:
            printf("Llego un codigo de operacion inexistente o rompio algo\n");
            exit(-1);
            break;
    }

    sem_post(&sem_planificadores);
    cantidad_bloqueados--;
    //liberar_pcb_estructura(pcb);
    liberar_paquete(package);
}

t_list_io* io_esta_en_diccionario(t_pcb* pcb, char* interfaz_nombre) {
    if(dictionary_has_key(diccionario_io, interfaz_nombre)) {
        t_list_io* interfaz = dictionary_get(diccionario_io, interfaz_nombre);
        printf("Nombre a comparar buscado %s\n", interfaz_nombre);
        printf("Nombre a comparar recibido %s\n", interfaz->nombreInterfaz);
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


void encolar_datos_std(t_pcb* pcb, t_pedido* pedido) {
    t_list_io* interfaz;
    interfaz = io_esta_en_diccionario(pcb, pedido->interfaz);

    if(interfaz != NULL) {
        int socket_io = interfaz->socket; // lo voy a necesitar?

        io_std* datos_std = malloc(sizeof(io_std));

        datos_std->lista_direcciones = pedido->lista_dir_tamanio;
        datos_std->registro_tamanio  = pedido->registro_tamanio;
        datos_std->cantidad_paginas  = pedido->cantidad_paginas;
        datos_std->pcb               = pcb;
        
        printf("Voy a imprimir los datos std antes de agregarlos a la cola de bloqueados de la interfaz\n");
        
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
            printf("entra aca\n");
            list_add(interfaz->cola_blocked, datos_std); // VER_SI_HAY_FREE
            pthread_mutex_unlock(&mutex_cola_io_stdout);
        } else {
            pthread_mutex_lock(&mutex_cola_io_stdin); // cambiar nombre_mutex
            printf("entra aca\n");
            list_add(interfaz->cola_blocked, datos_std); // VER_SI_HAY_FREE
            pthread_mutex_unlock(&mutex_cola_io_stdin);
        }

        pasar_a_blocked(pcb);

        log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->pid, interfaz->nombreInterfaz);   

        // use lo que estaba, antes decia sem_post al mutex, por eso, era un binario o eso entendi
        pthread_mutex_lock(&mutex_lista_io);
        sem_post(interfaz->semaforo_cola_procesos_blocked);
        printf("La operacion deberia realizarse...\n");
        pthread_mutex_unlock(&mutex_lista_io);
    } 

    // Habria que liberar toda la lista que se cargo adentro de pedido
    free(pedido->interfaz);
    // list_destroy(pedido->lista_dir_tamanio) -> Creo que no hay que liberar aca porque los malloc de la lista siguen estando en datos_std
    free(pedido);
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

void dormir_io(t_operacion_io* io, t_pcb* pcb) {
    // VER_SI_HAY_FREE

    // bool existe_io = dictionary_has_key(diccionario_io, operacion_io->interfaz);
    t_list_io* elemento_encontrado = io_esta_en_diccionario(pcb, io->nombre_interfaz);

    // INVALID_INTERFACE aparece en io_esta_en_diccionario

    // t_list_io* elemento_encontrado = validar_io(io, pcb); 
    // Aca tambien deberia cambiar su estado a blocked o Exit respectivamnete
    // capaz estaba mal el nombre y al ser null no lo agarraba y el print rompia
    if(elemento_encontrado != NULL) { // resultado_okey
        printf("El nombre de la interfaz es: %s\n", elemento_encontrado->nombreInterfaz); 

        io_gen_sleep* datos_sleep = malloc(sizeof(io_gen_sleep)); //VER_SI_HAY_FREE

        datos_sleep->unidad_trabajo = io->unidadesDeTrabajo;
        datos_sleep->pcb = pcb;
        
        printf("El pid de datos_sleep de dormir_io: %d\n", pcb->pid);
        
        pasar_a_blocked(pcb);

        pthread_mutex_lock(&mutex_cola_io_generica);
        list_add(elemento_encontrado->cola_blocked, datos_sleep);
        pthread_mutex_unlock(&mutex_cola_io_generica);

        log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->pid, io->nombre_interfaz);  

        // use lo que estaba, antes decia sem_post al mutex, por eso, era un binario o eso entendi
        pthread_mutex_lock(&mutex_lista_io);
        sem_post(elemento_encontrado->semaforo_cola_procesos_blocked);
        printf("La operacion deberia realizarse...\n");
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

t_list_io* validar_io(t_operacion_io* io, t_pcb* pcb) {
    /*
    bool match_nombre(void nodo_lista) {
        return strcmp(((t_list_io*)nodo_lista)->nombreInterfaz, io->nombre_interfaz) == 0;
    };
    */
    printf("Valida la io\n");
    
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

// Esta podria ser la funcion generica y pasarle el codOP por parametro

// Funcion para crear un nuevo PCB.
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

void wait_signal_recurso(t_pcb* pcb, char* key_nombre_recurso, DesalojoCpu desalojoCpu){ 
    printf("Hace %s con el pid %d en recurso %s\n", pasar_string_desalojo_recurso(desalojoCpu), pcb->pid, key_nombre_recurso);
    if(dictionary_has_key(datos_kernel->diccionario_recursos, key_nombre_recurso)) {
        printf("Tiene la llave.\n");
        t_recurso* recurso_obtenido = dictionary_get(datos_kernel->diccionario_recursos, key_nombre_recurso);
        if(desalojoCpu == WAIT_RECURSO) {
            printf("llego hasta WAIT\n");
            ejecutar_wait_recurso(recurso_obtenido, pcb, key_nombre_recurso);
            log_info(logger_kernel, "PID %d - Retengo el recurso: %s", pcb->pid, key_nombre_recurso);
        }else{
            printf("Llega hasta signal\n");
            //if(es_el_ultimo_recurso_a_liberar_por(pcb)) {
            //    ejecutar_signal_recurso(recurso_obtenido, pcb, true);
            //} else {
            // free(key_nombre_recurso);
            ejecutar_signal_recurso(recurso_obtenido, pcb, false);
            //}
            
        }
    } else {
        pasar_a_exit(pcb, "INVALID_RESOURCE");
    }
    
    imprimir_diccionario_recursos();
}

void ejecutar_wait_recurso(t_recurso* recurso_obtenido, t_pcb* pcb, char* recurso){
    if(recurso_obtenido->instancias > 0) {        
        recurso_obtenido->instancias --;
        cantidad_de_waits_que_se_hicieron++;

        printf("\n\nWAITS REALIZADOS: %d\n\n", cantidad_de_waits_que_se_hicieron);
        char* pid_string = string_itoa(pcb->pid);
        list_add(recurso_obtenido->procesos_que_lo_retienen, pid_string);

        pthread_mutex_lock(&mutex_prioritario_por_signal);
        queue_push(cola_prioritarios_por_signal, pcb); // Cuando dice devolver ejecucion, dice esperar a que salga el que esta ejecutando, o mandale una interrupcion
                                                      // y que se ejecute el proximo que tenemos en esta cola prioritaria?
                                                      // Tambien ver que hace wait y no hace signal y se libere el recurso -> ver sofi porfa
        pthread_mutex_unlock(&mutex_prioritario_por_signal);

        sem_post(&sem_hay_para_planificar);
    } else {
        pasar_a_blocked(pcb);
        list_add(recurso_obtenido->procesos_bloqueados, pcb);
        log_info(logger_kernel,"PID: %d - Bloqueado por: %s", pcb->pid, recurso);
    }
}

bool esta_el_pid_en_cola_de_procesos_que_retienen(int pid, t_recurso* recurso_obtenido) {
    bool seEncuentra = false;
    for(int i=0;i<list_size(recurso_obtenido->procesos_que_lo_retienen);i++) {
        char* pid_recibido = list_get(recurso_obtenido->procesos_que_lo_retienen, i);
        char* pid_string = string_itoa(pid);
        if(pid_recibido == pid_string) {
            seEncuentra = true;
        } 
        free(pid_string);
    }
    return seEncuentra;
}

void ejecutar_signal_recurso(t_recurso* recurso_obtenido, t_pcb* pcb, bool esta_para_finalizar) {
    //if(esta_el_pid_en_cola_de_procesos_que_retienen(pcb->pid, recurso_obtenido)) {
    //}
    printf("\n\nHAGO EL EJECUTAR SIGNAL RECURSO CON EL PID: %d\n\n", pcb->pid);
    log_info(logger_kernel, "CANTIDAD DE INSTANCIAS QUE TIENE EL RECURSO: %d", recurso_obtenido->instancias);
    recurso_obtenido->instancias++;
    contador_aumento_instancias++;
    printf("\n\nLA CANTIDAD DE VECES QUE SE EJECUTO SIGNAL ES: %d\n\n", contador_aumento_instancias);
    char* pid_string = string_itoa(pcb->pid);
    list_remove_element(recurso_obtenido->procesos_que_lo_retienen, pid_string);
    free(pid_string);
    
    printf("\nLLEGA HASTA EJECUTAR_SIGNAL_RECURSO\n\n");
    
    if(!esta_para_finalizar) {        
        printf("\nLlega hasta aca\n\n");
        pthread_mutex_lock(&mutex_prioritario_por_signal);
        queue_push(cola_prioritarios_por_signal, pcb);
        pthread_mutex_unlock(&mutex_prioritario_por_signal);
        sem_post(&sem_hay_para_planificar);
    }
    
    if(!list_is_empty(recurso_obtenido->procesos_bloqueados)) {
        t_pcb* proceso_liberado = list_remove(recurso_obtenido->procesos_bloqueados, 0);
        char* pid_liberado = string_itoa(proceso_liberado->pid);
        list_add(recurso_obtenido->procesos_que_lo_retienen, pid_liberado);
        log_info(logger_kernel, "Proceso liberado %s por %d", pid_liberado, pcb->pid);

        /*change_status(proceso_liberado, READY);
        queue_push(cola_prioritarios_por_signal, proceso_liberado); 
        sem_post(&sem_hay_para_planificar);*/
       
        t_pcb* proceso_liberado_desbloqueado = sacarDe(cola_blocked, proceso_liberado->pid); //ACA
        proceso_liberado_desbloqueado->program_counter--;
        pasar_a_ready(proceso_liberado);
    }
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

void FINALIZAR_PROCESO(int pid) {
    printf("Se ejecuta finalizar proceso\n");
    printf("el pid recibido es: %d\n", pid);
    
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
        printf("El proceso no se encuentra en ejecucion\n");
        pasar_a_exit(pcb, "INTERRUPTED_BY_USER");
    }
    
    // Ya se hace free en pasar a exit
}

void ejecutar_signal_de_recursos_bloqueados_por(t_pcb* pcb) {
    t_list* elementos = dictionary_elements(datos_kernel->diccionario_recursos);

    for(int i=0; i<list_size(elementos); i++) {
        t_recurso* recurso = list_get(elementos, i);
        for(int j=0;j<list_size(recurso->procesos_que_lo_retienen); j++) {
            char* pid = list_get(recurso->procesos_que_lo_retienen, j);
            char* pid_string = string_itoa(pcb->pid);
            if(strcmp(pid, pid_string) == 0 ) { // 0 es verdadero en strcmp
                log_info(logger_kernel, "Entra a ejecutar signal recurso del EXIT el proceso: %d\n", pcb->pid);
                ejecutar_signal_recurso(recurso, pcb, true);
                free(pid_string);
                break;
            }
            free(pid_string);
        }

        for(int k=0; k<list_size(recurso->procesos_bloqueados); k++) {
           t_pcb* pcb_bloqueado = list_get(recurso->procesos_bloqueados, k);
           if(pcb_bloqueado->pid == pcb->pid) {
                list_remove(recurso->procesos_bloqueados, k);
                break;
           }
        }
    }
    list_destroy(elementos);
}

// TODO: Buscar en las colas_blocked de io
// Estaba todo borrado cuando llegue

t_queue* encontrar_en_que_cola_esta(int pid) {
    if(esta_en_cola_pid(cola_new, pid, &mutex_estado_new)) {
        log_info(logger_kernel, "Esta en cola new\n");
        return cola_new;
    } else if(esta_en_cola_pid(cola_ready, pid, &mutex_estado_ready)) {
        log_info(logger_kernel, "Esta en cola ready\n");
        sem_wait(&sem_hay_para_planificar);
        return cola_ready;
    } else if(esta_en_cola_pid(cola_blocked, pid, &mutex_estado_blocked)) {
        log_info(logger_kernel, "Esta en cola blocked\n");
        return cola_blocked; // analizar si hay que sacarlo tmb de la cola de procesos bloqueados de una io
    } else if(esta_en_cola_pid(cola_ready_plus, pid, &mutex_estado_ready_plus)){
        log_info(logger_kernel, "Esta en cola ready PLUS\n");
        return cola_ready_plus;
    } else if(esta_en_cola_pid(cola_exec, pid, &mutex_estado_exec)) {
        log_info(logger_kernel, "Esta en cola exec\n");
        return cola_exec;
    } else {
        log_info(logger_kernel, "Esta en cola exit\n");
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

void INICIAR_PLANIFICACION() {
    if(!planificacion_pausada) {
        printf("La planificacion no esta pausada\n");
        return;
    } 
    
    planificacion_pausada = false;
    
    // sem_getvalue(&sem_planificadores, &cantidad_bloqueados);
    printf("Cantidad bloqueados: %d\n", cantidad_bloqueados);

    for(int i=0; i < cantidad_bloqueados; i++) {
        sem_post(&sem_planificadores);
    }

    cantidad_bloqueados = 0;

}

/*
Detener planificación: Este mensaje se encargará de pausar la planificación de corto y largo plazo. El proceso que se encuentra en ejecución NO es desalojado, 
pero una vez que salga de EXEC se va a pausar el manejo de su motivo de desalojo. De la misma forma, los procesos bloqueados van a pausar su transición a la 
cola de Ready.
*/

void DETENER_PLANIFICACION() {
    cantidad_bloqueados = 0;
    if(planificacion_pausada) {
        printf("La planificacion ya esta detenida\n");
        return;
    }

    planificacion_pausada = true;
    printf("Deteniendo planificaciones\n");   
    
    // SE QUEDA TRABADO  EN SCRIPT
    /*
    SOLUCION que vi  enn un issue
    - hacer un hilo con el semaforo de abajo
    */
    pthread_create(&hilo_detener_planificacion, NULL, (void*) detener_planificaciones, NULL); 
    pthread_join(hilo_detener_planificacion, NULL);
}

void* detener_planificaciones() {
    cantidad_bloqueados++;
    sem_wait(&sem_planificadores);
    return NULL;
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

    /*if(pcb_exec != NULL && hay_proceso_en_exec) {
        imprimir_pcb(pcb_exec);
    }else{
        printf("No hay proceso ejecutandose actualmente\n");
    }*/
}

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

/*

t_queue* mostrar_cola_pids(t_queue* cola)
{
    t_pcb *aux;
    t_queue *cola_aux = NULL;

    while (cola->elements->head != NULL)
    {
        aux = queue_pop(&cola);
        printf("%d\n",aux->pid);
        queue_push(&cola_aux, aux);
    }

    return cola_aux;
}
*/

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

/*
t_pedido_fs_create_delete* deserializar_pedido_fs_create_delete(t_buffer* buffer){
    t_pedido_fs_create_delete* pedido_fs = malloc(sizeof(t_pedido_fs_create_delete));

    void* stream = buffer->stream;
    
    memcpy(&(pedido_fs->longitud_nombre_interfaz), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_fs->nombre_interfaz), stream, sizeof(pedido_fs->longitud_nombre_interfaz));
    stream += pedido_fs->longitud_nombre_interfaz;
    memcpy(&(pedido_fs->longitud_nombre_archivo), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_fs->nombre_interfaz), stream, pedido_fs->longitud_nombre_archivo);

    return pedido_fs;    
}

void enviar_buffer_fs_escritura_lectura(int socket, int pid, int largo_archivo, char* nombre_archivo, uint32_t registro_direccion, uint32_t registro_tamanio,uint32_t registro_archivo, codigo_operacion operacion) {
    t_buffer* buffer = llenar_buffer_fs_escritura_lectura(pid, socket, largo_archivo,nombre_archivo,registro_direccion, registro_tamanio,registro_archivo);
    enviar_paquete(buffer, operacion, sockets->socket_memoria);
}

t_pedido_fs_escritura_lectura* deserializar_pedido_fs_escritura_lectura(t_buffer* buffer){
    t_pedido_fs_escritura_lectura* pedido_fs = malloc(sizeof(t_pedido_fs_create_delete));

    void* stream = buffer->stream;
    
    memcpy(&(pedido_fs->longitud_nombre_interfaz), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_fs->nombre_interfaz), stream, sizeof(pedido_fs->longitud_nombre_interfaz));
    stream += pedido_fs->longitud_nombre_interfaz;
    memcpy(&(pedido_fs->largo_archivo), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_fs->nombre_interfaz), stream, pedido_fs->largo_archivo);
    stream += pedido_fs->largo_archivo;
    memcpy(&(pedido_fs->registro_direccion), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pedido_fs->registro_tamanio), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pedido_fs->registro_archivo), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    // Si se agregann mas datos no olvidarse de aumentar el stream

    return pedido_fs;    
}

t_buffer* llenar_buffer_fs_escritura_lectura(int pid,int socket,int largo_archivo,char* nombre_archivo,uint32_t registro_direccion,uint32_t registro_tamanio,uint32_t registro_archivo){
   int size = sizeof(int) * 2 + largo_archivo + sizeof(uint32_t) * 2; 
   t_buffer *buffer = buffer_create(size);

    buffer_add_int(buffer,pid);
    buffer_add_int(buffer,socket);
    buffer_add_int(buffer,largo_archivo);
    buffer_add_string(buffer,largo_archivo,nombre_archivo);
    buffer_add_uint32(buffer,registro_direccion);
    buffer_add_uint32(buffer,registro_tamanio);
    buffer_add_uint32(buffer,registro_archivo); 

    return buffer;
}


t_pedido_fs_truncate* deserializar_fs_truncate(t_buffer* buffer){
    t_pedido_fs_truncate* fs_truncate = malloc(sizeof(t_pedido_fs_truncate));

    fs_truncate->largo_interfaz  = buffer_read_int(buffer);
    fs_truncate->nombre_interfaz = buffer_read_string(buffer,fs_truncate->largo_interfaz);
    fs_truncate->largo_archivo   = buffer_read_int(buffer);
    fs_truncate->nombre_archivo  = buffer_read_string(buffer,fs_truncate->largo_archivo);
    fs_truncate->truncador       = buffer_read_uint32(buffer);

    return fs_truncate;
}

t_buffer* llenar_buffer_fs_truncate(int pid,int largo_archivo,char* nombre_archivo,uint32_t truncador){
    int size = largo_archivo + sizeof(uint32_t) + sizeof(int) * 2;
    
    t_buffer* buffer = buffer_create(size);

    buffer_add_int(buffer,pid);
    buffer_add_int(buffer,largo_archivo);
    buffer_add_string(buffer,nombre_archivo,largo_archivo);
    buffer_add_uint32(buffer,truncador);

    return buffer;
}*/

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
        printf("entro a liberar_pcb");
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

void liberar_pcb_de_io (int pid) {
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


void enviar_eliminacion_pcb_a_memoria(int pid) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = sizeof(int);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &pid, sizeof(int));
    enviar_paquete(buffer, LIBERAR_PROCESO, sockets->socket_memoria);
    // Problema con memoria, hay que ir a buscar sus frames y marcarlos como libres y debe ser una comunicacion directa de Kernel con Memoria
}

