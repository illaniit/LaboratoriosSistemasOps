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


#endif