# Bylins MUD - OpenTelemetry Observability

Полная документация по системе наблюдаемости (observability) для Bylins MUD.

## Структура документации

```
tools/observability/
├── README.md                       # Этот файл
├── OTEL_INSTRUMENTATION.md         # Описание инструментации
├── PERFORMANCE_IMPACT.md           # Анализ влияния на производительность
├── DEPLOYMENT_GUIDE.md             # Руководство по развёртыванию стека
└── dashboards/                     # Grafana дашборды
    ├── performance-dashboard.json
    ├── business-logic-dashboard.json
    └── operational-dashboard.json
```

## Быстрый старт

### 1. Что инструментировано?

**9 критических систем**:
- ✅ Combat system (с overlapping traces!)
- ✅ Mobile AI
- ✅ Player save/load
- ✅ Beat points update + Player statistics
- ✅ Zone updates
- ✅ Magic/Spell system
- ✅ Script triggers
- ✅ Auction system
- ✅ Crafting system

**30+ метрик, 10+ типов трейсов, 3 Grafana дашборда**

Подробности → [OTEL_INSTRUMENTATION.md](OTEL_INSTRUMENTATION.md)

### 2. Влияние на производительность?

**TL;DR**: ~0.025-0.08% CPU overhead, ~100-250 KB памяти

**Безопасно для production** ✅

Детальный анализ → [PERFORMANCE_IMPACT.md](PERFORMANCE_IMPACT.md)

### 3. Как развернуть телеметрию?

**Stack**: OTEL Collector → Prometheus/Loki/Tempo → Grafana

**Время setup**: ~15 минут с Docker Compose

Пошаговое руководство → [DEPLOYMENT_GUIDE.md](DEPLOYMENT_GUIDE.md)

## Основные возможности

### Overlapping Combat Traces 🌟

Уникальная архитектура для анализа боёв:
- **Heartbeat trace** - системный view (как бой влияет на performance)
- **Combat trace** - бизнес view (весь бой от начала до конца)
- **Baggage propagation** - автоматическая корреляция логов через `combat_trace_id`

```
Heartbeat #100 (40ms)                Combat: alice vs bob (30s)
  ├─ Combat Processing               ├─ Round #1 (heartbeat=100)
  ├─ Mobile AI                       ├─ Round #2 (heartbeat=102)
  └─ Beat Points Update              └─ Round #N (heartbeat=104)
```

### Metrics по типам

**Counters** (события):
- `combat.rounds.total`, `combat.hits.total`
- `spell.cast.total`, `craft.completed.total`
- `auction.sale.total`, `zone.reset.total`

**Gauges** (состояние):
- `players.online.count`, `players.in_combat.count`
- `combat.active.count`, `mob.active.count`
- `auction.lots.active`

**Histograms** (латентности):
- `combat.round.duration`, `spell.cast.duration`
- `player.save.duration`, `zone.update.duration`
- `craft.duration`, `mob.ai.duration`

### Grafana Dashboards

**3 готовых дашборда** с 27+ панелями:

1. **Performance Dashboard** - латентности всех систем
2. **Business Logic Dashboard** - спеллы, крафт, аукцион
3. **Operational Dashboard** - игроки онлайн, распределение

Дашборды → [dashboards/](dashboards/)

## Примеры использования

### Prometheus (метрики)

```promql
# Средняя длительность раунда боя
rate(combat_round_duration_sum[5m]) / rate(combat_round_duration_count[5m])

# Top 10 популярных заклинаний
topk(10, rate(spell_cast_total[5m]))

# Игроки онлайн
players_online_count
```

### Tempo (traces)

```
# Найти долгие бои (> 1 минута)
{ rootSpanName = "Combat:" && traceDuration > 1m }

# Найти медленные заклинания (> 50ms)
{ span.name = "Spell Cast" && duration > 50ms }

# PvP бои
{ rootSpanName = "Combat:" && span.is_pk = true }
```

### Loki (логи)

```logql
# Все логи конкретного боя
{service="mud"} | logfmt | combat_trace_id = "xyz123"

# Критические попадания
{service="mud"} | logfmt | hit_type = "critical"

# Ошибки при сохранении
{service="mud"} | logfmt | level = "ERROR" | message =~ "save"
```

## Навигация по документам

### Для разработчиков

1. Начните с [OTEL_INSTRUMENTATION.md](OTEL_INSTRUMENTATION.md)
   - Что инструментировано
   - Архитектура overlapping traces
   - Примеры запросов

