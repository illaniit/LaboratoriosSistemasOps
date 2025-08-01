
#include "Comun.h"

volatile sig_atomic_t corriendo = 1;

// Manejador de señal para terminar el programa de forma segura
void manejador_salida(int sig)
{
    corriendo = 0;
}

void *detectar_transacciones_sospechosas(void *arg);
void *detectar_transferencias_consecutivas(void *arg);
void *detectar_retiros_consecutivos(void *arg);
void *detectar_saldo_negativo(void *arg);

int main()
{
    signal(SIGINT, manejador_salida);
    signal(SIGTERM, manejador_salida);

    Inicializar_semaforos();
    pthread_t h1, h2, h3, h4;

    // Bucle principal que se ejecuta continuamente mientras corriendo sea true
    while (corriendo)
    {
        // Se lanzan los hilos que detectan distintos tipos de comportamiento sospechoso
        pthread_create(&h1, NULL, detectar_transferencias_consecutivas, NULL);
        pthread_create(&h2, NULL, detectar_retiros_consecutivos, NULL);
        pthread_create(&h3, NULL, detectar_saldo_negativo, NULL);
        pthread_create(&h4, NULL, detectar_transacciones_sospechosas, NULL);

        pthread_join(h1, NULL);
        pthread_join(h2, NULL);
        pthread_join(h3, NULL);
        pthread_join(h4, NULL);

        sleep(60); // Espera de un minuto antes de volver a lanzar los hilos
    }
    Destruir_semaforos(); // Libera los semáforos al terminar
    return 0;
}

void enviar_alerta(const char *mensaje, const int *id, const int titular)
{
    MensajeAlerta msg;
    msg.mtype = TIPO_ALERTA;

    // Obtener la hora actual para registrar cuándo se detectó la alerta
    time_t t;
    struct tm *tm_info;
    char hora[30];
    time(&t);
    tm_info = localtime(&t);
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", tm_info);

    snprintf(msg.texto, sizeof(msg.texto),
             "[%s] Id de la transacci\xC3\xB3n: %d | %s\n",
             hora, *id, mensaje);

    // Enviar mensaje a la cola
    int id_cola = msgget(CLAVE_COLA1, 0666);
    if (id_cola == -1)
    {
        perror("Error al obtener cola");
        return;
    }

    if (msgsnd(id_cola, &msg, sizeof(msg.texto), 0) == -1)
    {
        perror("Error al enviar mensaje a la cola");
    }

    // Notificar al proceso padre con una señal
    kill(getppid(), SIGUSR1);
    Escribir_registro("Se ha enviado una alerta desde monitor.c");
}

void *detectar_transferencias_consecutivas(void *arg)
{
    Config config = leer_configuracion("variables.properties");
    Escribir_registro("Detectando transferencias consecutivas... en monitor.c");

    
    FILE *archivo = fopen(config.archivo_log, "r");
    if (!archivo)
    {
        perror("Error al abrir transaciones.txt");
        sleep(5);
    }

    char linea[256];
    int transferencias_consecutivas = 0, ultima_cuenta = -1;

    while (fgets(linea, sizeof(linea), archivo))
    {
        int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final1, saldo_final2;
        char tipo[20];

        if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final1, &saldo_final2) == 8)
        {
            limpiar_cadena(tipo);
            if (strcmp(tipo, "transferencia") == 0)
            {
                if (cuenta1 == ultima_cuenta)
                    transferencias_consecutivas++;
                else
                {
                    transferencias_consecutivas = 1;
                    ultima_cuenta = cuenta1;
                }

                // Si se detectan más de 4 transferencias seguidas de la misma cuenta
                if (transferencias_consecutivas > config.umbral_transferencias)
                {
                    Escribir_registro("Transferencias consecutivas detectadas desde monitor.c");
                    enviar_alerta("M\xC3\xBAltiples transferencias consecutivas", &id, 0);
                }
            }
        }
    }
   
    fclose(archivo);
    sleep(10);
    pthread_exit(NULL);
}

