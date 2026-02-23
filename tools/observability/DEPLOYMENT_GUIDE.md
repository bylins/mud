# OpenTelemetry Stack Deployment Guide для Bylins MUD

Полное руководство по развёртыванию телеметрии: OTEL Collector → Prometheus/Loki/Tempo → Grafana

## Архитектура

```
┌─────────────┐
│ Bylins MUD  │ (C++ app с OTEL SDK)
└──────┬──────┘
       │ OTLP/gRPC (4317)
       │
┌──────▼──────────────┐
│ OTEL Collector      │ (агрегация, фильтрация, маршрутизация)
└──────┬──────────────┘
       │
       ├─────────────┐─────────────┐
       │             │             │
┌──────▼──────┐ ┌───▼───────┐ ┌───▼────────┐
│ Prometheus  │ │   Tempo   │ │    Loki    │
│  (metrics)  │ │ (traces)  │ │   (logs)   │
└──────┬──────┘ └─────┬─────┘ └─────┬──────┘
       │              │             │
       └──────────────┴─────────────┘
                      │
              ┌───────▼────────┐
              │    Grafana     │ (visualization)
              └────────────────┘
```

## Требования

### Hardware
- **CPU**: 2+ ядра (рекомендуется 4)
- **RAM**: 4GB минимум (рекомендуется 8GB)
- **Disk**: 20GB минимум (зависит от retention)
- **Network**: 1 Gbps (локальная сеть)

### Software
- `docker.io` (Docker 20.10+)
- `docker-compose` (standalone, пакет `docker-compose`, не плагин `docker compose`)

Установка на Ubuntu:
```bash
sudo apt install docker.io docker-compose
sudo usermod -aG docker $USER  # добавить себя в группу docker
newgrp docker                  # применить без перелогина
```

## Вариант 1: Docker Compose (рекомендуется для начала)

### Структура файлов

```
mud.otel/
├── docker-compose.observability.yml
├── otel-collector-config.yaml
├── prometheus.yml
├── loki-config.yaml
├── tempo-config.yaml
├── grafana/
│   ├── provisioning/
│   │   ├── datasources/
│   │   │   └── datasources.yml
│   │   └── dashboards/
│   │       └── dashboards.yml
│   └── dashboards/
│       ├── performance-dashboard.json
│       ├── business-logic-dashboard.json
│       └── operational-dashboard.json
└── build/
    └── circle  (Bylins binary)
```

### 1. docker-compose.observability.yml

```yaml
version: '3.8'

services:
  # OpenTelemetry Collector
  otel-collector:
    image: otel/opentelemetry-collector-contrib:0.91.0
    container_name: mud-otel-collector
    command: ["--config=/etc/otel-collector-config.yaml"]
    volumes:
      - ./otel-collector-config.yaml:/etc/otel-collector-config.yaml:ro
    ports:
      - "4317:4317"   # OTLP gRPC receiver
      - "4318:4318"   # OTLP HTTP receiver
      - "8888:8888"   # Prometheus metrics (collector's own metrics)
      - "13133:13133" # Health check
    networks:
      - observability
    restart: unless-stopped

  # Prometheus (metrics)
  prometheus:
    image: prom/prometheus:v2.48.0
    container_name: mud-prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
      - '--storage.tsdb.retention.time=30d'
      - '--web.enable-remote-write-receiver'
      - '--enable-feature=exemplar-storage'
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - prometheus-data:/prometheus
    ports:
      - "9090:9090"
    networks:
      - observability
    restart: unless-stopped

  # Tempo (traces)
  tempo:
    image: grafana/tempo:2.3.1
    container_name: mud-tempo
    command: ["-config.file=/etc/tempo.yaml"]
    volumes:
      - ./tempo-config.yaml:/etc/tempo.yaml:ro
      - tempo-data:/tmp/tempo
    ports:
      - "3200:3200"   # Tempo HTTP
      - "4316:4317"   # OTLP gRPC (alternative port to avoid conflict)
    networks:
      - observability
    restart: unless-stopped

  # Loki (logs)
  loki:
    image: grafana/loki:2.9.3
    container_name: mud-loki
    command: -config.file=/etc/loki/config.yaml
    volumes:
      - ./loki-config.yaml:/etc/loki/config.yaml:ro
      - loki-data:/loki
    ports:
      - "3100:3100"
    networks:
      - observability
    restart: unless-stopped

  # Grafana (visualization)
  grafana:
    image: grafana/grafana:10.2.2
    container_name: mud-grafana
    environment:
      - GF_SECURITY_ADMIN_USER=admin
      - GF_SECURITY_ADMIN_PASSWORD=admin123
      - GF_USERS_ALLOW_SIGN_UP=false
      - GF_FEATURE_TOGGLES_ENABLE=traceqlEditor
    volumes:
      - ./grafana/provisioning:/etc/grafana/provisioning:ro
      - ./tools/observability/dashboards:/var/lib/grafana/dashboards:ro
      - grafana-data:/var/lib/grafana
    ports:
      - "3000:3000"
    networks:
      - observability
    depends_on:
      - prometheus
      - tempo
      - loki
    restart: unless-stopped

volumes:
  prometheus-data:
  tempo-data:
  loki-data:
  grafana-data:

networks:
  observability:
    driver: bridge
```

