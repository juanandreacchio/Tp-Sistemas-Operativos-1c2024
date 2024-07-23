#include "../include/kernel.h"

void iniciar_recurso(char *nombre, char *instancias)
{
    t_recurso_en_kernel *recurso = malloc(sizeof(t_recurso_en_kernel));

    recurso->instancias = atoi(instancias);
    pthread_mutex_init(&recurso->mutex_cola_recurso, NULL);
    pthread_mutex_init(&recurso->mutex, NULL);

    dictionary_put(recursos_disponibles, nombre, recurso);
    dictionary_put(cola_de_bloqueados_por_recurso, nombre, list_create());
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
    list_add(dictionary_get(cola_de_bloqueados_por_recurso, nombre), pcb);
    pthread_mutex_unlock(&recurso->mutex_cola_recurso);
}

void eliminar_pcb_de_cola_bloqueados_de_recurso(uint32_t pid, char *nombre)
{
    t_recurso_en_kernel *recurso = dictionary_get(recursos_disponibles, nombre);
    t_list *lista = dictionary_get(cola_de_bloqueados_por_recurso, nombre);
    pthread_mutex_lock(&recurso->mutex_cola_recurso);
    for (size_t i = 0; i < list_size(lista); i++)
    {
        t_pcb *pcb = list_get(lista, i);
        if (pcb->pid == pid)
        {
            list_remove(lista, i);
            pthread_mutex_unlock(&recurso->mutex_cola_recurso);
            log_info(logger_kernel, "Proceso %d eliminado de la cola de bloqueados por recurso %s", pid, nombre);
            return;
        }
    }
    log_info(logger_kernel, "Proceso %d eliminado de la cola de bloqueados por recurso %s", pid, nombre);

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
        t_list *lista = dictionary_get(cola_de_bloqueados_por_recurso, nombre);
        if (list_size(lista) > 0)
        {
            pthread_mutex_lock(&recurso->mutex_cola_recurso);
            t_pcb *pcb = list_remove(lista, 0);
            pthread_mutex_unlock(&recurso->mutex_cola_recurso);

            if (pcb != NULL)
            {
                uint32_t index = buscar_index_pid_bloqueado(pcb->pid);
                pthread_mutex_lock(&mutex_lista_de_blocked);
                list_remove(lista_procesos_blocked, index);
                pthread_mutex_unlock(&mutex_lista_de_blocked);

                if ((pcb->quantum > 0) && strcmp(algoritmo_planificacion, "VRR") == 0)
                {
                    set_add_pcb_cola(pcb, READY, cola_ready_plus, mutex_cola_de_ready_plus);
                    logear_cambio_estado(pcb, BLOCKED, READY);
                    sem_post(&hay_proceso_a_ready);
                    return;
                }
                log_info(logger_kernel, "Proceso %d pasa de BLOCKED a READY por RECURSOS", pcb->pid);
                logear_cambio_estado(pcb, BLOCKED, READY);
                set_add_pcb_cola(pcb, READY, cola_procesos_ready, mutex_cola_de_readys);
                //wait_contador(semaforo_multi);
                sem_post(&hay_proceso_a_ready);
            }
        }
    }
}

bool existe_recurso(char *nombre)
{
    return dictionary_has_key(recursos_disponibles, nombre);
}

void retener_instancia_de_recurso(char *nombre_recurso, uint32_t pid)
{

    t_list *recursos = dictionary_get(recursos_asignados_por_proceso, string_itoa(pid));

    for (size_t i = 0; i < list_size(recursos); i++)
    {
        t_recurso_asignado_a_proceso *recurso = list_get(recursos, i);
        log_info(logger_kernel, "Comparo %s con %s (solicitado)", recurso->nombre_recurso, nombre_recurso);
        if (string_equals_ignore_case(recurso->nombre_recurso, nombre_recurso))
        {
            recurso->instancias_asignadas++;

            return;
        }
    }
    t_recurso_asignado_a_proceso *recurso_nuevo = malloc(sizeof(t_recurso_asignado_a_proceso));
    recurso_nuevo->nombre_recurso = nombre_recurso;
    recurso_nuevo->instancias_asignadas = 1;
    list_add(recursos, recurso_nuevo);

    for (size_t i = 0; i < list_size(recursos); i++)
    {
        t_recurso_asignado_a_proceso *recursito = list_get(recursos, i);
        log_info(logger_kernel, "PID: %d - Recurso: %s - Instancias: %d", pid, recursito->nombre_recurso, recursito->instancias_asignadas);
    }
}

void liberar_recursos(uint32_t pid)
{
    t_list *recursos = dictionary_get(recursos_asignados_por_proceso, string_itoa(pid));
    log_info(logger_kernel, "Tengo un total de %d", list_size(recursos));
    for (size_t i = 0; i < list_size(recursos); i++)
    {
        t_recurso_asignado_a_proceso *recurso = list_get(recursos, i);
        eliminar_pcb_de_cola_bloqueados_de_recurso(pid, recurso->nombre_recurso);

        for (size_t j = 0; j < recurso->instancias_asignadas; j++)
        {
            log_info(logger_kernel, "Arranco a sumar instancias del recurso %s", recurso->nombre_recurso);
            sumar_instancia_a_recurso(recurso->nombre_recurso);
            log_info(logger_kernel, "PID: %d - Libero 1 instancia de %s", pid, recurso->nombre_recurso);
        }
    }
}
