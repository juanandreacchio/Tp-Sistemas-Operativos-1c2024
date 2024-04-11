#include <../include/memoria.h>

t_log *logger_memoria;
t_config *config_memoria;

char *puerto_memoria;
char *mensaje_recibido;
int socket_servidor_memoria;

int main(int argc, char *argv[])
{
    // logger memoria

    // servidor de cpu
    iniciar_config();
    socket_servidor_memoria = iniciar_servidor(logger_memoria, puerto_memoria, "MEMORIA");
    while (1)
    {
        pthread_t thread;
        int socket_cliente = esperar_cliente(socket_servidor_memoria, logger_memoria);
        char modulo_cliente[25];
        recv(socket_cliente, modulo_cliente, sizeof(modulo_cliente), 0);
        pthread_create(&thread,
                       NULL,
                       atender_cliente(modulo_cliente),
                       socket_cliente);
        pthread_detach(thread);
    }

    terminar_programa(socket_servidor_memoria, logger_memoria, config_memoria);

    return 0;
}

void iniciar_config()
{
    config_memoria = config_create("config/memoria.config");
    logger_memoria = iniciar_logger("config/memoria.log", "Memoria", LOG_LEVEL_INFO);
    puerto_memoria = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");
}

void iterator(char *value)
{
    log_info(logger_memoria, "%s", value);
}

void atender_cliente(char *modulo){
    if (strcmp(modulo, "Kernel") == 0) {
        log_info(logger_memoria, "Atendiendo cliente Kernel");
    } else if (strcmp(modulo, "CPU") == 0) {
        log_info(logger_memoria, "Atendiendo cliente CPU");
    } else if (strcmp(modulo, "IO") == 0) {
        log_info(logger_memoria, "Atendiendo cliente IO");
    }
}