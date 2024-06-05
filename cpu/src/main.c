#include <../include/cpu.h>

// // inicializacion

t_log *logger_cpu;
t_config *config_cpu;
char *ip_memoria;
char *puerto_memoria;
char *puerto_dispatch;
char *puerto_interrupt;
u_int32_t conexion_memoria, conexion_kernel_dispatch, conexion_kernel_interrupt, tamanio_de_pagina;
int socket_servidor_dispatch, socket_servidor_interrupt;
pthread_t hilo_dispatch, hilo_interrupt;
t_registros registros_cpu;
t_interrupcion *interrupcion_recibida = NULL; // Inicializa a NULL

//------------------------variables globales----------------
u_int8_t interruption_flag;
u_int8_t end_process_flag;
u_int8_t input_ouput_flag;

// ------------------------- MAIN---------------------------

int main(void)
{
    iniciar_config();
    inicializar_flags();

    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_cpu);
    enviar_mensaje("", conexion_memoria, CPU, logger_cpu);
    tamanio_de_pagina = recibir_tamanio(conexion_memoria);

    // iniciar servidor Dispatch y Interrupt
    pthread_create(&hilo_dispatch, NULL, iniciar_servidor_dispatch, NULL);
    pthread_create(&hilo_interrupt, NULL, iniciar_servidor_interrupt, NULL);

    pthread_join(hilo_dispatch, NULL);
    pthread_join(hilo_interrupt, NULL);

    liberar_conexion(socket_servidor_interrupt);
    terminar_programa(socket_servidor_dispatch, logger_cpu, config_cpu);
}

//------------------------FUNCIONES DE INICIO------------------------

void iniciar_config()
{
    config_cpu = config_create("./config/cpu.config");
    logger_cpu = iniciar_logger("config/cpu.log", "CPU", LOG_LEVEL_INFO);
    ip_memoria = config_get_string_value(config_cpu, "IP_MEMORIA");
    puerto_dispatch = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_DISPATCH");
    puerto_memoria = config_get_string_value(config_cpu, "PUERTO_MEMORIA");
    puerto_interrupt = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_INTERRUPT");
}

void inicializar_flags()
{
    interruption_flag = 0;
    end_process_flag = 0;
    input_ouput_flag = 0;
}
//--------------------------RECIBIR TAMAÑO "handshake"---------------------------
u_int32_t recibir_tamanio(u_int32_t  socket_cliente)
{
    int tamanio_pagina;
    recv(socket_cliente, &tamanio_pagina, sizeof(int), MSG_WAITALL);
    u_int32_t tam_cast = (u_int32_t)tamanio_pagina;
    return tam_cast;
}
//-------------------------------ATENDER_DISPATCH-------------------------------
void *iniciar_servidor_dispatch(void *arg)
{
    socket_servidor_dispatch = iniciar_servidor(logger_cpu, puerto_dispatch, "DISPATCH");
    conexion_kernel_dispatch = esperar_cliente(socket_servidor_dispatch, logger_cpu);
    log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Dispatch", cod_op_to_string(recibir_operacion(conexion_kernel_dispatch)));
    recibir_mensaje(conexion_kernel_dispatch, logger_cpu);
    while (1)
    {
        t_pcb *pcb = recibir_pcb(conexion_kernel_dispatch);
        //printf("recibi el proceso:%d", pcb->pid);
        comenzar_proceso(pcb, conexion_memoria, conexion_kernel_dispatch);
    }
    return NULL;
}

//-------------------------------ATENDER_INTERRUPTION-------------------------------

