# План реализации модуля Loot Tables для Bylins MUD

## Обзор

Создание модульной системы таблиц наград (loot tables) для замены существующих механизмов dead_load и GlobalDrop. Система будет поддерживать:
- Гибкие фильтры источников (мобы, квесты, контейнеры)
- Вероятностную генерацию с учетом MIW лимитов
- YAML формат конфигурационных файлов
- Hot-reload без перезапуска сервера
- Локальные таблицы зон и глобальные таблицы

## Ключевые архитектурные решения

### Формат данных: YAML
- **Решение**: Использовать YAML (как запрошено) вместо XML
- **Обоснование**: Удобнее для ручного редактирования билдерами
- **Реализация**: Добавить yaml-cpp в зависимости, создать адаптер для parser_wrapper

### Приоритет таблиц
- **Локальные таблицы зон** дополняют глобальные (не заменяют)
- Локальные имеют priority=150 по умолчанию, глобальные=100
- Все применимые таблицы сортируются по приоритету и применяются

### Механика fallback при MIW лимите
- **Первичная вероятность**: weight из probability_width (например, 2000/10000 = 20%)
- **Вторичная вероятность**: fallback_weight из probability_width (например, 50/10000 = 0.5%)
- Если MIW достигнут, проверяется fallback_weight вместо weight

### Повторный выбор предметов
- **Флаг allow_duplicates** в LootEntry (по умолчанию false)
- false: предмет выбирается максимум 1 раз за генерацию (без возврата)
- true: предмет может выпасть несколько раз (с возвратом)

## Этапы реализации

### Этап 1: Добавление yaml-cpp и базовые структуры (2-3 дня)

**Файлы для создания:**
- `src/gameplay/mechanics/loot_tables/loot_table_types.h` - Энумы и структуры данных
- `src/gameplay/mechanics/loot_tables/loot_table_types.cpp` - Реализация методов

**Изменения в существующих файлах:**
- `CMakeLists.txt` - добавить yaml-cpp зависимость и новые исходники

**Структуры данных:**

```cpp
namespace loot_tables {

// Энумы
enum class ELootSourceType { kMobDeath, kContainer, kQuest };
enum class ELootTableType { kSourceCentric, kItemCentric };
enum class ELootFilterType { kVnum, kLevelRange, kRace, kClass, kFlags, kZone };
enum class ELootEntryType { kItem, kCurrency, kNestedTable };

// Фильтр источника
struct LootFilter {
    ELootFilterType type;
    ObjVnum vnum;
    int min_level, max_level;
    int race, mob_class;
    FlagData flags;
    ZoneVnum zone_vnum;

    bool Matches(const CharData* mob) const;
};

// Запись в таблице
struct LootEntry {
    ELootEntryType type;
    ObjVnum item_vnum;              // для kItem
    int currency_id;                 // для kCurrency
    std::string nested_table_id;     // для kNestedTable

    int min_count, max_count;        // количество
    int probability_weight;          // вес (из probability_width)
    int fallback_weight;             // вторичный вес при MIW лимите
    bool allow_duplicates;           // может выпасть несколько раз?

    // Фильтры игрока
    int min_player_level, max_player_level;
    std::vector<int> allowed_classes;

    bool CheckPlayerFilters(const CharData* player) const;
    int RollCount() const;
};

}
```

### Этап 2: Класс LootTable и генерация (2-3 дня)

**Файлы для создания:**
- `src/gameplay/mechanics/loot_tables/loot_table.h` - Класс LootTable
- `src/gameplay/mechanics/loot_tables/loot_table.cpp` - Реализация генерации

**Ключевой функционал:**

```cpp
class LootTable {
private:
    std::string id_;
    ELootTableType table_type_;
    ELootSourceType source_type_;
    std::vector<LootFilter> source_filters_;
    std::vector<LootEntry> entries_;

    int probability_width_{10000};   // база для вероятности
    int max_items_per_roll_{10};     // макс предметов за раз
    int priority_{100};              // приоритет таблицы

public:
    struct GenerationContext {
        const CharData* player;
        const CharData* source_mob;
        int luck_bonus;
    };

    struct GeneratedLoot {
        std::vector<std::pair<ObjVnum, int>> items;
        std::map<int, long> currencies;
    };

    GeneratedLoot Generate(const GenerationContext& ctx) const;
    bool IsApplicable(const CharData* mob) const;
};
```

