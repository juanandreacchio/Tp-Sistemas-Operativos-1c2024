#include "../include/kernel.h"

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
            if (pcb->quantum >= 0)
            {
                set_add_pcb_cola(pcb, READY, cola_ready_plus, mutex_cola_de_ready_plus);
                logear_cambio_estado(pcb->pid, estado_to_string(BLOCKED), estado_to_string(READY));
                procesos_en_ready_plus++;
                sem_post(&hay_proceso_a_ready);
                return;
            }

            logear_cambio_estado(pcb->pid, estado_to_string(BLOCKED), estado_to_string(READY));
            set_add_pcb_cola(pcb, READY, cola_procesos_ready, mutex_cola_de_readys);
            listar_procesos_en_ready();
            sem_post(&hay_proceso_a_ready);
        }
    }
}

bool existe_recurso(char *nombre)
{
    return dictionary_has_key(recursos_disponibles, nombre);
}

void liberar_recursos(uint32_t pid)
{
    t_dictionary *recursos_asignados = dictionary_get(recursos_asignados_por_proceso, (void *)(intptr_t)pid);
    t_list *recursos_asignados_lista = dictionary_elements(recursos_asignados);
    for (int i = 0; i < list_size(recursos_asignados_lista); i++)
    {
        t_recurso_asignado_a_proceso *recurso_asignado = list_get(recursos_asignados_lista, i);
        for (size_t i = 0; i < recurso_asignado->instancias_asignadas; i++)
        {
            sumar_instancia_a_recurso(recurso_asignado->nombre_recurso);
        }
    }
}

void retener_instancia_de_recurso(char *nombre_recurso, uint32_t pid)
{
    t_dictionary *recursos = dictionary_get(recursos_asignados_por_proceso, (void *)(intptr_t)pid);
    if (!dictionary_has_key(recursos, nombre_recurso))
    {
        dictionary_put(recursos, nombre_recurso, (void *)1);
    }
    else
    {
        dictionary_put(recursos, nombre_recurso, dictionary_get(recursos, nombre_recurso) + 1);
    }
}
