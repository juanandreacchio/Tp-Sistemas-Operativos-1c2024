#include <../include/entradasalida.h>

uint32_t buscar_bloque_libre()
{

    uint32_t bloque_libre = -1;

    uint32_t tamanio_bitmap = bitarray_get_max_bit(bitmap);
    for (int i = 0; i < tamanio_bitmap; i++)
    {

        if (!bitarray_test_bit(bitmap, i))
        { // el bit en la pos i del bitmap es 0 osea esta libre
            bloque_libre = i;
            break;
        }
    }
    return bloque_libre;
}

int verificar_bloques_contiguos_libres(uint32_t bloque_inicial, uint32_t cantidad_bloques) {
    uint32_t tamanio_bitmap = bitarray_get_max_bit(bitmap);

    // Verificar si hay suficientes bloques desde el bloque_inicial hasta el final del bitmap
    if (bloque_inicial + cantidad_bloques > tamanio_bitmap) {
        log_error(logger_entradasalida, "tamanio pedido fuera de rango");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 1; i <= cantidad_bloques; i++) {
        if (bitarray_test_bit(bitmap, bloque_inicial + i)) {
            // log_info(logger_entradasalida, "bloque: %d ocupado", bloque_inicial + i);
            return -1; // Si encuentra un bloque ocupado, retorna -1
        }
        // log_info(logger_entradasalida, "bloque: %d libre", bloque_inicial + i);
    }

    // Si todos los bloques están libres, retorna el índice del bloque inicial
    return 1;
}

void asignar_bloque(uint32_t bloque_libre) {
    if (bloque_libre >= bitarray_get_max_bit(bitmap)) {
        log_error(logger_entradasalida, "Bloque %d fuera de rango", bloque_libre);
        exit(EXIT_FAILURE);
    }

    if (bitarray_test_bit(bitmap, bloque_libre)) {
        log_error(logger_entradasalida, "Bloque %d ya está ocupado", bloque_libre);   
        return;
    }

    // Marcar el bloque como ocupado en el bitmap
    bitarray_set_bit(bitmap, bloque_libre);
    
    // sincronizar bitmap con disco

    if (msync(bitmap->bitarray, bitmap->size, MS_SYNC) == -1) {
        log_error(logger_entradasalida, "Error al sincronizar el archivo de bitmap");
        exit(EXIT_FAILURE);
    }

    // log_info(logger_entradasalida, "Bloque %d asignado al archivo ", bloque_libre);//TODO: aca al final ponian el nombre del archivo pero no estaba y como no c que va se los dejo asi.
}

void liberar_bloque(uint32_t bloque) {
    if (bloque >= bitarray_get_max_bit(bitmap)) {
        log_error(logger_entradasalida, "Bloque %d fuera de rango", bloque);
        exit(EXIT_FAILURE);
    }

    bitarray_clean_bit(bitmap, bloque);

    // sincronizar bitmap con disco

    if (msync(bitmap->bitarray, bitmap->size, MS_SYNC) == -1) {
        log_error(logger_entradasalida, "Error al sincronizar el archivo de bitmap");
        exit(EXIT_FAILURE);
    }

    // log_info(logger_entradasalida, "Bloque %d liberado", bloque);
}

char* buscar_archivo(const char* archivo_buscar) {
    struct dirent* entrada;
    DIR* dir = opendir(path_fs);

    if (dir == NULL) {
        log_error(logger_entradasalida,"No se pudo abrir el directorio");
        return NULL;
    }

    char* ruta_completa = NULL;
    while ((entrada = readdir(dir)) != NULL) {
        if (strcmp(entrada->d_name, archivo_buscar) == 0) {
            size_t len = strlen(path_fs) + strlen(archivo_buscar) + 2;
            ruta_completa = malloc(len);
            if (ruta_completa == NULL) {
                log_error(logger_entradasalida,"No se pudo asignar memoria para la ruta completa");
                closedir(dir);
                return NULL;
            }
            snprintf(ruta_completa, len, "%s%s", path_fs, archivo_buscar);
            break;
        }
    }

    closedir(dir);
    return ruta_completa;
}

uint32_t calcular_bloques_adicionales(uint32_t tamanio_actual,uint32_t tamanio_nuevo) {
    uint32_t bloques_actuales = (uint32_t)ceil((double)tamanio_actual / (double)(block_size));
    if(bloques_actuales == 0){
        bloques_actuales += 1;
    }
    uint32_t bloques_nuevos = (uint32_t)ceil((double)tamanio_nuevo / (double)(block_size));
    return bloques_nuevos - bloques_actuales;
}

uint32_t calcular_bloques_a_liberar(uint32_t tamanio_actual, uint32_t tamanio_nuevo) {
    uint32_t bloques_actuales = (uint32_t)ceil((double)tamanio_actual / (double)(block_size));
    if(bloques_actuales == 0){
        bloques_actuales += 1;
    }
    uint32_t bloques_nuevos = (uint32_t)ceil((double)tamanio_nuevo / (double)(block_size));
    return bloques_actuales - bloques_nuevos;
}

uint32_t obtener_tamanio_archivo(char* metadata_path){
    
    t_config* config_metadata = config_create(metadata_path);

    if (config_metadata == NULL) {
        log_error(logger_entradasalida, "Error al abrir el archivo de metadata para actualizar");
        return (uint32_t)-1;
    }

    uint32_t tamanio_archivo = config_get_int_value(config_metadata, "TAMANIO_ARCHIVO");
    config_destroy(config_metadata);
    return tamanio_archivo;
}

