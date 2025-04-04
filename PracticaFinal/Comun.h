#ifndef COMUN_H
#define COMUN_H
/// @brief 
// defino la funcion del menu para poder usarlo desde init_cuenta.c
typedef struct Config
{
    int limite_retiro;
    int limite_transferencia;
    int umbral_retiros;
    int umbral_transferencias;
    int num_hilos;
    char archivo_cuentas[50];
    char archivo_log[50];
} Config;

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define CLAVE_COLA 1234  // Puedes usar ftok si prefieres algo dinámico

#define TIPO_ALERTA 1    // Tipo de mensaje usado por monitor

typedef struct {
    long mtype;
    char texto[256];
} MensajeAlerta;


Config leer_configuracion(const char *ruta);

extern sem_t *sem_usuarios;
extern sem_t *sem_transacciones;
extern sem_t *sem_registro;

void crear_cola_mensajes();
void Inicializar_semaforos();
void Destruir_semaforos();
void Escribir_registro(const char *mensaje_registro);
void limpiar_cadena(char *cadena);
int obtener_id_usuario(const char *nombre, const char *contraseña);
#endif