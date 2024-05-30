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
t_dictionary *recursos_disponibles;
t_dictionary *cola_de_bloqueados_por_recurso;
t_dictionary *recursos_asignados_por_proceso;

pthread_mutex_t mutex_pid;
pthread_mutex_t mutex_cola_de_readys;
pthread_mutex_t mutex_lista_de_blocked;
pthread_mutex_t mutex_cola_de_new;
pthread_mutex_t mutex_proceso_en_ejecucion;
pthread_mutex_t mutex_interfaces_conectadas;
pthread_mutex_t mutex_cola_interfaces; // PARA AGREGAR AL DICCIOANRIO de la cola de cada interfaz
pthread_mutex_t mutex_diccionario_interfaces_de_semaforos;
pthread_mutex_t mutex_flag_cpu_libre;
pthread_mutex_t mutex_motivo_ultimo_desalojo;
sem_t contador_grado_multiprogramacion, hay_proceso_a_ready, cpu_libre, arrancar_quantum;
t_queue *cola_procesos_ready;
t_queue *cola_procesos_new;
t_queue *cola_procesos_exit;
t_list *lista_procesos_blocked; // lsita de los prccesos bloqueados
t_pcb *pcb_en_ejecucion;
pthread_t planificador_corto;
pthread_t dispatch;
pthread_t hilo_quantum;
op_code motivo_ultimo_desalojo;
char **recursos;
char **instancias_recursos;

//-----------------varaibles globales------------------
int grado_multiprogramacion;

