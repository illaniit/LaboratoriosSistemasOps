
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define Punt_Archivo_Properties "Variables.properties"
#include "Usuario.h"
#define MAX_LENGTH 256
#define NUM_USUARIOS 5
#define MAX_USUARIOS 5
#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "Comun.h"


//incializamos las funciones
void *Menu_Usuario();
void Registro();
void InicioDeSesion();
int main()
{
  //  Llamamos a menu usuario
    Menu_Usuario();
}
//esto hay que meterlo en abrir_propertis.c y asi no lo tenemos que declarar en todos falta hacerlo

// Función para abrir y leer el archivo de propiedades
// Esta funcion nos permite cargar las variables que usemos en el codigo para que sea mas accesible

/// @brief Esta funcion despliega un menu que le permite al usuario inciar sesion o registrarse
/// @return 
void *Menu_Usuario()
{
    
    Escribir_registro("Se ha accedido al menú de entrada"); // escribimos en el registro

    int Eleccion;
    do
    {
    
        //este es el menu de incio de sesion
        system("clear");
        printf("\n------------------💰Elija una opción💰------------------\n");
        printf("|   1️⃣  Inicio de sesión                                 |\n");
        printf("|   2️⃣  Registro                                         |\n");
        printf("|       Pulse una opción (1/2):                           |\n");
        printf("---------------------------------------------------------\n");
        scanf("%d", &Eleccion);

        if (Eleccion == 1)
        {

            InicioDeSesion();

            Escribir_registro2("El usuario ha elegido iniciar sesión");
        }
        else if (Eleccion == 2)
        {
            Registro();
            Escribir_registro("El usuario ha elegido la opción de registro");
            printf("Volviendo a tu menu...");
            sleep(4);
            Menu_Usuario();
        }
    } while (Eleccion != 1 && Eleccion != 2);

    return NULL;
}

