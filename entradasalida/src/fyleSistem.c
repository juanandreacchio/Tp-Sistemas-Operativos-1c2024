#include <../include/entradasalida.h>
char* path_archivo_bloques = "bloques.dat"; // Ruta del archivo de bloques
char* path_bitmap = "bitmap.dat"; // Ruta del archivo de bitmap
void levantarFileSystem(){
     
    create_archivos_bloques();
    crear_bitmap();
}


void crear_archivo_metadata(const char* filename, int initial_block) {
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_fs, filename);

    t_config* config = config_create(metadata_path);
    if (!config) {
        log_error(logger_entradasalida,"Error al crear el archivo de metadata");
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
}