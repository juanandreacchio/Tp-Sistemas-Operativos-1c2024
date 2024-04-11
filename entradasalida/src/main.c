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

    conexion_kernel = crear_conexion(ip_kernel, puerto_kernel);

    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    send(conexion_memoria, "IO", strlen("IO") + 1, 0);


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