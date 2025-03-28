
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
#include "Comun.h"
#include <sys/prctl.h> // Para PR_SET_PDEATHSIG
#define Cantidad_limite 1000
#define Punt_Archivo_Properties "Variables.properties"
#define MAX_LENGTH 256

int pipefd[2]; // Tubería que la declaramos para poder pasar informacion de monitor a banco y registrarlo en alertas .txt

// Definimos las funciones  que vamos a utilizar
void Menu_Procesos();
void CrearMonitor();
void *detectar_transacciones(void *arg);
void enviar_alerta(const char *mensaje, const int *id, const int titular);
void limpiar(int sig);
void registrar_alerta(const char *mensaje);

/// @brief este es el main en el cual leemos propertis con las variables
/// @return y devolvemos 0 si la ejecuccion ha sido exitosa
int main()
{
    Config config = leer_configuracion("variables.properties");
    // Lo primero abrimos el archivo de Properties y "nos traemos las variables"

    // Iniciamos monitor para que encuentre  anomalias
    Inicializar_semaforos();

    CrearMonitor();

    // Cargamos el menu del usuario que se encuentra en init_cuentas.c donde cada usuario sera un hilo de ejecuccion
    Menu_Procesos();

    // Volvemos a llamara monitor para que encuentre las anomalias despues de que el usuario haya cerrado la sesion
    Destruir_semaforos();
    return 0;
}

/// @brief Esta funcion crea procesos en una nueva terminal que lo que
/// hacen es ejecutar la instancia de usuario y ejecutar el codigo del mismo

#define MAX_HIJOS 100  // Máximo número de hijos permitidos

// Array de PIDs de los hijos
pid_t hijos[MAX_HIJOS];  
int num_hijos = 0;         // Contador de hijos creados

// Función para matar todos los procesos hijos usando system()
void matar_hijos()
{
    printf("\nEl padre ha terminado. Terminando todos los procesos hijos...\n");

    for (int i = 0; i < num_hijos; i++)
    {
        if (hijos[i] > 0)  // Verifica que el PID sea válido
        {
            printf("Matando hijo PID: %d\n", hijos[i]);

            // Matar proceso hijo directamente
            char comando[100];
            snprintf(comando, sizeof(comando), "sudo kill -9 %d", hijos[i]);
            system(comando);
            sleep(1);
        }
    }
}

void Menu_Procesos()
{
    char respuesta;
    int continuar = 1;

  

    while (continuar)
    {
        if (num_hijos >= MAX_HIJOS)
        {
            printf("Se ha alcanzado el límite de usuarios.\n");
            break;
        }

        pid_t pid = fork(); // Crear un proceso hijo
        
        if (pid == 0)
        {   
            // Crear un nuevo grupo de procesos para el hijo
            setpgid(0, 0);

            // Ejecutar en una nueva terminal y obtener el PID real
            char comando[200];
            snprintf(comando, sizeof(comando), 
                     "gnome-terminal -- sh -c 'echo $$ > /tmp/pid_%d.txt; gcc init_cuentas.c Usuario.c Transferencia.c ConsultarDatos.c ExtraerDinero.c IntroducirDinero.c Comun.c -o usuario && ./usuario'", getpid());
            
            system(comando);
            
            exit(0); // El hijo termina aquí, la terminal sigue corriendo
        }
        else if (pid > 0)
        { 
            // Esperar un momento para que el archivo con el PID se cree
            sleep(1);

            // Leer el PID real del hijo desde el archivo
            char filename[50];
            snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", pid);
            FILE *fp = fopen(filename, "r");
            if (fp)
            {
                fscanf(fp, "%d", &hijos[num_hijos]);  // Guardar el PID real del hijo
                fclose(fp);
            }
            else
            {
                perror("Error al leer el archivo de PID");
                hijos[num_hijos] = pid;  // Si falla, usar el PID de fork()
            }

            num_hijos++;

            // Preguntar si desea aceptar otro usuario
            printf("¿Desea aceptar otro usuario? (s/n): ");
            scanf(" %c", &respuesta); 
            if (respuesta != 's' && respuesta != 'S')
            {
                continuar = 0; 
            }
        }
        else
        {
            perror("Error en fork"); 
            exit(EXIT_FAILURE);
        }
    }

    // El padre espera a que todos los hijos terminen
    for (int i = 0; i < num_hijos; i++)
    {
        if (hijos[i] > 0)
        {
            waitpid(hijos[i], NULL, 0);
        }
    }
    
    // Cuando el padre termina, mata a todos los hijos
    matar_hijos();
}


// Esta funcion se encarga de "pegar" la alerta en el pipe para poder pasarlo al proceso padre

