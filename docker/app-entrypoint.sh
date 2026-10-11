#!/bin/sh
set -eu

echo "Applying database migrations..."
/app/scripts/migrate.sh "${AUCTIONHUB_DB_NAME:-auctionhub}"

echo "Starting AuctionHub..."
exec /usr/local/bin/auctionhub_server
