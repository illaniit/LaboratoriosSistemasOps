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
//Utilizamos este struct para almacenar los datos 
struct Usuario {
    char nombre[50];
    char contrasena[50];
};

/// @brief 
/// @param arg 
/// @return 
void *Transferencia(void *arg) {
    // Cargar configuración
    Config config = leer_configuracion("variables.properties");
    struct Usuario *usuario = (struct Usuario *)arg;

    int id_destino, Cantidad_transferir;

    // Mostrar menú interactivo
    printf("\n-------------------------------------------------\n");
    printf("            💸 ¡Bienvenido a Transferencias! 💸\n");
    printf("-------------------------------------------------\n");

    // Solicitar el ID de la cuenta destino
    printf("\n🔑 Ingrese el ID de la cuenta destino: ");
    scanf("%d", &id_destino);

    // Solicitar la cantidad a transferir
    printf("\n💰 Ingrese la cantidad a transferir: ");
    scanf("%d", &Cantidad_transferir);

    // Bloquear semáforos para evitar concurrencia
    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    int id_origen = obtener_id_usuario(usuario->nombre, usuario->contrasena);

    if(id_origen == id_destino) {
        printf("\n❌ ¡No puedes transferir dinero a ti mismo!\n");
        sleep(2);
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    if (id_origen == -1) {
        printf("\n❌ Error: Usuario no encontrado.\n");
        sleep(2);
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    // Verificar límite de transferencia
    if (Cantidad_transferir > config.limite_transferencia) {
        printf("\n❌ La cantidad excede el límite de transferencia permitido. El límite es de $%d\n", config.limite_transferencia);
        sleep(2);
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    // Abrir archivos de usuarios y transacciones
    FILE *archivoUsuarios = fopen("usuarios.txt", "r");
    FILE *tempFile = fopen("temp_transaciones.txt", "w");

    if (!archivoUsuarios || !tempFile) {
        perror("\n❌ Error al abrir los archivos");
        if (archivoUsuarios) fclose(archivoUsuarios);
        if (tempFile) fclose(tempFile);
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

    // Leer el archivo de usuarios
    while (fgets(linea, sizeof(linea), archivoUsuarios)) {
        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &Id, Nombre, COntrasena, Apellidos, domicilio, pais, &Saldo, &Num_transacciones) == 8) {

            // Procesar cuenta de origen
            if (Id == id_origen) {
                if (Saldo < Cantidad_transferir) {
                    printf("\n❌ Saldo insuficiente para realizar la transferencia.\n");
                    sleep(2);
                    fclose(archivoUsuarios);
                    fclose(tempFile);
                    sem_post(sem_usuarios);
                    sem_post(sem_transacciones);
                    return NULL;
                }
                saldo_origen = Saldo;
                Saldo -= Cantidad_transferir;
                cuenta_origen_encontrada = true;
            }

            // Procesar cuenta destino
            if (Id == id_destino) {
                saldo_destino = Saldo;
                Saldo += Cantidad_transferir;
                cuenta_destino_encontrada = true;
            }

            // Guardar cambios en archivo temporal
            fprintf(tempFile, "%d | %s | %s | %s | %s | %s | %d | %d\n",
                    Id, Nombre, COntrasena, Apellidos, domicilio, pais, Saldo, Num_transacciones);
        }
    }

    fclose(archivoUsuarios);
    fclose(tempFile);

    // Verificar si ambas cuentas fueron encontradas
    if (!cuenta_origen_encontrada || !cuenta_destino_encontrada) {
        printf("\n❌ Error: Una o ambas cuentas no fueron encontradas.\n");
        remove("temp_transacciones.txt");
        sleep(2);
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    // Reemplazar archivo de usuarios con los nuevos datos
    remove("usuarios.txt");
    rename("temp_transaciones.txt", "usuarios.txt");

    // Generar un nuevo ID para la transacción
    int id_transacciones = 0;
    FILE *archivoTransacciones = fopen("transaciones.txt", "r+");
    if (archivoTransacciones) {
        while (fgets(linea, sizeof(linea), archivoTransacciones)) {
            int temp_id;
            if (sscanf(linea, "%d |", &temp_id) == 1) {
                id_transacciones = temp_id;
            }
        }
    }
    id_transacciones++; // Incrementar el ID para la nueva transacción

    // Registrar la transacción
    if (archivoTransacciones) {
        fprintf(archivoTransacciones, "%d | Transferencia | %d | %d | %d | %d | %d | %d\n",
                id_transacciones, id_origen, id_destino, saldo_origen, saldo_destino,
                saldo_origen - Cantidad_transferir, saldo_destino + Cantidad_transferir);
        fclose(archivoTransacciones);
    } else {
        perror("\n❌ Error al registrar la transacción");
    }

    // Liberar semáforos
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    // Confirmar transferencia
    printf("\n🎉 ¡Transferencia realizada con éxito! 🎉\n");
    sleep(2);
    return NULL;
}
