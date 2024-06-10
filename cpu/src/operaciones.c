#include "../include/operaciones.h"

t_log *logger;
int hay_interrupcion;
int tamanio_pagina;

// t_cantidad_instrucciones* cantidad_instrucciones;

void ejecutar_pcb(t_pcb *pcb, int socket_memoria) {
    pedir_cantidad_instrucciones_a_memoria(pcb->pid, socket_memoria);
    int cantidad_instrucciones = recibir_cantidad_instrucciones(socket_memoria, pcb->pid);

    while(pcb->program_counter < cantidad_instrucciones) {
        // sem_wait(pedir_instruccion);
        pedir_instruccion_a_memoria(socket_memoria, pcb);
        t_instruccion* instruccion = recibir_instruccion(socket_memoria, pcb); // Se ejecuta la instruccion tambien
        // sem_post(pedir_instruccion);
        printf("Esta ejecutando %d\n", pcb->pid);    
        int resultado_ok = ejecutar_instruccion(instruccion, pcb);

        if(resultado_ok == 1) {
            return;
        }

        pcb->program_counter++; // Ojo con esto! porque esta despues del return, osea q en el IO_GEN_SLEEP antes de desalojar hay que aumentar el pc
        // Podria ser una funcion directa -> Por ahora no es esencial
        printf("Registro_actual->AX: %d\n", registros_cpu->AX);
        printf("pcb->AX: %d\n", pcb->registros->AX);
        printf("Registro_actual->BX: %d\n", registros_cpu->BX);
        printf("pcb->BX: %d\n", pcb->registros->BX);
        printf("pcb->CX %d\n", pcb->registros->CX);
        printf("Registro_actual->CX: %d\n", registros_cpu->CX);

        int result_int = check_interrupt(pcb);
        if(result_int == 1) {
            printf("Hubo una interrupcion, y salgo...\n");
            return;
        }

        free(instruccion);
    }

    /*while(pcb->estadoActual < cantidad_instrucciones) {
        pedir_instruccion_a_memoria(socket_memoria, pcb);
        //sem_wait(&sem_memori, pcbtr
        recibir(socket_memoria,pcb); // recibir cada instruccion
        pcb->program_counter++;
    }*/

    // La unica que le encuentro es llevarlo al switch
}

int check_interrupt(t_pcb* pcb) {
    if(hay_interrupcion) {
        // printf("Hubo una interrupcion.\n");
        desalojar(pcb, INTERRUPCION_QUANTUM, NULL);
        hay_interrupcion = 0;
    } 
    return 1;
}

void pedir_cantidad_instrucciones_a_memoria(int pid, int socket_memoria) {
    t_buffer *buffer = malloc(sizeof(t_buffer));
    // t_cantidad_instrucciones* cantidad = malloc(sizeof(t_cantidad_instrucciones));

    buffer->size = sizeof(int);
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    memcpy(stream + buffer->offset, &pid, sizeof(int));

    buffer->stream = stream;

    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = QUIERO_CANTIDAD_INSTRUCCIONES; // Podemos usar una constante por operación
    paquete->buffer = buffer;                                  // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void *a_enviar = malloc(buffer->size + sizeof(int) * 2);
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));

    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // printf("Pedi la cantidad de instrucciones del proceso %d.\n", pid);
    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) * 2, 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

int cantidad_instrucciones_deserializar(t_buffer *buffer) {
    // printf("Deserializa la cantidad de instrucciones.\n");
    int cantidad_instrucciones;

    void *stream = buffer->stream;
    memcpy(&cantidad_instrucciones, stream, sizeof(int));

    return cantidad_instrucciones;
}

// Manda a memoria el pcb, espera una instruccion y la devuelve
void pedir_instruccion_a_memoria(int socket_memoria, t_pcb* pcb) {
    // Recibimos cada parámetro
    t_solicitud_instruccion *pid_pc = malloc(sizeof(t_solicitud_instruccion));
    pid_pc->pid = pcb->pid;
    pid_pc->pc = pcb->program_counter;

    t_buffer *buffer = llenar_buffer_solicitud_instruccion(pid_pc);

    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = QUIERO_INSTRUCCION; // Podemos usar una constante por operación
    paquete->buffer = buffer;                       // Nuestro buffer de antes.

    void *a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); // siceof
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    // printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
    // free(pid_pc); -> ver si va aca
}