void enviar_alerta(const char *mensaje, const int *id, const int titular)
{
    Escribir_registro("Se ha enviado una alerta");

    time_t t;
    struct tm *tm_info;
    char hora[30]; // Para almacenar la fecha y hora formateadas

    // Obtiene la hora actual
    time(&t);
    tm_info = localtime(&t);

    // Formatea la fecha y hora en "YYYY-MM-DD HH:MM:SS"
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);
    char auxiliar[256];

    if (titular == 1)
    {
        snprintf(auxiliar, sizeof(auxiliar), "[%s] Id del Usuario :%d | %s\n", hora, *id, mensaje); // esta funcion nos permite agregar un \n en el archivo de texto para que sea entendible y legible
    }
    else if (titular == 0)
    {
        snprintf(auxiliar, sizeof(auxiliar), "[%s] Id de la transaccion:%d | Transaccion | %s\n", hora, *id, mensaje); // esta funcion nos permite agregar un \n en el archivo de texto para que sea entendible y legible
    }

    write(pipefd[1], auxiliar, strlen(auxiliar));
}

// Mediante señales limpiamos la tuberia
void limpiar(int sig)
{
    close(pipefd[0]);
    close(pipefd[1]);
    exit(0);
}

// Esta funcion abre/ crea si no existe el archivo de texto donde se almacenan las alertas y escribe las alertas que se encuentren en la variable mensaje
void registrar_alerta(const char *mensaje)
{
    Escribir_registro("Se ha abierto el fichero de alertas");
    FILE *archivo_alertas = fopen("alertas.log", "a"); // Abrimos el fichero
    if (archivo_alertas)
    {
        fprintf(archivo_alertas, "%s\n", mensaje); // escribimos el mensaje
        fclose(archivo_alertas);                   // cerramos el archivo
    }
    else
    {
        perror("Error al abrir alertas.log"); // si hay un error en la apertura lo notificamos
        Escribir_registro("Se ha producido un error al abrir el fichero de alertas");
    }
}

/// @brief Esta funcion es la encargada de detectar anomaliase en usuarios y transacciones en el proceso monohilo monitor
/// @param arg
/// @return
void *detectar_transacciones(void *arg)
{
    Config config = leer_configuracion("variables.properties");
    signal(SIGINT, limpiar); // limpiamos la tubería
    Escribir_registro("Se ha abierto el fichero de transacciones");
    FILE *archivo = fopen("transaciones.txt", "r"); // abrimos el archivo transacciones para buscar anomalías
    if (!archivo)
    {
        perror("Error al abrir el archivo de transacciones"); // Comprobamos que el archivo no dé error
        Escribir_registro("Se ha producido un error al abrir el archivo de transacciones");
        return NULL; // devolvemos null en este caso
    }

    char linea[256];
    int retiros_consecutivos = 0;
    int transferencias_consecutivas = 0;
    int usuario_id_anterior = -1;

    while (fgets(linea, sizeof(linea), archivo)) // leemos el archivo transacciones.txt
    {
        int id, saldo1, saldo2, saldo_final, id2, saldo_t1, saldo_t2, saldo_finalt1, saldo_finalt2;
        char tipo[20], cuenta1[20], cuenta2[20], tipo1[20], cuenta_t1[20], cuenta_t2[20];

        // Procesamos retiros o ingresos con 7 campos

        // Procesamos transferencias con 8 campos
        if (sscanf(linea, "%d | %39[^|] | %39[^|] | %39[^|] | %d | %d | %d | %d", &id2, tipo1, cuenta_t1, cuenta_t2, &saldo_t1, &saldo_t2, &saldo_finalt1, &saldo_finalt2) == 8)
        {
            limpiar_cadena(tipo1);

            // Verificación de transferencia

            if (saldo_finalt2 - saldo_t2 > config.limite_transferencia) // Transferencia superior a 10000
            {
                Escribir_registro("Se ha detectado una transferencia superior a 10000");
                enviar_alerta("Transferencia superior a 10000 detectada", &id2, 0);
            }

            // Verificación de transferencias consecutivas
            if (usuario_id_anterior == id2)
            {
                transferencias_consecutivas++;
            }
            else
            {
                transferencias_consecutivas = 1; // Iniciar contador si el usuario cambia
            }

            if (transferencias_consecutivas > config.umbral_transferencias)
            {
                Escribir_registro("Se han detectado más de 5 transferencias consecutivas");
                enviar_alerta("Usuario ha hecho más de 5 transferencias consecutivas", &id2, 0);
            }

            // Verificación de saldos negativos en transferencias
            if (saldo_t1 < 0 || saldo_t2 < 0 || saldo_finalt1 < 0 || saldo_finalt2 < 0)
            {
                Escribir_registro("Se ha detectado un saldo negativo en una transacción");
                enviar_alerta("Saldo negativo detectado en una transacción", &id2, 0);
            }
        }
        else
        {
            sscanf(linea, "%d | %39[^|] | %39[^|] | %39[^|] | %d | %d | %d", &id, tipo, cuenta1, cuenta2, &saldo1, &saldo2, &saldo_final);
            limpiar_cadena(tipo);

            // Verificación de retiro
            if (strcmp(tipo, " retiro ") == 0)
            {
                if (saldo1 - saldo_final > config.limite_retiro) // Retiro superior a 5000
                {
                    Escribir_registro("Se ha detectado un retiro superior a 5000");
                    enviar_alerta("Retiro superior a 5000 detectado", &id, 0);
                }

                // Verificación de retiros consecutivos
                if (usuario_id_anterior == id)
                {
                    retiros_consecutivos++;
                }
                else
                {
                    retiros_consecutivos = 1; // Iniciar contador si el usuario cambia
                }

                if (retiros_consecutivos > config.umbral_retiros)
                {
                    Escribir_registro("Se han detectado más de 3 retiros consecutivos");
                    enviar_alerta("Usuario ha hecho más de 3 retiros consecutivos", &id, 0);
                }
            }

            // Verificación de saldos negativos
            if (saldo1 < 0 || saldo2 < 0 || saldo_final < 0)
            {
                Escribir_registro("Se ha detectado un saldo negativo");
                enviar_alerta("Saldo negativo detectado", &id, 0);
            }

            // Verificación de formato inválido para ingresos y retiros
        }

        usuario_id_anterior = id2; // Actualizamos el usuario actual para las validaciones consecutivas
    }

    fclose(archivo); // cerramos el archivo

    // Verificación de usuarios.dat
    FILE *usuarios = fopen("usuarios.txt", "r"); // abrimos el archivo de usuarios
    if (!usuarios)
    {
        perror("Error al abrir el archivo de usuarios");
        Escribir_registro("Se ha producido un error en la apertura del archivo de usuarios");
        return NULL;
    }

    Escribir_registro("Se ha abierto el archivo de usuarios");

    while (fgets(linea, sizeof(linea), usuarios)) // Leemos línea por línea
    {
        int id, saldo, num_transacciones;
        char nombre[50], apellidos[50], contraseña[50], domicilio[100], pais[50];

        // Parseamos la línea según el formato: id | nombre | apellidos | domicilio | pais | saldo | numero_transacciones
        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d", &id, nombre, contraseña, apellidos, domicilio, pais, &saldo, &num_transacciones) == 8)
        {
            if (saldo < 0)
            {
                Escribir_registro("Se ha encontrado un usuario con saldo negativo");
                enviar_alerta("Usuario con saldo negativo", &id, 1);
            }

            if (num_transacciones < 0)
            {
                Escribir_registro("Se ha encontrado un usuario con un número de transacciones negativo");
                enviar_alerta("Número de transacciones inválido", &id, 1);
            }
        }
        else
        {
            Escribir_registro("Error en el formato de la línea del archivo");
        }
    }

    fclose(usuarios); // Cerramos el archivo
}

