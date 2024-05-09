#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <errno.h>


#define SIZE_REGISTROS 32
typedef enum
{
	SOLICITUD_INSTRUCCION,
	INSTRUCCION,
	CREAR_PROCESO,
	CPU,
	KERNEL,
	MEMORIA,
	ENTRADA_SALIDA,
	PCB

} op_code;

typedef struct
{
	uint32_t size;
	uint32_t offset;
	void *stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

typedef enum
{
	NEW,
	READY,
	EXEC,
	BLOCKED,
	TERMINATED //cambie de exit a salida porque esta tambien declarado como isntruccion
} estados;
typedef struct
{
	uint8_t AX, BX, CX, DX;					 
	uint32_t EAX, EBX, ECX, EDX, SI, DI, PC; 
} t_registros;

typedef struct
{
	u_int32_t pid;		   //  Identificador del proceso (deberá ser un número entero, único en todo el sistema)
	u_int32_t pc;		   // Program Counter (Número de la próxima instrucción a ejecutar)
	u_int32_t quantum;	   // Unidad de tiempo utilizada por el algoritmo de planificación VRR
	t_registros registros; // Estructura que contendrá los valores de los registros de uso general de la CPU
	t_list *instrucciones;
	estados estado_actual;
} t_pcb;

typedef enum
{
	// 5 PARÁMETROS
	IO_FS_WRITE,
	IO_FS_READ,
	// 3 PARAMETROS
	IO_FS_TRUNCATE,
	IO_STDOUT_WRITE,
	IO_STDIN_READ,
	// 2 PARAMETROS
	SET,
	MOV_IN,
	MOV_OUT,
	SUM,
	SUB,
	JNZ,
	IO_GEN_SLEEP,
	IO_FS_DELETE,
	IO_FS_CREATE,
	// 1 PARAMETRO
	RESIZE,
	COPY_STRING,
	WAIT,
	SIGNAL,
	// 0 PARAMETROS
	EXIT
} t_identificador;

typedef struct
{
	t_identificador identificador;
	uint32_t param1_length;
	uint32_t param2_length;
	uint32_t param3_length;
	uint32_t param4_length;
	uint32_t param5_length;
	uint32_t cant_parametros;
	char **parametros; // Lista de parámetros

} t_instruccion;





t_registros inicializar_registros();
t_pcb *crear_pcb(u_int32_t pid, t_list *lista_instrucciones, u_int32_t quantum, estados estado);
t_pcb *recibir_pcb( int socket);
void destruir_pcb(t_pcb *pcb);
t_log *iniciar_logger(char *path, char *nombre, t_log_level nivel);
int crear_conexion(char *ip, char *puerto, t_log *logger);
void liberar_conexion(int socket_cliente);
int iniciar_servidor(t_log *logger, char *puerto, char *nombre);
int esperar_cliente(int socket_servidor, t_log *logger);
void *serializar_paquete(t_paquete *paquete, int bytes);
t_paquete *crear_paquete(op_code codigo_operacion);
void enviar_paquete(t_paquete *paquete, int socket_cliente);

void enviar_mensaje(char *mensaje, int socket_cliente, op_code codigo_operaciom, t_log *logger);
void eliminar_paquete(t_paquete *paquete);
op_code recibir_operacion(int socket_cliente);
void *recibir_buffer(int *size, int socket_cliente);
void recibir_mensaje(int socket_cliente, t_log *logger);
t_paquete *recibir_paquete(int socket_cliente);
void buffer_read(t_buffer *buffer, void *data, uint32_t size);
void buffer_add(t_buffer *buffer, void *data, uint32_t size);
t_buffer *crear_buffer();
void destruir_buffer(t_buffer *buffer);
t_buffer *serializar_instruccion(t_instruccion *instruccion);
t_instruccion *instruccion_deserializar(t_buffer *buffer,u_int32_t offset);
t_buffer *serializar_lista_instrucciones(t_list *lista_instrucciones);
t_list *deserializar_lista_instrucciones(t_buffer *buffer,u_int32_t offset);
void imprimir_instruccion(t_instruccion instruccion);

#endif /* UTILS_H_ */