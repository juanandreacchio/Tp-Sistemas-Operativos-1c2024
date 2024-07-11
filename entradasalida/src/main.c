#include <../include/entradasalida.h>

// inicializacion
t_log *logger_entradasalida;
t_config *config_entradasalida;

char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;
int tiempo_unidad_trabajo;
int block_size;
int block_count;
int retraso_compactacion;
char *path_fs;
pthread_t thread_memoria, thread_kernel;
cod_interfaz tipo_interfaz;
uint32_t socket_conexion_kernel, socket_conexion_memoria;
t_interfaz *interfaz_creada;
t_bitarray* bitmap;

//                                                                   <NOMBRE>          <RUTA>
int main(int argc, char *argv[]) // se corre haciendo --> make start generica1 config/entradasalida.config
{
    if (argc != 3)
    {
        printf("falta ingresar algun parametro\n");
        exit(1);
    }

    char *nombre = argv[1];
    char *ruta = argv[2];

    iniciar_config(ruta);

    interfaz_creada = iniciar_interfaz(nombre, ruta);

    if ((int)(long int)tipo_interfaz != GENERICA)
    {
    socket_conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_entradasalida);
    enviar_mensaje(interfaz_creada->nombre, socket_conexion_memoria, ENTRADA_SALIDA, logger_entradasalida);
    }

    if((int)(long int)tipo_interfaz == DIALFS){
        levantarFileSystem();
    }

    pthread_create(&thread_kernel, NULL, iniciar_conexion_kernel, interfaz_creada);
    pthread_join(thread_kernel, NULL);

    log_info(logger_entradasalida,"tipo de interfaz es: %d",tipo_interfaz);
    

    log_info(logger_entradasalida, "I/O %s terminada", interfaz_creada->nombre);
    config_destroy(config_entradasalida);
    log_destroy(logger_entradasalida);

    return 0;
}

