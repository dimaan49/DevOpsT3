#!/bin/bash
set -euo pipefail

# ============================================================
# Резервное копирование БД AuctionHub.
#
# Использование:
#   bash scripts/backup.sh [db_name] [output_file]
#
# Примеры:
#   bash scripts/backup.sh
#   bash scripts/backup.sh auctionhub
#   bash scripts/backup.sh auctionhub /tmp/my_backup.sql
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DB_NAME="${1:-auctionhub}"
BACKUPS_DIR="${PROJECT_ROOT}/backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_FILE="${2:-${BACKUPS_DIR}/${DB_NAME}_${TIMESTAMP}.sql}"

DB_HOST="${AUCTIONHUB_DB_HOST:-localhost}"
DB_PORT="${AUCTIONHUB_DB_PORT:-5432}"
DB_USER="${AUCTIONHUB_DB_USER:-dbuser}"

if [ -z "${AUCTIONHUB_DB_PASSWORD:-}" ]; then
    echo "ERROR: AUCTIONHUB_DB_PASSWORD is not set"
    exit 1
fi

export PGPASSWORD="${AUCTIONHUB_DB_PASSWORD}"

mkdir -p "$(dirname "${OUTPUT_FILE}")"

echo "=== Backing up ${DB_NAME} to ${OUTPUT_FILE} ==="

pg_dump \
    -h "${DB_HOST}" \
    -p "${DB_PORT}" \
    -U "${DB_USER}" \
    -d "${DB_NAME}" \
    --no-owner \
    --no-privileges \
    --clean \
    --if-exists \
    -f "${OUTPUT_FILE}"

SIZE=$(du -h "${OUTPUT_FILE}" | cut -f1)


echo
echo "=== Backup completed ==="
echo "File: ${OUTPUT_FILE}"
echo "Size: ${SIZE}"
