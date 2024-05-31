#include <../include/kernel.h>

t_config *config_kernel;

t_log *logger_kernel;
char *ip_memoria;
char *ip_cpu;
char *puerto_memoria;
char *puerto_escucha;
char *puerto_dispatch;
char *puerto_interrupt;
char *algoritmo_planificacion;
char *ip;
u_int32_t conexion_memoria, conexion_dispatch, conexion_interrupt, flag_cpu_libre;
int socket_servidor_kernel;
int contador_pcbs;
uint32_t contador_pid;
int quantum;
char *nombre_entrada_salida_conectada;

t_dictionary *conexiones_io;
t_dictionary *colas_blocks_io; // cola de cada interfaz individual, adentro están las isntrucciones a ejecutar
t_dictionary *diccionario_semaforos_io;
t_dictionary *recursos_disponibles;
t_dictionary *cola_de_bloqueados_por_recurso;
t_dictionary *recursos_asignados_por_proceso;

pthread_mutex_t mutex_pid;
pthread_mutex_t mutex_cola_de_readys;
pthread_mutex_t mutex_lista_de_blocked;
pthread_mutex_t mutex_cola_de_new;
pthread_mutex_t mutex_proceso_en_ejecucion;
pthread_mutex_t mutex_interfaces_conectadas;
pthread_mutex_t mutex_cola_interfaces; // PARA AGREGAR AL DICCIOANRIO de la cola de cada interfaz
pthread_mutex_t mutex_diccionario_interfaces_de_semaforos;
pthread_mutex_t mutex_flag_cpu_libre;
pthread_mutex_t mutex_motivo_ultimo_desalojo;
sem_t contador_grado_multiprogramacion, hay_proceso_a_ready, cpu_libre, arrancar_quantum;
pthread_mutex_t mutex_cola_de_exit;
sem_t hay_proceso_exit;
sem_t hay_proceso_nuevo;
t_queue *cola_procesos_ready;
t_queue *cola_procesos_new;
t_queue *cola_procesos_exit;
t_list *lista_procesos_blocked; // lsita de los prccesos bloqueados
t_pcb *pcb_en_ejecucion;
pthread_t planificador_corto;
pthread_t planificador_largo;
pthread_t dispatch;
pthread_t hilo_quantum;
pthread_t planificador_largo_creacion;
pthread_t planificador_largo_eliminacion;
op_code motivo_ultimo_desalojo;
char **recursos;
char **instancias_recursos;

int grado_multiprogramacion;

int main(void)
{
    iniciar_config();
    iniciar_listas();
    iniciar_colas_de_estados_procesos();
    iniciar_diccionarios();
    iniciar_semaforos();
    iniciar_recursos();

    // iniciar conexion con Kernel
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_kernel);
    enviar_mensaje("", conexion_memoria, KERNEL, logger_kernel);

    // iniciar conexion con CPU Dispatch e Interrupt
    conexion_dispatch = crear_conexion(ip_cpu, puerto_dispatch, logger_kernel);
    conexion_interrupt = crear_conexion(ip_cpu, puerto_interrupt, logger_kernel);
    enviar_mensaje("", conexion_dispatch, KERNEL, logger_kernel);
    enviar_mensaje("", conexion_interrupt, KERNEL, logger_kernel);

    // iniciar Servidor
    socket_servidor_kernel = iniciar_servidor(logger_kernel, puerto_escucha, "KERNEL");

    pthread_create(&dispatch, NULL, (void *)recibir_dispatch, NULL);
    pthread_detach(dispatch);

    pthread_create(&planificador_corto, NULL, (void *)iniciar_planificador_corto_plazo, NULL);
    pthread_detach(planificador_corto);

    pthread_create(&planificador_largo, NULL, (void *)iniciar_planificador_largo_plazo, NULL);
    pthread_detach(planificador_largo);

    pthread_t thread_consola;
    pthread_create(&thread_consola, NULL, (void *)iniciar_consola_interactiva, NULL);
    pthread_detach(thread_consola);

    if (strcmp(algoritmo_planificacion, "RR") == 0)
    {
        log_info(logger_kernel, "iniciando contador de quantum");
        pthread_create(&hilo_quantum, NULL, (void *)verificar_quantum, NULL);
        pthread_detach(hilo_quantum);
    }

    while (1)
    {
        pthread_t thread;
        int *socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(socket_servidor_kernel, logger_kernel);
        pthread_create(&thread, NULL, (void *)atender_cliente, socket_cliente);
        pthread_detach(thread);
    }

    log_info(logger_kernel, "Se cerrará la conexión.");
    terminar_programa(socket_servidor_kernel, logger_kernel, config_kernel);
}





