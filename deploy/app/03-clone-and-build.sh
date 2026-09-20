#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../deploy.conf"
# Клонирование и сборка приложения.

mkdir -p "${APP_DIR}"
chown "${APP_USER}:${APP_USER}" "${APP_DIR}"

echo "=== Cloning repository ==="

if [ ! -d "${APP_DIR}/.git" ]; then
    sudo -u "${APP_USER}" git clone \
        --branch "${REPO_BRANCH}" \
        "${REPO_URL}" "${APP_DIR}"
else
    echo "Repository already cloned."
    sudo -u "${APP_USER}" git -C "${APP_DIR}" fetch
    sudo -u "${APP_USER}" git -C "${APP_DIR}" checkout "${REPO_BRANCH}"
    sudo -u "${APP_USER}" git -C "${APP_DIR}" pull
fi

echo "=== Building application ==="

sudo -u "${APP_USER}" bash -c "
    cd '${APP_DIR}'
    make setup
    make build
"

if [ ! -x "${APP_DIR}/build/auctionhub_server" ]; then
    echo "ERROR: build/auctionhub_server not found"
    exit 1
fi

echo
echo "=== Build completed ==="
ls -la "${APP_DIR}/build/auctionhub_server"
