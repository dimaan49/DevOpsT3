#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../deploy.conf"
# Установка systemd-службы AuctionHub.

SERVICE_NAME="auctionhub"
SERVICE_SRC="${APP_DIR}/deploy/app/auctionhub.service"
SERVICE_DST="/etc/systemd/system/${SERVICE_NAME}.service"

if [ ! -f "${SERVICE_SRC}" ]; then
    echo "ERROR: ${SERVICE_SRC} not found"
    exit 1
fi

echo "=== Installing systemd service ==="

cp "${SERVICE_SRC}" "${SERVICE_DST}"
chmod 644 "${SERVICE_DST}"

systemctl daemon-reload
systemctl enable "${SERVICE_NAME}"
systemctl restart "${SERVICE_NAME}"

sleep 2

echo "=== Service status ==="
systemctl status "${SERVICE_NAME}" --no-pager || true
