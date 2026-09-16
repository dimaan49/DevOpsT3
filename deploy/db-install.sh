#!/bin/bash
set -euo pipefail

DB_NAME="auctionhub"
DB_USER="auction_app"
APP_IP="172.20.10.3"
DB_IP="172.20.10.4"

echo "=== Installing PostgreSQL ==="

apt update

apt install -y \
    postgresql \
    postgresql-contrib

systemctl enable postgresql
systemctl start postgresql

echo "=== Creating database user ==="

read -r -s -p "Enter PostgreSQL password for ${DB_USER}: " DB_PASSWORD
echo

if [ -z "${DB_PASSWORD}" ]; then
    echo "Password cannot be empty"
    exit 1
fi

echo "=== Creating database ==="

psql -h srvbd -U auction_app -d auctionhub \
    -f /opt/auctionhub/src/db/schema.sql

echo "=== Configuring PostgreSQL ==="

PG_VERSION=$(sudo -u postgres psql -tAc "SHOW server_version" | cut -d. -f1)

PG_CONFIG_DIR="/etc/postgresql/${PG_VERSION}/main"

if [ ! -d "${PG_CONFIG_DIR}" ]; then
    echo "PostgreSQL configuration directory not found:"
    echo "${PG_CONFIG_DIR}"
    exit 1
fi

sed -i \
    "s/^#listen_addresses =.*/listen_addresses = '${DB_IP}'/" \
    "${PG_CONFIG_DIR}/postgresql.conf"

if grep -q "^listen_addresses" "${PG_CONFIG_DIR}/postgresql.conf"; then
    sed -i \
        "s/^listen_addresses.*/listen_addresses = '${DB_IP}'/" \
        "${PG_CONFIG_DIR}/postgresql.conf"
fi

HBA_FILE="${PG_CONFIG_DIR}/pg_hba.conf"

if ! grep -q "${DB_NAME}.*${DB_USER}.*${APP_IP}/24" "${HBA_FILE}"; then
    cat >> "${HBA_FILE}" <<EOF

# AuctionHub application server
host    ${DB_NAME}    ${DB_USER}    ${APP_IP}/24    scram-sha-256
EOF
fi

echo "=== Restarting PostgreSQL ==="

systemctl restart postgresql

echo
echo "=== PostgreSQL installation completed ==="
echo
echo "Database: ${DB_NAME}"
echo "User:     ${DB_USER}"
echo "Allowed application IP: ${APP_IP}"
echo "PostgreSQL address: ${DB_IP}"
