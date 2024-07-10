#include <commons/log.h>
#include <commons/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <../include/utils.h>
#include <../include/utils.h>
#include <sys/time.h>
#include <time.h>

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

typedef struct {
    u_int32_t id_proceso;
    u_int32_t numero_pagina;
    u_int32_t numero_marco;
    struct timeval tiempo_creacion;
    struct timeval ultimo_acceso;
} entrada_tlb;

typedef struct {
    uint32_t numero_pagina;
    t_direc_fisica* direc_fisica;
} t_pagina_direccion;


// ------------------------- FUNCIONES DE INICIO ---------------------------
void iniciar_config();
void inicializar_flags();

//--------------------------RECIBIR TAMAÑO "handshake"---------------------------
u_int32_t recibir_tamanio(u_int32_t  socket_cliente);

// ------------------ FUNCIONES DE CONEXION -------------------------
void *iniciar_servidor_dispatch(void *arg);
void *iniciar_servidor_interrupt(void *arg);

// ------------------------ CICLO BASICO DE INSTRUCCION ------------------------
t_instruccion *fetch_instruccion(uint32_t pid, uint32_t *pc, uint32_t conexionParam);
void decode_y_execute_instruccion(t_instruccion *instruccion, t_pcb *pcb);
bool check_interrupt(uint32_t pid);
void accion_interrupt(t_pcb *pcb, op_code motivo, int socket);

// ------------------------ FUNCIONES DE PCB ------------------------
t_instruccion *siguiente_instruccion(t_pcb *pcb, int socket);
void comenzar_proceso(t_pcb *pcb, int socket_Memoria, int socket_Kernel);

//------------------------FUNCIONES DE OPERACIONES------------------------------
void set_registro(t_registros *registros, char *registro, u_int32_t valor);
u_int8_t get_registro_int8(t_registros *registros, char *registro);
u_int32_t get_registro_int32(t_registros *registros, char *registro);
u_int32_t get_registro_generico(t_registros *registros, char *registro);
void sum_registro(t_registros *registros, char *registroOrigen, char *registroDestino);
void sub_registro(t_registros *registros, char *registroOrigen, char *registroDestino);
void JNZ_registro(t_registros *registros, char *registro, u_int32_t valor);
void mov_in(t_pcb *pcb, char *registro_datos, char *registro_direccion);
void mov_out(t_pcb *pcb, char *registro_direccion, char *registro_datos);
void copy_string(t_pcb *pcb, size_t tamanio);

//--------------------MMU----------------------------
t_list *traducir_DL_a_DF_generico(uint32_t DL, uint32_t pid, size_t tamanio);
bool comparar_paginas(void* a, void* b);

//-----------------------TLB-----------------------------
void agregar_entrada_tlb(u_int32_t id_proceso, u_int32_t numero_pagina, u_int32_t numero_marco);
void liberar_entrada_tlb(entrada_tlb* entrada);
void reemplazo_tlb(int id_proceso, int numero_pagina, int numero_marco);
bool comparar_ultimo_acceso(void* primera_entrada, void* segunda_entrada);
int buscar_en_tlb(int numero_pagina, int id_proceso);
void reemplazo_fifo();
void reemplazo_lru();
#endif