**Алгоритм генерации:**
1. Фильтрация записей по игроку (уровень, класс)
2. Weighted random выбор с учетом luck_bonus
3. Проверка MIW лимита:
   - Если лимит не достигнут: сравниваем с probability_weight
   - Если лимит достигнут: сравниваем с fallback_weight
4. Если allow_duplicates=false, удаляем запись из пула
5. Повторяем до max_items_per_roll

### Этап 3: Реестр и индексация (2 дня)

**Файлы для создания:**
- `src/gameplay/mechanics/loot_tables/loot_registry.h` - Класс LootTablesRegistry
- `src/gameplay/mechanics/loot_tables/loot_registry.cpp` - Реализация

**Индексы для быстрого поиска:**
```cpp
class LootTablesRegistry {
private:
    // SOURCE_CENTRIC индексы
    std::multimap<ObjVnum, TablePtr> mob_vnum_index_;
    std::multimap<ZoneVnum, TablePtr> zone_index_;
    std::vector<TablePtr> filtered_tables_;  // с фильтрами

    // ITEM_CENTRIC индексы
    std::multimap<ObjVnum, TablePtr> item_vnum_index_;

    TableMap all_tables_;

public:
    std::vector<TablePtr> FindTablesForMob(const CharData* mob) const;
    GeneratedLoot GenerateFromMob(const CharData* mob, const CharData* killer, int luck) const;
    void BuildIndices();
};
```

### Этап 4: YAML парсер и загрузчик (3-4 дня)

**Файлы для создания:**
- `src/gameplay/mechanics/loot_tables/yaml_adapter.h` - Адаптер yaml-cpp к parser_wrapper
- `src/gameplay/mechanics/loot_tables/yaml_adapter.cpp` - Реализация
- `src/gameplay/mechanics/loot_tables/loot_loader.h` - ICfgLoader для loot tables
- `src/gameplay/mechanics/loot_tables/loot_loader.cpp` - Парсинг YAML

**YAML адаптер:**
```cpp
// Расширение parser_wrapper для поддержки YAML
namespace parser_wrapper {

class YamlDataNode : public DataNode {
private:
    YAML::Node yaml_node_;

public:
    explicit YamlDataNode(const std::filesystem::path& yaml_file);

    // Реализация всех методов DataNode через YAML::Node
    bool GoToChild(const std::string& name) override;
    std::string GetValue(const std::string& attr) override;
    // ...
};

}
```

**Парсинг таблиц:**
```cpp
class LootTablesLoader : public cfg_manager::ICfgLoader {
public:
    void Load(parser_wrapper::DataNode data) override;
    void Reload(parser_wrapper::DataNode data) override;

private:
    LootTable::TablePtr ParseTable(YAML::Node node);
    LootFilter ParseFilter(YAML::Node node);
    LootEntry ParseEntry(YAML::Node node);
    void ValidateTable(const LootTable& table);
    void ValidateNoCycles();
};
```

**Формат YAML:**
```yaml
# lib/cfg/loot_tables/global.yaml
tables:
  - id: boss_dragon_loot
    type: source_centric
    source: mob_death
    priority: 200

    filters:
      - type: vnum
        vnum: 12345

    params:
      probability_width: 10000
      max_items: 5

    entries:
      - type: item
        vnum: 50001
        count: {min: 1, max: 1}
        weight: 10000
        fallback_weight: 0
        allow_duplicates: false

      - type: currency
        currency_id: 0  # GOLD
        count: {min: 5000, max: 10000}
        weight: 3000
```

### Этап 5: Интеграция с corpse.cpp (1-2 дня)

**Файл для изменения:**
- `src/gameplay/mechanics/corpse.cpp` - интеграция в make_corpse()

