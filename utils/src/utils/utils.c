#include <../include/utils.h>
// uint32_t size_registros = sizeof(uint32_t) * 7 + sizeof(uint8_t) * 4;

//----------------- FUNCIONES DE REGISTROS -------------------------------

// TODO testear que todas estas funciones anden

sem_t semaforo;

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
	return -1;
}

void sum_registro(t_registros *registros, char *registroOrigen, char *registroDestino)
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

void imprimir_registros_por_pantalla(t_registros registros)
{
	printf("-------------------\n");
	printf("Valores de los Registros:\n");
	printf(" + AX = %d\n", registros.AX);
	printf(" + BX = %d\n", registros.BX);
	printf(" + CX = %d\n", registros.CX);
	printf(" + DX = %d\n", registros.DX);
	printf(" + EAX = %d\n", registros.EAX);
	printf(" + EBX = %d\n", registros.EBX);
	printf(" + ECX = %d\n", registros.ECX);
	printf(" + EDX = %d\n", registros.EDX);
	printf(" + PC = %d\n", registros.PC);
	printf("-------------------\n");
}

//------------------ FUNCIONES DE PCB ------------------

t_pcb *crear_pcb(u_int32_t pid, t_list *lista_instrucciones, u_int32_t quantum, estados estado)
{
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->pid = pid;
	pcb->instrucciones = lista_instrucciones;
	pcb->pc = 0;
	pcb->quantum = quantum;
	pcb->estado_actual = estado;
	pcb->registros = inicializar_registros();
	return pcb;
}

void destruir_pcb(t_pcb *pcb)
{
	free(pcb);
}

void enviar_pcb(t_pcb *pcb, int socket)
{
	t_buffer *buffer_instrucciones = crear_buffer();
	buffer_instrucciones = serializar_lista_instrucciones(pcb->instrucciones);
	t_paquete *paquete = crear_paquete(PCB);
	t_buffer *buffer = paquete->buffer;
	buffer_add(buffer, &(pcb->pid), sizeof(uint32_t));
	buffer_add(buffer, &(pcb->pc), sizeof(uint32_t));
	buffer_add(buffer, &(pcb->quantum), sizeof(uint32_t));
	buffer_add(buffer, &(pcb->registros), SIZE_REGISTROS);
	buffer_add(buffer, &(pcb->estado_actual), sizeof(estados));
	buffer_add(buffer, buffer_instrucciones->stream, buffer_instrucciones->size);
	buffer->offset = 0;

	enviar_paquete(paquete, socket);

	destruir_buffer(buffer_instrucciones);
	eliminar_paquete(paquete);
}

t_pcb *recibir_pcb(int socket)
{
	t_paquete *paquete_PCB = recibir_paquete(socket);
	t_buffer *buffer_PCB = paquete_PCB->buffer;

	t_pcb *pcb = malloc(sizeof(t_pcb));

	if (paquete_PCB->codigo_operacion == PCB)
	{
		buffer_read(buffer_PCB, &(pcb->pid), sizeof(pcb->pid));
		buffer_read(buffer_PCB, &(pcb->pc), sizeof(pcb->pc));
		buffer_read(buffer_PCB, &(pcb->quantum), sizeof(pcb->quantum));
		buffer_read(buffer_PCB, &(pcb->registros), SIZE_REGISTROS);
		buffer_read(buffer_PCB, &(pcb->estado_actual), sizeof(estados));

		pcb->instrucciones = list_create();
		t_list *lista_instrucciones = deserializar_lista_instrucciones(buffer_PCB, buffer_PCB->offset);
		list_add_all(pcb->instrucciones, lista_instrucciones);
	}

	eliminar_paquete(paquete_PCB);
	// TODO: deserializar las instrucciones
	return pcb;
}

