#include "../include/funcionalidades.h"
#include "../include/conexiones.h"
#include "../include/sockets.h"

// Definicion de variables globales
int grado_multiprogramacion;
int quantum_config;
int contador_pid = 0;
int client_dispatch;

pthread_mutex_t mutex_estado_new;
pthread_mutex_t mutex_estado_ready;
pthread_mutex_t mutex_estado_exec;
pthread_mutex_t mutex_estado_exit;
pthread_mutex_t no_hay_nadie_en_cpu;
pthread_mutex_t mutex_estado_blocked;
pthread_mutex_t mutex_estado_ready_plus;

sem_t sem_grado_multiprogramacion;
sem_t sem_hay_pcb_esperando_ready;
sem_t sem_hay_para_planificar;
sem_t sem_contador_quantum;

t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_blocked;
t_queue* cola_exit;
t_queue* cola_prioritarios_por_signal;
t_queue* cola_ready_plus;

char* algoritmo_planificacion; // Tomamos en convencion que los algoritmos son "FIFO", "VRR" , "RR" (siempre en mayuscula)
t_log* logger_kernel;
char* path_kernel;
ptr_kernel* datos_kernel;

t_temporal* timer;
int ms_transcurridos;
bool planificacion_pausada;

void *interaccion_consola() { //no se si deberian pasarse los sockets
    char *path_ejecutable = malloc(sizeof(char) * 100); // Cantidad?
    int respuesta = 0;
    int valor;
    while (respuesta != 8) {
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
        // /script_solo_cpu_1
        // /script_io_basico_1
        // /script_solo_cpu_3
        // /script_1
        switch (respuesta) {
        case 1:
            printf("Ingrese el path del script a ejecutar %s\n", path_ejecutable);
            
            scanf("%s", path_ejecutable);
            EJECUTAR_SCRIPT(path_ejecutable);
            break;
        case 2:
            printf("Ingrese el path del script a ejecutar %s\n", path_ejecutable);
            scanf("%s", path_ejecutable);
            INICIAR_PROCESO(path_ejecutable); // Ver problemas con caracteres como _ o /

            // INICIAR_PROCESO("/script_io_basico_1", sockets);

            // home/utnso/c-comenta/pruebas -> Esto tendria en memoria y lo uno con este que le mando -> Ver sockets como variable global
            break;
        case 3:
           // int pid_deseado;
            printf("Ingrese el pid del proceso que quiere finalizar\n");
            // scanf(&pid_deseado);
            // FINALIZAR_PROCESO(pid_deseado);
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
            printf("Ingrese grado de multiprogramacion a cambiar %d\n", valor);
            scanf("%d",&valor);
            MULTIPROGRAMACION(valor);
            break;
        case 8:
            printf("Finalizacion del modulo\n");
            exit(1);
            break;
        default:
            break;
        }
    }

    free(path_ejecutable);
    return NULL;
}


void EJECUTAR_SCRIPT(char* path) {
    // Abrir el archivo en modo de lectura (r)
    size_t length = 0;
    char* path_local_kernel = strdup(path_kernel);
    char* path_completo = malloc(strlen(path_local_kernel) + strlen(path));
    path_completo = strcat(path_local_kernel, path);
    FILE* file = fopen(path_completo, "r");
    // char* linea_leida = NULL;
    // Verificar si el archivo se abrió correctamente
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
    }

    aplicar_sobre_cada_linea_del_archivo(file, NULL, (void*) _ejecutarComando);
    
    // Cerrar el archivo
    fclose(file);
}
/*
void EJECUTAR_SCRIPT(char* path) {
    // Abrir el archivo en modo de lectura (r)
    size_t length = 0;
    char* path_local_kernel = strdup(path_kernel);
    char* path_completo = malloc(strlen(path_local_kernel) + strlen(path));
    path_completo = strcat(path_local_kernel, path);
    FILE* file = fopen(path_completo, "r");
    char* linea_leida = NULL;
    // Verificar si el archivo se abrió correctamente
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
    }
    while (getline(&linea_leida, &length, file) != -1) {
    	if(strcmp(linea_leida,"\n")){
        	ejecutarComando(linea_leida);
    	}
    }
    // Cerrar el archivo
    fclose(file);
}
*/

