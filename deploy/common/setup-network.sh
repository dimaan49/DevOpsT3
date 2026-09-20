#!/bin/bash
set -euo pipefail

# Настройка статического IP на Host-only интерфейсе.
# Использование:
#   sudo bash setup-network.sh --role app
#   sudo bash setup-network.sh --role db
#
# --role app → 192.168.56.101
# --role db  → 192.168.56.102

ROLE=""

while [ $# -gt 0 ]; do
    case "$1" in
        --role)
            ROLE="${2:-}"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
done

if [ -z "${ROLE}" ]; then
    echo "Usage: $0 --role {app|db}"
    exit 1
fi

case "${ROLE}" in
    app) STATIC_IP="192.168.56.101" ;;
    db)  STATIC_IP="192.168.56.102" ;;
    *)
        echo "Invalid role: ${ROLE}. Use 'app' or 'db'."
        exit 1
        ;;
esac

PREFIX="24"

echo "=== Detecting Host-only interface ==="

# NAT — интерфейс с default route
NAT_IFACE=$(ip -o route show default | awk '{print $5}' | head -1)

if [ -z "${NAT_IFACE}" ]; then
    echo "ERROR: cannot detect NAT interface (no default route)"
    exit 1
fi

echo "NAT interface: ${NAT_IFACE}"

# Host-only — первый ethernet-интерфейс, кроме NAT
HOSTONLY_IFACE=$(nmcli -t -f DEVICE,TYPE,STATE device status \
    | awk -F: '$2=="ethernet" && $3=="connected" {print $1}' \
    | grep -v "^${NAT_IFACE}$" \
    | head -1)

if [ -z "${HOSTONLY_IFACE}" ]; then
    echo "ERROR: cannot detect host-only interface"
    exit 1
fi

echo "Host-only interface: ${HOSTONLY_IFACE}"

# Имя NM-подключения, привязанного к этому интерфейсу
CONN_NAME=$(nmcli -t -f NAME,DEVICE connection show --active \
    | awk -F: -v dev="${HOSTONLY_IFACE}" '$2==dev {print $1}' \
    | head -1)

if [ -z "${CONN_NAME}" ]; then
    echo "ERROR: no active NetworkManager connection for ${HOSTONLY_IFACE}"
    exit 1
fi

echo "Connection name: ${CONN_NAME}"

echo "=== Configuring static IP ==="

nmcli connection modify "${CONN_NAME}" \
    ipv4.method manual \
    ipv4.addresses "${STATIC_IP}/${PREFIX}" \
    ipv4.gateway "" \
    ipv4.dns "" \
    connection.autoconnect yes

echo "=== Bringing connection up ==="

nmcli connection up "${CONN_NAME}"

sleep 2

echo "=== Result ==="

ip -br addr show "${HOSTONLY_IFACE}"

echo
echo "=== Network setup completed ==="
echo "Host-only interface: ${HOSTONLY_IFACE}"
echo "Connection: ${CONN_NAME}"
echo "Static IP: ${STATIC_IP}/${PREFIX}"
