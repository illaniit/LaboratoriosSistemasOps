
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define Punt_Archivo_Properties "Variables.properties"
#include "init_cuentas.h"

void Menu_Usuario()
{

    int Eleccion;
    do
    {
        // menu de los usuarios
        printf(" ------------------Elija una opcion---------------------- ");
        printf(" |   1.Incio de sesion                                    |");
        printf(" |   2.Registro                                           |");
        printf(" |       Pulse una opcion (1/2):                          |");
        printf(" --------------------------------------------------------- ");
        scanf("%d", &Eleccion);

        if (Eleccion == 1)
        {
            InicioDeSesion(); // Funcion de inicio de sesion
        }
        else if (Eleccion == 2)
        {
            Registro(); // funcion de registro
        }
    } while (Eleccion != 1 || Eleccion != 2);
}
void InicioDeSesion()
{
    FILE *archivo = fopen("usuarios.dat", "r"); // Abrimos el archivos usuarios.dat
    // declaramos todas las variables
    char user[255];
    char passwd[255];
    char Usuario[255];
    char Contraseña[255];
    char linea[256];
    do
    {

        printf("\n -------------------Inicio de Sesion------------------------ \n ");
        printf("\n Introduce tu nombre de usuario: \n");
        scanf("%s", Usuario);
        printf("\n Contraseña:");
        scanf("%s", Contraseña);
        if (archivo == NULL)
        {
            perror("Error al abrir el archivo");
            exit(-1);
        }

        while (fgets(linea, sizeof(linea), archivo))
        {

            // Parsear la línea del CSV
            if (sscanf(linea, "%254[^,],%254[^,]", user, passwd) == 2)
            {
                // Verificar las condiciones
                if (Usuario == user)
                {
                    if (Contraseña == passwd)
                    {
                        printf("Dando acceso al sistema...");
                        sleep(10);
                        // Funcion para crear hilos de usuario.c con un semaforo
                        // Abrir el registros.log y meter que el usuario ha entrado al sistema
                    }
                    else
                    {
                        printf("Contraseña incorrecta");
                        // abrir el registros.log y introducir que el usuario ha intentado iniciar sesion
                    }
                }
                else
                {
                    printf("Nombre de usuario o contraseña incorrectas");
                    // abrir el registros.log y introducir que el usurio ha intentado iniciar sesion
                }
            }
        }

    } while (Contraseña == passwd && Usuario == user); // si la contraseña o el usuario no son correctas volvemos a mostrar el menu y este mensaje , tambien se podria hacer un contandor para que te avise de que has fallado muchas veces o algo asi

    fclose(archivo); // cerramos el archivo con fclose
}
void Registro()
{
    struct Cuenta
    {
        int id;                   // id de la cuenta
        char Nombre[50];          // Nombre de usuario de la cuenta
        double saldo;             // saldo de la cuenta
        int Numero_transacciones; // Numero de transacciones
    };

    struct Cuenta cuenta;
    printf("--------Registro-------");
    printf("Introduce tu Nombre : ");
    scanf("%s", cuenta.Nombre);
    printf("Introudce el saldo inicial");
    scanf("%lf", cuenta.saldo);

    FILE *usuarios=fopen("usuarios.dat","w+");
    if(!usuarios){
        perror("Error al abrir el archivo de propiedades");
        return;
    }

    // Pedir los datos y almacenarlos en el usuario.dat : id,nombre,apellidos,numeroDeCuenta,saldo_inicial
    // Registrar el registro con el .log
}
