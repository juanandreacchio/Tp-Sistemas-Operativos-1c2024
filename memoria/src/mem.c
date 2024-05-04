#include <../include/mem.h>

// --------------------- FUNCIONES DE TABLA DE PAGINAS ---------------------
void inicializar_pagina(t_pagina* pagina, int numero_pagina) {
    pagina->contenido = NULL;
    pagina->pagina = numero_pagina;
}

t_tabla_paginas* inicializar_tabla_paginas() {
    t_tabla_paginas* tabla_paginas = malloc(sizeof(t_tabla_paginas));
    if (tabla_paginas == NULL) {
        printf("Error: no se pudo asignar memoria para la tabla de páginas\n");
        exit(1);
    }
    for (int i = 0; i < NUM_PAGINAS; i++) {
        t_pagina* pagina = malloc(sizeof(t_pagina));
        if (pagina == NULL) {
            printf("Error: no se pudo asignar memoria para la página %d\n", i);
            exit(1);
        }
        inicializar_pagina(pagina, i);
        tabla_paginas->tabla_paginas[i] = pagina;
    }
    return tabla_paginas;
}

void liberar_tabla_paginas(t_tabla_paginas* tabla_paginas) {
    for (int i = 0; i < NUM_PAGINAS; i++) {
        free(tabla_paginas->tabla_paginas[i]);
    }
    free(tabla_paginas);
}

void* obtener_contenido_pagina(t_proceso* proceso, int numero_pagina) {
    t_pagina* pagina = proceso->tabla_paginas->tabla_paginas[numero_pagina];
    return pagina->contenido;
}

void asignar_contenido_pagina(t_proceso* proceso, int numero_pagina, void* contenido) {
    t_pagina* pagina = proceso->tabla_paginas->tabla_paginas[numero_pagina];
    pagina->contenido = contenido;
}

// --------------------- FUNCIONES DE PROCESO ---------------------
// Función para crear un proceso y agregarlo a la lista de procesos
void crear_proceso(t_list* lista_procesos, int pid, char* path) {
    t_proceso* proceso = malloc(sizeof(t_proceso));
    if (proceso == NULL) {
        printf("Error: no se pudo asignar memoria para el proceso con PID %d\n", pid);
        exit(1);
    }
    proceso->pid = pid;
    proceso->path = malloc(strlen(PATH_INSTRUCCIONES) + strlen(path) + 1);
    if (proceso->path == NULL) {
        printf("Error: no se pudo asignar memoria para el path del proceso con PID %d\n", pid);
        exit(1);
    }
    strcpy(proceso->path, PATH_INSTRUCCIONES);
    strcat(proceso->path, path);
    proceso->tabla_paginas = inicializar_tabla_paginas();
    list_add(lista_procesos, proceso);
}

// Función para liberar un proceso
void liberar_proceso(t_proceso* proceso) {
    liberar_tabla_paginas(proceso->tabla_paginas);
    free(proceso);
}

// Funcion que a partir de un path de un archivo y el program counter, devuelve una instruccion
t_instruccion *leer_instruccion(char* path, uint32_t pc) {
    FILE* archivo = fopen(path, "r");
    if (archivo == NULL) {
        printf("Error: no se pudo abrir el archivo %s\n", path);
        exit(1);
    }
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    if (instruccion == NULL) {
        printf("Error: no se pudo asignar memoria para la instrucción\n");
        exit(1);
    }

    // Leo todas las instrucciones anteriores y las descarto
    for (int i = 0; i < pc; i++) {
        char* buffer = NULL;
        size_t buffer_size = 0;
        ssize_t bytes_leidos = getline(&buffer, &buffer_size, archivo);
        if (bytes_leidos == -1) {
            printf("Error: no se pudo leer la instrucción del archivo %s\n", path);
            exit(1);
        }
        free(buffer);
    }

    // Leo la instrucción que necesito
    char* buffer = NULL;
    size_t buffer_size = 0;
    ssize_t bytes_leidos = getline(&buffer, &buffer_size, archivo);
    if (bytes_leidos == -1) {
        printf("Error: no se pudo leer la instrucción del archivo %s\n", path);
        exit(1);
    }
    *instruccion = string_to_instruccion(buffer);
    free(buffer);
    fclose(archivo);
    return instruccion;
}

