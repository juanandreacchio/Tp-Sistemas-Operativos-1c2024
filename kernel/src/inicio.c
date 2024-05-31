#include "../include/kernel.h"

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
    sem_init(&contador_grado_multiprogramacion, 0, 5);
    sem_init(&hay_proceso_a_ready, 0, 0);
    sem_init(&cpu_libre, 0, 1);
    sem_init(&arrancar_quantum, 0, 0);
    sem_init(&hay_proceso_nuevo, 0, 0);
    sem_init(&hay_proceso_exit, 0, 0);
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
    pthread_mutex_init(&mutex_cola_de_exit, NULL);
}