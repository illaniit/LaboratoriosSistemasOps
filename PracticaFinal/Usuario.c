
#include "Usuario.h"
#include "Operaciones.h"
#include "Comun.h"
/// @brief
/// En este bloque de codigo mostraremos el menu del usuario donde le daremos opciones a realizar diferentes operaciones

// Aqui hay que meter las funciones de properties y
struct Usuario
{
    char Usuario[50];
    char Contraseña[50];
} Usuario;

/// @brief En este menu permitimos al usuario realizar distintas funciones y para cada funcion creamos un hilo del proceso en el que le
// pasamos un struct con el usuario y la contraseña de la cuenta
/// @param user
/// @param passwd
void Mostrar_Menu(char *user, char *passwd)
{

    // Cargamos configuración desde el archivo .properties
    Config config = leer_configuracion("variables.properties");

    // Inicializamos el struct del usuario con los datos ingresados
    struct Usuario usuario;
    strcpy(usuario.Usuario, user);
    strcpy(usuario.Contraseña, passwd);

    // Declaramos los hilos para las operaciones
    pthread_t hilo1, hilo2, hilo3, hilo4;
    int Eleccion = 0;

    // Abrimos los semáforos necesarios para controlar el acceso a recursos compartidos
    sem_usuarios = sem_open("/sem_usuarios", 0);
    sem_transacciones = sem_open("/sem_transacciones", 0);


    // Iniciamos ciclo del menú hasta que el usuario elija salir
    do
    {
        system("clear");
        printf("\n====================================\n");
        printf("        🏦 MENÚ INTERACTIVO 🏦      \n");
        printf("====================================\n\n");
        printf("1️⃣  Introducir Dinero\n");
        printf("2️⃣  Extraer Dinero\n");
        printf("3️⃣  Hacer una Transferencia\n");
        printf("4️⃣  Consultar mis Datos\n");
        printf("5️⃣  Cerrar Sesión\n\n");
        printf("------------------------------------\n");
        printf("\n🔹 Ingrese su opción: ");

        // Validamos la entrada del usuario
        if (scanf("%d", &Eleccion) != 1)
        {
            printf("❌ Entrada inválida. Intente nuevamente.\n");
            while (getchar() != '\n')
                ; // Limpiar el buffer de entrada
            sleep(2);
            continue;
        }

        switch (Eleccion)
        {
        case 1:
            if (pthread_create(&hilo1, NULL, IntroducirDinero, &usuario) != 0)
                printf("❌ Error al crear el hilo para Introducir Dinero.\n");

            else
                pthread_join(hilo1, NULL); // Esperamos a que termine
            break;

        case 2:
            if (pthread_create(&hilo2, NULL, ExtraerDinero, &usuario) != 0)
                printf("❌ Error al crear el hilo para Extraer Dinero.\n");

            else
                pthread_join(hilo2, NULL);

            break;

        case 3:
            if (pthread_create(&hilo3, NULL, Transferencia, &usuario) != 0)
                printf("❌ Error al crear el hilo para Transferencia.\n");

            else
                pthread_join(hilo3, NULL);

            break;

        case 4:
            if (pthread_create(&hilo4, NULL, ConsultarDatos, &usuario) != 0)
                printf("❌ Error al crear el hilo para Consultar Datos.\n");

            else
                pthread_join(hilo4, NULL);

            break;

        case 5:
            printf("🔒 Cerrando sesión... Hasta pronto!\n");
            sleep(2);
            break;

        default:
            printf("❌ Opción inválida. Inténtelo de nuevo.\n");
            sleep(2);
            break;
        }

    } while (Eleccion != 5); // Repetir mientras no se elija salir
}