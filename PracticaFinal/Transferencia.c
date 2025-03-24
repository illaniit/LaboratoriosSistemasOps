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
#include "Comun.h"

struct Usuario {
    char nombre[50];
    char contrasena[50];
    int saldo;
};

void *Transferencia(void *arg) {
   

    // Leemos la configuración
    Config config = leer_configuracion("variables.properties");
    struct Usuario *usuario = (struct Usuario *)arg; // Convertimos el argumento a un puntero de tipo Usuario
    char cuenta_destino[50]; // Almacena el nombre de la cuenta destino
    int Cantidad_transferir; // Almacena el monto a transferir

    // Solicitamos al usuario la cuenta destino y el monto a transferir
    printf("Ingrese la cuenta destino: "); // pedir el id
    scanf("%s", cuenta_destino);
    printf("Ingrese el monto a transferir: ");
    scanf("%d", &Cantidad_transferir);

    // Verificamos si el monto a transferir excede el límite permitido
    if (Cantidad_transferir > config.limite_transferencia) {
        printf("El monto excede el límite de transferencia permitido.\n");
     
        return NULL;
    }

    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);
    // Abrimos los archivos necesarios para leer y escribir los datos de los usuarios
    FILE *archivoUsuarios = fopen("usuarios.txt", "r");
    FILE *tempFile3 = fopen("temp3.txt", "w");
    if (!archivoUsuarios || !tempFile3) {
        perror("Error al abrir los archivos");
        
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
                if (saldo < Cantidad_transferir) {
                    printf("Saldo insuficiente para realizar la transferencia.\n");
                    fclose(archivoUsuarios);
                    fclose(tempFile3);
                    
                    return NULL;
                }
                saldo -= Cantidad_transferir; // Descontamos el monto del saldo de la cuenta de origen
                cuenta_origen_encontrada = true;
                
                 // Marcamos que la cuenta de origen fue encontrada
            }
            // Verificamos si la cuenta destino coincide
            if (strcmp(nombre, cuenta_destino) == 0) {
                saldo += Cantidad_transferir; // Sumamos el monto al saldo de la cuenta destino
                cuenta_destino_encontrada = true; // Marcamos que la cuenta destino fue encontrada
                
            }
            
            // Escribimos los datos actualizados en el archivo temporal
            fprintf(tempFile3, "%d | %s | %s | %s | %s | %s | %d | %d\n",
                    id, nombre, contrasena, apellidos, domicilio, pais, saldo, num_transacciones);
        }
    }

    fclose(archivoUsuarios); // Cerramos el archivo de usuarios
    fclose(tempFile3); // Cerramos el archivo temporal
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    // Verificamos si ambas cuentas fueron encontradas
    if (!cuenta_origen_encontrada || !cuenta_destino_encontrada) {
        printf("Error: Una de las cuentas no fue encontrada.\n");
        remove("temp3.txt"); // Eliminamos el archivo temporal
   
        return NULL;
    }

    // Reemplazamos el archivo de usuarios original con el archivo temporal actualizado
    remove("usuarios.txt");
    rename("temp3.txt", "usuarios.txt");

    // Registramos la transacción en el archivo de transacciones
   // FILE *archivoTransacciones = fopen("transaciones.txt", "a");
  //  if (archivoTransacciones) {
     //   fprintf(archivoTransacciones, "%d | transferencia | %s | %s | | %d \n",
     //           id, usuario->nombre, cuenta_destino, Cantidad_transferir);
    //    fclose(archivoTransacciones);
    //}
//
    printf("Transferencia realizada con éxito.\n"); // Mensaje de éxito
   
    return NULL; // Finalizamos la función
}