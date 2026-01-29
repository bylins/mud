# OpenTelemetry Instrumentation - Анализ влияния на производительность

## Резюме

**Общий overhead**: ~0.5-2% CPU, ~1-5MB памяти на систему
**Рекомендация**: Безопасно для production при правильной настройке sampling

## Методология измерений

Производительность OpenTelemetry зависит от:
1. **Тип операции**: создание span, запись метрики, установка атрибута
2. **Частота вызовов**: редкие vs частые операции (combat rounds каждые 80ms)
3. **Конфигурация экспорта**: batch vs streaming, sampling rate
4. **Сеть**: latency до OTEL Collector

### Типичные значения (на базе OpenTelemetry C++ SDK benchmarks)

| Операция | Время (наносекунды) | Аллокации памяти |
|----------|---------------------|------------------|
| Создать span (no-op) | 50-100 ns | 0 байт |
| Создать span (active) | 500-1000 ns | 256 байт |
| Установить атрибут string | 100-200 ns | ~100 байт (копия строки) |
| Установить атрибут int64 | 50-100 ns | 0 байт |
| Закрыть span | 200-500 ns | 0 байт |
| Записать Counter | 100-200 ns | 0 байт |
| Записать Histogram | 200-500 ns | 0 байт |
| Записать Gauge | 100-200 ns | 0 байт |

**Примечание**: 1 микросекунда (μs) = 1000 наносекунд (ns)

## Детальный анализ по системам

### Phase 1: Performance-critical системы

#### 1.1 Combat System

**Частота**: Каждые 80ms (12.5 раз в секунду) при активном бое

##### Per-round overhead (каждый раунд боя)

**Heartbeat span** (создаётся один раз для всех боёв):
```cpp
auto span = StartSpan("Combat Processing");  // ~500 ns
```
- **Overhead**: 500 ns один раз на все бои в heartbeat

**DualSpan для каждого раунда**:
```cpp
DualSpan round_span(...);                    // ~1500 ns (2 spans)
round_span.SetAttribute("round_number", ...);  // ~50 ns
round_span.SetAttribute("heartbeat_pulse", ...); // ~50 ns
round_span.SetAttribute("attacker", ...);     // ~150 ns (string copy)
round_span.SetAttribute("defender", ...);     // ~150 ns (string copy)
round_span.SetAttribute("attacker_hp", ...);  // ~50 ns
round_span.SetAttribute("defender_hp", ...);  // ~50 ns
// Автоматическое закрытие spans                // ~500 ns (2 spans)
```
- **Overhead**: ~2.5 μs на раунд (2 spans + 6 атрибутов)

**ScopedMetric**:
```cpp
ScopedMetric metric("combat.round.duration", attrs); // ~300 ns
// ... раунд боя ...
// Автоматическая запись histogram               // ~500 ns
```
- **Overhead**: ~800 ns на раунд

**Counters**:
```cpp
RecordCounter("combat.rounds.total", 1);       // ~150 ns
RecordGauge("combat.active.count", count);     // ~150 ns
```
- **Overhead**: ~300 ns на раунд

**Итого на один раунд боя**: ~3.6 μs = 0.0036 ms

**При длительности раунда** ~2-5 ms:
- **Relative overhead**: 0.0036 / 2 = **0.18%** (best case)
- **Relative overhead**: 0.0036 / 5 = **0.072%** (worst case)

##### Per-hit overhead (каждая атака в раунде)

```cpp
ScopedMetric hit_metric("combat.hit.duration"); // ~300 ns
// ... расчёт удара ...
// Автоматическая запись                        // ~500 ns
```
- **Overhead**: ~800 ns на атаку

**При длительности hit** ~0.3-1 ms:
- **Relative overhead**: 0.0008 / 0.3 = **0.27%** (best case)
- **Relative overhead**: 0.0008 / 1.0 = **0.08%** (worst case)

##### Combat trace lifecycle overhead

