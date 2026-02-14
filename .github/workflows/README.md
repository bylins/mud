# GitHub Actions CI/CD

## Workflows

### `build.yml` - Multi-Platform Build Matrix

Проверяет сборку проекта на всех поддерживаемых платформах с различными конфигурациями.

#### Linux Configurations (Matrix)

| Конфигурация | Тесты | YAML | SQLite | Admin API | Описание |
|-------------|:-----:|:----:|:------:|:---------:|----------|
| Base | ❌ | ❌ | ❌ | ❌ | Базовая сборка (legacy format) |
| With Tests | ✅ | ❌ | ❌ | ❌ | Базовая сборка + unit tests |
| YAML | ❌ | ✅ | ❌ | ❌ | Поддержка YAML без админки |
| YAML + Admin API | ❌ | ✅ | ❌ | ✅ | YAML с Admin API |
| YAML + Tests | ✅ | ✅ | ❌ | ❌ | YAML + unit tests |
| SQLite | ❌ | ❌ | ✅ | ❌ | Поддержка SQLite |
| SQLite + Tests | ✅ | ❌ | ✅ | ❌ | SQLite + unit tests |

**Важно:** YAML и SQLite - взаимоисключающие форматы данных мира.

**CMake флаги:**
- `-DHAVE_YAML=ON` - включает поддержку YAML world format
- `-DHAVE_SQLITE=ON` - включает поддержку SQLite world format
- `-DENABLE_ADMIN_API=ON` - включает Admin API (требует YAML)
- `-DBUILD_TESTS=OFF` - отключает сборку тестов (по умолчанию включены)

**Зависимости:**
- YAML: `libyaml-cpp-dev`
- SQLite: `libsqlite3-dev`
- Tests: `libgtest-dev`

#### Linux GCC 15

| Компилятор | Тесты | Описание |
|-----------|:-----:|----------|
| GCC 15 | ✅ | Сборка с GCC 15 в Debian Sid container |

#### Other Platforms

| Платформа | Компилятор | Статус |
|-----------|------------|--------|
| Windows | MSVC | Soft-failure |
| Windows | MinGW (MSYS2) | Soft-failure |
| Cygwin | GCC | Soft-failure |
| WSL | GCC | Soft-failure |

**Режим работы:** Все jobs работают в режиме `continue-on-error: true` (soft-failure), то есть:
- ❌ Падение сборки **НЕ блокирует** merge PR
- ✅ Результаты видны в Summary и статусах PR
- 📊 Позволяет отслеживать состояние кросс-платформенной совместимости

### `quick-check.yml` - Fast CI Check

Быстрая проверка на каждый push/PR:
- Только базовая Linux сборка (без опциональных feature flags)
- Кэширование apt-пакетов для ускорения
- Проверка базовых синтаксических ошибок

## Локальное тестирование

Перед push можно проверить сборку локально:

```bash
# Базовая сборка (legacy format)
mkdir build && cd build
cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Test ..
make -j$(nproc)

# С тестами
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Test ..
make tests -j$(nproc)
./tests/tests

# С YAML support
mkdir build_yaml && cd build_yaml
cmake -DHAVE_YAML=ON -DCMAKE_BUILD_TYPE=Test ..
make -j$(nproc)

# С SQLite support
mkdir build_sqlite && cd build_sqlite
cmake -DHAVE_SQLITE=ON -DCMAKE_BUILD_TYPE=Test ..
make -j$(nproc)

# YAML + Admin API
mkdir build_admin && cd build_admin
cmake -DHAVE_YAML=ON -DENABLE_ADMIN_API=ON -DCMAKE_BUILD_TYPE=Test ..
make -j$(nproc)

# YAML + Tests
mkdir build_yaml_tests && cd build_yaml_tests
cmake -DHAVE_YAML=ON -DCMAKE_BUILD_TYPE=Test ..
make tests -j$(nproc)
./tests/tests

# SQLite + Tests
mkdir build_sqlite_tests && cd build_sqlite_tests
cmake -DHAVE_SQLITE=ON -DCMAKE_BUILD_TYPE=Test ..
make tests -j$(nproc)
./tests/tests
```

## Известные проблемы

### Admin API
- ⚠️ Admin API работает **только с YAML** world format
- Требует `-DHAVE_YAML=ON -DENABLE_ADMIN_API=ON`
- Unix socket создается в world directory (напр. `small/admin_api.sock`)

### Windows (MSVC)
- Может требовать адаптации кода под Windows API
- Некоторые POSIX-функции недоступны
- vcpkg используется для управления зависимостями

### Cygwin
- Производительность ниже, чем у нативного Linux
- Могут быть проблемы с путями (Windows vs POSIX)

### WSL
- Ограничения на доступ к некоторым системным функциям
- Может отличаться от чистого Linux в edge cases

## Переход на строгий режим

Когда все платформы будут стабильно собираться, можно убрать `continue-on-error: true` из jobs для включения строгого режима (блокировка PR при падении).

## Кэширование

В `quick-check.yml` добавлено кэширование apt-пакетов для ускорения повторных сборок.

В будущем можно добавить:
- Кэширование vcpkg пакетов (Windows)
- Кэширование CMake build cache
- Кэширование submodules

## Мониторинг

Результаты CI доступны:
- В разделе **Actions** на GitHub
- В статусах Pull Request
- В автоматическом **Summary** (markdown таблица с результатами)

## Badge для README

Для добавления статус-badge в основной README.md:

```markdown
[![Multi-Platform Build](https://github.com/bylins/mud/actions/workflows/build.yml/badge.svg)](https://github.com/bylins/mud/actions/workflows/build.yml)
[![Quick Check](https://github.com/bylins/mud/actions/workflows/quick-check.yml/badge.svg)](https://github.com/bylins/mud/actions/workflows/quick-check.yml)
```
