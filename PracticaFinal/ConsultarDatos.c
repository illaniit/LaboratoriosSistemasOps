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


/// @brief En esta funcion permitimos al usuario consualtar sus datos , abriendo el archivo y tambien sus transferencias, para verficiar el
/// usuario que nos pasamos como parametro al hilo de ejecuccion 
/// @param user  estos parametros permiten que comprobremos en el archivo que usuario es 
/// @param passwd 

// Inicializamos las funciones de Datos cuentas , Consultar Transferencias , Comprobar Cuenta y limpiar cadena
// Todas estas funciones son llamadas por la funcion que llama el hilo , ConsultarDatos  y cada una tiene una funcion especifica
void DatosCuenta(char *user,char *passwd); 
void ConsultarTransferencias(char *user,char *passwd);

bool ComprobarCuenta(char *cuenta, char *contraseña);

//Este es el struct usuario para cargar las variables que pasan por parametro del hilo
struct Usuario2
    {
        char Usuario1[50];
        char Contraseña1[50];
    }Usuario2;


/// @brief Esta funcion la llamamos mas adelante y nos sirve para comprobar que la cuenta sea correcta
/// @param cuenta 
/// @param contraseña 
/// @return devuelve true si la cuenta es correcta y false si no encuentra la cuenta

/// @brief Esta funcion despliega un menu con opciones y sobretodo es una seccion critica ya que maneja archivos , comprobar cuenta tambien maneja un archivo pero 
/// realmente la llama desde esta funcion por lo que no es necesario 
/// @param arg 
/// @return // esta funcion no devuelve nada
void *ConsultarDatos(void *arg) {
    
   

    struct Usuario2 *usuario = (struct Usuario2 *)arg; // hacemos un cast del argumneto y cambiamos el void por el tipo struct
    int Eleccion = 0;

    do {
        // este es el menu de la consulta de datos
        system("clear");
        printf("\n----------📊 Menu de consulta de datos 📊----------\n");
        printf("1️⃣  Datos de tu cuenta\n");
        printf("2️⃣  Consultar las transferencias\n");
        printf("3️⃣  Volver al menu\n");
        printf("Introduce tu elección: ");
        
        if (scanf("%d", &Eleccion) != 1) {
            printf("Entrada inválida. Inténtalo de nuevo.\n");
            while (getchar() != '\n'); // Limpiar buffer de entrada
            continue;
        }
        sem_wait(sem_usuarios);
        sem_wait(sem_transacciones);
        switch (Eleccion) {
            case 1:
                DatosCuenta(usuario->Usuario1,usuario->Contraseña1); // le pasamos a cada funcion el usuario y contraseña de la persona que ha iniciado sesion
                
                break;
            case 2:
                ConsultarTransferencias(usuario->Usuario1,usuario->Contraseña1);
                break;
            case 3:
                printf("Volviendo al menú...\n");
                sleep(2);
                break;
            default:
                printf("Opción no válida, intenta de nuevo.\n");
        }
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);
    } while (Eleccion != 3);

 
    return NULL; // devolvemos NULL
    
}

void DatosCuenta(char *user,char *passwd) {
    system("clear");
    bool encontrado=false;
    char var[100];
    FILE *archivoCuentas = fopen("usuarios.txt", "r");
    if (!archivoCuentas) {
        perror("Error al abrir usuario.txt");
        return;
    }
    limpiar_cadena(user);
    char linea[256];
    int id1, saldo1, num_transacciones1;
    char nombre1[50], contrasena1[50], apellidos1[50], domicilio1[100], pais1[50];
    while (fgets(linea, sizeof(linea), archivoCuentas)) {
        linea[strcspn(linea, "\n")] = '\0';

        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &id1, nombre1, contrasena1, apellidos1, domicilio1, pais1, &saldo1, &num_transacciones1) == 8) {
                    limpiar_cadena(nombre1);
                    limpiar_cadena(contrasena1);
            if (strcmp(nombre1, user) == 0) {
                if(strcmp(contrasena1,passwd)==0){
                    encontrado=true;
                    break;
                }
                
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

void ConsultarTransferencias(char *user, char *passwd) {
    char var;
    system("clear");
    FILE *archivoTransacciones = fopen("transaciones.txt", "r");
    if (!archivoTransacciones) {
        perror("Error al abrir transaciones.txt");
        return;
    }

    char linea[256];
    printf("\n------ Transferencias ------\n");

    while (fgets(linea, sizeof(linea), archivoTransacciones)) {
        int id, id1, id2, saldo1, saldo2, saldofinal1, saldofinal2;
        char tipo[20];

        // Leer la línea en el formato correcto
        if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &id1, &id2, &saldo1, &saldo2, &saldofinal1, &saldofinal2) == 8) {
            
            // Convertimos el ID de usuario a entero
            int user_id = atoi(user);

            // Si el usuario está involucrado en la transacción, la mostramos
            if (user_id == id1 || user_id == id2) {
                printf("ID: %d | Tipo: %s | De: %d | A: %d | Saldo antes: %d | %d | Saldo después: %d | %d\n",
                       id, tipo, id1, id2, saldo1, saldo2, saldofinal1, saldofinal2);
            }
        }
    }

    printf("Presione 's' para volver al menú...");
    scanf(" %c", &var);  // Espacio antes de %c para evitar problemas con el buffer

    fclose(archivoTransacciones);
}