**При начале боя** (SetFighting):
```cpp
auto combat_id = GenerateCombatId(...);        // ~500 ns (timestamp + strings)
auto span = StartSpan("Combat: alice vs bob");  // ~500 ns (no Scope!)
span->SetAttribute(...) * 6;                   // ~600 ns
BaggageScope("combat_trace_id", ...);          // ~1000 ns
BaggageScope("combat_id", ...);                // ~1000 ns
RecordGauge("combat.active.count", ...);       // ~150 ns
```
- **Overhead**: ~3.75 μs **один раз** при начале боя

**При окончании боя** (stop_fighting):
```cpp
span->SetAttribute("end_pulse", ...);          // ~50 ns
span->AddEvent("combat_ended");                // ~300 ns
span->End();                                   // ~250 ns
RecordGauge("combat.active.count", ...);       // ~150 ns
// Багgage scopes автоматически очищаются       // ~200 ns
```
- **Overhead**: ~950 ns **один раз** при окончании боя

**Вывод для Combat**:
- Overhead на раунд: **0.07-0.18%** - пренебрежимо мало
- Overhead на атаку: **0.08-0.27%** - пренебрежимо мало
- Overhead на бой (start/stop): **~4.7 μs** - один раз, несущественно

#### 1.2 Mobile AI

**Частота**: Каждые 10 pulses (400ms)

```cpp
auto span = StartSpan("Mobile Activity");      // ~500 ns
ScopedMetric metric("mob.ai.duration", ...);   // ~300 ns
// ... обработка AI для всех мобов (~5-15ms) ...
RecordGauge("mob.active.count", count);        // ~150 ns
// Автоматическое закрытие                      // ~750 ns
```
- **Overhead**: ~1.7 μs на цикл AI

**При длительности AI** ~5-15 ms:
- **Relative overhead**: 0.0017 / 5 = **0.034%** (best case)
- **Relative overhead**: 0.0017 / 15 = **0.011%** (worst case)

#### 1.3 Player Save/Load

**Частота**: Save - каждые 5 минут (frac) или 30 минут (full), Load - при входе игрока

##### Load overhead
```cpp
auto span = StartSpan("Load Player");          // ~500 ns
span->SetAttribute("character_name", ...);     // ~150 ns
ScopedMetric metric("player.load.duration");   // ~300 ns
// ... загрузка персонажа (~10-100ms) ...
// Автоматическое закрытие                      // ~750 ns
```
- **Overhead**: ~1.7 μs на загрузку

**При длительности load** ~10-100 ms:
- **Relative overhead**: 0.0017 / 10 = **0.017%** (best case)
- **Relative overhead**: 0.0017 / 100 = **0.0017%** (worst case)

##### Save overhead (per player)
```cpp
auto span = StartSpan("Player Save (...)");    // ~500 ns
span->SetAttribute("save_type", ...);          // ~100 ns
span->SetAttribute("character", ...);          // ~150 ns
// ... сохранение (~5-50ms) ...
RecordHistogram("player.save.duration", ...);  // ~500 ns
RecordCounter("player.save.total", 1, ...);    // ~150 ns
```
- **Overhead**: ~1.4 μs на сохранение

**Overhead пренебрежимо мал** по сравнению с I/O операциями.

#### 1.4 Beat Points Update

**Частота**: Каждый pulse (~40ms) для всех игроков

```cpp
auto span = StartSpan("Beat Points Update");   // ~500 ns
ScopedMetric metric("player.beat_update.duration"); // ~300 ns
// ... обновление HP/Mana/Move для всех (~3-8ms) ...
span->SetAttribute("player_count", ...);       // ~50 ns
RecordGauge("players.online.count", ...);      // ~150 ns
RecordGauge("players.in_combat.count", ...);   // ~150 ns
// Цикл по level_remort_distribution (~10-50 итераций):
//   RecordGauge * N                             // ~150 ns * N
// Автоматическое закрытие                      // ~750 ns
```
- **Overhead**: ~2.0 μs + (0.15 μs * уникальных level/remort)

**При 20 уникальных level/remort комбинациях**: ~5.0 μs
**При длительности beat update** ~3-8 ms:
- **Relative overhead**: 0.005 / 3 = **0.17%** (best case)
- **Relative overhead**: 0.005 / 8 = **0.063%** (worst case)

