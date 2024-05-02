#ifndef UTILS_H_
#define UTILS_H_r

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

typedef enum
{
	KERNEL,
	CPU,
	MEMORIA,
	ENTRADA_SALIDA,
} t_modulo;

typedef enum{
	INSTRUCCION,
	SOLICITUD_INSTRUCCION,
	MENSAJE,
} op_code;

typedef struct
{
	int size;
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
	EXIT
}t_psw;
typedef struct
{
    uint8_t AX,BX,CX,DX; // Registro Numérico de propósito general
    uint32_t EAX,EBX,ECX,EDX, SI, DI, PC; // Registro Numérico de propósito general
	
}t_registros;

typedef struct {
    u_int32_t pid; //  Identificador del proceso (deberá ser un número entero, único en todo el sistema)
    u_int32_t pc; // Program Counter (Número de la próxima instrucción a ejecutar)
    u_int32_t quantum; // Unidad de tiempo utilizada por el algoritmo de planificación VRR
    t_registros registros; //Estructura que contendrá los valores de los registros de uso general de la CPU
	t_list* instrucciones;
}t_pcb;

typedef enum{
	SET,
	MOV_IN,
	MOV_OUT,
	SUM,
	SUB,
	JNZ,
	RESIZE,
	COPY_STRING,
	WAIT,
	SIGNAL,
	IO_GEN_SLEEP,
	IO_STDIN_READ,
	IO_STDOUT_WRITE,
	IO_STDOUT_WRITE,
	IO_FS_DELETE,
	IO_FS_TRUNCATE,
	IO_FS_WRITE,
	IO_FS_READ,
	EXIT
} t_identificador;

typedef struct{
	t_identificador identificador;
	uint32_t cant_parametros;
	char** parametros; // Lista de parámetros?

} t_instruccion;

t_registros inicializar_registros();
t_pcb* crear_pcb(u_int32_t pid, u_int32_t quantum);
void destruir_pcb(t_pcb* pcb);
t_log *iniciar_logger(char *path, char *nombre, t_log_level nivel);
int crear_conexion(char *ip, char *puerto, t_log *logger);
void liberar_conexion(int socket_cliente);
int iniciar_servidor(t_log *logger, char *puerto, char *nombre);
int esperar_cliente(int socket_servidor, t_log *logger);
void *serializar_paquete(t_paquete *paquete, int bytes);
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void enviar_mensaje(char *mensaje, int socket_cliente, op_code codigo_operaciom, t_log *logger);
void eliminar_paquete(t_paquete *paquete);
op_code recibir_operacion(int socket_cliente);
void *recibir_buffer(int *size, int socket_cliente);
void recibir_mensaje(int socket_cliente, t_log *logger);
t_list *recibir_paquete(int socket_cliente);

#endif /* UTILS_H_ */