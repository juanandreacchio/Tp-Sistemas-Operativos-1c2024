#include "../include/kernel.h"

void iniciar_planificador_largo_plazo()
{
    pthread_create(&planificador_largo_creacion, NULL, (void *)creacion_de_procesos, NULL);
    pthread_detach(planificador_largo_creacion);

    pthread_create(&planificador_largo_eliminacion, NULL, (void *)eliminacion_de_procesos, NULL);
    pthread_detach(planificador_largo_eliminacion);
}

void creacion_de_procesos()
{
    while (1)
    {
        if (planificacion_detenida)
        {
            sem_wait(&podes_crear_procesos);
        }
        sem_wait(&hay_proceso_nuevo);
        wait_contador(semaforo_multi);
        pthread_mutex_lock(&mutex_cola_de_new);
        t_pcb *pcb_ready = queue_pop(cola_procesos_new);
        pthread_mutex_unlock(&mutex_cola_de_new);
        log_info(logger_kernel, "Se va a pasar el proceso %d a ready", pcb_ready->pid);
        log_info(logger_kernel, "Grado de multi: %d", semaforo_multi->valor_actual);

        set_add_pcb_cola(pcb_ready, READY, cola_procesos_ready, mutex_cola_de_readys);
        listar_procesos_en_ready();
        logear_cambio_estado(pcb_ready, NEW, READY);

        sem_post(&hay_proceso_a_ready);
    }
}

void eliminacion_de_procesos()
{
    while (1)
    {
        if (planificacion_detenida)
        {
            sem_wait(&podes_eliminar_procesos);
        }
        sem_wait(&hay_proceso_exit);
        pthread_mutex_lock(&mutex_cola_de_exit);
        t_pcb *pcb_exit = queue_pop(cola_procesos_exit);
        pthread_mutex_unlock(&mutex_cola_de_exit);
        liberar_recursos(pcb_exit->pid);
        // Falta eliminarlo de memoria
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        list_remove_element(procesos_en_sistema, pcb_exit);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);

        signal_contador(semaforo_multi);
        destruir_pcb(pcb_exit);
    }
}

uint32_t procesos_en_memoria()
{
    if (hay_proceso_ejecutandose())
    {
        return queue_size(cola_procesos_ready) + queue_size(cola_ready_plus) + 1;
    }
    return queue_size(cola_procesos_ready) + queue_size(cola_ready_plus);
}

bool hay_proceso_ejecutandose()
{
    return pcb_en_ejecucion != NULL;
}

void signal_contador(t_semaforo_contador *semaforo)
{
    if (semaforo->valor_actual < 0)
    {
        pthread_mutex_lock(&semaforo->mutex_valor_actual);
        semaforo->valor_actual++;
        pthread_mutex_unlock(&semaforo->mutex_valor_actual);
        return;
    }
    else
    {
        sem_post(&semaforo->contador);
        pthread_mutex_lock(&semaforo->mutex_valor_actual);
        semaforo->valor_actual++;
        pthread_mutex_unlock(&semaforo->mutex_valor_actual);
    }
}

void wait_contador(t_semaforo_contador *semaforo)
{
    sem_wait(&semaforo->contador);
    pthread_mutex_lock(&semaforo->mutex_valor_actual);
    semaforo->valor_actual--;
    pthread_mutex_unlock(&semaforo->mutex_valor_actual);
}

void cambiar_grado(uint32_t nuevo_grado)
{
    int32_t nuevo_valor_actual = nuevo_grado - procesos_en_memoria();
    uint32_t distancia = abs(semaforo_multi->valor_actual - nuevo_valor_actual);
    if (nuevo_grado < grado_multiprogramacion)
    {
        for (size_t i = 0; i < distancia; i++)
        {
            if (semaforo_multi->valor_actual == 0)
            {
                break;
            }
            wait_contador(semaforo_multi);
        }
    }
    else if (nuevo_grado > grado_multiprogramacion)
    {
        for (size_t i = 0; i < distancia; i++)
        {
            signal_contador(semaforo_multi);
        }
        pthread_mutex_lock(&semaforo_multi->mutex_valor_actual);
    }
    pthread_mutex_lock(&semaforo_multi->mutex_valor_maximo);
    semaforo_multi->valor_maximo = nuevo_grado;
    pthread_mutex_unlock(&semaforo_multi->mutex_valor_maximo);

    pthread_mutex_lock(&semaforo_multi->mutex_valor_actual);
    semaforo_multi->valor_actual += nuevo_valor_actual;
    pthread_mutex_unlock(&semaforo_multi->mutex_valor_actual);
}
