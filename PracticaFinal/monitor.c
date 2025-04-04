
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
#include <sys/prctl.h> // Para PR_SET_PDEATHSIG
#include <ctype.h>
#include <sys/stat.h>
#define ALERT_PIPE "/tmp/alerta_pipe"
int pipefd[2];

void CrearHilos();
void *detectar_transacciones_sospechosas(void *arg) ;
void *detectar_transferencias_consecutivas(void *arg);
void *detectar_retiros_consecutivos(void *arg);
void *detectar_saldo_negativo(void *arg);


int main()
{
    while (1)
    {
        CrearHilos();
        sleep(10);
    }
}


void enviar_alerta(const char *mensaje, const int *id, const int titular)
{
    // Registra que se ha enviado una alerta
    Escribir_registro("Se ha enviado una alerta");

    // Obtenemos la hora actual
    time_t t;
    struct tm *tm_info;
    char hora[30]; // Para almacenar la fecha y hora formateadas
    time(&t);
    tm_info = localtime(&t);
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);

    // Creamos el mensaje a enviar
    char auxiliar[256];
    if (titular == 1)
    {
        snprintf(auxiliar, sizeof(auxiliar), "[%s] Id del Usuario: %d | %s\n", hora, *id, mensaje);
    }
    else if (titular == 0)
    {
        snprintf(auxiliar, sizeof(auxiliar), "[%s] Id de la transacción: %d | Transacción | %s\n", hora, *id, mensaje);
    }

    // Abrimos la named pipe en modo de escritura
    int fd = open(ALERT_PIPE, O_WRONLY);
    if (fd == -1) {
        perror("Error al abrir la named pipe");
        return;
    }

    // Escribimos el mensaje en la named pipe
    if (write(fd, auxiliar, strlen(auxiliar)) == -1) {
        perror("Error al escribir en la named pipe");
    }

    // Cerramos el archivo de la named pipe
    close(fd);
}
// Mediante señales limpiamos la tuberia
void limpiar(int sig)
{
    close(pipefd[0]);
    close(pipefd[1]);
    exit(0);
}

/// @brief Esta funcion es la encargada de detectar anomaliase en usuarios y transacciones en el proceso monohilo monitor
/// @param arg
/// @return
void CrearHilos()
{
    pthread_t anomalia1, anomalia2, anomalia3, anomalia4;

    // Crear los hilos y asignarles las funciones correspondientes
    if (pthread_create(&anomalia1, NULL, detectar_transferencias_consecutivas, NULL) != 0)
    {
        perror("Error al crear el hilo 1");
    }
    if (pthread_create(&anomalia2, NULL, detectar_retiros_consecutivos, NULL) != 0)
    {
        perror("Error al crear el hilo 2");
    }
    if (pthread_create(&anomalia3, NULL, detectar_saldo_negativo, NULL) != 0)
    {
        perror("Error al crear el hilo 3");
    }
    if (pthread_create(&anomalia4, NULL, detectar_transacciones_sospechosas, NULL) != 0)
    {
        perror("Error al crear el hilo 4");
    }

    // Esperar a que los hilos terminen (si es necesario)
    pthread_join(anomalia1, NULL);
    pthread_join(anomalia2, NULL);
    pthread_join(anomalia3, NULL);
    pthread_join(anomalia4, NULL);
}

