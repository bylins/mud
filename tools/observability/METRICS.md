# Bylins MUD — Метрики OpenTelemetry

Список всех метрик, реально отправляемых в Prometheus.

## Соглашения

**Gauges** хранятся как histograms с суффиксом `_gauge`:

```promql
# Текущее значение gauge:
rate(metric_name_gauge_sum[5m]) / rate(metric_name_gauge_count[5m])
```

**Histograms** — перцентили:

```promql
histogram_quantile(0.99, rate(metric_name_bucket[5m]))
```

**Counters** — скорость событий:

```promql
rate(metric_name_total[5m])
```

---

## Игроки

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `players_online_count` | gauge | — | Игроков онлайн |
| `players_in_combat_count` | gauge | — | Игроков в бою |
| `players_by_level_remort_count` | gauge | `level`, `remort` | Распределение по уровню/реморту |
| `player_beat_update_duration` | histogram | — | Длительность обновления HP/Mana/Move за тик |
| `player_load_duration` | histogram | — | Длительность загрузки персонажа |
| `player_save_duration` | histogram | `save_type` (`frac`/`full`) | Длительность сохранения |
| `player_save_total` | counter | `save_type` | Количество сохранений |

Prometheus-имена gauges: `players_online_count_gauge_{bucket,sum,count}` и т.д.

---

## Хартбит

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `heartbeat_step_duration` | histogram | `step` | Длительность каждого шага хартбита |
| `heartbeat_total_duration` | histogram | — | Длительность полного тика |
| `heartbeat_missed_pulses_total` | counter | — | Пропущенные пульсы (лаг сервера) |

---

## Мобы

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `mob_active_count` | gauge | — | Активные (агрессивные) мобы |
| `mob_ai_duration` | histogram | — | Длительность цикла AI всех мобов |

Prometheus: `mob_active_count_gauge_{bucket,sum,count}`

---

## Зоны

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `zone_update_duration` | histogram | — | Длительность цикла обновления зон |
| `zone_reset_duration` | histogram | — | Длительность сброса одной зоны |
| `zone_reset_total` | counter | `zone_vnum`, `reset_mode` | Количество сбросов зон |
| `zone_command_Q_duration` | histogram | — | Длительность обработки очереди команд зоны |

---

## Бой

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `combat_hit_duration` | histogram | — | Длительность расчёта одного удара |

> `combat_active_count`, `combat_round_duration`, `combat_rounds_total` — в Prometheus **отсутствуют**.

---

## Магия

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `spell_cast_duration` | histogram | `spell_id`, `caster_class` | Длительность выполнения заклинания |
| `spell_cast_total` | counter | `spell_id`, `caster_class` | Количество кастов |

---

## Скрипты

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `script_trigger_duration` | histogram | `trigger_type` (`MOB`/`OBJ`/`WLD`) | Длительность проверки триггеров за цикл |

---

## Аукцион

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `auction_lots_active` | gauge | — | Активные лоты |

Prometheus: `auction_lots_active_gauge_{bucket,sum,count}`

> `auction_sale_total`, `auction_revenue_total`, `auction_duration_seconds` — в Prometheus **отсутствуют**.

---

## Span Metrics (генерируются Tempo)

| Метрика | Описание |
|---------|----------|
| `traces_spanmetrics_calls_total` | Количество вызовов по имени спана |
| `traces_spanmetrics_latency_{bucket,sum,count}` | Латентность вызовов |
| `traces_spanmetrics_size_total` | Размер спанов |
| `traces_target_info` | Информация о сервисе |

Атрибуты: `service`, `span_name`, `status_code`

---

## Примеры запросов

```promql
# Игроки онлайн (текущее)
rate(players_online_count_gauge_sum[5m]) / rate(players_online_count_gauge_count[5m])

# p99 тика хартбита
histogram_quantile(0.99, rate(heartbeat_total_duration_bucket[5m]))

# Длительность шагов хартбита p95 (по шагам)
histogram_quantile(0.95, sum by (step) (rate(heartbeat_step_duration_bucket[5m])))

# p95 длительности зоны
histogram_quantile(0.95, rate(zone_update_duration_bucket[5m]))

# Топ заклинаний по частоте
topk(10, sum by (spell_id) (rate(spell_cast_total[5m])))

# Пропуски пульсов за последний час
increase(heartbeat_missed_pulses_total[1h])

# Активные мобы (текущее)
rate(mob_active_count_gauge_sum[5m]) / rate(mob_active_count_gauge_count[5m])
```
