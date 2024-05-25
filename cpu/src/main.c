#include <../include/cpu.h>

// // inicializacion

t_log *logger_cpu;
t_config *config_cpu;
char *ip_memoria;
char *puerto_memoria;
char *puerto_dispatch;
char *puerto_interrupt;
u_int32_t conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt;
int socket_servidor_dispatch, socket_servidor_interrupt;
pthread_t hilo_dispatch, hilo_interrupt;
t_registros registros_cpu;
t_interrupcion *interrupcion_recibida;

//------------------------variables globales----------------
u_int8_t interruption_flag;
u_int8_t end_process_flag;
u_int8_t input_ouput_flag;

// ------------------------- MAIN---------------------------

int main(void)
{
    iniciar_config();
    inicializar_flags();

    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_cpu);
    enviar_mensaje("", conexion_memoria, CPU, logger_cpu);

    // iniciar servidor Dispatch y Interrupt
    pthread_create(&hilo_dispatch, NULL, iniciar_servidor_dispatch, NULL);
    pthread_create(&hilo_interrupt, NULL, iniciar_servidor_interrupt, NULL);

    pthread_join(hilo_dispatch, NULL);
    pthread_join(hilo_interrupt, NULL);

    liberar_conexion(socket_servidor_interrupt);
    terminar_programa(socket_servidor_dispatch, logger_cpu, config_cpu);
}

//------------------------FUNCIONES DE INICIO------------------------

void iniciar_config()
{
    config_cpu = config_create("./config/cpu.config");
    logger_cpu = iniciar_logger("config/cpu.log", "CPU", LOG_LEVEL_INFO);
    ip_memoria = config_get_string_value(config_cpu, "IP_MEMORIA");
    puerto_dispatch = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_DISPATCH");
    puerto_memoria = config_get_string_value(config_cpu, "PUERTO_MEMORIA");
    puerto_interrupt = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_INTERRUPT");
}

void inicializar_flags()
{
    interruption_flag = 0;
    end_process_flag = 0;
    input_ouput_flag = 0;
}

//-------------------------------ATENDER_DISPATCH-------------------------------
void *iniciar_servidor_dispatch(void *arg)
{
    socket_servidor_dispatch = iniciar_servidor(logger_cpu, puerto_dispatch, "DISPATCH");
    while (1)
    {
        conexion_kernel_dispatch = esperar_cliente(socket_servidor_dispatch, logger_cpu);
        log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Dispatch", cod_op_to_string(recibir_operacion(conexion_kernel_dispatch)));
        recibir_mensaje(conexion_kernel_dispatch, logger_cpu);

        t_pcb *pcb = recibir_pcb(conexion_kernel_dispatch);
        printf("recibi el proceso:%d", pcb->pid);
        comenzar_proceso(pcb, conexion_memoria, conexion_kernel_dispatch);
    }
    return NULL;
}

//-------------------------------ATENDER_INTERRUPTION-------------------------------

void *iniciar_servidor_interrupt(void *arg)
{
    socket_servidor_interrupt = iniciar_servidor(logger_cpu, puerto_interrupt, "INTERRUPT");
    while (1)
    {
        conexion_kernel_interrupt = esperar_cliente(socket_servidor_interrupt, logger_cpu);
        log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Interrupt", cod_op_to_string(recibir_operacion(conexion_kernel_interrupt)));
        t_paquete *respuesta_kernel = recibir_paquete(conexion_kernel_interrupt);

        int cod_op = respuesta_kernel->codigo_operacion;
        if (cod_op == INTERRUPTION)
        {
            log_info(logger_cpu, "Ocurrio una interrupcion");
            interrupcion_recibida = deserializar_interrupcion(respuesta_kernel->buffer);
        }
        else
        {
            log_info(logger_cpu, "operacion desconocida dentro de interrupciones");
        }
    }
    return NULL;
}

void accion_interrupt(t_pcb *pcb, op_code motivo, int socket)
{
    interruption_flag = 0;
    log_info(logger_cpu, "estaba ejecutando y encontre una interrupcion");
    enviar_motivo_desalojo(motivo, socket);
    enviar_pcb(pcb, socket);
}

// ------------------------ CICLO BASICO DE INSTRUCCION ------------------------

