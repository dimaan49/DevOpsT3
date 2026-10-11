#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DB_NAME="${AUCTIONHUB_BACKUP_TEST_DB_NAME:-auctionhub_backup_test}"
DB_HOST="${AUCTIONHUB_DB_HOST:-localhost}"
DB_PORT="${AUCTIONHUB_DB_PORT:-5432}"
DB_USER="${AUCTIONHUB_DB_USER:-dbuser}"
DB_PASSWORD="${AUCTIONHUB_DB_PASSWORD:-}"

# Разрешаем удалять только специально выделенные тестовые БД.
if [[ ! "$DB_NAME" =~ ^auctionhub_backup_(test|ci)$ ]]; then
    echo "ERROR: refusing to use database: $DB_NAME"
    exit 1
fi

if [[ -z "$DB_PASSWORD" ]]; then
    echo "ERROR: AUCTIONHUB_DB_PASSWORD is not set"
    exit 1
fi

export PGPASSWORD="$DB_PASSWORD"

BACKUP_FILE="$(mktemp "${TMPDIR:-/tmp}/auctionhub-backup-XXXXXX.sql")"

cleanup() {
    rm -f "$BACKUP_FILE"
    dropdb -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" \
        --if-exists "$DB_NAME" >/dev/null 2>&1 || true
}
trap cleanup EXIT

echo "=== Create isolated database: $DB_NAME ==="
dropdb -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" \
    --if-exists "$DB_NAME"
createdb -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" "$DB_NAME"

echo "=== Apply migrations ==="
bash "$SCRIPT_DIR/migrate.sh" "$DB_NAME"

PSQL=(psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" \
    -d "$DB_NAME" -v ON_ERROR_STOP=1)

echo "=== Insert test record ==="
"${PSQL[@]}" -q -c \
    "CREATE TABLE backup_restore_probe (
        id INTEGER PRIMARY KEY,
        marker TEXT NOT NULL
    )"

"${PSQL[@]}" -q -c \
    "INSERT INTO backup_restore_probe VALUES (1, 'backup-restore-ok')"

echo "=== Create backup ==="
bash "$SCRIPT_DIR/backup.sh" "$DB_NAME" "$BACKUP_FILE"

echo "=== Remove test record ==="
"${PSQL[@]}" -q -c "DELETE FROM backup_restore_probe"

echo "=== Restore backup ==="
bash "$SCRIPT_DIR/restore.sh" "$BACKUP_FILE" "$DB_NAME"

echo "=== Verify restored record ==="
MARKER="$("${PSQL[@]}" -tAc \
    "SELECT marker FROM backup_restore_probe WHERE id = 1" | xargs)"

if [[ "$MARKER" != "backup-restore-ok" ]]; then
    echo "FAIL: restored marker does not match"
    exit 1
fi

echo "PASS: backup/restore verified."