void *iniciar_servidor_interrupt(void *arg)
{
    socket_servidor_interrupt = iniciar_servidor(logger_cpu, puerto_interrupt, "INTERRUPT");
    conexion_kernel_interrupt = esperar_cliente(socket_servidor_interrupt, logger_cpu);
    log_info(logger_cpu, "Se recibio un mensaje del modulo %s en el Interrupt", cod_op_to_string(recibir_operacion(conexion_kernel_interrupt)));
    recibir_mensaje(conexion_kernel_interrupt, logger_cpu);
    while (1)
    {
        t_paquete *respuesta_kernel = recibir_paquete(conexion_kernel_interrupt);

        if (respuesta_kernel->codigo_operacion == INTERRUPTION)
        {
            if (interrupcion_recibida != NULL)
            {
                free(interrupcion_recibida);
            }
            interrupcion_recibida = deserializar_interrupcion(respuesta_kernel->buffer);
            log_info(logger_cpu, "ME LLEGO UNA INTERRUPCION DEL KERNEL DEL PROCESO: %d",interrupcion_recibida->pid);
        }
        else
        {
            log_info(logger_cpu, "operacion desconocida dentro de interrupciones");
        }
    }
    return NULL;
}

void accion_interrupt(t_pcb *pcb, op_code motivo, int socket)
{
    if (interrupcion_recibida == NULL)
    {
        log_error(logger_cpu, "No hay interrupcion recibida");
        return;
    }

    interruption_flag = 0;
    log_info(logger_cpu, "estaba ejecutando y encontre una interrupcion nevio PCB y motivo de desalojo a Kernel");
    enviar_motivo_desalojo(motivo, socket);
    enviar_pcb(pcb, socket);
}

// ------------------------ CICLO BASICO DE INSTRUCCION ------------------------

t_instruccion *fetch_instruccion(uint32_t pid, uint32_t *pc, uint32_t conexionParam)
{
    t_paquete *paquete = crear_paquete(SOLICITUD_INSTRUCCION);
    t_buffer *buffer = crear_buffer();

    buffer_add(buffer, &pid, sizeof(uint32_t));
    buffer_add(buffer, pc, sizeof(uint32_t));
    paquete->buffer = buffer;

    enviar_paquete(paquete, conexionParam);

    log_info(logger_cpu, "PID: %d - FETCH - Program Counter pc: %d", pid, *pc);

    t_paquete *respuesta_memoria = recibir_paquete(conexionParam);

    if (respuesta_memoria == NULL)
    {
        log_error(logger_cpu, "Error al recibir instruccion de memoria");
        eliminar_paquete(paquete);
        return NULL;
    }

    if (respuesta_memoria->codigo_operacion != INSTRUCCION)
    {
        log_error(logger_cpu, "Error al recibir instruccion de memoria");
        eliminar_paquete(paquete);
        eliminar_paquete(respuesta_memoria);
        return NULL;
    }

    t_instruccion *instruccion = instruccion_deserializar(respuesta_memoria->buffer, 0);

    eliminar_paquete(paquete);
    eliminar_paquete(respuesta_memoria);
    return instruccion;
}

