#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "Comun.h"
#include "Cuenta.h"

volatile sig_atomic_t corriendo = 1;

void manejador_salida(int sig)
{
    corriendo = 0;
}

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

            FILE *file = fopen("cuentas.txt", "r");
            if (!file)
            {
                perror("Error al abrir el archivo");
                return NULL;
            }

            FILE *tempFile = fopen("cuentas_temp.txt", "w");
            if (!tempFile)
            {
                perror("Error al crear el archivo temporal");
                fclose(file);
                return NULL;
            }

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
    signal(SIGINT, manejador_salida);
    signal(SIGTERM, manejador_salida);

    Inicializar_semaforos();

    Buffer buffer;
    buffer.acceso = 0; // Aseguramos que el buffer esté habilitado para escribir
    // Nota: aquí podrías inicializar buffer.cuentas si es necesario.

    pthread_t hilo_volcado;

    while (corriendo)
    {
        if (pthread_create(&hilo_volcado, NULL, VolcadoBuffer, (void *)&buffer) != 0)
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

        sleep(10); // Espera 10 segundos antes de hacer el siguiente volcado
    }

    Destruir_semaforos();
    return 0;
}
