#include <../include/kernel.h>


void iniciar_consola_interactiva()
{
    char *comandoRecibido;
    comandoRecibido = readline("> ");

    while (strcmp(comandoRecibido, ""))
    {
        if (!validar_comando(comandoRecibido))
        {
            log_error(logger_kernel, "Comando de CONSOLA no reconocido");
            free(comandoRecibido);
            comandoRecibido = readline("> ");
            continue;
        }

        //ejecutar_comando(comandoRecibido);
        free(comandoRecibido);
        comandoRecibido = readline("> ");
    }

    free(comandoRecibido);
}

bool validar_comando(char comando){
    bool validacion = false; 
    switch (comando) {
        case "Ejecutar Script de Operaciones":
            validacion = true;
        case "":
            validacion = true;
    }
    return validacion;
}