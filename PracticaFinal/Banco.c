
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

void CrearCarpetas();
void Menu_Procesos();
void CrearMonitor();
void limpiar(int sig);
void *LeerDeMonitor(void *arg);
void matar_hijos();
void leer_alerta_cola(int sig);
void CrearMemoria();
void Limpiar_MemoriaCompartida();
void ListarCuentas();
void ActualizarArchivoCuentas();
void CrearBuffer();

/// @brief este es el main en el cual leemos propertis con las variables
/// @return y devolvemos 0 si la ejecuccion ha sido exitosa


int main()
{
    Inicializar_semaforos();// Iniciar semaforos

    CrearMemoria();//Crear la memoria compartida

    Escribir_registro("Se ha accedido al main de banco y se han incializado los semaforos en banco.c");

    CrearCarpetas();// Crear carpetas para transacciones

    crear_cola_mensajes(); // Crear la cola de mensajes

    // Lo primero abrimos el archivo de Properties y "nos traemos las variables"

    signal(SIGUSR1, leer_alerta_cola); // Manejar señal del monitor

    CrearBuffer(); // Lanzar buffer

    CrearMonitor(); // Lanzar monitor

    Menu_Procesos(); // Ejecutar usuarios

    ActualizarArchivoCuentas();// Actualizar el archivo de cuentas

    Destruir_semaforos();// se destruyen los semaforos
    
    Limpiar_MemoriaCompartida(); // se desconecta y elimina la memoria compartida
    return 0;
}

/// @brief Esta función se encarga de crear la memoria compartida y
//  cargar las cuentas desde el archivo
void CrearMemoria()
{
    Config config = leer_configuracion("variables.properties");
    key_t clave = ftok("Cuenta.h", 66);
    int shmid = shmget(clave, config.max_cuentas * sizeof(Cuenta), 0666 | IPC_CREAT);
    cuentas = (Cuenta *)shmat(shmid, NULL, 0);
    
    if (shmid == -1)
    {
        Escribir_registro("❌ Error al crear memoria compartida");
        perror("shmget falló");
        exit(1);
    }

    // Abrir el archivo cuentas.txt
    FILE *archivo = fopen("cuentas.txt", "r+");
    if (!archivo)
    {
        Escribir_registro("❌ Error al abrir el archivo cuentas.txt");
        perror("No se pudo abrir cuentas.txt");
        exit(1);
    }

    // Leer y cargar las cuentas en la memoria compartida
    char linea[512];
    int i = 0;
    while (fgets(linea, sizeof(linea), archivo) && i < config.max_cuentas)
    {
        Cuenta cuenta_temp;
        char fecha[20], hora[20];

        // Parsear la línea del archivo
        if (sscanf(linea, "%d|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d|%[^|]|%s",
                   &cuenta_temp.id,
                   cuenta_temp.Nombre,
                   cuenta_temp.Apellido,
                   cuenta_temp.Contraseña,
                   cuenta_temp.domicilio,
                   cuenta_temp.pais,
                   &cuenta_temp.saldo,
                   cuenta_temp.fecha,
                   cuenta_temp.hora) == 9)
        {
            // Copiar la cuenta al arreglo de memoria compartida
            cuentas[i] = cuenta_temp;
            i++;
        }
        else
        {
            Escribir_registro("⚠️ Error al parsear una línea del archivo cuentas.txt");
        }
    }

    fclose(archivo);

    return;
}

