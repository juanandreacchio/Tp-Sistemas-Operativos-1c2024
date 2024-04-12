#include <../include/kernel.h>

t_config *config_kernel;

t_log *logger_kernel;
char *ip_memoria;
char *ip_cpu;
char *puerto_memoria;
char *puerto_escucha;
char *puerto_dispatch;
char *puerto_interrupt;
char *ip;
u_int32_t conexion_memoria, conexion_dispatch, conexion_interrupt;
int socket_servidor_kernel, socket_cliente_kernel;

int main(void)
{
    iniciar_config();

    //iniciar conexion con Kernel
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_mensaje("", conexion_memoria, KERNEL);

    //iniciar conexion con CPU_dispath
    conexion_dispatch = crear_conexion(ip_cpu, puerto_dispatch);
    conexion_interrupt = crear_conexion(ip_cpu, puerto_interrupt);
    enviar_mensaje("", conexion_dispatch, KERNEL);
    enviar_mensaje("", conexion_interrupt, KERNEL);

    //iniciar Servidor
    socket_servidor_kernel = iniciar_servidor(logger_kernel, puerto_escucha, "Kernel");

    while (1)
    {
        pthread_t thread;
        int socket_cliente = esperar_cliente(socket_servidor_kernel, logger_kernel);
        pthread_create(&thread, NULL, atender_cliente, socket_cliente);
    }

    log_info(logger_kernel, "Se cerrará la conexión.");
    terminar_programa(socket_servidor_kernel, logger_kernel, config_kernel);
}

void iniciar_config(void)
{
    config_kernel = config_create("config/kernel.config");
    logger_kernel = iniciar_logger("config/kernel.log", "KERNEL", LOG_LEVEL_INFO);
    ip_memoria = config_get_string_value(config_kernel, "IP_MEMORIA");
    ip_cpu = config_get_string_value(config_kernel, "IP_CPU");
    puerto_dispatch = config_get_string_value(config_kernel, "PUERTO_CPU_DISPATCH");
    puerto_interrupt = config_get_string_value(config_kernel, "PUERTO_CPU_INTERRUPT");
    puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
    puerto_escucha = config_get_string_value(config_kernel, "PUERTO_ESCUCHA");
}

void* atender_cliente(int socket_cliente)
{
    op_code codigo_operacion = recibir_operacion(socket_cliente);
    switch (codigo_operacion)
    {
    case ENTRADA_SALIDA:
        log_info(logger_kernel, "Se recibio un mensaje del modulo ENTRADA_SALIDA");
        // TODO: implementar
        break;
    default:
        log_info(logger_kernel, "Se recibio un mensaje de un modulo desconocido");
        break;
    }
    return NULL;
}