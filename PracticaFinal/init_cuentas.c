
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define Punt_Archivo_Properties "Variables.properties"
#include "init_cuentas.h"
#include "Usuario.h"
#define MAX_LENGTH 256
#define NUM_USUARIOS 5
#define MAX_USUARIOS 5
#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>

void *Menu_Usuario();

sem_t semaforo;

void Escribir_registro2(const char *mensaje_registro)
{
    // declaramos la variable time_t
    time_t t;
    struct tm *tm_info;
    char buffer[30]; // Para almacenar la fecha y hora formateadas

    // Obtiene la hora actual
    time(&t);
    tm_info = localtime(&t);

    // Formatea la fecha y hora en "YYYY-MM-DD HH:MM:SS"
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

    // Abre el archivo en modo "a" para añadir sin sobrescribir
    FILE *ArchivoDeRegistro = fopen("registro.log", "a");
    if (!ArchivoDeRegistro)
    {
        perror("Error al abrir el archivo de registro");
        return;
    }

    // Escribe la fecha, la hora y el mensaje en el archivo
    fprintf(ArchivoDeRegistro, "[%s] %s\n", buffer, mensaje_registro);

    // Cierra el archivo
    fclose(ArchivoDeRegistro);
}

// Función para abrir y leer el archivo de propiedades
// Esta funcion nos permite cargar las variables que usemos en el codigo para que sea mas accesible
void AbrirPropertis2()
{
    // Llamamos al registro.log
    Escribir_registro2("Se ha abierto el archivo de properties de init cuentas");
    char *key, *value;
    char line[MAX_LENGTH];
    char username[MAX_LENGTH] = {0};
    char password[MAX_LENGTH] = {0};

    FILE *ArchivoPro = fopen(Punt_Archivo_Properties, "r");

    if (!ArchivoPro)
    {
        perror("Error al abrir el archivo de propiedades");
        Escribir_registro2("Fallo al abrir el arhivo de properties");
        return;
    }

    while (fgets(line, MAX_LENGTH, ArchivoPro))
    {

        line[strcspn(line, "\n")] = 0;
        key = strtok(line, "=");
        value = strtok(NULL, "=");
        if (key && value)
        {
            if (strcmp(key, "username") == 0)
            {
                strncpy(username, value, MAX_LENGTH - 1);
            }
            else if (strcmp(key, "password") == 0)
            {
                strncpy(password, value, MAX_LENGTH - 1);
            }
        }
    }

    fclose(ArchivoPro);
}
int contador = 1;
void Menu_Procesos()
{

    char respuesta;
    sem_t *sem; // Declaración del semáforo
    int continuar = 1;

    // Crear un semáforo nombrado (SEM_UNDO garantiza que se libere cuando el proceso termine)
    sem = sem_open("/semaforo_banco", O_CREAT, 0644, 0); // Inicializa el semáforo a 0
    if (sem == SEM_FAILED)
    {
        perror("Error al crear el semáforo");
        exit(1);
    }

    while (continuar)
    {
        printf("Esperando conexión de un nuevo usuario...\n");

        pid_t pid = fork(); // Crear un proceso hijo
        if (pid == 0)
        {                   // Si estamos en el proceso hijo
            Menu_Usuario(); // Llamamos a la función que maneja el usuario
            sem_post(sem);  // Liberar el semáforo para que el padre sepa que terminó
            exit(0);        // Termina el proceso hijo
        }
        else if (pid > 0)
        {                  // Si estamos en el proceso padre
            sem_wait(sem); // Espera a que el hijo termine (semaforo bloqueado hasta que el hijo lo libere)

            // Preguntar si desea aceptar otro usuario después de que el hijo termine
            printf("¿Desea aceptar otro usuario? (s/n): ");
            scanf(" %c", &respuesta); // Espacio antes de %c para consumir el salto de línea pendiente
            if (respuesta != 's' && respuesta != 'S')
            {
                continuar = 0; // Si no quiere otro usuario, salimos del bucle
            }
        }
        else
        {
            perror("Error en fork");
            exit(EXIT_FAILURE);
        }
    }

    // El padre espera a que todos los procesos hijos terminen antes de cerrar
    while (wait(NULL) > 0)
        ; // Espera a que todos los hijos terminen

    // Cerramos el semáforo y lo eliminamos
    sem_close(sem);
    sem_unlink("/semaforo_banco");

   
}


