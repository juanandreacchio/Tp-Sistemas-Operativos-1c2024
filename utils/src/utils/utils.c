#include <../include/utils.h>


// ------------------ FUNCIONES DE LOGGER ------------------



t_log* iniciar_logger(char *path, char *nombre, t_log_level nivel)
{
	t_log* nuevo_logger;
	nuevo_logger = log_create(path, nombre, 1, nivel);
	if(nuevo_logger == NULL ){printf("no pude cargar el logger");exit(1);}
	log_info(nuevo_logger,"Soy un Log");
	return nuevo_logger;
}

t_config *iniciar_config(char *path)
{
    t_config *nuevo_config = config_create(path);
    return nuevo_config;
}



// ------------------ FUNCIONES DE CONEXION/CLIENTE ------------------



int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,server_info->ai_socktype,server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1){
		printf("error con la conexion");
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}



// ------------------ FUNCIONES DE SERVIDOR ------------------




int iniciar_servidor(t_log* logger, char* puerto, char* nombre)
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
	log_trace(logger, "Listo para escuchar a %s:%s (%s)", ip, puerto, nombre);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log* logger)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}


// ------------------ FUNCIONES DE FINALIZACION ------------------



void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}
