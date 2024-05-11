#include <../include/utils.h>

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

// int main(void)
// {
// 	iniciar_config();

// 	conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_cpu);
// 	enviar_mensaje("", conexion_memoria, CPU, logger_cpu);

// 	// iniciar servidor Dispatch y Interrupt
// 	pthread_create(&hilo_dispatch, NULL, iniciar_servidor_dispatch, NULL);
// 	pthread_create(&hilo_interrupt, NULL, iniciar_servidor_interrupt, NULL);

// 	pthread_join(hilo_dispatch, NULL);
// 	pthread_join(hilo_interrupt, NULL);
// }

void iniciar_config()
{
    config_cpu = config_create("./config/cpu.config");
    logger_cpu = iniciar_logger("config/cpu.log", "CPU", LOG_LEVEL_INFO);
    ip_memoria = config_get_string_value(config_cpu, "IP_MEMORIA");
    puerto_dispatch = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_DISPATCH");
    puerto_memoria = config_get_string_value(config_cpu, "PUERTO_MEMORIA");
    puerto_interrupt = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_INTERRUPT");
}

// void *iniciar_servidor_dispatch(void *arg)
// {
// 	socket_servidor_dispatch = iniciar_servidor(logger_cpu, puerto_dispatch, "DISPATCH");
// 	while (1)
// 	{
// 		conexion_kernel_dispatch = esperar_cliente(socket_servidor_dispatch, logger_cpu);
// 		log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Dispatch", cod_op_to_string(recibir_operacion(conexion_kernel_dispatch)));
// 		recibir_mensaje(conexion_kernel_dispatch, logger_cpu);

// 		t_pcb *pcb = recibir_pcb(conexion_kernel_dispatch);
// 		log_info(logger_cpu,"recibi el pcb");
// 		log_info(logger_cpu,"del pid tengo:%u",pcb->pid);
// 		log_info(logger_cpu,"del quantum tengo:%u",pcb->quantum);
// 		log_info(logger_cpu,"del psw tengo:%u",pcb->estado_actual);
// 		log_info(logger_cpu,"del pc tengo:%u",pcb->pc);
// 		t_instruccion *inst1 = list_get(pcb->instrucciones,0);
// 		t_instruccion *inst2 = list_get(pcb->instrucciones,1);
// 		imprimir_instruccion(*inst1);
// 		imprimir_instruccion(*inst2);

// 	}
// 	return NULL;
// }

// void *iniciar_servidor_interrupt(void *arg)
// {
// 	socket_servidor_interrupt = iniciar_servidor(logger_cpu, puerto_interrupt, "INTERRUPT");
// 	while (1)
// 	{
// 		conexion_kernel_interrupt = esperar_cliente(socket_servidor_interrupt, logger_cpu);
// 		log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Interrupt", cod_op_to_string(recibir_operacion(conexion_kernel_interrupt)));
// 		recibir_mensaje(conexion_kernel_interrupt, logger_cpu);
// 	}
// 	return NULL;
// }

t_instruccion *fetch_instruccion(uint32_t pid, uint32_t *pc)
{
    // Buscar instruccion en memoria
    t_paquete *paquete = crear_paquete(SOLICITUD_INSTRUCCION);
    // Le voy a mandar a memoria el paquete con el pid y el pc,
    // para q busque en la lista de procesos el proceso, y me devuelva la instruccion según el PC

    t_buffer *buffer = crear_buffer();
    
    buffer_add(buffer, &pid, sizeof(uint32_t));
    buffer_add(buffer, pc, sizeof(uint32_t));
    paquete->buffer = buffer;
    

    enviar_paquete(paquete, conexion_memoria);

    log_info(logger_cpu, "Se envio la solicitud de instruccion a memoria con pid: %d y pc: %d", pid, *pc);

    recv(conexion_memoria, "hola", 5, 0);

    log_info(logger_cpu, "Se recibio la instruccion de memoria");


    t_paquete *respuesta_memoria = recibir_paquete(conexion_memoria);

    if (respuesta_memoria->codigo_operacion != INSTRUCCION){
        log_error(logger_cpu, "Error al recibir instruccion de memoria");
        eliminar_paquete(paquete);
        eliminar_paquete(respuesta_memoria);
        return NULL;
    }

    t_instruccion *instruccion = instruccion_deserializar(respuesta_memoria->buffer, 0);
    imprimir_instruccion(*instruccion);

    *pc += 1; // Nosé si está bien así

    eliminar_paquete(paquete);
    eliminar_paquete(respuesta_memoria);
    return instruccion;
}

// void decode_y_execute_instruccion(t_instruccion *instruccion)
// {
// TODO MatiD
// 	switch (instruccion->identificador)
// 	{
// 	case SET:
// 		break;
// 	case MOV_IN:
// 		break;
// 	case MOV_OUT:
// 		break;
// 	case SUM:
// 		break;
// 	case SUB:
// 		break;
// 	case JNZ:
// 		break;
// 	case IO_FS_TRUNCATE:
// 		break;
// 	case IO_STDIN_READ:
// 		break;
// 	case IO_STDOUT_WRITE:
// 		break;
// 	case IO_GEN_SLEEP:
// 		break;
// 	case IO_FS_DELETE:
// 		break;
// 	case IO_FS_CREATE:
// 		break;
// 	case IO_FS_WRITE:
// 		break;
// 	case IO_FS_READ:
// 		break;
// 	case RESIZE:
// 		break;
// 	case COPY_STRING:
// 		break;
// 	case WAIT:
// 		break;
// 	case SIGNAL:
// 		break;
// 	case EXIT:
// 		break;
// 	default:
// 		break;
// 	}
// }

t_log *logger;
t_config *config;

u_int32_t conexion;
int main(void)
{
    iniciar_config();

    logger = iniciar_logger("config/test.log", "TEST", LOG_LEVEL_INFO);
    conexion = crear_conexion("127.0.0.1", "6004", logger);
    t_paquete *paquete = crear_paquete(CREAR_PROCESO);

    char *path = "test";
    uint32_t pid = 5;
    t_solicitudCreacionProcesoEnMemoria *ptr_solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));
    ptr_solicitud->pid = pid;
    ptr_solicitud->path_length = strlen(path) + 1;
    ptr_solicitud->path = path;
    t_buffer *buffer = serializar_solicitud_crear_proceso(ptr_solicitud);
    paquete->buffer = buffer;
    enviar_paquete(paquete, conexion);
    printf("Se envio el paquete 1\n");
    eliminar_paquete(paquete);
    
    
    
    // Pruebas para solicitar instrucciones a memoria

    // SOLICITUD_INSTRUCCION, se envia el pid y el program counter
    uint32_t pcInicial = 0;
    t_instruccion *instruccionRecibida = fetch_instruccion(5, &pcInicial);
    imprimir_instruccion(*instruccionRecibida);
    printf("PC: %d\n", pcInicial);

    destruir_instruccion(instruccionRecibida);

    terminar_programa(conexion, logger, config);
    return 0;
}