int main(void)
{
    iniciar_config();
    iniciar_listas();
    iniciar_colas_de_estados_procesos();
    iniciar_diccionarios();
    iniciar_semaforos();
    iniciar_recursos();

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

    pthread_create(&planificador_corto, NULL, (void *)iniciar_planificador_corto_plazo, NULL);
    pthread_detach(planificador_corto);

    pthread_t thread_consola;
    pthread_create(&thread_consola, NULL, (void *)iniciar_consola_interactiva, NULL);
    pthread_detach(thread_consola);

    if (strcmp(algoritmo_planificacion, "RR") == 0)
    {
        log_info(logger_kernel, "iniciando contador de quantum");
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
    recursos = config_get_array_value(config_kernel, "RECURSOS");
    instancias_recursos = config_get_array_value(config_kernel, "INSTANCIAS_RECURSOS");

    if (string_array_size(recursos) != string_array_size(instancias_recursos))
    {
        log_error(logger_kernel, "La cantidad de recursos y de instancias de recursos no coincide");
        exit(EXIT_FAILURE);
    }
}

void *atender_cliente(void *socket_cliente_ptr)
{
    int socket_cliente = *(int *)socket_cliente_ptr;
    free(socket_cliente_ptr); // Liberamos el puntero ya que no lo necesitamos más

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

        dictionary_put(recursos_asignados_por_proceso, string_itoa(contador_pid), dictionary_create());

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
            log_info(logger_kernel, "puse el pcb en ready y lo meti dentro de la cola de ready");
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

void iniciar_planificador_corto_plazo()
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

        pthread_mutex_lock(&mutex_motivo_ultimo_desalojo);
        motivo_ultimo_desalojo = motivo_desalojo;
        pthread_mutex_unlock(&mutex_motivo_ultimo_desalojo);

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

            pthread_mutex_lock(&mutex_lista_de_blocked);
            list_add(lista_procesos_blocked, pcb_actualizado);
            pthread_mutex_unlock(&mutex_lista_de_blocked);

            sem_post(&contador_grado_multiprogramacion);

            // 1. La agregamos a la cola de blocks io. Ultima instrucción y PID
            t_instruccionEnIo *instruccion_en_io = malloc(sizeof(t_instruccionEnIo));
            instruccion_en_io->instruccion_io = utlima_instruccion;
            instruccion_en_io->pid = pcb_actualizado->pid;

            pthread_mutex_lock(&mutex_cola_interfaces); // estopy casi seguro que este mutex esta mal porque es el mismo que esta arriba y son listas distintas
            queue_push((t_queue *)dictionary_get(colas_blocks_io, nombre_io), instruccion_en_io);
            pthread_mutex_unlock(&mutex_cola_interfaces);

            t_semaforosIO *semaforos_interfaz = dictionary_get(diccionario_semaforos_io, nombre_io);
            sem_post(&semaforos_interfaz->instruccion_en_cola);

            sem_post(&cpu_libre);
            pthread_mutex_lock(&mutex_flag_cpu_libre);
            flag_cpu_libre = 1;
            pthread_mutex_unlock(&mutex_flag_cpu_libre);

            break;
        case FIN_CLOCK:
            log_info(logger_kernel, "entre al fin de clock por dispatcher");
            set_add_pcb_cola(pcb_actualizado, READY, cola_procesos_ready, mutex_cola_de_readys);
            sem_post(&hay_proceso_a_ready);

            sem_post(&cpu_libre);
            pthread_mutex_lock(&mutex_flag_cpu_libre);
            flag_cpu_libre = 1;
            pthread_mutex_unlock(&mutex_flag_cpu_libre);
            break;
        case KILL_PROCESS:
            // TODO
            break;
        case END_PROCESS:
            log_info(logger_kernel, "entre al end process por dispatcher");
            sem_post(&contador_grado_multiprogramacion);

            sem_post(&cpu_libre);
            pthread_mutex_lock(&mutex_flag_cpu_libre);
            flag_cpu_libre = 1;
            pthread_mutex_unlock(&mutex_flag_cpu_libre);
            break;
        case WAIT_SOLICITADO:
            t_paquete *respuesta = recibir_paquete(conexion_dispatch);
            t_instruccion *utlima = instruccion_deserializar(respuesta->buffer, 0);
            char *recurso_solicitado = utlima->parametros[0];

            if (!existe_recurso(recurso_solicitado))
            {
                finalizar_proceso(pcb_actualizado);
                break;
            }
            int32_t instancias_restantes = restar_instancia_a_recurso(recurso_solicitado);
            retener_instancia_de_recurso(recurso_solicitado, pcb_actualizado->pid);
            if (instancias_restantes < 0)
            {
                agregar_pcb_a_cola_bloqueados_de_recurso(pcb_actualizado, recurso_solicitado);

                pcb_actualizado->estado_actual = BLOCKED;
                sem_post(&cpu_libre);
                pthread_mutex_lock(&mutex_flag_cpu_libre);
                flag_cpu_libre = 1;
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
            }
            else
            {
                setear_pcb_en_ejecucion(pcb_actualizado);
            }
            break;
        case SIGNAL_SOLICITADO:
            t_paquete *respuesta_signal = recibir_paquete(conexion_dispatch);
            t_instruccion *instruccion_signal = instruccion_deserializar(respuesta_signal->buffer, 0);
            char *recurso_signal = instruccion_signal->parametros[0];

            if (!existe_recurso(recurso_signal))
            {
                finalizar_proceso(pcb_actualizado);
                break;
            }
            sumar_instancia_a_recurso(recurso_solicitado);
            setear_pcb_en_ejecucion(pcb_actualizado);
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
    dictionary_put(diccionario_semaforos_io, nombre, semaforos_interfaz);
    pthread_mutex_unlock(&mutex_diccionario_interfaces_de_semaforos);

    pthread_t hilo_IO;
    char *nombre2 = strdup(nombre);
    pthread_create(&hilo_IO, NULL, (void *)atender_interfaz, nombre2);
    pthread_detach(hilo_IO);
}

void ejecutar_instruccion_io(char *nombre_interfaz, t_instruccionEnIo *instruccionEnIO, t_interfaz_en_kernel *conexion_io)
{
    t_paquete *paquete = crear_paquete(EJECUTAR_IO);
    paquete->buffer = serializar_instruccion(instruccionEnIO->instruccion_io);
    enviar_paquete(paquete, conexion_io->conexion);
}

void atender_interfaz(char *nombre_interfaz)
{
    while (1)
    {
        t_semaforosIO *semaforos_interfaz = dictionary_get(diccionario_semaforos_io, nombre_interfaz);

        sem_wait(&(semaforos_interfaz->instruccion_en_cola));
        sem_wait(&(semaforos_interfaz->binario_io_libre));
        pthread_mutex_lock(&(semaforos_interfaz->mutex));

        t_queue *cola = dictionary_get(colas_blocks_io, nombre_interfaz);
        t_instruccionEnIo *instruccion = queue_pop(cola);

        pthread_mutex_unlock(&(semaforos_interfaz->mutex));

        t_interfaz_en_kernel *conexion_io = dictionary_get(conexiones_io, nombre_interfaz);
        ejecutar_instruccion_io(nombre_interfaz, instruccion, conexion_io);

        op_code resultado_operacion = recibir_operacion(conexion_io->conexion);
        switch (resultado_operacion)
        {
        case IO_SUCCESS:
            log_info(logger_kernel, "llegue a IO_SUCCESS");
            t_pcb *pcb = buscar_pcb_por_pid(instruccion->pid, lista_procesos_blocked);

            pthread_mutex_lock(&mutex_lista_de_blocked);
            list_remove_element(lista_procesos_blocked, pcb);
            pthread_mutex_unlock(&mutex_lista_de_blocked);

            if (pcb != NULL)
            {
                sem_wait(&contador_grado_multiprogramacion);
                set_add_pcb_cola(pcb, READY, cola_procesos_ready, mutex_cola_de_readys);
                sem_post(&hay_proceso_a_ready);
            }

            sem_post(&(semaforos_interfaz->binario_io_libre));

            break;
        case CERRAR_IO:
            close(conexion_io->conexion); // Cerramos el socket una vez que salimos del loop
            break;

        default:
            break;
        }
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
        sem_wait(&arrancar_quantum);

        pthread_mutex_lock(&mutex_proceso_en_ejecucion);
        pthread_mutex_unlock(&mutex_proceso_en_ejecucion);

        pthread_mutex_lock(&mutex_flag_cpu_libre);
        flag_cpu_libre = 0;
        pthread_mutex_unlock(&mutex_flag_cpu_libre);

        tiempo_transcurrido = temporal_create();

        while (1)
        {
            usleep(1000);

            if (temporal_gettime(tiempo_transcurrido) >= quantum)
            {
                log_info(logger_kernel, "FIN DE QUANTUM: MANDO INTERRUPCION");

                pthread_mutex_lock(&mutex_flag_cpu_libre);
                enviar_interrupcion(pcb_en_ejecucion->pid, FIN_CLOCK, conexion_interrupt);
                flag_cpu_libre = 1;
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
                break;
            }

            pthread_mutex_lock(&mutex_flag_cpu_libre);
            if (flag_cpu_libre == 1)
            {
                switch (motivo_ultimo_desalojo)
                {
                case FIN_CLOCK:
                    log_info(logger_kernel, "LLEGUE AL FIN DE QUANTUM");
                    break;
                case OPERACION_IO:
                    log_info(logger_kernel, "DESALOJÉ POR IO");
                    break;
                case KILL_PROCESS:
                    log_info(logger_kernel, "DESALOJÉ POR KILL");
                    break;
                case END_PROCESS:
                    log_info(logger_kernel, "DESALOJÉ POR END");
                    break;
                default:
                    break;
                }
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
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
    cola_procesos_exit = queue_create();
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
    recursos_disponibles = dictionary_create();
    cola_de_bloqueados_por_recurso = dictionary_create();
    recursos_asignados_por_proceso = dictionary_create();
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
    pthread_mutex_init(&mutex_motivo_ultimo_desalojo, NULL);
}

void iniciar_recurso(char *nombre, char *instancias)
{
    t_recurso_en_kernel *recurso = malloc(sizeof(t_recurso_en_kernel));

    recurso->instancias = atoi(instancias);
    pthread_mutex_init(&recurso->mutex_cola_recurso, NULL);
    pthread_mutex_init(&recurso->mutex, NULL);

    dictionary_put(recursos_disponibles, nombre, recurso);
    dictionary_put(cola_de_bloqueados_por_recurso, nombre, queue_create());
}

void iniciar_recursos()
{
    for (int i = 0; i < string_array_size(recursos); i++)
    {
        iniciar_recurso(recursos[i], instancias_recursos[i]);
    }
}

int32_t restar_instancia_a_recurso(char *nombre)
{
    t_recurso_en_kernel *recurso = dictionary_get(recursos_disponibles, nombre);
    pthread_mutex_lock(&recurso->mutex);
    recurso->instancias--;
    pthread_mutex_unlock(&recurso->mutex);
    return recurso->instancias;
}

void agregar_pcb_a_cola_bloqueados_de_recurso(t_pcb *pcb, char *nombre)
{
    t_recurso_en_kernel *recurso = dictionary_get(recursos_disponibles, nombre);
    pthread_mutex_lock(&recurso->mutex_cola_recurso);
    queue_push(dictionary_get(cola_de_bloqueados_por_recurso, nombre), pcb);
    pthread_mutex_unlock(&recurso->mutex_cola_recurso);
}

void sumar_instancia_a_recurso(char *nombre)
{
    t_recurso_en_kernel *recurso = dictionary_get(recursos_disponibles, nombre);
    pthread_mutex_lock(&recurso->mutex);
    recurso->instancias++;
    pthread_mutex_unlock(&recurso->mutex);
    if (recurso->instancias >= 0)
    {
        pthread_mutex_lock(&recurso->mutex_cola_recurso);
        t_pcb *pcb = queue_pop(dictionary_get(cola_de_bloqueados_por_recurso, nombre));
        pthread_mutex_unlock(&recurso->mutex_cola_recurso);
        if (pcb != NULL)
        {
            set_add_pcb_cola(pcb, READY, cola_procesos_ready, mutex_cola_de_readys);
            sem_post(&hay_proceso_a_ready);
        }
    }
}

bool existe_recurso(char *nombre){
    return dictionary_has_key(recursos_disponibles, nombre);
}

void finalizar_proceso(t_pcb *pcb)
{
    pcb->estado_actual = EXIT;
    liberar_recursos(pcb->pid);

    sem_post(&contador_grado_multiprogramacion);
    destruir_pcb(pcb);
}

void liberar_recursos(uint32_t pid)
{
    t_dictionary *recursos_asignados = dictionary_get(recursos_asignados_por_proceso, pid);
    t_list *recursos_asignados_lista = dictionary_elements(recursos_asignados);
    for (int i = 0; i < list_size(recursos_asignados_lista); i++)
    {
        t_recurso_asignado_a_proceso *recurso_asignado = list_get(recursos_asignados, i);
        for (size_t i = 0; i < recurso_asignado->instancias_asignadas; i++)
        {
            sumar_instancia_a_recurso(recurso_asignado->nombre_recurso);
        }
    }
}

void retener_instancia_de_recurso(char *nombre_recurso, uint32_t pid)
{
    t_dictionary *recursos = dictionary_get(recursos_asignados_por_proceso, pid);
    if (!dictionary_has_key(recursos, nombre_recurso))
    {
        dictionary_put(recursos, nombre_recurso, 1);
    }
    else
    {
        dictionary_put(recursos, nombre_recurso, dictionary_get(recursos, nombre_recurso) + 1);
    }
}