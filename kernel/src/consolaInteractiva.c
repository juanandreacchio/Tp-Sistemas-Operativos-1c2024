#include <../include/kernel.h>

int grado_multiprogramacion;

void iniciar_consola_interactiva()
{
    char *comandoRecibido;
    comandoRecibido = readline("> ");

    while (strcmp(comandoRecibido, "") != 0)
    {
        if (!validar_comando(comandoRecibido))
        {
            log_error(logger_kernel, "Comando de CONSOLA no reconocido");
            free(comandoRecibido);
            comandoRecibido = readline("> ");
            continue; // se saltea el resto del while y vuelve a empezar
        }

        ejecutar_comando(comandoRecibido);
        free(comandoRecibido);
        comandoRecibido = readline("> ");
    }

    free(comandoRecibido);
}

int asigno_pid (){
    int valor_pid;

    pthread_mutex_lock(mutex_pid); 
    valor_pid = identificador_pid; 
    identificador_pid++; 
    pthread_mutex_unlock(mutex_pid); 

    return valor_pid;
}
bool validar_comando(char* comando){
    bool validacion = false; 
    char** consola = string_split(comando, " "); 
    if (strcmp(consola[0], "EJECUTAR_SCRIPT") == 0)
        validacion = true;
    else if(strcmp(consola[0], "INICIAR_PROCESO") == 0)
        validacion = true;
    else if(strcmp(consola[0], "FINALIZAR_PROCESO") == 0)
        validacion = true;
    else if(strcmp(consola[0], "DETENER_PLANIFICACION") == 0)
        validacion = true;
    else if(strcmp(consola[0], "INICIAR_PLANIFICACION") == 0)
        validacion = true; 
    else if(strcmp(consola[0], "MULTIPROGRAMACION") == 0)
        validacion = true;
    else if(strcmp(consola[0], "PROCESO_ESTADO") == 0)
        validacion = true;

string_array_destroy(consola); 
return validacion; 
}


void ejecutar_comando(char* comando){
    char** consola = string_split(comando, " "); 
    t_buffer* buffer = crear_buffer();

    if(strcmp(consola[0], "EJECUTAR_SCRIPT") == 0){
        cargar_string_al_buffer(buffer, consola[1]);//[path]
        ejecutar_script(buffer);
    }
    else if(strcmp(consola[0], "INICIAR_PROCESO") == 0){
        cargar_string_al_buffer(buffer, consola[1]);//[path]
        iniciar_proceso(buffer);
    }
    else if(strcmp(consola[0], "FINALIZAR_PROCESO") == 0){
        cargar_string_al_buffer(buffer, consola[1]);//[pid]
        finalizar_proceso(buffer);
    }
    else if(strcmp(consola[0], "MULTIPROGRAMACION") == 0){
        cargar_string_al_buffer(buffer, consola[1]);//[valor]
        multiprogramacion(buffer);
    }
    else if(strcmp(consola[0], "PROCESO_ESTADO") == 0)
        proceso_estado();
    else if(strcmp(consola[0], "DETENER_PLANIFICACION") == 0 )
        detener_planificacion();
    else if(strcmp(consola[0], "INICIAR_PLANIFICACION") == 0) 
        iniciar_planificacion();
   else{
        log_error(logger_kernel,"comando no reconocido, a pesar de que entro al while");
        exit(EXIT_FAILURE);
    }
    string_array_destroy(consola); 
}

void multiprogramacion(t_buffer* buffer){
    int nuevoValor = atoi(extraer_string_del_buffer(buffer));
    grado_multiprogramacion= nuevoValor;
}

