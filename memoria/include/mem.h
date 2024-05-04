#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>

#include <../src/utils/utils.c>
#include <mem.h>

extern int NUM_PAGINAS;
extern int TAM_PAGINA;

typedef struct {
    void* contenido;
    int pagina;
} t_pagina;

typedef struct {
    t_pagina* tabla_paginas[NUM_PAGINAS];
} t_tabla_paginas;

typedef struct
{
	uint32_t pid;
	char* path;
    t_tabla_paginas* tabla_paginas;
} t_proceso;

extern t_list* procesos;

t_proceso* crear_proceso(int pid, char* path);
void liberar_proceso(t_proceso* proceso);
t_instruccion leer_instruccion(char* path);
t_instruccion *buscar_instruccion(t_list* lista_procesos, uint32_t pid, uint32_t pc);
void liberar_lista_procesos(t_list* lista_procesos);
void liberar_instruccion(t_instruccion* instruccion);

void inicializar_pagina(t_pagina* pagina, int numero_pagina);
t_tabla_paginas* inicializar_tabla_paginas();
void liberar_tabla_paginas(t_tabla_paginas* tabla_paginas);
void* obtener_contenido_pagina(t_proceso* proceso, int numero_pagina);
void asignar_contenido_pagina(t_proceso* proceso, int numero_pagina, void* contenido);

