
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "Comun.h"

volatile sig_atomic_t corriendo = 1;

void manejador_salida(int sig)
{
    corriendo = 0;
}

void *detectar_transacciones_sospechosas(void *arg);
void *detectar_transferencias_consecutivas(void *arg);
void *detectar_retiros_consecutivos(void *arg);
void *detectar_saldo_negativo(void *arg);

int main()
{
    signal(SIGINT, manejador_salida);
    signal(SIGTERM, manejador_salida);

    Inicializar_semaforos();
    pthread_t h1, h2, h3, h4;
    while (corriendo)
    {
       
        pthread_create(&h1, NULL, detectar_transferencias_consecutivas, NULL);
        pthread_create(&h2, NULL, detectar_retiros_consecutivos, NULL);
        pthread_create(&h3, NULL, detectar_saldo_negativo, NULL);
        pthread_create(&h4, NULL, detectar_transacciones_sospechosas, NULL);

        

        pthread_join(h1, NULL);
        pthread_join(h2, NULL);
        pthread_join(h3, NULL);
        pthread_join(h4, NULL);

        sleep(200);
    }
    Destruir_semaforos();
    return 0;
}

void enviar_alerta(const char *mensaje, const int *id, const int titular)
{
    MensajeAlerta msg;
    msg.mtype = TIPO_ALERTA;

    time_t t;
    struct tm *tm_info;
    char hora[30];
    time(&t);
    tm_info = localtime(&t);
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);

    snprintf(msg.texto, sizeof(msg.texto),
             "[%s] Id de la transacci\xC3\xB3n: %d | %s\n",
             hora, *id, mensaje);

    int id_cola = msgget(CLAVE_COLA, 0666);
    if (id_cola == -1)
    {
        perror("Error al obtener cola");
        return;
    }

    if (msgsnd(id_cola, &msg, sizeof(msg.texto), 0) == -1)
    {
        perror("Error al enviar mensaje a la cola");
    }

    kill(getppid(), SIGUSR1);
}

void *detectar_transferencias_consecutivas(void *arg)
{
  
        Escribir_registro("Detectando transferencias consecutivas...");
        FILE *archivo = fopen("transaciones.txt", "r");
        if (!archivo)
        {
            perror("Error al abrir transaciones.txt");
            sleep(5);
            
        }

        char linea[256];
        int transferencias_consecutivas = 0, ultima_cuenta = -1;

        while (fgets(linea, sizeof(linea), archivo))
        {
            int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final1, saldo_final2;
            char tipo[20];

            if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                       &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final1, &saldo_final2) == 8)
            {
                limpiar_cadena(tipo);
                if (strcmp(tipo, "transferencia") == 0)
                {
                    if (cuenta1 == ultima_cuenta)
                        transferencias_consecutivas++;
                    else
                    {
                        transferencias_consecutivas = 1;
                        ultima_cuenta = cuenta1;
                    }
                    if (transferencias_consecutivas > 4)
                    {
                        Escribir_registro("Transferencias consecutivas detectadas");
                        enviar_alerta("M\xC3\xBAltiples transferencias consecutivas", &id, 0);
                    }
                }
            }
        
        fclose(archivo);
        sleep(10);
    }
    pthread_exit(NULL);
}

void *detectar_retiros_consecutivos(void *arg)
{
   
        Escribir_registro("Detectando retiros consecutivos...");
        FILE *archivo = fopen("transaciones.txt", "r");
        if (!archivo)
        {
            perror("Error al abrir transaciones.txt");
            sleep(5);
           
        }

        char linea[256];
        int retiros = 0, ultima_cuenta = -1;

        while (fgets(linea, sizeof(linea), archivo))
        {
            int id, cuenta1, saldo1, saldo_final;
            char tipo[20];

            if (sscanf(linea, "%d | %19[^|] | %d | - | %d | - | %d",
                       &id, tipo, &cuenta1, &saldo1, &saldo_final) == 5)
            {
                limpiar_cadena(tipo);
                if (strcmp(tipo, "retiro") == 0)
                {
                    if (cuenta1 == ultima_cuenta)
                        retiros++;
                    else
                    {
                        retiros = 1;
                        ultima_cuenta = cuenta1;
                    }
                    if (retiros > 3)
                    {
                        Escribir_registro("Retiros consecutivos detectados");
                        enviar_alerta("M\xC3\xBAltiples retiros consecutivos", &id, 0);
                    }
                }
            }
        }
        fclose(archivo);
        sleep(10);
    
    pthread_exit(NULL);
}

void *detectar_saldo_negativo(void *arg)
{
   
        Escribir_registro("Detectando saldos negativos...");
        FILE *archivo = fopen("transaciones.txt", "r");
        if (!archivo)
        {
            perror("Error al abrir transaciones.txt");
            sleep(5);
            
        }

        char linea[256];
        while (fgets(linea, sizeof(linea), archivo))
        {
            int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final1, saldo_final2;
            char tipo[20];

            if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                       &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final1, &saldo_final2) == 8)
            {
                if (saldo1 < 0 || saldo2 < 0 || saldo_final1 < 0)
                {
                    Escribir_registro("Saldo negativo detectado");
                    enviar_alerta("Saldo negativo", &id, 0);
                }
            }
        }
        fclose(archivo);
        sleep(10);
    
    pthread_exit(NULL);
}

void *detectar_transacciones_sospechosas(void *arg)
{
   
        Escribir_registro("Detectando transacciones sospechosas...");
        FILE *archivo = fopen("transaciones.txt", "r");
        if (!archivo)
        {
            perror("Error al abrir transaciones.txt");
            sleep(5);
           
        }

        char linea[256];
        while (fgets(linea, sizeof(linea), archivo))
        {
            int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final1, saldo_final2;
            char tipo[20];

            if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                       &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final1, &saldo_final2) == 8)
            {
                if (saldo_final1 < 0)
                {
                    Escribir_registro("Transacci\xC3\xB3n sospechosa detectada");
                    enviar_alerta("Transacci\xC3\xB3n sospechosa", &id, 0);
                }
            }
        }
        fclose(archivo);
        sleep(10);
    
    pthread_exit(NULL);
}
