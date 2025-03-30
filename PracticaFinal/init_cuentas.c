
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define Punt_Archivo_Properties "Variables.properties"
#include "Usuario.h"
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
    sem_registro= sem_open("/sem_registro",0);//  Llamamos a menu usuario
    Menu_Usuario();
}
//esto hay que meterlo en abrir_propertis.c y asi no lo tenemos que declarar en todos falta hacerlo

// Función para abrir y leer el archivo de propiedades
// Esta funcion nos permite cargar las variables que usemos en el codigo para que sea mas accesible

/// @brief Esta funcion despliega un menu que le permite al usuario inciar sesion o registrarse
/// @return 
void *Menu_Usuario() {
    Escribir_registro("Se ha accedido al menú de entrada");

    int Eleccion;

    do {
        system("clear");  // Limpia la pantalla en sistemas UNIX

    printf("\n==========================================\n");
    printf("            💰 BANCO 💰                        \n");
    printf("==========================================\n");
    printf(" 1️⃣  Inicio de sesión\n");
    printf(" 2️⃣  Registro\n");
    printf(" 3️⃣  Salir\n");
    printf("------------------------------------------\n");
    printf("\n  Pulse una opción: ");
      
        scanf("%d", &Eleccion);  // Aquí el usuario ingresa la opción
        switch (Eleccion) {
            case 1:
                Escribir_registro("El usuario ha elegido iniciar sesión");
                InicioDeSesion();
                break;

            case 2:
                Escribir_registro("El usuario ha elegido la opción de registro");
                Registro();
                printf("Volviendo al menú...💰\n");
                sleep(2);
                break;

            case 3:
                printf("Tenga un buen día 😊...\n");
                sleep(2);
                return NULL;

            default:
                printf("⚠️  Opción no válida. Intente de nuevo.\n");
                sleep(2);
                break;
        }

    } while (Eleccion != 3);

    return NULL;
}

