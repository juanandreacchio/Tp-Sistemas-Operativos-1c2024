#include <../include/cpu.h>

// inicializacion

t_log *logger_cpu;
t_config* config_cpu;
char* ip_memoria;
char *puerto_memoria;
char *puerto_dispatch;
char *puerto_interrupt;
uint32_t conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt;
int socket_servidor_dispatch, socket_servidor_interrupt;

int main(void) {
    iniciar_config();

	conexion_memoria = crear_conexion(ip_memoria,puerto_memoria);
	send(conexion_memoria, "CPU", strlen("CPU") + 1, 0);


	socket_servidor_dispatch = iniciar_servidor(logger_cpu, puerto_dispatch, "Dispatch");
	socket_servidor_interrupt = iniciar_servidor(logger_cpu, puerto_interrupt, "Interrupt");

	conexion_kernel_dispatch = esperar_cliente(socket_servidor_dispatch, logger_cpu);
	conexion_kernel_interrupt = esperar_cliente(socket_servidor_interrupt, logger_cpu);

}



void iniciar_config()
{
	config_cpu = config_create("./config/cpu.config");
	logger_cpu = iniciar_logger("config/cpu.log","CPU",LOG_LEVEL_INFO);
	ip_memoria = config_get_string_value(config_cpu, "IP_MEMORIA");
	puerto_dispatch = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_DISPATCH");
	puerto_memoria = config_get_string_value(config_cpu, "PUERTO_MEMORIA");
	puerto_interrupt = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_INTERRUPT");
}
