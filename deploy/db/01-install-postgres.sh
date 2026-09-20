#!/bin/bash
set -euo pipefail

# Установка PostgreSQL на сервере БД.

echo "=== Installing PostgreSQL ==="

apt update
apt install -y \
    postgresql \
    postgresql-contrib

echo "=== Enabling and starting PostgreSQL ==="

systemctl enable postgresql
systemctl start postgresql

echo "=== PostgreSQL version ==="

sudo -u postgres psql -tAc "SELECT version();"

echo
echo "=== PostgreSQL installed ==="
