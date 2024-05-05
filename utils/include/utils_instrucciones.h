#ifndef UTILS_INSTRUCCIONES_H_
#define UTILS_INSTRUCCIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include <errno.h>
#include "utils.h"

t_buffer *serializar_instruccion(t_instruccion *instruccion);
t_instruccion *instruccion_deserializar(t_buffer *buffer);
t_buffer *serializar_lista_instrucciones(t_list *lista_instrucciones);
t_list *deserializar_lista_instrucciones(t_buffer *buffer);

#endif