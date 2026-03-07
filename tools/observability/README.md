# Bylins MUD — OpenTelemetry Observability

Стек мониторинга: **OTEL Collector → Prometheus / Loki / Tempo → Grafana**

## Файлы

```
tools/observability/
├── docker-compose.observability.yml  # Docker Compose стек
├── otel-collector-config.yaml        # OTEL Collector
├── prometheus.yml                    # Prometheus scrape config
├── loki-config.yaml                  # Loki (логи)
├── tempo-config.yaml                 # Tempo (трейсы)
├── METRICS.md                        # Список всех метрик
├── grafana/
│   └── provisioning/
│       ├── datasources/datasources.yml
│       └── dashboards/dashboards.yml
└── dashboards/
    ├── operational-dashboard.json    # Игроки, активность
    ├── performance-dashboard.json    # Хартбит, мобы, I/O
    └── business-logic-dashboard.json # Заклинания, скрипты, аукцион
```

## Быстрый старт

```bash
cd tools/observability
docker-compose -f docker-compose.observability.yml up -d
```

Grafana: http://localhost:12000 (admin / admin123)

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

## Сборка сервера с OTEL

```bash
cmake -S . -B build_otel \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_OTEL=ON \
  -DCMAKE_TOOLCHAIN_FILE=~/repos/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=~/repos/vcpkg/installed/x64-linux
make -C build_otel -j$(($(nproc)/2))
```

## Архитектура

```
Bylins MUD (OTEL C++ SDK)
    │ OTLP/gRPC :4317
    ▼
OTEL Collector
    ├── Prometheus (метрики)  :9090
    ├── Tempo      (трейсы)   :3200
    └── Loki       (логи)     :3100
                   ▼
              Grafana          :12000
```

## Prometheus: запросы для gauge метрик

Gauges хранятся как histograms с суффиксом `_gauge`. Для текущего значения:

```promql
rate(players_online_count_gauge_sum[5m]) / rate(players_online_count_gauge_count[5m])
```
