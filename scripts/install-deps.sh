#!/bin/bash
set -euo pipefail

echo "=== Checking dependencies ==="

if command -v pacman >/dev/null 2>&1; then
    PKGS="base-devel cmake ninja gcc clang clang-tools-extra \
          qt6-base qt6-httpserver qt6-websockets \
          postgresql postgresql-libs gcovr git pkgconf"
    echo "Installing via pacman: ${PKGS}"
    sudo pacman -S --needed --noconfirm ${PKGS}
elif command -v apt >/dev/null 2>&1; then
    PKGS="build-essential cmake ninja-build gcc clang clang-tools-extra \
          libpq-dev qt6-base-dev qt6-httpserver-dev qt6-websockets-dev \
          libqt6sql6-psql postgresql-client gcovr git pkg-config"
    echo "Installing via apt: ${PKGS}"
    sudo apt update
    sudo apt install -y ${PKGS}
else
    echo "Unsupported package manager."
    echo "Install manually: cmake, ninja, gcc, clang, Qt6, PostgreSQL, gcovr"
    exit 1
fi

echo
echo "=== Dependencies installed ==="
