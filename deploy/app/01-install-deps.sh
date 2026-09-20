#!/bin/bash
set -euo pipefail

# Установка зависимостей для сборки и запуска AuctionHub.

echo "=== Installing build dependencies ==="

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
    qt6-websockets-dev \
    libqt6sql6-psql \
    postgresql-client \
    openssl

echo
echo "=== Dependencies installed ==="
