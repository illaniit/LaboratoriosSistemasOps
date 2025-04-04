
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
#include "Comun.h"
#include <sys/prctl.h> // Para PR_SET_PDEATHSIG
#include <ctype.h>
#include <sys/stat.h>
#define Cantidad_limite 1000
#define Punt_Archivo_Properties "Variables.properties"
#define MAX_LENGTH 256
#define ALERT_PIPE "/tmp/alerta_pipe"
int pipefd[2]; // Tubería que la declaramos para poder pasar informacion de monitor a banco y registrarlo en alertas .txt

// Definimos las funciones  que vamos a utilizar

void Menu_Procesos();
void CrearMonitor();
void limpiar(int sig);
void *LeerDeMonitor(void *arg);
void crear_named_pipe();

/// @brief este es el main en el cual leemos propertis con las variables
/// @return y devolvemos 0 si la ejecuccion ha sido exitosa
int main()
{
    Config config = leer_configuracion("variables.properties");
    // Lo primero abrimos el archivo de Properties y "nos traemos las variables"
    crear_named_pipe();
    // Iniciamos monitor para que encuentre  anomalias
    Inicializar_semaforos();

    CrearMonitor();
    // Cargamos el menu del usuario que se encuentra en init_cuentas.c donde cada usuario sera un hilo de ejecuccion
    Menu_Procesos();

    // Volvemos a llamara monitor para que encuentre las anomalias despues de que el usuario haya cerrado la sesion
    Destruir_semaforos();
    return 0;
}
void crear_named_pipe()
{
    // Comprobar si la named pipe ya existe
    if (access(ALERT_PIPE, F_OK) != -1)
    {
        // La named pipe ya existe, no es necesario crearla

        return;
    }

    // Crear la named pipe (FIFO) si no existe
    if (mkfifo(ALERT_PIPE, 0666) == -1)
    {
        perror("Error al crear la named pipe");
        exit(EXIT_FAILURE);
    }

    printf("Named pipe creada correctamente.\n");
}

/// @brief Esta funcion crea procesos en una nueva terminal que lo que
/// hacen es ejecutar la instancia de usuario y ejecutar el codigo del mismo

#define MAX_HIJOS 100 // Máximo número de hijos permitidos

// Array de PIDs de los hijos
pid_t hijos[MAX_HIJOS];
int num_hijos = 0; // Contador de hijos creados

// Función para matar todos los procesos hijos usando system()
void matar_hijos()
{
    printf("\n Cerrando sesiones...\n");

    for (int i = 0; i < num_hijos; i++)
    {
        if (hijos[i] > 0) // Verifica que el PID sea válido
        {
            // Matar proceso hijo directamente
            char comando[100];
            snprintf(comando, sizeof(comando), "kill -9 %d", hijos[i]);
            system(comando);
            sleep(0.5);
        }
    }
}

void Menu_Procesos()
{
    char respuesta;
    int continuar = 1;

    while (continuar)
    {
        if (num_hijos >= MAX_HIJOS)
        {
            printf("Se ha alcanzado el límite de usuarios.\n");
            break;
        }

        pid_t pid = fork(); // Crear un proceso hijo

        if (pid == 0)
        {
            // Crear un nuevo grupo de procesos para el hijo
            setpgid(0, 0);

            // Ejecutar en una nueva terminal y obtener el PID real
            char comando[200];
            snprintf(comando, sizeof(comando),
                     "gnome-terminal -- sh -c 'echo $$ > /tmp/pid_%d.txt; gcc init_cuentas.c Usuario.c Transferencia.c ConsultarDatos.c ExtraerDinero.c IntroducirDinero.c Comun.c -o usuario && ./usuario'", getpid());

            system(comando);

            exit(0); // El hijo termina aquí, la terminal sigue corriendo
        }
        else if (pid > 0)
        {
            // Esperar un momento para que el archivo con el PID se cree
            sleep(1);

            // Leer el PID real del hijo desde el archivo
            char filename[50];
            snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", pid);
            FILE *fp = fopen(filename, "r");
            if (fp)
            {
                fscanf(fp, "%d", &hijos[num_hijos]);
                fclose(fp);
            }
            else
            {
                perror("Error al leer el archivo de PID");
                hijos[num_hijos] = pid; // Si falla, usar el PID de fork()
            }

            num_hijos++;

            // Preguntar si desea aceptar otro usuario
            printf("¿Desea aceptar otro usuario? (s/n): ");
            scanf(" %c", &respuesta);
            if (respuesta != 's' && respuesta != 'S')
            {
                continuar = 0;
            }
        }
        else
        {
            perror("Error en fork");
            exit(EXIT_FAILURE);
        }
    }

    // El padre espera a que todos los hijos terminen
    for (int i = 0; i < num_hijos; i++)
    {
        if (hijos[i] > 0)
        {
            waitpid(hijos[i], NULL, 0);
        }
    }

    // Cuando el padre termina, mata a todos los hijos
    matar_hijos();
}

// Esta funcion se encarga de "pegar" la alerta en el pipe para poder pasarlo al proceso padre

// Función para crear el proceso Monitor y manejar alertas en tiempo real
void CrearMonitor()
{
    if (pipe(pipefd) == -1)
    {
        perror("Error al crear la tubería");
        return;
    }

    pid_t Monitor = fork(); // Crea el proceso monitor

    if (Monitor < 0)
    {
        perror("Error al crear el proceso Monitor");
        Escribir_registro("Fallo al crear el proceso monitor");
        return;
    }

    if (Monitor == 0)
    {

        execl("./monitor", "monitor", NULL); // Ejecuta el proceso monitor
        // Si execl falla:
        perror("Error al ejecutar monitor");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Código del padre
        while (1)
        {
            // Lanza un hilo para leer las alertas
            pthread_t hilo_alertas;
            pthread_create(&hilo_alertas, NULL, LeerDeMonitor, NULL);
            pthread_detach(hilo_alertas); // Hilo que no bloquea la ejecución principal
            sleep(5);
        }
    }
}
void *LeerDeMonitor(void *arg)
{

    char buffer[256]; // Buffer para almacenar las alertas
    ssize_t leido;

    // Abrimos la named pipe en modo de lectura
    int fd = open(ALERT_PIPE, O_RDONLY);
    if (fd == -1)
    {
        perror("Error al abrir la named pipe");
        return NULL;
    }

    while ((leido = read(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[leido] = '\0'; // Asegura que el string esté bien terminado

        // Muestra la alerta parpadeando durante 3 segundos
        for (int i = 0; i < 3; i++)
        {
            // Cambiar el color para hacerla parpadear
            printf("\033[1;31m%s\033[0m\n", buffer); // Rojo brillante
            usleep(500000);                          // Pausa de 0.5 segundos

            // Limpiar la pantalla para crear el efecto de parpadeo
            printf("\033[H\033[J");
            usleep(500000); // Pausa de 0.5 segundos para crear el parpadeo
        }

        // Imprimir normalmente al final para dejar de parpadear
        printf("\033[0m%s\n", buffer);
    }

    if (leido == -1)
    {
        perror("Error al leer desde la named pipe");
    }

    // Cierra la named pipe cuando haya terminado de leer
    close(fd);
}