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


/// @brief En esta funcion permitimos al usuario consualtar sus datos , abriendo el archivo y tambien sus transferencias, para verficiar el
/// usuario que nos pasamos como parametro al hilo de ejecuccion 
/// @param user  estos parametros permiten que comprobremos en el archivo que usuario es 
/// @param passwd 

// Inicializamos las funciones de Datos cuentas , Consultar Transferencias , Comprobar Cuenta y limpiar cadena
// Todas estas funciones son llamadas por la funcion que llama el hilo , ConsultarDatos  y cada una tiene una funcion especifica
void DatosCuenta(char *user,char *passwd); 
void ConsultarTransferencias(char *user,char *passwd);
void limpiar_cadena1(char *cadena);
bool ComprobarCuenta(char *cuenta, char *contraseña);

//Este es el struct usuario para cargar las variables que pasan por parametro del hilo
struct Usuario2
    {
        char Usuario1[50];
        char Contraseña1[50];
    }Usuario2;


/// @brief esta funcion limpia la cadena para el strcmp posterior , lo que hace es quitar los espacios en blanco y tabulaciones de la cadena
/// @param cadena 
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

/// @brief Esta funcion la llamamos mas adelante y nos sirve para comprobar que la cuenta sea correcta
/// @param cuenta 
/// @param contraseña 
/// @return devuelve true si la cuenta es correcta y false si no encuentra la cuenta
bool ComprobarCuenta(char *cuenta, char *contraseña){
    
        FILE *archivo = fopen("usuarios.txt", "r"); // Abrimos el archivo en modo lectura
    
        if (archivo == NULL)
        {
            printf("❌ No se pudo abrir el archivo de usuarios.\n"); 
            return false;  // No se pudo abrir el archivo
        }
    
        char linea[200]; // Para almacenar cada línea del archivo
        while (fgets(linea, sizeof(linea), archivo) != NULL)
        {
            // Separar la línea usando strtok() para obtener id, nombre y contraseña
            char *id = strtok(linea, "|");
            char *nombre = strtok(NULL, "|");
            char *contraseña_archivo = strtok(NULL, "|");
    
            // Limpiar posibles espacios en blanco alrededor de las cadenas
            if (nombre != NULL) {
                nombre[strcspn(nombre, "\n")] = 0;  // Eliminar el salto de línea al final de nombre
            }
            if (contraseña_archivo != NULL) {
                contraseña_archivo[strcspn(contraseña_archivo, "\n")] = 0;  // Eliminar el salto de línea al final de contraseña
            }
    
            // Comprobamos si el nombre y la contraseña coinciden
            if (nombre != NULL && contraseña_archivo != NULL &&
                strcmp(nombre, cuenta) == 0 && strcmp(contraseña_archivo, contraseña) == 0)
            {
                fclose(archivo);  // Cerramos el archivo antes de regresar
                return true;      // Se encontró la cuenta con la contraseña correcta
            }
        }
    
        fclose(archivo);  // Cerramos el archivo si no se encontró la cuenta
        return false;     // No se encontró la cuenta o la contraseña no coincide
    
}
/// @brief Esta funcion despliega un menu con opciones y sobretodo es una seccion critica ya que maneja archivos , comprobar cuenta tambien maneja un archivo pero 
/// realmente la llama desde esta funcion por lo que no es necesario 
/// @param arg 
/// @return // esta funcion no devuelve nada
void *ConsultarDatos(void *arg) {
    sem_t s1;  // declaramos el semaforo de hilos 
    sem_init(&s1,0,1); //  inicializamos el semaforo a 1
    sem_wait(&s1); // esta es la funcion critica y disminuimos uno el valor del semaforo
    
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
    } while (Eleccion != 3);
  sem_post(&s1); // incrementamos 1 el valor del semaforo porque la seccion critica ha finalizado
  sem_destroy(&s1); // destruimos los recursos del semaforo
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
    limpiar_cadena1(user);
    char linea[256];
    int id1, saldo1, num_transacciones1;
    char nombre1[50], contrasena1[50], apellidos1[50], domicilio1[100], pais1[50];
    while (fgets(linea, sizeof(linea), archivoCuentas)) {
        linea[strcspn(linea, "\n")] = '\0';

        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &id1, nombre1, contrasena1, apellidos1, domicilio1, pais1, &saldo1, &num_transacciones1) == 8) {
                    limpiar_cadena1(nombre1);
                    limpiar_cadena1(contrasena1);
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
    char var[20];
    system("clear");
    FILE *archivoTransacciones = fopen("transaciones.txt", "r");
    if (!archivoTransacciones) {
        perror("Error al abrir transacciones.txt");
        return;
    }
     bool CuentaVer=false;
    char linea[256];
    printf("\n------ Transferencias ------\n");
    while (fgets(linea, sizeof(linea), archivoTransacciones)) {
        int id, saldo1, saldo2, saldofinal;
        char tipo[20], cuenta1[50], cuenta2[50];

        if (sscanf(linea, "%d , %19[^,] , %49[^,] , %49[^,] , %d , %d , %d",
                   &id, tipo, cuenta1, cuenta2, &saldo1, &saldo2, &saldofinal) == 7) {
            limpiar_cadena1(cuenta1);
            limpiar_cadena1(cuenta2);
            if (strcmp(cuenta1, user) == 0) {
                CuentaVer = ComprobarCuenta(cuenta1,passwd);
                if(CuentaVer){
                printf("ID: %d | Tipo: %s | De: %s | A: %s | Saldo antes: %d | Saldo después: %d\n",
                       id, tipo, cuenta1, cuenta2, saldo1, saldofinal);
                }
            }
            if(strcmp(cuenta2,user)==0){
               CuentaVer = ComprobarCuenta(cuenta2,passwd);
               printf("ID: %d | Tipo: %s | De: %s | A: %s | Saldo antes: %d | Saldo después: %d\n",
                id, tipo, cuenta1, cuenta2, saldo1, saldofinal);
            }
        }
        
    }
    printf("Presione s  para volver al menu....");
    scanf("%s",var);
    fclose(archivoTransacciones);
}
