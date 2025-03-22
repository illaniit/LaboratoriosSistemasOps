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

struct Usuario3 {
    char Usuario2[50];
    char Contraseña1[50];
};

void limpiar_cadena2(char *cadena) {
    int inicio = 0;
    int fin = strlen(cadena) - 1;
    while (cadena[inicio] == ' ' || cadena[inicio] == '\t') {
        inicio++;
    }
    while (fin >= inicio && (cadena[fin] == ' ' || cadena[fin] == '\t')) {
        fin--;
    }
    for (int i = 0; i <= fin - inicio; i++) {
        cadena[i] = cadena[inicio + i];
    }
    cadena[fin - inicio + 1] = '\0';
}

void *IntroducirDinero(void *arg2) {
    sem_t semaforo3;
    sem_init(&semaforo3, 0, 1);
    sem_wait(&semaforo3);

    struct Usuario3 *usuario = (struct Usuario3 *)arg2;
    bool encontrado = false;

    FILE *ArchivoUsuarios = fopen("usuarios.txt", "r");
    if (!ArchivoUsuarios) {
        perror("Error al abrir el archivo");
        sem_post(&semaforo3);
        return NULL;
    }
    FILE *tempFile = fopen("temp.txt", "w");
    if (!tempFile) {
        perror("Error al abrir el archivo temporal");
        fclose(ArchivoUsuarios);
        sem_post(&semaforo3);
        return NULL;
    }

    int id2, saldo2, num_transacciones2;
    char nombre2[50], contrasena2[50], apellidos2[50], domicilio2[100], pais2[50];
    char linea[200], linea2[200];

    int id_transacciones = 0;
    FILE *ArchivoTransacciones = fopen("transaciones.txt", "r"); // Abrimos en modo lectura
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
                   &id2, nombre2, contrasena2, apellidos2, domicilio2, pais2, &saldo2, &num_transacciones2) == 8) {
            limpiar_cadena2(nombre2);
            limpiar_cadena2(contrasena2);
            if (strcmp(nombre2, usuario->Usuario2) == 0 && strcmp(contrasena2, usuario->Contraseña1) == 0) {
                int dinero_inicial = saldo2;
                encontrado = true;
                int saldo_introducir;
                printf("Introduzca la cantidad que desea ingresar: ");
                scanf("%d", &saldo_introducir);
                saldo2 += saldo_introducir;
                num_transacciones2++;
                
                ArchivoTransacciones = fopen("transaciones.txt", "a");
                if (ArchivoTransacciones) {
                    fprintf(ArchivoTransacciones, "%d | ingreso | %s | - | %d | - | %d \n", id_transacciones, nombre2, dinero_inicial, saldo2);
                    fclose(ArchivoTransacciones);
                }
            }
        }
        fprintf(tempFile, "%d | %s | %s | %s | %s | %s | %d | %d\n",
                id2, nombre2, contrasena2, apellidos2, domicilio2, pais2, saldo2, num_transacciones2);
    }

    fclose(ArchivoUsuarios);
    fclose(tempFile);

    if (encontrado) {
        remove("usuarios.txt");
        rename("temp.txt", "usuarios.txt");
        
        printf("Saldo actualizado correctamente.\n");
        sleep(2);
    } else {
        remove("temp.txt");
        printf("Usuario no encontrado o contraseña incorrecta.\n");
    }

    sem_post(&semaforo3);
    return NULL;
}