void decode_y_execute_instruccion(t_instruccion *instruccion, t_pcb *pcb)
{
    switch (instruccion->identificador)
    {
    case SET:
        set_registro(&pcb->registros, instruccion->parametros[0], atoi(instruccion->parametros[1]));
        break;
    case MOV_IN:
        mov_in(pcb,instruccion->parametros[0], instruccion->parametros[1]);
        break;
    case MOV_OUT:
        mov_out(pcb,instruccion->parametros[0], instruccion->parametros[1]);
        break;
    case SUM:
        sum_registro(&pcb->registros, instruccion->parametros[0], instruccion->parametros[1]);
        break;
    case SUB:
        sub_registro(&pcb->registros, instruccion->parametros[0], instruccion->parametros[1]);
        break;
    case JNZ:
        JNZ_registro(&pcb->registros, instruccion->parametros[0], atoi(instruccion->parametros[1]));
        break;
    case IO_FS_TRUNCATE:
        break;
    case IO_STDIN_READ:
        break;
    case IO_STDOUT_WRITE:
        break;
    case IO_GEN_SLEEP:
        enviar_motivo_desalojo(OPERACION_IO, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);

        t_paquete *paquete = crear_paquete(OPERACION_IO);
        paquete->buffer = serializar_instruccion(instruccion);
        enviar_paquete(paquete, conexion_kernel_dispatch);
        input_ouput_flag = 1;
        eliminar_paquete(paquete);

        break;
    case IO_FS_DELETE:
        break;
    case IO_FS_CREATE:
        break;
    case IO_FS_WRITE:
        break;
    case IO_FS_READ:
        break;
    case RESIZE:
        t_paquete *paquete_a_enviar = crear_paquete(AJUSTAR_TAMANIO_PROCESO);
        buffer_add(paquete_a_enviar->buffer,&pcb->pid,sizeof(u_int32_t));
        u_int32_t tamanio = (u_int32_t)atoi(instruccion->parametros[0]);
        buffer_add(paquete_a_enviar->buffer,&tamanio ,sizeof(u_int32_t));
        enviar_paquete(paquete_a_enviar,conexion_memoria);
        eliminar_paquete(paquete_a_enviar);
        t_paquete *paquete_recibido = recibir_paquete(conexion_memoria);
        if(paquete_recibido->codigo_operacion == OUT_OF_MEMORY){
        enviar_motivo_desalojo(OUT_OF_MEMORY, conexion_kernel_dispatch);
        enviar_pcb(pcb, conexion_kernel_dispatch);
        }else if(paquete_recibido->codigo_operacion != OK)
        {
            log_error(logger_cpu,"error:recibi un codigo de operacion desconocido dentro de RESIZE");
        }
        eliminar_paquete(paquete_recibido);
        break;
    case COPY_STRING:
        break;
    case WAIT:
        break;
    case SIGNAL:
        break;
    case EXIT:
        end_process_flag = 1;
        break;
    default:
        break;
    }
    log_info(logger_cpu, "PID: %d - EJECUTANDO ", pcb->pid);
    imprimir_instruccion(instruccion);
}

bool check_interrupt(uint32_t pid)
{
    if (interrupcion_recibida != NULL)
    {
        return interrupcion_recibida->pid == pid;
    }
    return false;
}

// ------------------------ FUNCIONES PARA EJECUTAR PCBS ------------------------

t_instruccion *siguiente_instruccion(t_pcb *pcb, int socket)
{
    t_instruccion *instruccion = fetch_instruccion(pcb->pid, &pcb->pc, socket);
    if (instruccion != NULL)
    {
        pcb->pc += 1;
    }
    return instruccion;
}

void comenzar_proceso(t_pcb *pcb, int socket_Memoria, int socket_Kernel)
{
    t_instruccion *instruccion = NULL;
    input_ouput_flag = 0;
    interrupcion_recibida = NULL;
    while (interruption_flag != 1 && end_process_flag != 1 && input_ouput_flag != 1)
    {
        if (instruccion != NULL)
            free(instruccion);

        instruccion = siguiente_instruccion(pcb, socket_Memoria);
        if (instruccion == NULL)
        {
            // Manejar error: siguiente_instruccion no debería devolver NULL
            perror("Error: siguiente_instruccion devolvió NULL");
            break;
        }
        decode_y_execute_instruccion(instruccion, pcb);
        if(check_interrupt(pcb->pid))
        {
            log_info(logger_cpu,"el chequeo de interrupcion encontro que hay una interrupcion");
            interruption_flag = 1;
        }
            
        usleep(500000);
    }

    imprimir_registros_por_pantalla(pcb->registros);

    if (instruccion != NULL)
        free(instruccion);

    if (end_process_flag == 1)
    {
        end_process_flag = 0;
        interruption_flag = 0;
        enviar_motivo_desalojo(END_PROCESS, socket_Kernel);
        enviar_pcb(pcb, socket_Kernel);
        log_info(logger_cpu, "Se envio el pcb al kernel con motivo de desalojo END_PROCESS");
    }
    if (interruption_flag == 1)
    {
        accion_interrupt(pcb, interrupcion_recibida->motivo, socket_Kernel);
        //interruption_flag = 0; no es necesario esta dentro del accion_interrupt
    }
    // input_ouput_flag = 0;
}
//------------------------FUNCIONES DE OPERACIONES------------------------------

