
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
#define MAX_HIJOS 100

/// @brief Esta funcion crea procesos en una nueva terminal que lo que
/// hacen es ejecutar la instancia de usuario y ejecutar el codigo del mismo

// Función para matar todos los procesos hijos usando system()
pid_t hijos[MAX_HIJOS];
int num_hijos = 0; // Contador de hijos creados
int temp[100];     // Arreglo donde almacenarás los números

// Definimos las funciones  que vamos a utilizar

void Menu_Procesos();
void CrearMonitor();
void limpiar(int sig);
void *LeerDeMonitor(void *arg);
void matar_hijos();
void leer_alerta_cola(int sig);

/// @brief este es el main en el cual leemos propertis con las variables
/// @return y devolvemos 0 si la ejecuccion ha sido exitosa
int main()
{

    Inicializar_semaforos();

    Escribir_registro("Se ha accedido al main de banco y se han incializado los semaforos");

    crear_cola_mensajes();

    Config config = leer_configuracion("variables.properties");
    // Lo primero abrimos el archivo de Properties y "nos traemos las variables"
    signal(SIGUSR1, leer_alerta_cola); // Manejar señal del monitor

    CrearMonitor(); // Lanzar monitor

    Menu_Procesos(); // Ejecutar usuarios

    Destruir_semaforos();

    return 0;
}

/// @brief 
/// @param sig 
void leer_alerta_cola(int sig)
{
    Escribir_registro("Ha llegado una alerta de monitor mediante una señal en banco.c");
    MensajeAlerta msg;
    int id_cola = msgget(CLAVE_COLA, 0666);
    if (id_cola == -1)
    {
        perror("Error al obtener cola");
        return;
    }

    if (msgrcv(id_cola, &msg, sizeof(msg.texto), TIPO_ALERTA, IPC_NOWAIT) == -1)
    {
        perror("No se pudo leer alerta de la cola");
        return;
    }
    Escribir_registro("se ha accedido a la cola");
    // Abrir el archivo alertas.log en modo append
    FILE *archivo = fopen("alertas.log", "a");
    if (!archivo)
    {
        perror("No se pudo abrir alertas.log");
        return;
    }

    // Escribir la alerta en el archivo
    fprintf(archivo, "🚨 ALERTA DEL MONITOR 🚨\n%s\n", msg.texto);
    fclose(archivo);
    Escribir_registro("Se ha escrito ene  el archvio una alerta");

    printf("🚨 Se ha registrado una nueva alerta\n");

    sleep(4);
    Escribir_registro("Se muestra que se ha registrado una alerta");

    // Mueve el cursor una línea arriba y limpia esa línea
    printf("\033[F");  // Mueve el cursor una línea arriba
    printf("\033[2K"); // Borra toda la línea
    fflush(stdout);    // Asegura que el borrado se aplique antes de terminar
}

void matar_hijos()
{
    printf("\n Cerrando sesiones...\n");

    for (int i = 0; i < num_hijos; i++)
    {

        // Matar proceso hijo directamente
        char comando[100];
        snprintf(comando, sizeof(comando), "kill -9 %d", temp[i]);
        system(comando);
        sleep(0.5);
    }
    Escribir_registro("se ha matado los hijos");
}
int contador = 0;
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
            Escribir_registro("se ha abierto una nueva terminal con usuario");

            exit(0); // El hijo termina aquí, la terminal sigue corriendo
        }
        else if (pid > 0)
        {
            // Esperar un momento para que el archivo con el PID se cree
            sleep(1);
            char filename[50];
            // Leer el PID real del hijo desde el archivo
            snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", pid);
            Escribir_registro("se ha generado un archvio con el pid del usuario");

            // Esperamos 20 segundos (puedes quitar esto si no es necesario)
            sleep(3);

            // Abrimos el archivo en modo lectura y escritura
            FILE *fp = fopen(filename, "r+");

            if (fp)
            {

                // Leemos un único número entero del archivo
                if (fscanf(fp, "%d", &temp[contador]) == 1)
                {
                    sleep(1);
                }
                else
                {
                    perror("No se pudo leer un número del archivo.\n");
                }
                contador++;
                // Cerramos el archivo
                fclose(fp);
                Escribir_registro("Se ha alamcenado el pid del usuario");
            }
            else
            {
                perror("Error al leer el archivo de PID");
                hijos[num_hijos] = pid; // Si falla, usar el PID de fork()
            }

            num_hijos++;
            system("clear");
            // Preguntar si desea aceptar otro usuario
            printf("┌────────────────────────────────────────────┐\n");
            printf("│        🏦 BANCO CENTRAL - NUEVO USUARIO    │\n");
            printf("├────────────────────────────────────────────┤\n");
            printf("│ ¿Desea aceptar otro usuario? (s / n)       │\n");
            printf("└────────────────────────────────────────────┘\n");
            printf("Ingrese su opción: ");
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
    Escribir_registro("Se acaban con todos los procesos");
}
// Esta funcion se encarga de "pegar" la alerta en el pipe para poder pasarlo al proceso padre

// Función para crear el proceso Monitor y manejar alertas en tiempo real
void CrearMonitor()
{
    pid_t Monitor = fork(); // Crea el proceso monitor
    Escribir_registro("Se crea el fork de monitor");
    if (Monitor < 0)
    {
        perror("Error al crear el proceso Monitor");
        Escribir_registro("Fallo al crear el proceso monitor");
        return;
    }

    if (Monitor == 0)
    {
        // Proceso hijo
        Escribir_registro("Proceso monitor hijo iniciado");

        // Guardar PID en archivo
        char filename[50];
        snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", getpid());

        FILE *fp = fopen(filename, "w");
        if (fp)
        {
            fprintf(fp, "%d\n", getpid());
            fclose(fp);
        }
        else
        {
            perror("No se pudo crear el archivo de PID");
        }

        // Ejecutar el programa monitor
        execl("./monitor", "monitor", (char *)NULL);

        // Si execl falla:
        perror("Error al ejecutar monitor");
        Escribir_registro("Error al ejecutar monitor");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Proceso padre
        char filename[50];
        snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", Monitor);

        sleep(5); // Dar tiempo al hijo para crear el archivo

        FILE *fp = fopen(filename, "r+");
        if (fp)
        {
            if (fscanf(fp, "%d", &temp[contador]) == 1)
            {
                sleep(1);
            }
            else
            {
                perror("No se pudo leer un número del archivo.\n");
            }
            contador++;
            fclose(fp);
        }
        else
        {
            perror("Error al leer el archivo de PID");
            hijos[num_hijos] = Monitor; // fallback
        }

        num_hijos++;
        Escribir_registro("Proceso monitor creado correctamente");
    }
}
