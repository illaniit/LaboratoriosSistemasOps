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
#include <stdbool.h>
#include "Operaciones.h"
void DatosCuenta(char *user);
void ConsultarTransferencias(char *user);
void limpiar_cadena1(char *cadena);


void limpiar_cadena1(char* cadena) {
    int inicio = 0;
    int fin = strlen(cadena) - 1;

    // Eliminar los espacios al principio
    while (cadena[inicio] == ' ' || cadena[inicio] == '\t') {
        inicio++;
    }

    // Eliminar los espacios al final
    while (fin >= inicio && (cadena[fin] == ' ' || cadena[fin] == '\t')) {
        fin--;
    }

    // Mover la cadena limpia
    for (int i = 0; i <= fin - inicio; i++) {
        cadena[i] = cadena[inicio + i];
    }
    cadena[fin - inicio + 1] = '\0'; // Añadir el carácter nulo al final
}

void *ConsultarDatos(void *arg) {
    sem_t s1;
    sem_init(&s1,0,1);
    sem_wait(&s1);
    
    char *user = (char *)arg;
    int Eleccion = 0;

    do {
        system("clear");
        printf("\n---------- Menu de consulta de datos ----------\n");
        printf("1. Datos de tu cuenta\n");
        printf("2. Consultar las transferencias\n");
        printf("3. Volver al menu\n");
        printf("Introduce tu elección: ");
        
        if (scanf("%d", &Eleccion) != 1) {
            printf("Entrada inválida. Inténtalo de nuevo.\n");
            while (getchar() != '\n'); // Limpiar buffer de entrada
            continue;
        }

        switch (Eleccion) {
            case 1:
                DatosCuenta(user);
                
                break;
            case 2:
                ConsultarTransferencias(user);
                break;
            case 3:
                printf("Volviendo al menú...\n");
                sleep(2);
                break;
            default:
                printf("Opción no válida, intenta de nuevo.\n");
        }
    } while (Eleccion != 3);
  sem_post(&s1);
    return NULL;
    
}

void DatosCuenta(char *user) {
    system("clear");
    bool encontrado=false;
    char var[100];
    FILE *archivoCuentas = fopen("usuarios.txt", "r");
    if (!archivoCuentas) {
        perror("Error al abrir usuario.txt");
        return;
    }
      limpiar_cadena1(user);
    char linea[256];
    int id1, saldo1, num_transacciones1;
    char nombre1[50], contrasena1[50], apellidos1[50], domicilio1[100], pais1[50];
    while (fgets(linea, sizeof(linea), archivoCuentas)) {
        linea[strcspn(linea, "\n")] = '\0';

       

        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &id1, nombre1, contrasena1, apellidos1, domicilio1, pais1, &saldo1, &num_transacciones1) == 8) {
                    limpiar_cadena1(nombre1);
            if (strcmp(nombre1, user) == 0) {
            encontrado=true;
                break;
            }
        }
    }
    if(encontrado){
    printf("\n------ Datos de la cuenta ------\n");
                printf("ID: %d\n", id1);
                printf("Nombre: %s\n", nombre1);
                printf("Apellidos: %s\n", apellidos1);
                printf("Domicilio: %s\n", domicilio1);
                printf("Pais: %s\n", pais1);
                printf("Saldo: %d\n", saldo1);
                printf("Número de Transacciones: %d\n", num_transacciones1);
                printf("Presione s  para volver al menu....");
                scanf("%s",var);
            
    }
    fclose(archivoCuentas);
}

void ConsultarTransferencias(char *user) {
    char var[20];
    system("clear");
    FILE *archivoTransacciones = fopen("transaciones.txt", "r");
    if (!archivoTransacciones) {
        perror("Error al abrir transacciones.txt");
        return;
    }

    char linea[256];
    printf("\n------ Transferencias ------\n");
    while (fgets(linea, sizeof(linea), archivoTransacciones)) {
        int id, saldo1, saldo2, saldofinal;
        char tipo[20], cuenta1[50], cuenta2[50];

        if (sscanf(linea, "%d , %19[^,] , %49[^,] , %49[^,] , %d , %d , %d",
                   &id, tipo, cuenta1, cuenta2, &saldo1, &saldo2, &saldofinal) == 7) {
            limpiar_cadena1(cuenta1);
            limpiar_cadena1(cuenta2);
            if (strcmp(cuenta1, user) == 0 || strcmp(cuenta2, user) == 0) {
                printf("ID: %d | Tipo: %s | De: %s | A: %s | Saldo antes: %d | Saldo después: %d\n",
                       id, tipo, cuenta1, cuenta2, saldo1, saldofinal);
            
            }
        }
        
    }
    printf("Presione s  para volver al menu....");
    scanf("%s",var);
    fclose(archivoTransacciones);
}
