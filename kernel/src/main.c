#include <../include/kernel.h>

t_config *config_kernel;

t_log *logger_kernel;
char *ip_memoria;
char *ip_cpu;
char *puerto_memoria;
char *puerto_escucha;
char *puerto_dispatch;
char *puerto_interrupt;
char *ip;
u_int32_t conexion_memoria, conexion_dispatch, conexion_interrupt;
int socket_servidor_kernel, socket_cliente_kernel;
int contador_pcbs, identificador_pid = 1; 
int grado_multiprogramacion;
uint32_t contador_pid;
int quantum;
t_dictionary *colas_blocked;

pthread_mutex_t mutex_pid;
pthread_mutex_t mutex_cola_de_readys;
pthread_mutex_t mutex_proceso_en_ejecucion;
sem_t contador_grado_multiprogramacion;
t_list *lista_procesos_ready;
t_pcb *pcb_en_ejecucion;

//-----------------varaibles globales------------------
int grado_multiprogramacion = 5;

int main(void)
{
    iniciar_config();
    lista_procesos_ready = list_create();
    colas_blocked = dictionary_create();
    sem_init(&contador_grado_multiprogramacion, 0, grado_multiprogramacion);
    
    // iniciar conexion con Kernel
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_kernel);
    enviar_mensaje("", conexion_memoria, KERNEL, logger_kernel);

    // iniciar conexion con CPU Dispatch e Interrupt
    conexion_dispatch = crear_conexion(ip_cpu, puerto_dispatch, logger_kernel);
    conexion_interrupt = crear_conexion(ip_cpu, puerto_interrupt, logger_kernel);
    enviar_mensaje("", conexion_dispatch, KERNEL, logger_kernel);
    enviar_mensaje("", conexion_interrupt, KERNEL, logger_kernel);
    //enviar_codigo_operacion(INTERRUPTION,conexion_interrupt);

    // iniciar Servidor
    socket_servidor_kernel = iniciar_servidor(logger_kernel, puerto_escucha, "KERNEL");

    pthread_t thread_consola;
    pthread_create(&thread_consola, NULL,(void*) iniciar_consola_interactiva,NULL);
    pthread_detach(thread_consola);

    while (1)
    {
        pthread_t thread;
        int *socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(socket_servidor_kernel, logger_kernel);
        pthread_create(&thread, NULL,(void*) atender_cliente,socket_cliente);
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
}

void *atender_cliente(void *socket_cliente)
{
    op_code codigo_operacion = recibir_operacion(*(int*)socket_cliente);
    switch (codigo_operacion)
    {
    case ENTRADA_SALIDA:
        char* nombre_entrada_salida = recibir_mensaje_guardar_variable(*(int*)socket_cliente);
        log_info(logger_kernel, "Se conecto la I/O llamda: %s",nombre_entrada_salida);
        dictionary_put(colas_blocked, nombre_entrada_salida, crear_cola_de_bloqueados_de_interfaz());
        break;
    default:
        log_info(logger_kernel, "Se recibio un mensaje de un modulo desconocido");
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
        // ---------------------- CREAR PCB EN EL KERNEL -------------------
        char *path = consola[1];

        
        //t_list *listaInstrucciones = list_create();
        //listaInstrucciones = parsear_instrucciones(archivo_pseudocodigo);
        t_pcb *pcb_creado = crear_pcb(contador_pid, quantum, NEW);

        // ---------------------- ENVIAR SOLICITUD PARA QUE SE CREE EN MEMORIA -------------------
        t_paquete *paquete = crear_paquete(CREAR_PROCESO);
        t_solicitudCreacionProcesoEnMemoria *ptr_solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));
        ptr_solicitud->pid = contador_pid;
        ptr_solicitud->path_length = strlen(path) + 1;
        ptr_solicitud->path = path;
        t_buffer *buffer = serializar_solicitud_crear_proceso(ptr_solicitud);
        paquete->buffer = buffer;

        
        sem_wait(&contador_grado_multiprogramacion);
        enviar_paquete(paquete, conexion_memoria);
        pthread_mutex_lock(&mutex_cola_de_readys);
        list_add(lista_procesos_ready, pcb_creado);
        pcb_creado->estado_actual = READY;
        pthread_mutex_unlock(&mutex_cola_de_readys);

        pthread_mutex_lock(&mutex_pid);
        contador_pid++;
        pthread_mutex_unlock(&mutex_pid);

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
    t_cola_bloqueados_io* cola_bloqueados_io = malloc(sizeof(t_cola_bloqueados_io));
    cola_bloqueados_io->lista_instrucciones = list_create();
    return cola_bloqueados_io;
}


// Planificacion

void ejecutar_PCB(t_pcb *pcb){
    setear_pcb_en_ejecucion(pcb);
    enviar_pcb(pcb, conexion_dispatch);
    log_info(logger_kernel, "Se envio el PCB con PID %d a CPU Dispatch", pcb->pid);
    MOTIVO_DESALOJO motivo = recibir_motivo_desalojo(conexion_dispatch);
    t_pcb *pcb_actualizado = recibir_pcb(conexion_dispatch);
    destruir_pcb(pcb);
}

void setear_pcb_en_ejecucion(t_pcb * pcb){
    pcb->estado_actual = EXEC;

    pthread_mutex_lock(&mutex_proceso_en_ejecucion);
    pcb_en_ejecucion = pcb;
    pthread_mutex_unlock(&mutex_proceso_en_ejecucion);

    pthread_mutex_lock(&mutex_cola_de_readys);
    list_remove_by_condition(lista_procesos_ready, pcb->pid);
    pthread_mutex_unlock(&mutex_cola_de_readys);
}