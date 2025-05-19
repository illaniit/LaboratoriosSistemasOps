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

# Colores
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
CYAN='\033[1;36m'
NC='\033[0m' # Sin color

# Rutas derivadas
RUTA_DEST="/home/$NUEVO_USUARIO/Bank"
DESKTOP_DIR="/home/$NUEVO_USUARIO/Desktop"
LAUNCHER="$DESKTOP_DIR/$NOMBRE_PROGRAMA.desktop"
SCRIPT_COMPLETO="$RUTA_DEST/$RUTA_SCRIPT"

echo -e "${CYAN}"
echo "==============================================="
echo "   Script de Instalación del Sistema Bancario   "
echo "==============================================="
echo -e "${NC}"

# 1. Crear nuevo usuario
echo -e "${YELLOW}>> [1/5] Creando usuario '$NUEVO_USUARIO'...${NC}"
sudo useradd -m "$NUEVO_USUARIO"
echo "$NUEVO_USUARIO:$CLAVE" | sudo chpasswd
echo -e "${GREEN}Usuario creado correctamente.${NC}"

# 2. Copiar programa
echo -e "${YELLOW}>> [2/5] Copiando el programa a su carpeta...${NC}"
sudo cp -r "$RUTA_ORIG" "$RUTA_DEST"
sudo chown -R "$NUEVO_USUARIO:$NUEVO_USUARIO" "$RUTA_DEST"
sudo chmod -R u+rwX "$RUTA_DEST"
echo -e "${GREEN}Programa copiado a $RUTA_DEST.${NC}"

# 3. Crear acceso directo en escritorio
echo -e "${YELLOW}>> [3/5] Creando acceso directo en el escritorio...${NC}"
sudo mkdir -p "$DESKTOP_DIR"
sudo bash -c "cat > '$LAUNCHER'" <<EOF
[Desktop Entry]
Type=Application
Name=$NOMBRE_PROGRAMA
Exec=gnome-terminal -- bash -c 'cd /home/$NUEVO_USUARIO/Bank && ./Bancos ; exec bash'
Icon=utilities-terminal
Terminal=false
EOF

sudo chmod +x "$LAUNCHER"
sudo chown "$NUEVO_USUARIO:$NUEVO_USUARIO" "$LAUNCHER"
echo -e "${GREEN}Acceso directo creado en el escritorio.${NC}"

# 4. Agregar regla sudo para ejecutar el script sin contraseña
echo -e "${YELLOW}>> [4/5] Configurando sudoers para ejecutar el programa sin contraseña...${NC}"
SUDOERS_LINE="$NUEVO_USUARIO ALL=(ALL) NOPASSWD: $SCRIPT_COMPLETO"
echo "$SUDOERS_LINE" | sudo tee "/etc/sudoers.d/$NUEVO_USUARIO-programa" > /dev/null
sudo chmod 440 "/etc/sudoers.d/$NUEVO_USUARIO-programa"
echo -e "${GREEN}Permiso sudo configurado correctamente.${NC}"

# 5. Instrucciones finales
echo -e "${CYAN}"
echo "========================================================"
echo -e "${GREEN}>>> CONFIGURACIÓN COMPLETA <<<${NC}"
echo ""
echo -e "${YELLOW}Usuario: ${NC}$NUEVO_USUARIO"
echo -e "${YELLOW}Contraseña: ${NC}$CLAVE"
echo -e "${YELLOW}Programa copiado a: ${NC}$RUTA_DEST"
echo -e "${YELLOW}Acceso directo creado en: ${NC}$LAUNCHER"
echo ""
echo -e "${CYAN}>>> PASOS FINALES:${NC}"
echo ""
echo "1. Cierra tu sesión actual (haz log out)."
echo "2. En la pantalla de inicio de sesión, elige '$NUEVO_USUARIO'."
echo "3. Ingresa la contraseña."
echo "4. En el escritorio, haz doble clic en '$NOMBRE_PROGRAMA'."
echo "   Se abrirá GNOME Terminal y ejecutará el programa con permisos."
echo ""
echo "========================================================"
echo -e "${NC}"