2. Изучите overhead в [PERFORMANCE_IMPACT.md](PERFORMANCE_IMPACT.md)
   - Детальный анализ по системам
   - Рекомендации по оптимизации
   - Worst-case сценарии

### Для DevOps/SRE

1. Следуйте [DEPLOYMENT_GUIDE.md](DEPLOYMENT_GUIDE.md)
   - Docker Compose setup (15 минут)
   - Конфигурация всех компонентов
   - Production best practices
   - Troubleshooting

2. Импортируйте [dashboards/](dashboards/) в Grafana
   - Автоматический import через provisioning
   - Или вручную через UI

### Для аналитиков

1. Откройте Grafana дашборды
   - **Performance** - для анализа lag
   - **Business Logic** - для game balance
   - **Operational** - для player activity

2. Используйте Explore для ad-hoc запросов
   - Prometheus для метрик
   - Tempo для traces
   - Loki для логов

## Архитектура

```
┌─────────────┐
│ Bylins MUD  │ (OTEL C++ SDK)
└──────┬──────┘
       │ OTLP/gRPC
       │
┌──────▼──────────────┐
│ OTEL Collector      │ (батчинг, sampling, routing)
└──────┬──────────────┘
       │
       ├─────────────┬─────────────┐
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

## Коммиты

Инструментация добавлена в ветке `metrics-traces-instrumentation` (12 коммитов):

```
a9f46ce9c - Add comprehensive OpenTelemetry instrumentation documentation
3249eb074 - Add Grafana dashboards for OpenTelemetry observability
c6a80f693 - Add OpenTelemetry instrumentation to Crafting system
d237880cb - Add OpenTelemetry instrumentation to Auction system
2aeda6e24 - Add OpenTelemetry instrumentation to DG Script Trigger system
eea6505ac - Add OpenTelemetry instrumentation to Magic/Spell system
5b4dcdaf4 - Add OpenTelemetry instrumentation to Zone Update system
ed4774fc6 - Add OpenTelemetry instrumentation to Beat Points Update + Player Statistics
9b87d08e3 - Add OpenTelemetry instrumentation to Player save/load system
57ad6c319 - Add OpenTelemetry instrumentation to Mobile AI system
da1a55940 - Add OpenTelemetry instrumentation to combat system
a58c7f0fa - Add RAII helper classes for OpenTelemetry instrumentation
```

## FAQ

**Q: Насколько это замедлит сервер?**
A: ~0.025-0.08% CPU overhead. Пренебрежимо мало. См. [PERFORMANCE_IMPACT.md](PERFORMANCE_IMPACT.md)

**Q: Нужно ли sampling для production?**
A: Рекомендуется 10-20% sampling для traces при >50 игроках. Метрики - всегда 100%.

**Q: Как найти конкретный бой в Tempo?**
A: Через baggage `combat_trace_id` из логов, или через TraceQL: `{ rootSpanName = "Combat:" && span.attacker = "alice" }`

**Q: Можно ли отключить инструментацию?**
A: Да, через environment variables или feature flags в коде.

**Q: Как коррелировать логи и traces?**
A: Автоматически! Логи содержат `trace_id` и `combat_trace_id`. В Grafana есть кнопки для перехода.

**Q: Сколько места займут метрики/traces/логи?**
A: При настройках retention 30 дней: ~10-50 GB (зависит от активности). Настраивается в конфигах.

## Поддержка

Вопросы по инструментации? Проблемы с развёртыванием?

1. Проверьте [DEPLOYMENT_GUIDE.md](DEPLOYMENT_GUIDE.md) → Troubleshooting
2. Проверьте логи: `docker logs mud-otel-collector`
3. Проверьте метрики коллектора: `curl http://localhost:8888/metrics`

## Следующие шаги

После развёртывания:

1. ✅ Импортировать дашборды в Grafana
2. ✅ Настроить алерты на критичные метрики
3. ✅ Протестировать на нагрузке
4. ⚠️  Включить sampling (10-20%) для production
5. ⚠️  Настроить backups Grafana dashboards

## Ссылки

- [OpenTelemetry Docs](https://opentelemetry.io/docs/)
- [OTEL Collector Docs](https://opentelemetry.io/docs/collector/)
- [Grafana Docs](https://grafana.com/docs/)
- [Prometheus Docs](https://prometheus.io/docs/)
- [Tempo Docs](https://grafana.com/docs/tempo/)
- [Loki Docs](https://grafana.com/docs/loki/)

---
*Дата: 2026-01-28*
*Bylins MUD OpenTelemetry Instrumentation v1.0*
