#include "../include/kernel.h"

void iniciar_planificador_corto_plazo()
{
    while (1)
    {
        sem_wait(&hay_proceso_a_ready);
        sem_wait(&cpu_libre);
        if (planificacion_detenida)
        {
            sem_wait(&podes_planificar_corto_plazo);
        }

        if (strcmp(algoritmo_planificacion, "FIFO") == 0)
        {
            log_info(logger_kernel, "algoritmo de planificacion: FIFO");

            pthread_mutex_lock(&mutex_cola_de_readys);
            t_pcb *pcb_a_ejecutar = queue_pop(cola_procesos_ready);
            pthread_mutex_unlock(&mutex_cola_de_readys);

            ejecutar_PCB(pcb_a_ejecutar);
        }
        else if (strcmp(algoritmo_planificacion, "RR") == 0)
        {
            log_info(logger_kernel, "algoritmo de planificacion: RR");

            pthread_mutex_lock(&mutex_cola_de_readys);
            t_pcb *pcb_a_ejecutar = queue_pop(cola_procesos_ready);
            pthread_mutex_unlock(&mutex_cola_de_readys);

            ejecutar_PCB(pcb_a_ejecutar);

            sem_post(&arrancar_quantum);
        }
        else if (strcmp(algoritmo_planificacion, "VRR") == 0)
        {
            log_info(logger_kernel, "algoritmo de planificacion: VRR");
            t_pcb *pcb_a_ejecutar;

            if (procesos_en_ready_plus > 0)
            {
                planificar_ready_plus = 1;
                pthread_mutex_lock(&mutex_cola_de_ready_plus);
                pcb_a_ejecutar = queue_pop(cola_ready_plus);
                pthread_mutex_unlock(&mutex_cola_de_ready_plus);
                procesos_en_ready_plus--;
            }
            else
            {
                planificar_ready_plus = 0;
                pthread_mutex_lock(&mutex_cola_de_readys);
                pcb_a_ejecutar = queue_pop(cola_procesos_ready);
                pthread_mutex_unlock(&mutex_cola_de_readys);
            }

            ejecutar_PCB(pcb_a_ejecutar);

            sem_post(&arrancar_quantum);
        }
        else
        {
            log_error(logger_kernel, "Algoritmo invalido");
        }
    }
}

void *verificar_quantum()
{
    t_temporal *tiempo_transcurrido;

    while (1)
    {

        sem_wait(&arrancar_quantum);

        pthread_mutex_lock(&mutex_flag_cpu_libre);
        flag_cpu_libre = 0;
        pthread_mutex_unlock(&mutex_flag_cpu_libre);

        tiempo_transcurrido = temporal_create();

        while (1)
        {
            usleep(1000);
            

            if (temporal_gettime(tiempo_transcurrido) >= quantum)
            {
                log_info(logger_kernel, "FIN DE QUANTUM: MANDO INTERRUPCION");

                pthread_mutex_lock(&mutex_flag_cpu_libre);
                enviar_interrupcion(pcb_en_ejecucion->pid, FIN_CLOCK, conexion_interrupt);
                flag_cpu_libre = 1;
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
                break;
            }

            pthread_mutex_lock(&mutex_flag_cpu_libre);
            if (flag_cpu_libre == 1)
            {
                switch (motivo_ultimo_desalojo)
                {
                case OPERACION_IO:
                    log_info(logger_kernel, "DESALOJÉ POR IO");
                    break;
                case WAIT_SOLICITADO:
                    log_info(logger_kernel, "DESALOJÉ POR WAIT");
                    break;
                case KILL_PROCESS:
                    log_info(logger_kernel, "DESALOJÉ POR KILL");
                    break;
                case END_PROCESS:
                    log_info(logger_kernel, "DESALOJÉ POR END");
                    break;
                default:
                    log_info(logger_kernel, "MOTIVO DESCONOCIDO DE DESALOJO");
                    break;
                }
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
                break;
            }
            pthread_mutex_unlock(&mutex_flag_cpu_libre);
        }

        temporal_destroy(tiempo_transcurrido);
    }

    return NULL;
}