uint32_t obtener_bloque_inicial(char* metadata_path){
    
    t_config* config_metadata = config_create(metadata_path);
    
    if (config_metadata == NULL) {
        log_error(logger_entradasalida, "Error al abrir el archivo de metadata: %s", metadata_path);
        return (uint32_t)-1;
    }
    uint32_t bloque_inicial = config_get_int_value(config_metadata, "BLOQUE_INICIAL");
    config_destroy(config_metadata);
    return bloque_inicial;
}

uint32_t obtener_ultimo_bloque(uint32_t bloque_inicial, uint32_t tamanio_actual) {
    uint32_t bloques_actuales = (uint32_t)ceil((double)tamanio_actual / (double)(block_size));
    return bloque_inicial + bloques_actuales - 1;
}

void actualizar_metadata_tamanio(char* metadata_path, uint32_t tamanio_nuevo) {
    t_config* config = config_create(metadata_path);
    
    if (config == NULL) {
        log_error(logger_entradasalida, "Error al abrir el archivo de metadata para actualizar");
        exit(EXIT_FAILURE);
    }

    char buffer[10];
    sprintf(buffer, "%u", tamanio_nuevo);
    config_set_value(config, "TAMANIO_ARCHIVO", buffer);

    if (!config_save(config)) {
        log_error(logger_entradasalida, "Error al guardar el archivo de metadata después de actualizar");
        exit(EXIT_FAILURE);
    }

    config_destroy(config);
}

void actualizar_metadata_bloque_inicial( char* metadata_path, uint32_t bloque_inicial) {
    t_config* config = config_create(metadata_path);
    
    if (config == NULL) {
        log_error(logger_entradasalida, "Error al abrir el archivo de metadata para actualizar");
        exit(EXIT_FAILURE);
    }

    char buffer[10];
    sprintf(buffer, "%u", bloque_inicial);
    config_set_value(config, "BLOQUE_INICIAL", buffer);

    if (!config_save(config)) {
        log_error(logger_entradasalida, "Error al guardar el archivo de metadata después de actualizar");
        exit(EXIT_FAILURE);
    }
    
    config_destroy(config);
}

archivo_info* listar_archivos(int* cantidad_archivos) {
    archivo_info* archivos = NULL;
    *cantidad_archivos = 0;

    DIR* dir = opendir(path_fs);
    if (dir == NULL) {
        log_error(logger_entradasalida, "No se pudo abrir el directorio de archivos");
        return NULL;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            // Excluir bloques.dat y bitmap.dat
            if (strcmp(entry->d_name, "bloques.dat") != 0 && strcmp(entry->d_name, "bitmap.dat") != 0) {
                (*cantidad_archivos)++;
            }
        }
    }

    if (*cantidad_archivos > 0) {
        archivos = malloc((*cantidad_archivos) * sizeof(archivo_info));
        if (archivos == NULL) {
            log_error(logger_entradasalida, "No se pudo asignar memoria para archivos");
            closedir(dir);
            return NULL;
        }
        rewinddir(dir);  // rewinddir reinicia el puntero del directorio
        int index = 0;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) { // Solo archivos regulares
                // Excluir bloques.dat y bitmap.dat
                if (strcmp(entry->d_name, "bloques.dat") != 0 && strcmp(entry->d_name, "bitmap.dat") != 0) {
                    archivo_info* info = &archivos[index++];
                    snprintf(info->nombre_archivo, sizeof(info->nombre_archivo), "%s", entry->d_name);

                    size_t path_fs_tamanio = strlen(path_fs);
                    uint32_t tamanio_nombre_archivo = strlen(entry->d_name);
                    uint32_t metadata_path_tamanio = path_fs_tamanio + tamanio_nombre_archivo + 2; // +2 por el '/' y '\0'

                    char* metadata_path = malloc(metadata_path_tamanio * sizeof(char));
                    if (metadata_path == NULL) {
                        log_error(logger_entradasalida, "No se pudo asignar memoria para metadata_path");
                        free(archivos);
                        closedir(dir);
                        return NULL;
                    }

                    // Copiar path_fs al buffer
                    snprintf(metadata_path, metadata_path_tamanio, "%s/%s", path_fs, entry->d_name);

                    info->bloque_inicial = obtener_bloque_inicial(metadata_path);
                    if (info->bloque_inicial == (uint32_t)-1) {
                        log_error(logger_entradasalida, "Error al obtener el bloque inicial para %s", metadata_path);
                        free(metadata_path);
                        free(archivos);
                        closedir(dir);
                        return NULL;
                    }

                    uint32_t tamanio_archivo = obtener_tamanio_archivo(metadata_path);
                    if (tamanio_archivo == (uint32_t)-1) {
                        log_error(logger_entradasalida, "Error al obtener el tamaño del archivo para %s", metadata_path);
                        free(metadata_path);
                        free(archivos);
                        closedir(dir);
                        return NULL;
                    }

                    uint32_t cantidad_bloques = (uint32_t)ceil((double)tamanio_archivo / (double)block_size);
                    if (cantidad_bloques == 0) {
                        cantidad_bloques += 1;
                    }
                    info->cantidad_bloques = cantidad_bloques;
                    free(metadata_path);
                }
            }
        }
    }

    closedir(dir);
    return archivos;
}

void mover_bloque(void* mmap_bloques,uint32_t bloque_origen, uint32_t bloque_destino) {
    void* origen = mmap_bloques + (bloque_origen * block_size);
    void* destino = mmap_bloques + (bloque_destino * block_size);
    memcpy(destino, origen, block_size);

    liberar_bloque(bloque_origen); 
    asignar_bloque(bloque_destino);  
}
