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
    
    if (!buffer || !buffer->acceso)
    {
        return NULL;
    }

    FILE *file = fopen("cuentas.txt", "r+");
    if (file == NULL)
    {
        perror("Error al abrir el archivo");
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
            for (int i = 0; i < 10; i++)
            {
                if (buffer->cuentas[i].id == cuentaArchivo.id)
                {
                    fseek(file, -strlen(line), SEEK_CUR);
                    fprintf(file, "%d|%s|%s|%s|%s|%s|%d|%s|%s\n",
                            buffer->cuentas[i].id, buffer->cuentas[i].Nombre,
                            buffer->cuentas[i].Apellido, buffer->cuentas[i].Contraseña,
                            buffer->cuentas[i].domicilio, buffer->cuentas[i].pais,
                            buffer->cuentas[i].saldo, buffer->cuentas[i].fecha,
                            buffer->cuentas[i].hora);
                    fflush(file);
                    break;
                }
            }
        }
    }

    fclose(file);
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