void *detectar_transferencias_consecutivas(void *arg)
{
    Config config = leer_configuracion("variables.properties");
    Escribir_registro("Detectando transferencias consecutivas...");

    FILE *archivo = fopen(config.archivo_log, "r");
    if (!archivo)
    {
        perror("Error al abrir el archivo de transacciones");
        return NULL;
    }

    char linea[256];
    int transferencias_consecutivas = 0;
    int ultima_cuenta_transferencia = -1;

    while (fgets(linea, sizeof(linea), archivo))
    {
        int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final, saldo_final2;
        char tipo[20];

        if (sscanf(linea, "%d | %39[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final, &saldo_final2) == 8)
        {
            limpiar_cadena(tipo);

            if (strcmp(tipo, "transferencia") == 0)
            {
                if (ultima_cuenta_transferencia == cuenta1)
                {
                    transferencias_consecutivas++;
                }
                else
                {
                    transferencias_consecutivas = 1;
                    ultima_cuenta_transferencia = cuenta1;
                }

                if (transferencias_consecutivas > config.umbral_transferencias)
                {
                    Escribir_registro("Cuenta ha realizado múltiples transferencias consecutivas");
                    enviar_alerta("Actividad sospechosa: múltiples transferencias consecutivas", &id, 0);
                }
            }
        }
    }

    fclose(archivo);
    return NULL;
}

void *detectar_retiros_consecutivos(void *arg)
{
    Config config = leer_configuracion("variables.properties");
    Escribir_registro("Detectando retiros consecutivos...");

    FILE *archivo = fopen(config.archivo_log, "r");
    if (!archivo)
    {
        perror("Error al abrir el archivo de transacciones");
        return NULL;
    }

    char linea[256];
    int retiros_consecutivos = 0;
    int ultima_cuenta_retiro = -1;

    while (fgets(linea, sizeof(linea), archivo))
    {
        int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final, saldo_final2;
        char tipo[20];

        if (sscanf(linea, "%d | %39[^|] | %d | - | %d | - | %d",
                   &id, tipo, &cuenta1, &saldo1, &saldo_final) == 7)
        {
            limpiar_cadena(tipo);

            if (strcmp(tipo, "retiro") == 0)
            {
                if (ultima_cuenta_retiro == cuenta1)
                {
                    retiros_consecutivos++;
                }
                else
                {
                    retiros_consecutivos = 1;
                    ultima_cuenta_retiro = cuenta1;
                }

                if (retiros_consecutivos > config.umbral_retiros)
                {
                    Escribir_registro("Cuenta ha realizado múltiples retiros consecutivos");
                    enviar_alerta("Actividad sospechosa: múltiples retiros consecutivos", &id, 0);
                }
            }
        }
    }

    fclose(archivo);
    return NULL;
}

void *detectar_saldo_negativo(void *arg)
{
    Config config = leer_configuracion("variables.properties");
    Escribir_registro("Detectando saldos negativos...");

    FILE *archivo = fopen(config.archivo_log, "r");
    if (!archivo)
    {
        perror("Error al abrir el archivo de transacciones");
        return NULL;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo))
    {
        int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final, saldo_final2;
        char tipo[20];

        if (sscanf(linea, "%d | %39[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final, &saldo_final2) == 8)
        {
            if (saldo1 < 0 || saldo2 < 0 || saldo_final < 0)
            {
                Escribir_registro("Se ha detectado un saldo negativo");
                enviar_alerta("Saldo negativo detectado", &id, 0);
            }
        }
    }

    fclose(archivo);
    return NULL;
}

void *detectar_transacciones_sospechosas(void *arg)
{
    Config config = leer_configuracion("variables.properties");
    Escribir_registro("Detectando transacciones sospechosas...");

    FILE *archivo = fopen(config.archivo_log, "r");
    if (!archivo)
    {
        perror("Error al abrir el archivo de transacciones");
        return NULL;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo))
    {
        int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final, saldo_final2;
        char tipo[20];

        if (sscanf(linea, "%d | %39[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final, &saldo_final2) == 8)
        {
            // Aquí puedes agregar más validaciones sobre transacciones sospechosas
            if (saldo_final < 0)
            {
                Escribir_registro("Transacción sospechosa con saldo final negativo");
                enviar_alerta("Transacción sospechosa detectada", &id, 0);
            }
        }
    }

    fclose(archivo);
    return NULL;
}

// Función para crear el proceso Monitor y manejar alertas en tiempo real
