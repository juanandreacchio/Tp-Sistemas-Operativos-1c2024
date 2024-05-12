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
    procesos= list_create();
    pthread_mutex_init(&mutex, NULL);
    // logger memoria

    // iniciar servidor
    iniciar_config();
    socket_servidor_memoria = iniciar_servidor(logger_memoria, PUERTO_MEMORIA, "MEMORIA");

    crear_proceso(procesos, 5, "test.txt");

    imprimir_proceso(list_get(procesos, 0));

    while (1)
    {
        pthread_t thread;
        int socket_cliente = esperar_cliente(socket_servidor_memoria, logger_memoria);
        pthread_mutex_lock(&mutex);
        pthread_create(&thread, NULL, atender_cliente, (void *)(long int)socket_cliente);
        pthread_detach(thread);

        log_info(logger_memoria, "DESCONECTO AL HILO");
        pthread_mutex_unlock(&mutex);
        log_info(logger_memoria, "DESBLOQUEO");
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
    
    while (1){
        paquete = recibir_paquete((int)(long int)socket_cliente);

        if (paquete == NULL) {
            log_info(logger_memoria, "Se desconecto el cliente");
            break;
        }
        uint32_t pid, pc;
        t_instruccion *instruccion = malloc(sizeof(t_instruccion));
        
        op_code codigo_operacion = paquete->codigo_operacion;
        t_buffer *buffer = paquete->buffer;
        printf("codigo de operacion: %d\n", codigo_operacion);
        switch (codigo_operacion){
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
                buffer_read(buffer,&(soli->pc), sizeof(uint32_t));
                
                printf("pid: %d\n", soli->pid);
                printf("pc: %d\n", soli->pc);
                instruccion = buscar_instruccion(procesos, soli->pid, soli->pc);

                log_info(logger_memoria, "Se envio la instruccion %d", instruccion->identificador);
    
                paquete = crear_paquete(INSTRUCCION);
                agregar_instruccion_a_paquete(paquete, instruccion);

                t_buffer *buffer_prueba = paquete->buffer;
                t_instruccion *inst = instruccion_deserializar(buffer_prueba, 0);
                
                printf("\nINSTRUCCION DESERIALIZADA:\n");
                imprimir_instruccion(*inst);
                
                send((uint32_t)socket_cliente, "hola", 5, 0);
                printf("\nSe envía el paquete con código de operacion %d\n", paquete->codigo_operacion);
                enviar_paquete(paquete, (uint32_t)socket_cliente);
                // Probar mandar algo al cliente

                destruir_instruccion(instruccion);
                break;
            case CREAR_PROCESO:
                char *path;

                t_solicitudCreacionProcesoEnMemoria *solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));

                solicitud = deserializar_solicitud_crear_proceso(buffer);
                log_info(logger_memoria, "Se recibio un mensaje para crear un proceso con pid %d y path %s", solicitud->pid, solicitud->path);
                crear_proceso(procesos, solicitud->pid, solicitud->path);

                printf("proceso creado\n");
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