**Точка интеграции (после строки 516):**
```cpp
// В make_corpse() после создания ингредиентов
if (ch->IsNpc() && !IS_CHARMICE(ch)) {
    // Новая система лута
    auto& loot_registry = loot_tables::GetGlobalRegistry();

    int luck_bonus = killer ? CalculateLuckBonus(killer) : 0;
    auto generated = loot_registry.GenerateFromMob(ch, killer, luck_bonus);

    // Размещаем предметы
    for (const auto& [vnum, count] : generated.items) {
        for (int i = 0; i < count; ++i) {
            auto obj = world_objects.create_from_prototype_by_vnum(vnum);
            if (obj) {
                obj->set_vnum_zone_from(GetZoneVnumByCharPlace(ch));
                PlaceObjIntoObj(obj.get(), corpse.get());
            }
        }
    }

    // Размещаем валюты
    for (const auto& [currency_id, amount] : generated.currencies) {
        AddCurrencyToCorpse(corpse.get(), currency_id, amount);
    }
}
```

**Хелпер для валют:**
```cpp
void AddCurrencyToCorpse(ObjData* corpse, int currency_id, long amount) {
    if (currency_id == currency::GOLD) {
        const auto money = CreateCurrencyObj(amount);
        PlaceObjIntoObj(money.get(), corpse);
    }
    else if (currency_id == currency::GLORY) {
        // TODO: создать объект славы или добавить через другую систему
    }
    else if (currency_id == currency::TORC) {
        // Торки - через ExtMoney::drop_torc()
    }
    // Другие валюты...
}
```

### Этап 6: Регистрация в CfgManager (1 день)

**Файл для изменения:**
- `src/engine/boot/cfg_manager.cpp` - добавить загрузчик

**Регистрация:**
```cpp
CfgManager::CfgManager() {
    // ... существующие загрузчики ...

    loaders_.emplace("loot_tables",
        LoaderInfo("cfg/loot_tables/global.yaml",
            std::make_unique<loot_tables::LootTablesLoader>(
                &loot_tables::GetGlobalRegistry()
            )
        )
    );
}
```

**Поддержка reload:**
```
> reload loot_tables
LOOT_TABLES: загружено 45 таблиц из global.yaml
LOOT_TABLES: загружено 12 локальных таблиц из зон
Loot tables перезагружены.
```

### Этап 7: Команды для иммов (1 день)

**Файл для создания:**
- `src/engine/ui/cmd_god/do_loottable.cpp` - команда loottable
- `src/engine/ui/cmd_god/do_loottable.h` - заголовок

**Команды:**
```
loottable list                        - список всех таблиц
loottable info <table_id>             - детали таблицы
loottable reload                      - перезагрузить таблицы
loottable test <table_id> [luck]      - тестовая генерация
loottable generate <mob_vnum> [luck]  - генерация для моба
loottable stats                       - статистика использования
```

### Этап 8: Логирование и диагностика (1 день)

**Файлы для создания:**
- `src/gameplay/mechanics/loot_tables/loot_logger.h` - класс LootLogger
- `src/gameplay/mechanics/loot_tables/loot_logger.cpp` - реализация

**Типы логов:**
- Редкие дропы (weight < rare_threshold)
- MIW override (fallback сработал)
- Ошибки (циклы, невалидные vnum)

**Формат:**
```
[2026-01-28 18:00:00] RARE: boss_dragon_loot -> item[50002] x1 | killer: Гретта(25) | weight: 500/10000
[2026-01-28 18:01:00] MIW_OVERRIDE: undead_loot -> item[60005] | fallback: 50/10000 | killer: Иван(30)
```

### Этап 9: Unit-тесты (2 дня)

**Файл для создания:**
- `tests/loot_tables.cpp` - GoogleTest тесты

**Тестовые сценарии:**
1. Фильтрация: LootFilter::Matches() для разных типов
2. Weighted random: проверка распределения (1000 итераций)
3. MIW проверка: корректность fallback механизма
4. Повторный выбор: allow_duplicates=true/false
5. Циклические ссылки: обнаружение nested table циклов
6. Парсинг YAML: валидация корректности загрузки

### Этап 10: Миграция и документация (2-3 дня)

**Файлы для создания:**
- `tools/convert_deadload_to_yaml.py` - конвертер старых DL в YAML
- `docs/loot_tables_guide.md` - руководство для билдеров