/// @brief esta funcion le permite al usuario inciar sesion con su cuenta y guardar sus claves en el archivo
void InicioDeSesion() {
    system("clear");
    Config config = leer_configuracion("variables.properties");
    FILE *archivo;
    char Usuario[50], Contraseña[50]; // Datos ingresados por el usuario
    char linea[256]; // Buffer para leer las líneas del archivo
    int intentos = 0, max_intentos = 3; // Contador de intentos

    do {
        archivo = fopen(config.archivo_cuentas, "r"); // Abrimos el archivo en modo lectura
        if (archivo == NULL) {
            perror("Error al abrir el archivo");
            exit(EXIT_FAILURE);
        }
        system("clear");
        printf("\n============================================================\n");
        printf("                 🔐  INICIO DE SESIÓN  🔐                   \n");
        printf("============================================================\n");
        printf("📌  Introduce tu nombre : ");
        scanf("%49s", Usuario);
    
        printf("🔑  Contraseña: ");
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
                        Escribir_registro("Se ha accedido al sistema correctamente");
                        fclose(archivo);
                        Mostrar_Menu(Usuario,Contraseña); // Función que muestra el menú principal
                        Menu_Usuario();
                        return;
                    } else {
                        printf("\n⚠️ Contraseña incorrecta. Inténtalo de nuevo.\n");
                        Escribir_registro("Intento de inicio de sesión con contraseña incorrecta");
                        break;
                    }
                }
            }
        }

        if (!acceso) {
            Escribir_registro("Intento fallido de inicio de sesión");
            printf("\n⚠️ Nombre de usuario o contraseña incorrectos \n\n");
            sleep(2);
    
        }

        fclose(archivo); // Cerramos el archivo después de leer
        intentos++;

    } while (intentos < max_intentos);

    printf("\n⛔ Demasiados intentos fallidos. Inténtalo más tarde.\n");
    Escribir_registro("Se ha bloqueado el acceso por múltiples intentos fallidos.");
}
/// @brief esta funcion permite que el usuario se registre en una cuenta nueva
void Registro() {
    Escribir_registro("El usuario ha entrado en la sección de registro");
    Config config = leer_configuracion("variables.properties");
    struct Cuenta {
        int id;
        char Nombre[50];
        char Apellido[50];
        char Contraseña[50];
        char RepetirContraseña[50];
        char domicilio[100];
        char pais[50];
        int saldo;
        int Numero_transacciones;
    };

    struct Cuenta cuenta;
    cuenta.id = 1;
    cuenta.Numero_transacciones = 0;

    system("clear");  // Limpia la pantalla antes de mostrar el menú

    printf("\n============================================================\n");
    printf("                 📝 REGISTRO DE NUEVO USUARIO               \n");
    printf("============================================================\n");

    // Capturar el nombre de usuario
    printf("\n👤 Introduce tu Nombre: ");
    while (getchar() != '\n'); // Limpieza del buffer
    fgets(cuenta.Nombre, sizeof(cuenta.Nombre), stdin);
    cuenta.Nombre[strcspn(cuenta.Nombre, "\n")] = '\0';

    // Capturar el apellido
    printf("Introduce tus Apellidos: ");
    fgets(cuenta.Apellido, sizeof(cuenta.Apellido), stdin);
    cuenta.Apellido[strcspn(cuenta.Apellido, "\n")] = '\0';

    // Validar contraseña
    do {
        printf("🔑 Escribe tu contraseña: ");
        fgets(cuenta.Contraseña, sizeof(cuenta.Contraseña), stdin);
        cuenta.Contraseña[strcspn(cuenta.Contraseña, "\n")] = '\0';

        printf(" Repite tu contraseña: ");
        fgets(cuenta.RepetirContraseña, sizeof(cuenta.RepetirContraseña), stdin);
        cuenta.RepetirContraseña[strcspn(cuenta.RepetirContraseña, "\n")] = '\0';

        if (strcmp(cuenta.Contraseña, cuenta.RepetirContraseña) != 0) {
            printf("\n⚠️  ¡Las contraseñas no coinciden! Inténtalo nuevamente.\n\n");
        }
    } while (strcmp(cuenta.Contraseña, cuenta.RepetirContraseña) != 0);

    // Capturar domicilio
    printf("🏠 Introduce tu domicilio: ");
    fgets(cuenta.domicilio, sizeof(cuenta.domicilio), stdin);
    cuenta.domicilio[strcspn(cuenta.domicilio, "\n")] = '\0';

    // Capturar país
    printf("Introduce tu país de residencia: ");
    fgets(cuenta.pais, sizeof(cuenta.pais), stdin);
    cuenta.pais[strcspn(cuenta.pais, "\n")] = '\0';

    // Validación del saldo ingresado
    do {
        printf("💰 Introduce el saldo inicial (debe ser un número positivo): ");
        if (scanf("%d", &cuenta.saldo) != 1) {
            printf("\n❌ Error: Debes ingresar un número válido.\n\n");
            while (getchar() != '\n'); // Limpiar el buffer
        } else if (cuenta.saldo < 0) {
            printf("\n❌ Error: El saldo no puede ser negativo.\n\n");
        }
    } while (cuenta.saldo < 0);

    while (getchar() != '\n'); // Limpiar buffer tras scanf()

    printf("\n✅ Verificando datos... Por favor, espere.\n");
    sleep(2);

    // Abrir el archivo en modo lectura para contar usuarios existentes
    FILE *usuarios = fopen(config.archivo_cuentas, "r+");
    if (!usuarios) {
        usuarios = fopen(config.archivo_cuentas, "w");
        if (!usuarios) {
            perror("❌ Error al abrir el archivo de usuarios");
            return;
        }
    }

    // Contar líneas para asignar el ID del usuario
    char linea[255];
    while (fgets(linea, sizeof(linea), usuarios) != NULL) {
        cuenta.id++;
    }

    // Escribir los datos en el archivo
    fprintf(usuarios, "%d | %s | %s | %s | %s | %s | %d | %d\n",
            cuenta.id, cuenta.Nombre, cuenta.Contraseña, cuenta.Apellido, 
            cuenta.domicilio, cuenta.pais, cuenta.saldo, cuenta.Numero_transacciones);

    fclose(usuarios);

    printf("\n¡Registro exitoso! Bienvenido, %s.\n", cuenta.Nombre);
    sleep(2);

    Escribir_registro("Se ha registrado un nuevo usuario en el sistema");
}