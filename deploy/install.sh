#!/bin/bash
set -euo pipefail

# Usage:
#   sudo bash deploy/install.sh --common
#   sudo bash deploy/install.sh --db
#   sudo bash deploy/install.sh --app
#
# --common  настройка SSH и сети 
# # --db      установка и настройка сервера БД
# --app     установка и настройка сервера приложения
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root (use sudo)."
    exit 1
fi

run_scripts() {
    local dir="$1"
    shift

    if [ ! -d "${SCRIPT_DIR}/${dir}" ]; then
        echo "Directory not found: ${SCRIPT_DIR}/${dir}"
        exit 1
    fi

    local scripts
    scripts=$(find "${SCRIPT_DIR}/${dir}" -maxdepth 1 -name '*.sh' | sort)

    if [ -z "${scripts}" ]; then
        echo "No scripts found in ${SCRIPT_DIR}/${dir}"
        return
    fi

    for script in ${scripts}; do
        echo
        echo "=== Running ${script} ==="
        bash "${script}"
    done
}

case "${1:-}" in
    --common)
        run_scripts common
        ;;
    --db)
        run_scripts common
        run_scripts db
        ;;
    --app)
        run_scripts common
        run_scripts app
        ;;
    *)
        echo "Usage: $0 {--common|--db|--app}"
        exit 1
        ;;
esac

echo
echo "=== Done ==="