### 2. otel-collector-config.yaml

```yaml
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
      http:
        endpoint: 0.0.0.0:4318

processors:
  # Батчинг для эффективной отправки
  batch:
    timeout: 5s
    send_batch_size: 512
    send_batch_max_size: 1024

  # Фильтрация метрик (опционально)
  filter/drop_noisy:
    metrics:
      exclude:
        match_type: strict
        metric_names:
          # Добавьте сюда метрики, которые не нужны
          # - some.noisy.metric

  # Tail-based sampling для traces (опционально)
  tail_sampling:
    decision_wait: 10s
    num_traces: 10000
    expected_new_traces_per_sec: 100
    policies:
      # Всегда сохраняем ошибки
      - name: errors
        type: status_code
        status_code: {status_codes: [ERROR]}

      # Всегда сохраняем медленные операции
      - name: slow-operations
        type: latency
        latency: {threshold_ms: 100}

      # Всегда сохраняем combat traces с багами
      - name: combat-with-death
        type: string_attribute
        string_attribute: {key: event.name, values: [combat_ended, death]}

      # Семплируем остальное 10%
      - name: probabilistic
        type: probabilistic
        probabilistic: {sampling_percentage: 10}

  # Добавление resource attributes
  resource:
    attributes:
      - key: service.name
        value: bylins-mud
        action: upsert
      - key: deployment.environment
        value: production
        action: upsert

  # Memory limiter для защиты от OOM
  memory_limiter:
    check_interval: 1s
    limit_mib: 512
    spike_limit_mib: 128

exporters:
  # Prometheus для метрик
  prometheusremotewrite:
    endpoint: http://prometheus:9090/api/v1/write
    tls:
      insecure: true

  # Tempo для traces
  otlp/tempo:
    endpoint: tempo:4317
    tls:
      insecure: true

  # Loki для логов
  loki:
    endpoint: http://loki:3100/loki/api/v1/push
    tls:
      insecure: true

  # Debug exporter (опционально, для отладки)
  # logging:
  #   loglevel: debug

service:
  pipelines:
    # Pipeline для метрик
    metrics:
      receivers: [otlp]
      processors: [memory_limiter, batch, resource]
      exporters: [prometheusremotewrite]

    # Pipeline для traces
    traces:
      receivers: [otlp]
      processors: [memory_limiter, tail_sampling, batch, resource]
      exporters: [otlp/tempo]

    # Pipeline для логов
    logs:
      receivers: [otlp]
      processors: [memory_limiter, batch, resource]
      exporters: [loki]

  # Собственные метрики коллектора
  telemetry:
    logs:
      level: info
    metrics:
      address: 0.0.0.0:8888
```

### 3. prometheus.yml

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    cluster: 'bylins-production'
    environment: 'production'

# Алерты (опционально)
# alerting:
#   alertmanagers:
#     - static_configs:
#         - targets: ['alertmanager:9093']

# Правила для алертов
# rule_files:
#   - '/etc/prometheus/alerts/*.yml'

scrape_configs:
  # Prometheus scrapes itself
  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']

  # OTEL Collector metrics
  - job_name: 'otel-collector'
    static_configs:
      - targets: ['otel-collector:8888']

  # Bylins MUD metrics (если есть прямой Prometheus endpoint)
  # - job_name: 'bylins-mud'
  #   static_configs:
  #     - targets: ['bylins-mud:8080']

