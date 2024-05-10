#include <../include/memoria.h>
// Definir variables globales
char *PUERTO_MEMORIA;
int TAM_MEMORIA;
int TAM_PAGINA;
char *PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;

int NUM_PAGINAS;

pthread_mutex_t mutex;

t_log *logger_memoria;
t_config *config_memoria;

char *mensaje_recibido;
int socket_servidor_memoria;

t_list *procesos;

int main(int argc, char *argv[])
{
    pthread_mutex_init(&mutex, NULL);
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

    pthread_mutex_destroy(&mutex);
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

    NUM_PAGINAS = TAM_MEMORIA / TAM_PAGINA;
}

void iterator(char *value)
{
    log_info(logger_memoria, "%s", value);
}

void *atender_cliente(void *socket_cliente)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    uint32_t pid, pc;
    t_instruccion *instruccion;

    paquete = recibir_paquete(socket_cliente);

    // recv((int)(intptr_t)socket_cliente, &(paquete->codigo_operacion), sizeof(uint8_t), 0);
    // recv((int)(intptr_t)socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0);
    // paquete->buffer->stream = malloc(paquete->buffer->size);
    // recv((int)(intptr_t)socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

    op_code codigo_operacion = paquete->codigo_operacion;
    t_buffer *buffer = paquete->buffer;
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
        t_solicitudCreacionProcesoEnMemoria *soli = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));
        void *stream = buffer;

        memcpy(&pid, paquete->buffer->stream, sizeof(uint32_t));
        memcpy(&pc, paquete->buffer->stream + sizeof(uint32_t), sizeof(uint32_t));

        instruccion = buscar_instruccion(procesos, pid, pc);
        paquete = crear_paquete(INSTRUCCION);
        agregar_instruccion_a_paquete(paquete, instruccion);

        enviar_paquete(paquete, (int)(long int)socket_cliente);
        eliminar_paquete(paquete);

        destruir_instruccion(instruccion);
        break;
    case CREAR_PROCESO:
        char *path;
        log_info(logger_memoria, "Se recibio un mensaje para crear un proceso");
        t_buffer *buffer = paquete->buffer;
        t_solicitudCreacionProcesoEnMemoria *solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));

        void *bufferStream = buffer->stream;

        memcpy(&(solicitud->pid), bufferStream, sizeof(uint32_t));
        bufferStream += sizeof(uint32_t);
        memcpy(&(solicitud->path_length), bufferStream, sizeof(uint32_t));
        bufferStream += sizeof(uint32_t);
        solicitud->path = malloc(solicitud->path_length);
        memcpy(solicitud->path, bufferStream, solicitud->path_length);
        path = solicitud->path;
        pid=solicitud->pid;

            log_info(logger_memoria, "Se recibio un mensaje para crear un proceso con pid %d y path %s", solicitud->pid, path);
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
