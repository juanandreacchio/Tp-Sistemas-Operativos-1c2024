#include <../include/cpu.h>

// // inicializacion

t_log *logger_cpu;
t_config *config_cpu;
char *ip_memoria;
char *puerto_memoria;
char *puerto_dispatch;
char *puerto_interrupt;
char *algoritmo_tlb;
u_int32_t conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt, tamanio_de_pagina;
int socket_servidor_dispatch, socket_servidor_interrupt, cant_entradas_tlb;
pthread_t hilo_dispatch, hilo_interrupt;
t_registros registros_cpu;
t_interrupcion *interrupcion_recibida = NULL; // Inicializa a NULL
t_list *TLB;

//------------------------variables globales----------------
u_int8_t interruption_flag;
u_int8_t end_process_flag;
u_int8_t input_ouput_flag;
u_int8_t flag_out_of_memory;
uint8_t flag_bloqueado_por_resource;

// ------------------------- MAIN---------------------------

int main(void)
{
    iniciar_config();
    inicializar_flags();
    TLB = list_create();
    registros_cpu = inicializar_registros();

    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_cpu);
    enviar_mensaje("", conexion_memoria, CPU, logger_cpu);
    tamanio_de_pagina = recibir_tamanio(conexion_memoria);

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
    algoritmo_tlb = config_get_string_value(config_cpu, "ALGORITMO_TLB");
    cant_entradas_tlb = config_get_int_value(config_cpu, "CANTIDAD_ENTRADAS_TLB");
}

void inicializar_flags()
{
    interruption_flag = 0;
    end_process_flag = 0;
    input_ouput_flag = 0;
    flag_bloqueado_por_resource = 0;
    flag_out_of_memory = 0;
}
//--------------------------RECIBIR TAMAÑO "handshake"---------------------------
u_int32_t recibir_tamanio(u_int32_t socket_cliente)
{
    int tamanio_pagina;
    recv(socket_cliente, &tamanio_pagina, sizeof(int), MSG_WAITALL);
    u_int32_t tam_cast = (u_int32_t)tamanio_pagina;
    return tam_cast;
}
//-------------------------------ATENDER_DISPATCH-------------------------------
void *iniciar_servidor_dispatch(void *arg)
{
    socket_servidor_dispatch = iniciar_servidor(logger_cpu, puerto_dispatch, "DISPATCH");
    conexion_kernel_dispatch = esperar_cliente(socket_servidor_dispatch, logger_cpu);
    log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Dispatch", cod_op_to_string(recibir_operacion(conexion_kernel_dispatch)));
    recibir_mensaje(conexion_kernel_dispatch, logger_cpu);
    while (1)
    {
        t_pcb *pcb = recibir_pcb(conexion_kernel_dispatch);
        registros_cpu = pcb->registros;
        registros_cpu.PC = pcb->pc;
        comenzar_proceso(pcb, conexion_memoria, conexion_kernel_dispatch);
    }
    return NULL;
}

//-------------------------------ATENDER_INTERRUPTION-------------------------------

