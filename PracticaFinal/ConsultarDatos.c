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

/// @brief Esta funcion despliega un menu con opciones y sobretodo es una seccion critica ya que maneja archivos , comprobar cuenta tambien maneja un archivo pero 
/// realmente la llama desde esta funcion por lo que no es necesario 
/// @param arg 
/// @return // esta funcion no devuelve nada
void *ConsultarDatos(void *arg) {

    struct Usuario2 *usuario = (struct Usuario2 *)arg; // hacemos un cast del argumneto y cambiamos el void por el tipo struct
    int Eleccion = 0;

    do {
        Escribir_registro("El usuario ha desplegado el menu de consulta de datos en ConsultarDato.c");
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
                Escribir_registro("El usuario ha elegido la opcion de consultar datos en ConsultarDatos.c");
                break;
            case 2:
                ConsultarTransferencias(usuario->Usuario1,usuario->Contraseña1);
                Escribir_registro("El usuario ha elegido la opcion de consultar transferncias en ConsultarDatos.c");
                break;
            case 3:
                printf("Volviendo al menú...\n");
                sleep(2);
                break;
            default:
                printf("Opción no válida, intenta de nuevo.\n");
        }
   
    } while (Eleccion != 3);

 
    return NULL; // devolvemos NULL
    
}
/// @brief 
/// @param user 
/// @param passwd 
void DatosCuenta(char *user,char *passwd) {
    system("clear");

    // Leemos la configuración para obtener la ruta del archivo de cuentas
    Config config = leer_configuracion("variables.properties");
    bool encontrado=false;
    char var[100];

    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    // Abrimos el archivo de cuentas
    FILE *archivoCuentas = fopen(config.archivo_cuentas, "r");
    if (!archivoCuentas) {
        perror("Error al abrir usuario.txt");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return;
    }
    limpiar_cadena(user);   // Limpiamos espacios innecesarios en el usuario

    // Variables temporales para leer del archivo
    char linea[256];
    int id1, saldo1, num_transacciones1;
    char nombre1[50], contrasena1[50], apellidos1[50], domicilio1[100], pais1[50];

    // Buscamos la cuenta que coincida con el nombre y contraseña
    while (fgets(linea, sizeof(linea), archivoCuentas)) {
        linea[strcspn(linea, "\n")] = '\0';

        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &id1, nombre1, contrasena1, apellidos1, domicilio1, pais1, &saldo1, &num_transacciones1) == 8) {
                    limpiar_cadena(nombre1);
                    limpiar_cadena(contrasena1);

            // Si nombre y contraseña coinciden, lo encontramos
            if (strcmp(nombre1, user) == 0) {
                if(strcmp(contrasena1,passwd)==0){
                    encontrado=true;
                    break;
                }
                
            }
        }
    }
   
    fclose(archivoCuentas);
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);
   

    // Si encontramos el usuario, mostramos los datos en pantalla
    if(encontrado){
        Escribir_registro("El usuario ha visto sus datos en ConsultarDatos.c");
        printf("\n=================================================\n");
        printf("              💳 DATOS DE LA CUENTA 💳              \n");
        printf("=================================================\n");
        printf("🔹 Numero de cuenta:   %d\n", id1);
        printf("👤 Nombre:            %s\n", nombre1);
        printf("🗂️  Apellidos:         %s\n", apellidos1);
        printf("🏠 Domicilio:         %s\n", domicilio1);
        printf("🌍 País:             %s\n", pais1);
        printf("💰 Saldo:            %d\n", saldo1);
        printf("🔄 Transacciones:    %d\n", num_transacciones1);
        printf("=================================================\n");
        printf("📌 Presione 's' para volver al menú principal... ");
        scanf(" %s", var);
        
    }
    
}
/// @brief 
/// @param user 
/// @param passwd 
void ConsultarTransferencias(char *user, char *passwd) {
    char var;
    system("clear");
    Config config = leer_configuracion("variables.properties");
    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    // Abrimos el archivo de transacciones
    FILE *archivoTransacciones = fopen(config.archivo_log, "r");
    if (!archivoTransacciones) {
        perror("❌ Error al abrir transacciones.txt");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return;
    }

    char linea[256];
    int transacciones_encontradas = 0; // Para verificar si el usuario tiene transacciones

    printf("\n=================================================\n");
    printf("        💳 CONSULTA DE TRANSFERENCIAS 💳 \n");
    printf("=================================================\n");

    while (fgets(linea, sizeof(linea), archivoTransacciones)) {
        int id, id1, id2, saldo1, saldo2, saldofinal1, saldofinal2;
        char tipo[20];

        // Obtenemos el ID del usuario para buscar sus transferencias
        int user_id = obtener_id_usuario(user, passwd);
        
        // Leer la línea en el formato correcto
        if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &id1, &id2, &saldo1, &saldo2, &saldofinal1, &saldofinal2) == 8) {
      
            // Si el usuario está involucrado en la transacción, la mostramos
            if (user_id == id1 || user_id == id2) {
                transacciones_encontradas = 1; // Si encontramos al menos una transacción
                printf("\nNumero de tranferencia: %d | Tipo: %s | De cuenta: %d | A cuenta: %d\n", id, tipo, id1, id2);
                printf(" Saldo antes: %d | Saldo después: %d | \n", saldo1, saldofinal1);
                printf("-------------------------------------------------\n");
            }
        }
    }
    fclose(archivoTransacciones);
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    // Si no se encontraron transferencias
    if (!transacciones_encontradas) {
        printf("\n❗ No se han encontrado transferencias para este usuario.\n");
    }

    printf("\n📌 Presione 's' para volver al menú principal... ");
    scanf(" %c", &var);  // Espacio antes de %c para evitar problemas con el buffer

   
}

