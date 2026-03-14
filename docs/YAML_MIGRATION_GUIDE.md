# План миграции CircleMUD на YAML формат

## Подготовка окружения

### Настройка тестового зеркала

Тестовое зеркало - это отдельная установка MUD для проверки миграции перед применением на продакшене.

```bash
# Перейти в директорию зеркала
cd /home/stribog/mud

# Установить переменные окружения
export DIR=$(pwd)
export WORLD=$(pwd)/lib
export PROD_WORLD=/home/mud/mud/lib

# Скопировать мир из продакшена
rsync -av --delete /home/mud/mud/lib/ ./lib/

# Проверить структуру
ls -la $WORLD/world/
```

**Ожидаемый результат:**
- Переменные установлены
- Мир скопирован из продакшена
- Структура `lib/world/` с legacy форматом (wld/, mob/, obj/, zon/, trg/)

---

### Настройка продакшена для миграции

```bash
# Перейти в продакшен
cd /home/mud/mud

# Установить переменные окружения
export DIR=$(pwd)
export WORLD=$(pwd)/lib

# КРИТИЧНО: Создать резервную копию
mkdir -p backup
tar -czf backup/lib_before_yaml_$(date +%Y%m%d_%H%M%S).tar.gz lib

# Проверить бэкапы
ls -lh backup/
du -sh backup/*

# КРИТИЧНО: Остановить сервер
killall -TERM circle
sleep 5
ps aux | grep circle  # Проверить, что остановился
```

---

## Миграция (одинаковые команды для зеркала и прода)

С этого момента команды одинаковые для тестового зеркала и продакшена. Выполняются в текущей директории установки (`$DIR`).

### ЭТАП 1: Сборка бинарника с YAML

```bash
# Проверить yaml-cpp
dpkg -l | grep libyaml-cpp-dev
# Если нет: sudo apt-get install libyaml-cpp-dev

# Создать build директорию
mkdir -p build_yaml
cd build_yaml

# Сборка (указать путь к исходникам репозитория)
cmake -DCMAKE_BUILD_TYPE=Release \
      -DHAVE_YAML=ON \
      -DBUILD_TESTS=OFF \
      ..

make -j2

# Проверить сборку
ls -lh circle
ldd circle | grep yaml

# Вернуться в корень установки
cd $DIR
```

---

### ЭТАП 2: Конверсия мира

```bash
# Подготовить виртуальное окружение для конвертера
python3 -m venv .venv
source .venv/bin/activate
pip install ruamel.yaml

# Конвертировать lib/world/ (legacy) → world/ (YAML)
python3 tools/converter/convert_to_yaml.py \
    --input lib/ \
    --output lib/ \
    --format yaml \
    --type all \
    --workers $(nproc)

# Проверить результат
ls -la lib/world/
ls -la lib/world/zones/
ls -la lib/world/mobs/

# Статистика
echo "=== Статистика конверсии ==="

ZONES_FILES=$(ls -d lib/world/zones/*/ 2>/dev/null | wc -l)
ZONES_INDEX=$(grep -c '^[[:space:]]*- ' lib/world/zones/index.yaml 2>/dev/null || echo 0)
echo "Зоны:     файлов: $ZONES_FILES, в индексе: $ZONES_INDEX"

ROOMS_FILES=$(find lib/world/zones/*/rooms/ -name '*.yaml' 2>/dev/null | wc -l)
echo "Комнаты:  файлов: $ROOMS_FILES"

MOBS_FILES=$(ls lib/world/mobs/*.yaml 2>/dev/null | grep -v index.yaml | wc -l)
MOBS_INDEX=$(grep -c '^[[:space:]]*- ' lib/world/mobs/index.yaml 2>/dev/null || echo 0)
echo "Мобы:     файлов: $MOBS_FILES, в индексе: $MOBS_INDEX"

OBJS_FILES=$(ls lib/world/objects/*.yaml 2>/dev/null | grep -v index.yaml | wc -l)
OBJS_INDEX=$(grep -c '^[[:space:]]*- ' lib/world/objects/index.yaml 2>/dev/null || echo 0)
echo "Объекты:  файлов: $OBJS_FILES, в индексе: $OBJS_INDEX"

TRIGS_FILES=$(ls lib/world/triggers/*.yaml 2>/dev/null | grep -v index.yaml | wc -l)
TRIGS_INDEX=$(grep -c '^[[:space:]]*- ' lib/world/triggers/index.yaml 2>/dev/null || echo 0)
echo "Триггеры: файлов: $TRIGS_FILES, в индексе: $TRIGS_INDEX"

# Деактивировать виртуальное окружение
deactivate
```

**Ожидаемый результат:**
- Поддиректории: `zones/`, `mobs/`, `objects/`, `triggers/` созданы в `lib/world/`
- YAML файлы в KOI8-R кодировке

---

### ЭТАП 3: Тестовый запуск

```bash
# Выбрать порт (5555 для зеркала, 4000 для прода после остановки)
PORT=5555  # Для зеркала
# PORT=4000  # Для прода

# Запуск с контрольными суммами (из build директории)
cd build_yaml
./circle -W -d ../lib $PORT > boot.log 2>&1 &
PID=$!

# Подождать загрузки
sleep 30

# Проверить запуск
ps aux | grep $PID
netstat -tuln | grep $PORT

# Остановить
kill -TERM $PID
sleep 5

cd $DIR
```

