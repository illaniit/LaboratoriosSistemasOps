#include <stdbool.h>
#include "Operaciones.h"
#include "Comun.h"

// Introduzco un struct para que me pida la contraseña a la hora de introducir un usuario :) //
struct Usuario4
{
    char Usuario3[50];
    char Contraseña1[50];
} Usuario4;

/// @brief esta funcion permite extraer dinero y lo actualiza
/// @param arg3 
/// @return no devuelebe nada, es de tipo void actualiza 
void *ExtraerDinero(void *arg3)
{
    Config config = leer_configuracion("variables.properties"); // Cargamos la configuración del sistema
    struct Usuario4 *usuario = (struct Usuario4 *)arg3; // Cast del argumento recibido al tipo correcto
    bool encontrado = false;

    int saldo_extraer;
    system("clear");
    printf("\n==============================\n");
    printf("    💵 EXTRACCION DE DINERO 💵\n");
    printf("==============================\n");   // vemos cuento quiere ingresar el usuario
    printf("Introduzca la cantidad que desea extraer: ");
    scanf("%d", &saldo_extraer);  // Usuario ingresa el monto a retirar
    Escribir_registro("El usuario ha introducido el saldo a extraer");
    
    // Validaciones
    if(saldo_extraer<0){
        printf("No se puede extraer dinero negativo!\n");
        Escribir_registro("El usuario ha intentado extraer dinero negativo en ExtraerDinero.c");
        sleep(2);
        return(NULL);
    }
    if (saldo_extraer > config.limite_retiro)
    {
        printf("El dinero que desea extraer excede nuestro limite\n");
        Escribir_registro("El usuario ha intentado extraer mas cantidad que el limite permitido en ExtraerDinero.c");
        sleep(2);
        return NULL;
    }
    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    
    FILE *ArchivoUsuarios = fopen("usuarios.txt", "r");
    if (!ArchivoUsuarios)
    {
        perror("Error al abrir el archivo");

        return NULL;
    }

    // Archivo temporal para sobrescribir los datos actualizados
    FILE *tempFile1 = fopen("temp1.txt", "w");
    if (!tempFile1)
    {
        perror("Error al abrir el archivo temporal");
        fclose(ArchivoUsuarios);

        return NULL;
    }

    // Variables para lectura de archivo
    int id3, saldo3, num_transacciones3;
    char nombre3[50], contrasena3[50], apellidos3[50], domicilio3[100], pais3[50];
    char linea[200], linea2[200];

    // Buscar el ID más reciente de transacciones para asignar uno nuevo
    int id_transacciones = 0;
    FILE *ArchivoTransacciones = fopen("transaciones.txt", "r");
    if (ArchivoTransacciones)
    {
        while (fgets(linea2, sizeof(linea2), ArchivoTransacciones) != NULL)
        {
            int temp_id;
            if (sscanf(linea2, "%d |", &temp_id) == 1)
            {
                id_transacciones = temp_id; // Guardamos el último ID encontrado
            }
        }
        fclose(ArchivoTransacciones);
    }
    id_transacciones++; // Incrementamos para el nuevo registro

    // Leer línea por línea del archivo de usuarios
    while (fgets(linea, sizeof(linea), ArchivoUsuarios) != NULL)
    {
        linea[strcspn(linea, "\n")] = '\0';

        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d",
                   &id3, nombre3, contrasena3, apellidos3, domicilio3, pais3, &saldo3, &num_transacciones3) == 8)
        {
            limpiar_cadena(nombre3);
            limpiar_cadena(contrasena3);

            // Verificar si coincide con el usuario que desea extraer
            if (strcmp(nombre3, usuario->Usuario3) == 0 && strcmp(contrasena3, usuario->Contraseña1) == 0)
            {
                int dinero_inicial = saldo3;
                encontrado = true;

                // Validar si hay suficiente saldo
                if (saldo_extraer > saldo3)
                {
                    printf("Saldo insuficiente.\n");

                    fclose(tempFile1);
                    fclose(ArchivoUsuarios);
                    sem_post(sem_usuarios);
                    sem_post(sem_transacciones);
                    remove("temp1.txt");
                    return NULL;
                }

                // Actualizar saldo y número de transacciones
                saldo3 -= saldo_extraer;
                num_transacciones3++;

                // Guardar transacción en archivo
                ArchivoTransacciones = fopen("transaciones.txt", "a");
                if (ArchivoTransacciones)
                {
                    fprintf(ArchivoTransacciones, "%d | retiro | %d | - | %d | - | %d \n", id_transacciones, id3, dinero_inicial, saldo3);
                    fclose(ArchivoTransacciones);
                }
            }
        }
        fprintf(tempFile1, "%d | %s | %s | %s | %s | %s | %d | %d\n",
                id3, nombre3, contrasena3, apellidos3, domicilio3, pais3, saldo3, num_transacciones3);
    }

    // Cierre de archivos y salida de sección crítica
    fclose(ArchivoUsuarios);
    fclose(tempFile1);
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    if (encontrado)
    {
        remove("usuarios.txt");  // Eliminamos el archivo viejo
        rename("temp1.txt", "usuarios.txt");  // Renombramos el temporal como nuevo archivo
        printf("Saldo actualizado correctamente.\n");
        sleep(2);
    }
    else
    {
        remove("temp1.txt");
        printf("Usuario no encontrado o contraseña incorrecta.\n");
        Escribir_registro("No se ha encontrado el usuario en ExtraerDinero.c");
        sleep(2);
    }

  
    return NULL;
}