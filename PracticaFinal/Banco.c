
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
#define MONTO_LIMITE 1000
#define Punt_Archivo_Properties "Variables.properties"
#define MAX_LENGTH 256

int pipefd[2]; // Tubería que la declaramos para poder pasar informacion de monitor a banco y registrarlo en alertas .txt

// Definimos las funciones  que vamos a utilizar
void Menu_Procesos();
void AbrirPropertis();
void CrearMonitor();
void *detectar_transacciones(void *arg);
void enviar_alerta(const char *mensaje,const int *id,const char *titular);
void limpiar(int sig);
void registrar_alerta(const char *mensaje);
void Escribir_registro(const char *mensaje_registro);


int main()
{
    //Lo primero abrimos el archivo de Properties y "nos traemos las variables"
    AbrirPropertis();
    //Iniciamos monitor para que encuentre  anomalias 
    CrearMonitor();

    //Cargamos el menu del usuario que se encuentra en init_cuentas.c donde cada usuario sera un hilo de ejecuccion
    Menu_Procesos();

    //Volvemos a llamara monitor para que encuentre las anomalias despues de que el usuario haya cerrado la sesion
    CrearMonitor();
    return 0;
}

//Funcion para escribir en el registro de log 
void Escribir_registro(const char *mensaje_registro){
  //declaramos la variable time_t
    time_t t;
    struct tm *tm_info;
    char hora[30];  // Para almacenar la fecha y hora formateadas

    // Obtiene la hora actual
    time(&t);
    tm_info = localtime(&t);

    // Formatea la fecha y hora en "YYYY-MM-DD HH:MM:SS"
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);

    // Abre el archivo en modo "a" para añadir sin sobrescribir
    FILE *ArchivoDeRegistro = fopen("registro.log", "a");
    if (!ArchivoDeRegistro) {
        perror("Error al abrir el archivo de registro");
        return;
    }

    // Escribe la fecha, la hora y el mensaje en el archivo
    fprintf(ArchivoDeRegistro, "[%s] %s\n", hora, mensaje_registro);
    
    // Cierra el archivo
    fclose(ArchivoDeRegistro);
}




// Función para abrir y leer el archivo de propiedades 
//Esta funcion nos permite cargar las variables que usemos en el codigo para que sea mas accesible
void AbrirPropertis()
{


    //Llamamos al registro.log 
    Escribir_registro("Se ha abierto el archivo de properties");
    char *key, *value;
    char line[MAX_LENGTH];
    char username[MAX_LENGTH] = {0};
    char password[MAX_LENGTH] = {0};

    FILE *ArchivoPro = fopen(Punt_Archivo_Properties, "r");

    if (!ArchivoPro)
    {
        perror("Error al abrir el archivo de propiedades");
        Escribir_registro("Fallo al abrir el arhivo de properties");
        return;
    }

    while (fgets(line, MAX_LENGTH, ArchivoPro))
    {

        line[strcspn(line, "\n")] = 0;
        key = strtok(line, "=");
        value = strtok(NULL, "=");
        if (key && value)
        {
            if (strcmp(key, "username") == 0)
            {
                strncpy(username, value, MAX_LENGTH - 1);
            }
            else if (strcmp(key, "password") == 0)
            {
                strncpy(password, value, MAX_LENGTH - 1);
            }
        }
    }

    fclose(ArchivoPro);
}
void Menu_Procesos()
{

    char respuesta;
    int continuar = 1;
    while (continuar)
    {
        pid_t pid = fork(); // Crear un proceso hijo
        if (pid == 0)
        {                   // Si estamos en el proceso hijo
            execlp("gnome-terminal", "gnome-terminal", "--", "./usuario", NULL) ; // Llamamos a la función que maneja el usuario
             // Liberar el semáforo para que el padre sepa que terminó
            exit(0);        // Termina el proceso hijo
        }
        else if (pid > 0)
        {                  // Si estamos en el proceso padre
             // Espera a que el hijo termine (semaforo bloqueado hasta que el hijo lo libere)

            // Preguntar si desea aceptar otro usuario después de que el hijo termine
            printf("¿Desea aceptar otro usuario? (s/n): ");
            scanf(" %c", &respuesta); // Espacio antes de %c para consumir el salto de línea pendiente
            if (respuesta != 's' && respuesta != 'S')
            {
                continuar = 0; // Si no quiere otro usuario, salimos del bucle
            }
        }
        else
        {
            perror("Error en fork");
            exit(EXIT_FAILURE);
        }
    }

    // El padre espera a que todos los procesos hijos terminen antes de cerrar
    while (wait(NULL) > 0) ; // Espera a que todos los hijos terminen

    // Cerramos el semáforo y lo eliminamos

   
}

