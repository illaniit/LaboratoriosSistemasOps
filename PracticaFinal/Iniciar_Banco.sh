#!/bin/bash

#########################################
# Script para:
# - Crear usuario nuevo
# - Copiar programa
# - Crear acceso directo en escritorio
# - Permitir ejecutar el programa con sudo sin contraseña
#########################################

# === CONFIGURACIÓN: PERSONALIZA ESTO ===
NUEVO_USUARIO="SistemaBancario"
CLAVE="banco"
RUTA_ORIG="./"
RUTA_SCRIPT="Iniciar_Banco.sh"
NOMBRE_PROGRAMA="Banco"
# === FIN CONFIGURACIÓN ===

# Rutas derivadas
RUTA_DEST="/home/$NUEVO_USUARIO/tu_programa"
DESKTOP_DIR="/home/$NUEVO_USUARIO/Desktop"
LAUNCHER="$DESKTOP_DIR/$NOMBRE_PROGRAMA.desktop"
SCRIPT_COMPLETO="$RUTA_DEST/$RUTA_SCRIPT"

# 1. Crear nuevo usuario
echo ">> Creando usuario '$NUEVO_USUARIO'..."
sudo useradd -m "$NUEVO_USUARIO"
echo "$NUEVO_USUARIO:$CLAVE" | sudo chpasswd

# 2. Copiar programa
echo ">> Copiando el programa a su carpeta..."
sudo cp -r "$RUTA_ORIG" "$RUTA_DEST"
sudo chown -R "$NUEVO_USUARIO:$NUEVO_USUARIO" "$RUTA_DEST"
sudo chmod -R u+rwX "$RUTA_DEST"

# 3. Crear acceso directo en escritorio
echo ">> Creando acceso directo en el escritorio..."
sudo mkdir -p "$DESKTOP_DIR"
sudo bash -c "cat > '$LAUNCHER'" <<EOF
[Desktop Entry]
Type=Application
Name=$NOMBRE_PROGRAMA
Exec=gnome-terminal -- bash -c 'cd /home/$NUEVO_USUARIO/tu_programa && ./Bancos ; exec bash'
Icon=utilities-terminal
Terminal=false
EOF

sudo chmod +x "$LAUNCHER"
sudo chown "$NUEVO_USUARIO:$NUEVO_USUARIO" "$LAUNCHER"

# 4. Agregar regla sudo para ejecutar el script sin contraseña
echo ">> Configurando sudoers para permitir ejecutar el programa sin contraseña..."
SUDOERS_LINE="$NUEVO_USUARIO ALL=(ALL) NOPASSWD: $SCRIPT_COMPLETO"
echo "$SUDOERS_LINE" | sudo tee "/etc/sudoers.d/$NUEVO_USUARIO-programa" > /dev/null
sudo chmod 440 "/etc/sudoers.d/$NUEVO_USUARIO-programa"

# 5. Instrucciones finales
echo ""
echo "========================================================"
echo ">>> CONFIGURACIÓN COMPLETA <<<"
echo ""
echo "Usuario: $NUEVO_USUARIO"
echo "Contraseña: $CLAVE"
echo "Programa copiado a: $RUTA_DEST"
echo "Acceso directo creado en: $LAUNCHER"
echo ""
echo ">>> PASOS FINALES:"
echo ""
echo "1. Cierra tu sesión actual , haz log out"
echo ""
echo "2. En la pantalla de inicio de sesión, elige '$NUEVO_USUARIO'."
echo "3. Ingresa la contraseña."
echo "4. En el escritorio, haz doble clic en '$NOMBRE_PROGRAMA'."
echo "   Se abrirá GNOME Terminal y ejecutará el programa con permisos."
echo ""
echo "========================================================"