void set_registro(t_registros *registros, char *registro, u_int32_t valor)
{
	if (strcasecmp(registro, "AX") == 0)
	{
		u_int8_t valor8 = (u_int8_t)valor;
		registros->AX = valor8;
	}

	if (strcasecmp(registro, "BX") == 0)
	{
		u_int8_t valor8 = (u_int8_t)valor;
		registros->BX = valor8;
	}
	if (strcasecmp(registro, "CX") == 0)
	{
		u_int8_t valor8 = (u_int8_t)valor;
		registros->CX = valor8;
	}
	if (strcasecmp(registro, "DX") == 0)
	{
		u_int8_t valor8 = (u_int8_t)valor;
		registros->DX = valor8;
	}
	if (strcasecmp(registro, "EAX") == 0)
	{
		registros->EAX = valor;
	}
	if (strcasecmp(registro, "EBX") == 0)
	{
		registros->EBX = valor;
	}
	if (strcasecmp(registro, "ECX") == 0)
	{
		registros->ECX = valor;
	}
	if (strcasecmp(registro, "EDX") == 0)
	{
		registros->EDX = valor;
	}
	if (strcasecmp(registro, "PC") == 0)
	{
		registros->PC = valor;
	}
    if (strcasecmp(registro, "SI") == 0)
	{
		registros->SI = valor;
	}
    if (strcasecmp(registro, "DI") == 0)
	{
		registros->DI = valor;
	}
}

u_int8_t get_registro_int8(t_registros *registros, char *registro)
{
	if (strcasecmp(registro, "AX") == 0)
	{
		return registros->AX;
	}
	else if (strcasecmp(registro, "BX") == 0)
	{
		return registros->BX;
	}
	else if (strcasecmp(registro, "CX") == 0)
	{
		return registros->CX;
	}
	else if (strcasecmp(registro, "DX") == 0)
	{
		return registros->DX;
	}
	return -1;
}

u_int32_t get_registro_int32(t_registros *registros, char *registro)
{
	if (strcasecmp(registro, "EAX") == 0)
	{
		return registros->EAX;
	}
	else if (strcasecmp(registro, "EBX") == 0)
	{
		return registros->EBX;
	}
	else if (strcasecmp(registro, "ECX") == 0)
	{
		return registros->ECX;
	}
	else if (strcasecmp(registro, "EDX") == 0)
	{
		return registros->EDX;
	}
	else if (strcasecmp(registro, "PC") == 0)
	{
		return registros->PC;
	}
    else if (strcasecmp(registro, "SI") == 0)
	{
		return registros->SI;
	}
    else if (strcasecmp(registro, "DI") == 0)
	{
		return registros->DI;
	}
	return -1;
}

u_int32_t get_registro_generico(t_registros *registros, char *registro)
{
    if (strcasecmp(registro, "AX") == 0)
	{
		return registros->AX;
	}
	else if (strcasecmp(registro, "BX") == 0)
	{
		return registros->BX;
	}
	else if (strcasecmp(registro, "CX") == 0)
	{
		return registros->CX;
	}
	else if (strcasecmp(registro, "DX") == 0)
	{
		return registros->DX;
	}
	else if (strcasecmp(registro, "EAX") == 0)
	{
		return registros->EAX;
	}
	else if (strcasecmp(registro, "EBX") == 0)
	{
		return registros->EBX;
	}
	else if (strcasecmp(registro, "ECX") == 0)
	{
		return registros->ECX;
	}
	else if (strcasecmp(registro, "EDX") == 0)
	{
		return registros->EDX;
	}
	else if (strcasecmp(registro, "PC") == 0)
	{
		return registros->PC;
	}
    else if (strcasecmp(registro, "SI") == 0)
	{
		return registros->SI;
	}
    else if (strcasecmp(registro, "DI") == 0)
	{
		return registros->DI;
	}
	return -1;
}

