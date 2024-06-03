#include <../include/entradasalida.h>

// inicializacion
t_log *logger_entradasalida;
t_config *config_entradasalida;

char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;
char *tiempo_unidad_trabajo;
pthread_t thread_memoria, thread_kernel;
cod_interfaz tipo_interfaz;
uint32_t socket_conexion_kernel, socket_conexion_memoria;
t_interfaz *interfaz_creada;


//                                  <NOMBRE>          <RUTA>
int main(int argc, char *argv[]) // se corre haciendo --> make start generica1 config/entradasalida.config
{
    if (argc != 3)
    { // chequeo que hayan pasado argumentos
        printf("falta ingresar algun parametro\n");
        exit(1);
    }

    char *nombre = argv[1];
    char *ruta = argv[2];

    iniciar_config(ruta);

    interfaz_creada = iniciar_interfaz(nombre, ruta);

    pthread_create(&thread_kernel, NULL, iniciar_conexion_kernel, interfaz_creada);
    pthread_join(thread_kernel, NULL);

    if ((int)(long int)tipo_interfaz != GENERICA)
    {
        pthread_create(&thread_memoria, NULL, iniciar_conexion_memoria, NULL);
        pthread_join(thread_memoria, NULL);
    }

    log_info(logger_entradasalida, "I/O %s terminada", interfaz_creada->nombre);
    config_destroy(config_entradasalida);
    log_destroy(logger_entradasalida);

    return 0;
}

void iniciar_config(char *ruta)
{
    if ((config_entradasalida = config_create(ruta)) == NULL)
    { // chequeo que la ruta ingresada exista
        printf("la ruta ingresada no existe\n");
        exit(2);
    };

    logger_entradasalida = iniciar_logger("config/entradasalida.log", "ENTRADA_SALIDA", LOG_LEVEL_INFO);
    ip_kernel = config_get_string_value(config_entradasalida, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_entradasalida, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config_entradasalida, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_entradasalida, "PUERTO_MEMORIA");

    // pedido para la interfaz generica
    tiempo_unidad_trabajo = config_get_string_value(config_entradasalida, "TIEMPO_UNIDAD_TRABAJO");

    char *tipo_interfaz_str = config_get_string_value(config_entradasalida, "TIPO_INTERFAZ");
    if (strcmp(tipo_interfaz_str, "GENERICA") == 0)
    {
        tipo_interfaz = GENERICA;
    }
    else if (strcmp(tipo_interfaz_str, "STDIN") == 0)
    {
        tipo_interfaz = STDIN;
    }
    else if (strcmp(tipo_interfaz_str, "STDOUT") == 0)
    {
        tipo_interfaz = STDOUT;
    }
    else if (strcmp(tipo_interfaz_str, "DialFS") == 0)
    {
        tipo_interfaz = DIALFS;
    }
    else
    {
        printf("Tipo de interfaz desconocido: %s\n", tipo_interfaz_str);
        exit(3);
    }
    free(tipo_interfaz_str);
}

t_interfaz *iniciar_interfaz(char *nombre, char *ruta)
{
    t_interfaz *nueva_interfaz = malloc(sizeof(t_interfaz));
    nueva_interfaz->nombre = nombre;
    nueva_interfaz->ruta_archivo = ruta;
    return nueva_interfaz;
}

void *iniciar_conexion_kernel(void *arg)
{
    socket_conexion_kernel = crear_conexion(ip_kernel, puerto_kernel, logger_entradasalida);
    enviar_mensaje(interfaz_creada->nombre, socket_conexion_kernel, tipo_interfaz_to_cod_op(tipo_interfaz), logger_entradasalida);
    while (1)
    {
        atender_cliente(socket_conexion_kernel);
    }
    return NULL;
}

void *iniciar_conexion_memoria(void *arg)
{
    socket_conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_entradasalida);
    enviar_mensaje(interfaz_creada->nombre, socket_conexion_kernel, ENTRADA_SALIDA, logger_entradasalida);
    return NULL;
}

