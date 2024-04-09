#include <../include/memoria.h>

t_log *logger_memoria;
t_config *config_memoria;

char *puerto_memoria;
char* mensaje_recibido;

int main(int argc, char* argv[]) {
    //logger memoria
    logger_memoria = iniciar_logger("config/memoria.log", "Memoria", LOG_LEVEL_INFO);

    //servidor de cpu
    iniciar_config();
    int socket_servidor_memoria = iniciar_servidor(logger_memoria, puerto_memoria, "Cpu");
    int socket_cliente = esperar_cliente(socket_servidor_memoria, logger_memoria);

    
    t_list* lista;
        int cod_op = recibir_operacion(socket_cliente);

        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(socket_cliente, logger_memoria);
                break;
            case PAQUETE:
                lista = recibir_paquete(socket_cliente);
                log_info(logger_memoria, "Se recibio un paquete");
                list_iterate(lista, (void*) iterator);
                break;
            case -1:
                log_error(logger_memoria, "No se pudo recibir el codigo de operacion");
                break;
            default:
                log_warning(logger_memoria, "Operacion desconocida");
                break;
        }
    
    //TODO
    //servidor de kernel
    //servidor de interfaz de I/O
    
    
    


    terminar_programa(socket_servidor_memoria, logger_memoria, config_memoria);

    return 0;
}

void iniciar_config(){
    config_memoria = config_create("config/memoria.config");
    puerto_memoria = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");   
}

void iterator(char* value) {
	log_info(logger_memoria,"%s", value);
}



