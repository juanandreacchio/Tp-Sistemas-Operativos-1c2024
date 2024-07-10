#include "../include/kernel.h"

bool interfaz_conectada(char *nombre_interfaz)
{
    /*
    int conexion_io = (int)(intptr_t)dictionary_get(conexiones_io, nombre_interfaz);

    if(recv(conexion_io, NULL, 0, MSG_PEEK) == 0)
    {
        return false;
    } else{
        return true;
    }
    TODO: FALTA VER BIEN PARTE DE SI SIGUEN CONECTADOS */
    return dictionary_has_key(conexiones_io, nombre_interfaz);
}

bool esOperacionValida(t_identificador identificador, cod_interfaz tipo)
{
    switch (tipo)
    {
    case GENERICA:
        return identificador == IO_GEN_SLEEP;
    case STDIN:
        return identificador == IO_STDIN_READ;
    case STDOUT:
        return identificador == IO_STDOUT_WRITE;
    case DIALFS:
        return identificador == IO_FS_TRUNCATE || identificador == IO_FS_READ || identificador == IO_FS_WRITE || identificador == IO_FS_CREATE || identificador == IO_FS_DELETE;
    default:
        return false;
    }
}

void crear_interfaz(op_code tipo, char *nombre, uint32_t conexion)
{
    log_info(logger_kernel, "Se conectó la I/O llamada: %s", nombre);

    t_interfaz_en_kernel *interfaz = malloc(sizeof(t_interfaz_en_kernel));
    interfaz->conexion = conexion;
    interfaz->tipo_interfaz = cod_op_to_tipo_interfaz(tipo); 

    pthread_mutex_lock(&mutex_interfaces_conectadas);
    dictionary_put(conexiones_io, nombre, interfaz); // Guardar el socket como entero
    pthread_mutex_unlock(&mutex_interfaces_conectadas);

    t_cola_interfaz_io *cola_interfaz = malloc(sizeof(t_cola_interfaz_io));
    cola_interfaz->cola = queue_create();
    pthread_mutex_init(&(cola_interfaz->mutex), NULL);

    pthread_mutex_lock(&mutex_cola_interfaces);
    dictionary_put(colas_blocks_io, nombre, cola_interfaz);
    pthread_mutex_unlock(&mutex_cola_interfaces);

    t_semaforosIO *semaforos_interfaz = malloc(sizeof(t_semaforosIO));
    pthread_mutex_init(&semaforos_interfaz->mutex, NULL);
    sem_init(&semaforos_interfaz->instruccion_en_cola, 0, 0);
    sem_init(&semaforos_interfaz->binario_io_libre, 0, 1);

    pthread_mutex_lock(&mutex_diccionario_interfaces_de_semaforos);
    dictionary_put(diccionario_semaforos_io, nombre, semaforos_interfaz);
    pthread_mutex_unlock(&mutex_diccionario_interfaces_de_semaforos);

    pthread_t hilo_IO;
    char *nombre2 = strdup(nombre);
    pthread_create(&hilo_IO, NULL, (void *)atender_interfaz, nombre2);
    pthread_detach(hilo_IO);
}

void ejecutar_instruccion_io(char *nombre_interfaz, t_info_en_io *info_io, t_interfaz_en_kernel *conexion_io)
{
    t_paquete *paquete = crear_paquete(EJECUTAR_IO);
    buffer_add(paquete->buffer,info_io->info_necesaria,info_io->tam_info);
    enviar_paquete(paquete, conexion_io->conexion);
    
}

