
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

    int id_origen = -1;
    for (int i = 0; i < config.max_cuentas; i++) {
        if (strcmp(cuentas[i].Nombre, usuario->nombre) == 0 &&
            strcmp(cuentas[i].Contraseña, usuario->contrasena) == 0) {
            id_origen = cuentas[i].id;
            break;
        }
    }

    if (id_origen == -1) {
        printf("❌ Error: Usuario no encontrado.\n");
        Escribir_registro("Error al obtener ID del usuario.");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        sleep(2);
        return NULL;
    }

    if (id_origen == id_destino || id_destino < 0) {
        printf("❌ Error: ID destino no válido.\n");
        Escribir_registro("ID destino inválido.");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        sleep(2);
        return NULL;
    }

    Cuenta *cuenta_origen = NULL;
    Cuenta *cuenta_destino = NULL;

    for (int i = 0; i < config.max_cuentas; i++) {
        if (cuentas[i].id == id_origen) {
            cuenta_origen = &cuentas[i];
        }
        if (cuentas[i].id == id_destino) {
            cuenta_destino = &cuentas[i];
        }
    }

    if (!cuenta_destino) {
        FILE *archivo = fopen("cuentas.txt", "r");
        if (!archivo) {
            perror("Error al abrir cuentas.txt");
            Escribir_registro("Error al abrir cuentas.txt");
            sem_post(sem_usuarios);
            sem_post(sem_transacciones);
            return NULL;
        }

        Cuenta cuenta_temp;
        char fecha[20], hora[20];
        int ocupadas = 0;
        int indice_libre = -1;

        typedef struct {
            Cuenta *cuenta;
            char fecha[20];
            char hora[20];
        } CuentaConFecha;

        CuentaConFecha cuentas_memoria[config.max_cuentas];

        for (int i = 0; i < config.max_cuentas; i++) {
            if (cuentas[i].id != 0) {
                strcpy(cuentas_memoria[ocupadas].fecha, cuentas[i].fecha);
                strcpy(cuentas_memoria[ocupadas].hora, cuentas[i].hora);
                cuentas_memoria[ocupadas].cuenta = &cuentas[i];
                ocupadas++;
            } else if (indice_libre == -1) {
                indice_libre = i;
            }
        }

        while (fscanf(archivo, "%d|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d|%[^|]|%s\n",
                      &cuenta_temp.id,
                      cuenta_temp.Nombre,
                      cuenta_temp.Apellido,
                      cuenta_temp.Contraseña,
                      cuenta_temp.domicilio,
                      cuenta_temp.pais,
                      &cuenta_temp.saldo,
                      fecha,
                      hora) == 9) {
            strcpy(cuenta_temp.fecha, fecha);
            strcpy(cuenta_temp.hora, hora);

            if (cuenta_temp.id == id_destino) {
                if (indice_libre != -1) {
                    cuentas[indice_libre] = cuenta_temp;
                    cuenta_destino = &cuentas[indice_libre];

                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg), "Se cargó cuenta destino ID %d en memoria compartida (slot libre).", cuenta_temp.id);
                    Escribir_registro(log_msg);
                } else {
                    CuentaConFecha *mas_antigua = &cuentas_memoria[0];
                    for (int i = 1; i < ocupadas; i++) {
                        int cmp_fecha = strcmp(cuentas_memoria[i].fecha, mas_antigua->fecha);
                        int cmp_hora = strcmp(cuentas_memoria[i].hora, mas_antigua->hora);
                        if (cmp_fecha < 0 || (cmp_fecha == 0 && cmp_hora < 0)) {
                            mas_antigua = &cuentas_memoria[i];
                        }
                    }

                    int id_antiguo = mas_antigua->cuenta->id;
                    *mas_antigua->cuenta = cuenta_temp;
                    cuenta_destino = mas_antigua->cuenta;

                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Se desalojó cuenta ID %d para cargar cuenta destino ID %d en memoria compartida.", id_antiguo, cuenta_temp.id);
                    Escribir_registro(log_msg);
                }
                break;
            }
        }

        fclose(archivo);

        if (!cuenta_destino) {
            printf("❌ Cuenta destino no encontrada.\n");
            Escribir_registro("Cuenta destino no encontrada.");
            sem_post(sem_usuarios);
            sem_post(sem_transacciones);
            sleep(2);
            return NULL;
        }
    }

    if (Cantidad_transferir > config.limite_transferencia) {
        printf("❌ La cantidad excede el límite permitido.\n");
        Escribir_registro("Cantidad mayor al límite.");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        sleep(2);
        return NULL;
    }

    if (cuenta_origen->saldo < Cantidad_transferir) {
        printf("❌ Saldo insuficiente.\n");
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

    FILE *archivoTransacciones = fopen("transaciones.txt", "a+");
    if (!archivoTransacciones) {
        perror("Error abriendo transacciones.txt");
        Escribir_registro("Error al abrir transacciones.txt");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

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
    char ruta_log_origen[128], ruta_log_destino[128];
    snprintf(ruta_log_origen, sizeof(ruta_log_origen), "./transacciones/%d/transacciones.log", id_origen);
    snprintf(ruta_log_destino, sizeof(ruta_log_destino), "./transacciones/%d/transacciones.log", id_destino);

    // Registrar en el log del usuario que envía
    FILE *archivoLogOrigen = fopen(ruta_log_origen, "a+");
    if (!archivoLogOrigen) {
        perror("Error abriendo transacciones.log del origen");
        Escribir_registro("Error al abrir transacciones.log del origen");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    fprintf(archivoLogOrigen, "%d | transferencia enviada | %d | %d | %d | %d | %d | %d\n",
            nuevo_id, id_origen, id_destino,
            saldo_origen_antes, cuenta_origen->saldo,
            saldo_destino_antes, cuenta_destino->saldo);

    fclose(archivoLogOrigen);

    // Registrar en el log del usuario que recibe
    FILE *archivoLogDestino = fopen(ruta_log_destino, "a+");
    if (!archivoLogDestino) {
        perror("Error abriendo transacciones.log del destino");
        Escribir_registro("Error al abrir transacciones.log del destino");
        sem_post(sem_usuarios);
        sem_post(sem_transacciones);
        return NULL;
    }

    fprintf(archivoLogDestino, "%d | transferencia recibida | %d | %d | %d | %d | %d | %d\n",
            nuevo_id, id_origen, id_destino,
            saldo_origen_antes, cuenta_origen->saldo,
            saldo_destino_antes, cuenta_destino->saldo);

    fclose(archivoLogDestino); Escribir_registro("Transferencia registrada en transacciones.txt");

    printf("✅ Transferencia realizada con éxito.\n");

    sem_post(sem_usuarios);
    sem_post(sem_transacciones);
    sleep(2);
    return NULL;
}
