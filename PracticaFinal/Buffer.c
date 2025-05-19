#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "Comun.h"
#include "Cuenta.h"


volatile sig_atomic_t corriendo = 1;

/// @brief Maneja la señal de salida
void manejador_salida(int sig)
{
    corriendo = 0;
}

/// @brief Esta funcion se encarga de volver a 
void *VolcadoBuffer(void *arg)
{
    Buffer *buffer = (Buffer *)arg;
    if (!buffer) return NULL;

    int id_cola2 = msgget(CLAVE_COLA2, 0666);
    if (id_cola2 == -1)
    {
        perror("Error al abrir la cola de mensajes con CLAVE_COLA2");
        return NULL;
    }

    struct {
        long mtype;
        Buffer buffer;
    } mensaje;

    while (msgrcv(id_cola2, &mensaje, sizeof(Buffer), 0, IPC_NOWAIT) != -1)
    {
        for (int i = 0; i < mensaje.buffer.NumeroCuentas; i++)
        {
            Cuenta cuentaCola = mensaje.buffer.cuentas[i];
            sem_wait(sem_usuarios);
            FILE *file = fopen("cuentas.txt", "r");
            if (!file)
            {
                perror("Error al abrir el archivo");
                sem_post(sem_usuarios);
                return NULL;
            }

            FILE *tempFile = fopen("cuentas_temp.txt", "w");
            if (!tempFile)
            {
                perror("Error al crear el archivo temporal");
                sem_post(sem_usuarios);
                fclose(file);
                return NULL;
            }
            sem_wait(sem_MC);
            char line[512];
            while (fgets(line, sizeof(line), file))
            {
                Cuenta cuentaArchivo;
                if (sscanf(line, "%d|%49[^|]|%49[^|]|%49[^|]|%99[^|]|%49[^|]|%d|%19[^|]|%19[^\n]",
                           &cuentaArchivo.id, cuentaArchivo.Nombre, cuentaArchivo.Apellido,
                           cuentaArchivo.Contraseña, cuentaArchivo.domicilio, cuentaArchivo.pais,
                           &cuentaArchivo.saldo, cuentaArchivo.fecha, cuentaArchivo.hora) == 9)
                {
                    if (cuentaArchivo.id == cuentaCola.id)
                    {
                        // Modificar saldo
                        cuentaArchivo.saldo = cuentaCola.saldo;
                    }
                    fprintf(tempFile, "%d|%s|%s|%s|%s|%s|%d|%s|%s\n",
                            cuentaArchivo.id, cuentaArchivo.Nombre, cuentaArchivo.Apellido,
                            cuentaArchivo.Contraseña, cuentaArchivo.domicilio, cuentaArchivo.pais,
                            cuentaArchivo.saldo, cuentaArchivo.fecha, cuentaArchivo.hora);
                }
            }

            fclose(file);
            fclose(tempFile);
            sem_post(sem_MC);
            sem_post(sem_usuarios);
            // Sobrescribir el archivo original
            remove("cuentas.txt");
            rename("cuentas_temp.txt", "cuentas.txt");
        }
    }

    if (errno != ENOMSG)
    {
        perror("Error al recibir mensajes de la cola");
    }

    pthread_exit(NULL);
}

int main()
{
    signal(SIGINT, manejador_salida); // Manejador de la señal SIGINT
    signal(SIGTERM, manejador_salida);

    Inicializar_semaforos(); // inicializamos los semaforos

    Buffer buffer;
    buffer.acceso = 0; // Aseguramos que el buffer esté habilitado para escribir
    // Nota: aquí podrías inicializar buffer.cuentas si es necesario.

    pthread_t hilo_volcado; // Hilo para el volcado de buffer

    while (corriendo) //bucle de l buffer para que compruebe si hay que actualizado el archivo
    {
        if (pthread_create(&hilo_volcado, NULL, VolcadoBuffer, (void *)&buffer) != 0) // creamos el hilo
        {
            perror("Error al crear el hilo de volcado de buffer");
            break;
        }

        if (pthread_join(hilo_volcado, NULL) != 0)
        {
            perror("Error al unir el hilo de volcado de buffer");
            break;
        }

        Escribir_registro("✅ Archivo de cuentas actualizado correctamente en buffer.c");

        sleep(30); // Espera 30 segundos antes de hacer el siguiente volcado
    }

    Destruir_semaforos(); // Destruimos los semáforos
    return 0;
}
