#include <../include/entradasalida.h>

const char* path_archivo_bloques = "bloques.dat"; // Ruta del archivo de bloques

void create_archivos_bloques() {
    int archivo_bloques = open(path_archivo_bloques, O_CREAT | O_RDWR, 0744); // O_CREAT crea el archivo si no existe, O_RDWR abre el archivo para lectura y escritura
    if (archivo_bloques == -1) {
        log_error(logger_entradasalida,"Error al crear el archivo de bloques");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(archivo_bloques, &st) == -1) {
        log_error(logger_entradasalida,"Error al obtener el tamaño del archivo de bloques");
        close(archivo_bloques);
        exit(EXIT_FAILURE);
    }
    off_t current_size = st.st_size;

    int total_size = atoi(block_count) * atoi(block_size);
    if (current_size < total_size) {
        if (ftruncate(archivo_bloques, total_size) == -1) {
            log_error(logger_entradasalida,"Error al ajustar el tamaño del archivo");
            close(archivo_bloques);
            exit(EXIT_FAILURE);
        }
    }

    void* mapeo = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, archivo_bloques, 0);  // PROT_READ y PROT_WROTE -> produce error si alg intenta leer/escribir si no cumple la proteccion
    if (mapeo == MAP_FAILED) {                                                                     
        log_error(logger_entradasalida,"Error al mapear el archivo de bloques");
        close(archivo_bloques);
        exit(EXIT_FAILURE);
    }
    
    if (lseek(archivo_bloques, 0, SEEK_END) == 0) { // Verificar si el archivo está vacío
        memset(mapeo, 0, total_size); // Inicializa la memoria a cero
    }

    if (munmap(mapeo, total_size) == -1) {   //munmap libera la memoria mapeada y el buffer
        log_error(logger_entradasalida,"Error al desmapear la memoria");
    }
    
    close(archivo_bloques);
}

int crear_archivo(const char* filename) {
    if(buscar_archivo(filename) != NULL) {
        log_error(logger_entradasalida, "el archivo %s ya existe", filename);
        return -1;
    }

    uint32_t bloque_libre = buscar_bloque_libre();

    if (bloque_libre == -1) {
        log_error(logger_entradasalida, "No se encontraron bloques libres");
        return -1;
    }

    asignar_bloque(bloque_libre);

    iniciar_archivo_metadata(filename, bloque_libre);
    
    log_info(logger_entradasalida, "Archivo %s creado", filename);

    return 0;
}

void borrar_archivo(const char* filename) {
    char* metadata_path = buscar_archivo(filename);
    if (metadata_path == NULL) {
        log_error(logger_entradasalida, "Archivo %s no encontrado", filename);
        return;
    }

    uint32_t tamanio_archivo = obtener_tamanio_archivo(metadata_path);
    uint32_t bloque_inicial = obtener_bloque_inicial(metadata_path);
    
    if (tamanio_archivo == (uint32_t)-1 || bloque_inicial == (uint32_t)-1) {
        log_error(logger_entradasalida, "Error al obtener información del archivo %s", filename);
        free(metadata_path);
        return;
    }

    uint32_t bloques_actuales = (uint32_t)ceil((double)tamanio_archivo / (double)atoi(block_size));
    for (uint32_t i = 0; i < bloques_actuales; i++) {
        liberar_bloque(bloque_inicial + i);
    }

    // Eliminar el archivo de metadata
    if (unlink(metadata_path) == -1) {
        log_error(logger_entradasalida, "Error al eliminar el archivo de metadata %s", metadata_path);
    } else {
        log_info(logger_entradasalida, "Archivo %s eliminado correctamente", filename);
    }

    free(metadata_path);
}

void agrandar_archivo(const char* filename, uint32_t tamanio_nuevo) {
    char* metadata_path = buscar_archivo(filename);  
    uint32_t tamanio_actual = obtener_tamanio_archivo(metadata_path);
    uint32_t bloque_inicial = obtener_bloque_inicial(metadata_path);
    
    uint32_t bloques_adicionales = calcular_bloques_adicionales(tamanio_actual, tamanio_nuevo);

    if (bloques_adicionales > 0){
        int bloques_libres = verificar_bloques_contiguos_libres(bloque_inicial, bloques_adicionales);
        if (bloques_libres == -1) {
            log_error(logger_entradasalida, "No se encontraron bloques contiguos libres suficientes");
            free(metadata_path);
            return;
        }

        for (uint32_t i = 0; i < bloques_adicionales; i++) {
            asignar_bloque(bloque_inicial + i);
        }
    }

    actualizar_metadata_tamanio(metadata_path, tamanio_nuevo);
    log_info(logger_entradasalida, "Archivo %s ampliado en %u bytes y en %u bloques", filename, tamanio_nuevo ,bloques_adicionales);
}

void acortar_archivo(const char* filename, uint32_t tamanio_nuevo) {
    char* metadata_path = buscar_archivo(filename);  
    uint32_t tamanio_actual = obtener_tamanio_archivo(metadata_path);
    uint32_t bloque_inicial = obtener_bloque_inicial(metadata_path);

    uint32_t bloques_a_liberar = calcular_bloques_a_liberar(tamanio_actual, tamanio_nuevo); 

    if(bloques_a_liberar > 0){
        uint32_t ultimo_bloque = obtener_ultimo_bloque(bloque_inicial, tamanio_actual);//crear
        for(uint32_t i = 0; i < bloques_a_liberar; i++){
            liberar_bloque(ultimo_bloque - i);//crear
        }
    }
    actualizar_metadata_tamanio(metadata_path, tamanio_nuevo);
    log_info(logger_entradasalida, "Archivo %s reducido en %u bytes y en %u bloques", filename, tamanio_nuevo ,bloques_a_liberar);
}