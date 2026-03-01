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
| `players_by_level_remort_count` | gauge | `level` (1–60), `remort` (0–9) | Распределение по уровню и реморту |
| `player_beat_update_duration` | histogram | — | Длительность обновления HP/Mana/Move за тик |
| `player_load_duration` | histogram | — | Длительность загрузки персонажа при входе в игру |
| `player_save_duration` | histogram | `save_type` (`frac`/`full`), `character` (имя персонажа) | Длительность сохранения |
| `player_save_total` | counter | `save_type`, `character` | Количество сохранений |
| `player_deaths_total` | counter | `death_type` (`pvp`/`pve`/`other`) | Смерти игроков по типу |

Prometheus-имена gauges: `players_online_count_gauge_{bucket,sum,count}` и т.д.

**Расшифровка атрибутов:**
- `death_type=pvp` — убийство другим игроком (не чармис)
- `death_type=pve` — убийство мобом или чармисом
- `death_type=other` — смерть без явного убийцы (ловушки, среда и т.п.)

---

## Хартбит

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `heartbeat_step_duration` | histogram | `step` (название шага) | Длительность каждого шага хартбита |
| `heartbeat_total_duration` | histogram | — | Длительность полного тика (40ms норма) |
| `heartbeat_missed_pulses_total` | counter | — | Пропущенные пульсы (лаг сервера) |

---

## Мобы

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `mob_active_count` | gauge | — | Активные (агрессивные) мобы в текущем цикле AI |
| `mob_activity_duration` | histogram | `activity_level` | Длительность цикла AI всех мобов |

Prometheus: `mob_active_count_gauge_{bucket,sum,count}`

**Расшифровка атрибутов:**
- `activity_level` — уровень активности, переданный в `mobile_activity()` (числовой приоритет обработки)

---

## Зоны

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `zone_update_duration` | histogram | — | Длительность цикла обновления всех зон |
| `zone_reset_duration` | histogram | `zone` (название зоны) | Длительность сброса одной зоны |
| `zone_reset_total` | counter | `zone` (название зоны) | Количество сбросов зон |
| `zone_command_Q_duration` | histogram | `zone`, `rnum` (rnum моба) | Длительность обработки команды Q (извлечение мобов) |

**Расшифровка атрибутов:**
- `zone` — человекочитаемое название зоны (напр., `"Мидгаард"`)
- `rnum` — внутренний номер прототипа моба (rnum, не vnum); используется для отладки команды Q

---

## Бой

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `combat_active_count` | gauge | — | Количество активных комбатантов (обновляется раз в 2 сек) |
| `combat_hit_duration` | histogram | — | Длительность расчёта одного удара |

Prometheus: `combat_active_count_gauge_{bucket,sum,count}`

---

## Магия

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `spell_cast_duration` | histogram | `spell_name`, `caster_class` | Длительность выполнения заклинания |
| `spell_cast_total` | counter | `spell_name`, `caster_class` | Количество кастов |

**Расшифровка атрибутов:**
- `spell_name` — русское название заклинания (UTF-8), напр., `"магический снаряд"`
- `caster_class` — русское название класса кастера (UTF-8), напр., `"волшебник"`, `"колдун"`

---

## Крафтинг

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `craft_failures_total` | counter | `recipe_name`, `skill`, `failure_reason` | Неудачные попытки крафта |
| `craft_completed_total` | counter | `recipe_name`, `skill` | Успешные крафты |

> Метрики появятся в Prometheus после первого крафта.

**Расшифровка атрибутов:**
- `recipe_name` — русское название создаваемого предмета (UTF-8), напр., `"длинный меч"`. Если прототип не найден — числовой vnum.
- `skill` — русское название умения (UTF-8), напр., `"кузнечное дело"`, `"создание луков"`
- `failure_reason` — причина неудачи; сейчас всегда `craft_failed`

---

## Скрипты

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `script_trigger_duration` | histogram | `trigger_type` (`MOB`/`OBJ`/`WLD`) | Длительность проверки триггеров за цикл |

---

## Аукцион

| Метрика | Тип | Атрибуты | Описание |
|---------|-----|----------|----------|
| `auction_lots_active` | gauge | — | Количество активных лотов |
| `auction_sale_total` | counter | — | Количество завершённых продаж |
| `auction_revenue_total` | counter | — | Суммарная выручка (в кунах) |
| `auction_duration_seconds` | histogram | — | Время от выставления лота до продажи |

Prometheus: `auction_lots_active_gauge_{bucket,sum,count}`

> Детали по конкретным сделкам (продавец, покупатель, предмет, стоимость) доступны через трейсы (спан `"Auction Sale"`).

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

# p95 длительности обновления зон
histogram_quantile(0.95, rate(zone_update_duration_bucket[5m]))

# Топ заклинаний по частоте
topk(10, sum by (spell_name) (rate(spell_cast_total[5m])))

# Топ классов по количеству кастов
sum by (caster_class) (rate(spell_cast_total[5m]))

# Пропуски пульсов за последний час
increase(heartbeat_missed_pulses_total[1h])

# Активные мобы (текущее)
rate(mob_active_count_gauge_sum[5m]) / rate(mob_active_count_gauge_count[5m])

# Активные комбатанты
rate(combat_active_count_gauge_sum[5m]) / rate(combat_active_count_gauge_count[5m])

# Смерти игроков по типу за последний час
sum by (death_type) (increase(player_deaths_total[1h]))

# Топ создаваемых предметов
topk(10, sum by (recipe_name) (rate(craft_completed_total[5m])))

# Доля неудач крафта по умению
sum by (skill) (rate(craft_failures_total[5m]))
  / sum by (skill) (rate(craft_completed_total[5m]) + rate(craft_failures_total[5m]))

# Топ зон по частоте сбросов
topk(10, sum by (zone) (rate(zone_reset_total[5m])))
```
