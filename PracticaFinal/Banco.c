
#include "Comun.h"
#define Cantidad_limite 1000
#define Punt_Archivo_Properties "Variables.properties"
#define MAX_LENGTH 256
#define MAX_HIJOS 100
#define MAX_CUENTAS 100
#include "Cuenta.h"

// Aquí definimos las variables globales

pid_t hijos[MAX_HIJOS];
int num_hijos = 0; // Contador de hijos creados
int temp[100];     // Arreglo donde almacenarás los números

// Definimos las funciones  que vamos a utilizar
Cuenta *cuentas;

void Menu_Procesos();
void CrearMonitor();
void limpiar(int sig);
void *LeerDeMonitor(void *arg);
void matar_hijos();
void leer_alerta_cola(int sig);
void CrearMemoria();
void Limpiar_MemoriaCompartida();
void ListarCuentas();

/// @brief este es el main en el cual leemos propertis con las variables
/// @return y devolvemos 0 si la ejecuccion ha sido exitosa
int main()
{

    Inicializar_semaforos();

    CrearMemoria();
    Escribir_registro("Se ha accedido al main de banco y se han incializado los semaforos en banco.c");

    crear_cola_mensajes();

    Config config = leer_configuracion("variables.properties");
    // Lo primero abrimos el archivo de Properties y "nos traemos las variables"

    signal(SIGUSR1, leer_alerta_cola); // Manejar señal del monitor

    CrearMonitor(); // Lanzar monitor

    Menu_Procesos(); // Ejecutar usuarios

    Destruir_semaforos();

    // Limpiar_MemoriaCompartida(); // Limpiar memoria compartida
    Escribir_registro("Se ha limpiado la memoria compartida en banco.c");

    return 0;
}

void CrearMemoria()
{
    key_t clave = ftok("Cuenta.h", 65);
    printf("[DEBUG] Clave generada: %d\n", clave);
    int shmid = shmget(clave, MAX_CUENTAS * sizeof(Cuenta), 0666 | IPC_CREAT);
    cuentas = (Cuenta *)shmat(shmid, NULL, 0);

    if (shmid == -1)
    {
        perror("shmget falló");
        exit(1);
    }

  

    printf("✅ Memoria compartida creada con ID: %d\n", shmid);
    return;
}

void Limpiar_MemoriaCompartida()
{
    shmdt(cuentas);
    key_t clave = ftok("Cuenta.h", 65);
    int shmid = shmget(clave, MAX_CUENTAS * sizeof(Cuenta), 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("shmget falló");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl falló");
        exit(1);
    }
    printf("✅ Memoria compartida eliminada\n");
    return;
}
/// @brief
/// @param sig
void leer_alerta_cola(int sig)
{
    Escribir_registro("Ha llegado una alerta de monitor mediante una señal en banco.c");
    MensajeAlerta msg;
    int id_cola = msgget(CLAVE_COLA, 0666);
    if (id_cola == -1)
    {
        perror("Error al obtener cola");
        return;
    }

    if (msgrcv(id_cola, &msg, sizeof(msg.texto), TIPO_ALERTA, IPC_NOWAIT) == -1)
    {
        perror("No se pudo leer alerta de la cola");
        return;
    }
    Escribir_registro("se ha accedido a la cola en banco.c");
    // Abrir el archivo alertas.log en modo append
    FILE *archivo = fopen("alertas.log", "a");
    if (!archivo)
    {
        perror("No se pudo abrir alertas.log");
        return;
    }

    // Escribir la alerta en el archivo
    fprintf(archivo, "🚨 ALERTA DEL MONITOR 🚨\n%s\n", msg.texto);
    fclose(archivo);
    Escribir_registro("Se ha escrito en el archvio una alerta en banco.c");

    printf("🚨 Se ha registrado una nueva alerta\n");

    sleep(4);
    Escribir_registro("Se muestra que se ha registrado una alerta desde banco.c");

    // Mueve el cursor una línea arriba y limpia esa línea
    printf("\033[F");  // Mueve el cursor una línea arriba
    printf("\033[2K"); // Borra toda la línea
    fflush(stdout);    // Asegura que el borrado se aplique antes de terminar
}

