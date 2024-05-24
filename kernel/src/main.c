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
u_int32_t conexion_memoria, conexion_dispatch, conexion_interrupt, flag_cpu_libre;
int socket_servidor_kernel;
int contador_pcbs;
uint32_t contador_pid;
int quantum;
char *nombre_entrada_salida_conectada;

t_dictionary *conexiones_io;
t_dictionary *colas_blocks_io; // cola de cada interfaz individual, adentro están las isntrucciones a ejecutar
t_dictionary *diccionario_semaforos_io;

pthread_mutex_t mutex_pid;
pthread_mutex_t mutex_cola_de_readys;
pthread_mutex_t mutex_lista_de_blocked;
pthread_mutex_t mutex_cola_de_new;
pthread_mutex_t mutex_proceso_en_ejecucion;
pthread_mutex_t mutex_interfaces_conectadas;
pthread_mutex_t mutex_cola_interfaces; // PARA AGREGAR AL DICCIOANRIO de la cola de cada interfaz
pthread_mutex_t mutex_diccionario_interfaces_de_semaforos;
pthread_mutex_t mutex_flag_cpu_libre;
sem_t contador_grado_multiprogramacion, hay_proceso_a_ready, cpu_libre, arrancar_quantum;
t_queue *cola_procesos_ready;
t_queue *cola_procesos_new;
t_list *lista_procesos_blocked; // lsita de los prccesos bloqueados
t_pcb *pcb_en_ejecucion;
pthread_t planificador_corto;
pthread_t dispatch;
pthread_t hilo_quantum;

//-----------------varaibles globales------------------
int grado_multiprogramacion;

int main(void)
{
    iniciar_config();
    iniciar_listas();
    iniciar_colas_de_estados_procesos();

    iniciar_semaforos(); // TODO

    // iniciar conexion con Kernel
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_kernel);
    enviar_mensaje("", conexion_memoria, KERNEL, logger_kernel);

    // iniciar conexion con CPU Dispatch e Interrupt
    conexion_dispatch = crear_conexion(ip_cpu, puerto_dispatch, logger_kernel);
    conexion_interrupt = crear_conexion(ip_cpu, puerto_interrupt, logger_kernel);
    enviar_mensaje("", conexion_dispatch, KERNEL, logger_kernel);
    enviar_mensaje("", conexion_interrupt, KERNEL, logger_kernel);

    // iniciar Servidor
    socket_servidor_kernel = iniciar_servidor(logger_kernel, puerto_escucha, "KERNEL");

    pthread_create(&dispatch, NULL, (void *)recibir_dispatch, NULL);
    pthread_detach(dispatch);

    pthread_create(&planificador_corto, NULL, (void *)planificar_run, NULL);
    pthread_detach(planificador_corto);

    pthread_t thread_consola;
    pthread_create(&thread_consola, NULL, (void *)iniciar_consola_interactiva, NULL);
    pthread_detach(thread_consola);

    if (strcmp(algoritmo_planificacion, "RR") == 0)
    {
        log_info(logger_kernel, "iniciando quantum");
        pthread_create(&hilo_quantum, NULL, (void *)verificar_quantum, NULL);
        pthread_detach(hilo_quantum);
    }

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
    algoritmo_planificacion = config_get_string_value(config_kernel, "ALGORITMO_PLANIFICACION");
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
            nombre_entrada_salida_conectada = recibir_mensaje_guardar_variable(socket_cliente);

            crear_interfaz(INTERFAZ_GENERICA, nombre_entrada_salida_conectada, socket_cliente);

            free(nombre_entrada_salida_conectada);
            break;
        case INTERFAZ_STDIN:
            nombre_entrada_salida_conectada = recibir_mensaje_guardar_variable(socket_cliente);

            crear_interfaz(INTERFAZ_STDIN, nombre_entrada_salida_conectada, socket_cliente);

            free(nombre_entrada_salida_conectada);
            break;
        case INTERFAZ_STDOUT:
            nombre_entrada_salida_conectada = recibir_mensaje_guardar_variable(socket_cliente);

            crear_interfaz(INTERFAZ_STDOUT, nombre_entrada_salida_conectada, socket_cliente);

            free(nombre_entrada_salida_conectada);
            break;
        case INTERFAZ_DIALFS:
            nombre_entrada_salida_conectada = recibir_mensaje_guardar_variable(socket_cliente);

            crear_interfaz(INTERFAZ_DIALFS, nombre_entrada_salida_conectada, socket_cliente);

            free(nombre_entrada_salida_conectada);
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
        set_add_pcb_cola(pcb_creado, NEW, cola_procesos_new, mutex_cola_de_new);

        pthread_mutex_lock(&mutex_pid);
        contador_pid++;
        pthread_mutex_unlock(&mutex_pid);

        // estoy en duda si hay que ponerle semaforo a esto
        sem_wait(&contador_grado_multiprogramacion);
        if (!queue_is_empty(cola_procesos_new)) // tengo mis dudas sobre esto poruqe siemrpe quen llegue aca va a tener algo creo
        {
            pthread_mutex_lock(&mutex_cola_de_new); // por ahi esta de mas
            t_pcb *pcb_ready = queue_pop(cola_procesos_new);
            pthread_mutex_unlock(&mutex_cola_de_new);

            set_add_pcb_cola(pcb_ready, READY, cola_procesos_ready, mutex_cola_de_readys);
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
            t_pcb *pcb_a_ejecutar = queue_pop(cola_procesos_ready);
            pthread_mutex_unlock(&mutex_cola_de_readys);

            ejecutar_PCB(pcb_a_ejecutar);
        }
        else if (strcmp(algoritmo_planificacion, "RR") == 0)
        {
            log_info(logger_kernel, "algoritmo de planificacion: RR");

            pthread_mutex_lock(&mutex_cola_de_readys);
            t_pcb *pcb_a_ejecutar = queue_pop(cola_procesos_ready);
            pthread_mutex_unlock(&mutex_cola_de_readys);

            ejecutar_PCB(pcb_a_ejecutar);

            sem_post(&arrancar_quantum);
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
        op_code motivo_desalojo = recibir_motivo_desalojo(conexion_dispatch);
        t_pcb *pcb_actualizado = recibir_pcb(conexion_dispatch);

        flag_cpu_libre = 1; // no c si tendir aque ir aca o en cada uno de los casos del motivo de desalojo que lo requieran
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
            /* creo que esto no es necesario
            pthread_mutex_lock(&mutex_proceso_en_ejecucion);
            pcb_en_ejecucion = NULL;
            pthread_mutex_unlock(&mutex_proceso_en_ejecucion);
            */

            pcb_actualizado->estado_actual = BLOCKED;

            pthread_mutex_lock(&mutex_lista_de_blocked);
            list_add(lista_procesos_blocked, pcb_actualizado);
            pthread_mutex_unlock(&mutex_lista_de_blocked);

            sem_post(&contador_grado_multiprogramacion);

            // 1. La agregamos a la cola de blocks io. Ultima instrucción y PID
            t_instruccionEnIo *instruccion_en_io = malloc(sizeof(t_instruccionEnIo));
            instruccion_en_io->instruccion_io = utlima_instruccion;
            instruccion_en_io->pid = pcb_actualizado->pid;

            pthread_mutex_lock(&mutex_lista_de_blocked);
            queue_push((t_queue *)dictionary_get(colas_blocks_io, nombre_io), instruccion_en_io);
            pthread_mutex_unlock(&mutex_lista_de_blocked);

            t_semaforosIO *semaforos_interfaz = dictionary_get(diccionario_semaforos_io, nombre_io);
            sem_post(&semaforos_interfaz->instruccion_en_cola);

            // 3. sino nose
            break;
        case FIN_CLOCK:
            set_add_pcb_cola(pcb_actualizado, READY, cola_procesos_ready, mutex_cola_de_readys);
            break;
        case KILL_PROCESS:
            // TODO
            break;
        case END_PROCESS:
            sem_post(&contador_grado_multiprogramacion);
            destruir_pcb(pcb_actualizado);
            break;

        default:
            break;
        }
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

