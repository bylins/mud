#!/bin/sh
set -e

cd "$(dirname "$0")"

# Prefer docker compose v2 (plugin) over docker-compose v1
if docker compose version >/dev/null 2>&1; then
    COMPOSE="docker compose"
else
    COMPOSE="docker-compose"
fi

# Load .env if present (variables already in environment take precedence)
if [ -f .env ]; then
    # shellcheck disable=SC1091
    . .env
fi

# Deployment mode:
#   all-in-one         — full stack on this machine, OTEL on 127.0.0.1 (default)
#   monitoring-server  — full stack, OTEL also on WireGuard IP (OTEL_BIND=10.10.0.1)
#   agent              — OTEL Collector only, forwards to remote OTEL_GATEWAY
MODE=${MODE:-all-in-one}

# Fix config file permissions (containers run as non-root)
chmod 755 grafana/ grafana/provisioning/ grafana/provisioning/datasources/ grafana/provisioning/dashboards/ dashboards/ 2>/dev/null || true
chmod 644 ./*.yml ./*.yaml 2>/dev/null || true
chmod 644 grafana/provisioning/datasources/*.yml 2>/dev/null || true
chmod 644 grafana/provisioning/dashboards/*.yml 2>/dev/null || true
chmod 644 dashboards/*.json 2>/dev/null || true

if [ $# -eq 0 ]; then
    set -- up -d
fi

case "$MODE" in
    agent)
        echo "Mode: agent  (forwards to OTEL_GATEWAY=${OTEL_GATEWAY:-monitoring.bylins.su})"
        if [ -z "$OTEL_AUTH_TOKEN" ]; then
            echo "ERROR: OTEL_AUTH_TOKEN is not set. Use the same token as on the monitoring server." >&2
            exit 1
        fi
        exec $COMPOSE \
            -f docker-compose.agent.yml \
            "$@"
        ;;

    monitoring-server)
        export OTEL_BIND=${OTEL_BIND:-0.0.0.0}
        export OTEL_COLLECTOR_CONFIG=otel-collector-gateway-config.yaml
        echo "Mode: monitoring-server  (OTEL bound to ${OTEL_BIND}, TLS+auth enabled)"
        if [ -z "$OTEL_AUTH_TOKEN" ]; then
            echo "ERROR: OTEL_AUTH_TOKEN is not set. Generate one with: openssl rand -hex 32" >&2
            exit 1
        fi
        if [ -n "$DATA_DIR" ]; then
            echo "Using bind mounts in: $DATA_DIR"
            mkdir -p "$DATA_DIR/prometheus" "$DATA_DIR/tempo" "$DATA_DIR/loki" "$DATA_DIR/grafana"
            export UID=$(id -u)
            export GID=$(id -g)
            exec $COMPOSE \
                -f docker-compose.observability.yml \
                -f docker-compose.tls.yml \
                -f docker-compose.data-dir.yml \
                "$@"
        else
            echo "Using Docker named volumes (set DATA_DIR to use a host directory)"
            exec $COMPOSE \
                -f docker-compose.observability.yml \
                -f docker-compose.tls.yml \
                "$@"
        fi
        ;;

    *)
        export OTEL_BIND=${OTEL_BIND:-127.0.0.1}
        echo "Mode: all-in-one  (OTEL bound to ${OTEL_BIND})"
        if [ -n "$DATA_DIR" ]; then
            echo "Using bind mounts in: $DATA_DIR"
            mkdir -p "$DATA_DIR/prometheus" "$DATA_DIR/tempo" "$DATA_DIR/loki" "$DATA_DIR/grafana"
            export UID=$(id -u)
            export GID=$(id -g)
            exec $COMPOSE \
                -f docker-compose.observability.yml \
                -f docker-compose.data-dir.yml \
                "$@"
        else
            echo "Using Docker named volumes (set DATA_DIR to use a host directory)"
            exec $COMPOSE \
                -f docker-compose.observability.yml \
                "$@"
        fi
        ;;
esac
