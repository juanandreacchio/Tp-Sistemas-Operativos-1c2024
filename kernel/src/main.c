#include <../include/kernel.h>

t_config *config_kernel;

t_log *logger_kernel;
char *ip_memoria;
char *ip_cpu;
char *puerto_memoria;
char *puerto_escucha;
char *puerto_dispatch;
char *puerto_interrupt;
char *algoritmo_planificacion;
char *ip;
u_int32_t conexion_memoria, conexion_dispatch, conexion_interrupt;
int socket_servidor_kernel, socket_cliente_kernel;
int contador_pcbs, identificador_pid = 1;
int grado_multiprogramacion;
uint32_t contador_pid;
int quantum;

t_dictionary *conexiones_io;

pthread_mutex_t mutex_pid;
pthread_mutex_t mutex_cola_de_readys;
pthread_mutex_t mutex_cola_de_blocked;
pthread_mutex_t mutex_cola_de_new;
pthread_mutex_t mutex_proceso_en_ejecucion;
pthread_mutex_t mutex_interfaces_conectadas;
sem_t contador_grado_multiprogramacion;
sem_t hay_proceso_a_ready;
sem_t cpu_libre;
t_queue *lista_procesos_ready;
t_queue *lista_procesos_new;
t_list *lista_procesos_blocked;
t_pcb *pcb_en_ejecucion;
pthread_t planificador_corto;
pthread_t dispatch;

//-----------------varaibles globales------------------
int grado_multiprogramacion;

int main(void)
{
    iniciar_config();
    lista_procesos_ready = queue_create();
    lista_procesos_new = queue_create();
    colas_blocked = dictionary_create();
    sem_init(&contador_grado_multiprogramacion, 0, grado_multiprogramacion);
    sem_init(&hay_proceso_a_ready, 0, 0);
    sem_init(&cpu_libre, 0, 1);
    // iniciar conexion con Kernel
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_kernel);
    enviar_mensaje("", conexion_memoria, KERNEL, logger_kernel);

    // iniciar conexion con CPU Dispatch e Interrupt
    conexion_dispatch = crear_conexion(ip_cpu, puerto_dispatch, logger_kernel);
    conexion_interrupt = crear_conexion(ip_cpu, puerto_interrupt, logger_kernel);
    enviar_mensaje("", conexion_dispatch, KERNEL, logger_kernel);
    enviar_mensaje("", conexion_interrupt, KERNEL, logger_kernel);
    // enviar_codigo_operacion(INTERRUPTION,conexion_interrupt);

    // iniciar Servidor
    socket_servidor_kernel = iniciar_servidor(logger_kernel, puerto_escucha, "KERNEL");

    pthread_create(&dispatch, NULL, (void *)recibir_dispatch, NULL);
    pthread_detach(dispatch);

    pthread_create(&planificador_corto, NULL, (void *)planificar_run, NULL);
    pthread_detach(planificador_corto);

    pthread_t thread_consola;
    pthread_create(&thread_consola, NULL, (void *)iniciar_consola_interactiva, NULL);
    pthread_detach(thread_consola);

    while (1)
    {
        pthread_t thread;
        int *socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(socket_servidor_kernel, logger_kernel);
        pthread_create(&thread, NULL, (void *)atender_cliente, socket_cliente);
        pthread_detach(thread);
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
    quantum = config_get_int_value(config_kernel, "QUANTUM");
    algoritmo_planificacion = config_get_int_value(config_kernel, "ALGORITMO_PLANIFICACION");
    grado_multiprogramacion = config_get_int_value(config_kernel, "GRADO_MULTIPROGRAMACION");
}