void crear_interfaz(op_code tipo, char *nombre, uint32_t conexion)
{
    log_info(logger_kernel, "Se conectó la I/O llamada: %s", nombre);

    t_interfaz_en_kernel *interfaz = malloc(sizeof(t_interfaz_en_kernel));
    interfaz->conexion = conexion;
    interfaz->tipo_interfaz = cod_op_to_tipo_interfaz(tipo); // hacer cod_op_to_cod_interfaz

    pthread_mutex_lock(&mutex_interfaces_conectadas);
    dictionary_put(conexiones_io, nombre, interfaz); // Guardar el socket como entero
    pthread_mutex_unlock(&mutex_interfaces_conectadas);

    pthread_mutex_lock(&mutex_cola_interfaces);
    dictionary_put(colas_blocks_io, nombre, queue_create());
    pthread_mutex_unlock(&mutex_cola_interfaces);

    t_semaforosIO *semaforos_interfaz = malloc(sizeof(t_semaforosIO));
    pthread_mutex_init(&semaforos_interfaz->mutex, NULL);
    sem_init(&semaforos_interfaz->instruccion_en_cola, 0, 0);
    sem_init(&semaforos_interfaz->binario_io_libre, 0, 1);

    pthread_mutex_lock(&mutex_diccionario_interfaces_de_semaforos);
    dictionary_put(diccionario_semaforos_io, nombre,semaforos_interfaz);
    pthread_mutex_unlock(&mutex_diccionario_interfaces_de_semaforos);
}

