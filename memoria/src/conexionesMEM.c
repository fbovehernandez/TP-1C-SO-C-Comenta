#include "../include/conexionesMEM.h"

pthread_t cpu_thread;
pthread_t kernel_thread;
t_config* config_memoria;
t_dictionary* diccionario_instrucciones;

//Acepta el handshake del cliente, se podria hacer mas generico y que cada uno tenga un valor diferente
int esperar_cliente(int socket_servidor, t_log* logger_memoria)
{
	int handshake = 0;
	int resultError = -1;
	int socket_cliente = accept(socket_servidor, NULL, NULL);

    if(socket_cliente == -1) {
        return -1;
    }
    
	recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL); 
	if(handshake == 1) {
        // pthread_t kernel_thread;
        pthread_create(&kernel_thread, NULL, (void*)handle_kernel, (void*)(intptr_t)socket_cliente);
        
    } else if(handshake == 2) {
        // pthread_t cpu_thread;
        pthread_create(&cpu_thread, NULL, (void*)handle_cpu, (void*)(intptr_t)socket_cliente);
    } else {
        send(socket_cliente, &resultError, sizeof(int), 0);
        close(socket_cliente);
        return -1;
    }

	return socket_cliente;
}

void* handle_cpu(void* socket) { // Aca va a pasar algo parecido a lo que pasa en handle_kernel, se va a recibir peticiones de cpu y se va a hacer algo con ellas (iternado switch)
    int socket_cpu = (intptr_t)socket;
    int codigo_operacion = 0;
    // free(socket); 
    int resultOk = 0;
    // Envio confirmacion de handshake!
    send(socket_cpu, &resultOk, sizeof(int), 0);
    printf("Se conecto un el cpu!\n");
    
    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete\n");
        recv(socket_cpu, &(paquete->codigo_operacion), sizeof(int), 0);
        printf("Recibi el codigo de operacion : %d\n", paquete->codigo_operacion);

        // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
        recv(socket_cpu, &(paquete->buffer->size), sizeof(int), 0);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, 0);

        switch(paquete->codigo_operacion) { // Ver si no es mejor recibir primero el cod-op y adentro recibimos el resto (Cambiaria la forma de serializar, ya no habria un "Paquete") 
            case QUIERO_INSTRUCCION:
                // TO DO
                t_solicitud_instruccion* solicitud_cpu = deserializar_solicitud(paquete->buffer);
                enviar_instruccion(solicitud_cpu);
                break;
            default:
                printf("Rompio todo?\n");
                // close(socket_cpu); NO cierro conexion
                return NULL;
        }
        
        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }
    // Liberamos memoria
    return NULL;
}

// Ver error
void* handle_kernel(void* socket) {
    int socket_kernel = (intptr_t)socket; // (intptr_t)
    // free(socket); 
    int resultOk = 0;
    send(socket_kernel, &resultOk, sizeof(int), 0);
    printf("Se conecto el kernel!\n");

    char* path_completo;

    while(1) {
        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        // Primero recibimos el codigo de operacion
        printf("Esperando recibir paquete\n");
        recv(socket_kernel, &(paquete->codigo_operacion), sizeof(int), 0);
        printf("Recibi el codigo de operacion : %d\n", paquete->codigo_operacion);

        // Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
        recv(socket_kernel, &(paquete->buffer->size), sizeof(int), 0);
        paquete->buffer->stream = malloc(paquete->buffer->size);
        recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, 0);

        // Ahora en función del código recibido procedemos a deserializar el resto
        switch(paquete->codigo_operacion) {
            case PATH:                                       // Ver si no es mejor recibir primero el cod-op y adentro recibimos el resto (Cambiaria la forma de serializar, ya no habira)
                t_path* path = deserializar_path(paquete->buffer);
                imprimir_path(path);
                path_completo = agrupar_path(path); // Ojo con el SEG_FAULT
                printf("CUIDADO QUE IMPRIMO EL PATH: %s", path_completo);

                crear_estructuras(path_completo, path->PID);
                
                printf("Cuidado que voy a imprimir el diccionario...");

                imprimir_diccionario(); // Imprime todo, sin PID especifico...
                
                free(path_completo);
                break;
            default: 
                break;
        }

        // Liberamos memoria
        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }

    return NULL;
}
    
