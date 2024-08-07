#include "../include/kernel.h"

void *iniciar_consola_interactiva()
{
    char *comandoRecibido;
    comandoRecibido = readline("> ");

    while (strcmp(comandoRecibido, "") != 0)
    {
        if (!validar_comando(comandoRecibido))
        {
            log_error(logger_kernel, "Comando de CONSOLA no reconocido");
            free(comandoRecibido);
            comandoRecibido = readline("> ");
            continue; // se saltea el resto del while y vuelve a empezar
        }

        ejecutar_comando(comandoRecibido);
        free(comandoRecibido);
        comandoRecibido = readline("> ");
    }

    free(comandoRecibido);
    return NULL;
}

bool validar_comando(char *comando)
{
    bool validacion = false;
    char **consola = string_split(comando, " ");
    if (strcmp(consola[0], "EJECUTAR_SCRIPT") == 0)
        validacion = true;
    else if (strcmp(consola[0], "INICIAR_PROCESO") == 0)
        validacion = true;
    else if (strcmp(consola[0], "FINALIZAR_PROCESO") == 0)
        validacion = true;
    else if (strcmp(consola[0], "DETENER_PLANIFICACION") == 0)
        validacion = true;
    else if (strcmp(consola[0], "INICIAR_PLANIFICACION") == 0)
        validacion = true;
    else if (strcmp(consola[0], "MULTIPROGRAMACION") == 0)
        validacion = true;
    else if (strcmp(consola[0], "PROCESO_ESTADO") == 0)
        validacion = true;

    string_array_destroy(consola);
    return validacion;
}

