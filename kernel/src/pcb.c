#include <../include/kernel.h>

void ejecutar_PCB(t_pcb *pcb)
{
    setear_pcb_en_ejecucion(pcb);
    enviar_pcb(pcb, conexion_dispatch);
    log_info(logger_kernel, "Se envio el PCB con PID %d a CPU Dispatch", pcb->pid);
}

void setear_pcb_en_ejecucion(t_pcb *pcb)
{
    logear_cambio_estado(pcb,pcb->estado_actual, EXEC);
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

void finalizar_pcb(t_pcb *pcb, op_code motivo) // Agrega al proceso a la cola de exits para q sea eliminado
{
    logear_cambio_estado(pcb, pcb->estado_actual, TERMINATED);
    t_proceso_en_exit *proceso = malloc(sizeof(t_proceso_en_exit));
    proceso->pcb = pcb;
    proceso->motivo = motivo;
    pthread_mutex_lock(&mutex_cola_de_exit);
    queue_push(cola_procesos_exit, proceso);
    pthread_mutex_unlock(&mutex_cola_de_exit);

    sem_post(&hay_proceso_exit);
}

void logear_bloqueo_proceso(uint32_t pid, char* motivo){
    log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pid, motivo);
}

void listar_procesos(){
    for (size_t i = 0; i < list_size(procesos_en_sistema); i++)
    {
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        t_pcb *pcb = list_get(procesos_en_sistema,i);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);
        log_info(logger_kernel, "PID: %d - ESTADO: %s", pcb->pid, estado_to_string(pcb->estado_actual));
    }
    
}

void listar_procesos_en_ready_plus(){
    log_info(logger_kernel, "Cola de Ready Prioridad:");
    for (size_t i = 0; i < queue_size(cola_ready_plus); i++)
    {
        pthread_mutex_lock(&mutex_cola_de_ready_plus);
        t_pcb *pcb = queue_pop(cola_ready_plus);
        queue_push(cola_ready_plus, pcb);
        pthread_mutex_unlock(&mutex_cola_de_ready_plus);
        log_info(logger_kernel, "PID: %d ", pcb->pid);
    }
}

void listar_procesos_en_ready(){
    log_info(logger_kernel, "------------------------------");
    log_info(logger_kernel, "Cola de Ready:");
    for (size_t i = 0; i < queue_size(cola_procesos_ready); i++)
    {
        pthread_mutex_lock(&mutex_cola_de_readys);
        t_pcb *pcb = queue_pop(cola_procesos_ready);
        queue_push(cola_procesos_ready, pcb);
        pthread_mutex_unlock(&mutex_cola_de_readys);
        log_info(logger_kernel, "+ PID: %d ", pcb->pid);
    }
    log_info(logger_kernel, "------------------------------");
}

void logear_cambio_estado(t_pcb *pcb, estados estado_anterior, estados estado_actual){
    char *estado_ant = estado_to_string(estado_anterior);
    char *estado_post = estado_to_string(estado_actual);
    log_info(logger_kernel, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->pid, estado_ant, estado_post);
}

uint32_t tener_index_pid(uint32_t pid){
    for (size_t i = 0; i < list_size(procesos_en_sistema); i++)
    {
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        t_pcb *pcb = list_get(procesos_en_sistema,i);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);
        if(pcb->pid == pid){
            return i;
        }
    }
    return -1;
}

