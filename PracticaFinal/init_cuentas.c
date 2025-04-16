

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "Cuenta.h"
#include "Comun.h"

#define MAX_CUENTAS 100

// Prototipos
void Menu_Usuario();
void Registro();
void InicioDeSesion();
void GuardarEnMemoriaCompartida(Cuenta *cuenta);
void Mostrar_Menu(char *Usuario, char *Contraseña);

int main() {
    // Llamamos al menú de usuario
    Menu_Usuario();
    return 0;
}

/// @brief Esta funcion despliega un menu que le permite al usuario inciar sesion o registrarse
/// @return 
void Menu_Usuario() {
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
                InicioDeSesion();
                break;

            case 2:
                Registro();
                break;

            case 3:
                printf("Tenga un buen día 😊...\n");
                sleep(2);
                break;

            default:
                printf("⚠️  Opción no válida. Intente de nuevo.\n");
                sleep(2);
                break;
        }

    } while (Eleccion != 3);
}

/// @brief esta funcion le permite al usuario inciar sesion con su cuenta y guardar sus claves en el archivo
void InicioDeSesion() {
    char Usuario[50], Contraseña[50];
    int acceso = 0;

    key_t clave = ftok("shmfile", 65);
    int shmid = shmget(clave, sizeof(Cuenta) * MAX_CUENTAS, 0666);
    if (shmid == -1) {
        perror("No se pudo acceder a la memoria compartida");
        return;
    }

    Cuenta *cuentas = (Cuenta *) shmat(shmid, NULL, 0);
    if (cuentas == (void *) -1) {
        perror("shmat falló");
        return;
    }

    printf("👤 Nombre de usuario: ");
    scanf("%49s", Usuario);
    printf("🔑 Contraseña: ");
    scanf("%49s", Contraseña);

    // Comprobar si el usuario y contraseña coinciden en memoria
    for (int i = 0; i < MAX_CUENTAS; i++) {
        if (cuentas[i].id != 0 &&
            strcmp(Usuario, cuentas[i].Nombre) == 0 &&
            strcmp(Contraseña, cuentas[i].Contraseña) == 0) {
            acceso = 1;
            Mostrar_Menu(Usuario,Contraseña); // Función que muestra el menú principal
            Menu_Usuario();
            printf("✅ ¡Bienvenido %s!\n", Usuario);
            break;
        }
    }

    if (!acceso) {
        printf("❌ Usuario o contraseña incorrectos.\n");
    }

    shmdt(cuentas);  // Desvincular memoria compartida
}

/// @brief esta funcion permite que el usuario se registre en una cuenta nueva
void Registro() {
    Cuenta cuenta;
    cuenta.Numero_transacciones = 0;
    system("clear");
    // Captura de datos
    printf("\n============================================================\n");
    printf("                 📝 REGISTRO DE NUEVO USUARIO               \n");
    printf("============================================================\n");

    printf("👤 Introduce tu Nombre: ");
    while (getchar() != '\n'); // Limpiar el buffer
    fgets(cuenta.Nombre, sizeof(cuenta.Nombre), stdin);
    cuenta.Nombre[strcspn(cuenta.Nombre, "\n")] = '\0';

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
            printf("\n⚠️ ¡Las contraseñas no coinciden! Inténtalo nuevamente.\n\n");
        }
    } while (strcmp(cuenta.Contraseña, cuenta.RepetirContraseña) != 0);

    printf("🏠 Introduce tu domicilio: ");
    fgets(cuenta.domicilio, sizeof(cuenta.domicilio), stdin);
    cuenta.domicilio[strcspn(cuenta.domicilio, "\n")] = '\0';

    printf("Introduce tu país de residencia: ");
    fgets(cuenta.pais, sizeof(cuenta.pais), stdin);
    cuenta.pais[strcspn(cuenta.pais, "\n")] = '\0';

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

    // Crear la memoria compartida
    key_t clave = ftok("shmfile", 65);
    int shmid = shmget(clave, sizeof(Cuenta) * MAX_CUENTAS, 0666);
    if (shmid == -1) {
        perror("No se pudo acceder a la memoria compartida");
        return;
    }

    Cuenta *cuentas = (Cuenta *) shmat(shmid, NULL, 0);
    if (cuentas == (void *) -1) {
        perror("shmat falló");
        return;
    }

    // Buscar el primer espacio libre
    int i;
    for (i = 0; i < MAX_CUENTAS; i++) {
        if (cuentas[i].id == 0) {
            cuenta.id = i + 1;
            cuentas[i] = cuenta;
            printf("✅ ¡Cuenta registrada exitosamente! ID: %d\n", cuenta.id);
            break;
        }
    }

    if (i == MAX_CUENTAS) {
        printf("❌ No hay espacio para más cuentas.\n");
    }

    shmdt(cuentas);  // Desvincular memoria compartida
}