void sum_registro(t_registros *registros, char *registroDestino, char *registroOrigen)
{

	if (strcasecmp(registroDestino, "AX") == 0)
	{
		registros->AX += get_registro_int8(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "BX") == 0)
	{
		registros->BX += get_registro_int8(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "CX") == 0)
	{
		registros->CX += get_registro_int8(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "DX") == 0)
	{
		registros->DX += get_registro_int8(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "EAX") == 0)
	{
		registros->EAX += get_registro_int32(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "EBX") == 0)
	{
		registros->EBX += get_registro_int32(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "ECX") == 0)
	{
		registros->ECX += get_registro_int32(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "EDX") == 0)
	{
		registros->EDX += get_registro_int32(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "PC") == 0)
	{
		registros->PC += get_registro_int32(registros, registroOrigen);
	}
    else if (strcasecmp(registroDestino, "SI") == 0)
	{
		registros->SI += get_registro_int32(registros, registroOrigen);
	}
    else if (strcasecmp(registroDestino, "DI") == 0)
	{
		registros->DI += get_registro_int32(registros, registroOrigen);
	}
}

void sub_registro(t_registros *registros, char *registroOrigen, char *registroDestino)
{

	if (strcasecmp(registroDestino, "AX") == 0)
	{
		registros->AX -= get_registro_int8(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "BX") == 0)
	{
		registros->BX -= get_registro_int8(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "CX") == 0)
	{
		registros->CX -= get_registro_int8(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "DX") == 0)
	{
		registros->DX -= get_registro_int8(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "EAX") == 0)
	{
		registros->EAX -= get_registro_int32(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "EBX") == 0)
	{
		registros->EBX -= get_registro_int32(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "ECX") == 0)
	{
		registros->ECX -= get_registro_int32(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "EDX") == 0)
	{
		registros->EDX -= get_registro_int32(registros, registroOrigen);
	}
	else if (strcasecmp(registroDestino, "PC") == 0)
	{
		registros->PC -= get_registro_int32(registros, registroOrigen);
	}
    else if (strcasecmp(registroDestino, "SI") == 0)
	{
		registros->SI += get_registro_int32(registros, registroOrigen);
	}
    else if (strcasecmp(registroDestino, "DI") == 0)
	{
		registros->DI += get_registro_int32(registros, registroOrigen);
	}
}
void JNZ_registro(t_registros *registros, char *registro, u_int32_t valor)
{
	u_int8_t valoregistroInt8 = 0;
	u_int32_t valoregistroInt32 = 0;

	if (strcasecmp(registro, "AX") == 0 || strcasecmp(registro, "BX") == 0 || strcasecmp(registro, "CX") == 0 || strcasecmp(registro, "DX") == 0)
	{

		valoregistroInt8 = get_registro_int8(registros, registro);
		if (valoregistroInt8 != 0)
			registros->PC = valor;
	}
	if (strcasecmp(registro, "EAX") == 0 || strcasecmp(registro, "EBX") == 0 || strcasecmp(registro, "ECX") == 0 || strcasecmp(registro, "EDX") == 0 || strcasecmp(registro, "PC") == 0)
	{
		valoregistroInt32 = get_registro_int32(registros, registro);
		if (valoregistroInt32 != 0)
			registros->PC = valor;
	}
}
/*--------------esta es otra opcion para despues vemos cual preferimos -------------------
void get_registro(t_registros *registros, char *registro, void *valor) {
	// Define el mapa de cadenas a punteros
	struct {
		char *nombre;
		void *direccion;
	} mapa[] = {
		{"AX", &(registros->AX)},
		{"BX", &(registros->BX)},
		{"CX", &(registros->CX)},
		{"DX", &(registros->DX)},
		{"EAX", &(registros->EAX)},
		{"EBX", &(registros->EBX)},
		{"ECX", &(registros->ECX)},
		{"EDX", &(registros->EDX)}
	};

	// Itera sobre el mapa para encontrar el registro correspondiente
	for (size_t i = 0; i < sizeof(mapa) / sizeof(mapa[0]); i++) {
		if (strcasecmp(registro, mapa[i].nombre) == 0) {
			// Copia el valor del registro a la dirección proporcionada
			memcpy(valor, mapa[i].direccion, sizeof(uint32_t)); // Usa sizeof(uint8_t) o sizeof(uint32_t) según corresponda
			return;
		}
	}

	// Si no se encuentra el registro, aquí puedes manejar el error
}
*/

