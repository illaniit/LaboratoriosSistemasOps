

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
void CrearArchivoTransacciones(int id, const char *Nombre);

int main()
{
    // Conexión con la memoria compartida creada por banco.c
    key_t clave = ftok("Cuenta.h", 66);
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

void CrearArchivoTransacciones(int id, const char *nombre)
{
    char path_base[] = "./transacciones";
    char path_usuario[256];
    char log_path[300];

    // Crear carpeta ./transacciones si no existe
    if (mkdir(path_base, 0777) == -1 && errno != EEXIST)
    {
        perror("Error al crear el directorio ./transacciones");
        return;
    }

    // Construir ruta del usuario: ./transacciones/<id>
    snprintf(path_usuario, sizeof(path_usuario), "%s/%d", path_base, id);

    // Crear subcarpeta ./transacciones/<id>
    if (mkdir(path_usuario, 0777) == -1 && errno != EEXIST)
    {
        perror("Error al crear el directorio del usuario");
        return;
    }

    // Crear archivo: ./transacciones/<id>/transacciones.log
    snprintf(log_path, sizeof(log_path), "%s/transacciones.log", path_usuario);

    FILE *archivo = fopen(log_path, "a");
    if (!archivo)
    {
        perror("Error al crear transacciones.log");
        return;
    }

    fprintf(archivo, "Bienvenido a tu extracto\n");
    fprintf(archivo, "Registro de transacciones de %s\n", nombre);
    fclose(archivo);

    printf("Directorio y archivo creados en: %s\n", log_path);
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

    // Buscar en memoria compartida
    for (int i = 0; i < MAX_CUENTAS; i++)
    {
        if (cuenta[i].id != 0 &&
            strcmp(Usuario, cuenta[i].Nombre) == 0 &&
            strcmp(Contraseña, cuenta[i].Contraseña) == 0)
        {
            acceso = 1;
            Mostrar_Menu(Usuario, Contraseña);
            printf("✅ ¡Bienvenido %s!\n", Usuario);
            return;
        }
    }

    // Si no se encuentra en memoria compartida, buscar en el archivo
    FILE *archivo = fopen("cuentas.txt", "r");
    if (!archivo)
    {
        perror("❌ Error al abrir el archivo cuentas.txt");
        return;
    }

    char linea[256];
    Cuenta cuenta_temp;
    
    limpiar_cadena(Usuario);
    limpiar_cadena(Contraseña);

    while (fgets(linea, sizeof(linea), archivo))
    {
        if (sscanf(linea, "%d|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d|%[^|]|%s",
                   &cuenta_temp.id,
                   cuenta_temp.Nombre,
                   cuenta_temp.Apellido,
                   cuenta_temp.Contraseña,
                   cuenta_temp.domicilio,
                   cuenta_temp.pais,
                   &cuenta_temp.saldo,
                   cuenta_temp.fecha,
                   cuenta_temp.hora) == 9)
        {
            if (strcmp(Usuario, cuenta_temp.Nombre) == 0 &&
                strcmp(Contraseña, cuenta_temp.Contraseña) == 0)
            {
                // Usuario encontrado en el archivo
                acceso = 1;

                // Contar el número de cuentas en memoria compartida
                int numeroCuentas = 0;
                for (int i = 0; i < MAX_CUENTAS; i++)
                {
                    if (cuentas[i].id != 0)
                    {
                        numeroCuentas++;
                    }
                }

                if (numeroCuentas >= MAX_CUENTAS)
                {
                    // Desalojar el usuario más antiguo de la memoria compartida
                    int oldest_index = -1;
                    int oldest_year = 9999, oldest_month = 12, oldest_day = 31;

                    for (int i = 0; i < MAX_CUENTAS; i++)
                    {
                        if (cuentas[i].id != 0)
                        {
                            int year, month, day;
                            if (sscanf(cuentas[i].fecha, "%d-%d-%d", &year, &month, &day) == 3)
                            {
                                if (year < oldest_year ||
                                    (year == oldest_year && month < oldest_month) ||
                                    (year == oldest_year && month == oldest_month && day < oldest_day))
                                {
                                    oldest_year = year;
                                    oldest_month = month;
                                    oldest_day = day;
                                    oldest_index = i;
                                }
                            }
                        }
                    }

                    if (oldest_index != -1)
                    {
                        cuentas[oldest_index] = cuenta_temp; // Insertar el usuario en memoria compartida
                    }
                }
                else
                {
                    // Insertar el usuario en la primera posición libre
                    for (int i = 0; i < MAX_CUENTAS; i++)
                    {
                        if (cuentas[i].id == 0)
                        {
                            cuentas[i] = cuenta_temp;
                            break;
                        }
                    }
                }

                Mostrar_Menu(Usuario, Contraseña);
                printf("✅ ¡Bienvenido %s!\n", Usuario);
                fclose(archivo);
                return;
            }
        }
    }

    fclose(archivo);

    if (!acceso)
    {
        printf("❌ Usuario o contraseña incorrectos.\n");
        sleep(2);
    }
}

void Registro()
{
    Cuenta cuenta;
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

    // Abrir el archivo cuentas.txt para lectura y escritura
    FILE *archivo = fopen("cuentas.txt", "a+");
    if (!archivo)
    {
        perror("❌ Error al abrir el archivo cuentas.txt");
        return;
    }

    // Contar las líneas existentes para asignar el ID
    int id = 1;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), archivo))
    {
        id++;
    }
    cuenta.id = id;

    // Obtener la fecha y hora actual
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char fecha[32], hora[32];
    strftime(fecha, sizeof(fecha), "%Y-%m-%d", tm_info);
    strftime(hora, sizeof(hora), "%H:%M:%S", tm_info);

    // Escribir los datos de la cuenta en el archivo
    fprintf(archivo, "%d|%s|%s|%s|%s|%s|%d|%s|%s\n",
            cuenta.id,
            cuenta.Nombre,
            cuenta.Apellido,
            cuenta.Contraseña,
            cuenta.domicilio,
            cuenta.pais,
            cuenta.saldo,
            fecha,
            hora);

    fclose(archivo);
    printf("✅ Registro exitoso. Los datos se han guardado en cuentas.txt\n");
    CrearArchivoTransacciones(cuenta.id, cuenta.Nombre);
    Escribir_registro("Se ha registrado un nuevo usuario en el sistema");
}