t_buffer *llenar_buffer_solicitud_instruccion(t_solicitud_instruccion *solicitud_instruccion) {
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 2;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream + buffer->offset, &solicitud_instruccion->pid, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &solicitud_instruccion->pc, sizeof(int));

    buffer->stream = stream;

    return buffer;
}

t_paquete* recibir_memoria(int socket_memoria) {
    while(1) {
        t_paquete *paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir instruccion...\n");

        recv(socket_memoria, &(paquete->codigo_operacion), sizeof(codigo_operacion), MSG_WAITALL);
        printf("Codigo de operacion: %d\n", paquete->codigo_operacion);

        recv(socket_memoria, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        // printf("paquete->buffer->size: %d\n", paquete->buffer->size);

        paquete->buffer->stream = malloc(paquete->buffer->size);
        // recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);
        recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);
        return paquete;
    }  
}

int recibir_cantidad_instrucciones(int socket_memoria, int pid) {
    t_paquete *paquete = recibir_memoria(socket_memoria);
    int cantidad_instrucciones = 0;

    // Ahora en función del código recibido procedemos a deserializar el resto
    switch (paquete->codigo_operacion) {
        case ENVIO_CANTIDAD_INSTRUCCIONES:
            cantidad_instrucciones = cantidad_instrucciones_deserializar(paquete->buffer);
            printf("Recibi que la cantidad de instrucciones del proceso %d es %d.\n", pid, cantidad_instrucciones);
            break;
        default:
            printf("Error: Fallo!\n");
            break;
    }

    free(paquete->buffer->stream);
    free(paquete->buffer);  
    free(paquete);

    return cantidad_instrucciones;
}

t_instruccion* recibir_instruccion(int socket_memoria, t_pcb *pcb) {
    t_paquete* paquete = recibir_memoria(socket_memoria);
    t_instruccion *instruccion;

    // Ahora en función del código recibido procedemos a deserializar el resto
    switch (paquete->codigo_operacion) {
        case ENVIO_INSTRUCCION:
            instruccion = instruccion_deserializar(paquete->buffer);

            /* 
            int resultado_exec = ejecutar_instruccion(instruccion, pcb); //

                if(resultado_exec == 1) {
                    printf("Salgo porque el proceso se va a BLOCKED");
                    desalojado = 1;
                    break;
                }

                if (hay_interrupcion) {
                    printf("Hubo una interrupcion.\n");
                    guardar_estado(pcb);
                    // enviar_pcb(pcb, client_dispatch, INTERRUPCION_QUANTUM, NULL, 0); // Podria usar desalojar?
                    hay_interrupcion = 0;
                    break;
                }

                free(instruccion);
                */
                break;
            default:
                printf("Error: Fallo!\n");
                break;
        }
        
    // Liberamos memoria
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);

    return instruccion; // Podria ser NULL si recibe mal el codop
}

t_instruccion *instruccion_deserializar(t_buffer *buffer) {
    t_instruccion *instruccion = malloc(sizeof(t_instruccion));
    instruccion->parametros = list_create();

    void *stream = buffer->stream;

    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(instruccion->nombre), stream, sizeof(TipoInstruccion));
    stream += sizeof(TipoInstruccion);
    memcpy(&(instruccion->cantidad_parametros), stream, sizeof(int));
    stream += sizeof(int);

    // printf("Recibimos la instruccion de enum %d con %d parametros.\n", instruccion->nombre, instruccion->cantidad_parametros);

    if (instruccion->cantidad_parametros == 0)
    {
        return instruccion;
    }

    for (int i = 0; i < instruccion->cantidad_parametros; i++)
    {
        t_parametro *parametro = malloc(sizeof(t_parametro));
        memcpy(&(parametro->length), stream, sizeof(int));
        // printf("Este es el largo del parametro %d que llego: %d\n", i, parametro->length);
        stream += sizeof(int);

        parametro->nombre = malloc(sizeof(char) * parametro->length + 1);
        memcpy(parametro->nombre, stream, sizeof(char) * parametro->length);

        parametro->nombre[parametro->length] = '\0';
        stream += parametro->length;
        // printf("Parametro: %s\n", parametro->nombre);

        list_add(instruccion->parametros, parametro);
        // free(parametro->nombre);
    }

    return instruccion;
}

bool es_de_8_bits(char *registro) {
    return (strcmp(registro, "AX") == 0 || strcmp(registro, "BX") == 0 ||
            strcmp(registro, "CX") == 0 || strcmp(registro, "DX") == 0);
}

    // se podrian env en handshake