void *iniciar_servidor_interrupt(void *arg)
{
    socket_servidor_interrupt = iniciar_servidor(logger_cpu, puerto_interrupt, "INTERRUPT");
    conexion_kernel_interrupt = esperar_cliente(socket_servidor_interrupt, logger_cpu);
    log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Interrupt", cod_op_to_string(recibir_operacion(conexion_kernel_interrupt)));
    recibir_mensaje(conexion_kernel_interrupt, logger_cpu);
    while (1)
    {
        t_paquete *respuesta_kernel = recibir_paquete(conexion_kernel_interrupt);

        if (respuesta_kernel->codigo_operacion == INTERRUPTION)
        {
            if (interrupcion_recibida != NULL)
            {
                free(interrupcion_recibida);
            }
            interrupcion_recibida = deserializar_interrupcion(respuesta_kernel->buffer);
            log_info(logger_cpu, "ME LLEGO UNA INTERRUPCION DEL KERNEL DEL PROCESO: %d", interrupcion_recibida->pid);
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
    if (interrupcion_recibida == NULL)
    {
        log_error(logger_cpu, "No hay interrupcion recibida");
        return;
    }

    interruption_flag = 0;
    log_info(logger_cpu, "estaba ejecutando y encontre una interrupcion nevio PCB y motivo de desalojo a Kernel");
    enviar_motivo_desalojo(motivo, socket);
    enviar_pcb(pcb, socket);
}

// ------------------------ CICLO BASICO DE INSTRUCCION ------------------------

t_instruccion *fetch_instruccion(uint32_t pid, uint32_t *pc, uint32_t conexionParam)
{
    t_paquete *paquete = crear_paquete(SOLICITUD_INSTRUCCION);

    buffer_add(paquete->buffer, &pid, sizeof(uint32_t));
    buffer_add(paquete->buffer, pc, sizeof(uint32_t));

    enviar_paquete(paquete, conexionParam);

    //log_info(logger_cpu, "PID: %d - FETCH - Program Counter pc: %d", pid, *pc);

    t_paquete *respuesta_memoria = recibir_paquete(conexionParam);

    if (respuesta_memoria == NULL)
    {
        log_error(logger_cpu, "Error al recibir instruccion de memoria por null");
        eliminar_paquete(paquete);
        return NULL;
    }

    if (respuesta_memoria->codigo_operacion != INSTRUCCION)
    {
        log_error(logger_cpu, "Error al recibir instruccion de memoria por cod_op");
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
    {
        set_registro(instruccion->parametros[0], atoi(instruccion->parametros[1]));
        break;
    }
    case MOV_IN:
    {
        mov_in(pcb->pid, instruccion->parametros[0], instruccion->parametros[1]);
        break;
    }
    case MOV_OUT:
    {
        mov_out(pcb->pid, instruccion->parametros[0], instruccion->parametros[1]);
        break;
    }
    case SUM:
    {
        sum_registro(instruccion->parametros[0], instruccion->parametros[1]);
        break;
    }

    case SUB:
    {
        sub_registro(instruccion->parametros[0], instruccion->parametros[1]);
        break;
    }

    case JNZ:
    {
        JNZ_registro(instruccion->parametros[0], atoi(instruccion->parametros[1]));
        break;
    }

    case IO_FS_TRUNCATE:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        u_int32_t tamanio = get_registro_generico(instruccion->parametros[2]);
        t_paquete *paquete = crear_paquete(OPERACION_IO);
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param1_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[0], instruccion->param1_length);

        u_int32_t tam_a_enviar = instruccion->param2_length + sizeof(t_identificador)+ sizeof(u_int32_t)*2;
        buffer_add(paquete->buffer, &tam_a_enviar, sizeof(u_int32_t));
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param2_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[1], instruccion->param2_length);
        buffer_add(paquete->buffer, &tamanio,sizeof(u_int32_t));
        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);
        break;
    }
    case IO_STDIN_READ:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        u_int32_t direc_logica = get_registro_generico( instruccion->parametros[1]);
        size_t tamanio = (size_t)get_registro_generico( instruccion->parametros[2]);

        t_paquete *paquete = crear_paquete(OPERACION_IO);

        t_list *direc_fisicas = traducir_DL_a_DF_generico(direc_logica, pcb->pid, tamanio);
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param1_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[0], instruccion->param1_length);
        u_int32_t tam_a_enviar = (sizeof(t_direc_fisica) - sizeof(u_int32_t)) * list_size(direc_fisicas) + sizeof(u_int32_t) * 2;
        buffer_add(paquete->buffer, &tam_a_enviar, sizeof(u_int32_t));
        enviar_soli_lectura_sin_pid(paquete, direc_fisicas, tamanio, conexion_kernel_dispatch);

        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);

        list_destroy_and_destroy_elements(direc_fisicas, free);
        break;
    }
    case IO_STDOUT_WRITE:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        u_int32_t direc_logica = get_registro_generico( instruccion->parametros[1]);
        u_int32_t tamanio = get_registro_generico( instruccion->parametros[2])+1;

        t_paquete *paquete = crear_paquete(OPERACION_IO);
        t_list *direc_fisicas = traducir_DL_a_DF_generico(direc_logica, pcb->pid, tamanio);
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param1_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[0], instruccion->param1_length);
        u_int32_t tam_a_enviar = (sizeof(t_direc_fisica) - sizeof(u_int32_t)) * list_size(direc_fisicas) + sizeof(u_int32_t) * 2;
        buffer_add(paquete->buffer, &tam_a_enviar, sizeof(u_int32_t));
        enviar_soli_lectura_sin_pid(paquete, direc_fisicas, tamanio, conexion_kernel_dispatch);

        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);

        list_destroy_and_destroy_elements(direc_fisicas, free);
        break;
    }
    case IO_GEN_SLEEP:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        t_paquete *paquete = crear_paquete(OPERACION_IO);
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param1_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[0], instruccion->param1_length);
        u_int32_t tam_a_enviar = sizeof(u_int32_t);
        buffer_add(paquete->buffer, &tam_a_enviar, sizeof(u_int32_t));
        u_int32_t unidad_de_trabajo = atoi(instruccion->parametros[1]);
        buffer_add(paquete->buffer, &unidad_de_trabajo, sizeof(u_int32_t));
        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);
    }
    break;
    case IO_FS_DELETE:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        t_paquete *paquete = crear_paquete(OPERACION_IO);
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param1_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[0], instruccion->param1_length);
        u_int32_t tam_a_enviar = instruccion->param2_length + sizeof(t_identificador)+ sizeof(u_int32_t);
        buffer_add(paquete->buffer, &tam_a_enviar, sizeof(u_int32_t));
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param2_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[1], instruccion->param2_length);
        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);
        break;
    }
    case IO_FS_CREATE:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        t_paquete *paquete = crear_paquete(OPERACION_IO);
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param1_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[0], instruccion->param1_length);
        u_int32_t tam_a_enviar = instruccion->param2_length + sizeof(t_identificador)+sizeof(u_int32_t);
        buffer_add(paquete->buffer, &tam_a_enviar, sizeof(u_int32_t));
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param2_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[1], instruccion->param2_length);
        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);
        break;
    }
    case IO_FS_WRITE:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        u_int32_t tamanio = get_registro_generico(instruccion->parametros[3]);
        u_int32_t direc_logica = get_registro_generico( instruccion->parametros[2]);
        u_int32_t puntero = get_registro_generico( instruccion->parametros[4]);
        t_list *direc_fisicas = traducir_DL_a_DF_generico(direc_logica, pcb->pid, tamanio);
        t_paquete *paquete = crear_paquete(OPERACION_IO);
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param1_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[0], instruccion->param1_length);
        u_int32_t tam_a_enviar = instruccion->param2_length + sizeof(t_identificador) + sizeof(u_int32_t) +//esto es el tamanio de identificador+tamamnio_param+param
                                 (sizeof(t_direc_fisica) - sizeof(u_int32_t)) * list_size(direc_fisicas) +sizeof(u_int32_t)*2+//esto es el tamanio de lo qe amnda en enviar_soli_lectura_sin_pid
                                 sizeof(u_int32_t);//puntero
        buffer_add(paquete->buffer, &tam_a_enviar, sizeof(u_int32_t));
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param2_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[1], instruccion->param2_length);
        enviar_soli_lectura_sin_pid(paquete, direc_fisicas, tamanio, conexion_kernel_dispatch);
        buffer_add(paquete->buffer, &puntero, sizeof(u_int32_t));

        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);

        list_destroy_and_destroy_elements(direc_fisicas, free);
        break;
    }
    case IO_FS_READ:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        u_int32_t tamanio = get_registro_generico(instruccion->parametros[3]);
        u_int32_t direc_logica = get_registro_generico( instruccion->parametros[2]);
        u_int32_t puntero = get_registro_generico( instruccion->parametros[4]);
        t_list *direc_fisicas = traducir_DL_a_DF_generico(direc_logica, pcb->pid, tamanio);
        t_paquete *paquete = crear_paquete(OPERACION_IO);
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param1_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[0], instruccion->param1_length);
        u_int32_t tam_a_enviar = instruccion->param2_length + sizeof(t_identificador) + sizeof(u_int32_t)+//esto es el tamanio de identificador+tamamnio_param+param
                                 (sizeof(t_direc_fisica) - sizeof(u_int32_t)) * list_size(direc_fisicas) +sizeof(u_int32_t)*2+//esto es el tamanio de lo qe amnda en enviar_soli_lectura_sin_pid
                                 sizeof(u_int32_t);//puntero
        buffer_add(paquete->buffer, &tam_a_enviar, sizeof(u_int32_t));
        buffer_add(paquete->buffer, &instruccion->identificador, sizeof(t_identificador));
        buffer_add(paquete->buffer, &instruccion->param2_length, sizeof(u_int32_t));
        buffer_add(paquete->buffer, instruccion->parametros[1], instruccion->param2_length);
        enviar_soli_lectura_sin_pid(paquete, direc_fisicas, tamanio, conexion_kernel_dispatch);
        buffer_add(paquete->buffer, &puntero, sizeof(u_int32_t));

        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);

        list_destroy_and_destroy_elements(direc_fisicas, free);
        break;
    }
    case RESIZE:
    {
    t_paquete *paquete_a_enviar = crear_paquete(AJUSTAR_TAMANIO_PROCESO);
    buffer_add(paquete_a_enviar->buffer, &pcb->pid, sizeof(u_int32_t));
    u_int32_t tamanio = (u_int32_t)atoi(instruccion->parametros[0]);
    buffer_add(paquete_a_enviar->buffer, &tamanio, sizeof(u_int32_t));
    enviar_paquete(paquete_a_enviar, conexion_memoria);
    eliminar_paquete(paquete_a_enviar);
    op_code operacion = recibir_operacion(conexion_memoria);
    if (operacion == OUT_OF_MEMORY)
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(OUT_OF_MEMORY, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);
        flag_out_of_memory = 1;
    }
    else if (operacion != OK)
    {
        log_error(logger_cpu, "error: recibi un codigo de operacion desconocido dentro de RESIZE");
    }
    else if (operacion == OK)
        log_info(logger_cpu, "RESIZE hecho");
    break;
}
    case COPY_STRING:
    {
        
        size_t tamanio = (size_t)atoi(instruccion->parametros[0]);
        copy_string(pcb->pid, tamanio);
        break;

    }
    case WAIT:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(WAIT_SOLICITADO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        t_paquete *paquete = crear_paquete(WAIT_SOLICITADO);
        paquete->buffer = serializar_instruccion(instruccion);
        enviar_paquete(paquete, conexion_kernel_dispatch);
        op_code estado_operacion =  recibir_operacion(conexion_kernel_dispatch);
        if(estado_operacion == RESOURCE_FAIL)
        {
          end_process_flag = 1;
        } else if (estado_operacion == RESOURCE_BLOCKED)
        {
            flag_bloqueado_por_resource = 1;
        }
        
        break;
    }
    case SIGNAL:
    {
        pcb->registros = registros_cpu;
        pcb->pc = registros_cpu.PC;
        enviar_motivo_desalojo(SIGNAL_SOLICITADO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        t_paquete *paquete = crear_paquete(SIGNAL_SOLICITADO);
        paquete->buffer = serializar_instruccion(instruccion);
        enviar_paquete(paquete, conexion_kernel_dispatch);
        op_code estado_operacion =  recibir_operacion(conexion_kernel_dispatch);
        if(estado_operacion == RESOURCE_FAIL)
        {
          end_process_flag = 1;
        }
        break;
    }

    case EXIT:
    {
        end_process_flag = 1;
        break;
    }
    default:
        break;
    }
    //log_info(logger_cpu, "PID: %d - EJECUTANDO ", pcb->pid);
    imprimir_instruccion(instruccion);
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
    t_instruccion *instruccion = fetch_instruccion(pcb->pid, &registros_cpu.PC, socket);
    log_info(logger_cpu,"PID: %d - FETCH - Program Counter: %d",pcb->pid, registros_cpu.PC);
    if (instruccion != NULL)
    {
        registros_cpu.PC+= 1;
    }
    return instruccion;
}

