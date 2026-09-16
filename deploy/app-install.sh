#!/bin/bash
set -euo pipefail

APP_NAME="auctionhub"
APP_USER="appuser"
APP_DIR="/opt/auctionhub"
REPO_URL="https://github.com/dimaan49/DevOpsT3.git"
CONFIG_FILE="/opt/auctionhub/.env"

echo "=== Installing dependencies ==="

apt update

apt install -y \
    build-essential \
    gcc \
    g++ \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libpq-dev \
    qt6-base-dev \
    qt6-httpserver-dev \
    postgresql-client \
    qt6-websockets-dev \
    libqt6sql6-psql

echo "=== Creating application user ==="

if ! id "${APP_USER}" >/dev/null 2>&1; then
    useradd \
        --system \
        --create-home \
        --home-dir "${APP_DIR}" \
        --shell /usr/sbin/nologin \
        "${APP_USER}"
fi

echo "=== Preparing application directory ==="

mkdir -p "${APP_DIR}"
chown "${APP_USER}:${APP_USER}" "${APP_DIR}"

echo "=== Downloading AuctionHub ==="

if [ ! -d "${APP_DIR}/.git" ]; then
    rm -rf "${APP_DIR}"
    mkdir -p "${APP_DIR}"

    git clone "${REPO_URL}" "${APP_DIR}"
fi

chown -R "${APP_USER}:${APP_USER}" "${APP_DIR}"

echo "=== Building AuctionHub ==="

sudo -u "${APP_USER}" bash -c "
    cd '${APP_DIR}'
    make setup
    make build
"

echo "=== Creating configuration file ==="

if [ ! -f "${CONFIG_FILE}" ]; then
    cat > "${CONFIG_FILE}" <<EOF
AUCTIONHUB_DB_HOST=srvdb
AUCTIONHUB_DB_PORT=5432
AUCTIONHUB_DB_NAME=
AUCTIONHUB_DB_USER=
AUCTIONHUB_DB_PASSWORD=
AUCTIONHUB_HOST=0.0.0.0
AUCTIONHUB_PORT=8080
AUCTIONHUB_LOG_LEVEL=info
AUCTIONHUB_JWT_SECRET=
EOF
fi

chown root:"${APP_USER}" "${CONFIG_FILE}"
chmod 640 "${CONFIG_FILE}"

echo "=== Installing systemd service ==="

cp "${APP_DIR}/deploy/auctionhub.service" \
   /etc/systemd/system/auctionhub.service

chmod 644 /etc/systemd/system/auctionhub.service

systemctl daemon-reload

systemctl enable "${APP_NAME}"

echo
echo "=== Installation completed ==="
echo
echo "IMPORTANT:"
echo "Edit ${CONFIG_FILE} and set the real DB_PASSWORD, DB_NAME and DB_USER."
echo
echo "Then run:"
echo "  systemctl start ${APP_NAME}"
echo "  systemctl status ${APP_NAME}"