void *atender_cliente(void *socket_cliente_ptr)
{
    int socket_cliente = *(int *)socket_cliente_ptr;
    free(socket_cliente_ptr); // Liberamos el puntero ya que no lo necesitamos más

    while (1)
    {
        op_code codigo_operacion = recibir_operacion(socket_cliente);

        switch (codigo_operacion)
        {
        case INTERFAZ_GENERICA:
            char *nombre_entrada_salida = recibir_mensaje_guardar_variable(socket_cliente);
            log_info(logger_kernel, "Se conectó la I/O llamada: %s", nombre_entrada_salida);

            t_interfaz_en_kernel *interfaz = malloc(sizeof(t_interfaz_en_kernel));
            interfaz->conexion = socket_cliente;
            interfaz->tipo_interfaz = cod_op_to_cod_interfaz(INTERFAZ_GENERICA);

            pthread_mutex_lock(&mutex_interfaces_conectadas);
            dictionary_put(conexiones_io, nombre_entrada_salida, interfaz); // Guardar el socket como entero
            pthread_mutex_unlock(&mutex_interfaces_conectadas);

            // dictionary_put(colas_blocked, nombre_entrada_salida, crear_cola_de_bloqueados_de_interfaz());
            free(nombre_entrada_salida);
            break;
        case INTERFAZ_STDIN:
            char *nombre_entrada_salida = recibir_mensaje_guardar_variable(socket_cliente);
            log_info(logger_kernel, "Se conectó la I/O llamada: %s", nombre_entrada_salida);

            t_interfaz_en_kernel *interfaz = malloc(sizeof(t_interfaz_en_kernel));
            interfaz->conexion = socket_cliente;
            interfaz->tipo_interfaz = cod_op_to_cod_interfaz(INTERFAZ_STDIN);

            pthread_mutex_lock(&mutex_interfaces_conectadas);
            dictionary_put(conexiones_io, nombre_entrada_salida, interfaz); // Guardar el socket como entero
            pthread_mutex_unlock(&mutex_interfaces_conectadas);

            // dictionary_put(colas_blocked, nombre_entrada_salida, crear_cola_de_bloqueados_de_interfaz());
            free(nombre_entrada_salida);
            break;
        case INTERFAZ_STDOUT:
            char *nombre_entrada_salida = recibir_mensaje_guardar_variable(socket_cliente);
            log_info(logger_kernel, "Se conectó la I/O llamada: %s", nombre_entrada_salida);

            t_interfaz_en_kernel *interfaz = malloc(sizeof(t_interfaz_en_kernel));
            interfaz->conexion = socket_cliente;
            interfaz->tipo_interfaz = cod_op_to_cod_interfaz(INTERFAZ_STDOUT);

            pthread_mutex_lock(&mutex_interfaces_conectadas);
            dictionary_put(conexiones_io, nombre_entrada_salida, interfaz); // Guardar el socket como entero
            pthread_mutex_unlock(&mutex_interfaces_conectadas);

            // dictionary_put(colas_blocked, nombre_entrada_salida, crear_cola_de_bloqueados_de_interfaz());
            free(nombre_entrada_salida);
            break;
        case INTERFAZ_DIALFS:
            char *nombre_entrada_salida = recibir_mensaje_guardar_variable(socket_cliente);
            log_info(logger_kernel, "Se conectó la I/O llamada: %s", nombre_entrada_salida);

            t_interfaz_en_kernel *interfaz = malloc(sizeof(t_interfaz_en_kernel));
            interfaz->conexion = socket_cliente;
            interfaz->tipo_interfaz = cod_op_to_cod_interfaz(INTERFAZ_DIALFS);

            pthread_mutex_lock(&mutex_interfaces_conectadas);
            dictionary_put(conexiones_io, nombre_entrada_salida, interfaz); // Guardar el socket como entero
            pthread_mutex_unlock(&mutex_interfaces_conectadas);

            // dictionary_put(colas_blocked, nombre_entrada_salida, crear_cola_de_bloqueados_de_interfaz());
            free(nombre_entrada_salida);
            break;
        default:
            log_info(logger_kernel, "Se recibió un mensaje de un módulo desconocido");
            break;
        }
    }

    close(socket_cliente); // Cerramos el socket una vez que salimos del loop
    return NULL;
}

//----------------------CONSOLA INTERACTIVA-----------------------------

void *iniciar_consola_interactiva()
{
    char *comandoRecibido;
    comandoRecibido = readline("> ");

    while (strcmp(comandoRecibido, "") != 0)
    {
        if (!validar_comando(comandoRecibido))
        {
            log_error(logger_kernel, "Comando de CONSOLA no reconocido");
            free(comandoRecibido);
            comandoRecibido = readline("> ");
            continue; // se saltea el resto del while y vuelve a empezar
        }

        ejecutar_comando(comandoRecibido);
        free(comandoRecibido);
        comandoRecibido = readline("> ");
    }

    free(comandoRecibido);
    return NULL;
}

bool validar_comando(char *comando)
{
    bool validacion = false;
    char **consola = string_split(comando, " ");
    if (strcmp(consola[0], "EJECUTAR_SCRIPT") == 0)
        validacion = true;
    else if (strcmp(consola[0], "INICIAR_PROCESO") == 0)
        validacion = true;
    else if (strcmp(consola[0], "FINALIZAR_PROCESO") == 0)
        validacion = true;
    else if (strcmp(consola[0], "DETENER_PLANIFICACION") == 0)
        validacion = true;
    else if (strcmp(consola[0], "INICIAR_PLANIFICACION") == 0)
        validacion = true;
    else if (strcmp(consola[0], "MULTIPROGRAMACION") == 0)
        validacion = true;
    else if (strcmp(consola[0], "PROCESO_ESTADO") == 0)
        validacion = true;

    string_array_destroy(consola);
    return validacion;
}

