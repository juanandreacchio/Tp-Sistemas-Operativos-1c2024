#include <../include/entradasalida.h>

// inicializacion
t_log *logger_entradasalida;
uint32_t conexion_kernel, conexion_memoria;

t_config *config_entradasalida;
char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;
pthread_t thread_memoria, thread_kernel;
t_interfaz *interfaz_generica_prueba;
t_interfaz *interfaz_generica_prueba2;

int main(void)
{
    iniciar_config();

    interfaz_generica_prueba = iniciar_interfaz("generica","config/entradasalida.config");

    pthread_create(&thread_kernel, NULL, iniciar_conexion_kernel, interfaz_generica_prueba);
    pthread_create(&thread_memoria, NULL, iniciar_conexion_memoria, NULL);

    pthread_join(thread_kernel, NULL);
    pthread_join(thread_memoria, NULL);

    terminar_programa(conexion_kernel, logger_entradasalida, config_entradasalida);

    return 0;
}

void iniciar_config()
{
    config_entradasalida = config_create("config/entradasalida.config");
    logger_entradasalida = iniciar_logger("config/entradasalida.log", "ENTRADA_SALIDA", LOG_LEVEL_INFO);
    ip_kernel = config_get_string_value(config_entradasalida, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_entradasalida, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config_entradasalida, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_entradasalida, "PUERTO_MEMORIA");
}

t_interfaz* iniciar_interfaz(char* nombre,char* ruta)
{
    t_interfaz* nueva_interfaz = malloc(sizeof(t_interfaz));
    nueva_interfaz->nombre = nombre;
    nueva_interfaz->ruta_archivo = ruta;
    return nueva_interfaz;
}

void *iniciar_conexion_kernel(void *arg)
{
    conexion_kernel = crear_conexion(ip_kernel, puerto_kernel, logger_entradasalida);
    enviar_mensaje(interfaz_generica_prueba->nombre , conexion_kernel, ENTRADA_SALIDA, logger_entradasalida);
    while (1)
    {
        // TODO: implementar
    }
    return NULL;
}

void *iniciar_conexion_memoria(void *arg)
{
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_entradasalida);
    enviar_mensaje("", conexion_memoria, ENTRADA_SALIDA, logger_entradasalida);
    while (1)
    {
        // TODO: implementar
    }
    return NULL;
}