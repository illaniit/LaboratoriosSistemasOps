#include <stdbool.h>
#include "Operaciones.h"
#include "Comun.h"
#include "Cuenta.h"

// Introduzco un struct para que me pida la contraseña a la hora de introducir un usuario :) //
struct Usuario4
{
    char Usuario3[50];
    char Contraseña1[50];
} Usuario4;

/// @brief esta funcion permite extraer dinero y lo actualiza
/// @param arg3 
/// @return no devuelebe nada, es de tipo void actualiza 
void *ExtraerDinero(void *arg3)
{
    struct Usuario4 *usuario = (struct Usuario4 *)arg3;
    Config config = leer_configuracion("variables.properties");
    int saldo_extraer;
    bool encontrado = false;

    system("clear");
    printf("\n==============================\n");
    printf("    💵 EXTRACCION DE DINERO 💵\n");
    printf("==============================\n");
    printf("Introduzca la cantidad que desea extraer: ");
    scanf("%d", &saldo_extraer);

    if (saldo_extraer < 0) {
        printf("❌ No se puede extraer dinero negativo.\n");
        Escribir_registro("Intento de extracción negativa en ExtraerDinero.c");
        return NULL;
    }

    if (saldo_extraer > config.limite_retiro) {
        printf("❌ La cantidad excede el límite de retiro.\n");
        Escribir_registro("Intento de extracción superior al límite en ExtraerDinero.c");
        return NULL;
    }

    // 🔒 Entramos a zona crítica
    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    // Leer último ID de transacción del archivo
    int id_transacciones = 0;
    char linea[200];
    FILE *ArchivoTransacciones = fopen("transaciones.txt", "r");
    if (ArchivoTransacciones) {
        while (fgets(linea, sizeof(linea), ArchivoTransacciones) != NULL) {
            int temp_id;
            if (sscanf(linea, "%d |", &temp_id) == 1) {
                id_transacciones = temp_id;
            }
        }
        fclose(ArchivoTransacciones);
    }
    id_transacciones++; // Siguiente ID para esta transacción

    // Buscar el usuario en la memoria compartida
    for (int i = 0; i < MAX_CUENTAS; i++) {
        if (strcmp(cuentas[i].Nombre, usuario->Usuario3) == 0 &&
            strcmp(cuentas[i].Contraseña, usuario->Contraseña1) == 0) {

            encontrado = true;

            if (saldo_extraer > cuentas[i].saldo) {
                printf("❌ Saldo insuficiente.\n");
                sem_post(sem_transacciones);
                sem_post(sem_usuarios);
                return NULL;
            }

            int saldo_inicial = cuentas[i].saldo;
            cuentas[i].saldo -= saldo_extraer;
            cuentas[i].Numero_transacciones++;

            // Registrar la transacción
            ArchivoTransacciones = fopen("transaciones.txt", "a");
            if (ArchivoTransacciones) {
                fprintf(ArchivoTransacciones, "%d | retiro | %d | - | %d | - | %d\n",
                        id_transacciones, cuentas[i].id, saldo_inicial, cuentas[i].saldo);
                fclose(ArchivoTransacciones);
            }

            Escribir_registro("Dinero extraído correctamente en ExtraerDinero.c");
            printf("✅ Dinero extraído correctamente. Nuevo saldo: %d\n", cuentas[i].saldo);
            sleep(2);
            break;
        }
    }

    // 🔓 Salimos de zona crítica
    sem_post(sem_transacciones);
    sem_post(sem_usuarios);

    if (!encontrado) {
        printf("❌ Usuario no encontrado o contraseña incorrecta.\n");
        Escribir_registro("Usuario no encontrado en ExtraerDinero.c");
        sleep(2);
    }

    return NULL;
}