void atender_interfaz(char *nombre_interfaz)
{
    while (1)
    {
        t_semaforosIO *semaforos_interfaz = dictionary_get(diccionario_semaforos_io, nombre_interfaz);

        sem_wait(&(semaforos_interfaz->instruccion_en_cola));
        sem_wait(&(semaforos_interfaz->binario_io_libre));
        pthread_mutex_lock(&(semaforos_interfaz->mutex));

        t_cola_interfaz_io *cola = dictionary_get(colas_blocks_io, nombre_interfaz);

        pthread_mutex_lock(&(cola->mutex));
        t_info_en_io *info_io = queue_pop(cola->cola);
        pthread_mutex_unlock(&(cola->mutex));

        pthread_mutex_unlock(&(semaforos_interfaz->mutex));

        t_interfaz_en_kernel *conexion_io = dictionary_get(conexiones_io, nombre_interfaz);
        ejecutar_instruccion_io(nombre_interfaz, info_io, conexion_io);

        op_code resultado_operacion = recibir_operacion(conexion_io->conexion);
        switch (resultado_operacion)
        {
        case IO_SUCCESS:
            log_info(logger_kernel, "llegue a IO_SUCCESS");
            t_pcb *pcb = buscar_pcb_por_pid(info_io->pid, lista_procesos_blocked);

            pthread_mutex_lock(&mutex_lista_de_blocked);
            list_remove_element(lista_procesos_blocked, pcb);
            pthread_mutex_unlock(&mutex_lista_de_blocked);

            if (pcb != NULL)
            {

                wait_contador(semaforo_multi);
                if (pcb->quantum > 0 && strcmp(algoritmo_planificacion,"VRR") == 0)
                {
                    set_add_pcb_cola(pcb, READY, cola_ready_plus, mutex_cola_de_ready_plus);
                    logear_cambio_estado(pcb, BLOCKED, READY);
                    procesos_en_ready_plus++;
                    listar_procesos_en_ready_plus();
                    sem_post(&(semaforos_interfaz->binario_io_libre));
                    break;
                }
                set_add_pcb_cola(pcb, READY, cola_procesos_ready, mutex_cola_de_readys);
                logear_cambio_estado(pcb, BLOCKED, READY);
                listar_procesos_en_ready();
                sem_post(&hay_proceso_a_ready);
            }

            sem_post(&(semaforos_interfaz->binario_io_libre));

            break;
        case CERRAR_IO:
            close(conexion_io->conexion); // Cerramos el socket una vez que salimos del loop
            break;

        default:
            break;
        }
    }
}

void *atender_cliente(void *socket_cliente_ptr)
{
    int socket_cliente = *(int *)socket_cliente_ptr;
    free(socket_cliente_ptr); // Liberamos el puntero ya que no lo necesitamos más

    op_code codigo_operacion = recibir_operacion(socket_cliente);

    switch (codigo_operacion)
    {
    case INTERFAZ_GENERICA:
        nombre_entrada_salida_conectada = recibir_mensaje_guardar_variable(socket_cliente);

        crear_interfaz(INTERFAZ_GENERICA, nombre_entrada_salida_conectada, socket_cliente);

        free(nombre_entrada_salida_conectada);
        break;
    case INTERFAZ_STDIN:
        nombre_entrada_salida_conectada = recibir_mensaje_guardar_variable(socket_cliente);

        crear_interfaz(INTERFAZ_STDIN, nombre_entrada_salida_conectada, socket_cliente);

        free(nombre_entrada_salida_conectada);
        break;
    case INTERFAZ_STDOUT:
        nombre_entrada_salida_conectada = recibir_mensaje_guardar_variable(socket_cliente);

        crear_interfaz(INTERFAZ_STDOUT, nombre_entrada_salida_conectada, socket_cliente);

        free(nombre_entrada_salida_conectada);
        break;
    case INTERFAZ_DIALFS:
        nombre_entrada_salida_conectada = recibir_mensaje_guardar_variable(socket_cliente);

        crear_interfaz(INTERFAZ_DIALFS, nombre_entrada_salida_conectada, socket_cliente);

        free(nombre_entrada_salida_conectada);
        break;
    default:
        log_info(logger_kernel, "Se recibió un mensaje de un módulo desconocido");
        break;
    }

    return NULL;
}