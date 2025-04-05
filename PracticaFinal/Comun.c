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
sem_t *sem_usuarios;
sem_t *sem_transacciones;
sem_t *sem_registro;
// Inicializar semáforos compartidos

/// @brief 
void Inicializar_semaforos()
{
    sem_registro = sem_open("/sem_registro", O_CREAT, 0666, 1);
    sem_usuarios = sem_open("/sem_usuarios", O_CREAT, 0666, 1);
    sem_transacciones = sem_open("/sem_transacciones", O_CREAT, 0666, 1);
    if (sem_usuarios == SEM_FAILED || sem_transacciones == SEM_FAILED || sem_registro == SEM_FAILED)
    {
        perror("Error al inicializar semáforos");
        exit(EXIT_FAILURE);
    }
}

/// @brief Cierra y elimina los semáforos, también elimina la cola de mensajes
void Destruir_semaforos()
{
    sem_close(sem_usuarios);
    sem_close(sem_transacciones);
    sem_close(sem_registro);
    sem_unlink("/sem_registro");
    sem_unlink("/sem_usuarios");
    sem_unlink("/sem_transacciones");

    // Eliminar la cola de mensajes si existe
    int id_cola = msgget(CLAVE_COLA, 0666);
    if (id_cola != -1) {
        msgctl(id_cola, IPC_RMID, NULL);
    }
}


int id_cola;

//Crea una cola de mensajes del sistema con la clave definida
void crear_cola_mensajes() {
    id_cola = msgget(CLAVE_COLA, IPC_CREAT | 0666);
    if (id_cola == -1) {
        perror("Error al crear la cola de mensajes");
        exit(EXIT_FAILURE);
    }
}


Config leer_configuracion(const char *ruta)
{
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL)
    {
        perror("Error al abrir config.txt");
        exit(1);
    }
    Config config;
    char linea[100];

    // Leer cada línea del archivo de configuración
    while (fgets(linea, sizeof(linea), archivo))
    {
        if (linea[0] == '#' || strlen(linea) < 3)
            continue; // Ignorar comentarios y lineas vacias
        if (strstr(linea, "LIMITE_RETIRO"))
            sscanf(linea, "LIMITE_RETIRO=%d",
                   &config.limite_retiro);
        else if (strstr(linea, "LIMITE_TRANSFERENCIA"))
            sscanf(linea,
                   "LIMITE_TRANSFERENCIA=%d", &config.limite_transferencia);
        else if (strstr(linea, "UMBRAL_RETIROS"))
            sscanf(linea, "UMBRAL_RETIROS=%d",
                   &config.umbral_retiros);
        else if (strstr(linea, "UMBRAL_TRANSFERENCIAS"))
            sscanf(linea,
                   "UMBRAL_TRANSFERENCIAS=%d", &config.umbral_transferencias);
        else if (strstr(linea, "NUM_HILOS"))
            sscanf(linea, "NUM_HILOS=%d",
                   &config.num_hilos);
        else if (strstr(linea, "ARCHIVO_CUENTAS"))
            sscanf(linea, "ARCHIVO_CUENTAS=%s",
                   config.archivo_cuentas);
        else if (strstr(linea, "ARCHIVO_LOG"))
            sscanf(linea, "ARCHIVO_LOG=%s",
                   config.archivo_log);
    }
    fclose(archivo);
    return config;
}
// Funcion para escribir en el registro de log

/// @brief 
/// @param mensaje_registro 
void Escribir_registro(const char *mensaje_registro)
{
    if (sem_registro == NULL) {
        fprintf(stderr, "⚠️ Error: semáforo de registro no inicializado.\n");
        return;
    }

    sem_wait(sem_registro);    // Espera semáforo para acceso exclusivo al archivo

    time_t t;
    struct tm *tm_info;
    char hora[30];

    time(&t);
    tm_info = localtime(&t);
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);

    // Abrir archivo de registro en modo append
    FILE *ArchivoDeRegistro = fopen("registro.log", "a");
    if (!ArchivoDeRegistro)
    {
        perror("Error al abrir el archivo de registro");
        sem_post(sem_registro); // libera el semáforo incluso si hay error
        return;
    }

    // Escribir mensaje en el log
    fprintf(ArchivoDeRegistro, "[%s] %s\n", hora, mensaje_registro);
    fclose(ArchivoDeRegistro);

    sem_post(sem_registro);
}

/// @brief esta funcion limpia la cadena para el strcmp posterior , lo que hace es quitar los espacios en blanco y tabulaciones de la cadena
/// @param cadena
void limpiar_cadena(char *cadena)
{
    int inicio = 0;
    int fin = strlen(cadena) - 1;

    // Eliminar los espacios al principio
    while (cadena[inicio] == ' ' || cadena[inicio] == '\t')
    {
        inicio++;
    }

    // Eliminar los espacios al final
    while (fin >= inicio && (cadena[fin] == ' ' || cadena[fin] == '\t'))
    {
        fin--;
    }

    // Mover la cadena limpia
    for (int i = 0; i <= fin - inicio; i++)
    {
        cadena[i] = cadena[inicio + i];
    }
    cadena[fin - inicio + 1] = '\0'; // Añadir el carácter nulo al final
}

/// @brief Verifica credenciales de usuario y retorna su ID si es válido
/// @param nombre Nombre de usuario
/// @param contrasena Contraseña del usuario
/// @return ID del usuario si es válido, -1 si no se encuentra
int obtener_id_usuario(const char *nombre, const char *contrasena)
{
    FILE *archivo = fopen("usuarios.txt", "r");
    if (!archivo)
    {
        perror("Error al abrir usuarios.txt");
        return -1;
    }

    // Variables para los campos de usuario
    int id, saldo, num_transacciones;
    char nombre_archivo[50], contrasena_archivo[50], apellidos[50], domicilio[100], pais[50];


    // Leer línea por línea y parsear los datos de usuario
    while (fscanf(archivo, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d\n",
                  &id, nombre_archivo, contrasena_archivo, apellidos, domicilio, pais, &saldo, &num_transacciones) == 8)
    {
        limpiar_cadena(nombre_archivo);
        limpiar_cadena(contrasena_archivo);
        if (strcmp(nombre, nombre_archivo) == 0 && strcmp(contrasena, contrasena_archivo) == 0)
        {
            fclose(archivo);
            return id; // Retorna el ID del usuario
        }
    }
    fclose(archivo);
    return -1; // No se encontró el usuario
}