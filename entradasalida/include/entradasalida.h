#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>

#include <../src/utils/utils.c>

#ifndef ENTRADASALIDA_H_
#define ENTRADASALIDA_H_

extern t_log *logger_entradasalida;
extern uint32_t conexion_kernel, conexion_memoria;

extern t_config *config_entradasalida;
extern char *ip_kernel;
extern char *puerto_kernel;
extern char *ip_memoria;
extern char *puerto_memoria;
extern pthread_t thread_memoria, thread_kernel;

void iniciar_config();
void *iniciar_conexion_kernel(void *arg);
void *iniciar_conexion_memoria(void *arg);
#endif