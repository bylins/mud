# Deployment Guide

Инструкции по развёртыванию для разных конфигураций.

## Режим 1: All-in-one

Circle и весь стек мониторинга на одном сервере.

```bash
cd tools/observability
./start.sh
```

Grafana: http://localhost:12000

---

## Режим 2: Split — Bylins + monitoring.bylins.su

### Шаг 1: WireGuard

Установка на **обоих** серверах:

```bash
sudo apt install wireguard
```

**На monitoring.bylins.su:**

```bash
# Генерируем ключи
wg genkey | sudo tee /etc/wireguard/privatekey | wg pubkey | sudo tee /etc/wireguard/publickey

# Создаём конфиг
sudo tee /etc/wireguard/wg0.conf > /dev/null << 'EOF'
[Interface]
Address = 10.10.0.1/24
ListenPort = 51820
PrivateKey = <ВСТАВИТЬ приватный ключ monitoring.bylins.su>

[Peer]
PublicKey = <ВСТАВИТЬ публичный ключ bylins.su>
AllowedIPs = 10.10.0.2/32
EOF

sudo chmod 600 /etc/wireguard/wg0.conf
sudo wg-quick up wg0
sudo systemctl enable wg-quick@wg0
```

**На bylins.su:**

```bash
wg genkey | sudo tee /etc/wireguard/privatekey | wg pubkey | sudo tee /etc/wireguard/publickey

sudo tee /etc/wireguard/wg0.conf > /dev/null << 'EOF'
[Interface]
Address = 10.10.0.2/24
PrivateKey = <ВСТАВИТЬ приватный ключ bylins.su>

[Peer]
PublicKey = <ВСТАВИТЬ публичный ключ monitoring.bylins.su>
Endpoint = monitoring.bylins.su:51820
AllowedIPs = 10.10.0.1/32
PersistentKeepalive = 25
EOF

sudo chmod 600 /etc/wireguard/wg0.conf
sudo wg-quick up wg0
sudo systemctl enable wg-quick@wg0
```

**Проверка туннеля:**

```bash
# На bylins.su:
ping 10.10.0.1

# На monitoring.bylins.su:
ping 10.10.0.2

# Статус туннеля:
sudo wg show
```

### Шаг 2: Стек метрик на monitoring.bylins.su

```bash
cd tools/observability
MODE=monitoring-server ./start.sh
```

OTEL Collector будет слушать на `10.10.0.1:4317` и `127.0.0.1:4317`.
Grafana — только на `127.0.0.1:12000`.

Открыть порт WireGuard в файрволе (если есть ufw):

```bash
sudo ufw allow 51820/udp
```

OTEL порт **не открываем** в интернет — он доступен только через WireGuard.

### Шаг 3: OTEL агент на bylins.su

```bash
cd tools/observability
MODE=agent ./start.sh
```

Агент принимает данные от circle на `127.0.0.1:4317` и форвардит на `10.10.0.1:4317`.

Если нужно переопределить адрес gateway:

```bash
MODE=agent OTEL_GATEWAY=10.10.0.1 ./start.sh
```

### Шаг 4: Запуск circle

Circle отправляет метрики на локальный OTEL агент — конфигурация та же, что и при all-in-one:

```bash
cd build_otel
./circle -d small 4000
```

Адрес OTEL Collector в `configuration.xml` — `127.0.0.1:4317` (агент на том же сервере).

### Шаг 5: Grafana через SSH-туннель

Grafana работает только на `127.0.0.1:12000` на monitoring.bylins.su. Для доступа:

```bash
ssh -L 12000:127.0.0.1:12000 user@monitoring.bylins.su
```

Затем открыть http://localhost:12000 в браузере.

---

## Режим 3: Circle на monitoring.bylins.su (без агента)

Если circle запущен прямо на сервере метрик, дополнительный агент не нужен —
OTEL Collector уже слушает на `127.0.0.1:4317` как часть основного стека.

```bash
# На monitoring.bylins.su запускаем стек:
MODE=monitoring-server ./start.sh

# Запускаем circle (в build_otel):
cd build_otel
./circle -d small 4000
# Circle отправляет на 127.0.0.1:4317 — попадает прямо в OTEL Collector стека
```

---

## Остановка и обновление

```bash
# Остановить
./start.sh down

# Остановить с очисткой volumes (удалит все данные!)
./start.sh down -v

# Обновить образы
./start.sh pull
./start.sh up -d
```

## Диагностика

```bash
# Статус контейнеров
./start.sh ps

# Логи OTEL Collector
./start.sh logs otel-collector

# Логи агента (на bylins.su)
MODE=agent ./start.sh logs otel-agent

# Проверить, что агент получает данные
curl -s http://localhost:13133/  # health check агента

# Посмотреть метрики самого коллектора
curl -s http://localhost:8888/metrics | grep otelcol_exporter
```
