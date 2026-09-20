#!/bin/bash
set -euo pipefail

# Создание .env из .env.example с реальными значениями.
# JWT-секрет генерируется автоматически.

APP_USER="appuser"
APP_DIR="/opt/auctionhub/app"
ENV_FILE="${APP_DIR}/.env"
ENV_EXAMPLE="${APP_DIR}/.env.example"

DB_HOST="192.168.56.102"
DB_PORT="5432"
DB_NAME="auctionhub"
DB_USER="dbuser"

echo "=== Creating .env ==="

if [ ! -f "${ENV_EXAMPLE}" ]; then
    echo "ERROR: ${ENV_EXAMPLE} not found"
    exit 1
fi

read -r -s -p "Enter password for ${DB_USER}@${DB_HOST}: " DB_PASSWORD
echo

if [ -z "${DB_PASSWORD}" ]; then
    echo "Password cannot be empty"
    exit 1
fi

JWT_SECRET=$(openssl rand -hex 32)

cat > "${ENV_FILE}" <<EOF
AUCTIONHUB_HOST=0.0.0.0
AUCTIONHUB_PORT=8080
AUCTIONHUB_LOG_LEVEL=info

AUCTIONHUB_DB_HOST=${DB_HOST}
AUCTIONHUB_DB_PORT=${DB_PORT}
AUCTIONHUB_DB_NAME=${DB_NAME}
AUCTIONHUB_DB_USER=${DB_USER}
AUCTIONHUB_DB_PASSWORD=${DB_PASSWORD}

AUCTIONHUB_JWT_SECRET=${JWT_SECRET}
EOF

chown root:"${APP_USER}" "${ENV_FILE}"
chmod 640 "${ENV_FILE}"

echo
echo "=== .env created ==="
echo "Path: ${ENV_FILE}"
echo "JWT secret generated."
