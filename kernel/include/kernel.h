

#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>
#include <commons/string.h>
#include<readline/readline.h>
#include <../include/utils.h>
#include <semaphore.h>

typedef struct 
{
    char *nombre_interfaz;
    t_list *lista_instrucciones;
} t_cola_bloqueados_io;

typedef struct{
    uint32_t pid;
    t_instruccion *instruccion;
} t_instruccion_bloqueada_en_io;


extern t_list *lista_procesos_ready;
extern t_config *config_kernel;

extern t_log *logger_kernel;
extern char *ip_kernel;
extern char *ip_io;
extern char *ip_memoria;
extern char *ip_cpu;
extern char *puerto_memoria;
extern char *puerto_io;
extern char *puerto_escucha;
extern char *puerto_cpu;
extern char *ip;
extern u_int32_t conexion_memoria, conexion_io, conexion_cpu;;
extern int contador_pcbs; 
extern int identificador_pid; 
extern int grado_multiprogramacion;
extern uint32_t contador_pid;
extern int quantum;

extern pthread_mutex_t mutex_pid;
extern pthread_mutex_t mutex_cola_de_readys;
extern sem_t contador_grado_multiprogramacion;
extern t_list *lista_procesos_ready;

void iniciar_config();
void *iniciar_consola_interactiva(); 
void* atender_cliente(void *socket_cliente);
bool validar_comando(char* comando); 
void ejecutar_comando(char* comandoRecibido); 
void ejecutar_script(t_buffer* buffer);
int asigno_pid(); 


#endif