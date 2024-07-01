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
        return -1;
    }

    for (uint32_t i = 0; i < cantidad_bloques; i++) {
        if (bitarray_test_bit(bitmap, bloque_inicial + i)) {
            return -1; // Si encuentra un bloque ocupado, retorna -1
        }
    }

    // Si todos los bloques están libres, retorna el índice del bloque inicial
    return 1;
}

void asignar_bloque(uint32_t bloque_libre) {
    if (bloque_libre >= bitarray_get_max_bit(bitmap)) {
        log_error(logger_entradasalida, "Bloque %d fuera de rango", bloque_libre);
        return;
    }

    if (bitarray_test_bit(bitmap, bloque_libre)) {
        log_error(logger_entradasalida, "Bloque %d ya está ocupado", bloque_libre);
        return;
    }

    // Marcar el bloque como ocupado en el bitmap
    bitarray_set_bit(bitmap, bloque_libre);

    log_info(logger_entradasalida, "Bloque %d asignado al archivo %s", bloque_libre);
}

void liberar_bloque(uint32_t bloque) {
    if (bloque >= bitarray_get_max_bit(bitmap)) {
        log_error(logger_entradasalida, "Bloque %d fuera de rango", bloque);
        return;
    }

    bitarray_clean_bit(bitmap, bloque);
    log_info(logger_entradasalida, "Bloque %d liberado", bloque);
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
            snprintf(ruta_completa, len, "%s/%s", path_fs, archivo_buscar);
            break;
        }
    }

    closedir(dir);
    return ruta_completa;
}

uint32_t calcular_bloques_adicionales(uint32_t tamanio_actual,uint32_t tamanio_nuevo) {
    uint32_t bloques_actuales = (uint32_t)ceil((double)tamanio_actual / (double)atoi(block_size));
    uint32_t bloques_nuevos = (uint32_t)ceil((double)tamanio_nuevo / (double)atoi(block_size));
    return bloques_nuevos - bloques_actuales;
}

uint32_t calcular_bloques_a_liberar(uint32_t tamanio_actual, uint32_t tamanio_nuevo) {
    uint32_t bloques_actuales = (uint32_t)ceil((double)tamanio_actual / (double)atoi(block_size));
    uint32_t bloques_nuevos = (uint32_t)ceil((double)tamanio_nuevo / (double)atoi(block_size));
    return bloques_actuales - bloques_nuevos;
}

uint32_t obtener_tamanio_archivo(const char* metadata_path){
    
    t_config* config_metadata = config_create(metadata_path);

    if (config_metadata == NULL) {
        log_error(logger_entradasalida, "Error al abrir el archivo de metadata para actualizar");
        return;
    }

    uint32_t tamanio_archivo = atoi(config_get_string_value(config_metadata, "TAMANIO_ARCHIVO"));
    config_destroy(config_metadata);
    return tamanio_archivo;
}

uint32_t obtener_bloque_inicial(const char* metadata_path){
    
    t_config* config_metadata = config_create(metadata_path);
    
    if (config_metadata == NULL) {
        log_error(logger_entradasalida, "Error al abrir el archivo de metadata: %s", metadata_path);
        return (uint32_t)-1;
    }
    uint32_t bloque_inicial = atoi(config_get_string_value(config_metadata, "BLOQUE_INICIAL"));
    config_destroy(config_metadata);
    return bloque_inicial;
}

uint32_t obtener_ultimo_bloque(uint32_t bloque_inicial, uint32_t tamanio_actual) {
    uint32_t bloques_actuales = (uint32_t)ceil((double)tamanio_actual / (double)atoi(block_size));
    return bloque_inicial + bloques_actuales - 1;
}

void actualizar_metadata_tamanio(const char* metadata_path, uint32_t tamanio_nuevo) {
    t_config* config = config_create(metadata_path);
    
    if (config == NULL) {
        log_error(logger_entradasalida, "Error al abrir el archivo de metadata para actualizar");
        return;
    }

    char buffer[10];
    sprintf(buffer, "%u", tamanio_nuevo);
    config_set_value(config, "TAMANIO_ARCHIVO", buffer);

    if (!config_save(config)) {
        log_error(logger_entradasalida, "Error al guardar el archivo de metadata después de actualizar");
    }

    config_destroy(config);
}