# Remote write получение от OTEL Collector (уже включено в docker-compose через --web.enable-remote-write-receiver)
```

### 4. tempo-config.yaml

```yaml
server:
  http_listen_port: 3200

distributor:
  receivers:
    otlp:
      protocols:
        grpc:
          endpoint: 0.0.0.0:4317

ingester:
  max_block_duration: 5m

compactor:
  compaction:
    block_retention: 720h  # 30 дней

storage:
  trace:
    backend: local
    local:
      path: /tmp/tempo/blocks
    wal:
      path: /tmp/tempo/wal
    cache: memcached
    memcached:
      consistent_hash: true
      host: localhost
      service: memcached-client
      timeout: 500ms

overrides:
  per_tenant_override_config: /etc/tempo-overrides.yaml
  defaults:
    max_traces_per_user: 10000
    max_bytes_per_trace: 5000000
```

### 5. loki-config.yaml

```yaml
auth_enabled: false

server:
  http_listen_port: 3100
  grpc_listen_port: 9096

common:
  path_prefix: /loki
  storage:
    filesystem:
      chunks_directory: /loki/chunks
      rules_directory: /loki/rules
  replication_factor: 1
  ring:
    kvstore:
      store: inmemory

schema_config:
  configs:
    - from: 2024-01-01
      store: boltdb-shipper
      object_store: filesystem
      schema: v11
      index:
        prefix: index_
        period: 24h

storage_config:
  boltdb_shipper:
    active_index_directory: /loki/boltdb-shipper-active
    cache_location: /loki/boltdb-shipper-cache
    cache_ttl: 24h
    shared_store: filesystem
  filesystem:
    directory: /loki/chunks

limits_config:
  reject_old_samples: true
  reject_old_samples_max_age: 168h  # 7 дней
  ingestion_rate_mb: 10
  ingestion_burst_size_mb: 20
  max_streams_per_user: 10000
  max_query_series: 1000

chunk_store_config:
  max_look_back_period: 720h  # 30 дней

table_manager:
  retention_deletes_enabled: true
  retention_period: 720h  # 30 дней
```

### 6. grafana/provisioning/datasources/datasources.yml

```yaml
apiVersion: 1

datasources:
  # Prometheus
  - name: Prometheus
    type: prometheus
    access: proxy
    url: http://prometheus:9090
    isDefault: true
    editable: false
    jsonData:
      httpMethod: POST
      timeInterval: 15s
      exemplarTraceIdDestinations:
        - name: trace_id
          datasourceUid: tempo

  # Tempo
  - name: Tempo
    type: tempo
    access: proxy
    url: http://tempo:3200
    uid: tempo
    editable: false
    jsonData:
      httpMethod: GET
      tracesToLogs:
        datasourceUid: loki
        tags: ['trace_id', 'span_id']
        mappedTags: [{ key: 'service.name', value: 'service' }]
        mapTagNamesEnabled: true
        spanStartTimeShift: '-1m'
        spanEndTimeShift: '1m'
      tracesToMetrics:
        datasourceUid: prometheus
        tags: [{ key: 'service.name', value: 'service' }]
      serviceMap:
        datasourceUid: prometheus
      nodeGraph:
        enabled: true

  # Loki
  - name: Loki
    type: loki
    access: proxy
    url: http://loki:3100
    uid: loki
    editable: false
    jsonData:
      maxLines: 1000
      derivedFields:
        - datasourceUid: tempo
          matcherRegex: "trace_id=(\\w+)"
          name: TraceID
          url: '$${__value.raw}'
```

### 7. grafana/provisioning/dashboards/dashboards.yml

```yaml
apiVersion: 1

providers:
  - name: 'Bylins MUD Dashboards'
    orgId: 1
    folder: 'Bylins'
    type: file
    disableDeletion: false
    updateIntervalSeconds: 30
    allowUiUpdates: true
    options:
      path: /var/lib/grafana/dashboards
      foldersFromFilesStructure: true
```

## Сборка Bylins MUD с поддержкой OTEL

> Если вы деплоите готовый бинарник (собранный на другой машине или в CI),
> этот раздел можно пропустить — opentelemetry-cpp нужен только при сборке.

### Установка SDK

Пакет `libopentelemetry-cpp-dev` отсутствует в стандартных репозиториях Ubuntu.
Используйте скрипт `tools/observability/install-otel-sdk.sh`:

```bash
cd tools/observability

