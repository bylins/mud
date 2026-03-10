# Bylins MUD — OpenTelemetry Observability

Стек мониторинга: **OTEL Collector → Prometheus / Loki / Tempo → Grafana**

## Режимы развёртывания

| Режим | Где запускать | Команда |
|-------|--------------|---------|
| `all-in-one` | один сервер (circle + весь стек) | `./start.sh` |
| `monitoring-server` | выделенный сервер метрик (monitoring.bylins.su) | `MODE=monitoring-server ./start.sh` |
| `agent` | сервер Былин (только OTEL агент) | `MODE=agent ./start.sh` |

### All-in-one (по умолчанию)

Circle и весь стек мониторинга на одном сервере.

```
circle ──OTLP/HTTP──> OTEL Collector ──> Prometheus / Loki / Tempo ──> Grafana
                      127.0.0.1:4318                                    :12000
```

```bash
cd tools/observability
./start.sh          # запуск
./start.sh down     # остановка
./start.sh logs -f  # логи
```

Grafana: http://localhost:12000 (admin / admin123)

### Split: Сервер Былин + monitoring.bylins.su

Circle работает на `bylins.su`, весь стек — на `monitoring.bylins.su`.
Трафик идёт напрямую через интернет с TLS + bearer token аутентификацией.
Grafana доступна через SSH-туннель с `monitoring.bylins.su`.

Подробная инструкция: [DEPLOYMENT.md](DEPLOYMENT.md)

```
[bylins.su]                              [monitoring.bylins.su]
  circle                                   UFW: 4317/tcp открыт
    |                                        |
    v 127.0.0.1:4317                         v 0.0.0.0:4317  (TLS + bearer token)
  OTEL Agent ── OTLP/gRPC + TLS ──────────> OTEL Gateway
                bearer token auth            |  (docker internal network)
                                             |── Prometheus  127.0.0.1:9090
                                             |── Loki        127.0.0.1:3100
                                             └── Tempo       127.0.0.1:3200
                                            Grafana          127.0.0.1:12000
```

**На мониторинговом сервере** (`monitoring.bylins.su`):
```bash
MODE=monitoring-server ./start.sh
```
OTEL Collector поднимается на `0.0.0.0:4317` (TLS + auth) и `127.0.0.1:4319` (локально, без auth).

**На сервере Былин** (`bylins.su`):
```bash
MODE=agent ./start.sh
```
Агент принимает данные от circle на `127.0.0.1:4317` и форвардит на `monitoring.bylins.su:4317`.

**Если circle запущен прямо на monitoring.bylins.su** — агент не нужен.
Circle отправляет на `127.0.0.1:4319` (HTTP, без TLS/auth), который слушает OTEL Collector стека.

## Файлы

```
tools/observability/
├── start.sh                              # Скрипт запуска (все режимы)
├── docker-compose.observability.yml      # Полный стек (all-in-one / monitoring-server)
├── docker-compose.agent.yml              # Только OTEL агент (для bylins.su)
├── docker-compose.tls.yml                # TLS overlay для monitoring-server
├── docker-compose.data-dir.yml           # Bind mounts вместо named volumes
├── otel-collector-config.yaml            # Конфиг коллектора (all-in-one)
├── otel-collector-gateway-config.yaml    # Конфиг gateway (monitoring-server, TLS+auth)
├── otel-collector-agent-config.yaml      # Конфиг агента (форвард на remote)
├── prometheus.yml                        # Prometheus scrape config
├── loki-config.yaml                      # Loki
├── tempo-config.yaml                     # Tempo
├── METRICS.md                            # Список всех метрик
├── DEPLOYMENT.md                         # Пошаговые инструкции
├── grafana/
│   └── provisioning/
│       ├── datasources/datasources.yml
│       └── dashboards/dashboards.yml
└── dashboards/
    ├── operational-dashboard.json
    ├── performance-dashboard.json
    └── business-logic-dashboard.json
```

## Переменные окружения

| Переменная | Режим | Описание | По умолчанию |
|-----------|-------|----------|--------------|
| `MODE` | — | Режим: `all-in-one`, `monitoring-server`, `agent` | `all-in-one` |
| `OTEL_BIND` | `monitoring-server` | IP для внешних портов (4317/4318) | `0.0.0.0` |
| `OTEL_AUTH_TOKEN` | `monitoring-server`, `agent` | Bearer token для аутентификации | — |
| `OTEL_GATEWAY` | `agent` | Хост удалённого OTEL Collector | `monitoring.bylins.su` |
| `DATA_DIR` | `all-in-one`, `monitoring-server` | Путь для bind-mount данных | не задан |

## Сборка сервера с OTEL

Сначала установить opentelemetry-cpp SDK (один раз):

```bash
# Через vcpkg (рекомендуется, устанавливает vcpkg автоматически):
./tools/observability/install-otel-sdk.sh

# Или собрать из исходников (~15 минут, без vcpkg):
./tools/observability/install-otel-sdk.sh --source
```

Затем собрать circle:

```bash
cmake -S . -B build_otel \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_OTEL=ON \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=~/vcpkg/installed/x64-linux
make -C build_otel -j$(($(nproc)/2))
```

## Инструментированные системы

- **Хартбит** — длительность тика и каждого шага, пропущенные пульсы
- **Игроки** — онлайн, в бою, распределение по уровню/реморту, save/load
- **Мобы** — количество активных, длительность AI
- **Зоны** — обновление, сброс
- **Бой** — длительность расчёта удара
- **Магия** — кол-во и длительность кастов по заклинанию/классу
- **DG Scripts** — длительность проверки триггеров по типу (MOB/OBJ/WLD)
- **Аукцион** — количество активных лотов

> Полный список метрик с Prometheus-именами → [METRICS.md](METRICS.md)

## Идентификация сервера

Circle автоматически устанавливает `service.name = bylins-<hostname>-<port>` (например,
`bylins-overseer-4000`). В Prometheus это значение становится лейблом `job`.

Все дашборды в Grafana имеют дропдаун **Service** для фильтрации по серверу.

## Prometheus: запросы для gauge метрик

Gauges хранятся как histograms с суффиксом `_gauge`. Для текущего значения:

```promql
rate(players_online_count_gauge_sum{job=~"bylins-.*"}[5m]) /
rate(players_online_count_gauge_count{job=~"bylins-.*"}[5m])
```