// Funcion que busque una instruccion en un proceso a partir de un PID y un PC
t_instruccion *buscar_instruccion(t_list* lista_procesos, uint32_t pid, uint32_t pc) {
    t_proceso* proceso = NULL;
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso* proceso_actual = list_get(lista_procesos, i);
        if (proceso->pid == pid) {
            proceso = proceso_actual;
            break;
        }
    }
    if (proceso == NULL) {
        printf("Error: no se encontró el proceso con PID %d\n", pid);
        exit(1);
    }
    return leer_instruccion(proceso->path);
}

void liberar_lista_procesos(t_list* lista_procesos) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso* proceso = list_get(lista_procesos, i);
        liberar_proceso(proceso);
    }
    list_destroy(lista_procesos);
}

void liberar_instruccion(t_instruccion* instruccion) {
    for (int i = 0; i < instruccion->cant_parametros; i++) {
        free(instruccion->parametros[i]);
    }
    free(instruccion->parametros);
    free(instruccion);
}

t_instruccion string_to_instruccion(char *string)
{
	t_instruccion instruccion;
	char** tokens = string_split(string, ' ');
	char* instruccion = tokens[0];
	char** parametros = tokens + 1;
	switch (tokens[0]) {
		case "IO_FS_WRITE":
			instruccion = nueva_instruccion(IO_FS_WRITE, parametros, 5);
		break;
		case "IO_FS_READ":
				instruccion = nueva_instruccion(IO_FS_READ, parametros, 5);
		break;
		case "IO_FS_TRUNCATE":
			instruccion = nueva_instruccion(IO_FS_TRUNCATE, parametros, 3);
		break;
		case "IO_STDOUT_WRITE":
			instruccion = nueva_instruccion(IO_STDOUT_WRITE, parametros, 3);
		break;
		case "IO_STDIN_READ":
			instruccion = nueva_instruccion(IO_STDIN_READ, parametros, 3);
		break;
		case "SET":
			instruccion = nueva_instruccion(SET, parametros, 2);
		break;
		case "MOV_IN":
			instruccion = nueva_instruccion(MOV_IN, parametros, 2);
		break;
		case "MOV_OUT":
			instruccion = nueva_instruccion(MOV_OUT, parametros, 2);
		break;
		case "SUM":
			instruccion = nueva_instruccion(SUM, parametros, 2);
		break;
		case "SUB":
			instruccion = nueva_instruccion(SUB, parametros, 2);
		break;
		case "JNZ":
			instruccion = nueva_instruccion(JNZ, parametros, 2);
		break;
		case "IO_GEN_SLEEP":
			instruccion = nueva_instruccion(IO_GEN_SLEEP, parametros, 2);
		break;
		case "IO_FS_DELETE":
			instruccion = nueva_instruccion(IO_FS_DELETE, parametros, 2);
		break;
		case "IO_FS_CREATE":
			instruccion = nueva_instruccion(IO_FS_CREATE, parametros, 2);
		break;
		case "RESIZE":
			instruccion = nueva_instruccion(RESIZE, parametros, 1);
		break;
		case "COPY_STRING":
			instruccion = nueva_instruccion(COPY_STRING, parametros, 1);
		break;
		case "WAIT":
			instruccion = nueva_instruccion(WAIT, parametros, 1);
		break;
		case "SIGNAL":
			instruccion = nueva_instruccion(SIGNAL, parametros, 1);
		break;
		case "EXIT":
			instruccion = nueva_instruccion(EXIT, parametros, 0);
		break;
	}
	return instruccion;
}

t_instruccion nueva_instruccion(t_identificador identificador, char** parametros, uint32_t cant_parametros)
{
	t_instruccion instruccion;
	instruccion.identificador = identificador;
	instruccion.parametros = parametros;
	instruccion.cant_parametros = cant_parametros;
	return instruccion;
}