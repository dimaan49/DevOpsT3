#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../deploy.conf"
# Настройка SSH:
#   - создание администратора admin (в группе sudo)
#   - копирование SSH-ключа root в admin
#   - запрет root-логина
#   - запрет входа по паролю
#   - вход только по ключам

ADMIN_HOME="/home/${ADMIN_USER}"

echo "=== Creating admin user ==="

if ! id "${ADMIN_USER}" >/dev/null 2>&1; then
    useradd \
        --create-home \
        --home-dir "${ADMIN_HOME}" \
        --shell /bin/bash \
        "${ADMIN_USER}"

    usermod -aG sudo "${ADMIN_USER}"

    echo "User ${ADMIN_USER} created and added to sudo group."
else
    echo "User ${ADMIN_USER} already exists."
fi

echo "=== Copying SSH keys from root to admin ==="

if [ -f /root/.ssh/authorized_keys ]; then
    mkdir -p "${ADMIN_HOME}/.ssh"
    cp /root/.ssh/authorized_keys "${ADMIN_HOME}/.ssh/authorized_keys"
    chown -R "${ADMIN_USER}:${ADMIN_USER}" "${ADMIN_HOME}/.ssh"
    chmod 700 "${ADMIN_HOME}/.ssh"
    chmod 600 "${ADMIN_HOME}/.ssh/authorized_keys"
    echo "SSH keys copied."
else
    echo "WARNING: /root/.ssh/authorized_keys not found."
    echo "Add your public key to ${ADMIN_HOME}/.ssh/authorized_keys manually."
fi

echo "=== Hardening SSH configuration ==="

SSHD_CONFIG="/etc/ssh/sshd_config"

if [ ! -f "${SSHD_CONFIG}" ]; then
    echo "sshd_config not found: ${SSHD_CONFIG}"
    exit 1
fi

cp "${SSHD_CONFIG}" "${SSHD_CONFIG}.bak"

# Отключаем root-логин
if grep -qE '^\s*PermitRootLogin' "${SSHD_CONFIG}"; then
    sed -i 's/^\s*PermitRootLogin.*/PermitRootLogin no/' "${SSHD_CONFIG}"
else
    echo "PermitRootLogin no" >> "${SSHD_CONFIG}"
fi

# Отключаем вход по паролю
if grep -qE '^\s*PasswordAuthentication' "${SSHD_CONFIG}"; then
    sed -i 's/^\s*PasswordAuthentication.*/PasswordAuthentication no/' "${SSHD_CONFIG}"
else
    echo "PasswordAuthentication no" >> "${SSHD_CONFIG}"
fi

# Включаем вход по ключам
if grep -qE '^\s*PubkeyAuthentication' "${SSHD_CONFIG}"; then
    sed -i 's/^\s*PubkeyAuthentication.*/PubkeyAuthentication yes/' "${SSHD_CONFIG}"
else
    echo "PubkeyAuthentication yes" >> "${SSHD_CONFIG}"
fi

echo "=== Restarting SSH ==="

systemctl restart ssh

echo
echo "=== SSH setup completed ==="
echo
echo "IMPORTANT:"
echo "1. Make sure you have copied your public key to ${ADMIN_HOME}/.ssh/authorized_keys"
echo "2. Test login as ${ADMIN_USER} in a NEW terminal before closing current session."
echo "3. If login fails, restore config from ${SSHD_CONFIG}.bak"
