
#include "Operaciones.h"
#include "Comun.h"
#include "Cuenta.h"
//Utilizamos este struct para almacenar los datos 
struct Usuario {
    char nombre[50];
    char contrasena[50];
};

/// @brief 
/// @param arg 
/// @return 
void *Transferencia(void *arg) {
    Config config = leer_configuracion("variables.properties");
    struct Usuario *usuario = (struct Usuario *)arg;

    int id_destino, Cantidad_transferir;

    system("clear");
    printf("\n-------------------------------------------------\n");
    printf("      💸 ¡Bienvenido a Transferencias! 💸\n");
    printf("-------------------------------------------------\n");

    printf("Ingrese el ID de la cuenta destino: ");
    scanf("%d", &id_destino);
    printf("Ingrese la cantidad a transferir: ");
    scanf("%d", &Cantidad_transferir);

    sem_wait(sem_usuarios);
    sem_wait(sem_transacciones);

    printf("[DEBUG] Entrando a Transferencia\n");

    int id_origen = -1;

    // Buscar el ID del usuario que está logueado
    for (int i = 0; i < MAX_CUENTAS; i++) {
        if (strcmp(cuentas[i].Nombre, usuario->nombre) == 0 &&
            strcmp(cuentas[i].Contraseña, usuario->contrasena) == 0) {
            id_origen = cuentas[i].id;
            break;
        }
    }

    if (id_origen == -1) {
        printf("Error: Usuario no encontrado.\n");
        Escribir_registro("Error al obtener ID del usuario.");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        sleep(2);
        return NULL;
    }

    if (id_origen == id_destino || id_destino < 0 || id_destino >= MAX_CUENTAS) {
        printf("Error: ID destino no válido.\n");
        Escribir_registro("ID destino inválido.");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        sleep(2);
        return NULL;
    }

    // Buscar punteros a las cuentas de origen y destino por ID
    Cuenta *cuenta_origen = NULL;
    Cuenta *cuenta_destino = NULL;

    for (int i = 0; i < MAX_CUENTAS; i++) {
        if (cuentas[i].id == id_origen) {
            cuenta_origen = &cuentas[i];
        }
        if (cuentas[i].id == id_destino) {
            cuenta_destino = &cuentas[i];
        }
    }

    if (!cuenta_origen || !cuenta_destino) {
        printf("Error: No se encontró la cuenta de origen o destino.\n");
        Escribir_registro("Error buscando cuentas por ID.");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        sleep(2);
        return NULL;
    }

    if (Cantidad_transferir > config.limite_transferencia) {
        printf("La cantidad excede el límite permitido.\n");
        Escribir_registro("Cantidad mayor al límite.");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        sleep(2);
        return NULL;
    }

    if (cuenta_origen->saldo < Cantidad_transferir) {
        printf("Saldo insuficiente.\n");
        Escribir_registro("Saldo insuficiente.");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        sleep(2);
        return NULL;
    }

    int saldo_origen_antes = cuenta_origen->saldo;
    int saldo_destino_antes = cuenta_destino->saldo;

    cuenta_origen->saldo -= Cantidad_transferir;
    cuenta_destino->saldo += Cantidad_transferir;

    cuenta_origen->Numero_transacciones++;
    cuenta_destino->Numero_transacciones++;

    // Registrar en archivo
    FILE *archivoTransacciones = fopen("transaciones.txt", "a+");
    if (!archivoTransacciones) {
        perror("Error abriendo transacciones.txt");
        Escribir_registro("Error al abrir transacciones.txt");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    // Obtener último ID
    int ultimo_id = 0;
    char linea[256];
    while (fgets(linea, sizeof(linea), archivoTransacciones)) {
        int temp_id;
        if (sscanf(linea, "%d |", &temp_id) == 1) {
            if (temp_id > ultimo_id) {
                ultimo_id = temp_id;
            }
        }
    }

    int nuevo_id = ultimo_id + 1;
    fprintf(archivoTransacciones, "%d | transferencia | %d | %d | %d | %d | %d | %d\n",
            nuevo_id, id_origen, id_destino,
            saldo_origen_antes, cuenta_origen->saldo,
            saldo_destino_antes, cuenta_destino->saldo);

    fclose(archivoTransacciones);
    Escribir_registro("Transferencia registrada en transacciones.txt");

    printf("✅ Transferencia realizada con éxito.\n");

    sem_post(sem_usuarios);
    sem_post(sem_transacciones);

    sleep(2);
    return NULL;
}