void comenzar_proceso(t_pcb *pcb, int socket_Memoria, int socket_Kernel)
{
    t_instruccion *instruccion = NULL;
    input_ouput_flag = 0;
    while (interruption_flag != 1 && end_process_flag != 1 && input_ouput_flag != 1 && flag_bloqueado_por_resource != 1 && flag_out_of_memory != 1)
    {
        if (instruccion != NULL)
            free(instruccion);

        instruccion = siguiente_instruccion(pcb, socket_Memoria);
        if (instruccion == NULL)
        {
            // Manejar error: siguiente_instruccion no debería devolver NULL
            perror("Error: siguiente_instruccion devolvió NULL");
            break;
        }
        decode_y_execute_instruccion(instruccion, pcb);

        if (check_interrupt(pcb->pid))
        {
            log_info(logger_cpu, "el chequeo de interrupcion encontro que hay una interrupcion");
            interruption_flag = 1;
        }
        log_instruccion(pcb->pid,instruccion);
        //usleep(500000); este es el que no iba
    }

    imprimir_registros_por_pantalla(registros_cpu);
    pcb->registros = registros_cpu;
    pcb->pc = registros_cpu.PC;
    

    if (instruccion != NULL)
        free(instruccion);

    if (end_process_flag == 1)
    {
        end_process_flag = 0;
        interruption_flag = 0;
        enviar_motivo_desalojo(END_PROCESS, socket_Kernel);
        enviar_pcb(pcb, socket_Kernel);
        log_info(logger_cpu, "Se envio el pcb al kernel con motivo de desalojo END_PROCESS");
        return;
    }
    if (interruption_flag == 1)
    {
        accion_interrupt(pcb, interrupcion_recibida->motivo, socket_Kernel);
    }
    if (flag_bloqueado_por_resource == 1)
    {
        flag_bloqueado_por_resource = 0;
        enviar_motivo_desalojo(RESOURCE_BLOCKED, socket_Kernel);
        enviar_pcb(pcb, socket_Kernel);
    }
    
}
//------------------------FUNCIONES DE OPERACIONES------------------------------