void ejecutar_instruccion_io(char *nombre_interfaz, t_instruccionEnIo *instruccionEnIO)
{
    uint32_t conexion_io = *(uint32_t *)dictionary_get(conexiones_io, nombre_interfaz);

    t_paquete *paquete = crear_paquete(EJECUTAR_IO);
    paquete->buffer = serializar_instruccion_en_io(instruccionEnIO);
    enviar_paquete(paquete, conexion_io);

    op_code resultado_operacion = recibir_operacion(conexion_io);
    if (resultado_operacion == IO_SUCCESS)
    {
        t_pcb *pcb = buscar_pcb_por_pid(instruccionEnIO->pid, lista_procesos_blocked);

        pthread_mutex_lock(&mutex_lista_de_blocked);
        list_remove_element(lista_procesos_blocked, pcb);
        pthread_mutex_unlock(&mutex_lista_de_blocked);

        if (pcb != NULL)
        {
            sem_wait(&contador_grado_multiprogramacion);
            set_add_pcb_cola(pcb, READY, cola_procesos_ready, mutex_cola_de_readys);
        }
    }

    t_semaforosIO *semaforos_interfaz = dictionary_get(diccionario_semaforos_io, nombre_interfaz);
    sem_post(&semaforos_interfaz->binario_io_libre);
}

void atender_interfaz(char *nombre_interfaz)
{
    while (1)
    {
        t_semaforosIO *semaforos_interfaz = dictionary_get(diccionario_semaforos_io, nombre_interfaz);

        sem_wait(&semaforos_interfaz->instruccion_en_cola);
        sem_wait(&semaforos_interfaz->binario_io_libre);
        pthread_mutex_lock(&semaforos_interfaz->mutex);

        t_queue *cola = dictionary_get(colas_blocks_io, nombre_interfaz);
        t_instruccionEnIo *instruccion = queue_pop(cola);

        pthread_mutex_unlock(&semaforos_interfaz->mutex);

        ejecutar_instruccion_io(nombre_interfaz, instruccion);
    }
}

t_pcb *buscar_pcb_por_pid(u_int32_t pid, t_list *lista)
{
    t_pcb *pcb = malloc(sizeof(t_pcb));
    t_pcb *pcb_a_comparar;
    for (int i = 0; i < list_size(lista); i++)
    {
        pcb_a_comparar = list_get(lista, i);
        if (pcb_a_comparar->pid == pid)
        {
            pcb = pcb_a_comparar;
            break;
        }
    }

    if (pcb == NULL)
    {
        printf("Error: no se encontró el proceso con PID %d\n", pid);
        exit(1);
    }
    return pcb;
}

void *verificar_quantum()
{
    t_temporal *tiempo_transcurrido;
    while (1)
    {
        sem_wait(&arrancar_quantum); // hay que inicializarlo en la funcioin que esta en TODO

        pthread_mutex_lock(&mutex_flag_cpu_libre); // hay que inicializarlo en la funcioin que esta en TODO
        flag_cpu_libre = 0;
        pthread_mutex_unlock(&mutex_flag_cpu_libre);

        tiempo_transcurrido = temporal_create();

        while (1) // esto tieene espera activa, poderiamso usar un  usleep(500); pero no c si es lo mejor
                  //  depues tengop otra opcion usando combinación de semáforos y condicionales.
        {
            pthread_mutex_lock(&mutex_flag_cpu_libre);

            if (temporal_gettime(tiempo_transcurrido) >= quantum)
            {
                log_info(logger_kernel, "FIN DE QUANTUM: MANDO INTERRUPCION");
                enviar_interrupcion(pcb_en_ejecucion->pid, FIN_CLOCK, conexion_interrupt);
                break;
            }
            if (flag_cpu_libre == 1)
            {

                log_info(logger_kernel, "NO LLEGUE AL FIN DE QUANTUM, APARECIO ALGO ANTES");
                temporal_destroy(clock);
                break;
            }
            pthread_mutex_unlock(&mutex_flag_cpu_libre);
        }
        temporal_destroy(tiempo_transcurrido);
    }

    return NULL;
}

void iniciar_colas_de_estados_procesos()
{
    cola_procesos_ready = queue_create();
    cola_procesos_new = queue_create();
}

void iniciar_listas()
{
    lista_procesos_blocked = list_create();
}

void iniciar_diccionarios()
{
    conexiones_io = dictionary_create();
    colas_blocks_io = dictionary_create();
    diccionario_semaforos_io = dictionary_create();
}

void iniciar_semaforos()
{
    sem_init(&contador_grado_multiprogramacion, 0, grado_multiprogramacion);
    sem_init(&hay_proceso_a_ready, 0, 0);
    sem_init(&cpu_libre, 0, 1);
    sem_init(&arrancar_quantum, 0, 0);
    pthread_mutex_init(&mutex_pid, NULL);
    pthread_mutex_init(&mutex_cola_de_readys, NULL);
    pthread_mutex_init(&mutex_lista_de_blocked, NULL);
    pthread_mutex_init(&mutex_cola_de_new, NULL);
    pthread_mutex_init(&mutex_proceso_en_ejecucion, NULL);
    pthread_mutex_init(&mutex_interfaces_conectadas, NULL);
    pthread_mutex_init(&mutex_cola_interfaces, NULL);
    pthread_mutex_init(&mutex_diccionario_interfaces_de_semaforos, NULL);
    pthread_mutex_init(&mutex_flag_cpu_libre, NULL);
}