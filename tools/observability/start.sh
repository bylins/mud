#!/bin/sh
set -e

cd "$(dirname "$0")"

# Fix config file permissions (containers run as non-root)
chmod 644 ./*.yml ./*.yaml 2>/dev/null || true
chmod 644 grafana/provisioning/datasources/*.yml 2>/dev/null || true
chmod 644 grafana/provisioning/dashboards/*.yml 2>/dev/null || true

if [ $# -eq 0 ]; then
    set -- up -d
fi

if [ -n "$DATA_DIR" ]; then
    echo "Using bind mounts in: $DATA_DIR"
    mkdir -p "$DATA_DIR/prometheus" "$DATA_DIR/tempo" "$DATA_DIR/loki" "$DATA_DIR/grafana"

    export UID=$(id -u)
    export GID=$(id -g)

    exec docker-compose \
        -f docker-compose.observability.yml \
        -f docker-compose.data-dir.yml \
        "$@"
else
    echo "Using Docker named volumes (set DATA_DIR to use a host directory)"

    exec docker-compose \
        -f docker-compose.observability.yml \
        "$@"
fi