// Esta funcion se encarga de "pegar" la alerta en el pipe para poder pasarlo al proceso padre

void enviar_alerta(const char *mensaje, const int *id,const char *titular)
{
    Escribir_registro("Se ha enviado una alerta");

    time_t t;
    struct tm *tm_info;
    char hora[30];  // Para almacenar la fecha y hora formateadas

    // Obtiene la hora actual
    time(&t);
    tm_info = localtime(&t);

    // Formatea la fecha y hora en "YYYY-MM-DD HH:MM:SS"
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);
    char auxiliar[256];
    if(titular != ""){
        snprintf(auxiliar, sizeof(auxiliar), "[%s] %d | %s | %s\n", hora,*id,titular, mensaje); // esta funcion nos permite agregar un \n en el archivo de texto para que sea entendible y legible
    }
    else{
        snprintf(auxiliar, sizeof(auxiliar), "[%s] %d | Transaccion | %s\n",hora, *id, mensaje); // esta funcion nos permite agregar un \n en el archivo de texto para que sea entendible y legible
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
    FILE *archivo_alertas = fopen("alertas.txt", "a"); // Abrimos el fichero
    if (archivo_alertas)
    {
        fprintf(archivo_alertas, "%s\n", mensaje); // escribimos el mensaje
        fclose(archivo_alertas); // cerramos el archivo
    }
    else
    {
        perror("Error al abrir alertas.txt");
        Escribir_registro("Se ha producido un error al abrir el fichero de alertas");
    }
}

