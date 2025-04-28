#ifndef CUENTA_H
#define CUENTA_H

#define MAX_CUENTAS 100 // Número máximo de cuentas

// Definición de la estructura Cuenta
typedef struct Cuenta {
    int id;                         // ID único del usuario
    char Nombre[50];                // Nombre del usuario
    char Apellido[50];             // Apellido del usuario
    char Contraseña[50];           // Contraseña del usuario
    char RepetirContraseña[50];    // Contraseña para verificar la contraseña ingresada
    char domicilio[100];           // Domicilio del usuario
    char pais[50];                 // País de residencia del usuario
    int saldo;                     // Saldo en la cuenta
    char fecha[20];             // Fecha de creación de la cuenta
    char hora[20];              // Hora de creación de la cuenta
} Cuenta;

extern Cuenta *cuentas;

#include <stdbool.h>

typedef struct {
    Cuenta cuentas[10];
    bool acceso;
    int NumeroCuentas;
}Buffer;



// Punteros a memoria compartida



#endif // CUENTA_H
