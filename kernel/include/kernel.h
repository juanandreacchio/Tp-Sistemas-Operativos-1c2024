

#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>
#include <commons/string.h>
#include<readline/readline.h>
#include <../src/utils/utils.h>


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
extern pthread_mutex_t *mutex_pid;
extern int contador_pcbs; 
extern int identificador_pid; 
extern int grado_multiprogramacion;

void iniciar_config();
void iniciar_consola_interactiva(); 
void* atender_cliente(void *socket_cliente);
bool validar_comando(char* comando); 
void ejecutar_comando(char* comandoRecibido); 
void ejecutar_script(t_buffer* buffer);
int asigno_pid(); 


#endif