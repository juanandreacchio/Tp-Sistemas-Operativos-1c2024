#include <../include/memoria.h>

t_log *logger_memoria;
t_config *config_memoria;

char *puerto_memoria;
char *mensaje_recibido;
int socket_servidor_memoria;

int main(int argc, char *argv[])
{
    // logger memoria

    // iniciar servidor
    iniciar_config();
    socket_servidor_memoria = iniciar_servidor(logger_memoria, puerto_memoria, "MEMORIA");

    while (1)
    {
        pthread_t thread;
        int socket_cliente = esperar_cliente(socket_servidor_memoria, logger_memoria);
        pthread_create(&thread, NULL, atender_cliente, (void *)(long int)socket_cliente);
        pthread_detach(thread);
    }

    terminar_programa(socket_servidor_memoria, logger_memoria, config_memoria);

    return 0;
}

void iniciar_config()
{
    config_memoria = config_create("config/memoria.config");
    logger_memoria = iniciar_logger("config/memoria.log", "MEMORIA", LOG_LEVEL_INFO);
    puerto_memoria = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");
}

void iterator(char *value)
{
    log_info(logger_memoria, "%s", value);
}

void *atender_cliente(void *socket_cliente)
{
    op_code codigo_operacion = recibir_operacion((int)(long int)socket_cliente);
    switch (codigo_operacion)
    {
    case KERNEL:
        log_info(logger_memoria, "Se recibio un mensaje del modulo KERNEL");
        // TODO: implementar
        break;
    case CPU:
        log_info(logger_memoria, "Se recibio un mensaje del modulo CPU");
        // TODO: implementar
        break;
    case ENTRADA_SALIDA:
        log_info(logger_memoria, "Se recibio un mensaje del modulo ENTRADA_SALIDA");
        // TODO: implementar
        break;
    default:
        log_info(logger_memoria, "Se recibio un mensaje de un modulo desconocido");
        // TODO: implementar
        break;
    }
    return NULL;
}

