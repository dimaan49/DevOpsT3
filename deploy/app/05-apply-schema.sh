#!/bin/bash
set -euo pipefail

# Применение схемы БД с app-сервера на db-сервер.

APP_DIR="/opt/auctionhub"
ENV_FILE="${APP_DIR}/.env"

if [ ! -f "${ENV_FILE}" ]; then
    echo "ERROR: ${ENV_FILE} not found"
    exit 1
fi

# shellcheck disable=SC1090
set -a
source "${ENV_FILE}"
set +a

SCHEMA_FILE="${APP_DIR}/src/db/schema.sql"

if [ ! -f "${SCHEMA_FILE}" ]; then
    echo "ERROR: ${SCHEMA_FILE} not found"
    exit 1
fi

echo "=== Applying database schema ==="
echo "Host: ${AUCTIONHUB_DB_HOST}"
echo "Database: ${AUCTIONHUB_DB_NAME}"
echo "User: ${AUCTIONHUB_DB_USER}"

PGPASSWORD="${AUCTIONHUB_DB_PASSWORD}" psql \
    -h "${AUCTIONHUB_DB_HOST}" \
    -p "${AUCTIONHUB_DB_PORT}" \
    -U "${AUCTIONHUB_DB_USER}" \
    -d "${AUCTIONHUB_DB_NAME}" \
    -f "${SCHEMA_FILE}"

echo
echo "=== Schema applied ==="
