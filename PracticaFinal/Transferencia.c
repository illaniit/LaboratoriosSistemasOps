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

struct Usuario {
    char nombre[50];
    char contrasena[50];
    int saldo;
};

void *Transferencia(void *arg) {
    sem_t semaforo4; // Inicializamos el semáforo para manejar la concurrencia
    sem_init(&semaforo4, 0, 1);
    sem_wait(&semaforo4); // Esperamos a que el semáforo esté disponible

    // Leemos la configuración
    Config config = leer_configuracion("variables.properties");

    struct Usuario *usuario = (struct Usuario *)arg; // Convertimos el argumento a un puntero de tipo Usuario
    char cuenta_destino[50]; // Almacena el nombre de la cuenta destino
    int monto_transferir; // Almacena el monto a transferir

    // Solicitamos al usuario la cuenta destino y el monto a transferir
    printf("Ingrese la cuenta destino: ");
    scanf("%s", cuenta_destino);
    printf("Ingrese el monto a transferir: ");
    scanf("%d", &monto_transferir);

    // Verificamos si el monto a transferir excede el límite permitido
    if (monto_transferir > LIMITE_TRANSFERENCIA) {
        printf("El monto excede el límite de transferencia permitido.\n");
        sem_post(&semaforo4);
        return NULL;
    }

    // Abrimos los archivos necesarios para leer y escribir los datos de los usuarios
    FILE *archivoUsuarios = fopen("usuarios.txt", "r");
    FILE *tempFile = fopen("temp.txt", "w");
    if (!archivoUsuarios || !tempFile) {
        perror("Error al abrir los archivos");
        sem_post(&semaforo4);
        return NULL;
    }

    char linea[256]; // Buffer para leer cada línea del archivo
    int id, saldo, num_transacciones; // Variables para almacenar los datos de cada usuario
    char nombre[50], contrasena[50], apellidos[50], domicilio[100], pais[50]; // Variables para almacenar los datos de cada usuario
    bool cuenta_origen_encontrada = false, cuenta_destino_encontrada = false; // Banderas para verificar si las cuentas fueron encontradas

    // Leemos cada línea del archivo de usuarios
    while (fgets(linea, sizeof(linea), archivoUsuarios)) {
        // Extraemos los datos de cada línea
        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &id, nombre, contrasena, apellidos, domicilio, pais, &saldo, &num_transacciones) == 8) {
            // Verificamos si la cuenta de origen coincide con el usuario actual
            if (strcmp(nombre, usuario->nombre) == 0 && strcmp(contrasena, usuario->contrasena) == 0) {
                // Verificamos si hay saldo suficiente para la transferencia
                if (saldo < monto_transferir) {
                    printf("Saldo insuficiente para realizar la transferencia.\n");
                    fclose(archivoUsuarios);
                    fclose(tempFile);
                    sem_post(&semaforo4);
                    return NULL;
                }
                saldo -= monto_transferir; // Descontamos el monto del saldo de la cuenta de origen
                cuenta_origen_encontrada = true; // Marcamos que la cuenta de origen fue encontrada
            }
            // Verificamos si la cuenta destino coincide
            if (strcmp(nombre, cuenta_destino) == 0) {
                saldo += monto_transferir; // Sumamos el monto al saldo de la cuenta destino
                cuenta_destino_encontrada = true; // Marcamos que la cuenta destino fue encontrada
            }
            // Escribimos los datos actualizados en el archivo temporal
            fprintf(tempFile, "%d | %s | %s | %s | %s | %s | %d | %d\n",
                    id, nombre, contrasena, apellidos, domicilio, pais, saldo, num_transacciones);
        }
    }

    fclose(archivoUsuarios); // Cerramos el archivo de usuarios
    fclose(tempFile); // Cerramos el archivo temporal

    // Verificamos si ambas cuentas fueron encontradas
    if (!cuenta_origen_encontrada || !cuenta_destino_encontrada) {
        printf("Error: Una de las cuentas no fue encontrada.\n");
        remove("temp.txt"); // Eliminamos el archivo temporal
        sem_post(&semaforo4);
        return NULL;
    }

    // Reemplazamos el archivo de usuarios original con el archivo temporal actualizado
    remove("usuarios.txt");
    rename("temp.txt", "usuarios.txt");

    // Registramos la transacción en el archivo de transacciones
    FILE *archivoTransacciones = fopen("transaciones.txt", "a");
    if (archivoTransacciones) {
        fprintf(archivoTransacciones, "%d | transferencia | %s | %s | %d\n",
                id, usuario->nombre, cuenta_destino, monto_transferir);
        fclose(archivoTransacciones);
    }

    printf("Transferencia realizada con éxito.\n"); // Mensaje de éxito
    sem_post(&semaforo4); // Liberamos el semáforo
    sem_destroy(&semaforo4); // Destruimos el semáforo
    return NULL; // Finalizamos la función
}