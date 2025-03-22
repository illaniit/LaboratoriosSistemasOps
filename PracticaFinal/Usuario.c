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
#include "Comun.h"
/// @brief
/// En este bloque de codigo mostraremos el menu del usuario donde le daremos opciones a realizar diferentes operaciones

// Aqui hay que meter las funciones de properties y



    struct Usuario
    {
        char Usuario[50];
        char Contraseña[50];
    }Usuario;

    void Mostrar_Menu(char *user, char *passwd)
{
    leer_configuracion("variables.properties");
    struct Usuario usuario;
    strcpy(usuario.Usuario, user);
    strcpy(usuario.Contraseña, passwd);

    pthread_t hilo1, hilo2, hilo3, hilo4;
    int Eleccion = 0;

    do
    {
        system("clear"); 
        printf("------------- 🏦 Menú Interactivo 🏦 -------------\n");
        printf("\tSeleccione una opción:\n");
        printf("1️⃣  Introducir Dinero\n");
        printf("2️⃣  Extraer Dinero\n");
        printf("3️⃣  Hacer una Transferencia\n");
        printf("4️⃣  Consultar mis Datos\n");
        printf("5️⃣  Cerrar Sesión\n");

        printf("\nIngrese su opción: ");

        if (scanf("%d", &Eleccion) != 1)
        {
            printf("❌ Entrada inválida. Intente nuevamente.\n");
            while (getchar() != '\n');  // Limpiar el buffer de entrada
            sleep(2);
            continue;
        }

        switch (Eleccion)
        {
        case 1:
            if (pthread_create(&hilo1, NULL, IntroducirDinero, &usuario) != 0)
            {
                printf("❌ Error al crear el hilo para Introducir Dinero.\n");
            }
            else
            {
                pthread_join(hilo1, NULL);
            }
            break;

        case 2:
            if (pthread_create(&hilo2, NULL, ExtraerDinero, NULL) != 0)
            {
                printf("❌ Error al crear el hilo para Extraer Dinero.\n");
            }
            else
            {
                pthread_join(hilo2, NULL);
            }
            break;

        case 3:
            if (pthread_create(&hilo3, NULL, Transferencia, NULL) != 0)
            {
                printf("❌ Error al crear el hilo para Transferencia.\n");
            }
            else
            {
                pthread_join(hilo3, NULL);
            }
            break;

        case 4:
            if (pthread_create(&hilo4, NULL, ConsultarDatos, &usuario) != 0)
            {
                printf("❌ Error al crear el hilo para Consultar Datos.\n");
            }
            else
            {
                pthread_join(hilo4, NULL);
            }
            break;

        case 5:
            printf("🔒 Cerrando sesión... Hasta pronto!\n");
            sleep(2);
            break;

        default:
            printf("❌ Opción inválida. Inténtelo de nuevo.\n");
            sleep(2);
            break;
        }

    } while (Eleccion != 5);
}