#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#define PIPE_NAME "/tmp/alert_pipe"
#define MONTO_LIMITE 1000
#define Punt_Archivo_Properties "Variables.properties"
#define MAX_LENGTH 256
#include "init_cuentas.h"



// Prototipos de funciones
void AbrirPropertis();
void CrearMonitor();
int main() 
{
    AbrirPropertis();
    CrearMonitor(); 
    Menu_Usuario();
    return 0;
}

void AbrirPropertis()
{
    char *key, *value;
    char line[MAX_LENGTH];
    char username[MAX_LENGTH] = {0}; // Inicializar con ceros
    char password[MAX_LENGTH] = {0};
    FILE *ArchivoPro = fopen(Punt_Archivo_Properties, "r");

    if (!ArchivoPro)
    { // Validar si el archivo se abrió correctamente
        perror("Error al abrir el archivo de propiedades");
        return;
    }

    while (fgets(line, MAX_LENGTH, ArchivoPro))
    {
        line[strcspn(line, "\n")] = 0; // Eliminar salto de línea

        key = strtok(line, "=");
        value = strtok(NULL, "=");

        if (key && value)
        { // Validar que la línea sea válida
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

struct msgbuf {
    long tipo;
    char texto[100];
};

int cola_mensajes;

void enviar_alerta(const char *mensaje) {
    int fd = open(PIPE_NAME, O_WRONLY | O_NONBLOCK);
    if (fd != -1) {
        write(fd, mensaje, strlen(mensaje));
        close(fd);
    } else {
        perror("Error al abrir la tubería");
    }
}

void limpiar(int sig) {
    msgctl(cola_mensajes, IPC_RMID, NULL);
    unlink(PIPE_NAME);
    exit(0);
}

void detectar_transacciones() {
    signal(SIGINT, limpiar);

    key_t key = ftok("transaciones.txt", 65);
    cola_mensajes = msgget(key, 0666 | IPC_CREAT);
    struct msgbuf mensaje;

    if (cola_mensajes == -1) {
        perror("Error al acceder a la cola de mensajes");
        exit(1);
    }

    // Crear la tubería si no existe
    if (mkfifo(PIPE_NAME, 0666) == -1 && errno != EEXIST) {
        perror("Error al crear la tubería");
        exit(1);
    }

    while (1) {
        if (msgrcv(cola_mensajes, &mensaje, sizeof(mensaje.texto), 0, 0) != -1) {
            printf("Transacción recibida: %s\n", mensaje.texto);

            int monto;
            if (sscanf(mensaje.texto, "retiro %d", &monto) == 1 && monto > MONTO_LIMITE) {
                enviar_alerta("ALERTA: Retiro sospechoso detectado\n");
            }
        }
    }
}

void CrearMonitor()
{
    pid_t Monitor = fork(); // Crear un nuevo proceso hijo

    if (Monitor < 0)
    {
        perror("Error al crear el proceso Monitor");
        return;
    }

    if (Monitor == 0)
    { // Código del hijo esto es como tal el monitor
        pthread_t Transacciones;
        if (pthread_create(&Transacciones, NULL,detectar_transacciones, NULL) != 0)
        {
            perror("Error al crear el hilo de transacciones");
            exit(1);
        }

        pthread_join(Transacciones, NULL); // Esperar a que termine el hilo
        exit(0);                           // Terminar el proceso hijo correctamente
    }
}
