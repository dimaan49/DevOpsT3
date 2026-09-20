#!/bin/bash
set -euo pipefail

# Создание SQL-пользователя и БД.
#
# Пароль вводится интерактивно.

DB_NAME="auctionhub"
DB_USER="dbuser"

echo "=== Creating database user and database ==="

read -r -s -p "Enter PostgreSQL password for ${DB_USER}: " DB_PASSWORD
echo
read -r -s -p "Repeat password: " DB_PASSWORD_CONFIRM
echo

if [ -z "${DB_PASSWORD}" ]; then
    echo "Password cannot be empty"
    exit 1
fi

if [ "${DB_PASSWORD}" != "${DB_PASSWORD_CONFIRM}" ]; then
    echo "Passwords do not match"
    exit 1
fi

sudo -u postgres psql <<EOF
DO \$\$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = '${DB_USER}') THEN
        CREATE ROLE ${DB_USER} LOGIN PASSWORD '${DB_PASSWORD}';
    ELSE
        ALTER ROLE ${DB_USER} WITH PASSWORD '${DB_PASSWORD}';
    END IF;
END
\$\$;
EOF

sudo -u postgres psql -tAc \
    "SELECT 1 FROM pg_database WHERE datname = '${DB_NAME}'" \
    | grep -q 1 \
    || sudo -u postgres createdb -O "${DB_USER}" "${DB_NAME}"

echo
echo "=== User and database created ==="
echo "Database: ${DB_NAME}"
echo "User:     ${DB_USER}"
