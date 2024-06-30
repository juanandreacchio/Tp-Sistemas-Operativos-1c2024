#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
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
void crear_archivo_metadata(const char* base_path, const char* filename, int initial_block);
void crear_archivo(const char* base_path, const char* filename);
void levantarFileSystem();
void asignar_bloque(uint32_t bloque_libre);
void liberar_bloque(uint32_t bloque);
uint32_t buscar_bloque_libre();
char* buscar_archivo(const char* archivo_buscar);
uint32_t calcular_bloques_adicionales(uint32_t tamanio_actual,uint32_t tamanio_nuevo);
uint32_t calcular_bloques_a_liberar(uint32_t tamanio_actual, uint32_t tamanio_nuevo);
uint32_t obtener_tamanio_archivo(const char* metadata_path);
uint32_t obtener_bloque_inicial(const char* metadata_path);
uint32_t obtener_ultimo_bloque(uint32_t bloque_inicial, uint32_t tamanio_actual);
uint32_t buscar_bloques_contiguos_libres(uint32_t bloques_adicionales);
int verificar_bloques_contiguos_libres(uint32_t bloque_inicial, uint32_t cantidad_bloques);
void actualizar_metadata_tamanio(const char* metadata_path, size_t tamanio_nuevo);

void acortar_archivo(const char* filename, uint32_t tamanio_nuevo);
void agrandar_archivo(const char* filename, uint32_t tamanio_nuevo);
void borrar_archivo(const char* filename);
int crear_archivo(const char* filename);
#endif