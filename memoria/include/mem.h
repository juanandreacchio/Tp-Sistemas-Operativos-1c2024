#ifndef MEM_H_
#define MEM_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>
#include <commons/string.h>

#include <../include/utils.h>
#include <../include/memoria.h>

extern char *PUERTO_MEMORIA;
extern int TAM_MEMORIA;
extern int TAM_PAGINA;
extern char *PATH_INSTRUCCIONES;
extern int RETARDO_RESPUESTA;

typedef struct {
    void* contenido;
    int pagina;
} t_pagina;

typedef struct {
    t_pagina** tabla_paginas;
} t_tabla_paginas;

typedef struct
{
	uint32_t pid;
	char* path;
    t_tabla_paginas* tabla_paginas;
} t_proceso;

extern t_list* procesos;
extern int NUM_PAGINAS;

void crear_proceso(t_list* lista_procesos, int pid, char* path);
void liberar_proceso(t_proceso* proceso);
t_instruccion *leer_instruccion(char* path, uint32_t pc);
void liberar_lista_procesos(t_list* lista_procesos);
void liberar_instruccion(t_instruccion* instruccion);

void inicializar_pagina(t_pagina* pagina, int numero_pagina);
t_tabla_paginas* inicializar_tabla_paginas();
void liberar_tabla_paginas(t_tabla_paginas* tabla_paginas);
void* obtener_contenido_pagina(t_proceso* proceso, int numero_pagina);
void asignar_contenido_pagina(t_proceso* proceso, int numero_pagina, void* contenido);
t_instruccion *buscar_instruccion(t_list* lista_procesos, uint32_t pid, uint32_t pc);
t_instruccion *string_to_instruccion(char *string);
#endif