int ejecutar_instruccion(t_instruccion *instruccion, t_pcb *pcb) {

    // log_info(logger, "PID: %d - Ejecutando: %d ", pcb->pid, instruccion->nombre);

    TipoInstruccion nombreInstruccion = instruccion->nombre;
    t_list *list_parametros = instruccion->parametros;

    // printf("La instruccion es de numero %d y tiene %d parametros\n", instruccion->nombre, instruccion->cantidad_parametros);

    char* registro1;

    switch (nombreInstruccion) // Ver la repeticion de logica... -> Abstraer
    {
    case SET:
        // Obtengo los datos de la lista de parametros
        t_parametro *registro_param = list_get(list_parametros, 0);
        registro1 = registro_param->nombre;

        t_parametro *valor_param = list_get(list_parametros, 1);

        char *valor_char = valor_param->nombre;
        int valor = atoi(valor_char);

        // Aca obtengo la direccion de memoria del registro
        void *registro_pcb1 = seleccionar_registro_cpu(registro1);

        // Aca obtengo si es de 8 bits o no y uso eso para setear, sino funciona el bool uso un int
        bool es_registro_uint8 = es_de_8_bits(registro1);
        set(registro_pcb1, valor, es_registro_uint8);

        /*printf("Cuando hace SET AX 1 queda asi el registro AX del CPU: %u\n", registros_cpu->AX);
        printf("Cuando hace SET BX 1 queda asi el registro BX del CPU: %u\n", registros_cpu->BX);*/
        sleep(2);
        break;
    case MOV_IN: // MOV_IN EDX ECX
        t_parametro *registro_datos = list_get(list_parametros, 0);
        t_parametro *registro_direccion = list_get(list_parametros, 1);
        char* nombre_registro_dato = registro_datos->nombre;
        char* nombre_registro_dir = registro_direccion->nombre;

        void* registro_dato_1 = seleccionar_registro_cpu(nombre_registro_dato);
        bool es_registro_uint8_dato = es_de_8_bits(nombre_registro_dato);
        void* registro_direccion_1 = seleccionar_registro_cpu(nombre_registro_dir);
        bool es_registro_uint8_direccion = es_de_8_bits(nombre_registro_dir);

        int direccion_fisica = traducir_direccion_logica_a_fisica(tamanio_pagina, &registro_direccion_1, pcb->pid, es_registro_uint8_direccion); 
        printf("direccion fisica que vamos a usar %d \n", direccion_fisica);

        mandar_direccion_fisica_a_mem(direccion_fisica, es_registro_uint8_direccion);

        recv(socket_memoria, &valor, sizeof(int), MSG_WAITALL);
        printf(" el valor es %d \n", valor);
        set(registro_dato_1, valor, es_registro_uint8_dato);
        printf("Registro datos: %d\n", registro_dato_1);
        
        break;
    case MOV_OUT: // MOV_OUT EDX ECX
        t_parametro *registro_direccion1 = list_get(list_parametros, 0);
        t_parametro *registro_datos1 = list_get(list_parametros, 1);

        void *registro_direccion_cpu= seleccionar_registro_cpu(registro_direccion1); // este es la direccion logica
        bool es_registro_uint8_direccion = es_de_8_bits(registro_direccion_cpu);
        //FALTA PONERLE SI ES DE 8 BITS O 32 EN REGISTRO_DIRECCION_CPU
        int direccion_fisica = traducir_direccion_logica_a_fisica(tamanio_pagina, registro_direccion_cpu, pcb->pid, es_registro_uint8_direccion);
        void *registro_datos_cpu = seleccionar_registro_cpu(registro_datos1);
        escribir_registro_datos_en(registro_datos_cpu, direccion_fisica);

        break;
    case SUM:
        t_parametro *registro_param1 = list_get(list_parametros, 0);
        registro1 = registro_param1->nombre;

        t_parametro* registro_param2 = list_get(list_parametros, 1); // valor_void = 2do registro
        char *registro2 = registro_param2->nombre;

        void* registro_origen = seleccionar_registro_cpu(registro1);
        bool es_registro_uint8_origen = es_de_8_bits(registro1);
        void* registro_destino = seleccionar_registro_cpu(registro2);
        bool es_registro_uint8_destino = es_de_8_bits(registro2);

        // set()
        sum(registro_origen, registro_destino, es_registro_uint8_origen, es_registro_uint8_destino);
        break;
    case SUB:
        t_parametro *registro_origen_param_resta = list_get(list_parametros, 0);
        char *registro_origen_resta = registro_origen_param_resta->nombre;

        t_parametro *registro_destino_param_resta = list_get(list_parametros, 1);
        char *registro_destino_resta = registro_destino_param_resta->nombre;

        void *registro_origen_pcb_resta = seleccionar_registro_cpu(registro_origen_resta);
        void *registro_destino_pcb_resta = seleccionar_registro_cpu(registro_destino_resta);

        bool es_8_bits_origen_resta = es_de_8_bits(registro_origen_resta);
        bool es_8_bits_destino_resta = es_de_8_bits(registro_destino_resta);

        sub(registro_origen_pcb_resta, registro_destino_pcb_resta, es_8_bits_origen_resta, es_8_bits_destino_resta);
        break;
    case JNZ:
        t_parametro *registro_parametro = list_get(list_parametros, 0);
        registro1 = registro_parametro->nombre;
        
        t_parametro *pc = list_get(list_parametros, 1);
        int nuevo_pc = atoi(pc->nombre);

        void *registro = seleccionar_registro_cpu(registro1);
        jnz(registro, nuevo_pc, pcb);
        break;
    case RESIZE:
        break;
    case COPY_STRING:
        break;
    case WAIT:
        break;
    case SIGNAL:
        break;
    case IO_GEN_SLEEP: 
        t_parametro *interfaz1 = list_get(list_parametros, 0);
        char *interfazSeleccionada = interfaz1->nombre;

        printf("En la linea 476 tenemos esta interfaz: %s\n", interfazSeleccionada);

        t_parametro *unidades = list_get(list_parametros, 1);
        int unidadesDeTrabajo = atoi(unidades->nombre);

        t_buffer* buffer;
        buffer = llenar_buffer_dormir_IO(interfazSeleccionada, unidadesDeTrabajo);
        
        // el puntero al buffer de abajo YA ESTA SERIALIZADO
        
        pcb->program_counter++;
        desalojar(pcb, DORMIR_INTERFAZ, buffer);
        // recv(client_dispatch, &solicitud_unidades_trabajo, sizeof(int), MSG_WAITALL);
        // solicitud_dormirIO_kernel(interfazSeleccionada, unidadesDeTrabajo);

        free(buffer->stream);
        free(buffer);

        printf("Hizo IO_GEN_SLEEP\n");
        return 1; // Retorna 1 si desalojo PCB...
        break;
    case IO_STDIN_READ:
        /*
        t_parametro *interfaz2 = list_get(list_parametros,0);
        char *interfazSeleccionada2 = interfaz2->nombre;
        
        t_parametro *registro_destino2 = list_get(list_parametros,1);
        void* valor_registro_destino = seleccionar_registro_cpu(registro_destino2->nombre);

        t_parametro* registro_tamanio = list_get(list_parametros,2);
        void* valor_registro_tamanio = seleccionar_registro_cpu(registro_tamanio->nombre);

        solicitud_lecturaIO_kernel(interfazSeleccionada2,valor_registro_destino,valor_registro_tamanio);
        */
        break;
    case EXIT_INSTRUCCION:
        // guardar_estado(pcb); -> No estoy seguro si esta es necesaria, pero de todas formas nos va a servir cuando se interrumpa por quantum
        desalojar(pcb, FIN_PROCESO, NULL); // Envio 0 ya que no me importa size
        break;
    default:
        printf("Error: No existe ese tipo de intruccion\n");
        break;

        if (pcb->program_counter == 10) {
            imprimir_pcb(pcb); // Solo para ver.
        }
    }
    return 0;
}

