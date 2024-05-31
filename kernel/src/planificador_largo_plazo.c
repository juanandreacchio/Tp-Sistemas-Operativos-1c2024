#include "../include/kernel.h"

void iniciar_planificador_largo_plazo()
{
    pthread_create(&planificador_largo, NULL, (void *)creacion_de_procesos, NULL);
    pthread_detach(planificador_largo);

    pthread_create(&planificador_largo, NULL, (void *)eliminacion_de_procesos, NULL);
    pthread_detach(planificador_largo);
}

void creacion_de_procesos()
{
    sem_wait(&hay_proceso_nuevo);
    sem_wait(&contador_grado_multiprogramacion);
    pthread_mutex_lock(&mutex_cola_de_new);
    t_pcb *pcb_ready = queue_pop(cola_procesos_new);
    pthread_mutex_unlock(&mutex_cola_de_new);

    set_add_pcb_cola(pcb_ready, READY, cola_procesos_ready, mutex_cola_de_readys);
    log_info(logger_kernel, "puse el pcb en ready y lo meti dentro de la cola de ready");

    sem_post(&hay_proceso_a_ready);
}

void eliminacion_de_procesos()
{
    sem_wait(&hay_proceso_exit);
    pthread_mutex_lock(&mutex_cola_de_exit);
    t_pcb *pcb_exit = queue_pop(cola_procesos_exit);
    pthread_mutex_unlock(&mutex_cola_de_exit);
    liberar_recursos(pcb_exit->pid);

    sem_post(&contador_grado_multiprogramacion);
    destruir_pcb(pcb_exit);
}