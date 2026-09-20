#!/bin/bash
set -euo pipefail

# Настройка UFW на сервере БД.

APP_IP="192.168.56.101"

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
