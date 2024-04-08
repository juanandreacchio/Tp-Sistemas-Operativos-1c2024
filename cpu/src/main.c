#include <../include/cpu.h>

// inicializacion

t_log *logger_cpu;
t_config* config_cpu;
int conexion_memoria;

int main(void) {
	logger_cpu = iniciar_logger("config/cpu.log","CPU",LOG_LEVEL_INFO);
    iniciar_config();
	char* mensaje = "handshake cpu with memoria";
	send(conexion_memoria,mensaje,strlen(mensaje)+1,0);
	int socket_servidor_cpu = iniciar_servidor(logger_cpu, puerto_servidor, "Memoria");
}



void iniciar_config()
{
	config_cpu = config_create("./config/cpu.config");
	ip_cpu = config_get_string_value(config_cpu, "IP_CPU");
	ip_memoria = config_get_string_value(config_cpu, "IP_MEMORIA");
	puerto_servidor = config_get_string_value(config_cpu, "PUERTO_SERVIDOR");
	puerto_memoria = config_get_string_value(config_cpu, "PUERTO_MEMORIA");
	conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_cpu);
}
