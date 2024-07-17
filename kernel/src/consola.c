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
        char base_path[] = "/home/utnso";
        // Calcula el tamaño necesario para la cadena final
        size_t path_size = strlen(base_path) + strlen(consola[1]) + 1; // +1 para el carácter nulo
        char *full_path = (char *)malloc(path_size);
        if (full_path == NULL) {
            fprintf(stderr, "Error al asignar memoria\n");
            return;
        }

        // Construye la cadena completa
        strcpy(full_path, base_path);
        strcat(full_path, consola[1]);

        //log_info(logger_kernel,"el path es: %s",full_path);
        FILE *archivo = fopen(full_path, "r");
        if (archivo == NULL)
        {
            log_error(logger_kernel, "No se pudo abrir el archivo");
            exit(EXIT_FAILURE);
        }
        char *linea = malloc(100);
        while (fgets(linea, 100, archivo) != NULL)
        {
            linea[strcspn(linea,"\n")] = 0;
            ejecutar_comando(linea);
        }
        free(linea);
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

        t_buffer *buffer = serializar_solicitud_crear_proceso(ptr_solicitud);
        paquete->buffer = buffer;

        enviar_paquete(paquete, conexion_memoria);
        if(recibir_operacion(conexion_memoria)!= CREAR_PROCESO)
            log_error(logger_kernel,"error: no c recibio correctamente la respuesta de creacion de proceso de memoria");
        set_add_pcb_cola(pcb_creado, NEW, cola_procesos_new, mutex_cola_de_new);

        sem_post(&hay_proceso_nuevo);

        log_info(logger_kernel, "Se crea el proceso %d en NEW", pcb_creado->pid);

        pthread_mutex_lock(&mutex_procesos_en_sistema);
        list_add(procesos_en_sistema, pcb_creado);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);

        pthread_mutex_lock(&mutex_pid);
        contador_pid++;
        pthread_mutex_unlock(&mutex_pid);

        eliminar_paquete(paquete);
    }
    else if (strcmp(consola[0], "FINALIZAR_PROCESO") == 0)
    {
       uint32_t pid = atoi(consola[1]);

        t_paquete *paquete = crear_paquete(FINALIZAR_PROCESO);
        buffer_add(paquete->buffer, &pid, sizeof(uint32_t));
        enviar_paquete(paquete, conexion_memoria);

        uint32_t index = tener_index_pid(pid);
        pthread_mutex_lock(&mutex_procesos_en_sistema);
        t_pcb *pcb = list_get(procesos_en_sistema, index);
        pthread_mutex_unlock(&mutex_procesos_en_sistema);

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
                    pthread_mutex_unlock(&mutex_cola_de_new);
                    break;
                }
                pthread_mutex_unlock(&mutex_cola_de_new);
            }

            break;
        case READY:
            for (size_t i = 0; i < queue_size(cola_procesos_ready); i++)
            {
                pthread_mutex_lock(&mutex_cola_de_readys);
                t_pcb *pcb = queue_pop(cola_procesos_ready);
                if (pcb->pid != pid)
                {
                    queue_push(cola_procesos_ready, pcb);
                }
                else
                {
                    sem_wait(&hay_proceso_a_ready);
                }
                pthread_mutex_unlock(&mutex_cola_de_readys);
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

        log_info(logger_kernel, "Se liberaron los recursos del proceso %d", pid);

        if (semaforo_multi->valor_maximo != semaforo_multi->valor_actual)
        {
            signal_contador(semaforo_multi);
        }

        destruir_pcb(pcb);

        log_info(logger_kernel, "------------------------------");
        log_info(logger_kernel, "Se finaliza el proceso %d", pid);
        log_info(logger_kernel, "------------------------------");


        sem_post(&cpu_libre);
        return;
    }
    else if (strcmp(consola[0], "MULTIPROGRAMACION") == 0)
    {
        uint32_t nuevo_grado = atoi(consola[1]);
        cambiar_grado(nuevo_grado);
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
        iniciar_planificacion();
    }
    else
    {
        log_error(logger_kernel, "comando no reconocido, a pesar de que entro al while");
        exit(EXIT_FAILURE);
    }
    string_array_destroy(consola);
}