#### 1.5 Zone Updates

**Частота**: Раз в секунду (1000ms)

```cpp
auto span = StartSpan("Zone Update");          // ~500 ns
ScopedMetric metric("zone.update.duration");   // ~300 ns
// Цикл по зонам (обычно 1-5 зон сбрасываются):
//   RecordCounter("zone.reset.total", ...)     // ~150 ns * N
// ... обработка зон (~10-100ms) ...
// Автоматическое закрытие                      // ~750 ns
```
- **Overhead**: ~1.55 μs + (0.15 μs * количество зон)

**При 5 зонах**: ~2.3 μs
**Overhead пренебрежимо мал** по сравнению с zone processing.

### Phase 2: Business Logic системы

#### 2.1 Magic/Spell System

**Частота**: Зависит от игровой активности (10-100 заклинаний в секунду)

```cpp
auto span = StartSpan("Spell Cast");           // ~500 ns
ScopedMetric metric("spell.cast.duration");    // ~300 ns
span->SetAttribute("spell_id", ...);           // ~50 ns
span->SetAttribute("spell_name", ...);         // ~150 ns (string copy)
span->SetAttribute("caster_class", ...);       // ~100 ns (string copy)
span->SetAttribute("spell_level", ...);        // ~50 ns
span->SetAttribute("target_type", ...);        // ~100 ns (string copy)
// ... выполнение заклинания (~0.5-5ms) ...
RecordCounter("spell.cast.total", 1, ...);     // ~150 ns
// Автоматическое закрытие                      // ~750 ns
```
- **Overhead**: ~2.15 μs на заклинание

**При длительности spell** ~0.5-5 ms:
- **Relative overhead**: 0.00215 / 0.5 = **0.43%** (best case)
- **Relative overhead**: 0.00215 / 5 = **0.043%** (worst case)

#### 2.2 Script Triggers

**Частота**: Каждые 13 секунд

```cpp
auto span = StartSpan("Script Trigger Check"); // ~500 ns
ScopedMetric metric("script.trigger.duration", ...); // ~300 ns
span->SetAttribute("trigger_type", ...);       // ~100 ns
span->SetAttribute("mode", ...);               // ~50 ns
// ... проверка триггеров (~5-20ms) ...
// Автоматическое закрытие                      // ~750 ns
```
- **Overhead**: ~1.7 μs на проверку

**Overhead пренебрежимо мал**.

#### 2.3 Auction System

**Частота**: tact_auction() каждые 30 pulses (1.2 сек), sell_auction() - редко

##### tact_auction overhead
```cpp
// Цикл по активным лотам (обычно 0-3):
//   RecordGauge("auction.lots.active", ...)    // ~150 ns
```
- **Overhead**: ~150 ns (выполняется раз в 1.2 сек)

##### sell_auction overhead
```cpp
auto span = StartSpan("Auction Sale");         // ~500 ns
double duration = ...;                         // ~50 ns (арифметика)
span->SetAttribute(...) * 6;                   // ~600 ns
// ... выполнение продажи (~2-10ms) ...
RecordCounter("auction.sale.total", ...);      // ~150 ns
RecordCounter("auction.revenue.total", ...);   // ~150 ns
RecordHistogram("auction.duration.seconds", ...); // ~500 ns
// Автоматическое закрытие                      // ~250 ns
```
- **Overhead**: ~2.2 μs на продажу

**Overhead пренебрежимо мал** (продажи редкие).

#### 2.4 Crafting System

**Частота**: Редко (игроки крафтят вручную, ~1-10 раз в минуту на сервер)

```cpp
auto span = StartSpan("Craft Item");           // ~500 ns
ScopedMetric metric("craft.duration");         // ~300 ns
span->SetAttribute("recipe_id", ...);          // ~50 ns
span->SetAttribute("recipe_name", ...);        // ~150 ns
span->SetAttribute("skill", ...);              // ~100 ns
span->SetAttribute("materials_count", ...);    // ~50 ns
// ... крафтинг (~10-100ms) ...
RecordCounter("craft.completed.total", ...);   // ~150 ns
// ИЛИ
RecordCounter("craft.failures.total", ...);    // ~150 ns
// Автоматическое закрытие                      // ~750 ns
```
- **Overhead**: ~2.2 μs на крафт

