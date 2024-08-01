#include "../include/main.h"

//./bin/entradasalida generica.config Interfaz1
//  Bin entradasalida es 0
//   es 1
//  config es 2



int main(int argc, char* argv[2]) {
    
    // Este archivo lo recibe por parametro
    t_config* config_io = iniciar_config(argv[1]);
    logger_io = iniciar_logger("entradasalida.log"); // Declararla en el .h
    
    if(config_io == NULL) {
        log_error(logger_io, "No se pudo leer el archivo de configuracion");
        return 1;
    }

    char* nombre_io = argv[2];
    printf("Este es el nombre del config: %s\n", argv[1]);

    char* tipo_interfaz = config_get_string_value(config_io, "TIPO_INTERFAZ"); // TOdos
    char* IP_KERNEL = config_get_string_value(config_io, "IP_KERNEL"); // Todos
    char* puerto_kernel = config_get_string_value(config_io, "PUERTO_KERNEL"); // Todos
    char* IP_MEMORIA = config_get_string_value(config_io, "IP_MEMORIA"); // Todos
    char* puerto_memoria = config_get_string_value(config_io, "PUERTO_MEMORIA"); // Todos

    sem_init(&se_escribio_memoria, 0, 0);
    
    // int socket_kernel_io = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombre_io);     
    // close(socket_kernel_io);

    // Falta manejar los distintos tipos de interfaces
    printf("Tipo de interfaz: %s\n", tipo_interfaz);

    if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        iniciar_stdout(nombre_io, config_io, IP_KERNEL,IP_MEMORIA, puerto_kernel, puerto_memoria);
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        iniciar_stdin(nombre_io, config_io, IP_KERNEL, IP_MEMORIA, puerto_kernel, puerto_memoria) ;      
    } else if (strcmp(tipo_interfaz, "GENERICA") == 0) {
        iniciar_interfaz_generica(nombre_io, config_io, IP_KERNEL, puerto_kernel);
    } else if (strcmp(tipo_interfaz, "DialFS") == 0) {
        iniciar_dialfs(nombre_io,config_io,IP_KERNEL,IP_MEMORIA,puerto_kernel,puerto_memoria);
    } 

    free(nombre_io);
    return 0;
}

void iniciar_interfaz_generica(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* puerto_kernel) {
    // Saco el unico dato que tiene esta interfaz
    int tiempo_unidad_trabajo = config_get_int_value(config_io, "TIEMPO_UNIDAD_TRABAJO");
    kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, GENERICA, 5); 
    
    t_config_socket_io* config_generica_io = malloc(sizeof(t_config_socket_io));
    config_generica_io->config_io = config_io;
    config_generica_io->socket_io = kernelfd;
    
    recibir_kernel((void*) config_generica_io); 
}

void iniciar_stdin(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria) {
    kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, STDIN, 13); 
    memoriafd = conectar_io_memoria(IP_MEMORIA, puerto_memoria, logger_io, nombreInterfaz, STDIN, 91);

    t_config_socket_io* config_stdin_io = malloc(sizeof(t_config_socket_io));
    config_stdin_io->config_io = config_io;
    config_stdin_io->socket_io = kernelfd;
    
    recibir_kernel((void*) config_stdin_io);
}

void iniciar_stdout(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria) {
    kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, STDOUT, 15); 
    memoriafd = conectar_io_memoria(IP_MEMORIA, puerto_memoria, logger_io, nombreInterfaz, STDOUT, 79);

    t_config_socket_io* config_stdout_io = malloc(sizeof(t_config_socket_io));
    config_stdout_io->config_io = config_io;
    config_stdout_io->socket_io = memoriafd;

    recibir_memoria((void*) config_stdout_io);
} 

void iniciar_dialfs(char* nombreInterfaz, t_config* config_io, char* IP_KERNEL, char* IP_MEMORIA, char* puerto_kernel, char* puerto_memoria) {
    dialfs = inicializar_file_system(config_io);

    if(dialfs == NULL) {
        log_error(logger_io, "No se pudo inicializar el file system");
        return;
    }
    
    char filepath_bitmap[256]; 
    char filepath_bloques[256]; 

    snprintf(filepath_bloques, sizeof(filepath_bloques), "%s/bloques.dat", dialfs->path_base);
    snprintf(filepath_bitmap, sizeof(filepath_bitmap), "%s/bitmap.dat", dialfs->path_base);

    if (access(filepath_bitmap, F_OK) == -1 || access(filepath_bloques, F_OK) == -1) { // Si alguno de los archivos no existe
        crear_archivos_iniciales(filepath_bitmap, filepath_bloques);
    } else {
        recuperar_bitmap_y_bloques(filepath_bitmap, filepath_bloques);
    }

    diccionario_archivos = dictionary_create();

    rearmar_diccionario_archivos(dialfs->path_base);

    mostrar_diccionario_no_vacio(diccionario_archivos);

    kernelfd = conectar_io_kernel(IP_KERNEL, puerto_kernel, logger_io, nombreInterfaz, DIALFS, 17); 
    memoriafd = conectar_io_memoria(IP_MEMORIA, puerto_memoria, logger_io, nombreInterfaz, DIALFS, 81);

    t_config_socket_io* config_kernel_io = malloc(sizeof(t_config_socket_io));
    config_kernel_io->config_io = config_io;
    config_kernel_io->socket_io = kernelfd;

    t_config_socket_io* config_memoria_io = malloc(sizeof(t_config_socket_io));
    config_memoria_io->config_io = config_io;
    config_memoria_io->socket_io = memoriafd;

    recibir_kernel_y_memoria(config_kernel_io, config_memoria_io);
    // recibir_memoria(config_io, memoriafd); TO DO: CONECTAR CON MEMORIA DESDE MEMORIA
}

