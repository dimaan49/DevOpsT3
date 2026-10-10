#!/bin/bash
set -euo pipefail

# ============================================================
# Проверка миграций на чистой БД и на БД с данными.
# ============================================================

DB_NAME="auctionhub_migration_test"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DB_HOST="${AUCTIONHUB_DB_HOST:-localhost}"
DB_PORT="${AUCTIONHUB_DB_PORT:-5432}"
DB_USER="${AUCTIONHUB_DB_USER:-dbuser}"

if [ -z "${AUCTIONHUB_DB_PASSWORD:-}" ]; then
    echo "ERROR: AUCTIONHUB_DB_PASSWORD is not set"
    exit 1
fi

export PGPASSWORD="${AUCTIONHUB_DB_PASSWORD}"

PSQL="psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME}"

echo "=== Test 1: clean database ==="

dropdb -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" --if-exists "${DB_NAME}"
createdb -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" "${DB_NAME}"

AUCTIONHUB_DB_NAME="${DB_NAME}" \
    bash "${PROJECT_ROOT}/scripts/migrate.sh" "${DB_NAME}" > /dev/null

count=$(${PSQL} -tAc "SELECT COUNT(*) FROM schema_migrations")

if [ "${count}" -lt 4 ]; then
    echo "FAIL: expected at least 4 migrations, got ${count}"
    exit 1
fi

echo "OK: ${count} migrations applied"

echo
echo "=== Test 2: database with data ==="

${PSQL} -q -c "
INSERT INTO users (email, password_hash, role, age_confirmed)
VALUES ('migration-test@test.local', 'hash', 'seller', true);

INSERT INTO auctions (seller_id, title, description, step, start_price, status)
SELECT id, 'Test', '', 100, 1000, 'draft' FROM users WHERE email = 'migration-test@test.local';
"

users_before=$(${PSQL} -tAc "SELECT COUNT(*) FROM users")
auctions_before=$(${PSQL} -tAc "SELECT COUNT(*) FROM auctions")

echo "Before: users=${users_before}, auctions=${auctions_before}"

AUCTIONHUB_DB_NAME="${DB_NAME}" \
    bash "${PROJECT_ROOT}/scripts/migrate.sh" "${DB_NAME}" > /dev/null

users_after=$(${PSQL} -tAc "SELECT COUNT(*) FROM users")
auctions_after=$(${PSQL} -tAc "SELECT COUNT(*) FROM auctions")

echo "After: users=${users_after}, auctions=${auctions_after}"

if [ "${users_before}" != "${users_after}" ]; then
    echo "FAIL: users count changed"
    exit 1
fi

if [ "${auctions_before}" != "${auctions_after}" ]; then
    echo "FAIL: auctions count changed"
    exit 1
fi

dropdb -h "${DB_HOST}" -p "${DB_PORT}" -U "${DB_USER}" --if-exists "${DB_NAME}"

echo
echo "=== Migration tests passed ==="
