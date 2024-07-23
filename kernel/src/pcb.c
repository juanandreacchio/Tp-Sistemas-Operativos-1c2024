#include <../include/kernel.h>

void ejecutar_PCB(t_pcb *pcb)
{
    setear_pcb_en_ejecucion(pcb);
    log_info(logger_kernel, "Se envio el PCB con PID %d a CPU Dispatch", pcb->pid);
    enviar_pcb(pcb, conexion_dispatch);
}

void setear_pcb_en_ejecucion(t_pcb *pcb)
{
    logear_cambio_estado(pcb, pcb->estado_actual, EXEC);
    pcb->estado_actual = EXEC;

    if(pcb_en_ejecucion != NULL){
        destruir_pcb(pcb_en_ejecucion);
    }
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

    if (cola == cola_procesos_ready)
    {
        listar_procesos_en_ready();
    }
    if (cola == cola_ready_plus){
        listar_procesos_en_ready_plus();
    }
    
    
}

t_pcb *buscar_pcb_por_pid(u_int32_t pid, t_list *lista)
{
    t_pcb *pcb;
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

void finalizar_pcb(t_pcb *pcb, op_code motivo)
{
    logear_cambio_estado(pcb, pcb->estado_actual, TERMINATED);
    pcb->estado_actual = TERMINATED;
    actualizar_pcb_en_procesos_del_sistema(pcb);

    t_proceso_en_exit *proceso = malloc(sizeof(t_proceso_en_exit));
    proceso->pcb = pcb;
    proceso->motivo = motivo;
    pthread_mutex_lock(&mutex_cola_de_exit);
    queue_push(cola_procesos_exit, proceso);
    pthread_mutex_unlock(&mutex_cola_de_exit);

    sem_post(&hay_proceso_exit);
}

void logear_bloqueo_proceso(uint32_t pid, char *motivo)
{
    log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pid, motivo);
}

void listar_procesos()
{
    for (size_t i = 0; i < list_size(procesos_en_sistema); i++)
    {
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        t_pcb *pcb = list_get(procesos_en_sistema, i);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);
        log_info(logger_kernel, "PID: %d - ESTADO: %s", pcb->pid, estado_to_string(pcb->estado_actual));
    }
}

void listar_procesos_en_ready_plus()
{
    log_info(logger_kernel, "------------------------------");
    log_info(logger_kernel, "Cola de Ready Prioridad:");
    pthread_mutex_lock(&mutex_cola_de_ready_plus);
    for (size_t i = 0; i < queue_size(cola_ready_plus); i++)
    {

        t_pcb *pcb = queue_pop(cola_ready_plus);
        queue_push(cola_ready_plus, pcb);

        log_info(logger_kernel, "PID: %d ", pcb->pid);
    }
    pthread_mutex_unlock(&mutex_cola_de_ready_plus);
    log_info(logger_kernel, "------------------------------");
}

void listar_procesos_en_ready()
{
    log_info(logger_kernel, "------------------------------");
    log_info(logger_kernel, "Cola de Ready:");
    pthread_mutex_lock(&mutex_cola_de_readys);
    for (size_t i = 0; i < queue_size(cola_procesos_ready); i++)
    {

        t_pcb *pcb = queue_pop(cola_procesos_ready);
        queue_push(cola_procesos_ready, pcb);

        log_info(logger_kernel, "+ PID: %d ", pcb->pid);
    }
    pthread_mutex_unlock(&mutex_cola_de_readys);
    log_info(logger_kernel, "------------------------------");
}

void logear_cambio_estado(t_pcb *pcb, estados estado_anterior, estados estado_actual)
{
    char *estado_ant = estado_to_string(estado_anterior);
    char *estado_post = estado_to_string(estado_actual);
    log_info(logger_kernel, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->pid, estado_ant, estado_post);
}

uint32_t tener_index_pid(uint32_t pid)
{
    for (size_t i = 0; i < list_size(procesos_en_sistema); i++)
    {
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        t_pcb *pcb = list_get(procesos_en_sistema, i);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);
        if (pcb->pid == pid)
        {
            return i;
        }
    }
    return -1;
}

void bloquear_pcb(t_pcb *pcb)
{

    pcb->estado_actual = BLOCKED;

    pthread_mutex_lock(&mutex_lista_de_blocked);
    list_add(lista_procesos_blocked, pcb);
    pthread_mutex_unlock(&mutex_lista_de_blocked);
    actualizar_pcb_en_procesos_del_sistema(pcb);

    logear_lista_blocked();
}

void logear_lista_blocked()
{
    log_info(logger_kernel, "Procesos en cola de Blocked:");
    for (size_t i = 0; i < list_size(lista_procesos_blocked); i++)
    {
        pthread_mutex_lock(&mutex_lista_de_blocked);
        t_pcb *pcb = list_get(lista_procesos_blocked, i);
        pthread_mutex_unlock(&mutex_lista_de_blocked);
        log_info(logger_kernel, "PID: %d - ESTADO: %s", pcb->pid, estado_to_string(pcb->estado_actual));
    }
}

t_pcb *buscar_pcb_en_procesos_del_sistema(uint32_t pid)
{
    for (size_t i = 0; i < list_size(procesos_en_sistema); i++)
    {
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        t_pcb *pcb = list_get(procesos_en_sistema, i);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);
        if (pcb->pid == pid)
        {
            return pcb;
        }
    }
    return NULL;
}

void actualizar_pcb_en_procesos_del_sistema(t_pcb *pcb_actualizado)
{
    t_pcb *old = buscar_pcb_en_procesos_del_sistema(pcb_actualizado->pid);
    if (old != NULL)
    {
        uint32_t index = tener_index_pid(pcb_actualizado->pid);
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        t_pcb *pcb_viejo = list_replace(procesos_en_sistema, index, pcb_actualizado);
        free(pcb_viejo);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);
    }
}

uint32_t buscar_index_pid_bloqueado(uint32_t pid)
{
    for (size_t i = 0; i < list_size(lista_procesos_blocked); i++)
    {
        pthread_mutex_lock(&mutex_lista_de_blocked);
        t_pcb *pcb = list_get(lista_procesos_blocked, i);
        pthread_mutex_unlock(&mutex_lista_de_blocked);
        if (pcb->pid == pid)
        {
            return i;
        }
    }
    return -1;
}

bool tiene_mismo_pid(uint32_t pid1, uint32_t pid2){
    return pid1 == pid2;
}

void agregar_pid_alista(){
    
}