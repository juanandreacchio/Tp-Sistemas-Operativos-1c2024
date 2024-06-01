#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>

#include <../include/utils.h>

#ifndef ENTRADASALIDA_H_
#define ENTRADASALIDA_H_

typedef struct 
{
    char* nombre; //id de la interfaz
    char* ruta_archivo; // archivo de configuracion
}t_interfaz;


extern t_log *logger_entradasalida;
extern uint32_t socket_conexion_kernel, socket_conexion_memoria;

extern t_config *config_entradasalida;
extern char *ip_kernel;
extern char *puerto_kernel;
extern char *ip_memoria;
extern char *puerto_memoria;
extern pthread_t thread_memoria, thread_kernel;
extern cod_interfaz tipo_interfaz;
extern t_interfaz *interfaz_creada;


// ------------------------ FUNCIONES DE INICIO --------------------
void iniciar_config();
void *iniciar_conexion_kernel(void *arg);
void *iniciar_conexion_memoria(void *arg);
t_interfaz *iniciar_interfaz(char* nombre,char* ruta);

// ------------------------ FUNCIONES DE EJECUCION --------------------
void *atender_cliente(int socket_cliente);
#endif