#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define Punt_Archivo_Properties "Variables.properties"
/// @brief
/// En este bloque de codigo mostraremos el menu del usuario donde le daremos opciones a realizar diferentes operaciones

void Menu_User()
{

    pid_t Usuario = fork();
    if (Usuario < 0)
    {
        perror("Error con la creacion del usuario");
    }
    else if (Usuario == 0)
    {
        pthread_t HiloUsuario;

        // hay que hacer pthread_create de esos y to el rollo
        // hay que crear otra funcion que es MostrarMenu donde este el menu que lo llamas desde el hilo de ejeccucion 
        
    }
}
void Mostrar_Menu(){
    int Eleccion = 0;
        printf("-------------Hola Bienvenido al menu interactivo----------\n"); // la idea aqui despues de hola es poner el nombre del usuario para que quede chulo
        printf("\t Pulse la opcion que desea realizar.....\n");
        printf("1.Introducir Dinero \n");
        printf("2.Extraer Dinero\n ");
        printf("3.Hacer una Transferencia \n");
        printf("4.Consultar mis datos\n");
        printf("5.Cerrar Sesion\n ");
        // Mostrara el menu del usuario
        scanf("%d", &Eleccion);
        switch (Eleccion)
        {
        case (1):

            break;

        default:
            break;
        }
}