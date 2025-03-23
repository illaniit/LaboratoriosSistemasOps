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

Config leer_configuracion(const char *ruta);

extern sem_t *sem_usuarios;
extern sem_t *sem_transacciones;
void Inicializar_semaforos();
void Destruir_semaforos();
void Escribir_registro(const char *mensaje_registro);
void limpiar_cadena(char *cadena);

#endif