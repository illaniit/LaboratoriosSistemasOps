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
};

int obtener_id_usuario(const char *nombre, const char *contrasena) {
    FILE *archivo = fopen("usuarios.txt", "r");
    if (!archivo) {
        perror("Error al abrir usuarios.txt");
        return -1;
    }

    int id, saldo, num_transacciones;
    char nombre_archivo[50], contrasena_archivo[50], apellidos[50], domicilio[100], pais[50];
    
    while (fscanf(archivo, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d\n",
                  &id, nombre_archivo, contrasena_archivo, apellidos, domicilio, pais, &saldo, &num_transacciones) == 8) {
                    limpiar_cadena(nombre_archivo);
                    limpiar_cadena(contrasena_archivo);
        if (strcmp(nombre, nombre_archivo) == 0 && strcmp(contrasena, contrasena_archivo) == 0) {
            fclose(archivo);
            return id;  // Retorna el ID del usuario
        }
    }
    fclose(archivo);
    return -1; // No se encontró el usuario
}



void *Transferencia(void *arg) {
    
    Config config = leer_configuracion("variables.properties");
    struct Usuario *usuario = (struct Usuario *)arg;
   

    int id_destino, Cantidad_transferir;

    // Pedir el ID de la cuenta destino y el monto a transferir
    printf("Ingrese el ID de la cuenta destino: ");
    scanf("%d", &id_destino);
    printf("Ingrese la cantidad a transferir: ");
    scanf("%d", &Cantidad_transferir);

    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    int id_origen = obtener_id_usuario(usuario->nombre, usuario->contrasena);
    
    if(id_origen==id_destino){
        printf("No te puedes hacer una transferencia a ti mismo !!");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }
    if (id_origen == -1) {
        printf("Error: Usuario no encontrado.\n");
        return NULL;
    }
    // Verificar límite de transferencia
    if (Cantidad_transferir > config.limite_transferencia) {
        printf("La cantidad excede el límite de transferencia permitido.\n");
        return NULL;
    }


    // Abrir archivos
    FILE *archivoUsuarios = fopen("usuarios.txt", "r");
    FILE *tempFile = fopen("temp_transacciones.txt", "w");
    if (!archivoUsuarios || !tempFile) {
        perror("Error al abrir los archivos");
        if (archivoUsuarios) 
           fclose(archivoUsuarios);
        if (tempFile) 
           fclose(tempFile);
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    // Variables para procesar usuarios
    char linea[256];
    int Id, Saldo, Num_transacciones;
    char Nombre[50], COntrasena[50], Apellidos[50], domicilio[100], pais[50];
    bool cuenta_origen_encontrada = false, cuenta_destino_encontrada = false;
    int saldo_origen = 0, saldo_destino = 0;

    // Leer archivo de usuarios
    while (fgets(linea, sizeof(linea), archivoUsuarios)) {
        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &Id, Nombre, COntrasena, Apellidos, domicilio, pais, &Saldo, &Num_transacciones) == 8) {
                    
            // Cuenta de orige
            if (Id == id_origen) {
                if (Saldo < Cantidad_transferir) {
                    printf("Saldo insuficiente.\n");
                    sleep(2);
                    fclose(archivoUsuarios);
                    fclose(tempFile);
                    sem_post(sem_usuarios);
                    sem_post(sem_transacciones);
                    return NULL;
                }
                saldo_origen = Saldo; // Guardamos saldo antes de la transferencia
                Saldo -= Cantidad_transferir;
                cuenta_origen_encontrada = true;
            }

            // Cuenta destino
            if (Id == id_destino) {
                saldo_destino = Saldo; // Guardamos saldo antes de la transferencia
                Saldo += Cantidad_transferir;
                cuenta_destino_encontrada = true;
            }

            // Guardar en archivo temporal
            fprintf(tempFile, "%d | %s | %s | %s | %s | %s | %d | %d\n",
                    Id, Nombre, COntrasena, Apellidos, domicilio, pais, Saldo, Num_transacciones);
        }
    }

    fclose(archivoUsuarios);
    fclose(tempFile);
   

    // Verificar si ambas cuentas existen
    if (!cuenta_origen_encontrada || !cuenta_destino_encontrada) {
        printf("Error: Cuenta no encontrada.\n");
        remove("temp.txt");
        return NULL;
    }

    // Reemplazar archivo de usuarios
    remove("usuarios.txt");
    rename("temp_transacciones.txt", "usuarios.txt");

    // Obtener el nuevo ID para la transacción
    int id_transacciones = 0;
    FILE *archivoTransacciones = fopen("transaciones.txt", "r+"); // Abrimos en modo lectura
    if (archivoTransacciones)
    {
        while (fgets(linea, sizeof(linea), archivoTransacciones) != NULL)
        {
            int temp_id;
            if (sscanf(linea, "%d |", &temp_id) == 1)
            {
                id_transacciones = temp_id; // Guardamos el último ID encontrado
            }
        }

    }
    id_transacciones++; // Incrementamos para el nuevo registro

    // Registrar transacción
   
    if (archivoTransacciones) {
        fprintf(archivoTransacciones, "%d | transferencia | %d | %d | %d | %d | %d | %d\n",
                id_transacciones, id_origen, id_destino, saldo_origen, saldo_destino , saldo_origen - Cantidad_transferir,saldo_destino + Cantidad_transferir);
        fclose(archivoTransacciones);
    } else {
        perror("Error al escribir en transacciones.txt");
    }
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    printf("Transferencia realizada con éxito.\n");
    sleep(2);
    return NULL;
    
}