void ejecutar_comando(char *comando)
{
    char **consola = string_split(comando, " ");

    if (strcmp(consola[0], "EJECUTAR_SCRIPT") == 0)
    {
        char *base_path = "/home/utnso/c-comenta-pruebas";
        // Calcula el tamaño necesario para la cadena final
        size_t path_size = strlen(base_path) + strlen(consola[1]) + 1; // +1 para el carácter nulo
        char *full_path = malloc(path_size);
        if (full_path == NULL)
        {
            fprintf(stderr, "Error al asignar memoria\n");
            return;
        }

        // Construye la cadena completa
        strcpy(full_path, base_path);
        strcat(full_path, consola[1]);

        // log_info(logger_kernel,"el path es: %s",full_path);
        FILE *archivo = fopen(full_path, "r");
        if (archivo == NULL)
        {
            log_error(logger_kernel, "No se pudo abrir el archivo");
            free(full_path);
            string_array_destroy(consola);
            return;
        }
        char *linea = malloc(100);
        while (fgets(linea, 100, archivo) != NULL)
        {
            linea[strcspn(linea, "\n")] = 0;
            ejecutar_comando(linea);
        }
        free(linea);
        free(full_path);
        fclose(archivo);
    }
    else if (strcmp(consola[0], "INICIAR_PROCESO") == 0)
    {
        char *path = consola[1];
        // ---------------------- CREAR PCB EN EL KERNEL -------------------

        t_pcb *pcb_creado = crear_pcb(contador_pid, quantum, NEW);

        // ---------------------- ENVIAR SOLICITUD PARA QUE SE CREE EN MEMORIA -------------------
        t_paquete *paquete = crear_paquete(CREAR_PROCESO);
        t_solicitudCreacionProcesoEnMemoria *ptr_solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));
        ptr_solicitud->pid = contador_pid;
        ptr_solicitud->path_length = strlen(path) + 1;
        ptr_solicitud->path = path;

        dictionary_put(recursos_asignados_por_proceso, string_itoa(contador_pid), list_create());

        buffer_add(paquete->buffer, &ptr_solicitud->pid, sizeof(uint32_t));
	    buffer_add(paquete->buffer, &ptr_solicitud->path_length, sizeof(uint32_t));
	    buffer_add(paquete->buffer, ptr_solicitud->path, ptr_solicitud->path_length);

        enviar_paquete(paquete, conexion_memoria);
        eliminar_paquete(paquete);
        free(ptr_solicitud);
        if (recibir_operacion(conexion_memoria) != CREAR_PROCESO)
        {
            log_error(logger_kernel, "error: no c recibio correctamente la respuesta de creacion de proceso de memoria");
            return;
        }

        set_add_pcb_cola(pcb_creado, NEW, cola_procesos_new, mutex_cola_de_new);

        sem_post(&hay_proceso_nuevo);

        log_info(logger_kernel, "Se crea el proceso %d en NEW", pcb_creado->pid);

        pthread_mutex_lock(&mutex_procesos_en_sistema);
        list_add(procesos_en_sistema, pcb_creado);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);

        pthread_mutex_lock(&mutex_pid);
        contador_pid++;
        pthread_mutex_unlock(&mutex_pid);

        
    }
    else if (strcmp(consola[0], "FINALIZAR_PROCESO") == 0)
    {
        uint32_t pid = atoi(consola[1]);

        uint32_t index = tener_index_pid(pid);
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        t_pcb *pcb = list_get(procesos_en_sistema, index);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);
        //log_info(logger_kernel,"entre a finalziar proceso ");
        if (index == -1)
        {
            log_error(logger_kernel, "No se encontro el proceso con PID %d en el sistema", pid);
            return;
        }

        switch (pcb->estado_actual)
        {
        case NEW:
            for (size_t i = 0; i < queue_size(cola_procesos_new); i++)
            {
                pthread_mutex_lock(&mutex_cola_de_new);
                t_pcb *pcb = queue_pop(cola_procesos_new);
                if (pcb->pid != pid)
                {
                    queue_push(cola_procesos_new, pcb);
                }
                else
                {
                    sem_wait(&hay_proceso_nuevo);
                    logear_cambio_estado(pcb, NEW, TERMINATED);
                    pthread_mutex_unlock(&mutex_cola_de_new);
                    break;
                }
                pthread_mutex_unlock(&mutex_cola_de_new);
            }

            break;
        case READY:
        //log_info(logger_kernel,"entre por el case de ready ");
            bool eliminado = false;
            pthread_mutex_lock(&mutex_cola_de_readys);
            for (size_t i = 0; i < queue_size(cola_procesos_ready); i++)
            {

                t_pcb *pcb = queue_pop(cola_procesos_ready);
                if (pcb->pid != pid)
                {
                    queue_push(cola_procesos_ready, pcb);
                }
                else
                {
                    eliminado = true;
                    sem_wait(&hay_proceso_a_ready);
                    logear_cambio_estado(pcb, READY, TERMINATED);
                    if (flag_grado_multi == 0)
                    {
                        sem_post(&grado_multi);
                    }
                    else
                    {
                        flag_grado_multi -= 1;
                    }
                }
            }
            pthread_mutex_unlock(&mutex_cola_de_readys);

            if (!eliminado)
            {
                pthread_mutex_lock(&mutex_cola_de_ready_plus);
                for (size_t i = 0; i < queue_size(cola_ready_plus); i++)
                {

                    t_pcb *pcb = queue_pop(cola_ready_plus);
                    if (pcb->pid != pid)
                    {
                        queue_push(cola_ready_plus, pcb);
                    }
                    else
                    {
                        eliminado = true;
                        sem_wait(&hay_proceso_a_ready);
                        if (flag_grado_multi == 0)
                        {
                            sem_post(&grado_multi);
                        }
                        else
                        {
                            flag_grado_multi -= 1;
                        }
                        logear_cambio_estado(pcb, READY, TERMINATED);
                    }
                }
                pthread_mutex_unlock(&mutex_cola_de_ready_plus);
            }

            break;
        case EXEC:
            enviar_interrupcion(pcb->pid, KILL_PROCESS, conexion_interrupt);
            sem_wait(&podes_eliminar_loko);

            break;
        case BLOCKED:
            for (size_t i = 0; i < list_size(lista_procesos_blocked); i++)
            {
                pthread_mutex_lock(&mutex_lista_de_blocked);
                t_pcb *pcb = list_get(lista_procesos_blocked, i);
                if (pcb->pid == pid)
                {
                    list_remove(lista_procesos_blocked, i);
                    pthread_mutex_unlock(&mutex_lista_de_blocked);
                    logear_cambio_estado(pcb, BLOCKED, TERMINATED);
                    if (flag_grado_multi == 0)
                    {
                        sem_post(&grado_multi);
                    }
                    else
                    {
                        flag_grado_multi -= 1;
                    }
                    break;
                }
                pthread_mutex_unlock(&mutex_lista_de_blocked);
            }
            break;
        default:
            break;
        }

        pthread_mutex_lock(&mutex_procesos_en_sistema);
        list_remove(procesos_en_sistema, index);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);
        
        liberar_recursos(pid);

        t_paquete *paquete = crear_paquete(END_PROCESS);
        buffer_add(paquete->buffer, &pid, sizeof(uint32_t));
        enviar_paquete(paquete, conexion_memoria);
        eliminar_paquete(paquete);
        if (recibir_operacion(conexion_memoria) != END_PROCESS)
        {
            log_error(logger_kernel, "error de end process del PID %d", pid);
            return;
        }

        if (pcb->estado_actual == EXEC)
        {
            logear_cambio_estado(pcb, EXEC, TERMINATED);
            sem_post(&cpu_libre);
            if (flag_grado_multi == 0)
            {
                sem_post(&grado_multi);
            }
            else
            {
                flag_grado_multi -= 1;
            }
            pthread_mutex_lock(&mutex_flag_cpu_libre);
            flag_cpu_libre = 1;
            pthread_cond_signal(&cond_flag_cpu_libre);
            pthread_mutex_unlock(&mutex_flag_cpu_libre);
        }
        /*
        if (pcb->estado_actual != BLOCKED)
        {
            signal_contador(semaforo_multi);
        }
        */
        destruir_pcb(pcb);

        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: %s", pid, op_code_to_string(INTERRUPTED_BY_USER));

        return;
    }
    else if (strcmp(consola[0], "MULTIPROGRAMACION") == 0)
    {
        uint32_t nuevo_grado = atoi(consola[1]);
        if (grado_multiprogramacion < nuevo_grado)
        {
            u_int32_t cant_a_sumar = nuevo_grado - grado_multiprogramacion;
            grado_multiprogramacion = grado_multiprogramacion + cant_a_sumar;
            for (size_t i = 0; i < cant_a_sumar; i++)
            {
                sem_post(&grado_multi);
            }
        }
        else
        {
            u_int32_t cant_a_eliminar = grado_multiprogramacion - nuevo_grado;
            int value;
            sem_getvalue(&grado_multi, &value);
            if (value >= cant_a_eliminar)
            {
                for (size_t i = 0; i < cant_a_eliminar; i++)
                {
                    sem_wait(&grado_multi);
                }
            }
            else
            {
                //log_info(logger_kernel,"entre al else ");
                for (size_t i = 0; i < value; i++)
                {
                    sem_wait(&grado_multi);
                }
                flag_grado_multi = cant_a_eliminar - value;
                //log_info(logger_kernel,"el flag_grado_multi es: %d",flag_grado_multi);
            }
            grado_multiprogramacion = grado_multiprogramacion - cant_a_eliminar;
            //log_info(logger_kernel,"el grado multi actual es: %d",grado_multiprogramacion);

            // eliminar_procesos(cant_a_eliminar);
        }

        // cambiar_grado(nuevo_grado);
    }
    else if (strcmp(consola[0], "PROCESO_ESTADO") == 0)
    {
        listar_procesos();
    }
    else if (strcmp(consola[0], "DETENER_PLANIFICACION") == 0)
    {
        planificacion_detenida = true;
    }
    // detener_planificacion();
    else if (strcmp(consola[0], "INICIAR_PLANIFICACION") == 0)
    {
        if (planificacion_detenida)
        {
            iniciar_planificacion();
        }
    }
    else
    {
        log_error(logger_kernel, "comando no reconocido, a pesar de que entro al while");
        exit(EXIT_FAILURE);
    }
    string_array_destroy(consola);
}
