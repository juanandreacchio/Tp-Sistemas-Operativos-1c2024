#include <../include/cpu.h>

// inicializacion

t_log *logger_cpu;
t_config *config_cpu;
char *ip_memoria;
char *puerto_memoria;
char *puerto_dispatch;
char *puerto_interrupt;
u_int32_t conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt;
int socket_servidor_dispatch, socket_servidor_interrupt;
pthread_t hilo_dispatch, hilo_interrupt;

int main(void)
{
	iniciar_config();

	conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_cpu);
	enviar_mensaje("", conexion_memoria, CPU, logger_cpu);

	// iniciar servidor Dispatch y Interrupt
	pthread_create(&hilo_dispatch, NULL, iniciar_servidor_dispatch, NULL);
	pthread_create(&hilo_interrupt, NULL, iniciar_servidor_interrupt, NULL);

	pthread_join(hilo_dispatch, NULL);
	pthread_join(hilo_interrupt, NULL);
}

void iniciar_config()
{
	config_cpu = config_create("./config/cpu.config");
	logger_cpu = iniciar_logger("config/cpu.log", "CPU", LOG_LEVEL_INFO);
	ip_memoria = config_get_string_value(config_cpu, "IP_MEMORIA");
	puerto_dispatch = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_DISPATCH");
	puerto_memoria = config_get_string_value(config_cpu, "PUERTO_MEMORIA");
	puerto_interrupt = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_INTERRUPT");
}

void *iniciar_servidor_dispatch(void *arg)
{
	socket_servidor_dispatch = iniciar_servidor(logger_cpu, puerto_dispatch, "DISPATCH");
	while (1)
	{
		conexion_kernel_dispatch = esperar_cliente(socket_servidor_dispatch, logger_cpu);
		log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Dispatch", cod_op_to_string(recibir_operacion(conexion_kernel_dispatch)));
		recibir_mensaje(conexion_kernel_dispatch, logger_cpu);
		
		t_pcb *pcb = recibir_pcb(conexion_kernel_dispatch);
		log_info(logger_cpu,"recibi el pcb");
		log_info(logger_cpu,"del pid tengo:%u",pcb->pid);
		log_info(logger_cpu,"del quantum tengo:%u",pcb->quantum);
		log_info(logger_cpu,"del psw tengo:%u",pcb->estado_actual);
		log_info(logger_cpu,"del pc tengo:%u",pcb->pc);
		

	}
	return NULL;
}

void *iniciar_servidor_interrupt(void *arg)
{
	socket_servidor_interrupt = iniciar_servidor(logger_cpu, puerto_interrupt, "INTERRUPT");
	while (1)
	{
		conexion_kernel_interrupt = esperar_cliente(socket_servidor_interrupt, logger_cpu);
		log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Interrupt", cod_op_to_string(recibir_operacion(conexion_kernel_interrupt)));
		recibir_mensaje(conexion_kernel_interrupt, logger_cpu);
	}
	return NULL;
}
/*
t_instruccion *fetch_instruccion(uint32_t pid, uint32_t pc)
{
	// Buscar instruccion en memoria
	t_paquete *paquete = crear_paquete(SOLICITUD_INSTRUCCION);
	// Le voy a mandar a memoria el paquete con el pid y el pc,
	// para q busque en la lista de procesos el proceso, y me devuelva la instruccion según el PC
	agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
	agregar_a_paquete(paquete, &pc, sizeof(uint32_t));

	enviar_paquete(paquete, conexion_memoria);

	t_paquete *respuesta_memoria = recibir_paquete(conexion_memoria);
	
	if(respuesta_memoria->codigo_operacion != INSTRUCCION){
		log_error(logger_cpu, "Error al recibir instruccion de memoria");
		eliminar_paquete(paquete);
		eliminar_paquete(respuesta_memoria);
		return NULL;
	}
	t_instruccion *instruccion = instruccion_deserializar(respuesta_memoria->buffer);

	pc+=1; // Nosé si está bien así

	eliminar_paquete(paquete);
	eliminar_paquete(respuesta_memoria);
	return instruccion;

}
*/