void set_registro( char *registro, u_int32_t valor)
{
    if (strcasecmp(registro, "AX") == 0)
    {
        u_int8_t valor8 = (u_int8_t)valor;
        registros_cpu.AX = valor8;
    }

    if (strcasecmp(registro, "BX") == 0)
    {
        u_int8_t valor8 = (u_int8_t)valor;
        registros_cpu.BX = valor8;
    }
    if (strcasecmp(registro, "CX") == 0)
    {
        u_int8_t valor8 = (u_int8_t)valor;
        registros_cpu.CX = valor8;
    }
    if (strcasecmp(registro, "DX") == 0)
    {
        u_int8_t valor8 = (u_int8_t)valor;
        registros_cpu.DX = valor8;
    }
    if (strcasecmp(registro, "EAX") == 0)
    {
        registros_cpu.EAX = valor;
    }
    if (strcasecmp(registro, "EBX") == 0)
    {
        registros_cpu.EBX = valor;
    }
    if (strcasecmp(registro, "ECX") == 0)
    {
        registros_cpu.ECX = valor;
    }
    if (strcasecmp(registro, "EDX") == 0)
    {
        registros_cpu.EDX = valor;
    }
    if (strcasecmp(registro, "PC") == 0)
    {
        registros_cpu.PC = valor;
    }
    if (strcasecmp(registro, "SI") == 0)
    {
        registros_cpu.SI = valor;
    }
    if (strcasecmp(registro, "DI") == 0)
    {
        registros_cpu.DI = valor;
    }
}

u_int8_t get_registro_int8(char *registro)
{
    if (strcasecmp(registro, "AX") == 0)
    {
        return registros_cpu.AX;
    }
    else if (strcasecmp(registro, "BX") == 0)
    {
        return registros_cpu.BX;
    }
    else if (strcasecmp(registro, "CX") == 0)
    {
        return registros_cpu.CX;
    }
    else if (strcasecmp(registro, "DX") == 0)
    {
        return registros_cpu.DX;
    }
    return -1;
}

u_int32_t get_registro_int32(char *registro)
{
    if (strcasecmp(registro, "EAX") == 0)
    {
        return registros_cpu.EAX;
    }
    else if (strcasecmp(registro, "EBX") == 0)
    {
        return registros_cpu.EBX;
    }
    else if (strcasecmp(registro, "ECX") == 0)
    {
        return registros_cpu.ECX;
    }
    else if (strcasecmp(registro, "EDX") == 0)
    {
        return registros_cpu.EDX;
    }
    else if (strcasecmp(registro, "PC") == 0)
    {
        return registros_cpu.PC;
    }
    else if (strcasecmp(registro, "SI") == 0)
    {
        return registros_cpu.SI;
    }
    else if (strcasecmp(registro, "DI") == 0)
    {
        return registros_cpu.DI;
    }
    return -1;
}

u_int32_t get_registro_generico(char *registro)
{
    if (strcasecmp(registro, "AX") == 0)
    {
        return registros_cpu.AX;
    }
    else if (strcasecmp(registro, "BX") == 0)
    {
        return registros_cpu.BX;
    }
    else if (strcasecmp(registro, "CX") == 0)
    {
        return registros_cpu.CX;
    }
    else if (strcasecmp(registro, "DX") == 0)
    {
        return registros_cpu.DX;
    }
    else if (strcasecmp(registro, "EAX") == 0)
    {
        return registros_cpu.EAX;
    }
    else if (strcasecmp(registro, "EBX") == 0)
    {
        return registros_cpu.EBX;
    }
    else if (strcasecmp(registro, "ECX") == 0)
    {
        return registros_cpu.ECX;
    }
    else if (strcasecmp(registro, "EDX") == 0)
    {
        return registros_cpu.EDX;
    }
    else if (strcasecmp(registro, "PC") == 0)
    {
        return registros_cpu.PC;
    }
    else if (strcasecmp(registro, "SI") == 0)
    {
        return registros_cpu.SI;
    }
    else if (strcasecmp(registro, "DI") == 0)
    {
        return registros_cpu.DI;
    }
    return -1;
}