**Overhead пренебрежимо мал** (крафтинг редкий и долгий).

## Overhead по типам операций

### Spans

| Тип span | Overhead (создание) | Overhead (закрытие) | Атрибуты | Итого |
|----------|---------------------|---------------------|----------|-------|
| Simple span | 500 ns | 250 ns | ~400 ns (4-6 attrs) | ~1.15 μs |
| DualSpan (2 spans) | 1000 ns | 500 ns | ~800 ns (duplicate) | ~2.3 μs |
| Span with many attrs | 500 ns | 250 ns | ~1500 ns (10-15 attrs) | ~2.25 μs |

### Metrics

| Тип метрики | Overhead | Частота | Impact |
|-------------|----------|---------|--------|
| Counter (без attrs) | 100 ns | Частая | Низкий |
| Counter (с attrs) | 150 ns | Частая | Низкий |
| Gauge | 150 ns | Средняя | Низкий |
| Histogram | 500 ns | Частая | Средний |

### Baggage

| Операция | Overhead |
|----------|----------|
| SetBaggage (context) | 1000 ns |
| GetBaggage (context) | 200 ns |
| Propagation to logs | 500 ns (per log) |

## Aggregated overhead оценка

### Worst-case scenario (пик нагрузки)

**Предположения**:
- 50 игроков онлайн
- 10 активных боёв (20 игроков в бою)
- 5 мобов на игрока (250 активных мобов)
- 20 заклинаний в секунду
- 2 крафта в минуту
- 1 продажа на аукционе в минуту

**Per-second overhead**:
- Combat rounds: 10 боёв * 12.5 раундов/сек * 3.6 μs = **450 μs/sec**
- Combat hits: 10 боёв * 12.5 раундов/сек * 2 удара * 0.8 μs = **200 μs/sec**
- Beat points: 25 раз/сек * 5 μs = **125 μs/sec**
- Mob AI: 2.5 раз/сек * 1.7 μs = **4.25 μs/sec**
- Zone update: 1 раз/сек * 2.3 μs = **2.3 μs/sec**
- Spells: 20 раз/сек * 2.15 μs = **43 μs/sec**
- Script triggers: 0.077 раз/сек * 1.7 μs = **0.13 μs/sec**
- Auction tact: 0.83 раз/сек * 0.15 μs = **0.12 μs/sec**
- Crafting: 0.033 раз/сек * 2.2 μs = **0.07 μs/sec**

**ИТОГО**: ~825 μs/sec = **0.825 ms/sec** = **0.0825% CPU**

### Базовая нагрузка (нормальная активность)

**Предположения**:
- 20 игроков онлайн
- 3 активных боя
- 100 активных мобов
- 5 заклинаний в секунду

**Per-second overhead**: ~250 μs/sec = **0.25 ms/sec** = **0.025% CPU**

## Memory overhead

### Per-span memory

**Active span** (пока не закрыт):
- Span object: ~256 байт
- Attributes (средний): ~500 байт (строки копируются)
- **Итого**: ~756 байт на активный span

**После закрытия** (до export):
- Span в batch buffer: ~400 байт (сжато)
- Хранится до batch export (~1-5 секунд)

### Per-metric memory

**Histogram**:
- Buckets: ~2KB (default buckets)
- Rolling window: minimal (OTLP histogram)

**Counter/Gauge**:
- ~128 байт на метрику

### Estimated total memory

**Worst-case** (50 игроков, 10 боёв):
- Active spans (10 combats * 10 rounds buffered): ~75 KB
- Metrics state: ~50 KB
- Baggage contexts: ~20 KB
- Export buffers: ~100 KB
- **Итого**: ~**245 KB**

**Normal** (20 игроков, 3 боя):
- **Итого**: ~**100 KB**

## Network overhead

### Batch export размеры

