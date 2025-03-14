#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define Punt_Archivo_Properties "Variables.properties"
#include "Usuario.h"
/// @brief
/// En este bloque de codigo mostraremos el menu del usuario donde le daremos opciones a realizar diferentes operaciones

//Aqui hay que meter las funciones de properties y 



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
            //IntroducirDinero();
            break;
        case(2):
            //ExtraerDinero();
            break;
        case(3):
            //HacerTransferencia();
            break;
        case(4):
            //ConsultarDatos
            break;
        case(5):
                //salir del sistema 
                printf("Saliendo del sistema...");
                sleep(10);
            break;
        default:
            break;
        }
}