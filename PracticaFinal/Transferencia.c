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

void *Transferencia(){
    
    sem_t semaforo4;
    
    sem_init(&semaforo4, 0, 1);
    sem_wait(&semaforo4);
    


    sem_post(&semaforo4);
    sem_destroy(&semaforo4);

}