void enviar_soli_lectura(t_paquete *paquete_enviado,t_list *direcciones_fisicas,size_t tamanio_de_lectura)
{
    uint32_t num_direcciones = (uint32_t)list_size(direcciones_fisicas);
    buffer_add(paquete_enviado->buffer, &num_direcciones, sizeof(uint32_t));
    buffer_add(paquete_enviado->buffer, &tamanio_de_lectura, sizeof(uint32_t));
    
    for (size_t j = 0; j < list_size(direcciones_fisicas); j++) {
        t_direc_fisica *direccion_fisica = list_get(direcciones_fisicas, j);
        buffer_add(paquete_enviado->buffer, &(direccion_fisica->direccion_fisica), sizeof(uint32_t));
        buffer_add(paquete_enviado->buffer, &(direccion_fisica->desplazamiento_necesario), sizeof(uint32_t));
    }

    //buffer_add(paquete_enviado->buffer, &size_of_element, sizeof(uint32_t)); // Tamaño de lectura//creo que no es necesario
    enviar_paquete(paquete_enviado, conexion_memoria);
    eliminar_paquete(paquete_enviado);

}

void enviar_soli_escritura(t_paquete *paquete,t_list *direc_fisicas,size_t tamanio,void *valor)
{
    uint32_t num_direcciones = (uint32_t)list_size(direc_fisicas);
    buffer_add(paquete->buffer, &num_direcciones, sizeof(uint32_t));

    uint32_t offset = 0;
    for (size_t i = 0; i < list_size(direc_fisicas); i++) {
        t_direc_fisica *direc = list_get(direc_fisicas, i);
        size_t size_to_copy = (i == list_size(direc_fisicas) - 1) ? tamanio - offset : direc->desplazamiento_necesario;

        buffer_add(paquete->buffer, &(direc->direccion_fisica), sizeof(uint32_t));
        buffer_add(paquete->buffer, &(direc->desplazamiento_necesario), sizeof(uint32_t));
        buffer_add(paquete->buffer, ((char *)valor) + offset, size_to_copy);
        
        offset += direc->desplazamiento_necesario;
    }

    //buffer_add(paquete->buffer, &size_of_element, sizeof(uint32_t)); // Tamaño de escritura//creo que no es necesario
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
}