void ejecutar_comando(char *comando)
{
    char **consola = string_split(comando, " ");
    t_buffer *buffer = crear_buffer();

    if (strcmp(consola[0], "EJECUTAR_SCRIPT") == 0)
    {
    }
    else if (strcmp(consola[0], "INICIAR_PROCESO") == 0)
    {
        char *path = consola[1];
        // ---------------------- CREAR PCB EN EL KERNEL -------------------

        t_pcb *pcb_creado = crear_pcb(contador_pid, quantum, NEW);

        // ---------------------- ENVIAR SOLICITUD PARA QUE SE CREE EN MEMORIA -------------------
        t_paquete *paquete = crear_paquete(CREAR_PROCESO);
        t_solicitudCreacionProcesoEnMemoria *ptr_solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));
        ptr_solicitud->pid = contador_pid;
        ptr_solicitud->path_length = strlen(path) + 1;
        ptr_solicitud->path = path;
        t_buffer *buffer = serializar_solicitud_crear_proceso(ptr_solicitud);
        paquete->buffer = buffer;

        enviar_paquete(paquete, conexion_memoria);
        // tengo mis dudas sobre si hay que suar una cola de new proque no c si podemos tener mas de uno en new
        set_add_pcb_cola(pcb_creado, NEW, lista_procesos_new, mutex_cola_de_new);

        pthread_mutex_lock(&mutex_pid);
        contador_pid++;
        pthread_mutex_unlock(&mutex_pid);

        // estoy en duda si hay que ponerle semaforo a esto
        sem_wait(&contador_grado_multiprogramacion);
        if (!queue_is_empty(lista_procesos_new)) // tengo mis dudas sobre esto poruqe siemrpe quen llegue aca va a tener algo creo
        {
            pthread_mutex_lock(&mutex_cola_de_new); //por ahi esta de mas
            t_pcb *pcb_ready = queue_pop(lista_procesos_new);
            pthread_mutex_unlock(&mutex_cola_de_new);

            set_add_pcb_cola(pcb_ready, READY, lista_procesos_ready, mutex_cola_de_readys);
        }
        sem_post(&hay_proceso_a_ready);

        eliminar_paquete(paquete);
    }
    else if (strcmp(consola[0], "FINALIZAR_PROCESO") == 0)
    {
        buffer_add(buffer, consola[1], string_length(consola[1]) + 1); //[pid]
        // finalizar_proceso(buffer);
    }
    else if (strcmp(consola[0], "MULTIPROGRAMACION") == 0)
    {
        buffer_add(buffer, consola[1], string_length(consola[1]) + 1); //[valor]
        // multiprogramacion(buffer);
    }
    else if (strcmp(consola[0], "PROCESO_ESTADO") == 0)
    {
    }
    // proceso_estado();
    else if (strcmp(consola[0], "DETENER_PLANIFICACION") == 0)
    {
    }
    // detener_planificacion();
    else if (strcmp(consola[0], "INICIAR_PLANIFICACION") == 0)
    {
    }
    // iniciar_planificacion();
    else
    {
        log_error(logger_kernel, "comando no reconocido, a pesar de que entro al while");
        exit(EXIT_FAILURE);
    }
    string_array_destroy(consola);
}

void crear_cola_de_bloqueados_de_interfaz()
{
    t_cola_bloqueados_io *cola_bloqueados_io = malloc(sizeof(t_cola_bloqueados_io));
    cola_bloqueados_io->lista_instrucciones = list_create();
    return cola_bloqueados_io;
}

// Planificacion

void planificar_run()
{
    while (1)
    {
        sem_wait(&hay_proceso_a_ready);
        sem_wait(&cpu_libre);

        if (strcmp(algoritmo_planificacion, "FIFO") == 0)
        {
            log_info(logger_kernel, "algoritmo de planificacion: FIFO");

            pthread_mutex_lock(&mutex_cola_de_readys);
            t_pcb *pcb_a_ejecutar = queue_pop(lista_procesos_ready);
            pthread_mutex_unlock(&mutex_cola_de_readys);

            ejecutar_PCB(pcb_a_ejecutar);
        }
        else if (strcmp(config_kernel->ALGORITMO_PLANIFICACION, "RR") == 0)
        {
            log_info(logger_kernel, "algoritmo de planificacion: RR");

            pthread_mutex_lock(&mutex_cola_de_readys);
            t_pcb *pcb_a_ejecutar = queue_pop(lista_procesos_ready);
            pthread_mutex_unlock(&mutex_cola_de_readys);

            ejecutar_PCB(pcb_a_ejecutar);

            sem_post(&sem_beggin_quantum);
        }
        else
        {
            log_error(logger_kernel, "Algoritmo invalido");
        }
    }
}

