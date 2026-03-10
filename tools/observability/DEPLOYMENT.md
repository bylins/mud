# Deployment Guide

Инструкции по развёртыванию для разных конфигураций.

## Требования

Docker и docker-compose нужны на каждом сервере, где запускается стек или агент:

```bash
sudo apt install docker.io docker-compose
sudo usermod -aG docker $USER
newgrp docker  # применить без перелогина
```

---

## Режим 1: All-in-one

Circle и весь стек мониторинга на одном сервере. Авторизация не нужна — всё на localhost.

```bash
cd tools/observability
./start.sh
```

Grafana: http://localhost:12000

---

## Режим 2: Split — Bylins + monitoring.bylins.su

```
[bylins.su]                              [monitoring.bylins.su]
  circle                                   UFW: 4317/tcp только с bylins.su IP
    |                                        |
    v 127.0.0.1:4317                         v 0.0.0.0:4317  (TLS + bearer token)
  OTEL Agent ── TLS + bearer token ──────> OTEL Gateway
                                             |  (docker internal network)
                                             |── Prometheus  127.0.0.1:9090
                                             |── Loki        127.0.0.1:3100
                                             └── Tempo       127.0.0.1:3200
                                            Grafana          127.0.0.1:12000
```

### Шаг 1: TLS-сертификат на monitoring.bylins.su

```bash
sudo apt install certbot
sudo certbot certonly --standalone -d monitoring.bylins.su
```

Сертификат будет в `/etc/letsencrypt/live/monitoring.bylins.su/`.

Сертификат монтируется в OTEL Collector напрямую из `/etc/letsencrypt`.
Контейнер запускается от root (см. `docker-compose.tls.yml`), поэтому имеет
доступ к приватному ключу. При продлении certbot обновляет файлы на месте —
перезапускать контейнер не нужно (OTEL Collector перечитывает TLS-контекст
при каждом новом подключении).

### Шаг 2: Bearer token

Сгенерировать токен (один раз, хранить в тайне):

```bash
openssl rand -hex 32
```

Сохранить в `.env` файл на **каждом** сервере (в директории `tools/observability/`):

```bash
# На monitoring.bylins.su:
cat > tools/observability/.env << EOF
OTEL_AUTH_TOKEN=<сгенерированный_токен>
EOF

# На bylins.su:
cat > tools/observability/.env << EOF
OTEL_AUTH_TOKEN=<тот_же_токен>
OTEL_GATEWAY=monitoring.bylins.su
EOF
```

`start.sh` читает `.env` автоматически при запуске. Файл не хранится в git (`.gitignore`).

### Шаг 3: Файрвол на monitoring.bylins.su

Открыть SSH и OTEL-порт только с IP сервера Былин:

```bash
sudo ufw allow ssh
sudo ufw allow from <IP_bylins.su> to any port 4317 proto tcp
sudo ufw enable
```

Всё остальное (Prometheus, Loki, Tempo, Grafana) остаётся только на 127.0.0.1.

### Шаг 4: Стек метрик на monitoring.bylins.su

```bash
cd tools/observability
MODE=monitoring-server ./start.sh
```

OTEL Collector поднимается с TLS и bearer token auth на `0.0.0.0:4317`.
Grafana — только на `127.0.0.1:12000`.

### Шаг 5: OTEL агент на bylins.su

```bash
cd tools/observability
MODE=agent ./start.sh
```

Агент принимает данные от circle на `127.0.0.1:4317` и форвардит на `monitoring.bylins.su:4317` с TLS и токеном.

### Шаг 6: Запуск circle

```bash
cd build_otel
./circle -d small 4000
```

Адрес OTEL Collector в `configuration.xml` — `127.0.0.1:4317` (агент на том же сервере).

### Шаг 7: Grafana через SSH-туннель

```bash
ssh -L 12000:127.0.0.1:12000 user@monitoring.bylins.su
```

Открыть http://localhost:12000 в браузере.

---

## Режим 3: Circle на monitoring.bylins.su (без агента)

```
[monitoring.bylins.su]
  circle ──OTLP/HTTP──> OTEL Gateway (порт 4319, без TLS/auth)
                          |── Prometheus  127.0.0.1:9090
                          |── Loki        127.0.0.1:3100
                          └── Tempo       127.0.0.1:3200
                         Grafana          127.0.0.1:12000
```

Стек поднимается в режиме `monitoring-server`. Для circle, запущенного на том же хосте,
gateway слушает на `127.0.0.1:4319` (HTTP, без TLS, без авторизации).
Внешний порт 4317 по-прежнему требует TLS + bearer token (для агентов с других серверов).

### Шаг 1–4: как в Режиме 2 (TLS, токен, файрвол, стек)

### Шаг 5: Настройка circle

В `configuration.xml` укажи endpoint на порт 4319:

```xml
<telemetry>
  <otlp>
    <metrics><endpoint>http://localhost:4319/v1/metrics</endpoint></metrics>
    <traces><endpoint>http://localhost:4319/v1/traces</endpoint></traces>
    <logs_otlp><endpoint>http://localhost:4319/v1/logs</endpoint></logs_otlp>
  </otlp>
</telemetry>
```

### Шаг 6: Запуск circle

```bash
CIRCLE=./build_otel/circle ./launch -d <world_dir> 4000
```

---

## Остановка и обновление

```bash
# Остановить
./start.sh down                         # all-in-one
MODE=monitoring-server ./start.sh down  # мониторинговый сервер
MODE=agent ./start.sh down              # агент на bylins.su

# Сбросить Docker volumes (только если DATA_DIR=docker-volumes; bind mounts не затрагивает)
DATA_DIR=docker-volumes MODE=monitoring-server ./start.sh down -v

# Обновить образы
MODE=monitoring-server ./start.sh pull
MODE=monitoring-server ./start.sh up -d
```

## Диагностика

```bash
# Статус контейнеров
MODE=monitoring-server ./start.sh ps

# Логи OTEL gateway (на monitoring.bylins.su)
MODE=monitoring-server ./start.sh logs otel-collector

# Логи OTEL агента (на bylins.su)
MODE=agent ./start.sh logs otel-agent

# Health check агента
curl -s http://localhost:13133/

# Метрики самого коллектора (экспорт данных в Prometheus)
curl -s http://localhost:8888/metrics | grep otelcol_exporter

# Проверить TLS и аутентификацию вручную (с bylins.su):
grpcurl -H "Authorization: Bearer <token>" monitoring.bylins.su:4317 \
    grpc.health.v1.Health/Check
```