void *detectar_retiros_consecutivos(void *arg)
{
    Config config = leer_configuracion("variables.properties");
    Escribir_registro("Detectando retiros consecutivos... en monitor.c");
   
    FILE *archivo = fopen(config.archivo_log, "r");
    if (!archivo)
    {
        perror("Error al abrir transaciones.txt");
        sleep(5);
    }

    char linea[256];
    int retiros = 0, ultima_cuenta = -1;

    while (fgets(linea, sizeof(linea), archivo))
    {
        int id, cuenta1, saldo1, saldo_final;
        char tipo[20];

        if (sscanf(linea, "%d | %19[^|] | %d | - | %d | - | %d",
                   &id, tipo, &cuenta1, &saldo1, &saldo_final) == 5)
        {
            limpiar_cadena(tipo);
            if (strcmp(tipo, "retiro") == 0)
            {
                if (cuenta1 == ultima_cuenta)
                    retiros++;
                else
                {
                    retiros = 1;
                    ultima_cuenta = cuenta1;
                }

                // Si se detectan más de 3 retiros consecutivos
                if (retiros > 3)
                {
                    Escribir_registro("Retiros consecutivos detectados desde monitor.c");
                    enviar_alerta("M\xC3\xBAltiples retiros consecutivos", &id, 0);
                }
            }
        }
    }

    fclose(archivo);
    sleep(10);

    pthread_exit(NULL);
}

void *detectar_saldo_negativo(void *arg)
{
    Config config = leer_configuracion("variables.properties");
    Escribir_registro("Detectando saldos negativos... en monitor.c");
    
    FILE *archivo = fopen(config.archivo_log, "r");
    if (!archivo)
    {
        perror("Error al abrir transaciones.txt");
        sleep(5);
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo))
    {
        int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final1, saldo_final2,saldo_final;
        char tipo[20];

        // Leer línea y detectar saldos negativos
        if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final1, &saldo_final2) == 8)
        {
            if (saldo1 < 0 || saldo2 < 0 || saldo_final1 < 0 || saldo_final2 < 0)
            {
                Escribir_registro("Saldo negativo detectado desde monitor.c");
                enviar_alerta("Saldo negativo", &id, 0);
            }
        }
        if (sscanf(linea, "%d | %19[^|] | %d | - | %d | - | %d",
                   &id, tipo, &cuenta1, &saldo1, &saldo_final) == 5)
        {
            if (saldo1 < 0 || saldo_final < 0)
            {
                Escribir_registro("Saldo negativo detectado desde monitor.c");
                enviar_alerta("Saldo negativo", &id, 0);
            }           
        }
    }
  
    fclose(archivo);
    sleep(10);

    pthread_exit(NULL);
}

void *detectar_transacciones_sospechosas(void *arg)
{
    Config config = leer_configuracion("variables.properties");

    Escribir_registro("Detectando transacciones sospechosas... en monitor.c");
    
    FILE *archivo = fopen(config.archivo_log, "r");
    if (!archivo)
    {
        perror("Error al abrir transaciones.txt");
        sleep(5);
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo))
    {
        int id, cuenta1, cuenta2, saldo1, saldo2, saldo_final1, saldo_final2;
        char tipo[20];

        if (sscanf(linea, "%d | %19[^|] | %d | %d | %d | %d | %d | %d",
                   &id, tipo, &cuenta1, &cuenta2, &saldo1, &saldo2, &saldo_final1, &saldo_final2) == 8)
        {
            if (saldo_final1 < 0)
            {
                Escribir_registro("Transacci\xC3\xB3n sospechosa detectada desde monitor.c");
                enviar_alerta("Transacci\xC3\xB3n sospechosa", &id, 0);
            }
        }
    }
  
    fclose(archivo);
    sleep(10);

    pthread_exit(NULL);
}
