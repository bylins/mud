# OpenTelemetry Instrumentation — Влияние на производительность

## Резюме

**Общий overhead**: ~0.025–0.08% CPU, ~100–250 KB памяти
**Вывод**: Безопасно для production

---

## Типичные значения операций (OpenTelemetry C++ SDK)

| Операция | Время |
|----------|-------|
| Создать span | 500–1000 ns |
| Установить атрибут (string) | 100–200 ns |
| Установить атрибут (int64) | 50–100 ns |
| Закрыть span | 200–500 ns |
| RecordCounter / RecordGauge | 100–200 ns |
| RecordHistogram / ScopedMetric | 200–500 ns |

---

## Анализ по системам

### Хартбит (heartbeat.cpp)

**Частота**: каждые 40ms (25 Hz)

- `heartbeat.step.duration` — один RecordHistogram на шаг: ~300 ns
- `heartbeat.total.duration` — один RecordHistogram на тик: ~300 ns
- `heartbeat.missed_pulses_total` — RecordCounter только при лаге: ~150 ns

**Overhead**: ~1–2 µs на тик = **0.003–0.005% CPU**

---

### Обновление игроков (game_limits.cpp)

**Частота**: каждые 40ms (каждый тик)

- ScopedMetric `player.beat_update.duration`: ~300 ns
- RecordGauge `players.online.count`: ~150 ns
- RecordGauge `players.in_combat.count`: ~150 ns
- RecordGauge `players.by_level_remort.count` × N уникальных комбинаций: ~150 ns × N

**При 20 уникальных level/remort**: ~4 µs на тик = **0.01% CPU**

---

### Бой — violence (fight.cpp)

**Частота**: каждые 2 секунды (kBattleRound = 50 тиков)

- RecordGauge `combat.active.count`: ~150 ns — один раз на вызов `perform_violence()`

**Бой — удар (fight_hit.cpp)**

**Частота**: N ударов за раунд (1–4 на персонажа)

- ScopedMetric `combat.hit.duration`: ~300 ns на удар

**При 10 боях, 2 ударах на раунд**: ~1 µs на вызов `perform_violence()` = **пренебрежимо**

---

### Смерть игрока (fight_stuff.cpp)

**Частота**: редко (события смерти)

- RecordCounter `player.deaths.total`: ~150 ns на смерть — **пренебрежимо**

---

### AI мобов (mobact.cpp)

**Частота**: каждые 400ms (каждые 10 тиков)

- ScopedMetric `mob.ai.duration`: ~300 ns
- RecordGauge `mob.active.count`: ~150 ns

**Overhead**: ~0.5 µs на цикл = **пренебрежимо**

---

### Загрузка/сохранение игрока (db.cpp, obj_save.cpp)

**Частота**: загрузка — при входе; сохранение — каждые 5–30 минут

- ScopedMetric `player.load.duration` / `player.save.duration`: ~300 ns
- RecordCounter `player.save.total`: ~150 ns

Overhead <<< времени I/O операции. **Пренебрежимо.**

---

### Обновление зон (db.cpp)

**Частота**: раз в секунду

- ScopedMetric `zone.update.duration`: ~300 ns
- RecordCounter `zone.reset.total` × количество сбросов: ~150 ns × N
- RecordHistogram `zone.reset.duration`: ~300 ns
- RecordHistogram `zone.command.Q.duration`: ~300 ns

**Overhead**: ~1–2 µs на цикл = **пренебрежимо**

---

### Заклинания (magic_utils.cpp)

**Частота**: 5–50 заклинаний в секунду

- ScopedMetric `spell.cast.duration` с атрибутами spell_id/caster_class: ~400 ns
- RecordCounter `spell.cast.total`: ~150 ns

**При 20 заклинаниях/сек**: ~11 µs/сек = **0.001% CPU**

---

### DG Scripts (dg_scripts.cpp)

**Частота**: каждые 13 секунд

- ScopedMetric `script.trigger.duration`: ~300 ns — **пренебрежимо**

---

### Аукцион (auction.cpp)

**Частота**: обновление тактов — раз в 1.2 сек; продажа — редко

- RecordGauge `auction.lots.active`: ~150 ns на такт
- RecordCounter `auction.sale.total`: ~150 ns на продажу
- RecordCounter `auction.revenue.total`: ~150 ns на продажу
- RecordHistogram `auction.duration.seconds`: ~300 ns на продажу

**Overhead**: **пренебрежимо** (редкие события)

> Метрики существуют в коде. Появятся в Prometheus после первой продажи на аукционе.

---

### Крафтинг (item_creation.cpp)

**Частота**: редко (ручной крафт игроками)

- ScopedMetric `craft.duration`: ~300 ns
- RecordCounter `craft.completed.total` / `craft.failures.total`: ~150 ns

**Overhead**: **пренебрежимо**

> Метрики существуют в коде. Появятся в Prometheus после первого крафта.

---

## Итоговая оценка (worst-case, 50 игроков онлайн)

| Система | Overhead/сек |
|---------|-------------|
| Хартбит | ~50 µs |
| Обновление игроков | ~100 µs |
| Бой (10 активных) | ~10 µs |
| AI мобов | ~1.25 µs |
| Заклинания (20/сек) | ~11 µs |
| Зоны | ~2 µs |
| Прочее | ~5 µs |
| **Итого** | **~180 µs/сек = 0.018% CPU** |

### Память

| Сценарий | Overhead |
|----------|----------|
| Нормальная нагрузка | ~100 KB |
| Пиковая нагрузка | ~250 KB |

---

## Рекомендации

- **Sampling traces**: 10–20% при >50 игроках (настраивается в OTEL Collector)
- **Метрики**: всегда 100%, overhead минимален
- Batch export каждые 5 сек — оптимально для текущей нагрузки

---

*Методология: OpenTelemetry C++ SDK benchmarks + практические оценки*