**OTLP/gRPC** (рекомендуется):
- Traces: ~1-2 KB на span (после сжатия)
- Metrics: ~100-500 байт на метрику
- Batch size: обычно 512-1024 spans или 1000 metrics

**Примеры**:
- 10 боёв * 100 раундов = 1000 combat spans/мин ≈ **1.5 MB/мин** traces
- 30 метрик * 60 сек = 1800 точек/мин ≈ **0.9 MB/мин** metrics

**При batch экспорте** (каждые 5 секунд):
- **~40 KB/batch** (сжатие gRPC)

## Рекомендации по оптимизации

### 1. Sampling

**Head-based sampling** (на уровне SDK):
```cpp
// В конфиге OTEL
export OTEL_TRACES_SAMPLER=traceidratio
export OTEL_TRACES_SAMPLER_ARG=0.1  // 10% traces
```

**Tail-based sampling** (в OTEL Collector):
```yaml
processors:
  tail_sampling:
    policies:
      - name: combat-errors
        type: string_attribute
        string_attribute: {key: error, values: [true]}
      - name: slow-operations
        type: latency
        latency: {threshold_ms: 100}
      - name: sample-rate
        type: probabilistic
        probabilistic: {sampling_percentage: 10}
```

**Рекомендуемые sampling rates**:
- Combat traces: **10-20%** (достаточно для паттернов)
- Other traces: **50-100%** (редкие операции)
- Metrics: **100%** (всегда, низкий overhead)

### 2. Batch размеры

```yaml
# В OTEL SDK config
exporters:
  otlp:
    endpoint: collector:4317
    timeout: 10s

processors:
  batch:
    timeout: 5s
    send_batch_size: 512
    send_batch_max_size: 1024
```

### 3. Атрибуты

**Избегать**:
- Длинные строки (>100 символов) в атрибутах
- High-cardinality атрибуты (уникальные IDs) без лимита
- Дублирование данных между span и атрибутами

**Рекомендуется**:
- Использовать короткие константы для типов
- Ограничивать количество атрибутов (5-10 на span)
- Использовать enum/int вместо strings где возможно

### 4. Conditional instrumentation

Для production можно добавить feature flags:

```cpp
// В config
bool enable_combat_traces = true;
bool enable_detailed_attrs = false; // только для debug

// В коде
if (enable_combat_traces) {
    auto span = StartSpan("Combat");
    if (enable_detailed_attrs) {
        span->SetAttribute("detailed_info", ...);
    }
}
```

## Выводы

### CPU Overhead

| Сценарий | Overhead | Приемлемо? |
|----------|----------|------------|
| Нормальная нагрузка | 0.025% CPU | ✅ Да |
| Пиковая нагрузка | 0.08% CPU | ✅ Да |
| С sampling 10% | <0.01% CPU | ✅ Да |

### Memory Overhead

| Сценарий | Overhead | Приемлемо? |
|----------|----------|------------|
| Нормальная нагрузка | ~100 KB | ✅ Да |
| Пиковая нагрузка | ~250 KB | ✅ Да |

### Network Overhead

| Metric | Bandwidth | Приемлемо? |
|--------|-----------|------------|
| Traces | 1.5 MB/мин | ✅ Да (~200 Kbps) |
| Metrics | 0.9 MB/мин | ✅ Да (~120 Kbps) |
| **Итого** | **2.4 MB/мин** | ✅ Да (~320 Kbps) |

### Итоговая рекомендация

**OpenTelemetry инструментация безопасна для production** при условии:
1. ✅ Используется batch export (default)
2. ✅ OTEL Collector находится в локальной сети (low latency)
3. ⚠️  Для очень высокой нагрузки (>100 игроков) - включить sampling 10-20%
4. ✅ Monitoring самого OTEL (экспортирует свои метрики)

**Преимущества значительно перевешивают overhead**:
- Детальная visibility в production
- Root cause analysis проблем производительности
- Business intelligence из метрик
- Distributed tracing для сложных операций (overlapping combat traces!)

---
*Дата: 2026-01-28*
*Методология: OpenTelemetry C++ SDK benchmarks + практические оценки*
