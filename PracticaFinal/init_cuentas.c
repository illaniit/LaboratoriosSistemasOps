

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "Cuenta.h"
#include "Comun.h"
#define MAX_CUENTAS 100

Cuenta *cuentas; // Definición del puntero global

int shmid = -1;
// Prototipos
void Menu_Usuario();
void Registro();
void InicioDeSesion();
void Mostrar_Menu(char *Usuario, char *Contraseña);
void CrearArchivoTransacciones(int id,char *Nombre);

int main()
{
    // Conexión con la memoria compartida creada por banco.c
    key_t clave = ftok("Cuenta.h", 65);
    printf("[DEBUG] Clave generada: %d\n", clave);
    sleep(1);
    int shmid = shmget(clave, sizeof(Cuenta) * MAX_CUENTAS, 0666);
    if (shmid == -1)
    {
        perror("❌ No se pudo acceder a la memoria compartida");
        return 1;
    }

    cuentas = (Cuenta *)shmat(shmid, NULL, 0);
    if (cuentas == (void *)-1)
    {
        perror("❌ Error al enlazar la memoria compartida");
        return 1;
    }

    // Mapear memoria
   
    Inicializar_semaforos();
    sem_usuarios = sem_open("/sem_usuarios", 0);
    sem_transacciones = sem_open("/sem_transacciones", 0);

    Menu_Usuario();

    shmdt(cuentas);
    return 0;
}
void CrearArchivoTransacciones(int id,char *nombre)
{
    char path[256];
    char log_path[300];

    // Obtener ruta del directorio HOME
    const char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "No se pudo obtener la variable de entorno HOME\n");
        return;
    }

    // Construir ruta del directorio: ~/transacciones/<id>
    snprintf(path, sizeof(path), "%s/transacciones/%d", home, id);

    // Crear carpeta ~/transacciones si no existe
    char transacciones_dir[256];
    snprintf(transacciones_dir, sizeof(transacciones_dir), "%s/transacciones", home);
    mkdir(transacciones_dir, 0777);

    // Crear subdirectorio del usuario
    if (mkdir(path, 0777) == -1 && errno != EEXIST) {
        perror("Error al crear el directorio del usuario");
        return;
    }

    // Crear archivo transacciones.log dentro del directorio
    snprintf(log_path, sizeof(log_path), "%s/transacciones.log", path);
    FILE *archivo = fopen(log_path, "a");
    if (!archivo) {
        perror("Error al crear transacciones.log");
        return;
    }
    fprintf(archivo, "Bienvenido a tu extracto\n");
    fprintf(archivo, "Registro de transacciones de %s\n", nombre);
    fclose(archivo);

    printf("Directorio y log creados en: %s\n", log_path);
}

void Menu_Usuario()
{
    int Eleccion;

    do
    {
        system("clear");

        printf("\n==========================================\n");
        printf("            💰 BANCO 💰                     \n");
        printf("==========================================  \n");
        printf(" 1️⃣  Inicio de sesión\n");
        printf(" 2️⃣  Registro\n");
        printf(" 3️⃣  Salir\n");
        printf("------------------------------------------\n");
        printf("\n  Pulse una opción: ");

        scanf("%d", &Eleccion);
        switch (Eleccion)
        {
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

void InicioDeSesion()
{
    system("clear");
    char Usuario[50], Contraseña[50];
    int acceso = 0;

    Cuenta *cuenta = cuentas;

    printf("👤 Nombre de usuario: ");
    scanf("%49s", Usuario);
    printf("🔑 Contraseña: ");
    scanf("%49s", Contraseña);

    for (int i = 0; i < MAX_CUENTAS; i++)
    {
        if (cuenta[i].id != 0 &&
            strcmp(Usuario, cuenta[i].Nombre) == 0 &&
            strcmp(Contraseña, cuenta[i].Contraseña) == 0)
        {
            acceso = 1;
            Mostrar_Menu(Usuario, Contraseña);
            printf("✅ ¡Bienvenido %s!\n", Usuario);
            break;
        }
    }

    if (!acceso)
    {
        printf("❌ Usuario o contraseña incorrectos.\n");
        sleep(2);
    }
}

void Registro()
{
    Cuenta cuenta;
    cuenta.Numero_transacciones = 0;
    system("clear");

    printf("\n============================================================\n");
    printf("                 📝 REGISTRO DE NUEVO USUARIO               \n");
    printf("============================================================\n");

    printf("👤 Introduce tu Nombre: ");
    while (getchar() != '\n')
        ;
    fgets(cuenta.Nombre, sizeof(cuenta.Nombre), stdin);
    cuenta.Nombre[strcspn(cuenta.Nombre, "\n")] = '\0';

    printf("Introduce tus Apellidos: ");
    fgets(cuenta.Apellido, sizeof(cuenta.Apellido), stdin);
    cuenta.Apellido[strcspn(cuenta.Apellido, "\n")] = '\0';

    do
    {
        printf("🔑 Escribe tu contraseña: ");
        fgets(cuenta.Contraseña, sizeof(cuenta.Contraseña), stdin);
        cuenta.Contraseña[strcspn(cuenta.Contraseña, "\n")] = '\0';

        printf(" Repite tu contraseña: ");
        fgets(cuenta.RepetirContraseña, sizeof(cuenta.RepetirContraseña), stdin);
        cuenta.RepetirContraseña[strcspn(cuenta.RepetirContraseña, "\n")] = '\0';

        if (strcmp(cuenta.Contraseña, cuenta.RepetirContraseña) != 0)
        {
            printf("\n⚠️ ¡Las contraseñas no coinciden! Inténtalo nuevamente.\n\n");
        }
    } while (strcmp(cuenta.Contraseña, cuenta.RepetirContraseña) != 0);

    printf("🏠 Introduce tu domicilio: ");
    fgets(cuenta.domicilio, sizeof(cuenta.domicilio), stdin);
    cuenta.domicilio[strcspn(cuenta.domicilio, "\n")] = '\0';

    printf("Introduce tu país de residencia: ");
    fgets(cuenta.pais, sizeof(cuenta.pais), stdin);
    cuenta.pais[strcspn(cuenta.pais, "\n")] = '\0';

    do
    {
        printf("💰 Introduce el saldo inicial (debe ser un número positivo): ");
        if (scanf("%d", &cuenta.saldo) != 1)
        {
            printf("\n❌ Error: Debes ingresar un número válido.\n\n");
            while (getchar() != '\n')
                ;
        }
        else if (cuenta.saldo < 0)
        {
            printf("\n❌ Error: El saldo no puede ser negativo.\n\n");
        }
    } while (cuenta.saldo < 0);

    while (getchar() != '\n')
        ;

    int i;
    for (i = 0; i < MAX_CUENTAS; i++)
    {
        if (cuentas[i].id == 0)
        {
            cuenta.id = i + 1;
            cuentas[i] = cuenta;
            printf("✅ ¡Cuenta registrada exitosamente! ID: %d\n", cuenta.id);
            sleep(2);
            break;
        }
    }

    if (i == MAX_CUENTAS)
    {
        printf("❌ No hay espacio para más cuentas.\n");
        sleep(2);
    }

    CrearArchivoTransacciones(cuenta.id,cuenta.Nombre);
    Escribir_registro("Se ha registrado un nuevo usuario en el sistema");
}
