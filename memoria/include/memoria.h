#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>
#include <../include/utils.h>
#include <commons/bitarray.h>

extern t_log *logger_memoria;
extern t_config *config_memoria;
extern t_config *config_conexiones;

extern char *PUERTO_MEMORIA;
extern int TAM_MEMORIA;
extern int TAM_PAGINA;
extern char *PATH_INSTRUCCIONES;
extern int RETARDO_RESPUESTA;

extern char *puerto_memoria;
extern int socket_servidor_memoria;

extern t_list *procesos_en_memoria;
extern pthread_mutex_t mutex;

extern t_bitarray *marcos_ocupados;
extern void* memoria_principal;

// ---------------------------FUNCIONES DE INICIO-------------------------------------
void iniciar_config(char *ruta_config, char *ruta_logger);
void iniciar_semaforos();
void inciar_listas();

// ---------------------------FUNCIONES DE ATENCION-------------------------------------
void *atender_cliente(void *socket_cliente);

#endif