void sum_registro(char *registroDestino, char *registroOrigen)
{

    if (strcasecmp(registroDestino, "AX") == 0)
    {
        registros_cpu.AX += get_registro_int8(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "BX") == 0)
    {
        registros_cpu.BX += get_registro_int8(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "CX") == 0)
    {
        registros_cpu.CX += get_registro_int8(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "DX") == 0)
    {
        registros_cpu.DX += get_registro_int8(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "EAX") == 0)
    {
        registros_cpu.EAX += get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "EBX") == 0)
    {
        registros_cpu.EBX += get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "ECX") == 0)
    {
        registros_cpu.ECX += get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "EDX") == 0)
    {
        registros_cpu.EDX += get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "PC") == 0)
    {
        registros_cpu.PC += get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "SI") == 0)
    {
        registros_cpu.SI += get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "DI") == 0)
    {
        registros_cpu.DI += get_registro_int32(registroOrigen);
    }
}

void sub_registro(char *registroOrigen, char *registroDestino)
{

    if (strcasecmp(registroDestino, "AX") == 0)
    {
        registros_cpu.AX -= get_registro_int8(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "BX") == 0)
    {
        registros_cpu.BX -= get_registro_int8(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "CX") == 0)
    {
        registros_cpu.CX -= get_registro_int8(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "DX") == 0)
    {
        registros_cpu.DX -= get_registro_int8(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "EAX") == 0)
    {
        registros_cpu.EAX -= get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "EBX") == 0)
    {
        registros_cpu.EBX -= get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "ECX") == 0)
    {
        registros_cpu.ECX -= get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "EDX") == 0)
    {
        registros_cpu.EDX -= get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "PC") == 0)
    {
        registros_cpu.PC -= get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "SI") == 0)
    {
        registros_cpu.SI += get_registro_int32(registroOrigen);
    }
    else if (strcasecmp(registroDestino, "DI") == 0)
    {
        registros_cpu.DI += get_registro_int32(registroOrigen);
    }
}
void JNZ_registro(char *registro, u_int32_t valor)
{
    u_int8_t valoregistroInt8 = 0;
    u_int32_t valoregistroInt32 = 0;

    if (strcasecmp(registro, "AX") == 0 || strcasecmp(registro, "BX") == 0 || strcasecmp(registro, "CX") == 0 || strcasecmp(registro, "DX") == 0)
    {

        valoregistroInt8 = get_registro_int8(registro);
        if (valoregistroInt8 != 0)
            registros_cpu.PC = valor;
    }
    if (strcasecmp(registro, "EAX") == 0 || strcasecmp(registro, "EBX") == 0 || strcasecmp(registro, "ECX") == 0 || strcasecmp(registro, "EDX") == 0 || strcasecmp(registro, "PC") == 0)
    {
        valoregistroInt32 = get_registro_int32(registro);
        if (valoregistroInt32 != 0)
            registros_cpu.PC = valor;
    }
}
/*--------------esta es otra opcion para despues vemos cual preferimos -------------------
void get_registro(t_registros *registros, char *registro, void *valor) {
    // Define el mapa de cadenas a punteros
    struct {
        char *nombre;
        void *direccion;
    } mapa[] = {
        {"AX", &(registros->AX)},
        {"BX", &(registros->BX)},
        {"CX", &(registros->CX)},
        {"DX", &(registros->DX)},
        {"EAX", &(registros->EAX)},
        {"EBX", &(registros->EBX)},
        {"ECX", &(registros->ECX)},
        {"EDX", &(registros->EDX)}
    };

    // Itera sobre el mapa para encontrar el registro correspondiente
    for (size_t i = 0; i < sizeof(mapa) / sizeof(mapa[0]); i++) {
        if (strcasecmp(registro, mapa[i].nombre) == 0) {
            // Copia el valor del registro a la dirección proporcionada
            memcpy(valor, mapa[i].direccion, sizeof(uint32_t)); // Usa sizeof(uint8_t) o sizeof(uint32_t) según corresponda
            return;
        }
    }

    // Si no se encuentra el registro, aquí puedes manejar el error
}
*/

void mov_in(u_int32_t pid, char *registro_datos, char *registro_direccion)
{
    struct
    {
        char *nombre;
        void *direccion;
    } mapa[] = {
        {"AX", &(registros_cpu.AX)},
        {"BX", &(registros_cpu.BX)},
        {"CX", &(registros_cpu.CX)},
        {"DX", &(registros_cpu.DX)},
        {"EAX", &(registros_cpu.EAX)},
        {"EBX", &(registros_cpu.EBX)},
        {"ECX", &(registros_cpu.ECX)},
        {"EDX", &(registros_cpu.EDX)},
        {"SI", &(registros_cpu.SI)},
        {"DI", &(registros_cpu.DI)},
        {"PC", &(registros_cpu.PC)}};

    int encontrado = 0;
    uint32_t direccion_logica = get_registro_generico(registro_direccion);
    for (size_t i = 0; i < sizeof(mapa) / sizeof(mapa[0]); i++)
    {
        if (strcasecmp(registro_datos, mapa[i].nombre) == 0)
        {
            encontrado = 1;
            size_t size_of_element = (i < 4) ? sizeof(uint8_t) : sizeof(uint32_t);

            t_list *direcciones_fisicas = traducir_DL_a_DF_generico(direccion_logica,pid, size_of_element);
            t_paquete *paquete_enviado = crear_paquete(LECTURA_MEMORIA);
            enviar_soli_lectura(paquete_enviado, direcciones_fisicas, size_of_element, conexion_memoria,pid);

            t_paquete *paquete_recibido = recibir_paquete(conexion_memoria);
            if (paquete_recibido->codigo_operacion != LECTURA_MEMORIA)
            {
                log_error(logger_cpu, "error: codigo de operacion inesperado al recibir la lectura de memoria");
                eliminar_paquete(paquete_recibido);
                return;
            }

            paquete_recibido->buffer->offset = 0;
            void *buffer = malloc(size_of_element);
            buffer_read(paquete_recibido->buffer, buffer, size_of_element);
            memcpy(mapa[i].direccion, buffer, size_of_element);
            u_int32_t valor;
            memcpy(&valor, buffer, sizeof(uint32_t));
            t_direc_fisica *primer_direc_fisica = list_get(direcciones_fisicas,0);
            log_info(logger_cpu,"PID: %d - Acción: LEER - Dirección Física: %d - Valor: %d",pid,primer_direc_fisica->direccion_fisica,valor);
            free(buffer);
            eliminar_paquete(paquete_recibido);

            list_destroy_and_destroy_elements(direcciones_fisicas, free);
        }
    }
    if (!encontrado)
    {
        log_error(logger_cpu, "error: no encontre ese registro");
    }
}

