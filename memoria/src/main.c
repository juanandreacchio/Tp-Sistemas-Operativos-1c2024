#include <../include/memoria.h>
// Definir variables globales
char *PUERTO_MEMORIA;
int TAM_MEMORIA;
int TAM_PAGINA;
char *PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;

pthread_mutex_t mutex;

t_log *logger_memoria;
t_config *config_memoria;

char *mensaje_recibido;
int socket_servidor_memoria;

t_list *procesos;

int main(int argc, char *argv[])
{
    phread_mutex_init(&mutex, NULL);
    // logger memoria

    // iniciar servidor
    iniciar_config();
    socket_servidor_memoria = iniciar_servidor(logger_memoria, PUERTO_MEMORIA, "MEMORIA");

    while (1)
    {
        pthread_t thread;
        int socket_cliente = esperar_cliente(socket_servidor_memoria, logger_memoria);
        pthread_mutex_lock(&mutex);
        pthread_create(&thread, NULL, atender_cliente, (void *)(long int)socket_cliente);
        pthread_detach(thread);
        pthread_mutex_unlock(&mutex);
    }

    terminar_programa(socket_servidor_memoria, logger_memoria, config_memoria);

    phread_mutex_destroy(&mutex);
    return 0;
}

void iniciar_config()
{
    config_memoria = config_create("config/memoria.config");
    logger_memoria = iniciar_logger("config/memoria.log", "MEMORIA", LOG_LEVEL_INFO);

    PUERTO_MEMORIA = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");
    TAM_MEMORIA = config_get_int_value(config_memoria, "TAM_MEMORIA");
    TAM_PAGINA = config_get_int_value(config_memoria, "TAM_PAGINA");
    PATH_INSTRUCCIONES = config_get_string_value(config_memoria, "PATH_INSTRUCCIONES");
    RETARDO_RESPUESTA = config_get_int_value(config_memoria, "RETARDO_RESPUESTA");
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
        break;
    case CPU:
        log_info(logger_memoria, "Se recibio un mensaje del modulo CPU");
        break;
    case ENTRADA_SALIDA:
        log_info(logger_memoria, "Se recibio un mensaje del modulo ENTRADA_SALIDA");
        break;
    case SOLICITUD_INSTRUCCION:
        log_info(logger_memoria, "Se recibio una solicitud de instruccion");
        t_paquete *paquete = recibir_paquete((int)(long int)socket_cliente);
        uint32_t pid, pc;
        memcpy(&pid, paquete->buffer->stream, sizeof(uint32_t));
        memcpy(&pc, paquete->buffer->stream + sizeof(uint32_t), sizeof(uint32_t));

        t_instruccion *instruccion = buscar_instruccion(procesos, pid, pc);
        t_paquete *paquete_respuesta = crear_paquete(INSTRUCCION);
        
        serializar_instruccion(paquete_respuesta, instruccion);
        enviar_paquete(paquete_respuesta, (int)(long int)socket_cliente);
        eliminar_paquete(paquete);
        eliminar_paquete(paquete_respuesta);

        liberar_instruccion(instruccion);
    break;
    case CREAR_PROCESO:
        log_info(logger_memoria, "Se recibio un mensaje para crear un proceso");
        t_paquete *paquete = recibir_paquete((int)(long int)socket_cliente);
        uint32_t pid;
        memcpy(&pid, paquete->buffer->stream, sizeof(uint32_t));
        char *path = malloc(paquete->buffer->size - sizeof(uint32_t));
        memcpy(path, paquete->buffer->stream + sizeof(uint32_t), paquete->buffer->size - sizeof(uint32_t));
        crear_proceso(procesos, pid, path);
        eliminar_paquete(paquete);
    break;
    default:
        log_info(logger_memoria, "Se recibio un mensaje de un modulo desconocido");
        // TODO: implementar
        break;
    }
    return NULL;
}

