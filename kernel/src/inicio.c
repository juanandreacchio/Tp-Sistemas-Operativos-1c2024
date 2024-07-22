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
    cola_ready_plus = queue_create();
}

void iniciar_listas()
{
    lista_procesos_blocked = list_create();
    procesos_en_sistema = list_create();
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
    sem_init(&hay_proceso_a_ready, 0, 0);
    sem_init(&cpu_libre, 0, 1);
    sem_init(&arrancar_quantum, 0, 0);
    sem_init(&hay_proceso_nuevo, 0, 0);
    sem_init(&hay_proceso_exit, 0, 0);
    sem_init(&podes_revisar_lista_bloqueados, 0, 0);
    sem_init(&podes_planificar_corto_plazo, 0, 0);
    sem_init(&podes_crear_procesos, 0, 0);
    sem_init(&podes_manejar_desalojo, 0, 0);
    sem_init(&podes_eliminar_procesos, 0, 0);
    sem_init(&podes_eliminar_loko, 0, 0);
    sem_init(&podes_manejar_recepcion_de_interfaces, 0, 0);
    sem_init(&grado_multi, 0, grado_multiprogramacion);
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
    pthread_mutex_init(&mutex_procesos_en_sistema, NULL);
    pthread_mutex_init(&mutex_cola_de_ready_plus, NULL);
    pthread_mutex_init(&mutex_ultimo_pcb, NULL);
    pthread_mutex_init(&mutex_flag_planificar_plus, NULL);
    pthread_mutex_init(&mutex_nombre_interfaz_bloqueante, NULL);

    //iniciar_semaforo_contador(semaforo_multi, grado_multiprogramacion);
}

void iniciar_variables()
{
    flag_grado_multi = 0;
    contador_pid = 0;
    ultimo_pcb_ejecutado = NULL;
    planificar_ready_plus = 0;
    nombre_entrada_salida_conectada = NULL;
    semaforo_multi = malloc(sizeof(t_semaforo_contador));
    planificacion_detenida = false;
}

void iniciar_semaforo_contador(t_semaforo_contador *semaforo, uint32_t valor_inicial)
{
    semaforo->valor_maximo = valor_inicial;
    semaforo->valor_actual = valor_inicial;
    sem_init(&semaforo->contador, 0, valor_inicial);
    pthread_mutex_init(&semaforo->mutex_valor_actual, NULL);
    pthread_mutex_init(&semaforo->mutex_valor_maximo, NULL);
}

void destruir_semaforo_contador(t_semaforo_contador *semaforo)
{
    pthread_mutex_destroy(&semaforo->mutex_valor_actual);
    pthread_mutex_destroy(&semaforo->mutex_valor_maximo);
    free(semaforo);
}

void destruir_semaforos()
{
    sem_destroy(&hay_proceso_a_ready);
    sem_destroy(&cpu_libre);
    sem_destroy(&arrancar_quantum);
    sem_destroy(&hay_proceso_nuevo);
    sem_destroy(&hay_proceso_exit);
    sem_destroy(&podes_revisar_lista_bloqueados);
    sem_destroy(&podes_planificar_corto_plazo);
    sem_destroy(&podes_crear_procesos);
    sem_destroy(&podes_manejar_desalojo);
    sem_destroy(&podes_eliminar_procesos);
    sem_destroy(&podes_eliminar_loko);
    sem_destroy(&podes_manejar_recepcion_de_interfaces);
    sem_destroy(&grado_multi);
    pthread_mutex_destroy(&mutex_pid);
    pthread_mutex_destroy(&mutex_cola_de_readys);
    pthread_mutex_destroy(&mutex_lista_de_blocked);
    pthread_mutex_destroy(&mutex_cola_de_new);
    pthread_mutex_destroy(&mutex_proceso_en_ejecucion);
    pthread_mutex_destroy(&mutex_interfaces_conectadas);
    pthread_mutex_destroy(&mutex_cola_interfaces);
    pthread_mutex_destroy(&mutex_diccionario_interfaces_de_semaforos);
    pthread_mutex_destroy(&mutex_flag_cpu_libre);
    pthread_mutex_destroy(&mutex_motivo_ultimo_desalojo);
    pthread_mutex_destroy(&mutex_cola_de_exit);
    pthread_mutex_destroy(&mutex_procesos_en_sistema);
    pthread_mutex_destroy(&mutex_cola_de_ready_plus);
    pthread_mutex_destroy(&mutex_ultimo_pcb);
    pthread_mutex_destroy(&mutex_flag_planificar_plus);
    pthread_mutex_destroy(&mutex_nombre_interfaz_bloqueante);
    destruir_semaforo_contador(semaforo_multi);
}

void eliminar_diccionarios()
{
    dictionary_destroy_and_destroy_elements(conexiones_io, free);
    dictionary_destroy_and_destroy_elements(colas_blocks_io, free);
    dictionary_destroy_and_destroy_elements(diccionario_semaforos_io, free);
    dictionary_destroy_and_destroy_elements(recursos_disponibles, free);
    dictionary_destroy_and_destroy_elements(cola_de_bloqueados_por_recurso, free);
    dictionary_destroy_and_destroy_elements(recursos_asignados_por_proceso, free);
}

void eliminar_listas()
{
    list_destroy_and_destroy_elements(lista_procesos_blocked, free);
    list_destroy_and_destroy_elements(procesos_en_sistema, free);
}

void eliminar_colas()
{
    queue_destroy_and_destroy_elements(cola_procesos_ready, free);
    queue_destroy_and_destroy_elements(cola_procesos_new, free);
    queue_destroy_and_destroy_elements(cola_procesos_exit, free);
    queue_destroy_and_destroy_elements(cola_ready_plus, free);
}