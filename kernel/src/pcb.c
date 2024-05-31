#include <../include/kernel.h>

void ejecutar_PCB(t_pcb *pcb)
{
    setear_pcb_en_ejecucion(pcb);
    enviar_pcb(pcb, conexion_dispatch);
    log_info(logger_kernel, "Se envio el PCB con PID %d a CPU Dispatch", pcb->pid);
}

void setear_pcb_en_ejecucion(t_pcb *pcb)
{
    pcb->estado_actual = EXEC;
    logear_cambio_estado(pcb->pid, estado_to_string(pcb->estado_actual), "EXEC");

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

void logear_bloqueo_proceso(uint32_t pid, char* motivo){
    log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pid, motivo);
}

void logear_cambio_estado(uint32_t pid, char* estado_anterior, char * estado_actual){
    log_info(logger_kernel, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pid, estado_anterior, estado_actual);
}