# Вариант 1 (основной): vcpkg
# Устанавливает vcpkg в ~/vcpkg (или $VCPKG_DIR) и собирает SDK через него.
./install-otel-sdk.sh

# Вариант 2 (альтернатива): сборка из исходников
# Устанавливает SDK в /usr/local (или $OTEL_INSTALL_PREFIX).
# Занимает ~15 минут.
./install-otel-sdk.sh --source
```

### Сборка сервера

**После vcpkg:**
```bash
cmake -S . -B build_otel \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_OTEL=ON \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=~/vcpkg/installed/x64-linux
make -C build_otel -j$(($(nproc)/2))
```

**После сборки из исходников** (SDK в `/usr/local`, cmake найдёт сам):
```bash
cmake -S . -B build_otel \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_OTEL=ON
make -C build_otel -j$(($(nproc)/2))
```

## Запуск стека

### Шаг 1: Подготовка файлов

```bash
cd tools/observability

# Конфиги должны быть читаемы контейнерами (которые запускаются не от root)
chmod 644 *.yml *.yaml
chmod 644 grafana/provisioning/datasources/*.yml
chmod 644 grafana/provisioning/dashboards/*.yml
```

### Шаг 2: Запуск сервисов

#### Вариант A: данные в Docker named volumes (по умолчанию)

```bash
docker-compose -f docker-compose.observability.yml up -d
```

Данные хранятся в Docker volumes, управляемых демоном. Расположение:
`/var/lib/docker/volumes/observability_<name>/`

#### Вариант B: данные в директории хоста

Контейнеры записывают данные от своего внутреннего пользователя. Чтобы файлы
принадлежали запустившему пользователю, используется `user:` в override-файле.

```bash
export DATA_DIR=/var/lib/mud-observability
export UID=$(id -u)
export GID=$(id -g)
mkdir -p $DATA_DIR/{prometheus,tempo,loki,grafana}

docker-compose \
  -f docker-compose.observability.yml \
  -f docker-compose.data-dir.yml \
  up -d
```

`DATA_DIR` можно задать в `.env` файле рядом с compose-файлами:
```bash
echo "DATA_DIR=/var/lib/mud-observability" > .env
```

#### Общие команды

```bash
# Проверить статус
docker-compose -f docker-compose.observability.yml ps

# Посмотреть логи
docker-compose -f docker-compose.observability.yml logs -f

# Проверить health OTEL Collector
curl http://localhost:13133
```

### Шаг 3: Настройка Bylins MUD

#### Environment variables для Bylins

```bash
# В systemd service или docker-compose для Bylins
export OTEL_EXPORTER_OTLP_ENDPOINT=http://localhost:4317
export OTEL_EXPORTER_OTLP_PROTOCOL=grpc
export OTEL_SERVICE_NAME=bylins-mud
export OTEL_RESOURCE_ATTRIBUTES=deployment.environment=production,service.version=1.0.0

# Запустить Bylins
./build/circle 4000
```

#### Конфигурация в коде (уже сделано в otel-integration)

Убедитесь что в коде Bylins настроен OTEL endpoint:

```cpp
// src/engine/observability/otel_provider.cpp
OtelProvider::OtelProvider() {
    auto endpoint = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
    if (!endpoint) {
        endpoint = "http://localhost:4317";  // default
    }

    // ... инициализация OTLP exporter с endpoint
}
```

### Шаг 4: Проверка работы

```bash
# 1. Проверить OTEL Collector принимает данные
curl http://localhost:8888/metrics | grep otelcol_receiver

# 2. Проверить Prometheus получает метрики
curl http://localhost:9090/api/v1/query?query=up

# 3. Проверить Tempo получает traces
curl http://localhost:3200/api/search | jq

# 4. Проверить Loki получает логи
curl http://localhost:3100/loki/api/v1/label

# 5. Открыть Grafana
open http://localhost:3000
# Login: admin / admin123
```

### Шаг 5: Импорт дашбордов (автоматический)

Дашборды автоматически импортируются из `tools/observability/dashboards/` через provisioning.

Проверить в Grafana:
- Dashboards → Browse → Bylins folder
- Должны быть видны:
  - Performance Dashboard
  - Business Logic Dashboard
  - Operational Dashboard

## Проверка интеграции

### 1. Тест метрик

```bash
# В Grafana → Explore → Prometheus
# Запрос:
rate(combat_rounds_total[5m])

# Должен показать rate боёв
```

### 2. Тест traces

```bash
# В Grafana → Explore → Tempo
# Query: service.name = "bylins-mud"
# TraceQL: { span.name = "Combat round" }

# Должны быть видны traces боёв
```

### 3. Тест логов

```bash
# В Grafana → Explore → Loki
# Query:
{service_name="bylins-mud"} | logfmt | level = "INFO"

# Должны быть видны логи
```

### 4. Тест корреляции Logs ↔ Traces

```bash
# В Grafana → Explore → Loki
{service_name="bylins-mud"} | logfmt | trace_id != ""

# Кликнуть на trace_id → должен открыться Tempo с этим trace
```

## Production настройки

### 1. Включить tail-based sampling

В `otel-collector-config.yaml` раскомментировать `tail_sampling` processor.

**Рекомендуется** для production с >50 игроками онлайн.

### 2. Настроить retention

**Prometheus** (в docker-compose.observability.yml):
```yaml
- '--storage.tsdb.retention.time=30d'  # 30 дней метрик
```

**Tempo** (в tempo-config.yaml):
```yaml
compactor:
  compaction:
    block_retention: 720h  # 30 дней traces (720h = 30 days)
```

**Loki** (в loki-config.yaml):
```yaml
table_manager:
  retention_period: 720h  # 30 дней логов
```

### 3. Добавить алерты

Создать файл `prometheus-alerts.yml`:

```yaml
groups:
  - name: bylins_alerts
    interval: 30s
    rules:
      # Алерт на высокую латентность боя
      - alert: HighCombatLatency
        expr: histogram_quantile(0.99, rate(combat_round_duration_bucket[5m])) > 0.015
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High combat round latency"
          description: "p99 combat round duration is {{ $value }}s (threshold 15ms)"

      # Алерт на много активных боёв
      - alert: TooManyCombats
        expr: combat_active_count > 20
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Too many active combats"
          description: "{{ $value }} active combats (threshold 20)"

      # Алерт на падение игроков
      - alert: PlayersDropped
        expr: rate(players_online_count[5m]) < -5
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "Players dropping rapidly"
          description: "Player count dropped by {{ $value }}/sec"
```

### 4. Мониторинг самого OTEL

```bash
# OTEL Collector экспортирует свои метрики на :8888
# Добавить в Prometheus scrape_configs (уже есть выше)

# Ключевые метрики для мониторинга:
# - otelcol_receiver_accepted_spans
# - otelcol_receiver_refused_spans
# - otelcol_exporter_sent_spans
# - otelcol_processor_batch_batch_send_size
```

## Troubleshooting

### Проблема: Bylins не отправляет данные

**Решение**:
```bash
# 1. Проверить environment variables
env | grep OTEL

# 2. Проверить connectivity
telnet localhost 4317

# 3. Проверить логи Bylins
tail -f syslog | grep -i otel

# 4. Проверить OTEL Collector логи
docker logs mud-otel-collector
```

### Проблема: OTEL Collector не экспортирует в Prometheus

**Решение**:
```bash
# 1. Проверить метрики коллектора
curl http://localhost:8888/metrics | grep exporter

# 2. Проверить логи коллектора
docker logs mud-otel-collector | grep -i error

# 3. Проверить Prometheus принимает remote write
curl http://localhost:9090/api/v1/status/config | jq
```

### Проблема: Traces не видны в Tempo

**Решение**:
```bash
# 1. Проверить OTLP endpoint Tempo
curl http://localhost:3200/api/echo

# 2. Проверить traces в Tempo
curl http://localhost:3200/api/search | jq

# 3. Проверить sampling rate (возможно все traces отфильтрованы)
# Временно отключить tail_sampling в otel-collector-config.yaml
```

### Проблема: Высокий memory usage OTEL Collector

**Решение**:
```yaml
# В otel-collector-config.yaml увеличить memory_limiter
processors:
  memory_limiter:
    check_interval: 1s
    limit_mib: 1024  # увеличить с 512
    spike_limit_mib: 256  # увеличить с 128
```

### Проблема: Дашборды не загружаются

**Решение**:
```bash
# 1. Проверить provisioning
docker exec -it mud-grafana ls -la /etc/grafana/provisioning/dashboards/

# 2. Проверить путь к дашбордам
docker exec -it mud-grafana ls -la /var/lib/grafana/dashboards/

# 3. Проверить логи Grafana
docker logs mud-grafana | grep -i dashboard

# 4. Перезапустить Grafana
docker-compose -f docker-compose.observability.yml restart grafana
```

## Вариант 2: Kubernetes Deployment (опционально)

Для production с масштабированием рекомендуется Kubernetes.

### Helm Charts

```bash
# 1. Установить OTEL Operator
kubectl apply -f https://github.com/open-telemetry/opentelemetry-operator/releases/latest/download/opentelemetry-operator.yaml

# 2. Установить Prometheus (kube-prometheus-stack)
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install prometheus prometheus-community/kube-prometheus-stack

# 3. Установить Tempo
helm repo add grafana https://grafana.github.io/helm-charts
helm install tempo grafana/tempo

# 4. Установить Loki
helm install loki grafana/loki-stack

# 5. Установить Grafana (уже включён в kube-prometheus-stack)
```

### OpenTelemetry Collector в K8s

```yaml
# otel-collector-k8s.yaml
apiVersion: opentelemetry.io/v1alpha1
kind: OpenTelemetryCollector
metadata:
  name: bylins-otel-collector
spec:
  mode: deployment
  config: |
    # Вставить содержимое otel-collector-config.yaml
```

### Bylins Deployment

```yaml
# bylins-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: bylins-mud
spec:
  replicas: 1
  selector:
    matchLabels:
      app: bylins-mud
  template:
    metadata:
      labels:
        app: bylins-mud
    spec:
      containers:
      - name: bylins
        image: bylins-mud:latest
        env:
        - name: OTEL_EXPORTER_OTLP_ENDPOINT
          value: "http://bylins-otel-collector:4317"
        - name: OTEL_SERVICE_NAME
          value: "bylins-mud"
        ports:
        - containerPort: 4000
```

## Best Practices

### 1. Security

- ✅ Использовать TLS для OTLP в production
- ✅ Настроить authentication в Grafana
- ✅ Ограничить доступ к метрикам через firewall
- ✅ Регулярно обновлять образы Docker

### 2. Performance

- ✅ Использовать gRPC вместо HTTP для OTLP (быстрее)
- ✅ Настроить batch processors (уже настроено)
- ✅ Включить sampling для traces при высокой нагрузке
- ✅ Мониторить memory usage OTEL Collector

### 3. Reliability

- ✅ Настроить persistent volumes для данных
- ✅ Настроить backups для Grafana dashboards
- ✅ Мониторить сам OTEL Collector
- ✅ Настроить алерты на критичные метрики

### 4. Cost Optimization

- ⚠️ Настроить retention период (не хранить вечно)
- ⚠️ Включить tail-based sampling (10-20%)
- ⚠️ Удалять неиспользуемые метрики через processor filter
- ⚠️ Использовать compression для экспорта

## Полезные команды

```bash
# Перезапустить весь стек
docker-compose -f docker-compose.observability.yml restart

# Остановить стек
docker-compose -f docker-compose.observability.yml down

# Удалить все данные (volumes)
docker-compose -f docker-compose.observability.yml down -v

# Обновить конфигурацию без перезапуска (для большинства изменений)
docker-compose -f docker-compose.observability.yml up -d

# Экспорт дашборда из Grafana
curl -H "Authorization: Bearer $GRAFANA_API_KEY" \
  http://localhost:3000/api/dashboards/uid/bylins-performance > dashboard-backup.json

# Импорт дашборда в Grafana
curl -X POST -H "Content-Type: application/json" \
  -H "Authorization: Bearer $GRAFANA_API_KEY" \
  -d @dashboard-backup.json \
  http://localhost:3000/api/dashboards/db
```

## Резюме

После завершения setup вы получите:

✅ **Полный observability stack** для Bylins MUD
✅ **Автоматический сбор** метрик, traces, логов
✅ **3 готовых дашборда** в Grafana
✅ **Корреляция** между логами и traces
✅ **Production-ready** конфигурация с sampling и retention

**Порты**:
- `3000` - Grafana UI
- `9090` - Prometheus UI
- `3100` - Loki (internal)
- `3200` - Tempo (internal)
- `4317` - OTEL Collector (OTLP gRPC)
- `8888` - OTEL Collector metrics

**URLs**:
- Grafana: http://localhost:3000 (admin/admin123)
- Prometheus: http://localhost:9090
- OTEL Collector health: http://localhost:13133

---
*Дата: 2026-01-28*
*Версия: 1.0*