void matar_hijos()
{
    printf("\n Cerrando sesiones...\n");
    system("clear");
    for (int i = 0; i < num_hijos; i++)
    {

        // Matar proceso hijo directamente
        char comando[100];
        snprintf(comando, sizeof(comando), "kill -9 %d", temp[i]);
        system(comando);
        sleep(0.5);
    }
    Escribir_registro("se ha matado los hijos desde banco.c");
}
int contador = 0;
void Menu_Procesos()
{
    int respuesta;
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
            Escribir_registro("se ha abierto una nueva terminal con usuario desde banco.c");

            exit(0); // El hijo termina aquí, la terminal sigue corriendo
        }
        else if (pid > 0)
        {
            // Esperar un momento para que el archivo con el PID se cree
            sleep(1);
            char filename[50];
            // Leer el PID real del hijo desde el archivo
            snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", pid);
            Escribir_registro("se ha generado un archvio con el pid del usuario desde banco.c");

            // Esperamos 20 segundos (puedes quitar esto si no es necesario)
            sleep(3);

            // Abrimos el archivo en modo lectura y escritura
            FILE *fp = fopen(filename, "r+");

            if (fp)
            {

                // Leemos un único número entero del archivo
                if (fscanf(fp, "%d", &temp[contador]) == 1)
                {
                    sleep(1);
                }
                else
                {
                    perror("No se pudo leer un número del archivo.\n");
                }
                contador++;
                // Cerramos el archivo
                fclose(fp);
                Escribir_registro("Se ha alamcenado el pid del usuario desde banco.c");
            }
            else
            {
                perror("Error al leer el archivo de PID");
                hijos[num_hijos] = pid; // Si falla, usar el PID de fork()
            }

            num_hijos++;
            int valido = 0;
            do
            {
                system("clear");

                // Menú
                printf("\n");
                printf("╔════════════════════════════════════════════════╗\n");
                printf("║     🏦  BANCO CENTRAL - GESTIÓN DE USUARIOS    ║\n");
                printf("╠════════════════════════════════════════════════╣\n");
                printf("║  1.  Registrar otro usuario                    ║\n");
                printf("║  2.  Listar cuentas                            ║\n");
                printf("║  3.  Cerrar el sistema                         ║\n");
                printf("╚════════════════════════════════════════════════╝\n");
                printf("Seleccione una opción (1, 2 o 3):\n");

                if (scanf("%d", &respuesta) != 1)
                {
                    printf("\n⚠️  Entrada no válida. Solo números del 1 al 3.\n");
                    while (getchar() != '\n')
                        ; // Limpiar buffer
                    sleep(2);
                    continue;
                }

                if (respuesta == 1)
                {
                    valido = 1; // opción válida, seguir
                }
                else if (respuesta == 2)
                {
                    system("clear");
                    ListarCuentas(); // Llamar a la función para listar cuentas
                    printf("\nPresione Enter para continuar...");
                    while (getchar() != '\n')
                        ; // Esperar a que el usuario presione Enter
                    getchar();
                }
                else if (respuesta == 3)
                {
                    printf("\n👋 Cerrando el sistema. ¡Gracias por usar el banco!\n");
                    sleep(2);
                    continuar = 0;
                    valido = 1;
                }
                else
                {
                    printf("\n⚠️  Opción fuera de rango. Intente nuevamente.\n");
                    sleep(2);
                }

            } while (!valido);
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
    Escribir_registro("Se acaban con todos los procesos desde banco.c");
}
// Esta funcion se encarga de "pegar" la alerta en el pipe para poder pasarlo al proceso padre

// Función para crear el proceso Monitor y manejar alertas en tiempo real
void CrearMonitor()
{
    pid_t Monitor = fork(); // Crea el proceso monitor
    Escribir_registro("Se crea el fork de monitor desde banco.c");
    if (Monitor < 0)
    {
        perror("Error al crear el proceso Monitor");
        Escribir_registro("Fallo al crear el proceso monitor desde banco.c");
        return;
    }

    if (Monitor == 0)
    {
        // Proceso hijo
        Escribir_registro("Proceso monitor hijo iniciado desde banco.c");

        // Guardar PID en archivo
        char filename[50];
        snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", getpid());

        FILE *fp = fopen(filename, "w");
        if (fp)
        {
            fprintf(fp, "%d\n", getpid());
            fclose(fp);
        }
        else
        {
            perror("No se pudo crear el archivo de PID");
        }

        // Ejecutar el programa monitor
        execl("./monitor", "monitor", (char *)NULL);

        // Si execl falla:
        perror("Error al ejecutar monitor");
        Escribir_registro("Error al ejecutar monitor desde banco.c");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Proceso padre
        char filename[50];
        snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", Monitor);

        sleep(5); // Dar tiempo al hijo para crear el archivo

        FILE *fp = fopen(filename, "r+");
        if (fp)
        {
            if (fscanf(fp, "%d", &temp[contador]) == 1)
            {
                sleep(1);
            }
            else
            {
                perror("No se pudo leer un número del archivo.\n");
            }
            contador++;
            fclose(fp);
        }
        else
        {
            perror("Error al leer el archivo de PID");
            hijos[num_hijos] = Monitor; // fallback
        }

        num_hijos++;
        Escribir_registro("Proceso monitor creado correctamente desde banco.c");
    }
}
void ListarCuentas()
{
    printf("\n==============================\n");
    printf("    📋 LISTA DE CUENTAS\n");
    printf("==============================\n");

    if (cuentas == NULL)
    {
        printf("❌ Memoria compartida no inicializada.\n");
        return;
    }

    int i = 0;
    int hayCuentas = 0;

    // Recorremos las cuentas hasta encontrar una cuenta con campos vacíos (sin nombre o usuario)
    while (1)
    {
        // Comprobamos si la cuenta está "vacía" (sin nombre o usuario)
        if (strlen(cuentas[i].Nombre) == 0 || strlen(cuentas[i].Apellido) == 0)
        {
            // Si hemos encontrado al menos una cuenta válida, salimos
            if (hayCuentas)
                break;

            // Si no hay ninguna cuenta válida, se asume que no hay más cuentas
            if (i == 0)
            {
                printf("⚠️ No hay cuentas activas registradas.\n");
                return;
            }
        }
        else
        {
            hayCuentas = 1;

            printf("🧾 Cuenta #%d\n", i + 1);
            printf("   Nombre: %s\n", cuentas[i].Nombre);
            printf("   Apellidos: %s\n", cuentas[i].Apellido);
            printf("   Contraseña: %s\n", cuentas[i].Contraseña);
            printf("   País: %s\n", cuentas[i].pais);
            printf("   Saldo: %d\n", cuentas[i].saldo);
            printf("   Transacciones: %d\n", cuentas[i].Numero_transacciones);
            printf("   -----------------------------\n");
        }

        i++;

        // Seguridad: evitar leer fuera de los límites
        if (i >= MAX_CUENTAS)
            break;
    }
}