#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> // Para fork()
#define Punt_Archivo_Properties "Variables.properties"
#define MAX_LENGTH 256
#include "init_cuentas.h"
// Prototipos de funciones
void AbrirPropertis();
void CrearMonitor();
int main()
{
    AbrirPropertis();
    CrearMonitor(); 
    Menu_Usuario();
    return 0;
}

void AbrirPropertis()
{
    char *key, *value;
    char line[MAX_LENGTH];
    char username[MAX_LENGTH] = {0}; // Inicializar con ceros
    char password[MAX_LENGTH] = {0};
    FILE *ArchivoPro = fopen(Punt_Archivo_Properties, "r");

    if (!ArchivoPro)
    { // Validar si el archivo se abrió correctamente
        perror("Error al abrir el archivo de propiedades");
        return;
    }

    while (fgets(line, MAX_LENGTH, ArchivoPro))
    {
        line[strcspn(line, "\n")] = 0; // Eliminar salto de línea

        key = strtok(line, "=");
        value = strtok(NULL, "=");

        if (key && value)
        { // Validar que la línea sea válida
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

void *hilo_transacciones(void *arg)
{
    FILE *archivo = fopen("transaciones.txt", "r");
    if (archivo == NULL)
    {
        perror("Error al abrir el archivo");
        return NULL;
    }

    char linea[256];
    int ids[100]; // Suponiendo un máximo de 100 transacciones que cumplan la condición
    int contador = 0;

    while (fgets(linea, sizeof(linea), archivo))
    {
        int id;
        char tipo[20];
        int cuenta1, cuenta2;
        double saldo1, saldo2, dinero_transaccion;

        // Parsear la línea del CSV
        if (sscanf(linea, "%d,%19[^,],%d,%d,%lf,%lf,%lf", &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &dinero_transaccion) == 7)
        {
            // Verificar las condiciones
            if (dinero_transaccion > 5000 || saldo1 < 0 || saldo2 < 0)
            {
                ids[contador++] = id;
                // escribir en el .log la anomalia 
            }
        }
    }

    fclose(archivo);

    return NULL;
}

void CrearMonitor()
{
    pid_t Monitor = fork(); // Crear un nuevo proceso hijo

    if (Monitor < 0)
    {
        perror("Error al crear el proceso Monitor");
        return;
    }

    if (Monitor == 0)
    { // Código del hijo esto es como tal el monitor
        pthread_t Transacciones;
        if (pthread_create(&Transacciones, NULL, hilo_transacciones, NULL) != 0)
        {
            perror("Error al crear el hilo de transacciones");
            exit(1);
        }

        pthread_join(Transacciones, NULL); // Esperar a que termine el hilo
        exit(0);                           // Terminar el proceso hijo correctamente
    }
}
