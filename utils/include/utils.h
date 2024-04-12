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
	t_buffer* buffer;
} t_paquete;

t_log* iniciar_logger(char *path, char *nombre, t_log_level nivel);
int crear_conexion(char* ip, char* puerto);
void liberar_conexion(int socket_cliente);
int iniciar_servidor(t_log *logger, char *puerto, char *nombre);
int esperar_cliente(int socket_servidor, t_log *logger);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(char* mensaje, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
int recibir_operacion(int socket_cliente);
void *recibir_buffer(int *size, int socket_cliente);
void recibir_mensaje(int socket_cliente, t_log* logger);
t_list *recibir_paquete(int socket_cliente);



#endif /* UTILS_H_ */