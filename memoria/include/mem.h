#ifndef MEM_H_
#define MEM_H_

#include <../include/memoria.h>


// ------------------ FUNCIONES DE PROCESO EN MEMORIA
t_proceso *crear_proceso(t_list* lista_procesos, int pid, char* path);



// ------------------ FUNCIONES DE INSTRUCCIONES
t_instruccion *buscar_instruccion(t_list* lista_procesos, uint32_t pid, uint32_t pc);
t_instruccion *string_to_instruccion(char *string);
t_instruccion *leer_instruccion(char* path, uint32_t pc);

// ------------------ FUNCIONES DE TABLA DE PAGINAS
void inicializar_pagina(t_pagina* pagina, int numero_pagina);
t_tabla_paginas* inicializar_tabla_paginas();
void* obtener_contenido_pagina(t_proceso* proceso, int numero_pagina);
void asignar_contenido_pagina(t_proceso* proceso, int numero_pagina, void* contenido);

// ------------------ FUNCIONES DE LIBERAR
void liberar_tabla_paginas(t_tabla_paginas* tabla_paginas);
void liberar_proceso(t_proceso* proceso);
void liberar_lista_procesos(t_list* lista_procesos);
// void liberar_instruccion(t_instruccion* instruccion);//no esta en el mem.c
#endif