void escribir_registro_datos_en(void* registro_datos, int direccion_fisica, bool esDe8bits) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    t_buffer *buffer = malloc(sizeof(t_buffer));
    
    buffer->size = sizeof(int) * 2;
    if(esDe8bits) {
        buffer->size += sizeof(uint8_t);    
    }else{
        buffer->size += sizeof(uint32_t);    
    }
    
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);
    if(esDe8bits) {
        memcpy(stream + buffer->offset, &registro_datos, sizeof(uint8_t));
        buffer->offset += sizeof(uint8_t); 
    }else{
        memcpy(stream + buffer->offset, &registro_datos, sizeof(uint32_t));
        buffer->offset += sizeof(uint32_t);  
    } 

    buffer->stream = stream;

    paquete->codigo_operacion = ESCRIBIR_REGISTRO_DATOS; // Podemos usar una constante por operación

    void *a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); 
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    // printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void mandar_direccion_fisica_a_mem(int direccion_fisica, bool es_de_8_bits) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    t_buffer *buffer = malloc(sizeof(t_buffer));

    buffer->size = sizeof(int) * 2;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    void *stream = buffer->stream;

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(stream + buffer->offset, &direccion_fisica, sizeof(int));
    buffer->offset += sizeof(int);
    memcpy(stream + buffer->offset, &es_de_8_bits, sizeof(bool));
    buffer->offset += sizeof(bool);

    buffer->stream = stream;

    paquete->codigo_operacion = DIRECCION_FISICA; // Podemos usar una constante por operación

    void *a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int) + sizeof(bool));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int); 
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    send(socket_memoria, a_enviar, buffer->size + sizeof(int) + sizeof(int) + sizeof(bool), 0);
    // printf("Paquete enviado!\n");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void desalojar(t_pcb* pcb, DesalojoCpu motivo, t_buffer* datos_adicionales) {
    guardar_estado(pcb);
    setear_registros_cpu(); 
    enviar_pcb(pcb, client_dispatch, motivo, datos_adicionales);
}

