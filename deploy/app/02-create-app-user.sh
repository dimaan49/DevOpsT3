#!/bin/bash
set -euo pipefail

# Создание системного пользователя appuser.
# Пользователь без sudo, без входа в систему.

APP_USER="appuser"
APP_DIR="/opt/auctionhub"

echo "=== Creating application user ==="

if ! id "${APP_USER}" >/dev/null 2>&1; then
    useradd \
        --system \
        --create-home \
        --home-dir "${APP_DIR}" \
        --shell /usr/sbin/nologin \
        "${APP_USER}"
    echo "User ${APP_USER} created."
else
    echo "User ${APP_USER} already exists."
fi

mkdir -p "${APP_DIR}"
chown "${APP_USER}:${APP_USER}" "${APP_DIR}"

echo
echo "=== Application user ready ==="
echo "User: ${APP_USER}"
echo "Home: ${APP_DIR}"
