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
#define MONTO_LIMITE 1000
#define Punt_Archivo_Properties "Variables.properties"
#define MAX_LENGTH 256
#include "init_cuentas.h"
int pipefd[2]; // Tubería

// Prototipos de funciones

void AbrirPropertis();
void CrearMonitor();
void *detectar_transacciones(void *arg);
void enviar_alerta(const char *mensaje);
void limpiar(int sig);
void registrar_alerta(const char *mensaje);

int main()
{

    AbrirPropertis();
    CrearMonitor();
    Menu_Usuario();
    return 0;
}

// Función para abrir y leer el archivo de propiedades

void AbrirPropertis()
{

    char *key, *value;
    char line[MAX_LENGTH];
    char username[MAX_LENGTH] = {0};
    char password[MAX_LENGTH] = {0};

    FILE *ArchivoPro = fopen(Punt_Archivo_Properties, "r");

    if (!ArchivoPro)
    {
        perror("Error al abrir el archivo de propiedades");
        return;
    }

    while (fgets(line, MAX_LENGTH, ArchivoPro))
    {

        line[strcspn(line, "\n")] = 0;
        key = strtok(line, "=");
        value = strtok(NULL, "=");
        if (key && value)
        {
            if (strcmp(key, "username") == 0)
            {
                strncpy(username, value, MAX_LENGTH - 1);
            }
            else if (strcmp(key, "password") == 0)
            {
                strncpy(password, value, MAX_LENGTH - 1);
            }
        }
    }

    fclose(ArchivoPro);
}

// Función para enviar alertas mediante la tubería

void enviar_alerta(const char *mensaje)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s\n", mensaje); // Agregar '\n'
    write(pipefd[1], buffer, strlen(buffer));
}

// Función para manejar la señal SIGINT y limpiar la tubería

void limpiar(int sig)
{
    close(pipefd[0]);
    close(pipefd[1]);
    exit(0);
}

// Función para registrar alertas en un archivo

void registrar_alerta(const char *mensaje)
{

    FILE *archivo_alertas = fopen("alertas.txt", "a");
    if (archivo_alertas)
    {
        fprintf(archivo_alertas, "%s\n", mensaje);
        fclose(archivo_alertas);
    }
    else
    {
        perror("Error al abrir alertas.txt");
    }
}

// Función para detectar transacciones sospechosas

void *detectar_transacciones(void *arg)
{

    signal(SIGINT, limpiar);

    FILE *archivo = fopen("transaciones.txt", "r");

    if (!archivo)
    {

        perror("Error al abrir el archivo de transacciones");

        return NULL;
    }

    char linea[256];

    while (fgets(linea, sizeof(linea), archivo))
    {

        int id, saldo1, saldo2, saldo_final;

        char tipo[20], cuenta1[20], cuenta2[20];

        if (sscanf(linea, "%d,%19[^,],%19[^,],%19[^,],%d,%d,%d", &id, tipo, cuenta1, cuenta2, &saldo1, &saldo2, &saldo_final) == 7)
        {

            if (strcmp(tipo, "retiro") == 0 || strcmp(tipo, "ingreso") == 0)
            {

                if (strlen(cuenta2) > 0 || saldo2 != 0)
                {

                    enviar_alerta("Transacción inválida, campos incorrectos en retiro o ingreso");
                }
            }

            if (saldo1 < 0 || saldo2 < 0 || saldo_final < 0)
            {

                enviar_alerta("Saldo negativo detectado");
            }
        }
    }

    fclose(archivo);

    // Verificación de usuarios.dat

    FILE *usuarios = fopen("usuarios.dat", "r");

    if (!usuarios)
    {

        perror("Error al abrir el archivo de usuarios");

        return NULL;
    }

    while (fgets(linea, sizeof(linea), usuarios))
    {

        int id, saldo, num_transacciones;

        char titular[50];

        if (sscanf(linea, "%d,%49[^,],%d,%d", &id, titular, &saldo, &num_transacciones) == 4)
        {

            if (saldo < 0)
            {

                enviar_alerta("Usuario con saldo negativo");
            }

            if (num_transacciones < 0)
            {

                enviar_alerta("Número de transacciones inválido");
            }
        }
    }

    fclose(usuarios);

    return NULL;
}

// Función para crear el proceso Monitor y manejar alertas en tiempo real

void CrearMonitor()
{

    if (pipe(pipefd) == -1)
    {

        perror("Error al crear la tubería");

        exit(1);
    }

    pid_t Monitor = fork();

    if (Monitor < 0)
    {

        perror("Error al crear el proceso Monitor");

        return;
    }

    if (Monitor == 0)
    {

        // Código del hijo (Monitor)

        close(pipefd[0]); // Cierra lectura en el hijo

        pthread_t Transacciones;

        if (pthread_create(&Transacciones, NULL, detectar_transacciones, NULL) != 0)
        {

            perror("Error al crear el hilo de transacciones");

            exit(1);
        }

        pthread_join(Transacciones, NULL);

        close(pipefd[1]); // Cerrar escritura cuando termine

        exit(0);
    }

    else
    {

        // Código del padre (lector de alertas y almacenamiento en archivo)

        close(pipefd[1]); // Cierra escritura en el padre

        char buffer[256];

        int bytes_leidos;

        printf("⚠ Monitoreo de alertas iniciado... (Guardando en alertas.txt)\n");

        while (1)
        {

            bytes_leidos = read(pipefd[0], buffer, sizeof(buffer) - 1);

            if (bytes_leidos > 0)
            {

                buffer[bytes_leidos] = '\0'; // Convertir a string

                registrar_alerta(buffer);
            }
            else if (bytes_leidos == 0)
            {

                break; // Salir si no hay más datos
            }
            else
            {

                perror("Error al leer la tubería");

                break;
            }
        }

        close(pipefd[0]); // Cierra lectura al terminar
    }
}
