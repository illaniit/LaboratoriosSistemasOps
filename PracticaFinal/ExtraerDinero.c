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
#include "Operaciones.h"
#include <stdbool.h>
void limpiar_cadena2(char *cadena);
// Introduzco un struct para que me pida la contraseña a la hora de introducir un usuario :) //
struct Usuario4
{
    char Usuario3[50];
    char Contraseña1[50];
} Usuario4;

void limpiar_cadena3(char *cadena)
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

void *ExtraerDinero(void *arg3) {
    sem_t semaforo_extraer;
    sem_init(&semaforo_extraer, 0, 1);
    sem_wait(&semaforo_extraer);

    struct Usuario4 *usuario = (struct Usuario4 *)arg3;
    bool encontrado = false;

    FILE *ArchivoUsuarios = fopen("usuarios.txt", "r");
    if (!ArchivoUsuarios) {
        perror("Error al abrir el archivo");
        sem_post(&semaforo_extraer);
        return NULL;
    }

    FILE *tempFile1 = fopen("temp1.txt", "w");
    if (!tempFile1) {
        perror("Error al abrir el archivo temporal");
        fclose(ArchivoUsuarios);
        sem_post(&semaforo_extraer);
        return NULL;
    }

    int id3, saldo3, num_transacciones3;
    char nombre3[50], contrasena3[50], apellidos3[50], domicilio3[100], pais3[50];
    char linea[200], linea2[200];

    int id_transacciones = 0;
    FILE *ArchivoTransacciones = fopen("transaciones.txt", "r");
    if (ArchivoTransacciones) {
        while (fgets(linea2, sizeof(linea2), ArchivoTransacciones) != NULL) {
            int temp_id;
            if (sscanf(linea2, "%d |", &temp_id) == 1) {
                id_transacciones = temp_id; // Guardamos el último ID encontrado
            }
        }
        fclose(ArchivoTransacciones);
    }
    id_transacciones++; // Incrementamos para el nuevo registro

    while (fgets(linea, sizeof(linea), ArchivoUsuarios) != NULL) {
        linea[strcspn(linea, "\n")] = '\0';

        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &id3, nombre3, contrasena3, apellidos3, domicilio3, pais3, &saldo3, &num_transacciones3) == 8) {
            limpiar_cadena2(nombre3);
            limpiar_cadena2(contrasena3);
            if (strcmp(nombre3, usuario->Usuario3) == 0 && strcmp(contrasena3, usuario->Contraseña1) == 0) {
                int dinero_inicial = saldo3;
                encontrado = true;
                int saldo_extraer;
                printf("Introduzca la cantidad que desea extraer: ");
                scanf("%d", &saldo_extraer);
                
                if (saldo_extraer > saldo3) {
                    printf("Saldo insuficiente.\n");
                    fclose(tempFile1);
                    fclose(ArchivoUsuarios);
                    remove("temp1.txt");
                    sem_post(&semaforo_extraer);
                    return NULL;
                }

                saldo3 -= saldo_extraer;
                num_transacciones3++;

                ArchivoTransacciones = fopen("transaciones.txt", "a");
                if (ArchivoTransacciones) {
                    fprintf(ArchivoTransacciones, "%d | retiro | %s | - | %d | - | %d \n", id_transacciones, nombre3, dinero_inicial, saldo3);
                    fclose(ArchivoTransacciones);
                }
            }
        }
        fprintf(tempFile1, "%d | %s | %s | %s | %s | %s | %d | %d\n",
                id3, nombre3, contrasena3, apellidos3, domicilio3, pais3, saldo3, num_transacciones3);
    }

    fclose(ArchivoUsuarios);
    fclose(tempFile1);

    if (encontrado) {
        remove("usuarios.txt");
        rename("temp1.txt", "usuarios.txt");
        printf("Saldo actualizado correctamente.\n");
        sleep(2);
    } else {
        remove("temp1.txt");
        printf("Usuario no encontrado o contraseña incorrecta.\n");
    }

    sem_post(&semaforo_extraer);
    return NULL;
}