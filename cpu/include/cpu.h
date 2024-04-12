#include <commons/log.h>
#include <commons/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <../src/utils/utils.c>

#ifndef CPU_H_
#define CPU_H_

extern t_log *logger_cpu;
extern t_config *config_cpu;
extern char *ip_memoria;
extern char *puerto_memoria;
extern char *puerto_dispatch;
extern char *puerto_interrupt;
extern u_int32_t conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt;
extern int socket_servidor_dispatch, socket_servidor_interrupt;
extern pthread_t hilo_dispatch, hilo_interrupt;

void iniciar_config();
void *iniciar_servidor_dispatch(void *arg);
void *iniciar_servidor_interrupt(void *arg);
#endif
