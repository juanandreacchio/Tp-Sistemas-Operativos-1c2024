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

int main(void)
{
    iniciar_config();

    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    send(conexion_memoria, "Handshake kernel y memoria",strlen("Handshake kernel y memoria") + 1,0);
    
    conexion_cpu = crear_conexion(ip_cpu, puerto_dispatch);
    send(conexion_cpu, "Handshake kernel y puerto dispatch",strlen("Handshake kernel y cpu") + 1,0);

    conexion_cpu = crear_conexion(ip_cpu, puerto_interrupt);
    send(conexion_cpu, "Handshake kernel y puerto interrupt",strlen("Handshake kernel y cpu") + 1,0);

    socket_servidor_kernel = iniciar_servidor(logger_kernel, puerto_escucha, "Kernel");

    int conexion_kernel = esperar_cliente(socket_servidor_kernel);
    

    log_info(logger_kernel, "Se cerrará la conexión.");
    terminar_programa(socket_servidor_kernel, logger_kernel, config_kernel);
}

void iniciar_config(void)
{
    config_kernel = config_create("config/kernel.config");
    logger_kernel = iniciar_logger("config/kernel.log", "KERNEL", LOG_LEVEL_INFO);
    ip_memoria = config_get_string_value(config_kernel, "IP_MEMORIA");
    ip_cpu = config_get_string_value(config_kernel, "IP_CPU");
    puerto_dispatch = config_get_string_value(config_kernel, "PUERTO_DISPATCH");
    puerto_interrupt = config_get_string_value(config_kernel, "PUERTO_INTERRUPT");
    puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
    puerto_escucha = config_get_string_value(config_kernel, "PUERTO_ESCUCHA");
}