**Конвертер:**
```python
# Парсит .mob файлы с DL записями
# Конвертирует в YAML формат
# Генерирует zone_XXX_converted.yaml
```

**Dual-mode период:**
- Флаг USE_NEW_LOOT_SYSTEM в конфиге
- Параллельная работа старой и новой системы
- A/B тестирование на тестовом сервере

## Критические файлы для реализации

1. **src/gameplay/mechanics/loot_tables/loot_table_types.h** - Фундамент: все энумы и структуры
2. **src/gameplay/mechanics/loot_tables/loot_table.cpp** - Ядро: алгоритм генерации
3. **src/gameplay/mechanics/loot_tables/loot_registry.cpp** - Производительность: индексация и поиск
4. **src/gameplay/mechanics/loot_tables/loot_loader.cpp** - Загрузка: парсинг YAML и валидация
5. **src/gameplay/mechanics/corpse.cpp** - Интеграция: применение лута при смерти моба

## Структура файлов проекта

```
src/gameplay/mechanics/loot_tables/
├── loot_table_types.h/cpp       # Энумы, LootFilter, LootEntry
├── loot_table.h/cpp             # LootTable класс с Generate()
├── loot_registry.h/cpp          # LootTablesRegistry с индексами
├── yaml_adapter.h/cpp           # Адаптер yaml-cpp к parser_wrapper
├── loot_loader.h/cpp            # ICfgLoader для загрузки YAML
├── loot_logger.h/cpp            # Логирование дропов
└── loot_tables.h                # Публичный API

src/engine/ui/cmd_god/
└── do_loottable.cpp/h           # Команда для иммов

lib/cfg/loot_tables/
├── global.yaml                  # Глобальные таблицы
├── bosses.yaml                  # Боссы
├── common.yaml                  # Общие (gems, currency)
└── seasonal.yaml                # Сезонные события

lib/world/ltt/
├── 100.ltt.yaml                 # Локальные таблицы зоны 100
├── 120.ltt.yaml                 # Локальные таблицы зоны 120
└── ...

tools/
└── convert_deadload_to_yaml.py  # Конвертер DL -> YAML

tests/
└── loot_tables.cpp              # Unit-тесты
```

## Зависимости (CMakeLists.txt)

```cmake
# Добавить yaml-cpp
include(FetchContent)
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
)
FetchContent_MakeAvailable(yaml-cpp)

# Добавить исходники
set(LOOT_TABLES_SOURCES
    src/gameplay/mechanics/loot_tables/loot_table_types.cpp
    src/gameplay/mechanics/loot_tables/loot_table.cpp
    src/gameplay/mechanics/loot_tables/loot_registry.cpp
    src/gameplay/mechanics/loot_tables/yaml_adapter.cpp
    src/gameplay/mechanics/loot_tables/loot_loader.cpp
    src/gameplay/mechanics/loot_tables/loot_logger.cpp
)

# Линковка
target_link_libraries(circle PRIVATE yaml-cpp)
```

## Проверка реализации

### Функциональные тесты:
1. Создать тестового моба с таблицей лута
2. Убить моба, проверить содержимое трупа
3. Проверить фильтры по уровню/классу игрока
4. Проверить MIW лимиты и fallback
5. Проверить nested tables (3 уровня вложенности)
6. Проверить reload без перезапуска сервера

### Тесты производительности:
1. Генерация лута из 100 таблиц (должно < 1ms)
2. Поиск таблиц для моба (должно < 100μs)
3. Парсинг 1000 таблиц при загрузке (должно < 5s)

### End-to-end тест:
```
1. Запустить сервер
2. Загрузить персонажа уровня 30 (класс: warrior)
3. Убить моба vnum=12345 (босс дракон)
4. Проверить труп:
   - Предмет 50001 (100% шанс)
   - Золото 5000-10000 (30% шанс)
   - Редкий предмет 50002 (5% шанс, если MIW позволяет)
5. Логи должны показать RARE drop если выпал 50002
```

## Риски и митигация

### Риск 1: Производительность
**Проблема**: Генерация на каждую смерть может быть медленной
**Митигация**: Индексы, кэширование весов, профилирование (порог 1ms)

