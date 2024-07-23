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
        sem_wait(&hay_proceso_nuevo);
        if (planificacion_detenida)
        {
            sem_wait(&podes_crear_procesos);
        }

        //wait_contador(semaforo_multi);
        sem_wait(&grado_multi);
        pthread_mutex_lock(&mutex_cola_de_new);
        t_pcb *pcb_ready = queue_pop(cola_procesos_new);
        pthread_mutex_unlock(&mutex_cola_de_new);

        set_add_pcb_cola(pcb_ready, READY, cola_procesos_ready, mutex_cola_de_readys);
        logear_cambio_estado(pcb_ready, NEW, READY);

        sem_post(&hay_proceso_a_ready);
    }
}

void eliminacion_de_procesos()
{
    while (1)
    {
        sem_wait(&hay_proceso_exit);
        if (planificacion_detenida)
        {
            sem_wait(&podes_eliminar_procesos);
        }

        pthread_mutex_lock(&mutex_cola_de_exit);
        t_proceso_en_exit *proceso_exit = queue_pop(cola_procesos_exit);
        pthread_mutex_unlock(&mutex_cola_de_exit);
        t_pcb *pcb_exit = proceso_exit->pcb;

        liberar_recursos(pcb_exit->pid);
        uint32_t index = tener_index_pid(pcb_exit->pid);

        t_paquete *paquete = crear_paquete(END_PROCESS);
        buffer_add(paquete->buffer, &pcb_exit->pid, sizeof(uint32_t));
        enviar_paquete(paquete, conexion_memoria);
        if (recibir_operacion(conexion_memoria) != END_PROCESS)
        {
            log_error(logger_kernel, "error de end process del PID %d", pcb_exit->pid);
            return;
        }

        if (index == -1)
        {
            log_error(logger_kernel, "No se encontró el proceso con PID %d", pcb_exit->pid);
        }
        else
        {
            log_info(logger_kernel, "Finaliza el proceso %d - Motivo: %s", pcb_exit->pid, op_code_to_string(proceso_exit->motivo));
            list_remove(procesos_en_sistema, index);
        }

        //signal_contador(semaforo_multi);
        if(flag_grado_multi == 0)
        {
            sem_post(&grado_multi);
            
        }else{
            flag_grado_multi -=1;
        }
            
        destruir_pcb(pcb_exit);
        free(proceso_exit);
    }
}

uint32_t procesos_en_memoria()
{
    if (hay_proceso_ejecutandose())
    {
        pthread_mutex_lock(&mutex_cola_de_readys);
        pthread_mutex_lock(&mutex_cola_de_ready_plus);
        return queue_size(cola_procesos_ready) + queue_size(cola_ready_plus) + 1;
        pthread_mutex_unlock(&mutex_cola_de_readys);
        pthread_mutex_unlock(&mutex_cola_de_ready_plus);
    }
    pthread_mutex_lock(&mutex_cola_de_readys);
    pthread_mutex_lock(&mutex_cola_de_ready_plus);
    return queue_size(cola_procesos_ready) + queue_size(cola_ready_plus);
    pthread_mutex_unlock(&mutex_cola_de_readys);
    pthread_mutex_unlock(&mutex_cola_de_ready_plus);
}

bool hay_proceso_ejecutandose()
{
    pthread_mutex_lock(&mutex_proceso_en_ejecucion);
    return pcb_en_ejecucion != NULL;
    pthread_mutex_unlock(&mutex_proceso_en_ejecucion);
}

void signal_contador(t_semaforo_contador *semaforo) // aca me parece que hay que arreglar los mutex pero estoy en duda
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
        if (semaforo->valor_actual < semaforo->valor_maximo)
        {
            sem_post(&semaforo->contador);
            pthread_mutex_lock(&semaforo->mutex_valor_actual);
            semaforo->valor_actual++;
            pthread_mutex_unlock(&semaforo->mutex_valor_actual);
        }
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
    int32_t cantidad_de_signal_o_wait = abs(nuevo_grado - semaforo_multi->valor_maximo);

    pthread_mutex_lock(&semaforo_multi->mutex_valor_maximo);
    semaforo_multi->valor_maximo = nuevo_grado;
    pthread_mutex_unlock(&semaforo_multi->mutex_valor_maximo);

    if (nuevo_grado > semaforo_multi->valor_maximo)
    {
        for (size_t i = 0; i < cantidad_de_signal_o_wait; i++)
        {
            signal_contador(semaforo_multi);
        }
    }
    else if (nuevo_grado < semaforo_multi->valor_maximo)
    {
        int cantidad_de_wait = 0;
        while (semaforo_multi->valor_actual > 0 && cantidad_de_wait < cantidad_de_signal_o_wait)
        {
            wait_contador(semaforo_multi);
            cantidad_de_wait++;
        }
        int waits_restantes = cantidad_de_signal_o_wait - cantidad_de_wait;
        semaforo_multi->valor_actual -= waits_restantes;
    }
}
/*
void eliminar_procesos(u_int32_t cant_a_eliminar)
{
    for (size_t i = 0; i < cant_a_eliminar; i++)
    {
       finalizar_pcb(pcb_actualizado, OUT_OF_MEMORY);
    }
    
}
*/