void mov_out(u_int32_t pid, char *registro_direccion, char *registro_datos)
{
    uint32_t direc_logica = get_registro_generico(registro_direccion);
    t_paquete *paquete = crear_paquete(ESCRITURA_MEMORIA);
    size_t size_of_element;
    
    t_list *direc_fisicas;

    if (strcasecmp(registro_datos, "AX") == 0 || strcasecmp(registro_datos, "BX") == 0 ||
        strcasecmp(registro_datos, "CX") == 0 || strcasecmp(registro_datos, "DX") == 0)
    {
        size_of_element = sizeof(uint8_t);
        void *valor_registro = malloc(size_of_element);
        uint8_t valor = get_registro_int8(registro_datos);
        memcpy(valor_registro,&valor,size_of_element);
        //log_info(logger_cpu,"el numero en codigo ascci es: %s",(char*)valor_registro);
        direc_fisicas = traducir_DL_a_DF_generico(direc_logica,pid, size_of_element);
        t_direc_fisica *primer_direc_fisica = list_get(direc_fisicas,0);
        log_info(logger_cpu,"PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d",pid,primer_direc_fisica->direccion_fisica,valor);
        enviar_soli_escritura(paquete, direc_fisicas, size_of_element, valor_registro, conexion_memoria,pid);
        free(valor_registro);
    }
    else if (strcasecmp(registro_datos, "EAX") == 0 || strcasecmp(registro_datos, "EBX") == 0 ||
             strcasecmp(registro_datos, "ECX") == 0 || strcasecmp(registro_datos, "EDX") == 0 ||
             strcasecmp(registro_datos, "SI") == 0 || strcasecmp(registro_datos, "DI") == 0 ||
             strcasecmp(registro_datos, "PC") == 0)
    {

        size_of_element = sizeof(uint32_t);
        void *valor_registro = malloc(size_of_element);
        uint32_t valor = get_registro_generico(registro_datos);
        memcpy(valor_registro,&valor,size_of_element);
        direc_fisicas = traducir_DL_a_DF_generico(direc_logica, pid, size_of_element);
        t_direc_fisica *primer_direc_fisica = list_get(direc_fisicas,0);
        log_info(logger_cpu,"PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d",pid,primer_direc_fisica->direccion_fisica,valor);
        enviar_soli_escritura(paquete, direc_fisicas, size_of_element, valor_registro, conexion_memoria,pid);
        free(valor_registro);
    }
    else
    {
        log_error(logger_cpu, "error: el registro dato no existe dentro de los registros");
        eliminar_paquete(paquete);
        return;
    }
    op_code cod_op = recibir_operacion(conexion_memoria);
    if (cod_op != OK)
    {
        log_error(logger_cpu, "error: codigo de operacion inesperado al recibir la escritura de memoria");
    }
    list_destroy_and_destroy_elements(direc_fisicas, free);
}

void copy_string(u_int32_t pid, size_t tamanio)
{
    uint32_t direc_logica_si = get_registro_generico("SI");
    uint32_t direc_logica_di = get_registro_generico("DI");

    // Obtener las direcciones físicas para el origen (SI)
    t_list *direc_fisicas_si = traducir_DL_a_DF_generico(direc_logica_si, pid, tamanio);
    
    // Obtener las direcciones físicas para el destino (DI)
    t_list *direc_fisicas_di = traducir_DL_a_DF_generico(direc_logica_di, pid, tamanio);

    // Leer el string desde la memoria apuntada por SI
    t_paquete *paquete_lectura = crear_paquete(LECTURA_MEMORIA);
    enviar_soli_lectura(paquete_lectura, direc_fisicas_si, tamanio, conexion_memoria,pid);

    t_paquete *paquete_recibido = recibir_paquete(conexion_memoria);
    if (paquete_recibido->codigo_operacion != LECTURA_MEMORIA)
    {
        log_error(logger_cpu, "error: codigo de operacion inesperado al recibir la lectura de memoria");
        eliminar_paquete(paquete_recibido);
        list_destroy_and_destroy_elements(direc_fisicas_si, free);
        list_destroy_and_destroy_elements(direc_fisicas_di, free);
        return;
    }

    paquete_recibido->buffer->offset = 0;
    void *buffer = malloc(tamanio);
    buffer_read(paquete_recibido->buffer, buffer, tamanio); // por ahie sto lo tena que cambiar
    eliminar_paquete(paquete_recibido);
    char *valor = malloc(tamanio);
    memcpy(valor, buffer, tamanio);
    t_direc_fisica *primer_direc_fisica_si = list_get(direc_fisicas_si,0);
    log_info(logger_cpu,"PID: %d - Acción: LEER - Dirección Física: %d - Valor: %s",pid,primer_direc_fisica_si->direccion_fisica,valor);

    // Escribir el string a la memoria apuntada por DI
    t_paquete *paquete_escritura = crear_paquete(ESCRITURA_MEMORIA);
    enviar_soli_escritura(paquete_escritura, direc_fisicas_di, tamanio, buffer, conexion_memoria,pid);

    op_code cod_op = recibir_operacion(conexion_memoria);
    if (cod_op != OK)
    {
        log_error(logger_cpu, "error: codigo de operacion inesperado al recibir la escritura de memoria");
    }
    t_direc_fisica *primer_direc_fisica_di = list_get(direc_fisicas_di,0);
    log_info(logger_cpu,"PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %s",pid,primer_direc_fisica_di->direccion_fisica,valor);

    free(buffer);
    list_destroy_and_destroy_elements(direc_fisicas_si, free);
    list_destroy_and_destroy_elements(direc_fisicas_di, free);
}