void solicitud_dormirIO_kernel(char* interfaz, int unidades) {
    t_buffer* buffer; // Creo que no hace falta reservar memoria
    buffer = llenar_buffer_dormir_IO(interfaz, unidades);
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = DORMIR_INTERFAZ; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar   
    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    // hay que poner el socket kernel globallll
    send(client_dispatch, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    // printf("Paquete enviado!");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

/*
void solicitud_lecturaIO_kernel(char* interfaz,void* valorRegistroDestino,void* valorRegistroTamanio){
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer = llenar_buffer_leer_IO(interfaz, valorRegistroDestino,valorRegistroTamanio);
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = LEER_INTERFAZ; // Podemos usar una constante por operación
    paquete->buffer = buffer; // Nuestro buffer de antes.

    // Armamos el stream a enviar
    void* a_enviar = malloc(buffer->size + sizeof(int) + sizeof(int));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Por último enviamos
    // hay que poner el socket kernel globallll
    send(client_dispatch, a_enviar, buffer->size + sizeof(int) + sizeof(int), 0);
    printf("Paquete enviado!");

    // Falta liberar todo
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}
*/
// ver
/*
typedef struct {
    int nombre_interfaz_largo;
    char* nombre_interfaz;
    int unidadesDeTrabajo;
} t_operacion_io; 
*/

t_buffer* llenar_buffer_dormir_IO(char* interfaz, int unidades) {
    int length_interfaz = string_length(interfaz) + 1;

    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_operacion_io* io = malloc(sizeof(t_operacion_io));

    io->nombre_interfaz_largo = length_interfaz;
    io->nombre_interfaz = malloc(length_interfaz);
    strcpy(io->nombre_interfaz, interfaz);
    io->unidadesDeTrabajo = unidades;
    printf("Este es el nombre de la Interfaz segun operaciones.c: %s\n", io->nombre_interfaz);

    buffer->size = sizeof(int) * 2 + length_interfaz;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &io->unidadesDeTrabajo, sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &io->nombre_interfaz_largo, sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, io->nombre_interfaz, io->nombre_interfaz_largo);
    printf("nombre interfaz lalala %s",  io->nombre_interfaz);
    // buffer->offset += io->nombre_interfaz_largo;
    
    free(io->nombre_interfaz);
    free(io);
    return buffer;
}

/*
t_buffer* llenar_buffer_leer_IO(interfaz, valorRegistroDestino,valorRegistroTamanio){
    return NULL; // TODO
}
*/
// Dear Caro, our code has so many problems, please solve them, thank you very much, Mati 

/* 
void enviar_fin_programa(t_pcb *pcb) {
    enviar_pcb(pcb, client_dispatch, FIN_PROCESO, NULL, 0);
}
*/

void guardar_estado(t_pcb *pcb) {
    pcb->registros->AX = registros_cpu->AX;
    printf("Guardo el AX: %d\n", pcb->registros->AX);
    pcb->registros->BX = registros_cpu->BX;
    printf("Guardo el BX: %d\n", pcb->registros->BX);
    pcb->registros->CX = registros_cpu->CX;
    pcb->registros->DX = registros_cpu->DX;
    pcb->registros->EAX = registros_cpu->EAX;
    pcb->registros->EBX = registros_cpu->EBX;
    pcb->registros->ECX = registros_cpu->ECX;
    pcb->registros->EDX = registros_cpu->EDX;
    pcb->registros->SI = registros_cpu->SI;
    pcb->registros->DI = registros_cpu->DI;
}

// Deberia estar en otro lado
void setear_registros_cpu() {
    registros_cpu->AX = 0;
    registros_cpu->BX = 0;
    registros_cpu->CX = 0;
    registros_cpu->DX = 0;

    registros_cpu->EAX = 0;
    registros_cpu->EBX = 0;
    registros_cpu->ECX = 0;
    registros_cpu->EDX = 0;

    registros_cpu->SI = 0;
    registros_cpu->DI = 0;
}

void *seleccionar_registro_cpu(char *nombreRegistro) {

    if (strcmp(nombreRegistro, "AX") == 0)
    {
        return &registros_cpu->AX;
    }
    else if (strcmp(nombreRegistro, "BX") == 0)
    {
        return &registros_cpu->BX;
    }
    else if (strcmp(nombreRegistro, "CX") == 0)
    {
        return &registros_cpu->CX;
    }
    else if (strcmp(nombreRegistro, "DX") == 0)
    {
        return &registros_cpu->DX;
    }
    else if (strcmp(nombreRegistro, "SI") == 0)
    {
        return &registros_cpu->SI;
    }
    else if (strcmp(nombreRegistro, "DI") == 0)
    {
        return &registros_cpu->DI;
    }
    else if (strcmp(nombreRegistro, "EAX") == 0)
    {
        return &registros_cpu->EAX;
    }
    else if (strcmp(nombreRegistro, "EBX") == 0)
    {
        return &registros_cpu->EBX;
    }
    else if (strcmp(nombreRegistro, "ECX") == 0)
    {
        return &registros_cpu->ECX;
    }
    else if (strcmp(nombreRegistro, "EDX") == 0)
    {
        return &registros_cpu->EDX;
    }

    return NULL;
}

///////////////////////////
/////  INSTRUCCIONES //////
///////////////////////////

// Esta funcion transforma ese uint8 en un uint32;

void set(void *registro, uint32_t valor, bool es_8_bits) {
    // registro = &valor;
    if (es_8_bits) {
        *(uint8_t *)registro = (uint8_t)valor; // NO va a haber ninguno registro de 8 bits que pase el limite
    }
    else {
        *(uint32_t *)registro = valor;
    }
}

void sub(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino) {
    uint32_t valor_origen;
    uint32_t valor_destino;

    // Obtener el valor del registro origen
    if (es_8_bits_origen) {
        valor_origen = *(uint8_t *)registroOrigen;
    } else {
        valor_origen = *(uint32_t *)registroOrigen;
    }

    // Obtener el valor del registro destino
    if (es_8_bits_destino) {
        valor_destino = *(uint8_t *)registroDestino;
    } else {
        valor_destino = *(uint32_t *)registroDestino;
    }

    // Realizar la resta
    uint32_t resultado = valor_destino - valor_origen;

    // Actualizar el registro destino con el resultado usando la función set
    set(registroDestino, resultado, es_8_bits_destino);
}

void sum(void* registroOrigen, void* registroDestino, bool es_8_bits_origen, bool es_8_bits_destino) {
    uint32_t valor_origen;
    uint32_t valor_destino;

    // Obtener el valor del registro origen
    if (es_8_bits_origen) {
        valor_origen = *(uint8_t *)registroOrigen;
    } else {
        valor_origen = *(uint32_t *)registroOrigen;
    }

    // Obtener el valor del registro destino
    if (es_8_bits_destino) {
        valor_destino = *(uint8_t *)registroDestino;
    } else {
        valor_destino = *(uint32_t *)registroDestino;
    }

    // Realizar la suma
    uint32_t resultado_suma = valor_origen + valor_destino;

    // Actualizar el registro destino con el resultado usando la función set
    set(registroDestino, resultado_suma, es_8_bits_destino);
}


// Lo mismo que sum pero negativo
/*void sub(void* registroOrigen, void* registroDestino, bool es_8_bits, t_pcb* pcb) {
   if (es_8_bits) {
        *(uint8_t *)registroOrigen -= *(uint8_t *)registroDestino; // NO va a haber ninguno registro de 8 bits que pase el limite
    } else {
        *(uint32_t *)registroOrigen -= *(uint32_t *)registroDestino;
    }
}*/

void jnz(void* registro, int valor, t_pcb* pcb) {
    if ((void *)registro != 0) {
        pcb->program_counter = valor;
    }
}

/*
bool sonTodosDigitosDe(char *palabra) {
    while (*palabra)
    {
        if (!isdigit(*palabra))
        {
            return false;
        }
        palabra++;
    }
    return true;
}
*/