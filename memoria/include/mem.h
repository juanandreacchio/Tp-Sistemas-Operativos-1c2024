#ifndef MEM_H_
#define MEM_H_

#include <../include/memoria.h>

extern t_bitarray *marcos_ocupados;
extern void* memoria_principal;
typedef struct
{
	uint32_t pid;
	t_list *lista_instrucciones;
	t_list *tabla_paginas;
} t_proceso;

typedef struct {
    int numero_marco;
    bool presente;
} t_pagina;

extern t_list *tabla_paginas;
extern void* memoria_principal;
extern int TAM_PAGINA;
extern int TAM_MEMORIA;

//--------------------------ENVIAR TAMAÑO "handshake"---------------------------
void enviar_tamanio_pagina(int soket);
// ------------------ FUNCIONES DE MEMORIA

void inicializar_memoria_principal();
int obtener_primer_marco_libre();
void* obtener_direccion_fisica(int numero_marco);
void inicializar_bitarray(size_t size);
void destruir_bitarray();

// ------------------ FUNCIONES DE PROCESO EN MEMORIA
int posicion_proceso(t_list* lista_procesos, uint32_t pid);
t_proceso *crear_proceso(t_list* lista_procesos, int pid, char* path);
void imprimir_proceso(t_proceso *proceso);
t_solicitudCreacionProcesoEnMemoria *deserializar_solicitud_crear_proceso(t_buffer *buffer);
void imprimir_lista_de_procesos(t_list *lista_procesos);
t_proceso *buscar_proceso_por_pid(t_list *lista_procesos, uint32_t pid);

// ------------------ FUNCIONES DE INSTRUCCIONES
t_instruccion *buscar_instruccion(t_list* lista_procesos, uint32_t pid, uint32_t pc);
t_instruccion *string_to_instruccion(char *string);
t_instruccion *leer_instruccion(char* path, uint32_t pc);

// ------------------ FUNCIONES DE TABLA DE PAGINAS
t_pagina *inicializar_pagina(int numero_marco);
t_list* inicializar_tabla_paginas();
void* obtener_contenido_pagina(t_proceso* proceso, int numero_pagina);
void asignar_contenido_pagina(t_proceso* proceso, int numero_pagina, void* contenido);

// ------------------ FUNCIONES DE LIBERAR
void liberar_tabla_paginas(t_list* tabla_paginas);
void liberar_proceso(t_proceso* proceso);
void liberar_lista_procesos(t_list* lista_procesos);
void liberar_memoria_principal();
void liberar_marco_pagina(t_proceso* proceso, int numero_pagina);
void liberar_proceso_por_pid(uint32_t pid);
// void liberar_instruccion(t_instruccion* instruccion);//no esta en el mem.c
#endif