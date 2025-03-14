
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define Punt_Archivo_Properties "Variables.properties"
#include "init_cuentas.h"
#include "Usuario.h"
#define MAX_LENGTH 256
#define NUM_USUARIOS 5
#include <semaphore.h>

void *Menu_Usuario(void *arg);


sem_t semaforo;

void Escribir_registro2(const char *mensaje_registro){
    //declaramos la variable time_t
    time_t t;
      struct tm *tm_info;
      char buffer[30];  // Para almacenar la fecha y hora formateadas
  
      // Obtiene la hora actual
      time(&t);
      tm_info = localtime(&t);
  
      // Formatea la fecha y hora en "YYYY-MM-DD HH:MM:SS"
      strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
  
      // Abre el archivo en modo "a" para añadir sin sobrescribir
      FILE *ArchivoDeRegistro = fopen("registro.log", "a");
      if (!ArchivoDeRegistro) {
          perror("Error al abrir el archivo de registro");
          return;
      }
  
      // Escribe la fecha, la hora y el mensaje en el archivo
      fprintf(ArchivoDeRegistro, "[%s] %s\n", buffer, mensaje_registro);
      
      // Cierra el archivo
      fclose(ArchivoDeRegistro);
  }
  
  // Función para abrir y leer el archivo de propiedades 
  //Esta funcion nos permite cargar las variables que usemos en el codigo para que sea mas accesible
  void AbrirPropertis2()
  {
      //Llamamos al registro.log 
      Escribir_registro2("Se ha abierto el archivo de properties de init cuentas");
      char *key, *value;
      char line[MAX_LENGTH];
      char username[MAX_LENGTH] = {0};
      char password[MAX_LENGTH] = {0};
  
      FILE *ArchivoPro = fopen(Punt_Archivo_Properties, "r");
  
      if (!ArchivoPro)
      {
          perror("Error al abrir el archivo de propiedades");
          Escribir_registro2("Fallo al abrir el arhivo de properties");
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
  int contador = 1;
  void Menu_Hilos() {
    pthread_t hilos[NUM_USUARIOS];
    int id[NUM_USUARIOS];
    sem_init(&semaforo, 0, 1); // Inicializamos el semáforo

    // Crear 5 hilos para los usuarios
        
        pthread_create(&hilos[contador], NULL, Menu_Usuario(contador), (void *)&id[contador]);
    
    // Esperamos que todos los hilos terminen

        pthread_join(hilos[contador], NULL);


    sem_destroy(&semaforo); // Destruimos el semáforo

    return;
}

// Función para cada usuario ejecutándose en un hilo
void *Menu_Usuario(void *arg) {
    contador++;
    //printf("👤 Usuario %d ha iniciado el menú.\n", usuario_id);
    Escribir_registro2("Se ha accedido al menú de entrada");
    AbrirPropertis2();
    
    int Eleccion;
    do {
        Escribir_registro2("Se ha abierto el menú de inicio de sesión");

        printf("\n------------------ Elija una opción ----------------------\n");
        printf("|   1. Inicio de sesión                                   |\n");
        printf("|   2. Registro                                          |\n");
        printf("|       Pulse una opción (1/2):                          |\n");
        printf("---------------------------------------------------------\n");
        scanf("%d", &Eleccion);

        if (Eleccion == 1) {
            sem_wait(&semaforo); // Bloqueamos el semáforo
            InicioDeSesion();
            sem_post(&semaforo); // Liberamos el semáforo
            Escribir_registro2("El usuario ha elegido iniciar sesión");
        } else if (Eleccion == 2) {
            Registro();
            Escribir_registro2("El usuario ha elegido la opción de registro");
        }
    } while (Eleccion != 1 && Eleccion != 2);

    return NULL;
}
void InicioDeSesion()
{
    FILE *archivo = fopen("usuarios.dat", "r"); // Abrimos el archivos usuarios.dat
    // declaramos todas las variables
    char user[255];
    char passwd[255];
    char Usuario[255];
    char Contraseña[255];
    char linea[256];
    do
    {

        printf("\n -------------------Inicio de Sesion------------------------ \n ");
        printf("\n Introduce tu nombre de usuario: \n");
        scanf("%s", Usuario);
        printf("\n Contraseña:");
        scanf("%s", Contraseña);
        if (archivo == NULL)
        {
            perror("Error al abrir el archivo");
            exit(-1);
        }

        while (fgets(linea, sizeof(linea), archivo))
        {

            // Parsear la línea del CSV
            if (sscanf(linea, "%254[^,],%254[^,]", user, passwd) == 2)
            {
                // Verificar las condiciones
                if (Usuario == user)
                {
                    if (Contraseña == passwd)
                    {
                        printf("Dando acceso al sistema...");
                        sleep(10);
                        Mostrar_Menu();// Llama a la funcion de usuarios.c
                        Escribir_registro2("Se ha accedido al sistema");
                    }
                    else
                    {
                        printf("Contraseña incorrecta");
                        // abrir el registros.log y introducir que el usuario ha intentado iniciar sesion
                        Escribir_registro2("El usuario ha intentido iniciar sesion y ha tenido la contraseña incorrecta");
                    }
                }
                else
                {
                    printf("Nombre de usuario o contraseña incorrectas");
                    // abrir el registros.log y introducir que el usurio ha intentado iniciar sesion
                    Escribir_registro2("El usuario ha intentado iniciar sesion y ha falldao el usuaio o la contraseña");
                }
            }
        }

    } while (Contraseña == passwd && Usuario == user); // si la contraseña o el usuario no son correctas volvemos a mostrar el menu y este mensaje , tambien se podria hacer un contandor para que te avise de que has fallado muchas veces o algo asi

    fclose(archivo); // cerramos el archivo con fclose
}
void Registro()
{
    Escribir_registro2("El usuario ha entrado el la seccion del registro");
  
    char linea[255];
    struct Cuenta
    {
        int id;                   // id de la cuenta
        char Nombre[50];          // Nombre de usuario de la cuenta
        int saldo;             // saldo de la cuenta
        int Numero_transacciones; // Numero de transacciones
    };

    struct Cuenta cuenta;
    cuenta.Numero_transacciones=0;
    printf("--------Registro-------");
    printf("Introduce tu Nombre : ");
    scanf("%s", cuenta.Nombre);
    printf("Introudce el saldo inicial :");
    scanf("%d", &cuenta.saldo);

    FILE *usuarios=fopen("usuarios.dat","w+");
    if(!usuarios){
        perror("Error al abrir el archivo de propiedades");
        return;
    }
    while(fgets(linea, sizeof(linea), usuarios) !=0){
        cuenta.id++;
    }
    fprintf(usuarios,"%id | %s | %d | %d",cuenta.id , cuenta.Nombre,cuenta.saldo,cuenta.Numero_transacciones);
    Escribir_registro2("Se ha registrado un nuevo usuario en el sistema");
    // Pedir los datos y almacenarlos en el usuario.dat : id,nombre,apellidos,numeroDeCuenta,saldo_inicial
    // Registrar el registro con el .log
}
