#include <../include/entradasalida.h>
char* path_archivo_bloques = "bloques.dat"; // Ruta del archivo de bloques
char* path_bitmap = "bitmap.dat"; // Ruta del archivo de bitmap
void levantarFileSystem(){
     
    create_archivos_bloques();
    crear_bitmap();
}


void crear_archivo_metadata(const char* filename, int initial_block, uint32_t tamanio_nombre_archivo) {
    size_t path_fs_tamanio = strlen(path_fs);
    uint32_t metadata_path_tamanio = path_fs_tamanio + tamanio_nombre_archivo + 1; // +1 por el /0
    char* metadata_path = malloc(metadata_path_tamanio * sizeof(char));
    
    // Copiar path_fs al buffer
    memcpy(metadata_path, path_fs, path_fs_tamanio);

    // Copiar filename al buffer
    memcpy(metadata_path + path_fs_tamanio, filename, tamanio_nombre_archivo);

    // Asegurar el terminador nulo al final
    metadata_path[metadata_path_tamanio - 1] = '\0';

    log_debug(logger_entradasalida, "Ruta de metadata: %s", metadata_path);

    FILE* file = fopen(metadata_path, "w");
    if (file == NULL) {
        log_error(logger_entradasalida, "Error al crear el archivo de metadata: %s", metadata_path);
        free(metadata_path);
        exit(EXIT_FAILURE);
    }
    fclose(file);

    t_config* config = config_create(metadata_path);
    if (!config) {
        log_error(logger_entradasalida, "Error al crear el archivo de configuración de metadata: %s", metadata_path);
        free(metadata_path);
        exit(EXIT_FAILURE);
    }

    char buffer[10];
    sprintf(buffer, "%u", initial_block);
    config_set_value(config, "BLOQUE_INICIAL",buffer);
    config_set_value(config, "TAMANIO_ARCHIVO", "0");
    
    if (!config_save(config)) {
        log_error(logger_entradasalida,"Error al guardar el archivo de metadata");
        config_destroy(config);
        exit(EXIT_FAILURE);
    }

    config_destroy(config);
    free(metadata_path);
}