#include <../include/entradasalida.h>


// inicializacion
t_log *logger_entradasalida;
uint32_t conexion_kernel, conexion_memoria;

t_config *config_entradasalida;
char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;


int main(int argc, char* argv[]) {
    //decir_hola("una Interfaz de Entrada/Salida");

    logger_entradasalida = iniciar_logger("config/entradasalida.log", "ENTRADA_SALIDA", LOG_LEVEL_INFO);
    iniciar_config();

    conexion_kernel = crear_conexion(ip_kernel, puerto_kernel);
    send(conexion_kernel, "Handshake entrada salida y kernel",strlen("Handshake entrada salida y kernel") + 1,0);

    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    send(conexion_memoria, "Handshake entrada salida y memoria",strlen("Handshake entrada salida y memoria") + 1,0);

	terminar_programa(conexion_kernel ,logger_entradasalida, config_entradasalida);


    return 0;
}

void iniciar_config() {
    config_entradasalida = config_create("config/entradasalida.config");
    ip_kernel = config_get_string_value(config_entradasalida, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_entradasalida, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config_entradasalida, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_entradasalida, "PUERTO_MEMORIA");
}