//--------------------MMU----------------------------
t_list *traducir_DL_a_DF_generico(uint32_t DL, uint32_t pid, size_t tamanio)
{
    uint32_t numero_pagina = DL / tamanio_de_pagina;
    uint32_t desplazamiento = DL % tamanio_de_pagina;
    uint32_t num_paginas = (desplazamiento + tamanio + tamanio_de_pagina - 1) / tamanio_de_pagina;

    t_list *direcciones_fisicas = list_create();
    size_t tamanio_restante = tamanio;

    t_list *paginas_a_traducir = list_create();
    for (uint32_t i = 0; i < num_paginas; i++)
    {
        int marco;
        uint32_t pagina_actual = numero_pagina + i;
        if(cant_entradas_tlb == 0){
         marco = -1;
        }else{
         marco = buscar_en_tlb(pagina_actual, pid);
        }
             
        t_direc_fisica *direc = malloc(sizeof(t_direc_fisica));
        direc->num_pag = pagina_actual;

        if (marco != -1)
        {
            // log_info(logger_cpu, "TLB HIT: Página %d, Marco %d", pagina_actual, marco);
            direc->direccion_fisica = marco * tamanio_de_pagina + ((i == 0) ? desplazamiento : 0);
            log_info(logger_cpu,"PID: %d - OBTENER MARCO - Página: %d - Marco: %d ",pid,pagina_actual,marco);
        }
        else
        {
            // log_info(logger_cpu, "TLB MISS: Página %d", pagina_actual);
            uint32_t *pagina_a_traducir = malloc(sizeof(uint32_t));
            *pagina_a_traducir = pagina_actual;
            list_add(paginas_a_traducir, pagina_a_traducir);
        }

        size_t espacio_disponible = (i == 0) ? (tamanio_de_pagina - desplazamiento) : tamanio_de_pagina;
        direc->desplazamiento_necesario = (tamanio_restante < espacio_disponible) ? tamanio_restante : espacio_disponible;
        tamanio_restante -= direc->desplazamiento_necesario;
        list_add(direcciones_fisicas, direc);
    }

    if (list_size(paginas_a_traducir) > 0)
    {
        t_paquete *paquete_solicitud = crear_paquete(ACCESO_TABLA_PAGINAS);
        buffer_add(paquete_solicitud->buffer, &pid, sizeof(uint32_t));
        uint32_t cantidad_paginas = list_size(paginas_a_traducir);
        buffer_add(paquete_solicitud->buffer, &cantidad_paginas, sizeof(uint32_t));

        for (int i = 0; i < list_size(paginas_a_traducir); i++)
        {
            uint32_t *pagina_actual = (uint32_t *)list_get(paginas_a_traducir, i);
            buffer_add(paquete_solicitud->buffer, pagina_actual, sizeof(uint32_t));
        }

        enviar_paquete(paquete_solicitud, conexion_memoria);
        eliminar_paquete(paquete_solicitud);

        t_paquete *paquete_respuesta = recibir_paquete(conexion_memoria);
        if (paquete_respuesta->codigo_operacion != ACCESO_TABLA_PAGINAS)
        {
            log_error(logger_cpu, "Error: código de operación inesperado al recibir los marcos de memoria");
            eliminar_paquete(paquete_respuesta);
            list_destroy_and_destroy_elements(direcciones_fisicas, free);
            list_destroy_and_destroy_elements(paginas_a_traducir, free);
            return NULL;
        }

        paquete_respuesta->buffer->offset = 0;
        for (int i = 0; i < list_size(paginas_a_traducir); i++)
        {
            uint32_t nro_marco;
            buffer_read(paquete_respuesta->buffer, &nro_marco, sizeof(uint32_t));
            uint32_t *pagina_actual = (uint32_t *)list_get(paginas_a_traducir, i);
            reemplazo_tlb(pid, *pagina_actual, nro_marco);

            for (int j = 0; j < list_size(direcciones_fisicas); j++)
            {
                t_direc_fisica *direc = (t_direc_fisica *)list_get(direcciones_fisicas, j);
                if (direc->num_pag == *pagina_actual)
                {
                    direc->direccion_fisica = nro_marco * tamanio_de_pagina + ((direc->num_pag == numero_pagina) ? desplazamiento : 0);
                    log_info(logger_cpu,"PID: %d - OBTENER MARCO - Página: %d - Marco: %d ",pid,*pagina_actual,nro_marco);
                    break;
                }
            }
        }

        eliminar_paquete(paquete_respuesta);
    }

    list_destroy_and_destroy_elements(paginas_a_traducir, free);

    list_sort(direcciones_fisicas, (void *)comparar_paginas);
    return direcciones_fisicas;
}

