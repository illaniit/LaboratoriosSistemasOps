#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include "Comun.h"
sem_t *sem_usuarios;

sem_t *sem_transacciones;

// Inicializar semáforos compartidos

void Inicializar_semaforos()
{

    sem_usuarios = sem_open("/sem_usuarios", O_CREAT, 0666, 1);
    sem_transacciones = sem_open("/sem_transacciones", O_CREAT, 0666, 1);
    if (sem_usuarios == SEM_FAILED || sem_transacciones == SEM_FAILED)
    {
        perror("Error al inicializar semáforos");
        exit(EXIT_FAILURE);
    }
}

// Destruir semáforos compartidos

void Destruir_semaforos()
{
    sem_close(sem_usuarios);
    sem_close(sem_transacciones);
    sem_unlink("/sem_usuarios");
    sem_unlink("/sem_transacciones");
}

Config leer_configuracion(const char *ruta)
{
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL)
    {
        perror("Error al abrir config.txt");
        exit(1);
    }
    Config config;
    char linea[100];
    while (fgets(linea, sizeof(linea), archivo))
    {
        if (linea[0] == '#' || strlen(linea) < 3)
            continue; // Ignorar comentarios y
        if (strstr(linea, "LIMITE_RETIRO"))
            sscanf(linea, "LIMITE_RETIRO=%d",
                   &config.limite_retiro);
        else if (strstr(linea, "LIMITE_TRANSFERENCIA"))
            sscanf(linea,
                   "LIMITE_TRANSFERENCIA=%d", &config.limite_transferencia);
        else if (strstr(linea, "UMBRAL_RETIROS"))
            sscanf(linea, "UMBRAL_RETIROS=%d",
                   &config.umbral_retiros);
        else if (strstr(linea, "UMBRAL_TRANSFERENCIAS"))
            sscanf(linea,
                   "UMBRAL_TRANSFERENCIAS=%d", &config.umbral_transferencias);
        else if (strstr(linea, "NUM_HILOS"))
            sscanf(linea, "NUM_HILOS=%d",
                   &config.num_hilos);
        else if (strstr(linea, "ARCHIVO_CUENTAS"))
            sscanf(linea, "ARCHIVO_CUENTAS=%s",
                   config.archivo_cuentas);
        else if (strstr(linea, "ARCHIVO_LOG"))
            sscanf(linea, "ARCHIVO_LOG=%s",
                   config.archivo_log);
    }
    fclose(archivo);
    return config;
}
// Funcion para escribir en el registro de log
void Escribir_registro(const char *mensaje_registro)
{
    // declaramos la variable time_t
    time_t t;
    struct tm *tm_info;
    char hora[30]; // Para almacenar la fecha y hora formateadas

    // Obtiene la hora actual
    time(&t);
    tm_info = localtime(&t);

    // Formatea la fecha y hora en "YYYY-MM-DD HH:MM:SS"
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);

    // Abre el archivo en modo "a" para añadir sin sobrescribir
    FILE *ArchivoDeRegistro = fopen("registro.log", "a");
    if (!ArchivoDeRegistro)
    {
        perror("Error al abrir el archivo de registro");
        return;
    }

    // Escribe la fecha, la hora y el mensaje en el archivo
    fprintf(ArchivoDeRegistro, "[%s] %s\n", hora, mensaje_registro);

    // Cierra el archivo
    fclose(ArchivoDeRegistro);
}

/// @brief esta funcion limpia la cadena para el strcmp posterior , lo que hace es quitar los espacios en blanco y tabulaciones de la cadena
/// @param cadena
void limpiar_cadena(char *cadena)
{
    int inicio = 0;
    int fin = strlen(cadena) - 1;

    // Eliminar los espacios al principio
    while (cadena[inicio] == ' ' || cadena[inicio] == '\t')
    {
        inicio++;
    }

    // Eliminar los espacios al final
    while (fin >= inicio && (cadena[fin] == ' ' || cadena[fin] == '\t'))
    {
        fin--;
    }

    // Mover la cadena limpia
    for (int i = 0; i <= fin - inicio; i++)
    {
        cadena[i] = cadena[inicio + i];
    }
    cadena[fin - inicio + 1] = '\0'; // Añadir el carácter nulo al final
}