void *atender_cliente(int socket_cliente)
{
    t_paquete *paquete = recibir_paquete(socket_cliente);
    t_instruccion *instruccion = instruccion_deserializar(paquete->buffer, 0); 
    imprimir_instruccion(*instruccion);

    if (instruccion == NULL)
    {
        log_info(logger_entradasalida, "Se recibio una instruccion vacia");
        eliminar_paquete(paquete);
        return NULL;
    }

    switch (tipo_interfaz)
    {
    case GENERICA:
        log_info(logger_entradasalida,"arranco a dormir x tiempo");
        usleep(atoi(instruccion->parametros[1]) * atoi(tiempo_unidad_trabajo) * 1000); // *1000 para pasarlo a microsegundos
        log_info(logger_entradasalida,"termine de dormirme x tiempo");
        enviar_codigo_operacion(IO_SUCCESS, socket_cliente);
        break;
    case STDIN:
        t_paquete *paquete_escritura = crear_paquete(ESCRITURA_MEMORIA);
        t_solicitudEscribirEnMemoria *ptr_solicitud = malloc(sizeof(t_solicitudEscribirEnMemoria));
        ptr_solicitud->direccion = atoi(instruccion->parametros[0]); 
        ptr_solicitud->tamanio = atoi(instruccion->parametros[1]);

        // Leer desde el STDIN
        ptr_solicitud->dato = leer_desde_teclado(ptr_solicitud->tamanio);
        if (!ptr_solicitud->dato) {
            free(ptr_solicitud);
            break;
        }

        // falta agregar al buffer dato a escribir
        t_buffer* buffer1 = serializar_solicitud_escribir_memoria(ptr_solicitud);
        
        paquete_escritura->buffer = buffer1;

        // Enviar paquete a memoria
        enviar_paquete(paquete_escritura, socket_conexion_memoria);
        
        eliminar_paquete(paquete_escritura);
        liberar_buffer(buffer1); 
        free(ptr_solicitud);

        // recibir confirmacion de memoria

        t_paquete *paquete_respuesta = recibir_paquete(socket_conexion_memoria);

        enviar_codigo_operacion(IO_SUCCESS, socket_cliente);

        eliminar_paquete(paquete_respuesta);
        break;
    case STDOUT:
    
        t_paquete *paquete_lectura = crear_paquete(LECTURA_MEMORIA);
        t_solicitudLeerEnMemoria *ptr_solicitud_lectura = malloc(sizeof(t_solicitudEscribirEnMemoria));
        
        ptr_solicitud_lectura->direccion = atoi(instruccion->parametros[0]); 
        ptr_solicitud_lectura->tamanio = atoi(instruccion->parametros[1]);

        t_buffer *buffer = serializar_solicitud_leer_memoria(ptr_solicitud_lectura);
        
        paquete_lectura->buffer = buffer;

        // Enviar paquete a memoria 
        enviar_paquete(paquete_lectura, socket_conexion_memoria);   

        eliminar_paquete(paquete_lectura);
        liberar_buffer(buffer); 
        free(ptr_solicitud_lectura);

        // recibir dato de memoria

        t_paquete *paquete_dato = recibir_paquete(socket_conexion_memoria);

        // envio dato respuesta a kernel

        t_paquete *paquete_dato_respuesta = crear_paquete(IO_SUCCESS);
        paquete_dato_respuesta->buffer = paquete_dato->buffer;

        enviar_paquete(paquete_lectura, socket_cliente);   
        
        eliminar_paquete(paquete_lectura);
        eliminar_paquete(paquete_dato);
        break;
    case DIALFS:
        switch (instruccion->identificador)
        {
        case IO_FS_CREATE:
            log_info(logger_entradasalida, "Se recibio una instruccion IO_FS_CREATE");
            break;
        case IO_FS_DELETE:
            log_info(logger_entradasalida, "Se recibio una instruccion IO_FS_DELETE");
            break;
        case IO_FS_TRUNCATE:
            log_info(logger_entradasalida, "Se recibio una instruccion IO_FS_TRUNCATE");
            break;
        case IO_FS_WRITE:
            log_info(logger_entradasalida, "Se recibio una instruccion IO_FS_WRITE");
            break;
        case IO_FS_READ:
            log_info(logger_entradasalida, "Se recibio una instruccion IO_FS_READ");
            break;
        default:
            log_info(logger_entradasalida, "No se puede procesar la instruccion");
            break;
        }
        break;
    }

    return NULL;
}

void* leer_desde_teclado(uint32_t tamanio) {
    void *dato = malloc(tamanio);
    if (!dato) {
        log_error(logger_entradasalida, "Error al asignar memoria para el dato");
        return NULL;
    }

    log_info(logger_entradasalida, "Ingrese el dato a escribir en la memoria: ");
    if (fread(dato, 1, tamanio, stdin) != tamanio) {
        log_error(logger_entradasalida, "Error al leer el dato desde el STDIN");
        free(dato);
        return NULL;
    }

    return dato;
}
