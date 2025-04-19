#ifndef CUENTA_H
#define CUENTA_H

// Definición de la estructura Cuenta
typedef struct Cuenta {
    int id;                         // ID único del usuario
    char Nombre[50];                // Nombre del usuario
    char Apellido[50];              // Apellido del usuario
    char Contraseña[50];            // Contraseña del usuario
    char RepetirContraseña[50];     // Contraseña para verificar la contraseña ingresada
    char domicilio[100];            // Domicilio del usuario
    char pais[50];                  // País de residencia del usuario
    int saldo;                      // Saldo en la cuenta
    int Numero_transacciones;       // Número de transacciones realizadas
} Cuenta;

 extern Cuenta *cuentas;
#endif // CUENTA_H
