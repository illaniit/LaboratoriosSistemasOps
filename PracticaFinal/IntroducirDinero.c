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
#include "Operaciones.h"
#include <stdbool.h>
void limpiar_cadena2(char* cadena);
//Introduzco un struct para que me pida la contraseña a la hora de introducir un usuario :) //
struct Usuario3
    {
        char Usuario2[50];
        char Contraseña1[50];
    }Usuario3;


void limpiar_cadena2(char* cadena) {
        int inicio = 0;
        int fin = strlen(cadena) - 1;
    
        // Eliminar los espacios al principio
        while (cadena[inicio] == ' ' || cadena[inicio] == '\t') {
            inicio++;
        }
    
        // Eliminar los espacios al final
        while (fin >= inicio && (cadena[fin] == ' ' || cadena[fin] == '\t')) {
            fin--;
        }
    
        // Mover la cadena limpia
        for (int i = 0; i <= fin - inicio; i++) {
            cadena[i] = cadena[inicio + i];
        }
        cadena[fin - inicio + 1] = '\0'; // Añadir el carácter nulo al final
}


void *IntroducirDinero(void *arg2) {
    sem_t semaforo3;
    sem_init(&semaforo3, 0, 1);
   
}