bool comparar_paginas(void *a, void *b)
{
    t_direc_fisica *direc_a = (t_direc_fisica *)a;
    t_direc_fisica *direc_b = (t_direc_fisica *)b;
    return direc_a->num_pag < direc_b->num_pag;
}
//-----------------------TLB-----------------------------
void agregar_entrada_tlb(u_int32_t id_proceso, u_int32_t numero_pagina, u_int32_t numero_marco)
{
    entrada_tlb *nueva_entrada = malloc(sizeof(entrada_tlb));
    struct timeval tiempo_actual;
    gettimeofday(&tiempo_actual, NULL);

    nueva_entrada->id_proceso = id_proceso;
    nueva_entrada->numero_pagina = numero_pagina;
    nueva_entrada->numero_marco = numero_marco;
    nueva_entrada->tiempo_creacion = tiempo_actual;
    nueva_entrada->ultimo_acceso = tiempo_actual;

    list_add(TLB, nueva_entrada);
    log_info(logger_cpu, "Entrada TLB agregada");
}

void liberar_entrada_tlb(entrada_tlb *entrada)
{
    free(entrada);
}

void reemplazo_tlb(int id_proceso, int numero_pagina, int numero_marco)
{
    if (cant_entradas_tlb == 0)
    {
        log_info(logger_cpu, "La cantidad de entradas de la TLB es 0");
        return;
    }
    if (list_size(TLB) < cant_entradas_tlb)
    {
        agregar_entrada_tlb(id_proceso, numero_pagina, numero_marco);
        log_info(logger_cpu, "Agrege una entrada a la TLB");
    }
    else
    {
        if (strcmp(algoritmo_tlb, "FIFO") == 0)
        {
            reemplazo_fifo();
        }
        else if (strcmp(algoritmo_tlb, "LRU") == 0)
        {
            reemplazo_lru();
        }

        agregar_entrada_tlb(id_proceso, numero_pagina, numero_marco);
        log_info(logger_cpu, "Remplace una entrada a la TLB");
    }
}

bool comparar_ultimo_acceso(void *primera_entrada, void *segunda_entrada)
{
    entrada_tlb *entrada1 = (entrada_tlb *)primera_entrada;
    entrada_tlb *entrada2 = (entrada_tlb *)segunda_entrada;

    if (entrada1->ultimo_acceso.tv_sec == entrada2->ultimo_acceso.tv_sec)
    {
        return (entrada1->ultimo_acceso.tv_usec < entrada2->ultimo_acceso.tv_usec);
    }
    return (entrada1->ultimo_acceso.tv_sec < entrada2->ultimo_acceso.tv_sec);
}

int buscar_en_tlb(int numero_pagina, int id_proceso)
{
    for (int i = 0; i < list_size(TLB); i++)
    {
        entrada_tlb *entrada = (entrada_tlb *)list_get(TLB, i);

        if ((entrada->numero_pagina == numero_pagina) && (entrada->id_proceso == id_proceso))
        {
            struct timeval tiempo_actual;
            gettimeofday(&tiempo_actual, NULL);
            entrada->ultimo_acceso = tiempo_actual;

            log_info(logger_cpu, "PID: %d - TLB HIT - Pagina: %d", id_proceso, numero_pagina);
            return entrada->numero_marco;
        }
    }
    log_info(logger_cpu, "PID: %d - TLB MISS - Pagina: %d", id_proceso, numero_pagina);
    return -1;
}

// Reemplazo FIFO
void reemplazo_fifo()
{
    list_remove_and_destroy_element(TLB, 0, (void *)liberar_entrada_tlb);
}

// Reemplazo LRU
void reemplazo_lru()
{
    list_sort(TLB, comparar_ultimo_acceso);
    list_remove_and_destroy_element(TLB, 0, (void *)liberar_entrada_tlb);
}
//------------------------funciones extra-------------------------


size_t calcular_tamano_mensaje(int pid, const t_instruccion *instruccion) {
    // Tamaño inicial con el PID y el identificador
    size_t size = snprintf(NULL, 0, "PID: %d - Ejecutando: %s - ", pid, identificador_to_string(instruccion->identificador));

    // Array con las longitudes de los parámetros
    uint32_t param_lengths[] = {
        instruccion->param1_length,
        instruccion->param2_length,
        instruccion->param3_length,
        instruccion->param4_length,
        instruccion->param5_length
    };

    // Tamaño de todos los parámetros usando las longitudes conocidas
    for (uint32_t i = 0; i < instruccion->cant_parametros; ++i) {
        size += param_lengths[i];
        if (i < instruccion->cant_parametros - 1) {
            size += 2; // Para la coma y el espacio
        }
    }

    return size + 1; // +1 para el terminador nulo
}

// Función para loguear una instrucción
void log_instruccion(int pid, const t_instruccion *instruccion) {
    // Calcular el tamaño necesario para el mensaje
    size_t message_size = calcular_tamano_mensaje(pid, instruccion);
    char *message = malloc(message_size);

    if (!message) {
        fprintf(stderr, "Error al asignar memoria para el mensaje de log\n");
        return;
    }

    // Formatear el mensaje
    snprintf(message, message_size, "PID: %d - Ejecutando: %s - ", pid, identificador_to_string(instruccion->identificador));

    // Concatenar los parámetros
    for (uint32_t i = 0; i < instruccion->cant_parametros; ++i) {
        strncat(message, instruccion->parametros[i], message_size - strlen(message) - 1);
        if (i < instruccion->cant_parametros - 1) {
            strncat(message, " - ", message_size - strlen(message) - 1);
        }
    }

    // Loguear el mensaje
    log_info(logger_cpu,message);

    // Liberar la memoria asignada
    free(message);
}