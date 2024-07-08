

#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <../include/utils.h>
#include <semaphore.h>

typedef struct
{
    uint32_t pid;
    t_instruccion *instruccion;
} t_instruccion_bloqueada_en_io;


typedef struct{
    pthread_mutex_t mutex;
    t_queue *cola;
} t_cola_interfaz_io;

extern t_config *config_kernel;

extern t_log *logger_kernel;
extern char *ip_memoria;
extern char *ip_cpu;
extern char *puerto_memoria;
extern char *puerto_escucha;
extern char *puerto_dispatch;
extern char *puerto_interrupt;
extern char *algoritmo_planificacion;
extern char *ip;
extern u_int32_t conexion_memoria, conexion_dispatch, conexion_interrupt, flag_cpu_libre;
extern int socket_servidor_kernel;
extern int contador_pcbs;
extern uint32_t contador_pid;
extern int quantum;
extern op_code motivo_ultimo_desalojo;
extern char *nombre_entrada_salida_conectada;
extern int grado_multiprogramacion;
extern t_pcb *ultimo_pcb_ejecutado;
extern uint32_t procesos_en_ready_plus;
extern uint16_t planificar_ready_plus;
extern sem_t podes_revisar_lista_bloqueados;



extern t_dictionary *conexiones_io;
extern t_dictionary *colas_blocks_io; // cola de cada interfaz individual, adentro están las isntrucciones a ejecutar
extern t_dictionary *diccionario_semaforos_io;
extern t_dictionary *recursos_disponibles;
extern t_dictionary *cola_de_bloqueados_por_recurso;
extern t_dictionary *recursos_asignados_por_proceso;
extern t_dictionary *tiempo_restante_de_quantum_por_proceso;

extern pthread_mutex_t mutex_motivo_ultimo_desalojo;
extern pthread_mutex_t mutex_pid;
extern pthread_mutex_t mutex_cola_de_readys;
extern pthread_mutex_t mutex_lista_de_blocked;
extern pthread_mutex_t mutex_cola_de_new;
extern pthread_mutex_t mutex_proceso_en_ejecucion;
extern pthread_mutex_t mutex_interfaces_conectadas;
extern pthread_mutex_t mutex_cola_interfaces; // PARA AGREGAR AL DICCIOANRIO de la cola de cada interfaz
extern pthread_mutex_t mutex_diccionario_interfaces_de_semaforos;
extern pthread_mutex_t mutex_flag_cpu_libre;
extern pthread_mutex_t mutex_procesos_en_sistema;
extern pthread_mutex_t mutex_cola_de_ready_plus;
extern sem_t contador_grado_multiprogramacion, hay_proceso_a_ready, cpu_libre, arrancar_quantum;
extern t_queue *cola_procesos_ready;
extern t_queue *cola_procesos_new;
extern t_queue *cola_procesos_exit;
extern t_queue *cola_ready_plus;
extern t_list *lista_procesos_blocked; // lsita de los prccesos bloqueados
extern t_list *procesos_en_sistema;
extern t_pcb *pcb_en_ejecucion;
extern pthread_t planificador_corto;
extern pthread_t dispatch;
extern pthread_t hilo_quantum;
extern pthread_t planificador_largo_creacion;
extern pthread_t planificador_largo_eliminacion;
extern char **recursos;
extern char **instancias_recursos;
extern pthread_mutex_t mutex_cola_de_exit;
extern sem_t hay_proceso_exit;
extern sem_t hay_proceso_nuevo;
extern sem_t hay_proceso_a_ready_plus;
extern pthread_t planificador_largo;

// --------------------- FUNCIONES DE INICIO -------------------------
void iniciar_semaforos();
void iniciar_diccionarios();
void iniciar_listas();
void iniciar_colas_de_estados_procesos();
void iniciar_config();
void iniciar_recursos();
void iniciar_variables();

void *atender_cliente(void *socket_cliente);

// --------------------- FUNCIONES DE CONSOLA INTERACTIVA -------------------------
void *iniciar_consola_interactiva();
bool validar_comando(char *comando);
void ejecutar_comando(char *comandoRecibido);
// void ejecutar_script(t_buffer* buffer);

// --------------------- FUNCIONES DE PCB -------------------------
void ejecutar_PCB(t_pcb *pcb);
void setear_pcb_en_ejecucion(t_pcb *pcb);
void set_add_pcb_cola(t_pcb *pcb, estados estado, t_queue *cola, pthread_mutex_t mutex);
t_pcb *buscar_pcb_por_pid(u_int32_t pid, t_list *lista);
void agregar_pcb_a_cola_bloqueados_de_recurso(t_pcb *pcb, char *nombre);
void finalizar_pcb(t_pcb *pcb);


// --------------------- FUNCIONES DE PLANIFICACION -------------------------
void iniciar_planificador_corto_plazo();
void iniciar_planificador_largo_plazo();
void creacion_de_procesos();
void eliminacion_de_procesos();
void *recibir_dispatch();
void *verificar_quantum();
void *verificar_quantum_vrr();

// --------------------- FUNCIONES DE INTERFACES ENTRADA/SALIDA -------------------------
bool interfaz_conectada(char *nombre_interfaz);
bool esOperacionValida(t_identificador identificador, cod_interfaz tipo);
void crear_interfaz(op_code tipo, char *nombre, uint32_t conexion);
void ejecutar_instruccion_io(char *nombre_interfaz, t_info_en_io *info_io, t_interfaz_en_kernel *conexion_io);
void atender_interfaz(char *nombre_interfaz);


// -------------------- FUNCIONES DE RECURSOS -------------------------------
void iniciar_recurso(char* nombre, char* instancias);
bool existe_recurso(char *nombre);
void liberar_recursos(uint32_t pid);
void retener_instancia_de_recurso(char *nombre_recurso, uint32_t pid);
int32_t restar_instancia_a_recurso(char *nombre);
void sumar_instancia_a_recurso(char *nombre);

// ---------------------- FUNCIONES DE LOGS --------------------------
void logear_bloqueo_proceso(uint32_t pid, char* motivo);
void logear_cambio_estado(uint32_t pid, char* estado_anterior, char * estado_actual);

#endif