t_registros inicializar_registros()
{
	t_registros registros;
	registros.AX = 0;
	registros.BX = 0;
	registros.CX = 0;
	registros.DX = 0;
	registros.EAX = 0;
	registros.EBX = 0;
	registros.ECX = 0;
	registros.EDX = 0;
	registros.SI = 0;
	registros.DI = 0;
	registros.PC = 0;
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
void cargar_string_al_buffer(t_buffer *buffer, char *string) {
    int len = strlen(string);
    buffer->stream = malloc(len + 1); 
    if (buffer->stream == NULL) {
        printf("Error: no se pudo asignar memoria para el buffer\n");
        exit(EXIT_FAILURE);
    }
    strcpy(buffer->stream, string);
    buffer->size = len;
}
char *extraer_string_del_buffer(t_buffer *buffer) {
    char *string = malloc(buffer->size + 1);  
    if (string == NULL) {
        printf("Error: no se pudo asignar memoria para el string\n");
        exit(EXIT_FAILURE);
    }
    strncpy(string, buffer->stream, buffer->size);
    string[buffer->size] = '\0';
    free(buffer->stream);
    buffer->size = 0;
    return string;
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

	/*
		//Dejo esto aca es util para pritear lo que hay dentro de magic para ver si esta bien te lo de
		//			en exa pero se lo das a chatgpt y te dice lo que vale

	for (int i = 0; i < bytes; i++) {
	printf("%02X ", ((unsigned char*)magic)[i]);
	}
printf("\n");
	*/
	return magic;
}

t_paquete *crear_paquete(op_code codigo_operacion)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_operacion;

	paquete->buffer = crear_buffer();
	return paquete;
}

void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + sizeof(op_code) + sizeof(u_int32_t);
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

op_code recibir_operacion(int socket_cliente) // de utilziar esta funcion no se podria utilizar la funcion recibir paquete ya que esta ya recibe en codigo de operacion
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
	paquete->buffer->offset = 0;
	paquete->codigo_operacion = 0;

	int bytes_recibidos;

	bytes_recibidos = recv(socket_cliente, &(paquete->codigo_operacion), sizeof(op_code), 0);
	if (bytes_recibidos <= 0)
	{
		// El cliente se ha desconectado o ha ocurrido un error
		free(paquete->buffer);
		free(paquete);
		return NULL;
	}

	bytes_recibidos = recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0);
	if (bytes_recibidos <= 0)
	{
		// El cliente se ha desconectado o ha ocurrido un error
		free(paquete->buffer);
		free(paquete);
		return NULL;
	}

	paquete->buffer->stream = malloc(paquete->buffer->size);
	bytes_recibidos = recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);
	if (bytes_recibidos <= 0)
	{
		// El cliente se ha desconectado o ha ocurrido un error
		free(paquete->buffer->stream);
		free(paquete->buffer);
		free(paquete);
		return NULL;
	}

	return paquete;
}

// ------------------ FUNCIONES DE FINALIZACION ------------------

void terminar_programa(int conexion, t_log *logger, t_config *config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}

t_buffer *crear_buffer()
{
	t_buffer *buffer;
	buffer = malloc(sizeof(t_buffer));
	buffer->size = 0;
	buffer->stream = NULL;
	buffer->offset = 0;
	return buffer;
}

void liberar_buffer(t_buffer *buffer)
{
	if (buffer != NULL)
	{
		if (buffer->stream != NULL)
		{
			free(buffer->stream);
			buffer->stream = NULL;
		}
		free(buffer);
	}
}

void destruir_buffer(t_buffer *buffer)
{
	free(buffer->stream);
	free(buffer);
}

void buffer_add(t_buffer *buffer, void *data, uint32_t size)
{
	void *new_stream = realloc(buffer->stream, buffer->size + size);
	if (new_stream == NULL)
	{
		// handle error, e.g., by logging it and returning
		printf("Error al agregar datos al buffer");
		return;
	}
	buffer->stream = new_stream;
	memcpy(buffer->stream + buffer->offset, data, size);
	buffer->size += size;
	buffer->offset += size;
}

void buffer_read(t_buffer *buffer, void *data, uint32_t size)
{
	if (buffer == NULL || data == NULL)
	{
		printf("Manejar punteros nulos");
		return;
	}

	// Verificar límites de lectura
	if (buffer->offset + size > buffer->size)
	{
		printf("Manejar error de lectura fuera de límites");
		return;
	}

	// Leer datos desde el flujo de datos del buffer
	memcpy(data, buffer->stream + buffer->offset, size);

	// Actualizar el desplazamiento del buffer
	buffer->offset += size;
}

void imprimir_instruccion(t_instruccion instruccion)
{
	// Imprimir el identificador (como un entero)
	printf("Identificador: %d\n", instruccion.identificador);

	// Imprimir los parámetros
	printf("Parámetros:\n");
	for (uint32_t i = 0; i < instruccion.cant_parametros; i++)
	{
		printf("  Parametro %u: %s\n", i + 1, instruccion.parametros[i]);
	}
}

uint32_t espacio_parametros(t_instruccion *instruccion)
{
	uint32_t espacio = 0;
	for (int i = 0; i < instruccion->cant_parametros; i++)
	{
		espacio += strlen(instruccion->parametros[i]) + 1;
	}
	return espacio;
}