void iniciar_config(char *ruta)
{
    if ((config_entradasalida = config_create(ruta)) == NULL)
    {
        printf("la ruta ingresada no existe\n");
        exit(2);
    };

    logger_entradasalida = iniciar_logger("config/entradasalida.log", "ENTRADA_SALIDA", LOG_LEVEL_INFO);
    ip_kernel = config_get_string_value(config_entradasalida, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_entradasalida, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config_entradasalida, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_entradasalida, "PUERTO_MEMORIA");

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
        block_size = atoi(config_get_string_value(config_entradasalida, "BLOCK_SIZE"));
        block_count = atoi(config_get_string_value(config_entradasalida, "BLOCK_COUNT"));
        path_fs = config_get_string_value(config_entradasalida, "PATH_BASE_DIALFS");
        retraso_compactacion = atoi(config_get_string_value(config_entradasalida, "RETRASO_COMPACTACION"));
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

void *atender_cliente(int socket_cliente)
{
    t_paquete *paquete = recibir_paquete(socket_cliente);
    char* PID; //obtener
    
    switch (tipo_interfaz)
    {
    case GENERICA:{
        log_info(logger_entradasalida, "PID: %d - Operacion: IO_GEN_SLEEP", PID); // LOG OBLIGATORIO: falta obtener PID y operacion
        u_int32_t unidad_de_trabajo;
        buffer_read(paquete->buffer,&unidad_de_trabajo,sizeof(u_int32_t));

        log_info(logger_entradasalida,"arranco a dormir x tiempo");
        usleep(unidad_de_trabajo * tiempo_unidad_trabajo * 1000); // *1000 para pasarlo a microsegundos
        log_info(logger_entradasalida,"termine de dormirme x tiempo");
        enviar_codigo_operacion(IO_SUCCESS, socket_cliente);
        break;
        }
    case STDIN:{
        log_info(logger_entradasalida, "PID: %d - Operacion: IO_STDIN_READ", PID); // LOG OBLIGATORIO: falta obtener PID
        u_int32_t cantidad_marcos,total_tamanio;
        buffer_read(paquete->buffer, &cantidad_marcos, sizeof(uint32_t));
        buffer_read(paquete->buffer, &total_tamanio, sizeof(uint32_t));
        t_list *direcciones_fisicas = list_create();
        for (int i = 0; i < cantidad_marcos; i++)
        {
            t_direc_fisica *direc_fisica = malloc(sizeof(t_direc_fisica));
            buffer_read(paquete->buffer, &direc_fisica->direccion_fisica, sizeof(uint32_t));
            buffer_read(paquete->buffer, &direc_fisica->desplazamiento_necesario, sizeof(uint32_t));
            list_add(direcciones_fisicas,direc_fisica);
        }
       
        // Leer desde el STDIN
        char *dato = leer_desde_teclado(total_tamanio);
        if (dato == NULL) {
            log_info(logger_entradasalida,"error: el dato dio null");
        }
        if (strlen(dato) > total_tamanio) {
            log_info(logger_entradasalida,"error: el dato dio null");
        }

        t_paquete *paquete_escritura = crear_paquete(ESCRITURA_MEMORIA);
        enviar_soli_escritura(paquete_escritura, direcciones_fisicas,total_tamanio,dato,socket_conexion_memoria);

        // recibir confirmacion de memoria

        op_code operacion = recibir_operacion(socket_conexion_memoria);
        if(operacion==OK)
        {
            enviar_codigo_operacion(IO_SUCCESS, socket_cliente);
        }
        else
        {
            log_error(logger_entradasalida,"error: no OK la escritura");
        }     
        break;
        }
    case STDOUT:{
        log_info(logger_entradasalida, "PID: %d - Operacion: IO_STDOUT_WRITE", PID); // LOG OBLIGATORIO: falta obtener PID 
        u_int32_t cantidad_marcos,total_tamanio;
        buffer_read(paquete->buffer, &cantidad_marcos, sizeof(uint32_t));
        buffer_read(paquete->buffer, &total_tamanio, sizeof(uint32_t));
        t_list *direcciones_fisicas = list_create();
        for (int i = 0; i < cantidad_marcos; i++)
        {
            t_direc_fisica *direc_fisica = malloc(sizeof(t_direc_fisica));
            buffer_read(paquete->buffer, &direc_fisica->direccion_fisica, sizeof(uint32_t));
            buffer_read(paquete->buffer, &direc_fisica->desplazamiento_necesario, sizeof(uint32_t));
            list_add(direcciones_fisicas,direc_fisica);
        }
        t_paquete *paquete_lectura = crear_paquete(LECTURA_MEMORIA);
        enviar_soli_lectura(paquete_lectura,direcciones_fisicas,total_tamanio,socket_conexion_memoria);
        

        // recibir dato de memoria

        t_paquete *paquete_dato = recibir_paquete(socket_conexion_memoria);
        char *str = malloc(total_tamanio);
        buffer_read(paquete_dato->buffer,str, total_tamanio);
        eliminar_paquete(paquete_dato);

        log_info(logger_entradasalida,"leido: %s",str);
        enviar_codigo_operacion(IO_SUCCESS, socket_cliente); 
        break;
        }
    case DIALFS:

        t_identificador identificador;
        buffer_read(paquete->buffer,&identificador, sizeof(t_identificador));
        switch (identificador)
        {
        case IO_FS_CREATE:
            log_info(logger_entradasalida, "PID: %d - Operacion: IO_FS_CREATE", PID);// LOG OBLIGATORIO: falta obtener PID
            char* nombre_archivo [256];
            buffer_read(paquete->buffer, nombre_archivo, sizeof(nombre_archivo));

            log_info(logger_entradasalida, "PID: %d - Crear Archivo: %s",PID, nombre_archivo); //LOG OBLIGATORIO

            int resultado_operacion = crear_archivo(nombre_archivo);
            if(resultado_operacion == -1){
                // devolver error
            } else {     
                enviar_codigo_operacion(IO_SUCCESS, socket_cliente);
            }         
            break;
        case IO_FS_DELETE:
            log_info(logger_entradasalida, "PID: %d - Operacion: IO_FS_DELETE", PID);// LOG OBLIGATORIO: falta obtener PID
            char* nombre_archivo [256];
            buffer_read(paquete->buffer, nombre_archivo, sizeof(nombre_archivo));
            log_info(logger_entradasalida, "PID: %d - Eliminar Archivo: %s",PID, nombre_archivo); //LOG OBLIGATORIO
            
            borrar_archivo(nombre_archivo);

            enviar_codigo_operacion(IO_SUCCESS, socket_cliente);
            break;
        case IO_FS_TRUNCATE:
            log_info(logger_entradasalida, "Se recibio una instruccion IO_FS_TRUNCATE");
            char* nombre_archivo [256];
            u_int32_t tamanio;
            buffer_read(paquete->buffer, nombre_archivo, sizeof(nombre_archivo));
            buffer_read(paquete->buffer, tamanio, sizeof(uint32_t));
            log_info(logger_entradasalida, "PID: %d - Truncar Archivo: %s - Tamaño: %d",PID, nombre_archivo, tamanio); //LOG OBLIGATORIO
            int resultado_operacion = truncar_archivo(nombre_archivo, tamanio);
            if(resultado_operacion == -1){
                // devolver error
            } else {
                log_info(logger_entradasalida, "archivo truncado correctamente");
                enviar_codigo_operacion(IO_SUCCESS, socket_cliente);
            }  
            break;
        case IO_FS_WRITE:
            log_info(logger_entradasalida, "PID: %d - Operacion: IO_FS_WRITE", PID);// LOG OBLIGATORIO: falta obtener PID
            char* nombre_archivo [256];
            uint32_t cantidad_marcos, tamanio, puntero;
            buffer_read(paquete->buffer, nombre_archivo, sizeof(nombre_archivo));
            buffer_read(paquete->buffer, &cantidad_marcos, sizeof(uint32_t));
            buffer_read(paquete->buffer, &tamanio, sizeof(uint32_t));

            t_list *direcciones_fisicas = list_create();
            for (int i = 0; i < cantidad_marcos; i++) {
                t_direc_fisica *direc_fisica = malloc(sizeof(t_direc_fisica));
                buffer_read(paquete->buffer, &direc_fisica->direccion_fisica, sizeof(uint32_t));
                buffer_read(paquete->buffer, &direc_fisica->desplazamiento_necesario, sizeof(uint32_t));
                list_add(direcciones_fisicas,direc_fisica);
            }

            buffer_read(paquete->buffer, &puntero, sizeof(uint32_t));

            log_info(logger_entradasalida, "PID: %d - Escribir Archivo: %s - Tamaño a Escribir: %d - Puntero Archivo: %d",PID, nombre_archivo, tamanio, puntero); //LOG OBLIGATORIO
            
            t_paquete *paquete_lectura = crear_paquete(LECTURA_MEMORIA);
            enviar_soli_lectura(paquete_lectura, direcciones_fisicas, tamanio, socket_conexion_memoria);

            // recibir dato de memoria
            t_paquete *paquete_dato = recibir_paquete(socket_conexion_memoria);
            char *lectura = malloc(tamanio);
            buffer_read(paquete_dato->buffer, lectura, tamanio);
            eliminar_paquete(paquete_dato);
            
            // escribir archivo
            escribir_archivo(nombre_archivo, lectura, tamanio, puntero);
            log_info(logger_entradasalida, "se realizo escritura correctamente");   
            free(lectura);
            break;
        case IO_FS_READ:
            log_info(logger_entradasalida, "PID: %d - Operacion: IO_FS_READ", PID);// LOG OBLIGATORIO: falta obtener PID
            char* nombre_archivo [256];
            buffer_read(paquete->buffer, nombre_archivo, sizeof(nombre_archivo));

            u_int32_t cantidad_marcos,tamanio;
            buffer_read(paquete->buffer, &cantidad_marcos, sizeof(uint32_t));
            buffer_read(paquete->buffer, &tamanio, sizeof(uint32_t));
            t_list *direcciones_fisicas = list_create();
            for (int i = 0; i < cantidad_marcos; i++)
            {
                t_direc_fisica *direc_fisica = malloc(sizeof(t_direc_fisica));
                buffer_read(paquete->buffer, &direc_fisica->direccion_fisica, sizeof(uint32_t));
                buffer_read(paquete->buffer, &direc_fisica->desplazamiento_necesario, sizeof(uint32_t));
                list_add(direcciones_fisicas,direc_fisica);
            }

            buffer_read(paquete->buffer, &puntero, sizeof(uint32_t));

            log_info(logger_entradasalida, "PID: %d - Leer Archivo: %s - Tamaño a Leer: %d - Puntero Archivo: %d",PID, nombre_archivo, tamanio, puntero); //LOG OBLIGATORIO
            
            
            // leer archivo
            void* lectura = leer_archivo(nombre_archivo, tamanio, puntero, lectura);
            
            t_paquete *paquete_escritura = crear_paquete(ESCRITURA_MEMORIA);
            enviar_soli_escritura(paquete_escritura, direcciones_fisicas, tamanio, lectura, socket_conexion_memoria);
            op_code operacion = recibir_operacion(socket_conexion_memoria);
            if(operacion==OK) {
                enviar_codigo_operacion(IO_SUCCESS, socket_cliente);
            }
            else {
                log_error(logger_entradasalida,"error: no OK la escritura");
            }
            free(lectura);
            break;
        default:
            log_info(logger_entradasalida, "No se puede procesar la instruccion");
            break;
        }
        break;
    }
    usleep(tiempo_unidad_trabajo * 1000); // consumir unidad de trabajo
    eliminar_paquete(paquete);

    return NULL;
}

void* leer_desde_teclado(uint32_t tamanio) {
    void *dato = malloc(tamanio);
    if (!dato) {
        log_error(logger_entradasalida, "Error al asignar memoria para el dato");
        return NULL;
    }

    log_info(logger_entradasalida, "Ingrese el dato a escribir en la memoria: ");
    if (fgets(dato, tamanio, stdin) == NULL) {
        log_error(logger_entradasalida, "Error al leer el dato desde el STDIN");
        free(dato);
        return NULL;
    }

    // Elimina el carácter de nueva línea si está presente.
    char *newline = strchr(dato, '\n');
    if (newline) {
        *newline = '\0';
    }

    return dato;
}
