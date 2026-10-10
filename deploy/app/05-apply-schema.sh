MIGRATIONS_DIR="${APP_DIR}/migrations"

if [ ! -d "${MIGRATIONS_DIR}" ]; then
    echo "ERROR: ${MIGRATIONS_DIR} not found"
    exit 1
fi

export PGPASSWORD="${AUCTIONHUB_DB_PASSWORD}"

# Создать таблицу schema_migrations
psql -h "${AUCTIONHUB_DB_HOST}" -p "${AUCTIONHUB_DB_PORT}" \
     -U "${AUCTIONHUB_DB_USER}" -d "${AUCTIONHUB_DB_NAME}" \
     -v ON_ERROR_STOP=1 -q <<'EOF'
CREATE TABLE IF NOT EXISTS schema_migrations (
    version     TEXT PRIMARY KEY,
    applied_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
EOF

for file in $(ls "${MIGRATIONS_DIR}"/*.sql | sort); do
    version=$(basename "${file}" .sql)
    already=$(psql -h "${AUCTIONHUB_DB_HOST}" -p "${AUCTIONHUB_DB_PORT}" \
                   -U "${AUCTIONHUB_DB_USER}" -d "${AUCTIONHUB_DB_NAME}" \
                   -tAc "SELECT 1 FROM schema_migrations WHERE version = '${version}'")

    if [ "${already}" = "1" ]; then
        echo "  [skip] ${version}"
        continue
    fi

    echo "  [apply] ${version}"
    psql -h "${AUCTIONHUB_DB_HOST}" -p "${AUCTIONHUB_DB_PORT}" \
         -U "${AUCTIONHUB_DB_USER}" -d "${AUCTIONHUB_DB_NAME}" \
         -v ON_ERROR_STOP=1 -f "${file}"

    psql -h "${AUCTIONHUB_DB_HOST}" -p "${AUCTIONHUB_DB_PORT}" \
         -U "${AUCTIONHUB_DB_USER}" -d "${AUCTIONHUB_DB_NAME}" \
         -c "INSERT INTO schema_migrations (version) VALUES ('${version}')"
done
