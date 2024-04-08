#include <../include/memoria.h>

t_log *logger_memoria;
t_config *config_memoria;

char *puerto_memoria;

int main(int argc, char* argv[]) {
    logger_memoria = iniciar_logger("../config/memoria.log", "Memoria", LOG_LEVEL_INFO);
    config_memoria = iniciar_config("../config/memoria.config");

    int socket_servidor_memoria = iniciar_servidor(logger, puerto_memoria, "Memoria");
    int socket_cliente = esperar_cliente(logger_memoria, socket_servidor_memoria);
    
    t_list* lista;
    while (1) {
        int cod_op = recibir_operacion(socket_cliente);

        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(socket_cliente);
                break;
            case PAQUETE:
                lista = recibir_paquete(socket_cliente);
                log_info(logger_memoria, "Se recibio un paquete");
                list_iterate(lista, (void*) mostrar_paquete);
                break;
            case -1:
                log_error(logger_memoria, "No se pudo recibir el codigo de operacion");
                break;
            default:
                log_warning(logger_memoria, "Operacion desconocida");
                break;
        }
    }


    terminar_programa(socket_servidor_memoria, logger_memoria, config_memoria);

    return 0;
}


void mostrar_paquete(char* value) {
	log_info(logger,"%s", value);
}

void obtener_config(){
    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");   
}



