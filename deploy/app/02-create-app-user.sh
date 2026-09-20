#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../deploy.conf"
# Создание системного пользователя appuser.
# Пользователь без sudo, без входа в систему.


echo "=== Creating application user ==="

if ! id "${APP_USER}" >/dev/null 2>&1; then
    useradd \
        --system \
        --create-home \
        --home-dir "${APP_HOME}" \
        --shell /usr/sbin/nologin \
        "${APP_USER}"
    echo "User ${APP_USER} created."
else
    echo "User ${APP_USER} already exists."
fi

mkdir -p "${APP_HOME}"
chown "${APP_USER}:${APP_USER}" "${APP_HOME}"

echo
echo "=== Application user ready ==="
echo "User: ${APP_USER}"
echo "Home: ${APP_HOME}"
