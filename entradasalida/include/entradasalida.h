#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <fcntl.h> // Para open() y los flags O_CREAT, O_RDWR, etc.
#include <unistd.h> // Para close(), ftruncate(), lseek(), etc.
#include <../include/utils.h>
#include <sys/mman.h> // para usar mmap, munmap y las constantes PROT_READ, PROT_WRITE, MAP_SHARED, y MAP_FAILED.
#ifndef ENTRADASALIDA_H_
#define ENTRADASALIDA_H_

typedef struct 
{
    char* nombre; //id de la interfaz
    char* ruta_archivo; // archivo de configuracion
}t_interfaz;


extern t_log *logger_entradasalida;
extern uint32_t socket_conexion_kernel, socket_conexion_memoria;

extern t_config *config_entradasalida;
extern char *ip_kernel;
extern char *puerto_kernel;
extern char *ip_memoria;
extern char *puerto_memoria;
extern pthread_t thread_memoria, thread_kernel;
extern cod_interfaz tipo_interfaz;
extern t_interfaz *interfaz_creada;
extern char* path_fs;
extern char* block_count;
extern char* block_size;
extern char* retraso_compactacion;
extern t_bitarray* bitmap;

// ------------------------ FUNCIONES DE INICIO --------------------
void iniciar_config();
void *iniciar_conexion_kernel(void *arg);
void *iniciar_conexion_memoria(void *arg);
t_interfaz *iniciar_interfaz(char* nombre,char* ruta);
void* leer_desde_teclado(uint32_t tamanio);

// ------------------------ FUNCIONES DE EJECUCION --------------------
void *atender_cliente(int socket_cliente);

// -------------------- FUNCIONES DE FILE SYSTEM -----------------------
void create_archivos_bloques();
void crear_bitmap();

#endif