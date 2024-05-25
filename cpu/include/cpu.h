#include <commons/log.h>
#include <commons/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <../include/utils.h>


#ifndef CPU_H_
#define CPU_H_

extern t_log *logger_cpu;
extern t_config *config_cpu;
extern char *ip_memoria;
extern char *puerto_memoria;
extern char *puerto_dispatch;
extern char *puerto_interrupt;
extern u_int32_t conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt;
extern int socket_servidor_dispatch, socket_servidor_interrupt;
extern pthread_t hilo_dispatch, hilo_interrupt;
extern t_registros registros_cpu;
extern t_interrupcion *interrupcion_recibida;

extern u_int8_t interruption_flag;
extern u_int8_t end_process_flag;
extern u_int8_t input_ouput_flag;


// ------------------------- FUNCIONES DE INICIO ---------------------------
void iniciar_config();
void inicializar_flags();

// ------------------ FUNCIONES DE CONEXION -------------------------
void *iniciar_servidor_dispatch(void *arg);
void *iniciar_servidor_interrupt(void *arg);

// ------------------------ CICLO BASICO DE INSTRUCCION ------------------------
t_instruccion *fetch_instruccion(uint32_t pid, uint32_t *pc, uint32_t conexionParam);
void decode_y_execute_instruccion(t_instruccion *instruccion, t_pcb *pcb);
bool check_interrupt(uint32_t pid);

// ------------------------ FUNCIONES DE PCB ------------------------
t_instruccion *siguiente_instruccion(t_pcb *pcb, int socket);
void comenzar_proceso(t_pcb *pcb, int socket_Memoria, int socket_Kernel);
#endif
