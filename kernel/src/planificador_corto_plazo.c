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
            log_info(logger_kernel, "PID: %d - EJECUTANDO", pcb_a_ejecutar->pid);
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

            pthread_mutex_lock(&mutex_cola_de_ready_plus);
            uint32_t cantidad_de_procesos_plus = queue_size(cola_ready_plus);
            pthread_mutex_unlock(&mutex_cola_de_ready_plus);

            if (cantidad_de_procesos_plus > 0)
            {
                pthread_mutex_lock(&mutex_cola_de_ready_plus);
                pcb_a_ejecutar = queue_pop(cola_ready_plus);
                pthread_mutex_unlock(&mutex_cola_de_ready_plus);
            }
            else
            {

                pthread_mutex_lock(&mutex_cola_de_readys);
                pcb_a_ejecutar = queue_pop(cola_procesos_ready);
                pthread_mutex_unlock(&mutex_cola_de_readys);


                pcb_a_ejecutar->quantum = quantum;
            }
            log_info(logger_kernel, "El PID: %d tiene para ejecutar: %d", pcb_a_ejecutar->pid, pcb_a_ejecutar->quantum);
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
    struct timespec ts;
    t_temporal *tiempo_transcurrido;

    while (1)
    {
        // Espera hasta que arrancar_quantum esté disponible
        sem_wait(&arrancar_quantum);

        // Bloqueo y actualización de la bandera
        pthread_mutex_lock(&mutex_flag_cpu_libre);
        flag_cpu_libre = 0;
        pthread_mutex_unlock(&mutex_flag_cpu_libre);

        // Crear temporizador
        tiempo_transcurrido = temporal_create();

        // Obtener el tiempo actual y calcular el límite de tiempo
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += quantum / 1000;
        ts.tv_nsec += (quantum % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000)
        {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        // Bloqueo para esperar la señal de condición o el tiempo límite
        pthread_mutex_lock(&mutex_flag_cpu_libre);
        while (flag_cpu_libre == 0)
        {
            int wait_result = pthread_cond_timedwait(&cond_flag_cpu_libre, &mutex_flag_cpu_libre, &ts);
            if (wait_result == ETIMEDOUT)
            {
                // Si el tiempo límite se alcanza, romper el bucle
                break;
            }
            else if (wait_result != 0)
            {
                // Manejar otros posibles errores de pthread_cond_timedwait
                log_error(logger_kernel, "Error en pthread_cond_timedwait");
                break;
            }
        }
        
        // Manejar el caso cuando la CPU está libre o el tiempo se ha agotado
        if (flag_cpu_libre == 1)
        {
            flag_cpu_libre = 0;
        }
        else
        {
            pcb_en_ejecucion->quantum = 0;
            enviar_interrupcion(pcb_en_ejecucion->pid, FIN_CLOCK, conexion_interrupt);
        }

        // Desbloquear el mutex y destruir el temporizador
        pthread_mutex_unlock(&mutex_flag_cpu_libre);
        temporal_destroy(tiempo_transcurrido);
    }
}

void *verificar_quantum_vrr()
{
    struct timespec ts;
    t_temporal *tiempo_transcurrido;

    while (1)
    {
        sem_wait(&arrancar_quantum);

        pthread_mutex_lock(&mutex_flag_cpu_libre);
        flag_cpu_libre = 0;
        pthread_mutex_unlock(&mutex_flag_cpu_libre);

        tiempo_transcurrido = temporal_create();

        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += pcb_en_ejecucion->quantum / 1000;
        ts.tv_nsec += (pcb_en_ejecucion->quantum % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000)
        {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        pthread_mutex_lock(&mutex_flag_cpu_libre);
        while (flag_cpu_libre == 0)
        {
            int wait_result = pthread_cond_timedwait(&cond_flag_cpu_libre, &mutex_flag_cpu_libre, &ts);
            if (wait_result == ETIMEDOUT)
            {
                // Si el tiempo límite se alcanza, romper el bucle
                break;
            }
            else if (wait_result != 0)
            {
                // Manejar otros posibles errores de pthread_cond_timedwait
                log_error(logger_kernel, "Error en pthread_cond_timedwait");
                break;
            }
        }

        // Manejar el caso cuando la CPU está libre o el tiempo se ha agotado
        if (flag_cpu_libre == 1)
        {
            log_info(logger_kernel,"entre por el caso de que la cpu libre estaa en 1");
            flag_cpu_libre = 0;
            if (temporal_gettime(tiempo_transcurrido) >= ultimo_pcb_ejecutado->quantum)
            {
                ultimo_pcb_ejecutado->quantum = 0;
            }
            else
            {
                ultimo_pcb_ejecutado->quantum -= temporal_gettime(tiempo_transcurrido);
                log_info(logger_kernel, "PID: %d - Quantum restante: %d", ultimo_pcb_ejecutado->pid, ultimo_pcb_ejecutado->quantum);
            }
        }
        else
        {
            log_info(logger_kernel,"entre por el caso de que la cpu libre estaa en 0");
            pcb_en_ejecucion->quantum = 0;
            enviar_interrupcion(pcb_en_ejecucion->pid, FIN_CLOCK, conexion_interrupt);
        }

        // Desbloquear el mutex y destruir el temporizador
        pthread_mutex_unlock(&mutex_flag_cpu_libre);
        temporal_destroy(tiempo_transcurrido);
    }
}

void iniciar_planificacion()
{
    planificacion_detenida = false;
    sem_post(&podes_crear_procesos);
    sem_post(&podes_eliminar_procesos);
    sem_post(&podes_planificar_corto_plazo);
    sem_post(&podes_manejar_desalojo);
}