t_instruccion *fetch_instruccion(uint32_t pid, uint32_t *pc, uint32_t conexionParam)
{
    // Buscar instruccion en memoria
    t_paquete *paquete = crear_paquete(SOLICITUD_INSTRUCCION);
    // Le voy a mandar a memoria el paquete con el pid y el pc,
    // para q busque en la lista de procesos el proceso, y me devuelva la instruccion según el PC

    t_buffer *buffer = crear_buffer();

    buffer_add(buffer, &pid, sizeof(uint32_t));
    buffer_add(buffer, pc, sizeof(uint32_t));
    paquete->buffer = buffer;

    enviar_paquete(paquete, conexionParam);

    log_info(logger_cpu, "Se envio la solicitud de instruccion a memoria con pid: %d y pc: %d", pid, *pc);

    t_paquete *respuesta_memoria = recibir_paquete(conexionParam);

    if (respuesta_memoria->codigo_operacion != INSTRUCCION)
    {
        log_error(logger_cpu, "Error al recibir instruccion de memoria");
        eliminar_paquete(paquete);
        eliminar_paquete(respuesta_memoria);
        return NULL;
    }

    t_instruccion *instruccion = instruccion_deserializar(respuesta_memoria->buffer, 0);

    eliminar_paquete(paquete);
    eliminar_paquete(respuesta_memoria);
    return instruccion;
}

void decode_y_execute_instruccion(t_instruccion *instruccion, t_pcb *pcb)
{
    switch (instruccion->identificador)
    {
    case SET:
        set_registro(&pcb->registros, instruccion->parametros[0], atoi(instruccion->parametros[1]));
        break;
    case MOV_IN:
        break;
    case MOV_OUT:
        break;
    case SUM:
        sum_registro(&pcb->registros, instruccion->parametros[0], instruccion->parametros[1]);
        break;
    case SUB:
        sub_registro(&pcb->registros, instruccion->parametros[0], instruccion->parametros[1]);
        break;
    case JNZ:
        JNZ_registro(&pcb->registros, instruccion->parametros[0], atoi(instruccion->parametros[1]));
        break;
    case IO_FS_TRUNCATE:
        break;
    case IO_STDIN_READ:
        break;
    case IO_STDOUT_WRITE:
        break;
    case IO_GEN_SLEEP:
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        t_paquete *paquete = crear_paquete(OPERACION_IO);
        paquete->buffer = serializar_instruccion(instruccion);
        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;

        break;
    case IO_FS_DELETE:
        break;
    case IO_FS_CREATE:
        break;
    case IO_FS_WRITE:
        break;
    case IO_FS_READ:
        break;
    case RESIZE:
        break;
    case COPY_STRING:
        break;
    case WAIT:
        break;
    case SIGNAL:
        break;
    case EXIT:
        end_process_flag = 1;
        break;
    default:
        break;
    }
    printf("------------------ INSTRUCCION EJECUTADA: --------------------\n");
    imprimir_instruccion(*instruccion);
}

bool check_interrupt(uint32_t pid)
{
    if (interrupcion_recibida != NULL)
    {
        return interrupcion_recibida->pid == pid;
    }
    return false;
}

// ------------------------ FUNCIONES PARA EJECUTAR PCBS ------------------------

t_instruccion *siguiente_instruccion(t_pcb *pcb, int socket)
{

    t_instruccion *instruccion = fetch_instruccion(pcb->pid, &pcb->pc, socket);
    pcb->pc += 1;
    return instruccion;
}

void comenzar_proceso(t_pcb *pcb, int socket_Memoria, int socket_Kernel)
{
    t_instruccion *instruccion = malloc(sizeof(t_instruccion));
    while (interruption_flag != 1 && end_process_flag != 1 && input_ouput_flag != 1 /* && page_fault != 1 && sigsegv != 1*/)
    {
        instruccion = siguiente_instruccion(pcb, socket_Memoria);
        decode_y_execute_instruccion(instruccion, pcb);
        if (check_interrupt(pcb->pid))
            interruption_flag = 1;
        else
        {
            interrupcion_recibida = NULL;
        }
    }
    imprimir_registros_por_pantalla(pcb->registros);
    if (interruption_flag == 1)
    {
        accion_interrupt(pcb, interrupcion_recibida->motivo, socket_Kernel);
    }

    if (end_process_flag == 1)
    {
        end_process_flag = 0;
        interruption_flag = 0;
        enviar_pcb(pcb, socket_Kernel);
    }
    input_ouput_flag = 0;
}
