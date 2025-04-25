
#include "Operaciones.h"
#include "Comun.h"
#include "Cuenta.h"

// declaramos el struct de usuario
struct Usuario3
{
    char Usuario2[50];
    char Contraseña1[50];
};

/// @brief Esta funcion permite que un usuario introduzca dinero en el sistema, actualizamos el saldo y le numero de transaccioens
/// y tambien escribe en el archivo de transcciones
/// @param arg2
/// @return
void *IntroducirDinero(void *arg2)
{
    struct Usuario3 *usuario = (struct Usuario3 *)arg2;
    bool encontrado = false;
    int saldo_introducir;

    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);
    system("clear");
    printf("\n==============================\n");
    printf("    💵 INGRESO DE DINERO 💵\n");
    printf("==============================\n");
    printf("Introduzca la cantidad que desea ingresar: ");
    scanf("%d", &saldo_introducir);

    if (saldo_introducir < 0)
    {
        printf("❌ No puedes ingresar una cantidad negativa!\n");
        Escribir_registro("El usuario ha intentado introducir una cantidad negativa en IntroducirDinero.c");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    key_t clave = ftok("Cuenta.h", 66);
    if (clave == -1)
    {
        perror("❌ Error al generar clave con ftok");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    int shmid = shmget(clave, sizeof(Cuenta) * MAX_CUENTAS, 0666);
    if (shmid == -1)
    {
        perror("❌ Error al obtener segmento de memoria compartida");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    Cuenta *cuentas = (Cuenta *)shmat(shmid, NULL, 0);
    if (cuentas == (void *)-1)
    {
        perror("❌ Error al enlazar memoria compartida");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    int id_transacciones = 0;
    FILE *ArchivoTransacciones = fopen("transaciones.txt", "r");
    if (ArchivoTransacciones)
    {
        char linea[256];
        while (fgets(linea, sizeof(linea), ArchivoTransacciones))
        {
            int temp_id;
            if (sscanf(linea, "%d |", &temp_id) == 1)
            {
                id_transacciones = temp_id;
            }
        }
        fclose(ArchivoTransacciones);
    }
    id_transacciones++;

    for (int i = 0; i < MAX_CUENTAS; i++)
    {
        Cuenta *c = &cuentas[i];

        if (strlen(c->Nombre) == 0)
            continue;

        limpiar_cadena(c->Nombre);
        limpiar_cadena(c->Contraseña);

        if (strcmp(c->Nombre, usuario->Usuario2) == 0 && strcmp(c->Contraseña, usuario->Contraseña1) == 0)
        {
            int dinero_inicial = c->saldo;
            c->saldo += saldo_introducir;
            encontrado = true;

            Escribir_registro("El usuario ha introducido dinero correctamente en IntroducirDinero.c");

            // Registrar en transacciones globales
            ArchivoTransacciones = fopen("transaciones.txt", "a");
            if (ArchivoTransacciones)
            {
                fprintf(ArchivoTransacciones, "%d | ingreso | %d | - | %d | - | %d\n",
                        id_transacciones, c->id, dinero_inicial, c->saldo);
                fclose(ArchivoTransacciones);
            }

            // Carpeta ./transacciones/{id}
            char dir_usuario[64];
            snprintf(dir_usuario, sizeof(dir_usuario), "./transacciones/%d", c->id);
            mkdir("./transacciones", 0777);
            mkdir(dir_usuario, 0777);

            char ruta_archivo[128];
            snprintf(ruta_archivo, sizeof(ruta_archivo), "%s/transacciones.log", dir_usuario);

            FILE *ArchivoTransacciones2 = fopen(ruta_archivo, "a");
            if (!ArchivoTransacciones2)
            {
                perror("No se pudo abrir el archivo de transacciones");
                break;
            }

            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char hora[20];
            strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);

            fprintf(ArchivoTransacciones2, "[%s] Ingreso: %d ----> %d$\n",
                    hora, saldo_introducir, c->saldo);

            fclose(ArchivoTransacciones2);
            break;
        }
    }

    shmdt(cuentas);
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    if (encontrado)
    {
        printf("✅ Saldo actualizado correctamente.\n");
        sleep(2);
    }
    else
    {
        printf("❌ Usuario no encontrado o contraseña incorrecta.\n");
    }

    return NULL;
}