t_buffer *serializar_instruccion(t_instruccion *instruccion)
{
	t_buffer *buffer = crear_buffer();

	buffer_add(buffer, &instruccion->identificador, sizeof(uint32_t));
	buffer_add(buffer, &instruccion->cant_parametros, sizeof(uint32_t));
	buffer_add(buffer, &instruccion->param1_length, sizeof(uint32_t));
	buffer_add(buffer, &instruccion->param2_length, sizeof(uint32_t));
	buffer_add(buffer, &instruccion->param3_length, sizeof(uint32_t));
	buffer_add(buffer, &instruccion->param4_length, sizeof(uint32_t));
	buffer_add(buffer, &instruccion->param5_length, sizeof(uint32_t));
	// for (uint32_t i = 0; i < instruccion->cant_parametros; i++)
	// {
	// 	char *parametro_actual = instruccion->parametros[i];
	//     uint32_t longitud_parametro = strlen(parametro_actual) + 1; // Incluye el terminador nulo '\0'
	//     buffer_add(buffer, parametro_actual, longitud_parametro);
	// }
	if (instruccion->cant_parametros >= 1)
	{
		buffer_add(buffer, instruccion->parametros[0], instruccion->param1_length);
	}

	if (instruccion->cant_parametros >= 2)
	{
		buffer_add(buffer, instruccion->parametros[1], instruccion->param2_length);
	}
	if (instruccion->cant_parametros >= 3)
	{
		buffer_add(buffer, instruccion->parametros[2], instruccion->param3_length);
	}
	if (instruccion->cant_parametros >= 4)
	{
		buffer_add(buffer, instruccion->parametros[3], instruccion->param4_length);
	}
	if (instruccion->cant_parametros >= 5)
	{
		buffer_add(buffer, instruccion->parametros[4], instruccion->param5_length);
	}
	return buffer;
}

t_instruccion *instruccion_deserializar(t_buffer *buffer, u_int32_t offset)
{
	t_instruccion *instruccion = malloc(sizeof(t_instruccion));
	buffer->offset = offset;

	buffer_read(buffer, &instruccion->identificador, sizeof(uint32_t));
	buffer_read(buffer, &instruccion->cant_parametros, sizeof(uint32_t));
	buffer_read(buffer, &instruccion->param1_length, sizeof(uint32_t));
	buffer_read(buffer, &instruccion->param2_length, sizeof(uint32_t));
	buffer_read(buffer, &instruccion->param3_length, sizeof(uint32_t));
	buffer_read(buffer, &instruccion->param4_length, sizeof(uint32_t));
	buffer_read(buffer, &instruccion->param5_length, sizeof(uint32_t));
	u_int32_t tamanio = instruccion->param1_length + instruccion->param2_length + instruccion->param3_length + instruccion->param4_length + instruccion->param5_length;
	instruccion->parametros = malloc(tamanio);

	for (uint32_t i = 0; i < instruccion->cant_parametros; i++)
	{
		// Reservar memoria para el parámetro y leerlo del buffer
		uint32_t longitud_parametro = 0;
		switch (i)
		{
		case 0:
			longitud_parametro = instruccion->param1_length;
			break;
		case 1:
			longitud_parametro = instruccion->param2_length;
			break;
		case 2:
			longitud_parametro = instruccion->param3_length;
			break;
		case 3:
			longitud_parametro = instruccion->param4_length;
			break;
		case 4:
			longitud_parametro = instruccion->param5_length;
			break;
		default:
			break;
		}
		instruccion->parametros[i] = malloc(longitud_parametro);
		buffer_read(buffer, instruccion->parametros[i], longitud_parametro);
	}

	return instruccion;
}

t_buffer *serializar_lista_instrucciones(t_list *lista_instrucciones)
{
	t_buffer *buffer = crear_buffer();
	for (int i = 0; i < list_size(lista_instrucciones); i++)
	{
		t_instruccion *instruccion = list_get(lista_instrucciones, i);
		t_buffer *buffer_instruccion = serializar_instruccion(instruccion);
		buffer_add(buffer, buffer_instruccion->stream, buffer_instruccion->size);
		destruir_buffer(buffer_instruccion);
	}

	return buffer;
}

t_list *deserializar_lista_instrucciones(t_buffer *buffer, u_int32_t offset)
{
	t_list *lista_instrucciones = list_create();
	buffer->offset = offset;
	while (buffer->offset < buffer->size)
	{
		t_instruccion *instruccion = instruccion_deserializar(buffer, buffer->offset);
		list_add(lista_instrucciones, instruccion);
	}

	return lista_instrucciones;
}

