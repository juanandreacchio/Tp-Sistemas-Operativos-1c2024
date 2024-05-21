#ifndef MEM_H_
#define MEM_H_

#include <../include/utils.h>

extern char *PUERTO_MEMORIA;
extern int TAM_MEMORIA;
extern int TAM_PAGINA;
extern char *PATH_INSTRUCCIONES;
extern int RETARDO_RESPUESTA;




extern t_list* procesos_en_memoria;
extern int NUM_PAGINAS;

t_proceso *crear_proceso(t_list* lista_procesos, int pid, char* path);
void liberar_proceso(t_proceso* proceso);
t_instruccion *leer_instruccion(char* path, uint32_t pc);
void liberar_lista_procesos(t_list* lista_procesos);
void liberar_instruccion(t_instruccion* instruccion);//no esta en el mem.c

void inicializar_pagina(t_pagina* pagina, int numero_pagina);
t_tabla_paginas* inicializar_tabla_paginas();
void liberar_tabla_paginas(t_tabla_paginas* tabla_paginas);
void* obtener_contenido_pagina(t_proceso* proceso, int numero_pagina);
void asignar_contenido_pagina(t_proceso* proceso, int numero_pagina, void* contenido);
t_instruccion *buscar_instruccion(t_list* lista_procesos, uint32_t pid, uint32_t pc);
t_instruccion *string_to_instruccion(char *string);
#endif