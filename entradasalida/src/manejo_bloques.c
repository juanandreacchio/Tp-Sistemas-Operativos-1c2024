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

int crear_archivo(const char* base_path, const char* filename) {
    uint32_t bloque_libre = buscar_bloque_libre();

    if (bloque_libre == -1) {
        log_error(logger_entradasalida, "No se encontraron bloques libres");
        return -1;
    }

    asignar_bloque(bloque_libre);

    crear_archivo_metadata(base_path, filename, bloque_libre);
    
    log_info(logger_entradasalida, "Archivo %s creado", filename);

    return 0;
}