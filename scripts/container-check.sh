#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

echo "=== Container policy checks ==="

grep -q '^USER 10001:10001$' Dockerfile \
  || fail "Runtime image must use non-root UID/GID 10001"

grep -q '^\.env$' .dockerignore \
  || fail ".env must be excluded from the Docker build context"

grep -q '^\.env\.\*$' .dockerignore \
  || fail ".env.* files must be excluded from the Docker build context"

grep -q '^\*\.backup$' .dockerignore \
  || fail "Backup files must be excluded from the Docker build context"

if grep -Eiq '(^|[[:space:]])(FROM|image:).*:latest([[:space:]@]|$)' Dockerfile compose.yaml; then
  fail "Floating latest image tags are forbidden"
fi

if ! grep -Eq 'image: \$\{AUCTIONHUB_IMAGE:-[^}]+\}' compose.yaml \
   && ! grep -q 'image: auctionhub:1.0.0' compose.yaml; then
    fail "App image must have an explicit version tag or a variable with a fallback tag"
fi
if grep -Eiq '^[[:space:]]*(COPY|ADD)[[:space:]].*\.env' Dockerfile; then
  fail "Dockerfile must not copy .env files directly"
fi

grep -q 'healthcheck:' compose.yaml \
  || fail "Healthchecks are required"
[ "$(grep -c 'healthcheck:' compose.yaml)" -ge 2 ] \
  || fail "Both app and DB services need healthchecks"

grep -q 'http://127.0.0.1:8080/health' compose.yaml \
  || fail "App healthcheck must call the HTTP /health endpoint"

awk '
  /^  db:/ { in_db=1; next }
  /^  app:/ { in_db=0 }
  in_db && /^    ports:/ { found=1 }
  END { exit found ? 1 : 0 }
' compose.yaml || fail "Database port must not be published to the host"

grep -q 'internal: true' compose.yaml \
  || fail "Internal backend network is required"

grep -q 'postgres_data:/var/lib/postgresql/data' compose.yaml \
  || fail "Persistent database volume is required"

grep -q 'condition: service_healthy' compose.yaml \
  || fail "App startup must wait for the healthy database"

grep -q 'no-new-privileges:true' compose.yaml \
  || fail "no-new-privileges security option is required"

echo "Static policies: PASS"

echo "=== Compose configuration ==="
docker compose config --quiet
echo "Compose configuration: PASS"

echo "=== Container policy checks passed ==="
