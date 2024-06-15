#include <../include/mem.h>
//--------------------------ENVIAR TAMAÑO "handshake"---------------------------
void enviar_tamanio_pagina(int socket)
{
    send( socket, &TAM_PAGINA, sizeof(int), 0);
}

// --------------------- FUNCIONES DE MEMORIA ---------------------

// Inicializa la memoria principal con el tamaño de la misma pasado por configuración
void inicializar_memoria_principal(){
    memoria_principal = malloc(TAM_MEMORIA);
    inicializar_bitarray(TAM_MEMORIA / TAM_PAGINA);
    if (memoria_principal == NULL) {
        printf("Error: no se pudo asignar memoria para la memoria principal\n");
        exit(1);
    }
}

// Funcion para liberar la memoria principal
void liberar_memoria_principal() {
    free(memoria_principal);
}

// Obtener direccion fisica de memoria
void* obtener_direccion_fisica(int numero_marco) {
    return memoria_principal + numero_marco * TAM_PAGINA;
}

// Funcion para obtener el primer marco libre
int obtener_primer_marco_libre() {
    for (int i = 0; i < TAM_MEMORIA / TAM_PAGINA; i++) {
        if (!bitarray_test_bit(marcos_ocupados, i)) {
            return i;
        }
    }
    return -1;
}
void inicializar_bitarray(size_t size) {
    int bytes = (size + 7) / 8;  // Convertir el tamaño a bytes
    void *bitarray_mem = malloc(bytes);
    marcos_ocupados = bitarray_create_with_mode(bitarray_mem, bytes, LSB_FIRST);
}

// Función para destruir el bitarray
void destruir_bitarray() {
    bitarray_destroy(marcos_ocupados);
    free(marcos_ocupados->bitarray);
}


// --------------------- FUNCIONES DE TABLA DE PAGINAS ---------------------

// Inicializa una pagina con el numero de marco y la bandera presente
t_pagina *inicializar_pagina(int numero_marco) {
    t_pagina* pagina = malloc(sizeof(t_pagina));
    if (pagina == NULL) {
        printf("Error: no se pudo asignar memoria para la página\n");
        exit(1);
    }
    pagina->numero_marco = numero_marco;
    pagina->presente = false;
    bitarray_set_bit(marcos_ocupados, numero_marco);
    return pagina;
}

// Funcion para liberar la tabla de paginas y las paginas
void liberar_tabla_paginas(t_list* tabla_paginas) {
    for (int i = 0; i < list_size(tabla_paginas); i++) {
        t_pagina* pagina = list_get(tabla_paginas, i);
        // liberar el marco de la pagina
        bitarray_clean_bit(marcos_ocupados, pagina->numero_marco);
        free(pagina);
    }
    list_destroy(tabla_paginas);
}

// Funcion para obtener el marco de una pagina
int obtener_marco_pagina(t_proceso* proceso, int numero_pagina) {
    t_pagina* pagina = list_get(proceso->tabla_paginas, numero_pagina);
    return pagina->numero_marco;
}

// Funcion para liberar marco de una pagina
void liberar_marco_pagina(t_proceso* proceso, int numero_pagina) {
    t_pagina* pagina = list_get(proceso->tabla_paginas, numero_pagina);
    bitarray_clean_bit(marcos_ocupados, pagina->numero_marco);
}

// --------------------- FUNCIONES DE PROCESO ---------------------

// Obtener la posición de un proceso en la lista de procesos a partir de un PID
int posicion_proceso(t_list* lista_procesos, uint32_t pid) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso* proceso = list_get(lista_procesos, i);
        if (proceso->pid == pid) {
            return i;
        }
    }
    return -1;
}

// Funcion para encontrar un proceso por PID en una lista de procesos
t_proceso *buscar_proceso_por_pid(t_list *lista_procesos, uint32_t pid)
{
    for (int i = 0; i < list_size(lista_procesos); i++)
    {
        t_proceso *proceso = list_get(lista_procesos, i);
        if (proceso->pid == pid)
        {
            return proceso;
        }
    }
    return NULL;
}

// Funcion para imprimir una lista de procesos
void imprimir_lista_de_procesos(t_list *lista_procesos)
{
	for (int i = 0; i < list_size(lista_procesos); i++)
	{
		t_proceso *proceso = list_get(lista_procesos, i);
		printf("\n------------------------ PROCESO %d ------------------------", i + 1);
		imprimir_proceso(proceso);
	}
}

// Funcion para imprimir un proceso
void imprimir_proceso(t_proceso *proceso)
{
	printf("PID: %d\n", proceso->pid);
	// printf("Path del archivo de instrucciones: %s\n", proceso->path);
	printf("Lista de instrucciones:\n");
	for (int i = 0; i < list_size(proceso->lista_instrucciones); i++)
	{
		t_instruccion *instruccion = list_get(proceso->lista_instrucciones, i);
		imprimir_instruccion(instruccion);
    }
    printf("Tabla de paginas:\n");
    for (int i = 0; i < list_size(proceso->tabla_paginas); i++) {
        t_pagina* pagina = list_get(proceso->tabla_paginas, i);
        printf("Pagina %d: marco %d, presente %d\n", i, pagina->numero_marco, pagina->presente);
    }
}

// Funcion para deserializar una solicitud de creacion de proceso
t_solicitudCreacionProcesoEnMemoria *deserializar_solicitud_crear_proceso(t_buffer *buffer)
{
	t_solicitudCreacionProcesoEnMemoria *solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));
	buffer->offset = 0;

	buffer_read(buffer, &solicitud->pid, sizeof(uint32_t));
	buffer_read(buffer, &solicitud->path_length, sizeof(uint32_t));
	solicitud->path = malloc(solicitud->path_length);
	buffer_read(buffer, solicitud->path, solicitud->path_length);
	return solicitud;
}


// Función que a partir de un archivo de instrucciones, devuelve la lista de instrucciones

t_proceso *crear_proceso(t_list* lista_procesos, int pid, char* path) {
    printf("ENTRE A CREAR PROCESO PAAAA");
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
    proceso->tabla_paginas = list_create();
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
    if(pc >= list_size(proceso->lista_instrucciones)){
        return NULL;
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

