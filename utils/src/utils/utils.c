#include <../include/utils.h>

//------------------ FUNCIONES DE PCB ------------------
t_pcb *crear_pcb(u_int32_t pid, t_list *lista_instrucciones, u_int32_t quantum, t_psw psw)

{
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->pid = pid;
	pcb->instrucciones = lista_instrucciones;
	pcb->pc = 0;
	pcb->quantum = quantum;
	pcb->psw = psw;
	pcb->registros = inicializar_registros();
	return pcb;
}

void destruir_pcb(t_pcb *pcb)
{
	free(pcb);
}

void enviar_pcb(t_pcb* pcb, int socket) 
{
	t_paquete* paquete = crear_paquete(PCB);
	agregar_a_paquete(paquete,pcb,sizeof(pcb)+1);
	enviar_paquete(paquete,socket );
}

void recibir_pcb(t_pcb* pcb, int socket) 
{
	int cod_op = recibir_operacion(socket);
	pcb = recibir_paquete(socket);//esto esta mal recibir paquete lo da en una lsita es raro eso me tengo que fijar como funciona
}


t_registros inicializar_registros()
{
	t_registros registros;

	registros->AX = 0;
	registros->BX = 0;
	registros->CX = 0;
	registros->DX = 0;
	registros->EAX = 0;
	registros->EBX = 0;
	registros->ECX = 0;
	registros->EDX = 0;
	registros->SI = 0;
	registros->DI = 0;
	registros->PC = 0;
	return registros;
}

char *cod_op_to_string(op_code codigo_operacion)

{
	switch (codigo_operacion)
	{
	case KERNEL:
		return "KERNEL";
	case CPU:
		return "CPU";
	case MEMORIA:
		return "MEMORIA";
	case ENTRADA_SALIDA:
		return "ENTRADA_SALIDA";
	default:
		return "CODIGO DE OPERACION INVALIDO";
	}
}

// ------------------ FUNCIONES DE LOGGER ------------------

t_log *iniciar_logger(char *path, char *nombre, t_log_level nivel)
{
	t_log *nuevo_logger;
	nuevo_logger = log_create(path, nombre, 1, nivel);
	if (nuevo_logger == NULL)
	{
		printf("Error al crear el logger: %s", strerror(errno));
		exit(1);
	}
	log_info(nuevo_logger, "Logger creado");
	return nuevo_logger;
}

// ------------------ FUNCIONES DE CONEXION/CLIENTE ------------------

int crear_conexion(char *ip, char *puerto, t_log *logger)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
	{

		log_error(logger, "Error al conectar el socket: %s", strerror(errno));
		return -1;
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

// ------------------ FUNCIONES DE SERVIDOR ------------------

int iniciar_servidor(t_log *logger, char *puerto, char *nombre)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_info(logger, "Servidor de %s escuchando en el puerto %s", nombre, puerto);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log *logger)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

// ------------------ FUNCIONES DE ENVIO ----------------------

void *serializar_paquete(t_paquete *paquete, int bytes)
{
	void *magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;
	return magic;
}

void crear_buffer(t_paquete *paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}


t_paquete *crear_paquete(int codigo_operacion)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_operacion;

	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2 * sizeof(int);
	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void enviar_mensaje(char *mensaje, int socket_cliente, op_code codigo_operacion, t_log *logger)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = codigo_operacion;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + sizeof(op_code) + sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);
	// Printeo todo para ver que se envia
	/*printf("Codigo de operacion: %d\n", paquete->codigo_operacion);
	printf("Tamanio del buffer: %d\n", paquete->buffer->size);
	printf("Mensaje: %s\n", (char*)paquete->buffer->stream);
	printf("Tamnio del mensaje: %ld\n", strlen(mensaje)+1);
	printf("Tamanio del paquete: %d\n", bytes);
	printf("Bytes a enviar: %d\n", bytes);
	printf("Socket cliente: %d\n", socket_cliente);*/

	int resultado = send(socket_cliente, a_enviar, bytes, 0);
	if (resultado == -1)
	{
		log_error(logger, "Error al enviar mensaje al socket %d: %s", socket_cliente, strerror(errno));
	}

	free(a_enviar);
	eliminar_paquete(paquete);
}

void eliminar_paquete(t_paquete *paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

// ------------------ FUNCIONES DE RECIBIR ----------------------

op_code recibir_operacion(int socket_cliente)
{
	op_code cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(op_code), MSG_WAITALL) > 0)
	{
		return cod_op;
	}
	else
	{
		close(socket_cliente);
		return -1;
	}
}


void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);
	return buffer;
}

void recibir_mensaje(int socket_cliente, t_log *logger)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

char *recibir_mensaje_guardar_variable(int socket_cliente)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	return buffer;
	free(buffer);
}

t_paquete *recibir_paquete(int socket_cliente)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->stream = NULL;
    paquete->buffer->size = 0;
    paquete->codigo_operacion = 0;

    recv(socket_cliente, &(paquete->codigo_operacion), sizeof(uint8_t), 0);
    recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

    return paquete;
}

// ------------------ FUNCIONES DE FINALIZACION ------------------

void terminar_programa(int conexion, t_log *logger, t_config *config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}

// Serialización para TADS

uint32_t espacio_parametros(t_instruccion *instruccion)
{
	uint32_t espacio = 0;
	for (int i = 0; i < instruccion->cant_parametros; i++)
	{
		espacio += strlen(instruccion->parametros[i]) + 1;
	}
	return espacio;
}

t_buffer *instruccion_serializar(t_instruccion *instruccion)
{
	t_buffer *buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(t_identificador) +
				   sizeof(uint32_t) +
				   espacio_parametros(instruccion) +
				   sizeof(uint32_t) * 5; // Los 5 parámetros
	buffer->stream = malloc(buffer->size);
	int offset = 0;

	memccpy(buffer->stream + offset, &instruccion->identificador, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memccpy(buffer->stream + offset, &instruccion->cant_parametros, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memccpy(buffer->stream + offset, &instruccion->param1_length, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memccpy(buffer->stream + offset, &instruccion->param2_length, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memccpy(buffer->stream + offset, &instruccion->param3_length, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memccpy(buffer->stream + offset, &instruccion->param4_length, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memccpy(buffer->stream + offset, &instruccion->param5_length, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	for (int i = 0; i < instruccion->cant_parametros; i++)
	{
		memccpy(buffer->stream + offset, instruccion->parametros[i], strlen(instruccion->parametros[i]) + 1);
		offset += strlen(instruccion->parametros[i]) + 1;
	}
	return buffer;
}

t_instruccion *instruccion_deserializar(t_buffer *buffer)
{
	t_instruccion *instruccion = malloc(size_of(t_instruccion));

	void *stream = buffer->stream;

	memccpy(&(instruccion->identificador), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memccpy(&(instruccion->cant_parametros), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memccpy(&(instruccion->param1_length), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memccpy(&(instruccion->param2_length), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memccpy(&(instruccion->param3_length), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memccpy(&(instruccion->param4_length), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memccpy(&(instruccion->param5_length), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	// TODO: LISTA DE PARÁMETROS

	return instruccion;
	
}
