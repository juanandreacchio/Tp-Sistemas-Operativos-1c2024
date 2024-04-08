#ifndef UTILS_H_
#define UTILS_H_r

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include <commons/config.h>

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;s
} t_paquete;

t_log* iniciar_logger(char *path, char *nombre, t_log_level nivel);
t_config *iniciar_config(char *path);
int crear_conexion(char* ip, char* puerto);
void liberar_conexion(int socket_cliente);

/*
void recibir_mensaje(int socket_cliente, t_log* logger);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
void crear_buffer(t_paquete* paquete);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

*/
#endif /* UTILS_H_ */