### Риск 2: Совместимость
**Проблема**: Существующие DG Scripts используют %load%
**Митигация**: Dual-mode, новая команда %loottable%, постепенная миграция

### Риск 3: Балансировка
**Проблема**: Изменение системы может сломать баланс
**Митигация**: A/B тестирование, логирование всех дропов, команды для анализа

### Риск 4: YAML encoding
**Проблема**: Файлы должны быть в KOI8-R, yaml-cpp работает с UTF-8
**Митигация**: Конвертация при загрузке (iconv), либо хранить YAML в UTF-8

## Оценка времени

| Этап | Время | Зависимости |
|------|-------|-------------|
| 1. Базовые структуры | 2-3 дня | - |
| 2. Генерация | 2-3 дня | Этап 1 |
| 3. Реестр | 2 дня | Этап 2 |
| 4. YAML парсер | 3-4 дня | Этап 1 |
| 5. Интеграция corpse | 1-2 дня | Этапы 2-3 |
| 6. CfgManager | 1 день | Этап 4 |
| 7. Команды иммов | 1 день | Этапы 2-3 |
| 8. Логирование | 1 день | Этап 2 |
| 9. Тесты | 2 дня | Все этапы |
| 10. Миграция | 2-3 дня | Все этапы |

**Итого**: 17-22 рабочих дня (3.5-4.5 недели)

## Примеры YAML конфигов

### Простой дроп (100% шанс):
```yaml
tables:
  - id: newbie_wolf_loot
    type: source_centric
    source: mob_death
    filters:
      - {type: vnum, vnum: 1001}
    entries:
      - type: item
        vnum: 10001  # Шкура волка
        count: {min: 1, max: 1}
        weight: 10000
```

### Множественные предметы:
```yaml
tables:
  - id: treasure_chest
    type: source_centric
    source: container
    filters:
      - {type: vnum, vnum: 80001}
    params:
      probability_width: 10000
      max_items: 3
    entries:
      - type: currency
        currency_id: 0  # GOLD
        count: {min: 500, max: 1000}
        weight: 8000
      - type: item
        vnum: 50001  # Редкий меч (5%)
        weight: 500
        fallback_weight: 50  # 0.5% если MIW достигнут
      - type: item
        vnum: 50002  # Щит (30%)
        weight: 3000
```

### Nested tables:
```yaml
tables:
  - id: gems_common
    type: source_centric
    source: mob_death
    entries:
      - {type: item, vnum: 60001, count: {min: 1, max: 3}, weight: 5000, allow_duplicates: true}
      - {type: item, vnum: 60002, count: {min: 1, max: 2}, weight: 3000, allow_duplicates: true}

  - id: dragon_boss_loot
    type: source_centric
    source: mob_death
    filters:
      - {type: vnum, vnum: 99999}
    entries:
      - {type: item, vnum: 99001, weight: 10000}
      - {type: nested_table, table_id: gems_common, weight: 8000}
```

### Фильтры по игроку:
```yaml
tables:
  - id: class_specific_loot
    type: source_centric
    source: mob_death
    filters:
      - {type: level_range, min: 25, max: 35}
      - {type: race, race: 14}  # UNDEAD
    entries:
      - type: item
        vnum: 70001  # Меч для воинов
        weight: 5000
        player_filters:
          level: {min: 25, max: 50}
          classes: [1, 4]  # Warrior, Paladin
      - type: item
        vnum: 70002  # Посох для магов
        weight: 5000
        player_filters:
          classes: [2, 3]  # Mage, Cleric
```

## Резюме

Модуль loot tables заменит существующие системы dead_load и GlobalDrop единым гибким решением. Ключевые преимущества:

✅ **Гибкость**: Множественные фильтры, nested tables, условия по игроку
✅ **Производительность**: Индексация, кэширование, оптимизированный поиск
✅ **Удобство**: YAML конфиги, hot-reload, команды для иммов
✅ **Надежность**: Валидация, обнаружение циклов, логирование
✅ **Расширяемость**: Легко добавлять новые типы фильтров/записей

План предусматривает постепенное внедрение с сохранением обратной совместимости и тщательным тестированием.
