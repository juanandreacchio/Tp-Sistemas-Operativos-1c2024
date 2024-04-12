#include <../include/memoria.h>

t_log *logger_memoria;
t_config *config_memoria;

char *puerto_memoria;
char *mensaje_recibido;
int socket_servidor_memoria;

int main(int argc, char *argv[])
{
    // logger memoria

    // iniciar servidor
    iniciar_config();
    socket_servidor_memoria = iniciar_servidor(logger_memoria, puerto_memoria, "MEMORIA");

    while (1)
    {
        pthread_t thread;
        int socket_cliente = esperar_cliente(socket_servidor_memoria, logger_memoria);
        char* modulo_cliente;
        recibir_operacion(socket_cliente);
        modulo_cliente = recibir_mensaje_guardar_variable(socket_cliente);
        log_info(logger_memoria,modulo_cliente);
        //funciona la conexion y los mensajes tiene que correr:
        //memoria primero despues cpu y despues kernel y funciona
        //si lo corres en otro orden tira broken pipe porque intenta enviar mensajes a servidores que no estan levantados
        //podriamos manejar el error para que si pase se qude esperando pero no c como hacerlo todavia

        //lo que esta abajo esta comentado no c porque no funciona la verdad pero bueno ya lo vere otro dia 
        /*
        pthread_create(&thread,
                       NULL,
                       atender_cliente(modulo_cliente),
                       socket_cliente);
        pthread_detach(thread);
        */
    }
    
    terminar_programa(socket_servidor_memoria, logger_memoria, config_memoria);

    return 0;
}

void iniciar_config()
{
    config_memoria = config_create("config/memoria.config");
    logger_memoria = iniciar_logger("config/memoria.log", "Memoria", LOG_LEVEL_INFO);
    puerto_memoria = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");
}

void iterator(char *value)
{
    log_info(logger_memoria, "%s", value);
}

void atender_cliente(char *modulo){
    if (strcmp(modulo, "Kernel") == 0) {
        log_info(logger_memoria, "Atendiendo cliente Kernel");
    } else if (strcmp(modulo, "CPU") == 0) {
        log_info(logger_memoria, "Atendiendo cliente CPU");
    } else if (strcmp(modulo, "IO") == 0) {
        log_info(logger_memoria, "Atendiendo cliente IO");
    }
}