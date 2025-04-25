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

    printf("\n==============================\n");
    printf("    💵 EXTRACCION DE DINERO 💵\n");
    printf("==============================\n");
    printf("Introduzca la cantidad que desea extraer: ");
    scanf("%d", &saldo_extraer);

    if (saldo_extraer < 0)
    {
        printf("❌ No se puede extraer dinero negativo.\n");
        Escribir_registro("Intento de extracción negativa en ExtraerDinero.c");
        return NULL;
    }

    if (saldo_extraer > config.limite_retiro)
    {
        printf("❌ La cantidad excede el límite de retiro.\n");
        Escribir_registro("Intento de extracción superior al límite en ExtraerDinero.c");
        return NULL;
    }

    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    int id_transacciones = 0;
    char linea[200];
    FILE *ArchivoTransacciones = fopen("transaciones.txt", "r");
    if (ArchivoTransacciones)
    {
        while (fgets(linea, sizeof(linea), ArchivoTransacciones) != NULL)
        {
            int temp_id;
            if (sscanf(linea, "%d |", &temp_id) == 1)
            {
                id_transacciones = temp_id;
            }
        }
        fclose(ArchivoTransacciones);
    }
    id_transacciones++;

    for (int i = 0; i < config.max_cuentas; i++)
    {
        if (strcmp(cuentas[i].Nombre, usuario->Usuario3) == 0 &&
            strcmp(cuentas[i].Contraseña, usuario->Contraseña1) == 0)
        {
            encontrado = true;

            if (saldo_extraer > cuentas[i].saldo)
            {
                printf("❌ Saldo insuficiente.\n");
                goto liberar;
            }

            int saldo_inicial = cuentas[i].saldo;
            cuentas[i].saldo -= saldo_extraer;

            ArchivoTransacciones = fopen("transaciones.txt", "a");
            if (ArchivoTransacciones)
            {
                fprintf(ArchivoTransacciones, "%d | retiro | %d | - | %d | - | %d\n",
                        id_transacciones, cuentas[i].id, saldo_inicial, cuentas[i].saldo);
                fclose(ArchivoTransacciones);
            }

            // Ruta local: ./transacciones/{id}/transacciones.log
            char dir_usuario[64];
            snprintf(dir_usuario, sizeof(dir_usuario), "./transacciones/%d", cuentas[i].id);
            mkdir("./transacciones", 0777); // por si no existe la carpeta base
            mkdir(dir_usuario, 0777);       // carpeta del usuario

            char ruta_archivo[128];
            snprintf(ruta_archivo, sizeof(ruta_archivo), "%s/transacciones.log", dir_usuario);

            FILE *ArchivoTransacciones2 = fopen(ruta_archivo, "a");
            if (!ArchivoTransacciones2)
            {
                perror("No se pudo abrir el archivo de transacciones");
                goto liberar;
            }

            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char hora_actual[20];
            strftime(hora_actual, sizeof(hora_actual), "%Y-%m-%d %H:%M:%S", tm_info);

            fprintf(ArchivoTransacciones2, "[%s] Retiro: %d ----> %d\n",
                    hora_actual, saldo_extraer, cuentas[i].saldo);

            fclose(ArchivoTransacciones2);

            Escribir_registro("Dinero extraído correctamente en ExtraerDinero.c");
            printf("✅ Dinero extraído correctamente. Nuevo saldo: %d\n", cuentas[i].saldo);
            sleep(2);
            break;
        }
    }

liberar:
    sem_post(sem_transacciones);
    sem_post(sem_usuarios);

    if (!encontrado)
    {
        printf("❌ Usuario no encontrado o contraseña incorrecta.\n");
        Escribir_registro("Usuario no encontrado en ExtraerDinero.c");
        sleep(2);
    }

    return NULL;
}
