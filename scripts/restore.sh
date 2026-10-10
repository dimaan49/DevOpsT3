#!/bin/bash
set -euo pipefail

# ============================================================
# Восстановление БД AuctionHub из резервной копии.
#
# Использование:
#   bash scripts/restore.sh <backup_file> [db_name]
#
# Примеры:
#   bash scripts/restore.sh backups/auctionhub_20261006_120000.sql
#   bash scripts/restore.sh backups/auctionhub_20261006_120000.sql auctionhub
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <backup_file> [db_name]"
    exit 1
fi

BACKUP_FILE="$1"
DB_NAME="${2:-auctionhub}"

if [ ! -f "${BACKUP_FILE}" ]; then
    echo "ERROR: backup file not found: ${BACKUP_FILE}"
    exit 1
fi

DB_HOST="${AUCTIONHUB_DB_HOST:-localhost}"
DB_PORT="${AUCTIONHUB_DB_PORT:-5432}"
DB_USER="${AUCTIONHUB_DB_USER:-dbuser}"

if [ -z "${AUCTIONHUB_DB_PASSWORD:-}" ]; then
    echo "ERROR: AUCTIONHUB_DB_PASSWORD is not set"
    exit 1
fi

export PGPASSWORD="${AUCTIONHUB_DB_PASSWORD}"

echo "=== Restoring ${DB_NAME} from ${BACKUP_FILE} ==="

psql \
    -h "${DB_HOST}" \
    -p "${DB_PORT}" \
    -U "${DB_USER}" \
    -d "${DB_NAME}" \
    -v ON_ERROR_STOP=1 \
    -q \
    -f "${BACKUP_FILE}"

echo
echo "=== Restore completed ==="
