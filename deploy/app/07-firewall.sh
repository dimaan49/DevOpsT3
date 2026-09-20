#!/bin/bash
set -euo pipefail

# Настройка UFW на сервере приложения.

echo "=== Installing UFW ==="

apt update
apt install -y ufw

echo "=== Configuring firewall ==="

ufw --force reset

ufw default deny incoming
ufw default allow outgoing

# SSH
ufw allow 22/tcp

# AuctionHub HTTP API
ufw allow 8080/tcp

echo "=== Enabling firewall ==="

ufw --force enable

echo
echo "=== Firewall status ==="

ufw status verbose
