t_buffer *serializar_instruccion(t_instruccion *instruccion)
{
	t_buffer *buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(t_identificador) +
				   sizeof(uint32_t) +
				   espacio_parametros(instruccion) +
				   sizeof(uint32_t) * 5; // Los 5 parámetros
	buffer->stream = malloc(buffer->size);
	buffer->offset = 0;

	buffer_add(buffer, instruccion->identificador, sizeof(uint32_t));
	buffer_add(buffer, instruccion->cant_parametros, sizeof(uint32_t));
	buffer_add(buffer, instruccion->param1_length, sizeof(uint32_t));
	buffer_add(buffer, instruccion->param2_length, sizeof(uint32_t));
	buffer_add(buffer, instruccion->param3_length, sizeof(uint32_t));
	buffer_add(buffer, instruccion->param4_length, sizeof(uint32_t));
	buffer_add(buffer, instruccion->param4_length, sizeof(uint32_t));
	buffer_add(buffer, instruccion->param5_length, sizeof(uint32_t));
	for (int i = 0; i < instruccion->cant_parametros; i++)
	{
		buffer_add(buffer, instruccion->parametros[i], strlen(instruccion->parametros[i]) + 1)
	}
	return buffer;
}

t_instruccion *instruccion_deserializar(t_buffer *buffer)
{
	t_instruccion *instruccion = malloc(sizeof(t_instruccion));
	buffer->offset = 0;

	void *stream = buffer->stream;

	buffer_read(buffer, instruccion->identificador, sizeof(uint32_t));
	buffer_read(buffer, instruccion->cant_parametros, sizeof(uint32_t));
	buffer_read(buffer, instruccion->param1_length, sizeof(uint32_t));
	buffer_read(buffer, instruccion->param2_length, sizeof(uint32_t));
	buffer_read(buffer, instruccion->param3_length, sizeof(uint32_t));
	buffer_read(buffer, instruccion->param4_length, sizeof(uint32_t));
	buffer_read(buffer, instruccion->param5_length, sizeof(uint32_t));

	// TODO: LISTA DE PARÁMETROS

	return instruccion;
}

t_buffer *serializar_lista_instrucciones(t_list *lista_instrucciones) 
{
    t_buffer *buffer = crear_buffer();
    for (int i = 0; i < list_size(lista_instrucciones); i++)
    {
        t_instruccion *instruccion = list_get(lista_instrucciones, i);
        t_buffer *buffer_instruccion = serializar_instruccion(instruccion);
        buffer_add(buffer,buffer_instruccion->stream,buffer_instruccion->size);
        destruir_buffer(buffer_instruccion);
    }
}

// TODO DESERIALIZAR LISTA DE INSTRUCCIONES