/// @brief Esta función se encarga de limpiar la memoria compartida
/// y eliminarla al finalizar el programa
void Limpiar_MemoriaCompartida()
{
    Config config = leer_configuracion("variables.properties");
    key_t clave = ftok("Cuenta.h", 66);
    int shmid = shmget(clave, config.max_cuentas * sizeof(Cuenta), 0666 | IPC_CREAT);
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
/// @brief Cuando llega la señal de SIGUSR1, se ejecuta esta función
/// @param sig
void leer_alerta_cola(int sig)
{
    Escribir_registro("Ha llegado una alerta de monitor mediante una señal en banco.c");
    MensajeAlerta msg;
    int id_cola = msgget(CLAVE_COLA1, 0666);
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

/// @brief Esta función se encarga de matar los procesos hijos
/// y cerrar las sesiones de los usuarios
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

/// @brief Esta función se encarga de mostrar el menú de procesos
/// y gestionar la creación de nuevos usuarios
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
            sleep(1);

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

/// @brief Esta función se encarga de crear el proceso monitor
/// y manejar las alertas en tiempo real
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

        sleep(1); // Dar tiempo al hijo para crear el archivo

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

/// @brief Esta función se encarga de listar las cuentas
/// y mostrar la información de cada una
void ListarCuentas()
{
    printf("\n==============================\n");
    printf("    📋 LISTA DE CUENTAS\n");
    printf("==============================\n");
    Config config = leer_configuracion("variables.properties");
    if (cuentas == NULL)
    {
        printf("❌ Memoria compartida no inicializada.\n");
        return;
    }

    int i = 0;
    int hayCuentas = 0;

    // Recorremos las cuentas hasta encontrar una cuenta con campos vacíos (sin nombre o usuario)
        //sem_wait(sem_MC);
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
            printf("   ID: %d\n", cuentas[i].id);
            printf("   Nombre: %s\n", cuentas[i].Nombre);
            printf("   Apellidos: %s\n", cuentas[i].Apellido);
            printf("   Contraseña: %s\n", cuentas[i].Contraseña);
            printf("   Domicilio: %s\n", cuentas[i].domicilio);
            printf("   País: %s\n", cuentas[i].pais);
            printf("   Saldo: %d\n", cuentas[i].saldo);
            printf("   Fecha de creación: %s\n", cuentas[i].fecha);
            printf("   Hora de creación: %s\n", cuentas[i].hora);
            printf("   -----------------------------\n");
        }

        i++;

        // Seguridad: evitar leer fuera de los límites
        if (i >= config.max_cuentas)
            break;
    }
    sem_post(sem_MC);
}

/// @brief Esta función se encarga de crear las carpetas
/// y archivos de transacciones para cada usuario
void CrearCarpetas()
{
    // Crear la carpeta "transacciones" si no existe
    if (mkdir("transacciones", 0777) == -1 && errno != EEXIST)
    {
        perror("Error al crear la carpeta transacciones");
        Escribir_registro("❌ Error al crear la carpeta transacciones en banco.c");
        return;
    }

    // Abrir el archivo cuentas.txt
    FILE *archivo = fopen("cuentas.txt", "r");
    if (!archivo)
    {
        perror("No se pudo abrir cuentas.txt");
        Escribir_registro("❌ Error al abrir el archivo cuentas.txt en banco.c");
        return;
    }

    // Leer las cuentas del archivo
    char linea[512];
    while (fgets(linea, sizeof(linea), archivo))
    {
        Cuenta cuenta_temp;
        // Parsear la línea del archivo
        if (sscanf(linea, "%d|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d|%[^|]|%s",
                   &cuenta_temp.id,
                   cuenta_temp.Nombre,
                   cuenta_temp.Apellido,
                   cuenta_temp.Contraseña,
                   cuenta_temp.domicilio,
                   cuenta_temp.pais,
                   &cuenta_temp.saldo,
                   cuenta_temp.fecha,
                   cuenta_temp.hora) == 9)
        {
            // Crear una carpeta con el ID del usuario dentro de "transacciones"
            char carpeta_usuario[256];
            snprintf(carpeta_usuario, sizeof(carpeta_usuario), "transacciones/%d", cuenta_temp.id);
            if (mkdir(carpeta_usuario, 0777) == -1 && errno != EEXIST)
            {
                perror("Error al crear la carpeta del usuario");
                Escribir_registro("❌ Error al crear la carpeta del usuario en banco.c");
                continue;
            }

            // Crear el archivo transacciones.log dentro de la carpeta del usuario
            char archivo_transacciones[256];
            if (snprintf(archivo_transacciones, sizeof(archivo_transacciones), "%s/transacciones.log", carpeta_usuario) >= sizeof(archivo_transacciones))
            {
                perror("Error: ruta del archivo transacciones.log demasiado larga");
                Escribir_registro("❌ Error: ruta del archivo transacciones.log demasiado larga en banco.c");
                continue;
            }

            // Comprobar si el archivo ya contiene datos
            FILE *log = fopen(archivo_transacciones, "r");
            if (log)
            {
                fclose(log);
                continue; // Si el archivo ya existe, no lo sobrescribimos
            }

            // Crear y escribir en el archivo si no existe
            log = fopen(archivo_transacciones, "a");
            if (!log)
            {
                perror("Error al crear transacciones.log");
                Escribir_registro("❌ Error al crear transacciones.log en banco.c");
                continue;
            }


            // Escribir datos en un formato más bonito
            fprintf(log, "--------------------------------------------\n");
            fprintf(log, "🏦 TRANSACCIONES DEL USUARIO\n");
            fprintf(log, "--------------------------------------------\n");
            fprintf(log, "Nombre: %s\n", cuenta_temp.Nombre);
            fprintf(log, "Apellidos: %s\n", cuenta_temp.Apellido);
            fprintf(log, "Número de Cuenta: %d\n", cuenta_temp.id);
            fprintf(log, "Nacionalidad: %s\n", cuenta_temp.pais);
            fprintf(log, "--------------------------------------------\n\n");
            fclose(log);
        }
        else
        {
            Escribir_registro("⚠️ Error al parsear una línea del archivo cuentas.txt en banco.c");
        }
    }

    fclose(archivo);
    Escribir_registro("✅ Carpetas de transacciones creadas correctamente en banco.c");
}