**Ожидаемый результат:**
- Сервер запустился
- Нет критических ошибок
- Контрольные суммы рассчитаны

**Возможные ошибки:**
- `Cannot find world directory` → проверить `lib/misc/configuration.xml`
- `Failed to load zone` → проблема конверсии, проверить YAML

---

### ЭТАП 4: Валидация

```bash
# Запустить для интерактивной проверки
PORT=5555  # Для зеркала
# PORT=4000  # Для прода

cd build_yaml
./circle -W -d ../lib $PORT > validation.log 2>&1 &
PID=$!
sleep 30

# Подключиться telnet (в другом терминале)
# telnet localhost $PORT
# Проверить: look, who, help

# Остановить
kill -TERM $PID

cd $DIR
```

**Ожидаемый результат:**
- Все зоны загружены
- Telnet работает
- Команды работают

---

### ЭТАП 5: Развёртывание (только для прода)

**ВАЖНО:** Выполнять только на продакшене после успешного тестирования на зеркале!

```bash
# Убедиться, что в продакшене
cd /home/mud/mud
export DIR=$(pwd)

# Убедиться, что сервер остановлен
ps aux | grep circle
# Если запущен: killall -TERM circle

# Финальный бэкап перед развёртыванием
tar -czf backup/prod_before_deploy_$(date +%Y%m%d_%H%M%S).tar.gz lib

# Переименовать старую lib/ (не удалять!)
mv lib lib.legacy_backup

# Переименовать старый бинарник
mv circle circle.legacy_backup

# Скопировать YAML мир
# Если был на зеркале - скопировать оттуда:
# rsync -av --delete /home/stribog/mud/lib/ ./lib/
# Если конвертировали на продакшене - уже есть

# Скопировать YAML бинарник
cp build_yaml/circle ./circle

# Установить права
chmod +x circle

# ПЕРВЫЙ ЗАПУСК
cd build_yaml
./circle -W -d ../lib 4000 > first_boot.log 2>&1 &
PROD_PID=$!

# Мониторинг запуска (2 минуты)
for i in {1..24}; do
    sleep 5
    echo "=== Через $((i*5)) секунд ==="
    tail -3 first_boot.log
    netstat -tuln | grep 4000 && echo "Порт открыт!" || echo "Порт еще закрыт"
done

# Проверить успешность
tail -50 first_boot.log

cd $DIR
```

**Ожидаемый результат:**
- Старая `lib/` в `lib.legacy_backup`
- YAML мир в `lib/world/`
- Сервер запустился

**Если ошибки → см. ЭТАП 6 (Откат)**

---

### ЭТАП 6: Откат (в случае проблем)

**КРИТИЧНО:** Выполнять только если на ЭТАПЕ 5 критические проблемы!

```bash
# Остановить YAML сервер
killall -TERM circle
sleep 5
killall -KILL circle  # Если не остановился

# Восстановить старую lib/
rm -rf lib
mv lib.legacy_backup lib

# Восстановить старый бинарник
rm -f circle
mv circle.legacy_backup circle

# Запустить legacy версию
./circle -d lib 4000 &

# Проверить
sleep 10
tail -50 misc/syslog
netstat -tuln | grep 4000

# Записать причину отката
echo "ROLLBACK: $(date) - Reason: [УКАЗАТЬ ПРИЧИНУ]" >> MIGRATION.log
```

**Критерии для отката:**
- Не загружается >50% зон
- Крах сервера при запуске
- Потеря данных игроков
- Несоответствие контрольных сумм >10%

---

## Верификация успешной миграции

### Чек-лист:
- [ ] Резервные копии созданы
- [ ] YAML бинарник собран с `-DHAVE_YAML=ON`
- [ ] Конверсия завершена без ошибок
- [ ] Тестовое зеркало запустилось и прошло валидацию
- [ ] Продакшен запустился на YAML
- [ ] Контрольные суммы совпадают (или близки к legacy)
- [ ] Игроки могут подключаться
- [ ] Нет критических ошибок в логах

### Команда проверки состояния:

Запускать из билд-директории (поддиректория репозитория, lib/ на уровень выше).

```bash
cat << 'EOF' > check_yaml_status.sh
#!/bin/bash
echo "=== YAML Migration Status ==="
echo "Binary: $(ldd ./circle 2>&1 | grep -q yaml && echo 'YAML' || echo 'LEGACY')"
echo "World format: $([ -d ../lib/world/zones ] && echo 'YAML' || echo 'LEGACY')"
echo "Server running: $(pgrep -f circle >/dev/null && echo 'YES' || echo 'NO')"
echo "Port 4000: $(netstat -tuln | grep -q :4000 && echo 'OPEN' || echo 'CLOSED')"
echo "Recent errors: $(tail -100 ../lib/misc/syslog 2>/dev/null | grep -ci error)"
EOF
chmod +x check_yaml_status.sh
./check_yaml_status.sh
```
