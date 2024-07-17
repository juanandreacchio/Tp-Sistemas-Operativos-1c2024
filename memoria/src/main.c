#include <../include/memoria.h>
#include <../include/mem.h>

// Definir variables globales
char *PUERTO_MEMORIA;
int TAM_MEMORIA;
int TAM_PAGINA;
char *PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;

t_log *logger_memoria;
t_config *config_memoria;

int socket_servidor_memoria;

t_list *procesos_en_memoria;
pthread_mutex_t mutex;

t_bitarray *marcos_ocupados;
void *memoria_principal;

//---------------------------variables globales-------------------------------------

int main(int argc, char *argv[])
{

    iniciar_config();
    iniciar_semaforos();
    inciar_listas();

    socket_servidor_memoria = iniciar_servidor(logger_memoria, PUERTO_MEMORIA, "MEMORIA");
    inicializar_memoria_principal();

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
    liberar_memoria_principal();
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
        log_info(logger_memoria,"codigo de operacion: %s", op_code_to_string(codigo_operacion));

        // NO SE SI ESTO ESTA BIEN, CREO QUE DA LO MISMO DONDE SE PONGA
        usleep(RETARDO_RESPUESTA*1000);

        switch (codigo_operacion)
        {
        case KERNEL:
        {
            log_info(logger_memoria, "Se recibio un mensaje del modulo KERNEL");
            break;
        }
        case CPU:
        {
            log_info(logger_memoria, "Se recibio un mensaje del modulo CPU");
            enviar_tamanio_pagina((int)(long int)socket_cliente);
            break;
        }
        case ENTRADA_SALIDA:
        {
            log_info(logger_memoria, "Se recibio un mensaje del modulo ENTRADA_SALIDA");
            break;
        }
        case SOLICITUD_INSTRUCCION:
        {
            log_info(logger_memoria, "Se recibio una solicitud de instruccion");
            t_solicitudInstruccionEnMemoria *soli = malloc(sizeof(t_solicitudInstruccionEnMemoria));

            buffer_read(buffer, &(soli->pid), sizeof(uint32_t));
            buffer_read(buffer, &(soli->pc), sizeof(uint32_t));
            //  quiza hay que poner un semaforo para esperar a que las instruccione esten cargadas en el proceso
            instruccion = buscar_instruccion(procesos_en_memoria, soli->pid, soli->pc);

            paquete = crear_paquete(INSTRUCCION);
            agregar_instruccion_a_paquete(paquete, instruccion);

            // t_buffer *buffer_prueba = paquete->buffer;
            // t_instruccion *inst = instruccion_deserializar(buffer_prueba, 0);

            // enviar_paquete(paquete, (int)(long int)socket_cliente);
            enviar_paquete(paquete, (int)(long int)socket_cliente);
            // Probar mandar algo al cliente
            // sem_post(&semaforo);
            break;
        }
        case CREAR_PROCESO:
        {
            // char *path;

            t_solicitudCreacionProcesoEnMemoria *solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));

            solicitud = deserializar_solicitud_crear_proceso(buffer);
            log_info(logger_memoria, "Se recibio un mensaje para crear un proceso con pid %d y path %s", solicitud->pid, solicitud->path);
            t_proceso *proceso_creado = crear_proceso(procesos_en_memoria, solicitud->pid, solicitud->path);
            log_info(logger_memoria,"PID: %d - Tamaño: %d",solicitud->pid,list_size(proceso_creado->tabla_paginas));
            printf("--------------------------PROCESO CREADO-----------------\n");
            enviar_codigo_operacion(CREAR_PROCESO,(int)(long int)socket_cliente);
            break;
        }
        case END_PROCESS:
        {
            log_info(logger_memoria, "Se recibio un mensaje para finalizar un proceso");
            uint32_t pid;
            buffer_read(buffer, &pid, sizeof(uint32_t));
            // liberar el proceso y sacarlo de la lista
            t_proceso *proceso = buscar_proceso_por_pid(procesos_en_memoria, pid);
            log_info(logger_memoria,"PID: %d - Tamaño: %d",proceso->pid,list_size(proceso->tabla_paginas));
            liberar_proceso(proceso);
            list_remove(procesos_en_memoria, posicion_proceso(procesos_en_memoria, pid));
            
            break;
        }
        case ACCESO_TABLA_PAGINAS:
        {
            log_info(logger_memoria, "Se recibió un mensaje para acceder a la tabla de páginas");
            uint32_t pid, num_paginas;
            buffer_read(buffer, &pid, sizeof(uint32_t));
            buffer_read(buffer, &num_paginas, sizeof(uint32_t));

            t_proceso *proceso = buscar_proceso_por_pid(procesos_en_memoria, pid);
            t_paquete *paquete_respuesta = crear_paquete(ACCESO_TABLA_PAGINAS);

            for (uint32_t i = 0; i < num_paginas; i++)
            {
                uint32_t numero_pagina;
                buffer_read(buffer, &numero_pagina, sizeof(uint32_t));

                t_pagina *pag = list_get(proceso->tabla_paginas, numero_pagina);
                if (pag == NULL)
                {
                    log_error(logger_memoria, "Error: La página solicitada no existe en la tabla de páginas del proceso %d", pid);
                    uint32_t nro_marco_invalido = -1;
                    buffer_add(paquete_respuesta->buffer, &nro_marco_invalido, sizeof(uint32_t));
                }
                else
                {
                    uint32_t nro_marco = pag->numero_marco;
                    buffer_add(paquete_respuesta->buffer, &nro_marco, sizeof(uint32_t));
                    log_info(logger_memoria,"PID: %d - Pagina: %d - Marco: %d",pid,numero_pagina,nro_marco);
                }
                
            }

            enviar_paquete(paquete_respuesta, (int)(long int)socket_cliente);
            eliminar_paquete(paquete_respuesta);
            break;
        }
        case AJUSTAR_TAMANIO_PROCESO:
        {
            log_info(logger_memoria, "Se recibio un mensaje para ajustar el tamaño de un proceso");
            uint32_t pid, nuevo_tamanio;
            bool flag_break = false;
            buffer_read(buffer, &pid, sizeof(uint32_t));
            buffer_read(buffer, &nuevo_tamanio, sizeof(uint32_t));
            t_proceso *proceso = buscar_proceso_por_pid(procesos_en_memoria, pid);
            int tamanio_actual = list_size(proceso->tabla_paginas)*TAM_PAGINA;
            if (nuevo_tamanio > tamanio_actual)
            {
                // Ampliar el tamaño del proceso
                u_int32_t tam_a_ampliar = nuevo_tamanio - tamanio_actual;
                int paginas_a_agregar = (tam_a_ampliar+TAM_PAGINA-1)/TAM_PAGINA;
                for (int i = 0; i < paginas_a_agregar; i++)
                {
                    if (obtener_primer_marco_libre() == -1)
                    {
                        enviar_codigo_operacion(OUT_OF_MEMORY, (int)(long int)socket_cliente);
                        flag_break = true;
                        break;
                    }
                    t_pagina *pagina = inicializar_pagina(obtener_primer_marco_libre());
                    list_add(proceso->tabla_paginas, pagina);
                }
                log_info(logger_memoria,"PID: %d - Tamaño Actual: %d - Tamaño a Ampliar: %d",pid,tamanio_actual,tam_a_ampliar);
            }
            else if (nuevo_tamanio < tamanio_actual)
            {
                // Reducir el tamaño del proceso
                u_int32_t tam_a_reducir = tamanio_actual - nuevo_tamanio;
                int paginas_a_eliminar = tam_a_reducir/TAM_PAGINA;
                for (int i = 0; i < paginas_a_eliminar; i++)
                {
                    t_pagina *pagina = list_remove(proceso->tabla_paginas, list_size(proceso->tabla_paginas) - 1);
                    liberar_marco_pagina(proceso, pagina->numero_marco);
                    free(pagina);
                }
                log_info(logger_memoria,"PID: %d - Tamaño Actual: %d - Tamaño a Reducir: %d",pid,tamanio_actual,tam_a_reducir);
            }
            if(!flag_break){
            enviar_codigo_operacion(OK, (int)(long int)socket_cliente);
            log_info(logger_memoria,"mande el OK");
            }
            break;
        }
        case ESCRITURA_MEMORIA:
        {
            log_info(logger_memoria, "Se recibio un mensaje para escribir en memoria");
            // Llegan: cantidad de marcos a escribir, marco, offset, tamanio, buffer_escritura
            uint32_t cantidad_marcos;
            u_int32_t primer_direccion_fisica;
            u_int32_t tamanio_total;
            u_int32_t pid;
            buffer_read(buffer, &pid, sizeof(uint32_t));
            buffer_read(buffer, &cantidad_marcos, sizeof(uint32_t));
            for (int i = 0; i < cantidad_marcos; i++)
            {
                uint32_t direccion_fisica, tamanio;
                buffer_read(buffer, &direccion_fisica, sizeof(uint32_t));
                if(i==0)primer_direccion_fisica=direccion_fisica;
                buffer_read(buffer, &tamanio, sizeof(uint32_t));
                tamanio_total +=tamanio;
                void *buffer_escritura = malloc(tamanio);
                buffer_read(buffer, buffer_escritura, tamanio);
                memcpy(((char *)memoria_principal) + direccion_fisica, buffer_escritura, tamanio);
                free(buffer_escritura);
            }
            log_info(logger_memoria,"PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Tamaño %d",pid,primer_direccion_fisica,tamanio_total);//algo del tamanioo anda mal en esto
            enviar_codigo_operacion(OK,(int)(long int)socket_cliente);
            break;
        }
        case LECTURA_MEMORIA:
        {
            log_info(logger_memoria, "Se recibio un mensaje para leer de memoria");
            // Llegan: cantidad de marcos a leer, marco, offset, tamanio
            uint32_t cantidad_marcos,total_tamanio,primer_direccion_fisica,pid;
            buffer_read(buffer, &pid, sizeof(uint32_t));
            buffer_read(buffer, &cantidad_marcos, sizeof(uint32_t));
            buffer_read(buffer, &total_tamanio, sizeof(uint32_t));
            void *buffer_lectura = malloc(total_tamanio);
            size_t offset = 0;
            for (int i = 0; i < cantidad_marcos; i++)
            {
                uint32_t direccion_fisica, tamanio;
                buffer_read(buffer, &direccion_fisica, sizeof(uint32_t));
                if(i==0)primer_direccion_fisica = direccion_fisica;
                buffer_read(buffer, &tamanio, sizeof(uint32_t));
                memcpy(buffer_lectura + offset, ((char *)memoria_principal) + direccion_fisica, tamanio);
                offset += tamanio;
            }
            t_paquete *paquete_respuesta = crear_paquete(LECTURA_MEMORIA);
            buffer_add(paquete_respuesta->buffer, buffer_lectura, total_tamanio);
            log_info(logger_memoria,"PID: %d - Accion: LEER - Direccion fisica: %d - Tamaño %d",pid,primer_direccion_fisica,total_tamanio);
            enviar_paquete(paquete_respuesta, (int)(long int)socket_cliente);
            free(buffer_lectura);
            eliminar_paquete(paquete_respuesta);
            break;
        }
        case FINALIZAR_PROCESO:
        {
            uint32_t pid;
            buffer_read(paquete->buffer, &pid, sizeof(uint32_t));
            log_info(logger_memoria, "Se recibió un mensaje para finalizar un proceso");
            log_info(logger_memoria, "Elimino proceso con PID: %d", pid);
            liberar_proceso_por_pid(pid);
            break;
        }
        default:
        {
            log_info(logger_memoria, "Se recibio un mensaje de un modulo desconocido");
            // TODO: implementar
            break;
        }
        }
        eliminar_paquete(paquete);
    }

    log_info(logger_memoria, "SALE DEL WHILE");
    return NULL;
}