/// @brief Esta función se encarga de actualizar el archivo de cuentas
/// y sincronizarlo con la memoria compartida
void ActualizarArchivoCuentas()
{
    FILE *archivo = fopen("cuentas.txt", "r+");
    if (!archivo)
    {
        perror("No se pudo abrir cuentas.txt para lectura/escritura");
        Escribir_registro("❌ Error al abrir cuentas.txt para lectura/escritura en banco.c");
        return;
    }
    Config config = leer_configuracion("variables.properties");
    char linea[512];
    long posicion;
    int cuenta_actualizada = 0;
  // 
  sem_wait(sem_MC);
    while (fgets(linea, sizeof(linea), archivo))
    {
        Cuenta cuenta_temp;
        char fecha[20], hora[20];

        // Parsear la línea del archivo
        if (sscanf(linea, "%d|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d|%[^|]|%s",
                   &cuenta_temp.id,
                   cuenta_temp.Nombre,
                   cuenta_temp.Apellido,
                   cuenta_temp.Contraseña,
                   cuenta_temp.domicilio,
                   cuenta_temp.pais,
                   &cuenta_temp.saldo,
                   fecha,
                   hora) == 9)
        {
            // Buscar la cuenta en la memoria compartida
            for (int i = 0; i < config.max_cuentas; i++)
            {
                if (cuentas[i].id == cuenta_temp.id)
                {
                    // Actualizar la línea en el archivo
                    posicion = ftell(archivo) - strlen(linea);
                    fseek(archivo, posicion, SEEK_SET);
                    fprintf(archivo, "%d|%s|%s|%s|%s|%s|%d|%s|%s\n",
                            cuentas[i].id,
                            cuentas[i].Nombre,
                            cuentas[i].Apellido,
                            cuentas[i].Contraseña,
                            cuentas[i].domicilio,
                            cuentas[i].pais,
                            cuentas[i].saldo,
                            cuentas[i].fecha,
                            cuentas[i].hora);
                    fflush(archivo);
                    cuenta_actualizada = 1;
                    break;
                }
            }
        }
    }
    sem_post(sem_MC);

    if (!cuenta_actualizada)
    {
        Escribir_registro("⚠️ No se encontró ninguna cuenta para actualizar en banco.c");
    }
    else
    {
        Escribir_registro("✅ Archivo de cuentas actualizado correctamente en banco.c");
    }

    fclose(archivo);
}

/// @brief Esta función se encarga de crear el proceso buffer
/// y manejar la comunicación entre procesos
void CrearBuffer()
{
    pid_t Buffer = fork(); // Crea el proceso buffer
    Escribir_registro("Se crea el fork de buffer desde banco.c");
    if (Buffer < 0)
    {
        perror("Error al crear el proceso Buffer");
        Escribir_registro("Fallo al crear el proceso buffer desde banco.c");
        return;
    }

    if (Buffer == 0)
    {
        // Proceso hijo
        Escribir_registro("Proceso buffer hijo iniciado desde banco.c");

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
            perror("No se pudo crear el archivo de PID para buffer");
        }

        // Ejecutar el programa buffer_
        execl("./buffer", "buffer", (char *)NULL);

        // Si execl falla:
        perror("Error al ejecutar buffer_");
        Escribir_registro("Error al ejecutar buffer_ desde banco.c");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Proceso padre
        char filename[50];
        snprintf(filename, sizeof(filename), "/tmp/pid_%d.txt", Buffer);

        sleep(2); // Dar tiempo al hijo para crear el archivo

        FILE *fp = fopen(filename, "r+");
        if (fp)
        {
            if (fscanf(fp, "%d", &temp[contador]) == 1)
            {
                sleep(1);
            }
            else
            {
                perror("No se pudo leer un número del archivo del buffer\n");
            }
            contador++;
            fclose(fp);
        }
        else
        {
            perror("Error al leer el archivo de PID para buffer");
            hijos[num_hijos] = Buffer; // fallback
        }

        num_hijos++;
        Escribir_registro("Proceso buffer creado correctamente desde banco.c");
    }
}
