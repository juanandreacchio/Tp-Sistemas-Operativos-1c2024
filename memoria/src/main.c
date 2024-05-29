#include <../include/memoria.h>
#include <../include/mem.h>

// Definir variables globales
char *PUERTO_MEMORIA;
int TAM_MEMORIA;
int TAM_PAGINA;
char *PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;

int NUM_PAGINAS;

t_log *logger_memoria;
t_config *config_memoria;

int socket_servidor_memoria;

t_list *procesos_en_memoria;
pthread_mutex_t mutex;

//---------------------------variables globales-------------------------------------

int main(int argc, char *argv[])
{

    iniciar_config();
    iniciar_semaforos();
    inciar_listas();

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

void iniciar_semaforos()
{
    pthread_mutex_init(&mutex, NULL);
}

void inciar_listas()
{
    procesos_en_memoria = list_create();
}

void *atender_cliente(void *socket_cliente)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    while (1)
    {

        paquete = recibir_paquete((int)(long int)socket_cliente);

        if (paquete == NULL)
        {
            log_info(logger_memoria, "Se desconecto el cliente");
            break;
        }
        // uint32_t pid, pc;
        t_instruccion *instruccion = malloc(sizeof(t_instruccion));

        op_code codigo_operacion = paquete->codigo_operacion;
        t_buffer *buffer = paquete->buffer;
        printf("codigo de operacion: %s\n", cod_op_to_string(codigo_operacion));
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
            t_solicitudInstruccionEnMemoria *soli = malloc(sizeof(t_solicitudInstruccionEnMemoria));

            buffer_read(buffer, &(soli->pid), sizeof(uint32_t));
            buffer_read(buffer, &(soli->pc), sizeof(uint32_t));

            printf("Busco isntruccion para pid %d y pc %d\n", soli->pid, soli->pc);
            instruccion = buscar_instruccion(procesos_en_memoria, soli->pid, soli->pc);

            paquete = crear_paquete(INSTRUCCION);
            agregar_instruccion_a_paquete(paquete, instruccion);

            t_buffer *buffer_prueba = paquete->buffer;
            t_instruccion *inst = instruccion_deserializar(buffer_prueba, 0);

            enviar_paquete(paquete, (int)(long int)socket_cliente);
            // Probar mandar algo al cliente
            // sem_post(&semaforo);
            destruir_instruccion(instruccion);
            break;
        case CREAR_PROCESO:
            // char *path;

            t_solicitudCreacionProcesoEnMemoria *solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));

            solicitud = deserializar_solicitud_crear_proceso(buffer);
            log_info(logger_memoria, "Se recibio un mensaje para crear un proceso con pid %d y path %s", solicitud->pid, solicitud->path);
            t_proceso *proceso_creado = crear_proceso(procesos_en_memoria, solicitud->pid, solicitud->path);

            printf("--------------------------PROCESO CREADO-----------------\n");
            imprimir_proceso(proceso_creado);
            // imprimir_lista_de_procesos(procesos_en_memoria);

            break;
        default:
            log_info(logger_memoria, "Se recibio un mensaje de un modulo desconocido");
            // TODO: implementar
            break;
        }
        eliminar_paquete(paquete);
    }

    log_info(logger_memoria, "SALE DEL WHILE");
    return NULL;
}
