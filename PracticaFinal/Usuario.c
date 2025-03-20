#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#define Punt_Archivo_Properties "Variables.properties"
#include "Usuario.h"
#include "Operaciones.h"
/// @brief
/// En este bloque de codigo mostraremos el menu del usuario donde le daremos opciones a realizar diferentes operaciones

// Aqui hay que meter las funciones de properties y

void Mostrar_Menu()
{
    pthread_t hilo1, hilo2, hilo3, hilo4;
    int Eleccion = 0;
    do
    {
        system("clear");
        printf("------------- Hola, Bienvenido al Menú Interactivo ----------\n");
        printf("\tSeleccione una opción:\n");
        printf("1. Introducir Dinero\n");
        printf("2. Extraer Dinero\n");
        printf("3. Hacer una Transferencia\n");
        printf("4. Consultar mis Datos\n");
        printf("5. Cerrar Sesión\n");

        printf("Ingrese su opción: ");

        scanf("%d", &Eleccion);

        switch (Eleccion)
        {
        case 1:
            if (pthread_create(&hilo1, NULL, IntroducirDinero, NULL) != 0)
            {
                printf("❌ Error al crear el hilo para Introducir Dinero.\n");
                return;
            }
            pthread_join(hilo1, NULL);
           

        case 2:
            if (pthread_create(&hilo2, NULL, ExtraerDinero, NULL) != 0)
            {
                printf("❌ Error al crear el hilo para Extraer Dinero.\n");
                return;
            }
            pthread_join(hilo2, NULL);
           

        case 3:
            if (pthread_create(&hilo3, NULL, Transferencia, NULL) != 0)
            {
                printf("❌ Error al crear el hilo para Transferencia.\n");
                return;
            }
            pthread_join(hilo3, NULL);
           

        case 4:
            if (pthread_create(&hilo4, NULL, ConsultarDatos, NULL) != 0)
            {
                printf("❌ Error al crear el hilo para Consultar Datos.\n");
                return;
            }
            pthread_join(hilo4, NULL);
            

        case 5:
            printf("Saliendo del sistema...\n");
            sleep(2);
            return;

        default:
            printf("❌ Opción inválida. Inténtelo de nuevo.\n");
            break;
        }
    } while (Eleccion == 5);
}