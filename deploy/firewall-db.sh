#!/bin/bash
set -euo pipefail

APP_IP="172.20.10.3"

echo "=== Installing UFW ==="

apt update
apt install -y ufw

echo "=== Configuring firewall ==="

ufw --force reset

ufw default deny incoming
ufw default allow outgoing

# SSH
ufw allow 22/tcp

# PostgreSQL - ONLY from application server
ufw allow from "${APP_IP}" to any port 5432 proto tcp

echo "=== Enabling firewall ==="

ufw --force enable

echo
echo "=== Firewall status ==="

ufw status verbose