void recuperar_bitmap_y_bloques(char* filepath_bitmap, char* filepath_bloques) {
    // Abre los archivos existentes
    int fd_bitmap = open(filepath_bitmap, O_RDWR, S_IRUSR | S_IWUSR);
    int fd_bloques = open(filepath_bloques, O_RDWR, S_IRUSR | S_IWUSR);

    // Calcula el tamaño del bitmap y de los bloques
    int tamanio_bitmap = dialfs->block_count / 8;
    size_t tamanio_bloques = dialfs->block_size * dialfs->block_count;

    // Mapea el contenido de los archivos en memoria
    bitmap_data = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bitmap, 0);
    bloques_data = mmap(NULL, tamanio_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bloques, 0);

    // Crea un t_bitarray con la memoria mapeada
    bitmap = bitarray_create_with_mode(bitmap_data, tamanio_bitmap, LSB_FIRST);

    // Cierra los archivos
    close(fd_bloques);
    close(fd_bitmap);
}

void rearmar_diccionario_archivos(char* path_base) {
    DIR* dir;
    struct dirent* ent;

    dir = opendir(path_base);

    if (dir == NULL) {
        printf("No se pudo abrir el directorio %s\n", path_base);
        return;
    }

    while ((ent = readdir(dir)) != NULL && hay_files_txt(path_base)) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 
                            || strcmp(ent->d_name, "bloques.dat") == 0 || strcmp(ent->d_name, "bitmap.dat") == 0) continue;

        char* path_archivo = malloc(strlen(path_base) + strlen(ent->d_name) + 2); // +2?
        sprintf(path_archivo, "%s/%s", path_base, ent->d_name);

        FILE* metadata_file = fopen(path_archivo, "r"); // Abro para lectura

        if(metadata_file != NULL) {
            // Esto esta en revision, pero deberia descartarme la primera linea que es el first_block para ir directo al tamanio
            int first_block;
            fscanf(metadata_file, "BLOQUE_INICIAL=%d\n", &first_block); // Lee el bloque inicial

            int tamanio;
            fscanf(metadata_file, "TAMANIO_ARCHIVO=%d\n", &tamanio); // Lee el tamaño del archivo
            // fclose(metadata_file);

            t_archivo* archivo = malloc(sizeof(t_archivo));
            archivo->first_block = first_block; 
            archivo->block_count = asignar_bloques(tamanio); 
            archivo->name_file = strdup(ent->d_name);

            dictionary_put(diccionario_archivos, ent->d_name, archivo);
        } else {
            printf("metadatafile es NULL\n");
        }
     
        fclose(metadata_file);
        free(path_archivo);
    }

    closedir(dir);
}

// Revisar funcion
bool hay_files_txt(char* path_base) {
    DIR* dir = opendir(path_base);

    if (dir == NULL) {
        printf("No se pudo abrir el directorio %s\n", path_base);
        return false;
    }

    struct dirent* ent;

    while ((ent = readdir(dir)) != NULL) {
        // Comprueba si la extensión del archivo es .txt
        char* ext = strrchr(ent->d_name, '.');
        if (ext != NULL && strcmp(ext, ".txt") == 0) {
            // Se encontró un archivo .txt
            closedir(dir);
            return true;
        }
    }

    // No se encontró ningún archivo .txt
    closedir(dir);
    return false;
}

int asignar_bloques(int tamanio_file) {
    if(tamanio_file == 0) {
        return 1;
    }

    int bloques_necesarios = tamanio_file / dialfs->block_size;

    if (tamanio_file % dialfs->block_size != 0) {
        bloques_necesarios++;
    }

    return bloques_necesarios;

}

void recibir_kernel_y_memoria(t_config_socket_io* config_kernel_io, t_config_socket_io* config_memoria_io) {
    pthread_t hilo_kernel;
    pthread_t hilo_memoria;
    
    pthread_create(&hilo_kernel, NULL, (void*) recibir_kernel, (void*) config_kernel_io);
    pthread_create(&hilo_memoria, NULL, (void*) recibir_memoria, (void*) config_memoria_io);

    // Uso los join porque sino se me corta el main
    pthread_join(hilo_kernel, NULL);
    pthread_join(hilo_memoria, NULL);
}