void mov_in(t_pcb *pcb, char *registro_datos, char *registro_direccion) {
    struct {
        char *nombre;
        void *direccion;
    } mapa[] = {
        {"AX", &(pcb->registros.AX)},
        {"BX", &(pcb->registros.BX)},
        {"CX", &(pcb->registros.CX)},
        {"DX", &(pcb->registros.DX)},
        {"EAX", &(pcb->registros.EAX)},
        {"EBX", &(pcb->registros.EBX)},
        {"ECX", &(pcb->registros.ECX)},
        {"EDX", &(pcb->registros.EDX)},
        {"SI", &(pcb->registros.SI)},
        {"DI", &(pcb->registros.DI)},
        {"PC", &(pcb->registros.PC)}
    };

    int encontrado = 0;
    for (size_t i = 0; i < sizeof(mapa) / sizeof(mapa[0]); i++) {
        if (strcasecmp(registro_direccion, mapa[i].nombre) == 0) {
            encontrado = 1;
            uint32_t direccion_logica = get_registro_generico(&(pcb->registros), registro_direccion);
            size_t size_of_element = (i < 4) ? sizeof(uint8_t) : sizeof(uint32_t);

            t_list *direcciones_fisicas = traducir_DL_a_DF_generico(direccion_logica, pcb->tabla_paginas, size_of_element);
            t_paquete *paquete_enviado = crear_paquete(LECTURA_MEMORIA);
            enviar_soli_lectura(paquete_enviado,direcciones_fisicas,size_of_element);

            t_paquete *paquete_recibido = recibir_paquete(conexion_memoria);
            if (paquete_recibido->codigo_operacion != LECTURA_MEMORIA) {
                log_error(logger_cpu, "error: codigo de operacion inesperado al recibir la lectura de memoria");
                eliminar_paquete(paquete_recibido);
                return;
            }

            paquete_recibido->buffer->offset = 0;
            void *buffer = malloc(size_of_element);
            buffer_read(paquete_recibido->buffer, buffer, size_of_element);
            memcpy(mapa[i].direccion, buffer, size_of_element);
            free(buffer);
            eliminar_paquete(paquete_recibido);

            list_destroy_and_destroy_elements(direcciones_fisicas, free);
        }
    }
    if (!encontrado) {
        log_error(logger_cpu, "error: no encontre ese registro");
    }
}

void mov_out(t_pcb *pcb, char *registro_direccion, char *registro_datos) {
    uint32_t direc_logica = get_registro_generico(&pcb->registros, registro_direccion);
    t_paquete *paquete = crear_paquete(ESCRITURA_MEMORIA);
    size_t size_of_element;
    void *valor_registro;
    t_list *direc_fisicas;

    if (strcasecmp(registro_datos, "AX") == 0 || strcasecmp(registro_datos, "BX") == 0 ||
        strcasecmp(registro_datos, "CX") == 0 || strcasecmp(registro_datos, "DX") == 0) {
        
        size_of_element = sizeof(uint8_t);
        uint8_t valor = get_registro_generico(&pcb->registros, registro_datos);
        valor_registro = &valor;
        direc_fisicas = traducir_DL_a_DF_generico(direc_logica, pcb->tabla_paginas, size_of_element);

    } else if (strcasecmp(registro_datos, "EAX") == 0 || strcasecmp(registro_datos, "EBX") == 0 ||
               strcasecmp(registro_datos, "ECX") == 0 || strcasecmp(registro_datos, "EDX") == 0 ||
               strcasecmp(registro_datos, "SI") == 0 || strcasecmp(registro_datos, "DI") == 0 ||
               strcasecmp(registro_datos, "PC") == 0) {

        size_of_element = sizeof(uint32_t);
        uint32_t valor = get_registro_generico(&pcb->registros, registro_datos);
        valor_registro = &valor;
        direc_fisicas = traducir_DL_a_DF_generico(direc_logica, pcb->tabla_paginas, size_of_element);

    } else {
        log_error(logger_cpu, "error: el registro dato no existe dentro de los registros");
        eliminar_paquete(paquete);
        return;
    }

    enviar_soli_escritura(paquete,direc_fisicas, size_of_element,valor_registro);

    t_paquete *paquete_recibido = recibir_paquete(conexion_memoria);
    if (paquete_recibido->codigo_operacion != OK) {
        log_error(logger_cpu, "error: codigo de operacion inesperado al recibir la escritura de memoria");
    }
    eliminar_paquete(paquete_recibido);

    list_destroy_and_destroy_elements(direc_fisicas, free);
}

