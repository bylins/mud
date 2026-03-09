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

Let's Encrypt хранит приватный ключ с правами 600 (только root). Docker-контейнер
читать его не может, поэтому копируем сертификаты в отдельную директорию:

```bash
sudo mkdir -p /opt/otel-certs
sudo cp /etc/letsencrypt/live/monitoring.bylins.su/fullchain.pem /opt/otel-certs/
sudo cp /etc/letsencrypt/live/monitoring.bylins.su/privkey.pem /opt/otel-certs/
sudo chmod 644 /opt/otel-certs/*.pem
```

Настроить автоматическое обновление копии и перезапуск OTEL Collector после продления сертификата:

```bash
sudo tee /etc/letsencrypt/renewal-hooks/deploy/refresh-otel-certs.sh > /dev/null << 'EOF'
#!/bin/sh
cp /etc/letsencrypt/live/monitoring.bylins.su/fullchain.pem /opt/otel-certs/
cp /etc/letsencrypt/live/monitoring.bylins.su/privkey.pem /opt/otel-certs/
chmod 644 /opt/otel-certs/*.pem
cd /path/to/mud/tools/observability
MODE=monitoring-server ./start.sh restart otel-collector
EOF
sudo chmod +x /etc/letsencrypt/renewal-hooks/deploy/refresh-otel-certs.sh
```

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

Стек поднимается в режиме `monitoring-server`. Circle подключается к `127.0.0.1:4317`
без TLS и авторизации — OTEL gateway принимает локальные подключения без проверок.

> Примечание: в текущей конфигурации gateway требует bearer token на всех подключениях,
> включая локальные. Если circle запускается на том же сервере, передай токен через
> переменную окружения в `configuration.xml` или используй режим `all-in-one`.

---

## Остановка и обновление

```bash
# Остановить
./start.sh down                         # all-in-one
MODE=monitoring-server ./start.sh down  # мониторинговый сервер
MODE=agent ./start.sh down              # агент на bylins.su

# Остановить с очисткой volumes (удалит все данные!)
MODE=monitoring-server ./start.sh down -v

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
