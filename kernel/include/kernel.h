#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>
#include <../src/utils/utils.c>

#ifndef KERNEL_H_
#define KERNEL_H_



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


void iniciar_config();
void* atender_cliente(void *socket_cliente);


#endif