void copy_string(t_pcb *pcb, size_t tamanio) {
    uint32_t direc_logica_si = get_registro_generico(&pcb->registros, "SI");
    uint32_t direc_logica_di = get_registro_generico(&pcb->registros, "DI");

    // Obtener las direcciones físicas para el origen (SI)
    t_list *direc_fisicas_si = traducir_DL_a_DF_generico(direc_logica_si, pcb->tabla_paginas, tamanio);

    // Obtener las direcciones físicas para el destino (DI)
    t_list *direc_fisicas_di = traducir_DL_a_DF_generico(direc_logica_di, pcb->tabla_paginas, tamanio);

    // Leer el string desde la memoria apuntada por SI
    t_paquete *paquete_lectura = crear_paquete(LECTURA_MEMORIA);
    enviar_soli_lectura(paquete_lectura,direc_fisicas_si,tamanio);

    t_paquete *paquete_recibido = recibir_paquete(conexion_memoria);
    if (paquete_recibido->codigo_operacion != LECTURA_MEMORIA) {
        log_error(logger_cpu, "error: codigo de operacion inesperado al recibir la lectura de memoria");
        eliminar_paquete(paquete_recibido);
        list_destroy_and_destroy_elements(direc_fisicas_si, free);
        list_destroy_and_destroy_elements(direc_fisicas_di, free);
        return;
    }

    paquete_recibido->buffer->offset = 0;
    void *buffer = malloc(tamanio);
    buffer_read(paquete_recibido->buffer, buffer, tamanio);//por ahie sto lo tena que cambiar
    eliminar_paquete(paquete_recibido);

    // Escribir el string a la memoria apuntada por DI
    t_paquete *paquete_escritura = crear_paquete(ESCRITURA_MEMORIA);
    enviar_soli_escritura(paquete_escritura,direc_fisicas_di,tamanio,buffer);

    t_paquete *paquete_recibido_escritura = recibir_paquete(conexion_memoria);
    if (paquete_recibido_escritura->codigo_operacion != OK) {
        log_error(logger_cpu, "error: codigo de operacion inesperado al recibir la escritura de memoria");
    }
    eliminar_paquete(paquete_recibido_escritura);

    free(buffer);
    list_destroy_and_destroy_elements(direc_fisicas_si, free);
    list_destroy_and_destroy_elements(direc_fisicas_di, free);
}

//--------------------MMU----------------------------
t_list *traducir_DL_a_DF_generico(uint32_t DL, t_list *TP, size_t size_of_element) {
    uint32_t numero_pagina = floor(DL / tamanio_de_pagina);
    uint32_t desplazamiento = DL - numero_pagina * tamanio_de_pagina;
    t_list *direcciones_fisicas = list_create();

    uint32_t espacio_restante = size_of_element;

    while (espacio_restante > 0) {
        t_pagina *info_pagina = list_get(TP, numero_pagina);
        if (!info_pagina) {
            log_error(logger_cpu, "error: pagina no encontrada");
            return direcciones_fisicas;
        }
        if (!info_pagina->presente) {
            log_error(logger_cpu, "error: pagina no utilizada");
        }

        uint32_t espacio_en_pagina = tamanio_de_pagina - desplazamiento;

        t_direc_fisica *direc = malloc(sizeof(t_direc_fisica));
        direc->direccion_fisica = info_pagina->numero_marco * tamanio_de_pagina + desplazamiento;
        if (espacio_en_pagina >= espacio_restante) {
            direc->desplazamiento_necesario = espacio_restante;
            list_add(direcciones_fisicas, direc);
            break;
        } else {
            direc->desplazamiento_necesario = espacio_en_pagina;
            list_add(direcciones_fisicas, direc);
            espacio_restante -= espacio_en_pagina;
            desplazamiento = 0;
            numero_pagina++;
        }
    }
    return direcciones_fisicas;
}