// Función para crear el proceso Monitor y manejar alertas en tiempo real
void CrearMonitor()
{
    if (pipe(pipefd) == -1)
    {
        perror("Error al crear la tubería");
        Escribir_registro("Se ha producido un fallo en la comunicacion entre procesos");
        exit(1);
    }

    pid_t Monitor = fork(); // crea el proceso monitor
    if (Monitor < 0)
    {
        perror("Error al crear el proceso Monitor");
        Escribir_registro("Se ha producido un fallo en la creacion del proceso monitor");
        return;
    }
    if (Monitor == 0)
    {
        // Código del hijo (Monitor)
        close(pipefd[0]);                                                            // Cierra lectura en el hijo
        pthread_t Transacciones;                                                     // crea el hilo
        if (pthread_create(&Transacciones, NULL, detectar_transacciones, NULL) != 0) // crea el hilo y llama a detecatar transacciones
        {
            perror("Error al crear el hilo de transacciones"); // comprobamos que el hilo se haya creado correctamente
            exit(1);
        }
        Escribir_registro("Se ha creado correctamente el hilo que revisa ls anomalias");
        pthread_join(Transacciones, NULL);                                                                    // cerramos el hilo
        close(pipefd[1]);                                                                                     // Cerrar escritura cuando termine
        Escribir_registro("Se ha cerrado el extremo de escritura de la tuberia del proceso hijo de monitor"); // escrbimos en el registro
        exit(0);
    }
    else
    {
        // Código del padre (lector de alertas y almacenamiento en archivo)
        close(pipefd[1]); // Cierra escritura en el padre
        Escribir_registro("Se ha cerrado el extremo de escritura de la tuberia del proceso padre de monitor");
        // igual hay que hacer aqui un malloc para asignarle memoria al buffer que 256 es poco
        char buffer[256]; // declaramos una variable para leer de la pipe
        int bytes_leidos;
        while (1)
        {
            bytes_leidos = read(pipefd[0], buffer, sizeof(buffer) - 1); // lee la pipe y lo almacena en el buffer
            if (bytes_leidos > 0)
            {
                Escribir_registro("Se ha registrado una alerta en el sistema");
                buffer[bytes_leidos] = '\0'; // Convertir a string, añade el barra cero al final para indicar el final de la cadena en el ultimo char
                registrar_alerta(buffer);    // manda a registrar alerta con lo que tiene la  pipe
            }
            else if (bytes_leidos == 0)
            {
                break; // Salir si no hay más datos
            }
            else
            {
                perror("Error al leer la tubería"); // da error si se lee mal la tuberia
                Escribir_registro("Error al leer la tuberia");
                break;
            }
        }
        close(pipefd[0]); // Cierra lectura al terminar
        Escribir_registro("Se ha cerrado el extremo de lectura de la tuberia del proceso padre de monitor");
    }
}