/// @brief esta funcion le permite al usuario inciar sesion con su cuenta y guardar sus claves en el archivo
void InicioDeSesion() {
    system("clear");
    FILE *archivo;
    char Usuario[50], Contraseña[50]; // Datos ingresados por el usuario
    char linea[256]; // Buffer para leer las líneas del archivo
    int intentos = 0, max_intentos = 3; // Contador de intentos

    do {
        archivo = fopen("usuarios.txt", "r"); // Abrimos el archivo en modo lectura
        if (archivo == NULL) {
            perror("Error al abrir el archivo");
            exit(EXIT_FAILURE);
        }

        printf("\n------------------- Inicio de Sesión ------------------------\n");
        printf("Introduce tu nombre de usuario: ");
        scanf("%49s", Usuario);

        printf("Contraseña: ");
        scanf("%49s", Contraseña);

        // Limpiar las cadenas de los espacios antes de compararlas
        limpiar_cadena(Usuario);
        limpiar_cadena(Contraseña);

        int acceso = 0; // Bandera para verificar si el usuario es válido

        while (fgets(linea, sizeof(linea), archivo)) {
            // Eliminar el salto de línea al final de la línea
            linea[strcspn(linea, "\n")] = '\0';

            // Variables para almacenar los datos extraídos
            int id, saldo, num_transacciones;
            char nombre[50], contrasena[50], apellidos[50], domicilio[100], pais[50];

            // Extraemos correctamente los campos de acuerdo al formato "id | nombre | contraseña | apellidos | domicilio | pais | saldo | num_transacciones"
            if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                    &id, nombre, contrasena, apellidos, domicilio, pais, &saldo, &num_transacciones) == 8) {

                // Limpiar las cadenas leídas del archivo
                limpiar_cadena(nombre);
                limpiar_cadena(contrasena);

                // Comparar usuario y contraseña
                if (strcmp(Usuario, nombre) == 0) {
                    acceso = 1; // Se encontró el usuario

                    if (strcmp(Contraseña, contrasena) == 0) { 
                        printf("\n✅ Acceso concedido. Bienvenido, %s!\n", Usuario);
                        sleep(2);
                        Escribir_registro2("Se ha accedido al sistema correctamente");
                        fclose(archivo);
                        Mostrar_Menu(Usuario,Contraseña); // Función que muestra el menú principal
                        Menu_Usuario();
                        return;
                    } else {
                        printf("\n⚠️ Contraseña incorrecta. Inténtalo de nuevo.\n");
                        Escribir_registro2("Intento de inicio de sesión con contraseña incorrecta");
                        break;
                    }
                }
            }
        }

        if (!acceso) {
            printf("\n⚠️ Nombre de usuario o contraseña incorrectos.\n");
            Escribir_registro2("Intento fallido de inicio de sesión");
        }

        fclose(archivo); // Cerramos el archivo después de leer
        intentos++;

    } while (intentos < max_intentos);

    printf("\n⛔ Demasiados intentos fallidos. Inténtalo más tarde.\n");
    Escribir_registro("Se ha bloqueado el acceso por múltiples intentos fallidos.");
}
/// @brief esta funcion permite que el usuario se registre en una cuenta nueva
void Registro()
{
    Escribir_registro("El usuario ha entrado en la sección de registro");

    struct Cuenta
    {
        int id;            // ID de la cuenta
        char Nombre[50];   // Nombre de usuario
        char Apellido[50]; // Apellido del usuario
        char Contraseña[50];  
        char RepetirContraseña[50];
        char domicilio[100];      // Domicilio
        char pais[50];            // País
        int saldo;                // Saldo inicial
        int Numero_transacciones; // Número de transacciones
    };

    struct Cuenta cuenta;
    cuenta.id = 1; // Comenzamos en 1 en caso de ser el primer usuario
    cuenta.Numero_transacciones = 0;

    printf("-------- Registro -------\n");

    printf("Introduce tu Nombre: ");
    while (getchar() != '\n') ; // Limpieza del buffer de entrada
    fgets(cuenta.Nombre, sizeof(cuenta.Nombre), stdin);
    cuenta.Nombre[strcspn(cuenta.Nombre, "\n")] = '\0'; // Eliminar salto de línea

    printf("Introduce tus Apellidos: ");
    fgets(cuenta.Apellido, sizeof(cuenta.Apellido), stdin);
    cuenta.Apellido[strcspn(cuenta.Apellido, "\n")] = '\0';
    do {
        printf("Escribe tu contraseña: ");
        fgets(cuenta.Contraseña, sizeof(cuenta.Contraseña), stdin);
        cuenta.Contraseña[strcspn(cuenta.Contraseña, "\n")] = '\0'; // Elimina el salto de línea

        printf("Repite tu contraseña: ");
        fgets(cuenta.RepetirContraseña, sizeof(cuenta.RepetirContraseña), stdin);
        cuenta.RepetirContraseña[strcspn(cuenta.RepetirContraseña, "\n")] = '\0';

        if (strcmp(cuenta.Contraseña, cuenta.RepetirContraseña) != 0) {
            printf("⚠️ Las contraseñas no coinciden ⚠️\n");
            printf("Vuelve a escribirlas.\n");
        }
    } while (strcmp(cuenta.Contraseña, cuenta.RepetirContraseña) != 0);
    printf("Introduce tu domicilio: ");
    fgets(cuenta.domicilio, sizeof(cuenta.domicilio), stdin);
    cuenta.domicilio[strcspn(cuenta.domicilio, "\n")] = '\0';

    printf("Introduce tu país de residencia: ");
    fgets(cuenta.pais, sizeof(cuenta.pais), stdin);
    cuenta.pais[strcspn(cuenta.pais, "\n")] = '\0';

    // Validación del saldo ingresado
    do
    {
        printf("Introduce el saldo inicial (debe ser un número positivo): ");
        if (scanf("%d", &cuenta.saldo) != 1)
        {
            printf("Error: Debes ingresar un número válido.\n");
            while (getchar() != '\n'); // Limpiar el buffer de entrada
        }
        else if (cuenta.saldo < 0)
        {
            printf("Error: El saldo no puede ser negativo.\n");
        }
    } while (cuenta.saldo < 0);

    while (getchar() != '\n'); // Limpiar buffer tras scanf()

    printf("Verificando que el saldo que ha introducido es correcto...\n");
    sleep(2);

    // Abrir el archivo en modo lectura para contar usuarios existentes
    FILE *usuarios = fopen("usuarios.txt", "r+");
    if (!usuarios)
    {
        usuarios = fopen("usuarios.txt", "w"); // Si no existe, crearlo
        if (!usuarios)
        {
            perror("Error al abrir el archivo de usuarios");
            return;
        }
    }

    char linea[255];
    while (fgets(linea, sizeof(linea), usuarios) != NULL)
    {
        cuenta.id++; // Incrementar ID basado en el número de líneas
    }

    // Escribir los datos en el archivo
    fprintf(usuarios, "%d | %s | %s | %s | %s | %s | %d | %d\n",
            cuenta.id, cuenta.Nombre,cuenta.Contraseña ,cuenta.Apellido, cuenta.domicilio,
            cuenta.pais, cuenta.saldo, cuenta.Numero_transacciones);

    fclose(usuarios); // Cerrar el archivo

    Escribir_registro("Se ha registrado un nuevo usuario en el sistema");

   
}
