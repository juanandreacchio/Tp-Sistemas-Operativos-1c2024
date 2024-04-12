#include <../include/entradasalida.h>


// inicializacion
t_log *logger_entradasalida;
uint32_t conexion_kernel, conexion_memoria;

t_config *config_entradasalida;
char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;


int main(void) {
    iniciar_config();

    pthread_t hilo_kernel, hilo_memoria;

    pthread_create(&hilo_kernel, NULL, iniciar_conexion_kernel, NULL);
    pthread_create(&hilo_memoria, NULL, iniciar_conexion_memoria, NULL);

    pthread_join(hilo_kernel, NULL);
    pthread_join(hilo_memoria, NULL);



	terminar_programa(conexion_kernel ,logger_entradasalida, config_entradasalida);


    return 0;
}

void iniciar_config() {
    config_entradasalida = config_create("config/entradasalida.config");
    logger_entradasalida = iniciar_logger("config/entradasalida.log", "ENTRADA_SALIDA", LOG_LEVEL_INFO);
    ip_kernel = config_get_string_value(config_entradasalida, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_entradasalida, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config_entradasalida, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_entradasalida, "PUERTO_MEMORIA");
}

void* iniciar_conexion_kernel(void* arg) {
    conexion_kernel = crear_conexion(ip_kernel, puerto_kernel);
    enviar_mensaje("", conexion_kernel, ENTRADA_SALIDA);
    while(1) {
        // TODO: implementar
    }
    return NULL;
}

void* iniciar_conexion_memoria(void* arg) {
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_mensaje("", conexion_memoria, ENTRADA_SALIDA);
    while(1) {
        // TODO: implementar
    }
    return NULL;
}