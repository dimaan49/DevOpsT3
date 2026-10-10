#!/bin/bash
set -euo pipefail

# ============================================================
# Мигратор AuctionHub.
#
# Использование:
#   bash scripts/migrate.sh <db_name>
#
# Пример:
#   bash scripts/migrate.sh auctionhub
#   bash scripts/migrate.sh auctionhub_test
#
# Параметры подключения берутся из .env или из окружения:
#   AUCTIONHUB_DB_HOST
#   AUCTIONHUB_DB_PORT
#   AUCTIONHUB_DB_USER
#   AUCTIONHUB_DB_PASSWORD
# ============================================================

DB_NAME="${1:-}"

if [ -z "${DB_NAME}" ]; then
    echo "Usage: $0 <db_name>"
    echo "Example: $0 auctionhub"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
MIGRATIONS_DIR="${PROJECT_ROOT}/migrations"

DB_HOST="${AUCTIONHUB_DB_HOST:-localhost}"
DB_PORT="${AUCTIONHUB_DB_PORT:-5432}"
DB_USER="${AUCTIONHUB_DB_USER:-dbuser}"
DB_PASSWORD="${AUCTIONHUB_DB_PASSWORD:-}"

if [ -z "${DB_PASSWORD}" ]; then
    echo "ERROR: AUCTIONHUB_DB_PASSWORD is not set"
    exit 1
fi

if [ ! -d "${MIGRATIONS_DIR}" ]; then
    echo "ERROR: migrations directory not found: ${MIGRATIONS_DIR}"
    exit 1
fi

export PGPASSWORD="${DB_PASSWORD}"

PSQL="psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME} -v ON_ERROR_STOP=1 -q"

echo "=== Applying migrations to ${DB_NAME}@${DB_HOST}:${DB_PORT} ==="

# Создать таблицу schema_migrations, если её нет
${PSQL} <<'EOF'
CREATE TABLE IF NOT EXISTS schema_migrations (
    version     TEXT PRIMARY KEY,
    applied_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
EOF

APPLIED=0
SKIPPED=0

for file in $(ls "${MIGRATIONS_DIR}"/*.sql | sort); do
    version=$(basename "${file}" .sql)

    already=$(${PSQL} -tAc "SELECT 1 FROM schema_migrations WHERE version = '${version}'")

    if [ "${already}" = "1" ]; then
        echo "  [skip] ${version}"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    echo "  [apply] ${version}"
    ${PSQL} -f "${file}"

    ${PSQL} -c "INSERT INTO schema_migrations (version) VALUES ('${version}')"
    APPLIED=$((APPLIED + 1))
done

echo
echo "=== Migrations complete: ${APPLIED} applied, ${SKIPPED} skipped ==="
