
#include "Operaciones.h"
#include "Comun.h"
#include "Cuenta.h"
#define MAX_CUENTAS 100

 // Definido en otro archivo (main o banco.c)


void DatosCuenta(char *user, char *passwd);
void ConsultarTransferencias(char *user, char *passwd);
bool ComprobarCuenta(char *cuenta, char *contraseña);

// Estructura para pasar datos al hilo
struct UsuarioInfo {
    char Usuario[50];
    char Contraseña[50];
};

void *ConsultarDatos(void *arg) {
    struct UsuarioInfo *usuario = (struct UsuarioInfo *)arg;
    int Eleccion = 0;

    do {
        Escribir_registro("El usuario ha desplegado el menú de consulta de datos en ConsultarDatos.c");
        system("clear");

        printf("\n----------📊 Menu de consulta de datos 📊----------\n");
        printf("1️⃣  Datos de tu cuenta\n");
        printf("2️⃣  Consultar las transferencias\n");
        printf("3️⃣  Volver al menú\n");
        printf("Introduce tu elección: ");

        if (scanf("%d", &Eleccion) != 1) {
            printf("Entrada inválida. Inténtalo de nuevo.\n");
            while (getchar() != '\n');
            continue;
        }

        switch (Eleccion) {
            case 1:
                DatosCuenta(usuario->Usuario, usuario->Contraseña);
                Escribir_registro("El usuario ha elegido la opción de consultar datos.");
                break;
            case 2:
                ConsultarTransferencias(usuario->Usuario, usuario->Contraseña);
                Escribir_registro("El usuario ha elegido la opción de consultar transferencias.");
                break;
            case 3:
                printf("Volviendo al menú...\n");
                sleep(2);
                break;
            default:
                printf("Opción no válida, intenta de nuevo.\n");
        }

    } while (Eleccion != 3);

    return NULL;
}

void DatosCuenta(char *user, char *passwd) {
    system("clear");
    bool encontrado = false;
    char var[100];
    Cuenta cuenta;

    // 🔐 Esperar a los semáforos
    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    // ✅ Inicializar memoria compartida
    key_t clave = ftok("Cuenta.h", 65);
    if (clave == -1) {
        perror("❌ Error al generar clave con ftok");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return;
    }

    int shmid = shmget(clave, sizeof(Cuenta) * MAX_CUENTAS, 0666);
    if (shmid == -1) {
        perror("❌ Error al obtener el segmento de memoria compartida");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return;
    }

    Cuenta *cuentas = (Cuenta *) shmat(shmid, NULL, 0);
    if (cuentas == (void *) -1) {
        perror("❌ Error al enlazar memoria compartida con shmat");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return;
    }

    // 🔎 Buscar el usuario en la memoria
    limpiar_cadena(user);
    limpiar_cadena(passwd);

    for (int i = 0; i < MAX_CUENTAS; i++) {
        Cuenta c = cuentas[i];

        if (strlen(c.Nombre) == 0) continue;

        limpiar_cadena(c.Nombre);
        limpiar_cadena(c.Contraseña);

        if (strcmp(c.Nombre, user) == 0 && strcmp(c.Contraseña, passwd) == 0) {
            cuenta = c;
            encontrado = true;
            break;
        }
    }

    // ✅ Liberar la memoria compartida y semáforos
 
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    // 📋 Mostrar datos
    if (encontrado) {
        Escribir_registro("El usuario ha visto sus datos en ConsultarDatos.c");
        printf("\n=================================================\n");
        printf("              💳 DATOS DE LA CUENTA 💳              \n");
        printf("=================================================\n");
        printf("🔹 Numero de cuenta:   %d\n", cuenta.id);
        printf("👤 Nombre:             %s\n", cuenta.Nombre);
        printf("🗂️  Apellidos:         %s\n", cuenta.Apellido);
        printf("🏠 Domicilio:          %s\n", cuenta.domicilio);
        printf("🌍 País:               %s\n", cuenta.pais);
        printf("💰 Saldo:              %d\n", cuenta.saldo);
        printf("🔄 Transacciones:      %d\n", cuenta.Numero_transacciones);
        printf("=================================================\n");
        printf("📌 Presione 's' para volver al menú principal... ");
        scanf(" %s", var);
    } else {
        printf("\n⚠️ Usuario o contraseña incorrectos.\n");
        sleep(2);
    }
}

void ConsultarTransferencias(char *user, char *passwd) {
    char var;
    system("clear");

    Config config = leer_configuracion("variables.properties");

    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    FILE *archivoTransacciones = fopen(config.archivo_log, "r");
    if (!archivoTransacciones) {
        perror("❌ Error al abrir transacciones.txt");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return;
    }

    char linea[256];
    int transacciones_encontradas = 0;
    int user_id = obtener_id_usuario(user, passwd);

    printf("\n=================================================\n");
    printf("        💳 CONSULTA DE TRANSFERENCIAS 💳 \n");
    printf("=================================================\n");

    while (fgets(linea, sizeof(linea), archivoTransacciones)) {
        int id, id1, id2, saldo1, saldo2, saldofinal1, saldofinal2;
        char tipo[20];

        if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &id1, &id2, &saldo1, &saldo2, &saldofinal1, &saldofinal2) == 8) {
            
            if (user_id == id1 || user_id == id2) {
                transacciones_encontradas = 1;
                printf("\nNumero de transferencia: %d | Tipo: %s | De cuenta: %d | A cuenta: %d\n",
                       id, tipo, id1, id2);
                printf("Saldo antes: %d | Saldo después: %d\n", saldo1, saldofinal1);
                printf("-------------------------------------------------\n");
            }
        }
    }

    fclose(archivoTransacciones);
    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    if (!transacciones_encontradas) {
        printf("\n❗ No se han encontrado transferencias para este usuario.\n");
    }

    printf("\n📌 Presione 's' para volver al menú principal... ");
    scanf(" %c", &var);
}