void ejecutarComando(char* linea_leida) {
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

void INICIAR_PROCESO(char* path_instrucciones) {
    int pid_actual = obtener_siguiente_pid();
    printf("El pid del proceso es: %d\n", pid_actual);
    // Primero envio el path de instruccion a memoria y luego el PCB...
    enviar_path_a_memoria(path_instrucciones, sockets, pid_actual); 

    sleep(5);

    t_pcb *pcb = crear_nuevo_pcb(pid_actual); 
    //   list_add(lista_procesos,pcb);
    encolar_a_new(pcb);
}

void* enviar_path_a_memoria(char* path_instrucciones, t_sockets* sockets, int pid) {
    // Mando PID para saber donde asociar las instrucciones
    t_path* pathNuevo = malloc(sizeof(t_path));

    pathNuevo->path = strdup(path_instrucciones); 
    pathNuevo->path_length = strlen(pathNuevo->path) + 1;
    pathNuevo->PID = pid;

    t_buffer* buffer = llenar_buffer_path(pathNuevo);

    enviar_paquete(buffer, PATH, sockets->socket_memoria);
    // log_info(logger_kernel, "Se armo un paquete con este PATH: %s\n", pathNuevo->path);
    
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

int obtener_siguiente_pid() {
    contador_pid++;
    return contador_pid;
}

void encolar_a_new(t_pcb *pcb) {
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
        
        // sem_post(&sem_hay_pcb_esperando_ready); 
    }
    
}
// void liberar_memoria(int pid) {
    //Le mandaria a memoria que debe eliminar el pcb y toda la cosa
// }

void *planificar_corto_plazo(void *sockets_necesarios) {
    pthread_t hilo_quantum;

    t_sockets *sockets = (t_sockets *)sockets_necesarios;
    t_pcb *pcb;

    int socket_CPU = sockets->socket_cpu;
    int socket_INT = sockets->socket_int;

    while (1) {
        printf("Esperando a que haya un proceso para planificar\n");
        sem_wait(&sem_hay_para_planificar);
        printf("Hay un proceso para planificar\n");

        pthread_mutex_lock(&no_hay_nadie_en_cpu);
        pcb = proximo_a_ejecutar();
        pasar_a_exec(pcb);
        pthread_mutex_unlock(&no_hay_nadie_en_cpu);

        if (es_RR()) {
            pthread_create(&hilo_quantum, NULL, (void *)esperar_RR, (void *)pcb);
            sem_post(&sem_contador_quantum);
        } else if (es_VRR()) {
            pthread_create(&hilo_quantum, NULL, (void *)esperar_VRR, (void *)pcb);
            sem_post(&sem_contador_quantum);
        }
        //while(timer <= pcb->quantum){
        esperar_cpu();
        //}
        if (es_VRR_RR()){
            pthread_cancel(hilo_quantum);
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

void *esperar_VRR(t_pcb* pcb) {
    timer = temporal_create(); // Crearlo ya empieza a contar
    esperar_RR(pcb);
    
    /*int socket_Int = (intptr_t)socket_Int;
    sem_wait(&sem_contador_quantum);*/

    // posible send de interrupcion
    return NULL;
}

/*
    int64_t temporal_gettime(t_temporal* temporal);
    void temporal_stop(t_temporal* temporal);
    void temporal_resume(t_temporal* temporal);
*/

void *esperar_RR(t_pcb* pcb_anterior) {
    /* Esperar_cpu_RR */
    t_pcb* pcb = pcb_anterior;
    int quantum = pcb->quantum;
    int socket_Int = sockets->socket_int;
    sem_wait(&sem_contador_quantum);
    usleep(quantum * 1000);
    int interrupcion_cpu = INTERRUPCION_CPU;
    send(socket_Int, &interrupcion_cpu, sizeof(int), 0);
    printf("Envie interrupcion despues de %d\n", quantum);

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

    pthread_mutex_lock(&mutex_estado_ready);
    if (!queue_is_empty(cola_prioritarios_por_signal)) {
        pcb = queue_pop(cola_prioritarios_por_signal);
    } else if (!queue_is_empty(cola_ready_plus)){
        pcb = queue_pop(cola_ready_plus);
    }
    else if (!queue_is_empty(cola_ready)) {
        pcb = queue_pop(cola_ready);
    } else {
        log_info(logger_kernel,"No hay procesos para ejecutar.");
    }
    pthread_mutex_unlock(&mutex_estado_ready);

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

void mandar_datos_io_stdin(char* interfaz_nombre, uint32_t registro_direccion, uint32_t registro_tamanio) {
    t_buffer* buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(uint32_t) + sizeof(int);

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void* stream = buffer->stream;

    memcpy(stream + buffer->offset, &registro_direccion, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &registro_tamanio, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    buffer->stream = stream;

    /* 
    int socket_io = 0;
    if(dictionary_has_key(diccionario_io, interfaz_nombre)) {
        t_list_io* interfaz = malloc(sizeof(t_list_io));
        interfaz = dictionary_get(diccionario_io, interfaz_nombre);
        socket_io = interfaz->socket;
        free(interfaz);
    } else {
        printf("Error de diccionario io\n");
        exit(-1);
    }

    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = LEETE; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(int) * 2);
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(codigo_operacion));

    offset += sizeof(codigo_operacion);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_io, a_enviar, buffer->size + sizeof(int) * 2, 0);
    */
    // No nos olvidamos de liberar la memoria que ya no usaremos
    /*free(a_enviar);
    liberar_paquete(paquete);*/
} 

void esperar_cpu() { // Evaluar la idea de que esto sea otro hilo...
    DesalojoCpu devolucion_cpu;
    
    t_paquete* package = recibir_cpu(); // pcb y codigo de operacion (devolucion_cpu)
    devolucion_cpu = package->codigo_operacion;
    // Deserializo antes el pcb, me ahorro cierta logica y puedo hacer send si es IO_BLOCKED
    t_pcb* pcb = deserializar_pcb(package->buffer);
    imprimir_pcb(pcb);
    printf("pid del pcb: %d\n", pcb->pid);

    if (es_VRR()) {
        temporal_stop(timer); // Detenemos el temporizador

        /*if(!hay_interrupcion){

        }*/

        int64_t tiempo_transcurrido = temporal_gettime(timer);  // Obtenemos el tiempo transcurrido en milisegundos
        temporal_destroy(timer);
        printf("Tiempo transcurrido: %ld ms\n", tiempo_transcurrido);
      
        int64_t tiempo_restante = max(0,min(quantum_config,quantum_config - tiempo_transcurrido));  // Fede scarpa orgulloso
        printf("Tiempo restante: %ld ms\n", tiempo_restante);

        // Ajustamos el quantum, asegurándonos de que no sea menor a 0
        pcb->quantum = tiempo_restante;
        log_info(logger_kernel, "El quantum restante del proceso es: %d", pcb->quantum);
    }

    switch (devolucion_cpu) {
        case ERROR_STDOUT || ERROR_STDIN:
            pasar_a_exit(pcb);
            // free(pcb);
            break;
        case INTERRUPCION_QUANTUM:
            printf("Volvio a ready por interrupción de quantum\n");
            log_info(logger_kernel, "PID: %d - Desalojado por fin de Quantum", pcb->pid);
            pasar_a_ready(pcb);
            /*
            if (es_VRR() && leQuedaTiempoDeQuantum(pcb)) {
                pasar_a_ready_plus(pcb);
            } else {
                if(es_VRR()) {
                    volver_a_settear_quantum(pcb);
                }
                pasar_a_ready(pcb);
            }
            */
            break;
        case DORMIR_INTERFAZ:
            t_operacion_io* operacion_io = deserializar_io(package->buffer);
            dormir_io(operacion_io, pcb);
            // dormir_io(operacion_io);
            log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->pid, operacion_io->nombre_interfaz);  
            free(operacion_io);
            // free(pcb);
            break;
        case WAIT_RECURSO: case SIGNAL_RECURSO:
            t_parametro* recurso = deserializar_parametro(package->buffer);
            printf("el nombre del recurso es %s\n", recurso->nombre);  
            wait_signal_recurso(pcb, recurso->nombre, devolucion_cpu);
            
            // free(recurso->nombre);
            // free(recurso);
            break;
        case FIN_PROCESO:
            // pcb = deserializar_pcb(package->buffer); 
            log_info(logger_kernel, "Finaliza el proceso %d - Motivo: SUCCESS", pcb->pid);
            pasar_a_exit(pcb);
            //liberar_memoria(pcb->pid); // Por ahora esto seria simplemente decirle a memoria que elimine el PID del diccionario
            //change_status(pcb, EXIT); 
            //sem_post(&sem_grado_multiprogramacion); // Esto deberia liberar un grado de memoria para que acceda un proceso
            // free(pcb);
            break;
        case PEDIDO_LECTURA:

            /// HAY QUE ACTUALIZAR NO EXISTE ESO ES IO_STD!!!!!!!!
            t_pedido* pedido_lectura = deserializar_pedido(package->buffer);

            // mandar_io_blocked(pcb, pedido_lectura->interfaz);
            // mandar_datos_io_stdin(pcb, pedido_lectura->interfaz, pedido_lectura->direccion_fisica, pedido_lectura->registro_tamanio);
            
            encolar_datos_std(pcb, pedido_lectura);
            log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->pid, pedido_lectura->interfaz);   
            //free(pedido_lectura);
            break;
        case PEDIDO_ESCRITURA:
            printf("LLEGO A PEDIDO ESCRITURAAAAAAAAAAAAAAAAAA");
            t_pedido* pedido_escritura = deserializar_pedido(package->buffer); // llega bien el nombre de la interfaz
            printf("NOMBRE PEDIDO ESCRITURA: %s", pedido_escritura->interfaz);
            encolar_datos_std(pcb, pedido_escritura);
            log_info(logger_kernel,"PID: %d - Bloqueado por - %s", pcb->pid, pedido_escritura->interfaz);
            break;
        /*
        case FS_CREATE:
        case FS_DELETE:
            t_pedido_fs_create_delete* pedido_fs = deserializar_pedido_fs_create_delete(package->buffer);
            t_list_io* interfaz_crear_destruir = io_esta_en_diccionario(pcb, pedido_fs->nombre_interfaz);
            
            if (interfaz_crear_destruir != NULL) {
                // codigo_operacion operacion = (package->codigo_operacion == FS_CREATE) ? CREAR_ARCHIVO : ELIMINAR_ARCHIVO;
                codigo_operacion operacion = (devolucion_cpu == FS_CREATE) ? CREAR_ARCHIVO : ELIMINAR_ARCHIVO;

                // Esto se hace en la conexion, aca tiene que encolar el pedido
                enviar_buffer_fs(interfaz_crear_destruir->socket, pcb->pid, pedido_fs->longitud_nombre_archivo, pedido_fs->nombre_archivo, operacion);
                
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, pedido_fs->nombre_interfaz);
            }
            
            free(pedido_fs);
            break;
        case ESCRITURA_FS: case LECTURA_FS:
            t_pedido_fs_escritura_lectura* fs_read_write = deserializar_pedido_fs_escritura_lectura(package->buffer);
            t_list_io* interfaz = io_esta_en_diccionario(pcb,fs_read_write->nombre_interfaz);

            if (interfaz != NULL) {
                // codigo_operacion operacion = (package->codigo_operacion == ESCRITURA_FS) ? ESCRIBIR_FS_MEMORIA : LEER_FS_MEMORIA;
                codigo_operacion operacion = (devolucion_cpu == ESCRITURA_FS) ? ESCRIBIR_FS_MEMORIA : LEER_FS_MEMORIA;
                // Esto se hace en la conexion, aca tiene que encolar el pedido
                enviar_buffer_fs_escritura_lectura(interfaz->socket,pcb->pid,fs_read_write->largo_archivo,
                                                    fs_read_write->nombre_archivo,fs_read_write->registro_direccion,fs_read_write->registro_tamanio,fs_read_write->registro_archivo,operacion);
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, fs_read_write->nombre_interfaz);
            }
        case FS_TRUNCATE_KERNEL:
            t_pedido_fs_truncate* fs_truncate = deserializar_fs_truncate(package->buffer);
            t_list_io* interfaz_truncate = io_esta_en_diccionario(pcb,fs_truncate->nombre_interfaz);

            if (interfaz != NULL) {
                t_buffer* buffer_truncate = llenar_buffer_fs_truncate(pcb->pid,fs_truncate->largo_archivo,fs_truncate->nombre_archivo,fs_truncate->truncador);
                enviar_paquete(buffer_truncate,TRUNCAR_ARCHIVO,interfaz->socket);
                log_info(logger_kernel, "PID: %d - Bloqueado por - %s", pcb->pid, fs_read_write->nombre_interfaz);
            }
        */
        default:
            printf("Llego un codigo de operacion inexistente o rompio algo\n");
            exit(-1);
            break;
    }
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
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", pcb->pid);
        pasar_a_exit(pcb);
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
        
        printf("\nAca va la lista de bytes DEL STDOUT ANTES DE ENCOLAR -> ARA GIL ME DEBES UN ALFAJOR:\n");
        for(int i=0; i < list_size(datos_std->lista_direcciones); i++) {
            t_dir_fisica_tamanio* dir = list_get(datos_std->lista_direcciones, i);
            printf("Direccion fisica: %d\n", dir->direccion_fisica);
            printf("Bytes a leer: %d\n", dir->bytes_lectura);
        }

        printf("datos_std->registro_tamanio = %d\n", datos_std->registro_tamanio);
        printf("datos_std->cantidad_paginas = %d\n", datos_std->cantidad_paginas);
        printf("datos_std->pcb->pid = %d\n", datos_std->pcb->pid);
        
        // imprimir_datos_stdin();

        pthread_mutex_lock(&mutex_cola_io_generica); // cambiar nombre_mutex
        printf("entra aca\n");
        queue_push(interfaz->cola_blocked, datos_std);
        pthread_mutex_unlock(&mutex_cola_io_generica);

        // use lo que estaba, antes decia sem_post al mutex, por eso, era un binario o eso entendi
        pthread_mutex_lock(&mutex_lista_io);
        sem_post(interfaz->semaforo_cola_procesos_blocked);
        printf("La operacion deberia realizarse...\n");
        pthread_mutex_unlock(&mutex_lista_io);
    }
    // Habria que liberar toda la lista que se cargo adentro de pedido
    free(pedido);
    // free(interfaz); NECESITAMOS LA INTERFAZ A POSTERIORI
} 

/* 
t_operacion_io* operacion_io = malloc(sizeof(t_operacion_io));

    // Lo siguiente es el tamaño del pcb
    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6;

    // Deserializamos los campos que tenemos en el buffer
    // stream += sizeof(t_pcb);
    
    memcpy(&(operacion_io->unidadesDeTrabajo), stream, sizeof(int));
    stream += sizeof(int);
*/
t_pedido* deserializar_pedido(t_buffer* buffer) {
    t_pedido* pedido = malloc(sizeof(t_pedido));

    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6; 

    memcpy(&(pedido->cantidad_paginas), stream, sizeof(int));
    stream += sizeof(int);
    printf("Cantidad de paginas: %d\n", pedido->cantidad_paginas);

    pedido->lista_dir_tamanio = list_create();
    // Deserializar lista con dir fisica y tamanio en bytes a leer segun la cant de pags
    for(int i = 0; i < pedido->cantidad_paginas; i++) {
        t_dir_fisica_tamanio* dir_fisica_tam = malloc(sizeof(t_dir_fisica_tamanio));
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

/*
t_pedido_lectura* deserializar_pedido_lectura(t_buffer* buffer) {
    t_pedido_lectura* pedido_lectura = malloc(sizeof(t_pedido_lectura));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&pedido_lectura->registro_direccion, stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pedido_lectura->registro_tamanio), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(pedido_lectura->cantidad_paginas), stream, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(&(pedido_lectura->pagina),stream, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(&(pedido_lectura->length_interfaz), stream, sizeof(int));
    stream += sizeof(int);

    pedido_lectura->interfaz = malloc(pedido_lectura->length_interfaz);
    memcpy(pedido_lectura->interfaz, stream, pedido_lectura->length_interfaz);

    return pedido_lectura;
}
*/

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

/* 
void dormir_io(t_operacion_io* operacion_io) {
    pthread_t dormir_interfaz;
    pthread_create(&dormir_interfaz, NULL, (void*) hilo_dormir_io, operacion_io); 
}
*/

void dormir_io(t_operacion_io* io, t_pcb* pcb) {

    io_gen_sleep* datos_sleep = malloc(sizeof(io_gen_sleep));
    datos_sleep->pcb = malloc(sizeof(t_pcb)); // free cuando se desconecte

    // bool existe_io = dictionary_has_key(diccionario_io, operacion_io->interfaz);
    t_list_io* elemento_encontrado = io_esta_en_diccionario(pcb, io->nombre_interfaz);

    // t_list_io* elemento_encontrado = validar_io(io, pcb); 
    // Aca tambien deberia cambiar su estado a blocked o Exit respectivamnete

    if(elemento_encontrado == NULL) {
        // Aca estaria tamb la logica de matar al hilo
        printf("Es NULL LA INTERFAZ...\n");
        return;
    }

    printf("El nombre de la interfaz es: %s\n", elemento_encontrado->nombreInterfaz);
    
    if(1) { // resultado_okey
        datos_sleep->unidad_trabajo = io->unidadesDeTrabajo;
        datos_sleep->pcb = pcb;
        printf("El pid de datos_sleep de dormir_io: %d\n", pcb->pid);

        pthread_mutex_lock(&mutex_cola_io_generica);
        queue_push(elemento_encontrado->cola_blocked, datos_sleep);
        pthread_mutex_unlock(&mutex_cola_io_generica);

        pasar_a_blocked(pcb);

        // use lo que estaba, antes decia sem_post al mutex, por eso, era un binario o eso entendi
        pthread_mutex_lock(&mutex_lista_io);
        sem_post(elemento_encontrado->semaforo_cola_procesos_blocked);
        printf("La operacion deberia realizarse...\n");
        pthread_mutex_unlock(&mutex_lista_io);
    }
} 

/*  Al recibir una petición de I/O de parte de la CPU primero se deberá validar que exista y esté
conectada la interfaz solicitada, en caso contrario, se deberá enviar el proceso a EXIT.
En caso de que la interfaz exista y esté conectada, se deberá validar que la interfaz admite la
operación solicitada, en caso de que no sea así, se deberá enviar el proceso a EXIT. */

t_list_io* validar_io(t_operacion_io* io, t_pcb* pcb) {
    /*bool match_nombre(void* nodo_lista) {
        return strcmp(((t_list_io*)nodo_lista)->nombreInterfaz, io->nombre_interfaz) == 0;
    };*/

    printf("Valida la io\n");
    
    pthread_mutex_lock(&mutex_lista_io);
    // t_list_io* elemento_encontrado = list_find(lista_io, match_nombre); // Aca deberia buscar la interfaz en la lista de io
    t_list_io* elemento_encontrado = dictionary_get(diccionario_io, io->nombre_interfaz);

    if(elemento_encontrado == NULL || elemento_encontrado->TipoInterfaz != GENERICA) { // Deberia ser global
        // pasar_a_exit(pcb);
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

// Aca se desarma el path y se obtienen las instrucciones y se le pasan a memoria para que esta lo guarde en su tabla de paginas de este proceso
t_pcb *crear_nuevo_pcb(int pid){
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

/* 
void* recibir_peticion_de_cpu() {
    // recibiremos la instruccion
}

typedef struct {    
    char* instruccion; 
    int instruccion_length;  
} t_peticion_io;


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

/* 
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
*/

t_parametro* deserializar_parametro(t_buffer* buffer) {
    /*int largo_parametro;
    
    t_parametro* parametro = malloc(sizeof(t_parametro)); // sizeof(t_parametro)

    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6;

    if (stream >= buffer->stream + buffer->size) {
        printf("No hay nada en el buffer\n");
        return NULL;
    }

    memcpy(&largo_parametro, stream, sizeof(int));
    stream += sizeof(int);

    parametro->length = largo_parametro;
    parametro->nombre = malloc(parametro->length);

    memcpy(parametro->nombre, stream, parametro->length);

    parametro->nombre[parametro->length] = '\0';
    
    printf("el nombre del parametro es %s\n", parametro->nombre);

    return parametro; // free(parametro) y free(parametro->nombre)*/
    int largo_parametro;
    
    t_parametro* parametro = malloc(sizeof(t_parametro)); // sizeof(t_parametro)

    void* stream = buffer->stream + sizeof(int) * 3 + sizeof(Estado) * 2 + sizeof(uint8_t) * 4 + sizeof(uint32_t) * 6;

    if (stream >= buffer->stream + buffer->size) {
        printf("No hay nada en el buffer\n");
        return NULL;
    }

    memcpy(&largo_parametro, stream, sizeof(int));
    stream += sizeof(int);

    parametro->length = largo_parametro;
    parametro->nombre = malloc(parametro->length);

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
            ejecutar_wait_recurso(recurso_obtenido,pcb,key_nombre_recurso);
        }else{
            printf("Llega hasta signal\n");
            ejecutar_signal_recurso(recurso_obtenido,pcb);
        }
    } else {
        log_info(logger_kernel,"Finaliza el proceso %d - Motivo: INVALID_RESOURCE", pcb->pid);
        pasar_a_exit(pcb);
    }
    
    imprimir_diccionario_recursos();
}

void ejecutar_wait_recurso(t_recurso* recurso_obtenido,t_pcb* pcb,char* recurso){
    if(recurso_obtenido->instancias > 0) {
        recurso_obtenido->instancias --;
        // pasar_a_ready(pcb);
        queue_push(cola_prioritarios_por_signal, pcb);
        sem_post(&sem_hay_para_planificar);
    } else {
        pasar_a_blocked(pcb);
        queue_push(recurso_obtenido->procesos_bloqueados, pcb);
        log_info(logger_kernel,"PID: %d - Bloqueado por: %s", pcb->pid,recurso);
    }
}

void ejecutar_signal_recurso(t_recurso* recurso_obtenido, t_pcb* pcb) {
    recurso_obtenido->instancias ++;
    
    //mutex
    /*pthread_mutex_lock(&no_hay_nadie_en_cpu);
    pasar_a_exec(pcb);
    pthread_mutex_unlock(&no_hay_nadie_en_cpu);*/
    
    // change_status(pcb, READY);
    
    queue_push(cola_prioritarios_por_signal, pcb);
    sem_post(&sem_hay_para_planificar);
    
    if(!queue_is_empty(recurso_obtenido->procesos_bloqueados)) {
        t_pcb* proceso_liberado = queue_pop(recurso_obtenido->procesos_bloqueados);
        /*change_status(proceso_liberado, READY);
        queue_push(cola_prioritarios_por_signal, proceso_liberado); 
        sem_post(&sem_hay_para_planificar);*/
        pasar_a_ready(proceso_liberado);
    }
    
    /*
    change_status(pcb, READY);
    queue_push(cola_prioritarios_por_signal, pcb); 
    sem_post(&sem_hay_para_planificar);
    
    if(!queue_is_empty(recurso_obtenido->procesos_bloqueados)) {
        t_pcb* proceso_liberado = queue_pop(recurso_obtenido->procesos_bloqueados);
        pasar_a_ready(proceso_liberado);
        // queue_push(cola_prioritarios_por_signal, proceso_liberado); // corroborar
    }
    */
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
    printf("El recurso %s tiene %d instancias y %d procesos bloqueados.\n", nombre, recurso->instancias, queue_size(recurso->procesos_bloqueados));
}

void imprimir_diccionario_recursos() {
    printf("\n");
    dictionary_iterator(datos_kernel->diccionario_recursos, (void*)_imprimir_recurso);
}

void FINALIZAR_PROCESO(int pid) {
    /*
    t_pcb* pcb = list_find(lista_procesos, (proceso\->proceso->pid == pid));
    t_queue* cola = encontrar_en_que_cola_esta(pcb);
    sacarDe(cola, pcb);
    pasar_a_exit(pcb);
    */
    // t_pcb* pcb = sacarPCBDeDondeEste(int pid);
    // pasar_a_exit(pcb);
}

/* 
t_pcb* sacarPCBDeDondeEste(int pid) {
    t_pcb* pcb;

    if(pid == pcb_exec->pid) {
        send(socket_Int, &INTERRUPCION_FIN_PROCESO, sizeof(int), 0);
        // TO DO: Diferenciar las distintas interrupciones, por ahora solo recibe una.
        // Recibir pcb junto con motivo de desalojo
        recv(socket_int, &pcb, sizeof(t_pcb), MSG_WAITALL);
    } else {
        t_queue* cola = encontrar_en_que_cola_esta(pid);
        pcb = sacarDe(cola, pid);
    }

    return pcb;
}

t_queue* encontrar_en_que_cola_esta(int pid) {
    if(queue_find(cola_new, pid)) {
        return cola_new;
    } else if(queue_find(cola_ready, pid)) {
        return cola_ready;
    } else if(queue_find(cola_blocked), pid) {
        //TO DO: Buscar en las colas_blocked de io
        //Estaba todo borrado cuando llegue
        return cola_blocked;
    }
}
*/

/* 
int queue_find(t_queue* cola, int pid) {
    t_queue* colaAux = queue_create();
    int esta_el_pid = 0;
    while(!queue_is_empty(cola)){
        t_pcb* pcbAux = queue_pop(cola);
        if(pcbAux->pid != pid){
            queue_push(colaAux,pcbAux); 
        } else {
            esta_el_pid = 1;
        }
    }
    cola = colaAux; 

    return esta_el_pid;
}
*/

/* 
t_pcb* sacarDe(t_queue* cola, int pid){
    t_pcb* pcb;
    t_queue* colaAux = queue_create();
    while(!queue_is_empty(cola)){
        t_pcb* pcbAux = queue_pop(cola);
        if(pcbAux->pid != pid){
            queue_push(colaAux,pcbAux); // POSIBLE MUTEX
        } else {
            pcb = pcbAux;
        }
    }
    cola = colaAux;   
    return pcb;
}
*/

void INICIAR_PLANIFICACION() {

}

void DETENER_PLANIFICACION() {
}


void MULTIPROGRAMACION(int valor) {
    /* 
    
    int nuevo_grado_multiprogramacion;
    printf("Ingrese el nuevo grado de multiprogramacion\n");
    scanf("%d",&nuevo_grado_multiprogramacion);
    grado_multiprogramacion = nuevo_grado_multiprogramacion;
    // config_set_value(t_config*, char *key, char *value);
    // tambien puede ser config_set_value(config_kernel,"GRADO_MULTIPROGRAMACION",nuevo_grado_multiprogramacion);
    
    */
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
    
    /*printf("--------Listado de procesos--------\n");
    
    printf("Procesos en new\n");
    pthread_mutex_lock(&mutex_estado_new);
    mostrar_cola(cola_new);
    pthread_mutex_lock(&mutex_estado_new);
    
    printf("Procesos en ready\n");
    pthread_mutex_lock(&mutex_estado_ready);
    mostrar_cola(cola_ready);
    pthread_mutex_lock(&mutex_estado_ready);

    printf("Procesos en ready plus\n");
    pthread_mutex_lock(&mutex_estado_ready_plus);
    mostrar_cola(cola_ready_plus);
    pthread_mutex_lock(&mutex_estado_ready_plus );

    printf("Procesos en blocked\n");
    pthread_mutex_lock(&mutex_estado_blocked);
    mostrar_cola(cola_blocked);
    pthread_mutex_lock(&mutex_estado_blocked);

    printf("Procesos en exit\n");
    pthread_mutex_lock(&mutex_estado_exit);
    mostrar_cola(cola_exit);
    pthread_mutex_lock(&mutex_estado_exit);  

    printf("Proceso en exec\n");
    imprimir_pcb(pcb_exec);
    */
}

void mostrar_cola(t_queue* cola) {
    t_pcb *aux;
    t_queue *cola_aux = queue_create(); 

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
/*

void manejar_interrupcion(void *pcbElegida)
{
	log_info(logger, "Entrando a manejar interrupcion\n");
	t_tipo_algoritmo algoritmo = obtenerAlgoritmo();
	log_info(logger, "%d\n", algoritmo);
	t_pcb *pcb = (t_pcb *)pcbElegida;
	if (algoritmo == FEEDBACK)
	{
		log_info(logger, "pasar a ready aux");
		pasar_a_ready_auxiliar(pcb);
		sem_post(&sem_hay_pcb_lista_ready);
	}
	else if (algoritmo == RR)
	{
		log_info(logger, "El algoritmo obtenido es: %d\n", obtenerAlgoritmo());
		log_info(logger, "cantidad de elementos en lista exec: %d\n", list_size(LISTA_EXEC));

		pasar_a_ready(pcb);
		log_info(logger, "cantidad de elementos en ready: %d\n", list_size(LISTA_READY));
		sem_post(&sem_hay_pcb_lista_ready);
		log_info(logger, "cantidad de elementos en ready: %d\n", list_size(LISTA_READY));
	}
	log_info(logger, "termine de manejar la interrupcion");
}


void planifCortoPlazo()
{
	while (1)
	{
		sem_wait(&sem_hay_pcb_lista_ready);
		log_info(logger, "Llego pcb a plani corto plazo");
		t_tipo_algoritmo algoritmo = obtenerAlgoritmo();

		sem_wait(&contador_pcb_running);

		switch (algoritmo)
		{
		case FIFO:
			log_debug(logger, "Implementando algoritmo FIFO");
			log_debug(logger, " Cola Ready FIFO:");
			cargarListaReadyIdPCB(LISTA_READY);
			implementar_fifo();

			break;
		case RR:
			log_debug(logger, "Implementando algoritmo RR");
			log_debug(logger, " Cola Ready RR:");
			cargarListaReadyIdPCB(LISTA_READY);
			implementar_rr();

			break;
		case FEEDBACK:
			log_debug(logger, "Implementando algoritmo FEEDBACK");
			implementar_feedback();

			break;

		default:
			break;
		}
	}
}

void implementar_rr()
{
	t_pcb *pcb = algoritmo_fifo(LISTA_READY);

	pthread_t thrTimer;

	int hiloTimerCreado = pthread_create(&thrTimer, NULL, (void *)hilo_timer, NULL);

	int detach = pthread_detach(thr/ /script_solo_cpu_1
        // /script_io_basico_1
        // /script_solo_cpu_3
        // /script_1Timer);
	log_info(logger, "se creo el hilo timer correctamente?: %d, %d\n ", hiloTimerCreado, detach);
	hayTimer = true;
	log_info(logger, "Agregando UN pcb a lista exec rr");
	pasar_a_exec(pcb);
	log_info(logger, "Cant de eleme/ /script_solo_cpu_1
        // /script_io_basico_1
        // /script_solo_cpu_3
        // /script_1ntos de exec: %d\n", list_size(LISTA_EXEC));

	log_debug(logger, "Estado Anterior: READY , proceso id: %d", pcb->id);
	log_debug(logger, "Estado Actual: EXEC , proceso id: %d", pcb->id);

	// Cambio de estado
	log_info(loggerMinimo, "Cambio de Estado: PID %d - Estado Anterior: READY , Estado Actual: EXEC", pcb->id);

	sem_post(&sem_timer);
	sem_post(&sem_pasar_pcb_running);

	log_info(logger, "Esperando matar el timer\n");
	sem_wait(&sem_kill_trhread);

	// pthread_cancel(thrTimer);

	if (pthread_cancel(thrTimer) == 0)
	{
		log_info(logger, "Hilo cancelado con exito");
	}
	else
	{
		log_info(logger, "No mate el hilo");
	}
	log_warning(logger, "saliendo de RR\n");
}

void hilo_timer()
{
	sem_wait(&sem_timer);
	log_info(logger, "voy a dormir, soy el timer\n");
	usleep(configKernel.quantum * 1000);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	log_info(logger, "me desperte!\/ /script_solo_cpu_1
        // /script_io_basico_1
        // /script_solo_cpu_3
        // /script_1n");
	sem_post(&sem_desalojar_pcb);

	log_info(logger, "envie post desalojar pcb\n");
}/ /script_solo_cpu_1
        // /script_io_basico_1
        // /script_solo_cpu_3
        // /script_1

t_pcb *algoritmo_fifo(t_list *lista)
{
	t_pcb *pcb = (t_pcb *)list_remove(lista, 0);
	return pcb;
}
*/

void mandar_a_escribir_a_memoria(char* nombre_interfaz, int direccion_fisica, uint32_t tamanio){
    t_buffer* buffer = llenar_buffer_stdout(direccion_fisica, nombre_interfaz, tamanio);
    int socket_memoria = sockets->socket_memoria;
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