// Función para detectar transacciones sospechosas
// Lo que hago en esta funcion es abrir los dos ficheritos y mirar si hay cosas raras , enviarlas a la funcion que ya me habia creado
// , el trabajo duro ya estaba hecho =)
void *detectar_transacciones(void *arg)
{
    signal(SIGINT, limpiar); // limpiamos la tuberia 
    Escribir_registro("Se ha abierto el fichero de transacciones");
    FILE *archivo = fopen("transaciones.txt", "r"); // abrimos el archivo transacciones para buscar anomalias en el
    if (!archivo)
    {
        perror("Error al abrir el archivo de transacciones"); // Comprobamos que el archivo no de error 
        Escribir_registro("Se ha producido un error al abrirr el archivo de transacciones");
        return NULL; // devolvemos null en este caso
        
    }
    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) // leemos el archivo transacciones.txt
    {
        int id, saldo1, saldo2, saldo_final; // declaramos las variables 
        char tipo[20], cuenta1[20], cuenta2[20];
        if (sscanf(linea, "%d | %19[^,],%19[^,],%19[^,],%d,%d,%d", &id, tipo, cuenta1, cuenta2, &saldo1, &saldo2, &saldo_final) == 7) // leemos los campos correspondientes
        {
            if (strcmp(tipo, "retiro") == 0 || strcmp(tipo, "ingreso") == 0) // comprobamso que los campos sean validos es decir si es un ingreso o un retiro esos campos de cuenta 2 y saldo 2 deben estar vacios
            {
                if (strlen(cuenta2) > 0 || saldo2 != 0) // comprobamos que  
                {
                    Escribir_registro("Se ha detectado una transaccion invalida");
                    enviar_alerta("Transacción inválida, campos incorrectos en retiro o ingreso",&id,"");
                }
            }
            if (saldo1 < 0 || saldo2 < 0 || saldo_final < 0) // comprobamso que cualquiera de los 3 campos que indican saldo sean positivos
            {
                Escribir_registro("Se ha detectado un saldo negativo");
                enviar_alerta("Saldo negativo detectado",&id,"");
            }
        }
    }

    fclose(archivo); // cerramos el archivo

    // Verificación de usuarios.dat
     // hacemos exactamente lo mismo pero con el fichero que almacena los usuarios
    FILE *usuarios = fopen("usuarios.txt", "r"); //abrimos el archivo
    if (!usuarios)
    {
        perror("Error al abrir el archivo de usuarios");
        Escribir_registro("Se ha producido un error en la apertura del archivo de usuarios");
        return NULL;
    }
    Escribir_registro("Se ha abierto el archivo de usuarios");

    

    while (fgets(linea, sizeof(linea), usuarios)) { // Leemos línea por línea
        int id, saldo, num_transacciones;
        char nombre[50], apellidos[50], domicilio[100], pais[50];

        // Parseamos la línea según el formato: id | nombre | apellidos | domicilio | pais | saldo | numero_transacciones
        if (sscanf(linea, "%d | %49[^|] | %49[^|] | %99[^|] | %49[^|] | %d | %d", 
                   &id, nombre, apellidos, domicilio, pais, &saldo, &num_transacciones) == 7) {

            if (saldo < 0) {
                Escribir_registro("Se ha encontrado un usuario con saldo negativo");
                enviar_alerta("Usuario con saldo negativo", &id, nombre);
            }

            if (num_transacciones < 0) {
                Escribir_registro("Se ha encontrado un usuario con un número de transacciones negativo");
                enviar_alerta("Número de transacciones inválido", &id, nombre);
            }
        } else {
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

    pid_t Monitor = fork();
    if (Monitor < 0)
    {
        perror("Error al crear el proceso Monitor");
        Escribir_registro("Se ha producido un fallo en la creacion del proceso monitor");
        return;
    }
    if (Monitor == 0)
    {
        // Código del hijo (Monitor)
        close(pipefd[0]); // Cierra lectura en el hijo
        pthread_t Transacciones;
        if (pthread_create(&Transacciones, NULL, detectar_transacciones, NULL) != 0) // crea el hilo y llama a detecatar transacciones
        {
            perror("Error al crear el hilo de transacciones"); // comprobamos que el hilo se haya creado correctamente
            exit(1);
        }
        Escribir_registro("Se ha creado correctamente el hilo que revisa ls anomalias");
        pthread_join(Transacciones, NULL); //cerramos el hilo
        close(pipefd[1]); // Cerrar escritura cuando termine
        Escribir_registro("Se ha cerrado el extremo de escritura de la tuberia del proceso hijo de monitor");
        exit(0);
    }
    else
    {
        // Código del padre (lector de alertas y almacenamiento en archivo)
        close(pipefd[1]); // Cierra escritura en el padre
        Escribir_registro("Se ha cerrado el extremo de escritura de la tuberia del proceso padre de monitor");
        //igual hay que hacer aqui un malloc para asignarle memoria al buffer que 256 es poco
        char buffer[256]; // declaramos una variable para leer de la pipe
        int bytes_leidos;
        while (1)
        {
            bytes_leidos = read(pipefd[0], buffer, sizeof(buffer) - 1); // lee la pipe y lo almacena en el buffer
            if (bytes_leidos > 0)
            {
                Escribir_registro("Se ha registrado una alerta en el sistema");
                buffer[bytes_leidos] = '\0'; // Convertir a string, añade el barra cero al final para indicar el final de la cadena en el ultimo char
                registrar_alerta(buffer); // manda a registrar alerta con lo que tiene la  pipe
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
