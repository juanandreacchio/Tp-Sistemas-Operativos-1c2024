#include <../include/kernel.h>

t_config *config_kernel;

t_log *logger_kernel;
char *ip_memoria;
char *ip_cpu;
char *puerto_memoria;
char *puerto_escucha;
char *puerto_dispatch;
char *puerto_interrupt;
char *ip;
u_int32_t conexion_memoria, conexion_dispatch, conexion_interrupt;
int socket_servidor_kernel, socket_cliente_kernel;
int contador_pcbs, identificador_pid = 1; 
pthread_mutex_t* mutex_pid;

int main(void)
{
    iniciar_config();
    iniciar_consola_interactiva();
    // iniciar conexion con Kernel
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_kernel);
    enviar_mensaje("", conexion_memoria, KERNEL, logger_kernel);

    // iniciar conexion con CPU Dispatch e Interrupt
    conexion_dispatch = crear_conexion(ip_cpu, puerto_dispatch, logger_kernel);
    //conexion_interrupt = crear_conexion(ip_cpu, puerto_interrupt, logger_kernel);
    enviar_mensaje("", conexion_dispatch, KERNEL, logger_kernel);
    //enviar_mensaje("", conexion_interrupt, KERNEL, logger_kernel);
    



    //------------------------prueba de envio pcb------------------------------
    t_pcb *pcb;
    t_list *listaInstrucciones = list_create();

    //istruccion1
    t_list *parametros = list_create();
    char *num = "5";
    list_add(parametros, num);
    t_instruccion *instruccion1 = crear_instruccion(SUM,parametros);
    list_add(listaInstrucciones,instruccion1);

    //istruccion2
    t_list *parametros2 = list_create();
    char *num2 = "15";
    list_add(parametros2, num2);
    t_instruccion *instruccion2 =  crear_instruccion(SET,parametros2);
    list_add(listaInstrucciones,instruccion2);

    //envio pcb
    pcb = crear_pcb(10,listaInstrucciones,25,READY);
    enviar_pcb(pcb,conexion_dispatch);

    //----------------------prueba de las operaciones con registro-------------------------------

    t_registros registros = inicializar_registros();
    imprimir_registros_por_pantalla(registros);
    set_registro(&registros,"AX",10);
    imprimir_registros_por_pantalla(registros);
    set_registro(&registros,"BX",5);
    sum_registro(&registros,"AX","BX");
    imprimir_registros_por_pantalla(registros);
    set_registro(&registros,"CX",8);
    sub_registro(&registros,"CX","BX");
    imprimir_registros_por_pantalla(registros);
    JNZ_registro(&registros,"AX",15);
    imprimir_registros_por_pantalla(registros);






    // iniciar Servidor
    socket_servidor_kernel = iniciar_servidor(logger_kernel, puerto_escucha, "KERNEL");

    while (1)
    {
        pthread_t thread;
        int *socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(socket_servidor_kernel, logger_kernel);
        pthread_create(&thread, NULL,(void*) atender_cliente,socket_cliente);
        pthread_detach(thread);
    }

    log_info(logger_kernel, "Se cerrará la conexión.");
    terminar_programa(socket_servidor_kernel, logger_kernel, config_kernel);
}

void iniciar_config(void)
{
    config_kernel = config_create("config/kernel.config");
    logger_kernel = iniciar_logger("config/kernel.log", "KERNEL", LOG_LEVEL_INFO);
    ip_memoria = config_get_string_value(config_kernel, "IP_MEMORIA");
    ip_cpu = config_get_string_value(config_kernel, "IP_CPU");
    puerto_dispatch = config_get_string_value(config_kernel, "PUERTO_CPU_DISPATCH");
    puerto_interrupt = config_get_string_value(config_kernel, "PUERTO_CPU_INTERRUPT");
    puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
    puerto_escucha = config_get_string_value(config_kernel, "PUERTO_ESCUCHA");
}

void *atender_cliente(void *socket_cliente)
{
    op_code codigo_operacion = recibir_operacion(*(int*)socket_cliente);
    switch (codigo_operacion)
    {
    case ENTRADA_SALIDA:
        char* nombre_entrada_salida = recibir_mensaje_guardar_variable(*(int*)socket_cliente);
        log_info(logger_kernel, "Se conecto la I/O llamda: %s",nombre_entrada_salida);
        break;
    default:
        log_info(logger_kernel, "Se recibio un mensaje de un modulo desconocido");
        break;
    }
    return NULL;
}

// Planificacion