void *verificar_quantum_vrr()
{
    t_temporal *tiempo_transcurrido;

    while (1)
    {
        sem_wait(&arrancar_quantum);

        pthread_mutex_lock(&mutex_flag_cpu_libre);
        flag_cpu_libre = 0;
        pthread_mutex_unlock(&mutex_flag_cpu_libre);

        tiempo_transcurrido = temporal_create();

        while (1)
        {
            usleep(1000);
            if (planificar_ready_plus == 1)
            {
                planificar_ready_plus = 0;
                if (temporal_gettime(tiempo_transcurrido) >= pcb_en_ejecucion->quantum)
                {
                    pcb_en_ejecucion->quantum = 0;
                    log_info(logger_kernel, "FIN DE QUANTUM: MANDO INTERRUPCION");

                    pthread_mutex_lock(&mutex_flag_cpu_libre);
                    enviar_interrupcion(pcb_en_ejecucion->pid, FIN_CLOCK, conexion_interrupt);
                    flag_cpu_libre = 1;
                    pthread_mutex_unlock(&mutex_flag_cpu_libre);
                    break;
                }

                pthread_mutex_lock(&mutex_flag_cpu_libre);
                if (flag_cpu_libre == 1)
                {
                    switch (motivo_ultimo_desalojo)
                    {
                    case OPERACION_IO:
                        ultimo_pcb_ejecutado->quantum = ultimo_pcb_ejecutado->quantum - temporal_gettime(tiempo_transcurrido);
                        log_info(logger_kernel, "DESALOJÉ POR IO");
                        break;
                    case WAIT_SOLICITADO:
                        sem_wait(&podes_revisar_lista_bloqueados);
                        if (buscar_pcb_por_pid(ultimo_pcb_ejecutado->pid, lista_procesos_blocked) != NULL)
                        {
                            ultimo_pcb_ejecutado->quantum -= temporal_gettime(tiempo_transcurrido);
                        }
                        log_info(logger_kernel, "DESALOJÉ POR WAIT");
                        break;
                    case END_PROCESS:
                        log_info(logger_kernel, "DESALOJÉ POR END");
                        break;
                    default:
                        log_info(logger_kernel, "MOTIVO DESCONOCIDO DE DESALOJO");
                        break;
                    }
                    pthread_mutex_unlock(&mutex_flag_cpu_libre);
                    break;
                }
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
            }
            else
            {
                if (temporal_gettime(tiempo_transcurrido) >= quantum)
                {
                    log_info(logger_kernel, "FIN DE QUANTUM: MANDO INTERRUPCION");
                    ultimo_pcb_ejecutado->quantum = 0;
                    pthread_mutex_lock(&mutex_flag_cpu_libre);
                    enviar_interrupcion(pcb_en_ejecucion->pid, FIN_CLOCK, conexion_interrupt);
                    flag_cpu_libre = 1;
                    pthread_mutex_unlock(&mutex_flag_cpu_libre);
                    break;
                }

                pthread_mutex_lock(&mutex_flag_cpu_libre);
                if (flag_cpu_libre == 1)
                {
                    switch (motivo_ultimo_desalojo)
                    {
                    case OPERACION_IO:
                        ultimo_pcb_ejecutado->quantum = quantum - temporal_gettime(tiempo_transcurrido);
                        log_info(logger_kernel, "DESALOJÉ POR IO");
                        break;
                    case WAIT_SOLICITADO:
                        sem_wait(&podes_revisar_lista_bloqueados);
                        // x las dudas, voy a esperar si lo carga o no en la lista de bloqueados
                        if (buscar_pcb_por_pid(ultimo_pcb_ejecutado->pid, lista_procesos_blocked) != NULL)
                        {
                            ultimo_pcb_ejecutado->quantum = quantum - temporal_gettime(tiempo_transcurrido);
                        }
                        log_info(logger_kernel, "DESALOJÉ POR WAIT");
                        break;
                    case END_PROCESS:
                        log_info(logger_kernel, "DESALOJÉ POR END");
                        break;
                    default:
                        log_info(logger_kernel, "MOTIVO DESCONOCIDO DE DESALOJO");
                        break;
                    }
                    pthread_mutex_unlock(&mutex_flag_cpu_libre);
                    break;
                }
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
            }
        }

        temporal_destroy(tiempo_transcurrido);
    }

    return NULL;
}

void iniciar_planificacion()
{
    planificacion_detenida = false;
    sem_post(&podes_crear_procesos);
    sem_post(&podes_eliminar_procesos);
    sem_post(&podes_planificar_corto_plazo);
    sem_post(&podes_manejar_desalojo);
}