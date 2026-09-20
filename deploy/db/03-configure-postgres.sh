#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../deploy.conf"
# Настройка PostgreSQL: доступ только с app-сервера.



echo "=== Detecting PostgreSQL config directory ==="

PG_VERSION=$(sudo -u postgres psql -tAc "SHOW server_version" | cut -d. -f1)
PG_CONFIG_DIR="/etc/postgresql/${PG_VERSION}/main"

if [ ! -d "${PG_CONFIG_DIR}" ]; then
    echo "Config directory not found: ${PG_CONFIG_DIR}"
    exit 1
fi

echo "Config directory: ${PG_CONFIG_DIR}"

echo "=== Configuring listen_addresses ==="

CONF_FILE="${PG_CONFIG_DIR}/postgresql.conf"

cp "${CONF_FILE}" "${CONF_FILE}.bak"

if grep -qE '^\s*listen_addresses' "${CONF_FILE}"; then
    sed -i "s/^\s*listen_addresses.*/listen_addresses = '${DB_IP}'/" "${CONF_FILE}"
else
    echo "listen_addresses = '${DB_IP}'" >> "${CONF_FILE}"
fi

echo "=== Configuring pg_hba.conf ==="

HBA_FILE="${PG_CONFIG_DIR}/pg_hba.conf"

cp "${HBA_FILE}" "${HBA_FILE}.bak"

# Удалить старые правила для AuctionHub, если есть
sed -i '/# AuctionHub application server/d' "${HBA_FILE}"
sed -i "/^host\s\+${DB_NAME}\s\+${DB_USER}\s/d" "${HBA_FILE}"

cat >> "${HBA_FILE}" <<EOF

# AuctionHub application server
host    ${DB_NAME}    ${DB_USER}    ${APP_IP}/32    scram-sha-256
EOF

echo "=== Restarting PostgreSQL ==="

systemctl restart postgresql

sleep 2

echo "=== Verifying ==="

systemctl is-active postgresql

echo
echo "=== PostgreSQL configured ==="
echo "Listen address: ${DB_IP}"
echo "Allowed connection: ${DB_USER}@${APP_IP} → ${DB_NAME}"
