#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_LINE_LENGTH 256
#define NUM_THREADS 10

FILE* file;

void* readAndPrintLines(void* arg) {
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file) != NULL) {
        printf("Hilo %ld: %s", pthread_self(), line);
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];

    // Abrir el fichero
    file = fopen("archivo.txt", "r");
    if (file == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    // Crear los 10 hilos
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, &readAndPrintLines, NULL) != 0) {
            perror("pthread_create");
            fclose(file);
            return EXIT_FAILURE;
        }
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Limpieza
    fclose(file);

    return EXIT_SUCCESS;
}
