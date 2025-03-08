
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
#define MONTO_LIMITE 1000
#define Punt_Archivo_Properties "Variables.properties"
#define MAX_LENGTH 256
#include "init_cuentas.h"
int pipefd[2]; // Tubería que la declaramos para poder pasar informacion de monitor a banco yregistrarlo en alertas .txt



// Definimos las funciones  que vamos a utilizar

void AbrirPropertis();
void CrearMonitor();
void *detectar_transacciones(void *arg);
void enviar_alerta(const char *mensaje);
void limpiar(int sig);
void registrar_alerta(const char *mensaje);

int main()
{
    //Lo primero abrimos el archivo de Properties y "nos traemos las variables"
    AbrirPropertis();
    //Iniciamos monitor para que encuentre  anomalias 
    CrearMonitor();

    //Cargamos el menu del usuario que se encuentra en init_cuentas.c donde cada usuario sera un hilo de ejecuccion
    Menu_Usuario();

    //Volvemos a llamara monitor para que encuentre las anomalias despues de que el usuario haya cerrado la sesion
    CrearMonitor();
    return 0;
}

// Función para abrir y leer el archivo de propiedades 
//Esta funcion nos permite cargar las variables que usemos en el codigo para que sea mas accesible
void AbrirPropertis()
{

    char *key, *value;
    char line[MAX_LENGTH];
    char username[MAX_LENGTH] = {0};
    char password[MAX_LENGTH] = {0};

    FILE *ArchivoPro = fopen(Punt_Archivo_Properties, "r");

    if (!ArchivoPro)
    {
        perror("Error al abrir el archivo de propiedades");
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

// Esta funcion se encarga de "pegar" la alerta en el pipe para poder pasarlo al proceso padre

void enviar_alerta(const char *mensaje)
{
    char auxiliar[256];
    snprintf(auxiliar, sizeof(auxiliar), "%s\n", mensaje); // esta funcion nos permite agregar un \n en el archivo de texto para que sea entendible y legible
    write(pipefd[1], auxiliar, strlen(auxiliar));
}

// Mediante señales limpiamos la tuberia 
void limpiar(int sig)
{
    close(pipefd[0]);
    close(pipefd[1]);
    exit(0);
}

// Esta funcion abre/ crea si no existe el archivo de texto donde se almacenan las alertas y escribe las alertas que se encuentren en la variable mensaje
void registrar_alerta(const char *mensaje)
{

    FILE *archivo_alertas = fopen("alertas.txt", "a"); // Abrimos el fichero
    if (archivo_alertas)
    {
        fprintf(archivo_alertas, "%s\n", mensaje); // escribimos el mensaje
        fclose(archivo_alertas); // cerramos el archivo
    }
    else
    {
        perror("Error al abrir alertas.txt");
    }
}

// Función para detectar transacciones sospechosas
// Lo que hago en esta funcion es abrir los dos ficheritos y mirar si hay cosas raras , enviarlas a la funcion que ya me habia creado
// , el trabajo duro ya estaba hecho =)
void *detectar_transacciones(void *arg)
{
    signal(SIGINT, limpiar); // limpiamos la tuberia 
    FILE *archivo = fopen("transaciones.txt", "r"); // abrimos el archivo transacciones para buscar anomalias en el
    if (!archivo)
    {
        perror("Error al abrir el archivo de transacciones"); // Comprobamos que el archivo no de error 
        return NULL; // devolvemos null en este caso
    }
    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) // leemos el archivo transacciones.txt
    {
        int id, saldo1, saldo2, saldo_final; // declaramos las variables 
        char tipo[20], cuenta1[20], cuenta2[20];
        if (sscanf(linea, "%d,%19[^,],%19[^,],%19[^,],%d,%d,%d", &id, tipo, cuenta1, cuenta2, &saldo1, &saldo2, &saldo_final) == 7) // leemos los campos correspondientes
        {
            if (strcmp(tipo, "retiro") == 0 || strcmp(tipo, "ingreso") == 0) // comprobamso que los campos sean validos es decir si es un ingreso o un retiro esos campos de cuenta 2 y saldo 2 deben estar vacios
            {
                if (strlen(cuenta2) > 0 || saldo2 != 0) // comprobamos que  
                {
                    enviar_alerta("Transacción inválida, campos incorrectos en retiro o ingreso");
                }
            }
            if (saldo1 < 0 || saldo2 < 0 || saldo_final < 0) // comprobamso que cualquiera de los 3 campos que indican saldo sean positivos
            {

                enviar_alerta("Saldo negativo detectado");
            }
        }
    }

    fclose(archivo); // cerramos el archivo

    // Verificación de usuarios.dat
     // hacemos exactamente lo mismo pero con el fichero que almacena los usuarios
    FILE *usuarios = fopen("usuarios.dat", "r"); //abrimos el archivo
    if (!usuarios)
    {
        perror("Error al abrir el archivo de usuarios");
        return NULL;
    }

    while (fgets(linea, sizeof(linea), usuarios)) // leemos el archivo
    {
        int id, saldo, num_transacciones;
        char titular[50];
        if (sscanf(linea, "%d,%49[^,],%d,%d", &id, titular, &saldo, &num_transacciones) == 4)
        {
            if (saldo < 0)
            {
                enviar_alerta("Usuario con saldo negativo"); // enviamos alerta si un usuario tiene el saldo negativo
            }

            if (num_transacciones < 0)
            {
                enviar_alerta("Número de transacciones inválido"); // enviamos alerta si el numero de transacciones es menor que cero
            }
        }
    }

    fclose(usuarios); // cerramos el archivo
    return NULL; // ya que es de tipo void
}
// Función para crear el proceso Monitor y manejar alertas en tiempo real
void CrearMonitor()
{
    if (pipe(pipefd) == -1)
    {
        perror("Error al crear la tubería");
        exit(1);
    }

    pid_t Monitor = fork();
    if (Monitor < 0)
    {
        perror("Error al crear el proceso Monitor");
        return;
    }
    if (Monitor == 0)
    {
        // Código del hijo (Monitor)
        close(pipefd[0]); // Cierra lectura en el hijo
        pthread_t Transacciones;
        if (pthread_create(&Transacciones, NULL, detectar_transacciones, NULL) != 0) // crea el hilo y llama a detecatar transacciones
        {
            perror("Error al crear el hilo de transacciones"); // comprobamos que el hilo se haya creado correctamente
            exit(1);
        }
        pthread_join(Transacciones, NULL); //cerramos el hilo
        close(pipefd[1]); // Cerrar escritura cuando termine
        exit(0);
    }
    else
    {
        // Código del padre (lector de alertas y almacenamiento en archivo)
        close(pipefd[1]); // Cierra escritura en el padre
        char buffer[256]; // declaramos una variable para leer de la pipe
        int bytes_leidos;
       // printf("⚠ Monitoreo de alertas iniciado... (Guardando en alertas.txt)\n");
        while (1)
        {
            bytes_leidos = read(pipefd[0], buffer, sizeof(buffer) - 1); // lee la pipe y lo almacena en el buffer
            if (bytes_leidos > 0)
            {
                buffer[bytes_leidos] = '\0'; // Convertir a string, añade el barra cero al final para indicar el final de la cadena en el ultimo char
                registrar_alerta(buffer); // manda a registrar alerta con lo que tiene la  pipe
            }
            else if (bytes_leidos == 0)
            {
                break; // Salir si no hay más datos
            }
            else
            {
                perror("Error al leer la tubería"); // da error si se lee mal la tuberia
                break;
            }
        }
        close(pipefd[0]); // Cierra lectura al terminar
    }
}
