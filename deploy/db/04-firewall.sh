#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../deploy.conf"
# Настройка UFW на сервере БД.

echo "=== Installing UFW ==="

apt update
apt install -y ufw

echo "=== Configuring firewall ==="

ufw --force reset

ufw default deny incoming
ufw default allow outgoing

# SSH
ufw allow 22/tcp

# PostgreSQL — только с app-сервера
ufw allow from "${APP_IP}" to any port 5432 proto tcp

echo "=== Enabling firewall ==="

ufw --force enable

echo
echo "=== Firewall status ==="

ufw status verbose
