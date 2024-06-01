#include <../include/kernel.h>

void *recibir_dispatch()
{
    while (1)
    {
        op_code motivo_desalojo = recibir_motivo_desalojo(conexion_dispatch);
        t_pcb *pcb_actualizado = recibir_pcb(conexion_dispatch);

        pthread_mutex_lock(&mutex_motivo_ultimo_desalojo);
        motivo_ultimo_desalojo = motivo_desalojo;
        pthread_mutex_unlock(&mutex_motivo_ultimo_desalojo);

        switch (motivo_desalojo)
        {
        case OPERACION_IO:
            t_paquete *respuesta_kernel = recibir_paquete(conexion_dispatch);
            t_instruccion *utlima_instruccion = instruccion_deserializar(respuesta_kernel->buffer, 0);

            char *nombre_io = utlima_instruccion->parametros[0];
            if (!interfaz_conectada(nombre_io))
            {
                log_error(logger_kernel, "La interfaz %s no está conectada", nombre_io);
                exit(EXIT_FAILURE);
            }
            t_interfaz_en_kernel *interfaz = dictionary_get(conexiones_io, nombre_io);
            if (!esOperacionValida(utlima_instruccion->identificador, interfaz->tipo_interfaz))
            {
                log_error(logger_kernel, "La operacion %d no es valida para la interfaz %s", utlima_instruccion->identificador, nombre_io);
                exit(EXIT_FAILURE);
            }

            pthread_mutex_lock(&mutex_proceso_en_ejecucion);
            pcb_en_ejecucion = NULL;
            pthread_mutex_unlock(&mutex_proceso_en_ejecucion);

            pcb_actualizado->estado_actual = BLOCKED;
            logear_cambio_estado(pcb_actualizado->pid, "EXEC", "BLOCKED");

            pthread_mutex_lock(&mutex_lista_de_blocked);
            list_add(lista_procesos_blocked, pcb_actualizado);
            pthread_mutex_unlock(&mutex_lista_de_blocked);

            logear_bloqueo_proceso(pcb_actualizado->pid, nombre_io);

            sem_post(&contador_grado_multiprogramacion);

            // 1. La agregamos a la cola de blocks io. Ultima instrucción y PID
            t_instruccionEnIo *instruccion_en_io = malloc(sizeof(t_instruccionEnIo));
            instruccion_en_io->instruccion_io = utlima_instruccion;
            instruccion_en_io->pid = pcb_actualizado->pid;

            pthread_mutex_lock(&mutex_cola_interfaces); // estopy casi seguro que este mutex esta mal porque es el mismo que esta arriba y son listas distintas
            queue_push((t_queue *)dictionary_get(colas_blocks_io, nombre_io), instruccion_en_io);
            pthread_mutex_unlock(&mutex_cola_interfaces);

            t_semaforosIO *semaforos_interfaz = dictionary_get(diccionario_semaforos_io, nombre_io);
            sem_post(&semaforos_interfaz->instruccion_en_cola);

            sem_post(&cpu_libre);
            pthread_mutex_lock(&mutex_flag_cpu_libre);
            flag_cpu_libre = 1;
            pthread_mutex_unlock(&mutex_flag_cpu_libre);

            break;
        case FIN_CLOCK:
            log_info(logger_kernel, "entre al fin de clock por dispatcher");
            set_add_pcb_cola(pcb_actualizado, READY, cola_procesos_ready, mutex_cola_de_readys);
            sem_post(&hay_proceso_a_ready);

            logear_cambio_estado(pcb_actualizado->pid, "EXEC", "READY");

            sem_post(&cpu_libre);
            pthread_mutex_lock(&mutex_flag_cpu_libre);
            flag_cpu_libre = 1;
            pthread_mutex_unlock(&mutex_flag_cpu_libre);
            break;
        case KILL_PROCESS:
            // TODO
            break;
        case END_PROCESS:
            log_info(logger_kernel, "entre al end process por dispatcher");
            finalizar_pcb(pcb_actualizado);

            pthread_mutex_lock(&mutex_flag_cpu_libre);
            flag_cpu_libre = 1;
            pthread_mutex_unlock(&mutex_flag_cpu_libre);
            sem_post(&cpu_libre);
            break;
        case WAIT_SOLICITADO:
            t_paquete *respuesta = recibir_paquete(conexion_dispatch);
            t_instruccion *utlima = instruccion_deserializar(respuesta->buffer, 0);
            char *recurso_solicitado = utlima->parametros[0];

            if (!existe_recurso(recurso_solicitado))
            {
                finalizar_pcb(pcb_actualizado);

                pthread_mutex_lock(&mutex_flag_cpu_libre);
                flag_cpu_libre = 1;
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
                sem_post(&cpu_libre);
                break;
            }
            int32_t instancias_restantes = restar_instancia_a_recurso(recurso_solicitado);
            retener_instancia_de_recurso(recurso_solicitado, pcb_actualizado->pid);
            if (instancias_restantes < 0)
            {
                agregar_pcb_a_cola_bloqueados_de_recurso(pcb_actualizado, recurso_solicitado);

                pcb_actualizado->estado_actual = BLOCKED;
                logear_cambio_estado(pcb_actualizado->pid, "EXEC", "BLOCKED");

                sem_post(&cpu_libre);
                pthread_mutex_lock(&mutex_flag_cpu_libre);
                flag_cpu_libre = 1;
                pthread_mutex_unlock(&mutex_flag_cpu_libre);

                logear_bloqueo_proceso(pcb_actualizado->pid, recurso_solicitado);
                sem_post(&contador_grado_multiprogramacion);
            }
            else
            {
                setear_pcb_en_ejecucion(pcb_actualizado);
            }
            break;
        case SIGNAL_SOLICITADO:
            t_paquete *respuesta_signal = recibir_paquete(conexion_dispatch);
            t_instruccion *instruccion_signal = instruccion_deserializar(respuesta_signal->buffer, 0);
            char *recurso_signal = instruccion_signal->parametros[0];

            if (!existe_recurso(recurso_signal))
            {
                finalizar_pcb(pcb_actualizado);

                pthread_mutex_lock(&mutex_flag_cpu_libre);
                flag_cpu_libre = 1;
                pthread_mutex_unlock(&mutex_flag_cpu_libre);
                sem_post(&cpu_libre);
                break;
            }
            sumar_instancia_a_recurso(recurso_solicitado);
            setear_pcb_en_ejecucion(pcb_actualizado);
        default:
            break;
        }
    }
}
