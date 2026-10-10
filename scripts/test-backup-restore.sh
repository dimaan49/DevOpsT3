#!/bin/bash
set -euo pipefail

# ============================================================
# Проверка backup/restore:
#   - делает backup
#   - удаляет данные
#   - восстанавливает
#   - проверяет, что количество строк совпадает
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DB_NAME="auctionhub"

DB_HOST="${AUCTIONHUB_DB_HOST:-localhost}"
DB_PORT="${AUCTIONHUB_DB_PORT:-5432}"
DB_USER="${AUCTIONHUB_DB_USER:-dbuser}"

if [ -z "${AUCTIONHUB_DB_PASSWORD:-}" ]; then
    echo "ERROR: AUCTIONHUB_DB_PASSWORD is not set"
    exit 1
fi

export PGPASSWORD="${AUCTIONHUB_DB_PASSWORD}"

PSQL="psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME} -tAc"

count_rows() {
    local table="$1"
    ${PSQL} "SELECT COUNT(*) FROM ${table}"
}

echo "=== Step 1: baseline counts ==="

USERS_BEFORE=$(count_rows users)
AUCTIONS_BEFORE=$(count_rows auctions)
LOTS_BEFORE=$(count_rows lots)
BIDS_BEFORE=$(count_rows bids)
REVIEWS_BEFORE=$(count_rows reviews)

echo "users=${USERS_BEFORE} auctions=${AUCTIONS_BEFORE} lots=${LOTS_BEFORE} bids=${BIDS_BEFORE} reviews=${REVIEWS_BEFORE}"

echo
echo "=== Step 2: create backup ==="

BACKUP_FILE=$(find "${PROJECT_ROOT}/backups" -name "${DB_NAME}_*.sql" 2>/dev/null | sort | tail -1 || true)
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="${PROJECT_ROOT}/backups/${DB_NAME}_test_${TIMESTAMP}.sql"

mkdir -p "${PROJECT_ROOT}/backups"

bash "${SCRIPT_DIR}/backup.sh" "${DB_NAME}" "${BACKUP_FILE}"

echo
echo "=== Step 3: delete all data ==="

${PSQL} "DELETE FROM reviews"
${PSQL} "DELETE FROM bids"
${PSQL} "DELETE FROM lots"
${PSQL} "DELETE FROM auctions"
${PSQL} "DELETE FROM revoked_tokens"
${PSQL} "DELETE FROM users"

USERS_AFTER_DELETE=$(count_rows users)
echo "users after delete: ${USERS_AFTER_DELETE}"

if [ "${USERS_AFTER_DELETE}" != "0" ]; then
    echo "FAIL: expected 0 users after delete, got ${USERS_AFTER_DELETE}"
    exit 1
fi

echo
echo "=== Step 4: restore from backup ==="

bash "${SCRIPT_DIR}/restore.sh" "${BACKUP_FILE}" "${DB_NAME}"

echo
echo "=== Step 5: verify counts ==="

USERS_RESTORED=$(count_rows users)
AUCTIONS_RESTORED=$(count_rows auctions)
LOTS_RESTORED=$(count_rows lots)
BIDS_RESTORED=$(count_rows bids)
REVIEWS_RESTORED=$(count_rows reviews)

echo "users=${USERS_RESTORED} auctions=${AUCTIONS_RESTORED} lots=${LOTS_RESTORED} bids=${BIDS_RESTORED} reviews=${REVIEWS_RESTORED}"

if [ "${USERS_BEFORE}" != "${USERS_RESTORED}" ]; then
    echo "FAIL: users count mismatch: before=${USERS_BEFORE}, after=${USERS_RESTORED}"
    exit 1
fi

if [ "${AUCTIONS_BEFORE}" != "${AUCTIONS_RESTORED}" ]; then
    echo "FAIL: auctions count mismatch"
    exit 1
fi

if [ "${LOTS_BEFORE}" != "${LOTS_RESTORED}" ]; then
    echo "FAIL: lots count mismatch"
    exit 1
fi

if [ "${BIDS_BEFORE}" != "${BIDS_RESTORED}" ]; then
    echo "FAIL: bids count mismatch"
    exit 1
fi

if [ "${REVIEWS_BEFORE}" != "${REVIEWS_RESTORED}" ]; then
    echo "FAIL: reviews count mismatch"
    exit 1
fi

# Удалить временный дамп
rm -f "${BACKUP_FILE}"

echo
echo "=== Backup/restore test passed ==="
