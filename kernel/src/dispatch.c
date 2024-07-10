#include <../include/kernel.h>

void *recibir_dispatch()
{
    while (1)
    {
        op_code motivo_desalojo = recibir_motivo_desalojo(conexion_dispatch);
        t_pcb *pcb_actualizado = recibir_pcb(conexion_dispatch);

        if (planificacion_detenida)
        {
            sem_wait(&podes_manejar_desalojo);
        }
        

        pthread_mutex_lock(&mutex_motivo_ultimo_desalojo);
        motivo_ultimo_desalojo = motivo_desalojo;
        pthread_mutex_unlock(&mutex_motivo_ultimo_desalojo);

        ultimo_pcb_ejecutado = pcb_actualizado; // poner mutex

        switch (motivo_desalojo)
        {
        case OPERACION_IO:
            t_paquete *respuesta_kernel = recibir_paquete(conexion_dispatch);
            u_int32_t nombre_length;
            t_identificador identificador;
            buffer_read(respuesta_kernel->buffer,&identificador,sizeof(t_identificador));
            buffer_read(respuesta_kernel->buffer,&nombre_length,sizeof(u_int32_t));
            char *nombre_io = malloc(nombre_length + 1);
            buffer_read(respuesta_kernel->buffer, nombre_io, nombre_length);

            if (!interfaz_conectada(nombre_io))
            {
                log_error(logger_kernel, "La interfaz %s no está conectada", nombre_io);
                exit(EXIT_FAILURE);
            }
            t_interfaz_en_kernel *interfaz = dictionary_get(conexiones_io, nombre_io);
            if (!esOperacionValida(identificador, interfaz->tipo_interfaz))
            {
                log_error(logger_kernel, "La operacion %d no es valida para la interfaz %s", identificador, nombre_io);
                exit(EXIT_FAILURE);
            }

            //esto va?
            pthread_mutex_lock(&mutex_proceso_en_ejecucion);
            pcb_en_ejecucion = NULL;
            pthread_mutex_unlock(&mutex_proceso_en_ejecucion);


            pcb_actualizado->estado_actual = BLOCKED;
            logear_cambio_estado(pcb_actualizado, EXEC, BLOCKED);


            pthread_mutex_lock(&mutex_lista_de_blocked);
            list_add(lista_procesos_blocked, pcb_actualizado);
            pthread_mutex_unlock(&mutex_lista_de_blocked);

            logear_bloqueo_proceso(pcb_actualizado->pid, nombre_io);

            signal_contador(semaforo_multi);
            
            // 1. La agregamos a la cola de blocks io. datos necesarios para ahhacer el io y PID
            t_info_en_io *info_io = malloc(sizeof(t_info_en_io));
            buffer_read(respuesta_kernel->buffer,&info_io->tam_info,sizeof(u_int32_t));
            info_io->info_necesaria = malloc(info_io->tam_info);
            buffer_read(respuesta_kernel->buffer,info_io->info_necesaria,info_io->tam_info);
            info_io->pid = pcb_actualizado->pid;

            t_cola_interfaz_io *cola_interfaz = dictionary_get(colas_blocks_io, nombre_io);

            pthread_mutex_lock(&cola_interfaz->mutex);
            queue_push(cola_interfaz->cola, info_io);
            pthread_mutex_unlock(&cola_interfaz->mutex);

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
            listar_procesos_en_ready();
            sem_post(&hay_proceso_a_ready);

            logear_cambio_estado(pcb_actualizado, EXEC, READY);

            sem_post(&cpu_libre);
            pthread_mutex_lock(&mutex_flag_cpu_libre);
            flag_cpu_libre = 1;
            pthread_mutex_unlock(&mutex_flag_cpu_libre);
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
                sem_post(&podes_revisar_lista_bloqueados);
                sem_post(&cpu_libre);
                break;
            }
            int32_t instancias_restantes = restar_instancia_a_recurso(recurso_solicitado);
            retener_instancia_de_recurso(recurso_solicitado, pcb_actualizado->pid);
            if (instancias_restantes < 0)
            {
                agregar_pcb_a_cola_bloqueados_de_recurso(pcb_actualizado, recurso_solicitado);

                pcb_actualizado->estado_actual = BLOCKED;
                logear_cambio_estado(pcb_actualizado, EXEC, BLOCKED);

                pthread_mutex_lock(&mutex_flag_cpu_libre);
                flag_cpu_libre = 1;
                pthread_mutex_unlock(&mutex_flag_cpu_libre);

                logear_bloqueo_proceso(pcb_actualizado->pid, recurso_solicitado);

                sem_post(&cpu_libre);
                signal_contador(semaforo_multi);
            }
            else
            {
                setear_pcb_en_ejecucion(pcb_actualizado);
            }
            sem_post(&podes_revisar_lista_bloqueados);

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
            break;
        case KILL_PROCESS:
            liberar_recursos(pcb_actualizado->pid);

            signal_contador(semaforo_multi);
            destruir_pcb(pcb_actualizado);
        default:
            break;
        }
    }
}