void imprimir_diccionario() {
    // t_list* instrucciones = dictionary_elements(diccionario_instrucciones);

    dictionary_iterator(diccionario_instrucciones, (void*)print_instrucciones);  
}

void print_parametros(char* parametro) {
    printf("Parametro %s \n", parametro);
}

void print_instruccion(t_instruccion* instruccion) {
    printf("Nombre instruccion %s: \n", instruccion->nombre);
    printf("Cantidad parametros %d :", instruccion->cantidad_parametros);
    list_iterate(instruccion->parametros, (void*)print_parametros);
}

void print_instrucciones(char* pid, t_list* lista_instrucciones) {
    // closure(element->key, element->data);
    printf("El PID del proceso a mostrar es %s: \n", pid);
    list_iterate(lista_instrucciones, (void*)print_instruccion); 
}

void crear_estructuras(char* path_completo, int pid) {
    FILE* file = fopen(path_completo, "r");
    if(file == NULL) {
        printf("No se pudo abrir el archivo\n");
        return;
    }

    /* 
         char* linea_leida = NULL;
    size_t length = 0;

    while (getline(&linea_leida, &length, archivo_pseudocodigo) != -1) {
    	if(strcmp(linea_leida,"\n")){
        	list_add(instrucciones_leidas, parsear_instruccion(linea_leida));
    	}
    }
    */
    t_list* instrucciones = list_create();

    char* line = NULL;
    size_t bufsize = 0; // Tamaño inicial del buffer

    while ((getline(&line, &bufsize, file)) != -1) {
        if(strcmp(line, "\n") != 0) {
            t_instruccion* instruccion = build_instruccion(line);
            list_add(instrucciones, instruccion);
        }
    }
    
    char* pid_char = malloc(sizeof(int));
    sprintf(pid_char, "%d", pid);
    
    // char* pid_char = pid + '0'; 
    // pid_char = (char*)pid;
    // char *pidchar = (char *)&pid;
    
    diccionario_instrucciones = dictionary_create();
    dictionary_put(diccionario_instrucciones, pid_char, instrucciones);

    // ->  

    free(line);
    fclose(file);
}

t_instruccion* build_instruccion(char* line) {
    t_instruccion* instruccion = malloc(sizeof(t_instruccion)); // SET AX 1

    char* nombre_instruccion = strtok(line, " \n"); // char* nombre_instruccion = strtok(linea_leida, " \n");
    instruccion->nombre = strdup(nombre_instruccion); 

    instruccion->parametros = list_create();

    char* arg = strtok(NULL, " \n");
    
    if(arg == NULL) {
        printf("El argumento es nulo!!!!!!\n");
    }

    while(arg != NULL) {
        list_add(instruccion->parametros, strdup(arg));
        arg = strtok(NULL, " \n");
    }
    
    instruccion->cantidad_parametros = list_size(instruccion->parametros);

    return instruccion;
}

//Encuentra la carpeta en la que se va a encontrar el path y la suma al nombre del path
char* agrupar_path(t_path* path) {
    char* pathConfig = config_get_string_value(config_memoria, "PATH_INSTRUCCIONES");
    char* path_completo = malloc(strlen(pathConfig) + path->path_length); // 69  + 20  + 1
    path_completo = strcat(pathConfig, path->path);
    printf("Path completo: %s\n", path_completo);
    return path_completo;
}

t_path* deserializar_path(t_buffer* buffer) {
    t_path* path = malloc(sizeof(t_path));
    
    void* stream = buffer->stream;

    memcpy(&(path->PID), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(path->path_length), stream, sizeof(int));
    stream += sizeof(int);
    
    // Reservo memoria para el path
    path->path = malloc(path->path_length);
    memcpy(path->path, stream, path->path_length);
    path->path[path->path_length - 1] = '\0'; // Es necesario?

    // La posta es que toque un par de cosas en lo de seralizar con mem dinamica,
    // pero enrealidad fui viendo que era diferente del de la catedra je, asi
    // que no se como funciona, pero funciona :)

    return path;

}

void imprimir_path(t_path* path) {
    printf("PID: %d\n", path->PID);
    printf("Path length: %d\n", path->path_length);
    printf("Path: %s\n", path->path);
}

