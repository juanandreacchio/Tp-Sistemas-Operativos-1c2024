#include <../include/mem.h>

// --------------------- FUNCIONES DE TABLA DE PAGINAS ---------------------
void inicializar_pagina(t_pagina* pagina, int numero_pagina) {
    pagina->contenido = NULL;
    pagina->pagina = numero_pagina;
}

//Inicializa la tabla de paginas con la cantidad de paginas que entran en la memoria y las inicializa
t_tabla_paginas* inicializar_tabla_paginas() {
    t_tabla_paginas* tabla_paginas = malloc(sizeof(t_tabla_paginas));
    if (tabla_paginas == NULL) {
        printf("Error: no se pudo asignar memoria para la tabla de páginas\n");
        exit(1);
    }

    tabla_paginas->tabla_paginas = malloc(NUM_PAGINAS * sizeof(t_pagina*));
    if (tabla_paginas->tabla_paginas == NULL) {
        printf("Error: no se pudo asignar memoria para la tabla de páginas\n");
        exit(1);
    }
    for (int i = 0; i < NUM_PAGINAS; i++) {
        tabla_paginas->tabla_paginas[i] = malloc(sizeof(t_pagina));
        if (tabla_paginas->tabla_paginas[i] == NULL) {
            printf("Error: no se pudo asignar memoria para la página %d\n", i);
            exit(1);
        }
        inicializar_pagina(tabla_paginas->tabla_paginas[i], i);
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



// Función que a partir de un archivo de instrucciones, devuelve la lista de instrucciones


t_proceso *crear_proceso(t_list* lista_procesos, int pid, char* path) {
    size_t path_final_size = strlen(PATH_INSTRUCCIONES) + strlen(path) + 1;
    char *path_final = malloc(path_final_size); 
    strcpy(path_final, PATH_INSTRUCCIONES);
    strcat(path_final, path);
    FILE* archivo_instrucciones = fopen(path_final, "r");
    if (archivo_instrucciones == NULL) {
        printf("Error: no se pudo abrir el archivo %s\n", path);
        exit(1);
    }
    t_proceso* proceso = malloc(sizeof(t_proceso));
    if (proceso == NULL) {
        printf("Error: no se pudo asignar memoria para el proceso con PID %d\n", pid);
        exit(1);
    }
    proceso->pid = pid;
    // proceso->path = malloc(strlen(PATH_INSTRUCCIONES) + strlen(path) + 1);
    // if (proceso->path == NULL) {
    //     printf("Error: no se pudo asignar memoria para el path del proceso con PID %d\n", pid);
    //     exit(1);
    // }
    proceso->lista_instrucciones = parsear_instrucciones(archivo_instrucciones);
    fclose(archivo_instrucciones);
    // t_instruccion* instruccion = list_get(proceso->lista_instrucciones, 0);
    // imprimir_instruccion(instruccion);
    // strcpy(proceso->path, PATH_INSTRUCCIONES);
    // strcat(proceso->path, path);
    proceso->tabla_paginas = inicializar_tabla_paginas();
    // wait semáforo contador (con grado multiprogramación)
    list_add(lista_procesos, proceso);
    // signal semáforo contador (con grado multiprogramación)
    return proceso;

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
    instruccion = string_to_instruccion(buffer);
    free(buffer);
    fclose(archivo);
    return instruccion;
}

// Funcion que busque una instruccion en un proceso a partir de un PID y un PC
t_instruccion *buscar_instruccion(t_list* lista_procesos, uint32_t pid, uint32_t pc) {
    printf("Buscando instrucción con PID %d y PC %d\n", pid, pc);
    t_proceso* proceso = malloc(sizeof(t_proceso));
    t_proceso* proceso_actual = malloc(sizeof(t_proceso));
    for (int i = 0; i < list_size(lista_procesos); i++) {
        proceso_actual = list_get(lista_procesos, i);
        if (proceso_actual->pid == pid) {
            proceso = proceso_actual;
            break;
        }
    }
    if (proceso == NULL) {
        printf("Error: no se encontró el proceso con PID %d\n", pid);
        exit(1);
    }
    return list_get(proceso->lista_instrucciones, pc);
}

void liberar_lista_procesos(t_list* lista_procesos) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso* proceso = list_get(lista_procesos, i);
        liberar_proceso(proceso);
    }
    list_destroy(lista_procesos);
}

t_instruccion *string_to_instruccion(char *string)
{
	char** tokens = string_split(string, " ");
	t_identificador identificador = string_to_identificador(tokens[0]);
	t_list *parametros = list_create();
    for (int i = 1; tokens[i] != NULL; i++) {
        list_add(parametros, tokens[i]);
    }

	return crear_instruccion(identificador, parametros);
}