// Función para destruir una instrucción
void destruir_instruccion(t_instruccion *instruccion)
{
	if (instruccion == NULL)
	{
		return;
	}
	for (uint32_t i = 0; i < instruccion->cant_parametros; i++)
	{
		free(instruccion->parametros[i]);
	}
	free(instruccion->parametros);
	free(instruccion);
}

void agregar_parametro_a_instruccion(t_list *parametros, t_instruccion *instruccion)
{
	int i = 0;
	if (parametros != NULL)
		while (i < instruccion->cant_parametros)
		{
			char *parametro = list_get(parametros, i);
			instruccion->parametros[i] = strdup(parametro);
			i++;
		}
	instruccion->param1_length = 0;
	instruccion->param2_length = 0;
	instruccion->param3_length = 0;
	instruccion->param4_length = 0;
	instruccion->param5_length = 0;
	if (instruccion->cant_parametros >= 1)
		instruccion->param1_length = strlen(instruccion->parametros[0]) + 1;
	if (instruccion->cant_parametros >= 2)
		instruccion->param2_length = strlen(instruccion->parametros[1]) + 1;
	if (instruccion->cant_parametros >= 3)
		instruccion->param3_length = strlen(instruccion->parametros[2]) + 1;
	if (instruccion->cant_parametros >= 4)
		instruccion->param4_length = strlen(instruccion->parametros[3]) + 1;
	if (instruccion->cant_parametros >= 5)
		instruccion->param5_length = strlen(instruccion->parametros[4]) + 1;
}

t_instruccion *crear_instruccion(t_identificador identificador, t_list *parametros)
{
	t_instruccion *instruccionNueva = malloc(sizeof(t_instruccion));

	instruccionNueva->identificador = identificador;
	if (list_size(parametros) < 1)
	{
		instruccionNueva->cant_parametros = 0;
		instruccionNueva->parametros = NULL;
		instruccionNueva->param1_length = 0;
		instruccionNueva->param2_length = 0;
		instruccionNueva->param3_length = 0;
		instruccionNueva->param4_length = 0;
		instruccionNueva->param5_length = 0;
	}
	else
	{
		instruccionNueva->cant_parametros = list_size(parametros);
		instruccionNueva->parametros = malloc(sizeof(char *) * instruccionNueva->cant_parametros);
		agregar_parametro_a_instruccion(parametros, instruccionNueva);
	}
	return instruccionNueva;
}

void agregar_instruccion_a_paquete(t_paquete *paquete, t_instruccion *instruccion)
{
	t_buffer *buffer_instruccion = serializar_instruccion(instruccion);
	buffer_add(paquete->buffer, buffer_instruccion->stream, buffer_instruccion->size);
	destruir_buffer(buffer_instruccion);
}

// -----------------------

t_buffer *serializar_solicitud_crear_proceso(t_solicitudCreacionProcesoEnMemoria *solicitud)
{
	t_buffer *buffer = crear_buffer();

	buffer_add(buffer, &solicitud->pid, sizeof(uint32_t));
	buffer_add(buffer, &solicitud->path_length, sizeof(uint32_t));
	buffer_add(buffer, solicitud->path, solicitud->path_length);

	return buffer;
}

t_solicitudCreacionProcesoEnMemoria *deserializar_solicitud_crear_proceso(t_buffer *buffer)
{
	t_solicitudCreacionProcesoEnMemoria *solicitud = malloc(sizeof(t_solicitudCreacionProcesoEnMemoria));
	buffer->offset = 0;

	buffer_read(buffer, &solicitud->pid, sizeof(uint32_t));
	buffer_read(buffer, &solicitud->path_length, sizeof(uint32_t));
	solicitud->path = malloc(solicitud->path_length);
	buffer_read(buffer, solicitud->path, solicitud->path_length);
	return solicitud;
}

void imprimir_proceso(t_proceso *proceso)
{
	printf("PID: %d\n", proceso->pid);
	// printf("Path del archivo de instrucciones: %s\n", proceso->path);
	printf("Lista de instrucciones:\n");
	for (int i = 0; i < list_size(proceso->lista_instrucciones); i++)
	{
		t_instruccion *instruccion = list_get(proceso->lista_instrucciones, i);
		printf("Instrucción %d: ", i + 1);
		imprimir_instruccion(*instruccion);
	}
	// Si quieres imprimir también la tabla de páginas, puedes hacerlo aquí
}