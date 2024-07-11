#include <../include/entradasalida.h>

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

    int total_size = block_count * block_size;
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

    crear_archivo_metadata(filename, bloque_libre);

    log_info(logger_entradasalida, "archivo creado correctamente"); 
    return 0;
}

void borrar_archivo(char* filename) {
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

    uint32_t bloques_actuales = (uint32_t)ceil((double)tamanio_archivo / (double)(block_size));
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

int truncar_archivo(const char* filename, uint32_t tamanio_nuevo, char* PID){
    char* metadata_path = buscar_archivo(filename);
    if (!metadata_path) {
        log_error(logger_entradasalida, "No se encontró el archivo: %s", filename);
        return -1;
    } 
    uint32_t tamanio_actual = obtener_tamanio_archivo(metadata_path);
    if(tamanio_actual > tamanio_nuevo){
        acortar_archivo(filename, metadata_path, tamanio_nuevo, tamanio_actual);
    } else {
        agrandar_archivo(filename, metadata_path, tamanio_nuevo, tamanio_actual, PID);
    }
    return 0;
}

void agrandar_archivo(const char* filename ,const char* metadata_path, uint32_t tamanio_nuevo, uint32_t tamanio_actual, char* PID) {
    uint32_t bloque_inicial = obtener_bloque_inicial(metadata_path);
    
    uint32_t bloques_adicionales = calcular_bloques_adicionales(tamanio_actual, tamanio_nuevo);

    if (bloques_adicionales > 0){
         int bloques_libres = verificar_bloques_contiguos_libres(bloque_inicial + (tamanio_actual / block_size), bloques_adicionales);
        if (bloques_libres == -1) {
            log_info(logger_entradasalida, "No se encontraron bloques contiguos libres suficientes");         
            compactar_file_system(metadata_path, PID);
            
            bloque_inicial = obtener_bloque_inicial(metadata_path);
            bloques_libres = verificar_bloques_contiguos_libres(bloque_inicial + (tamanio_actual / block_size), bloques_adicionales);
            if (bloques_libres == -1) {
                log_error(logger_entradasalida, "No se encontraron bloques contiguos libres suficientes después de la compactación");
                free(metadata_path);
                return;
            }
        }

        for (uint32_t i = 0; i < bloques_adicionales; i++) {
            asignar_bloque(bloque_inicial + i);
        }
    }

    actualizar_metadata_tamanio(metadata_path, tamanio_nuevo);
    log_info(logger_entradasalida, "Archivo %s ampliado a %u bytes y en %u bloques", filename, tamanio_nuevo ,bloques_adicionales);
}

void acortar_archivo(const char* filename ,const char* metadata_path, uint32_t tamanio_nuevo, uint32_t tamanio_actual) {
    uint32_t bloque_inicial = obtener_bloque_inicial(metadata_path);

    uint32_t bloques_a_liberar = calcular_bloques_a_liberar(tamanio_actual, tamanio_nuevo); 

    if(bloques_a_liberar > 0){
        uint32_t ultimo_bloque = obtener_ultimo_bloque(bloque_inicial, tamanio_actual);//crear
        for(uint32_t i = 0; i < bloques_a_liberar; i++){
            liberar_bloque(ultimo_bloque - i);//crear
        }
    }
    actualizar_metadata_tamanio(metadata_path, tamanio_nuevo);
    log_info(logger_entradasalida, "Archivo %s reducido a %u bytes y en %u bloques", filename, tamanio_nuevo ,bloques_a_liberar);
}

void escribir_archivo(char* filename, char* datos, uint32_t tamanio_datos, int puntero_archivo){
    char* metadata_path = buscar_archivo(filename);
    if (!metadata_path) {
        printf("Archivo no encontrado\n");
        return;
    }

    uint32_t tamanio_archivo = obtener_tamanio_archivo(metadata_path);
    if (puntero_archivo + tamanio_datos > tamanio_archivo) {
        printf("El tamaño de datos excede el tamaño del archivo\n");
        free(metadata_path);
        return;
    }

    int archivo_fd = open(path_archivo_bloques, O_RDWR);
    if (archivo_fd == -1) {
        printf("Error al abrir el archivo de bloques\n");
        free(metadata_path);
        return;
    }

    void* mmap_bloques = mmap(NULL, block_size * block_count, PROT_READ | PROT_WRITE, MAP_SHARED, archivo_fd, 0);
    if (mmap_bloques == MAP_FAILED) {
        printf("Error al mapear el archivo de bloques en memoria\n");
        close(archivo_fd);
        free(metadata_path);
        return;
    }

    uint32_t bloque_inicial = obtener_bloque_inicial(metadata_path);
    uint32_t offset_inicial = puntero_archivo % block_size;
    uint32_t bloque_actual = bloque_inicial + (puntero_archivo / block_size);
    uint32_t bytes_escritos = 0;

    while (bytes_escritos < tamanio_datos) {
        size_t offset = bloque_actual * block_size + offset_inicial;

        size_t bytes_a_escribir;
        if (tamanio_datos - bytes_escritos > block_size - offset_inicial) {
        bytes_a_escribir = block_size - offset_inicial;
        } else {
            bytes_a_escribir = tamanio_datos - bytes_escritos;
        }

        memcpy(mmap_bloques + offset, datos + bytes_escritos, bytes_a_escribir);
        bytes_escritos += bytes_a_escribir;
        offset_inicial = 0;  // Para siguientes iteraciones, offset_inicial será 0
        bloque_actual++;
    }

    // Sincronizar los cambios
    if (msync(mmap_bloques, block_size * block_count, MS_SYNC) == -1) {
        printf("Error al sincronizar el archivo de bloques\n");
    }

    if (munmap(mmap_bloques, block_size * block_count) == -1) {
        printf("Error al desmapear la memoria\n");
    }

    close(archivo_fd);
    free(metadata_path);
}

void* leer_archivo(char* filename, uint32_t tamanio_datos, int puntero_archivo) {
    void* buffer;
    char* metadata_path = buscar_archivo(filename);
    if (!metadata_path) {
        printf("Archivo no encontrado\n");
        return;
    }

    uint32_t tamanio_archivo = obtener_tamanio_archivo(metadata_path);
    if (puntero_archivo + tamanio_datos > tamanio_archivo) {
        printf("El tamaño de datos a leer excede el tamaño del archivo\n");
        free(metadata_path);
        return;
    }

    int archivo_fd = open(path_archivo_bloques, O_RDONLY);
    if (archivo_fd == -1) {
        printf("Error al abrir el archivo de bloques\n");
        free(metadata_path);
        return;
    }

    void* mmap_bloques = mmap(NULL, block_size * block_count, PROT_READ, MAP_SHARED, archivo_fd, 0);
    if (mmap_bloques == MAP_FAILED) {
        printf("Error al mapear el archivo de bloques en memoria\n");
        close(archivo_fd);
        free(metadata_path);
        return;
    }

    uint32_t bloque_inicial = obtener_bloque_inicial(metadata_path);
    uint32_t offset_inicial = puntero_archivo % block_size;
    uint32_t bloque_actual = bloque_inicial + (puntero_archivo / block_size);

    uint32_t bytes_leidos = 0;
    while (bytes_leidos < tamanio_datos) {
        size_t offset = bloque_actual * block_size + offset_inicial;
        size_t bytes_a_leer;
        if (tamanio_datos - bytes_leidos > block_size - offset_inicial) {
            bytes_a_leer = block_size - offset_inicial;
        } else {
            bytes_a_leer = tamanio_datos - bytes_leidos;
        }
        memcpy(buffer + bytes_leidos, mmap_bloques + offset, bytes_a_leer);
        bytes_leidos += bytes_a_leer;
        offset_inicial = 0;  // Para siguientes iteraciones, offset_inicial será 0
        bloque_actual++;
    }

    if (munmap(mmap_bloques, block_size * block_count) == -1) {
        printf("Error al desmapear la memoria\n");
    }

    close(archivo_fd);
    free(metadata_path);
    return buffer;
}

void compactar_file_system(const char* archivo_a_mover, char* PID) {
    
    log_info(logger_entradasalida, "PID: %s - Inicio Compactacion", PID); //LOG OBLIGATORIO
    int cantidad_archivos = 0;
    archivo_info* archivos = listar_archivos(&cantidad_archivos);

    if (archivos == NULL) {
        log_error(logger_entradasalida, "No se encontraron archivos para compactar");
        return;
    }

    archivo_info archivo_mover_info;
    int encontrado = 0;

    // Primero, encontrar la información del archivo que se moverá al final
    for (int i = 0; i < cantidad_archivos; i++) {
        if (strcmp(archivos[i].nombre_archivo, archivo_a_mover) == 0) {
            archivo_mover_info = archivos[i];
            encontrado = 1;

            // Eliminar el archivo de la lista
            for (int j = i; j < cantidad_archivos - 1; j++) {
                archivos[j] = archivos[j + 1];
            }
            cantidad_archivos--;
            break;
        }
    }

    if (!encontrado) {
        log_error(logger_entradasalida, "No se encontró el archivo a mover: %s", archivo_a_mover);
        free(archivos);
        return;
    }

    // Crear un buffer para almacenar temporalmente los bloques del archivo a mover
    void* buffer = malloc(archivo_mover_info.cantidad_bloques * block_size);
    if (buffer == NULL) {
        log_error(logger_entradasalida, "Error al asignar memoria para el buffer");
        free(archivos);
        return;
    }

    int archivo_fd = open(path_archivo_bloques, O_RDWR);
    if (archivo_fd == -1) {
        log_error(logger_entradasalida, "Error al abrir el archivo de bloques");
        free(buffer);
        free(archivos);
        return;
    }

    void* mmap_bloques = mmap(NULL, block_size * block_count, PROT_READ | PROT_WRITE, MAP_SHARED, archivo_fd, 0);
    if (mmap_bloques == MAP_FAILED) {
        log_error(logger_entradasalida, "Error al mapear el archivo de bloques");
        close(archivo_fd);
        free(buffer);
        free(archivos);
        return;
    }

    // Copiar los bloques del archivo a mover al buffer
    for (uint32_t j = 0; j < archivo_mover_info.cantidad_bloques; j++) {
        void* origen = mmap_bloques + ((archivo_mover_info.bloque_inicial + j) * block_size);
        void* destino = buffer + (j * block_size);
        memcpy(destino, origen, block_size);

        // Marcar el bloque antiguo como libre
        bitarray_clean_bit(bitmap, archivo_mover_info.bloque_inicial + j);
    }

    // Mover todos los archivos excepto el archivo a mover al inicio del espacio libre contiguo
    uint32_t bloque_libre = 0; // Inicialmente, el primer bloque libre  
    for (int i = 0; i < cantidad_archivos; i++) {
        archivo_info* info = &archivos[i];

        // Obtener la ruta completa del archivo de metadata
        char* metadata_path = buscar_archivo(info->nombre_archivo);
        if (metadata_path == NULL) {
            log_error(logger_entradasalida, "No se pudo encontrar la metadata para el archivo: %s", info->nombre_archivo);
            continue;
        }

        for (uint32_t j = 0; j < info->cantidad_bloques; j++) {
            uint32_t bloque_actual = info->bloque_inicial + j;

            if (bloque_actual == bloque_libre) {
                bloque_libre++;
                continue;
            }

            // Mover el bloque y actualizar el bitmap
            mover_bloque(mmap_bloques,bloque_actual, bloque_libre);

            bloque_libre++;
        }

        // Actualizar la metadata con el nuevo bloque inicial
        actualizar_metadata_bloque_inicial(metadata_path, bloque_libre - info->cantidad_bloques);
        free(metadata_path);
    }

    // Ahora escribir los bloques del archivo específico al final del espacio libre
    for (uint32_t j = 0; j < archivo_mover_info.cantidad_bloques; j++) {
        void* origen = buffer + (j * block_size);
        void* destino = mmap_bloques + (bloque_libre * block_size);
        memcpy(destino, origen, block_size);

        // Marcar el nuevo bloque como ocupado en el bitmap
        bitarray_set_bit(bitmap, bloque_libre);

        bloque_libre++;
    }

    // Actualizar la metadata del archivo movido
    char* metadata_path = buscar_archivo(archivo_mover_info.nombre_archivo);
    if (metadata_path == NULL) {
        log_error(logger_entradasalida, "No se pudo encontrar la metadata para el archivo: %s", archivo_mover_info.nombre_archivo);
        free(buffer);
        free(archivos);
        return;
    }
    actualizar_metadata_bloque_inicial(metadata_path, bloque_libre - archivo_mover_info.cantidad_bloques);
    
    free(metadata_path);

    if (munmap(mmap_bloques, block_size * block_count) == -1) {
        log_error(logger_entradasalida, "Error al desmapear la memoria");
    }
    
    if (msync(mmap_bloques, block_size * block_count, MS_SYNC) == -1) {
        log_error(logger_entradasalida, "Error al sincronizar el archivo de bloques");
    }

    close(archivo_fd);
    free(buffer);

    // Esperar el tiempo determinado por RETRASO_COMPACTACION antes de finalizar
    usleep(retraso_compactacion * 1000); // Convertir milisegundos a microsegundos

    free(archivos);
    log_info(logger_entradasalida, "PID: %s - Fin Compactacion", PID); //LOG OBLIGATORIO
}