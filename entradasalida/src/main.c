#include <../include/entradasalida.h>

// inicializacion
t_log *logger_entradasalida;
uint32_t conexion_kernel;
int socket_conexion_kernel, socket_conexion_memoria;

t_config *config_entradasalida;
char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;
cod_interfaz tipo_interfaz;
char *tiempo_unidad_trabajo;
pthread_t thread_memoria, thread_kernel;
t_interfaz *interfaz_creada;


                                                //                                  <NOMBRE>          <RUTA>
int main(int argc, char *argv[])                // se corre haciendo --> make start generica1 config/entradasalida.config
{
    if (argc != 3) { //chequeo que hayan pasado argumentos
        printf("falta ingresar algun parametro\n");
        exit(1);
    }
    
    char* nombre = argv[1];
    char* ruta = argv[2];

    iniciar_config(ruta);

    interfaz_creada = iniciar_interfaz(nombre, ruta);
            
    pthread_create(&thread_kernel, NULL, iniciar_conexion_kernel, interfaz_creada);
    pthread_join(thread_kernel, NULL);
 

    if ((int)(long int)tipo_interfaz != GENERICA) {
        pthread_create(&thread_memoria, NULL, iniciar_conexion_memoria, NULL);
        pthread_join(thread_memoria, NULL);
    }

    log_info(logger_entradasalida, "I/O %s terminada", interfaz_creada->nombre);
    terminar_programa(conexion_kernel, logger_entradasalida, config_entradasalida);
    
    return 0;
}

void iniciar_config(char* ruta)
{
    if((config_entradasalida = config_create(ruta)) == NULL) {              //chequeo que la ruta ingresada exista
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

    char* tipo_interfaz_str = config_get_string_value(config_entradasalida, "TIPO_INTERFAZ");
    if (strcmp(tipo_interfaz_str, "GENERICA") == 0) {
        tipo_interfaz = GENERICA;
    } else if (strcmp(tipo_interfaz_str, "STDIN") == 0) {
        tipo_interfaz = STDIN;
    } else if (strcmp(tipo_interfaz_str, "STDOUT") == 0) {
        tipo_interfaz = STDOUT;
    } else if (strcmp(tipo_interfaz_str, "DialFS") == 0) {
        tipo_interfaz = DIALFS;
    } else {
        printf("Tipo de interfaz desconocido: %s\n", tipo_interfaz_str);
        exit(3);
    }
}


t_interfaz* iniciar_interfaz(char* nombre,char* ruta)
{
    t_interfaz* nueva_interfaz = malloc(sizeof(t_interfaz));
    nueva_interfaz->nombre = nombre;
    nueva_interfaz->ruta_archivo = ruta;
    return nueva_interfaz;
}


void *iniciar_conexion_kernel(void *arg)
{
    socket_conexion_kernel = crear_conexion(ip_kernel, puerto_kernel, logger_entradasalida);
    enviar_mensaje(interfaz_creada->nombre , socket_conexion_kernel, ENTRADA_SALIDA, logger_entradasalida);
    while (1)
    {   
        atender_cliente(socket_conexion_kernel);
        enviar_mensaje("", socket_conexion_kernel, FIN_OPERACION_IO, logger_entradasalida); // mensaje avisando que termine
    }
    return NULL;
}

void *iniciar_conexion_memoria(void *arg)
{
    socket_conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_entradasalida);
    enviar_mensaje("", socket_conexion_memoria, ENTRADA_SALIDA, logger_entradasalida);
    while (1)
    {
        // TODO: implementar
    }
    return NULL;
}

void *atender_cliente(int socket_cliente)
{
    t_paquete *paquete = recibir_paquete(socket_cliente);  
    t_instruccion *instruccion = instruccion_deserializar(paquete->buffer, 0); // se deberia pasar el offset tmb

    if(instruccion == NULL){
        log_info(logger_entradasalida, "Se recibio una instruccion vacia");
        return NULL;
    }
    
    switch(tipo_interfaz) {
        case GENERICA:
                usleep(atoi(instruccion->parametros[1]) * atoi(tiempo_unidad_trabajo) * 1000);     // *1000 para pasarlo a microsegundos
                enviar_codigo_operacion(IO_SUCCESS,socket_cliente);
            break;
        case STDIN:
            
            break;
        case STDOUT:
           
            break;
        case DIALFS:
            switch(instruccion->identificador){
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

op_code tipo_interfaz_to_cod_op(cod_interfaz tipo){
    switch (tipo)
    {
    case GENERICA:
        return INTERFAZ_GENERICA;
    case STDIN:
        return INTERFAZ_STDIN;
    case STDOUT:
        return INTERFAZ_STDOUT;
    case DIALFS:
        return INTERFAZ_DIALFS;
    default:
        break;
    }
}

