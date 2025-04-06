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
#include "Comun.h"

// declaramos el struct de usuario
struct Usuario3
{
    char Usuario2[50];
    char Contraseña1[50];
};

/// @brief Esta funcion permite que un usuario introduzca dinero en el sistema, actualizamos el saldo y le numero de transaccioens
/// y tambien escribe en el archivo de transcciones
/// @param arg2
/// @return
void *IntroducirDinero(void *arg2)
{

    struct Usuario3 *usuario = (struct Usuario3 *)arg2; // cargamos el struct que le hemso pasado al hilo en nuestro struct
    bool encontrado = false; 
                               // declaramos una variable boolean de tipo encontrado a false
    int saldo_introducir;  
    system("clear");   
    printf("\n==============================\n");
    printf("    💵 INGRESO DE DINERO 💵\n");
    printf("==============================\n");   // vemos cuento quiere ingresar el usuario
    printf("Introduzca la cantidad que desea ingresar: ");
    scanf("%d", &saldo_introducir);
    if(saldo_introducir<0){
        printf("No puedes ingresar una cantidad negativa!");
        Escribir_registro("El usuario ha intentado introducir una cantidad negativa en IntroducirDinero.c");
        return NULL;
    }
    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    FILE *ArchivoUsuarios = fopen("usuarios.txt", "r"); // abrimos el archivo de usuarios
    if (!ArchivoUsuarios)
    {
        perror("Error al abrir el archivo");
        return NULL;
    }
    FILE *tempFile = fopen("temp.txt", "w"); // abrimos un archivo temporal
    if (!tempFile)
    {
        perror("Error al abrir el archivo temporal");
        fclose(ArchivoUsuarios);
        return NULL;
    }
    // declaramos las variables necesarias
    int id2, saldo2, num_transacciones2;
    char nombre2[50], contrasena2[50], apellidos2[50], domicilio2[100], pais2[50];
    char linea[200], linea2[200];

    // este bloqeu lo que hace es hacer que el id de las transcciones se actulice teneiendo en cuent el ultimo id registrado
    int id_transacciones = 0;
    FILE *ArchivoTransacciones = fopen("transaciones.txt", "r"); // Abrimos en modo lectura
    if (ArchivoTransacciones)
    {
        while (fgets(linea2, sizeof(linea2), ArchivoTransacciones) != NULL)
        {
            int temp_id;
            if (sscanf(linea2, "%d |", &temp_id) == 1)
            {
                id_transacciones = temp_id; // Guardamos el último ID encontrado
            }
        }
        fclose(ArchivoTransacciones); // cerramos el archivo
    }
    id_transacciones++; // Incrementamos para el nuevo registro

    // este while lee el archivo de usuarios
    while (fgets(linea, sizeof(linea), ArchivoUsuarios) != NULL)
    {
        linea[strcspn(linea, "\n")] = '\0';

        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &id2, nombre2, contrasena2, apellidos2, domicilio2, pais2, &saldo2, &num_transacciones2) == 8)
        {
            limpiar_cadena(nombre2); // llamamos a limpiar cadena
            limpiar_cadena(contrasena2);
            if (strcmp(nombre2, usuario->Usuario2) == 0 && strcmp(contrasena2, usuario->Contraseña1) == 0)
            {                                // comparamos el nombre del inicio de sesion con el de cda uno de las lineas de lso archivos
                int dinero_inicial = saldo2; // declaramos una variable que almacene el dinero inciarl
                encontrado = true;           // cambiamos el estado de la variable encontrdo a true
                Escribir_registro("El usuario ha introducido la cantidad ha ingresar en introducirDinero.c");
                saldo2 += saldo_introducir; // incrementeamos su saldo
                num_transacciones2++;       // incrementamos el nuemro de transacciones

                ArchivoTransacciones = fopen("transaciones.txt", "a"); // abrimos el archivo de transacciones para registrar la transaccione con este formato
                if (ArchivoTransacciones)
                {
                    fprintf(ArchivoTransacciones, "%d | ingreso | %d | - | %d | - | %d \n", id_transacciones, id2, dinero_inicial, saldo2);
                    fclose(ArchivoTransacciones);
                }
            }
        }
        // reescribimos el archivo entero en un archivo temporal
        fprintf(tempFile, "%d | %s | %s | %s | %s | %s | %d | %d\n", id2, nombre2, contrasena2, apellidos2, domicilio2, pais2, saldo2, num_transacciones2);
    }

    fclose(ArchivoUsuarios); // cerramos ambos archiovs
    fclose(tempFile);

    if (encontrado)
    {
        remove("usuarios.txt");             // eliminamos le usuarios.txt
        rename("temp.txt", "usuarios.txt"); // cambiamos el nombre del archivo temporal al usuario.txt
        printf("Saldo actualizado correctamente.\n");
        sleep(2);
    }
    else
    {
        remove("temp.txt");
        printf("Usuario no encontrado o contraseña incorrecta.\n");
    }
    Escribir_registro("El usuario ha ingresado dinero en IngresarDinero.c");
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);
    return NULL;
}