void ejecutar_PCB(t_pcb *pcb)
{
    setear_pcb_en_ejecucion(pcb);
    enviar_pcb(pcb, conexion_dispatch);
    log_info(logger_kernel, "Se envio el PCB con PID %d a CPU Dispatch", pcb->pid);
    /* lo ahcemos desde el hilo de dispatch
    MOTIVO_DESALOJO motivo = recibir_motivo_desalojo(conexion_dispatch);
    t_pcb *pcb_actualizado = recibir_pcb(conexion_dispatch);
    */

    destruir_pcb(pcb);
}

void setear_pcb_en_ejecucion(t_pcb *pcb)
{
    pcb->estado_actual = EXEC;

    pthread_mutex_lock(&mutex_proceso_en_ejecucion);
    pcb_en_ejecucion = pcb;
    pthread_mutex_unlock(&mutex_proceso_en_ejecucion);
}
void set_add_pcb_cola(t_pcb *pcb, estados estado, t_queue *cola, pthread_mutex_t mutex)
{
    pcb->estado_actual = estado;

    pthread_mutex_lock(&mutex);
    queue_push(cola, pcb);
    pthread_mutex_unlock(&mutex);
}

void *recibir_dispatch()
{
    while (1)
    {
        MOTIVODESALOJO motivo_desalojo = recibir_motivo_desalojo(conexion_dispatch);
        t_pcb *pcb_actualizado = recibir_pcb(conexion_dispatch);

        sem_post(&cpu_libre);

        switch (motivo_desalojo)
        {
        case OPERACION_IO:
            t_paquete *respuesta_kernel = recibir_paquete(conexion_dispatch);
            t_instruccion *utlima_instruccion = instruccion_deserializar(respuesta_kernel->buffer, 0);

            char *nombre_io = utlima_instruccion->parametros[0];
            if (!interfaz_conectada(nombre_io))
            {
                log_error(logger_kernel, "La interfaz %s no está conectada", nombre_io);
                exit(EXIT_FAILURE);
            }
            t_interfaz_en_kernel *interfaz = dictionary_get(conexiones_io, nombre_io);
            if (!esOperacionValida(utlima_instruccion->identificador, interfaz->tipo_interfaz))
            {
                log_error(logger_kernel, "La operacion %d no es valida para la interfaz %s", utlima_instruccion->identificador, nombre_io);
                exit(EXIT_FAILURE);
            }

            pthread_mutex_lock(&mutex_proceso_en_ejecucion);
            pcb_en_ejecucion = NULL;
            pthread_mutex_unlock(&mutex_proceso_en_ejecucion);

            pcb_actualizado->estado_actual = BLOCKED;

            sem_post(&contador_grado_multiprogramacion);

            pthread_mutex_lock(&mutex_cola_de_blocked);r
            list_add(colas_blocked, pcb_actualizado);
            pthread_mutex_unlock(&mutex_cola_de_blocked);



            t_paquete *paquete = crear_paquete(NULL);
            paquete->buffer = serializar_instruccion(instruccion);
            enviar_paquete(paquete, conexion_kernel_dispatch);
            break;
        case FIN_CLOCK:
            break;
        case END_PROCESS:
            break;

        default:
            break;
        }

        // set_add_pcb_cola(pcb, READY, lista_procesos_ready, mutex_cola_de_readys);
        sem_post(&hay_proceso_a_ready);
    }
}

bool interfaz_conectada(char *nombre_interfaz)
{
    /*
    int conexion_io = (int)(intptr_t)dictionary_get(conexiones_io, nombre_interfaz);

    if(recv(conexion_io, NULL, 0, MSG_PEEK) == 0)
    {
        return false;
    } else{
        return true;
    }
    FALTA VER BIEN PARTE DE SI SIGUEN CONECTADOS */
    return dictionary_has_key(conexiones_io, nombre_interfaz);
}

bool esOperacionValida(t_identificador identificador, cod_interfaz tipo)
{
    switch (tipo)
    {
    case GENERICA:
        return identificador == IO_GEN_SLEEP;
    case STDIN:
        return identificador == IO_STDIN_READ;
    case STDOUT:
        return identificador == IO_STDOUT_WRITE;
    case DIALFS:
        return identificador == IO_FS_TRUNCATE || identificador == IO_FS_READ || identificador == IO_FS_WRITE || identificador == IO_FS_CREATE || identificador == IO_FS_DELETE;
    default:
        return false;
    }
}

void cod_op_to_cod_interfaz(op_code codigo_operacion)
{
    switch (codigo_operacion)
    {
    case INTERFAZ_GENERICA:
        return GENERICA;
    case INTERFAZ_STDIN:
        return STDIN;
    case INTERFAZ_STDOUT:
        return STDOUT;
    case INTERFAZ_DIALFS:
        return DIALFS;
    default:
        return -1;
    }
}