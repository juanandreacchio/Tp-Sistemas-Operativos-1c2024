#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>

#include <../src/utils/utils.c>

#ifndef MEMORIA_H_
#define MEMORIA_H_

extern t_log *logger_memoria;
extern t_config *config_memoria;

extern char *PUERTO_MEMORIA;
extern int TAM_MEMORIA;
extern int TAM_PAGINA;
extern char *PATH_INSTRUCCIONES;
extern int RETARDO_RESPUESTA;

extern char *puerto_memoria;
extern char *mensaje_recibido;
extern int socket_servidor_memoria;

void iterator(char* value);
void iniciar_config();
void* atender_cliente(void *socket_cliente);

#endif