// Función para cada usuario ejecutándose en un hilo
void *Menu_Usuario()
{
    contador++;
    // printf("👤 Usuario %d ha iniciado el menú.\n", usuario_id);
    Escribir_registro2("Se ha accedido al menú de entrada");
    AbrirPropertis2();

    int Eleccion;
    do
    {
        Escribir_registro2("Se ha abierto el menú de inicio de sesión");

        printf("\n------------------ Elija una opción ----------------------\n");
        printf("|   1. Inicio de sesión                                   |\n");
        printf("|   2. Registro                                          |\n");
        printf("|       Pulse una opción (1/2):                          |\n");
        printf("---------------------------------------------------------\n");
        scanf("%d", &Eleccion);

        if (Eleccion == 1)
        {
            sem_wait(&semaforo); // Bloqueamos el semáforo
            InicioDeSesion();
            sem_post(&semaforo); // Liberamos el semáforo
            Escribir_registro2("El usuario ha elegido iniciar sesión");
        }
        else if (Eleccion == 2)
        {
            Registro();
            Escribir_registro2("El usuario ha elegido la opción de registro");
            printf("volviendo a tu menu...");
            sleep(4);
            Menu_Usuario();
        }
    } while (Eleccion != 1 && Eleccion != 2);

    return NULL;
}
void InicioDeSesion()
{
    FILE *archivo = fopen("usuarios.txt", "r"); // Abrimos el archivos usuarios.dat
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
                        Mostrar_Menu(); // Llama a la funcion de usuarios.c
                        Escribir_registro2("Se ha accedido al sistema");
                    }
                    else
                    {
                        printf("Contraseña incorrecta");
                        // abrir el registros.log y introducir que el usuario ha intentado iniciar sesion
                        Escribir_registro2("El usuario ha intentido iniciar sesion y ha tenido la contraseña incorrecta");
                    }
                }
                else
                {
                    printf("Nombre de usuario o contraseña incorrectas");
                    // abrir el registros.log y introducir que el usurio ha intentado iniciar sesion
                    Escribir_registro2("El usuario ha intentado iniciar sesion y ha falldao el usuaio o la contraseña");
                }
            }
        }

    } while (Contraseña == passwd && Usuario == user); // si la contraseña o el usuario no son correctas volvemos a mostrar el menu y este mensaje , tambien se podria hacer un contandor para que te avise de que has fallado muchas veces o algo asi

    fclose(archivo); // cerramos el archivo con fclose
}
void Registro() {
    Escribir_registro2("El usuario ha entrado en la sección de registro");

    struct Cuenta {
        int id;                    // ID de la cuenta
        char Nombre[50];            // Nombre de usuario
        char Apellido[50];          // Apellido del usuario
        char domicilio[100];        // Domicilio
        char pais[50];              // País
        int saldo;                  // Saldo inicial
        int Numero_transacciones;   // Número de transacciones
    };

    struct Cuenta cuenta;
    cuenta.id = 1; // Comenzamos en 1 en caso de ser el primer usuario
    cuenta.Numero_transacciones = 0;

    printf("-------- Registro -------\n");

    printf("Introduce tu Nombre: ");
    while (getchar() != '\n');  // Limpieza del buffer de entrada
    fgets(cuenta.Nombre, sizeof(cuenta.Nombre), stdin);
    cuenta.Nombre[strcspn(cuenta.Nombre, "\n")] = '\0'; // Eliminar salto de línea

    printf("Introduce tus Apellidos: ");
    fgets(cuenta.Apellido, sizeof(cuenta.Apellido), stdin);
    cuenta.Apellido[strcspn(cuenta.Apellido, "\n")] = '\0';

    printf("Introduce tu domicilio: ");
    fgets(cuenta.domicilio, sizeof(cuenta.domicilio), stdin);
    cuenta.domicilio[strcspn(cuenta.domicilio, "\n")] = '\0';

    printf("Introduce tu país de residencia: ");
    fgets(cuenta.pais, sizeof(cuenta.pais), stdin);
    cuenta.pais[strcspn(cuenta.pais, "\n")] = '\0';

    // Validación del saldo ingresado
    do {
        printf("Introduce el saldo inicial (debe ser un número positivo): ");
        if (scanf("%d", &cuenta.saldo) != 1) {
            printf("Error: Debes ingresar un número válido.\n");
            while (getchar() != '\n'); // Limpiar el buffer de entrada
        } else if (cuenta.saldo < 0) {
            printf("Error: El saldo no puede ser negativo.\n");
        }
    } while (cuenta.saldo < 0);

    while (getchar() != '\n'); // Limpiar buffer tras scanf()

    printf("Verificando que el saldo que ha introducido es correcto...\n");
    sleep(2);

    // Abrir el archivo en modo lectura para contar usuarios existentes
    FILE *usuarios = fopen("usuarios.txt", "r+");
    if (!usuarios) {
        usuarios = fopen("usuarios.txt", "w"); // Si no existe, crearlo
        if (!usuarios) {
            perror("Error al abrir el archivo de usuarios");
            return;
        }
    }

    char linea[255];
    while (fgets(linea, sizeof(linea), usuarios) != NULL) {
        cuenta.id++; // Incrementar ID basado en el número de líneas
    }

    // Escribir los datos en el archivo
    fprintf(usuarios, "%d | %s | %s | %s | %s | %d | %d\n", 
            cuenta.id, cuenta.Nombre, cuenta.Apellido, cuenta.domicilio, 
            cuenta.pais, cuenta.saldo, cuenta.Numero_transacciones);

    fclose(usuarios); // Cerrar el archivo

    Escribir_registro2("Se ha registrado un nuevo usuario en el sistema");

    printf("\nRegistro completado con éxito. Tu ID es: %d\n", cuenta.id);
}
