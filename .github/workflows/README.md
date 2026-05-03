# GitHub Actions CI/CD

## Workflows

### `build.yml` — Multi-Platform Build Matrix

Проверяет сборку проекта на всех поддерживаемых платформах с различными конфигурациями.

#### Linux (GCC, Ubuntu Latest)

| Конфигурация | YAML | SQLite | Admin API | OpenTelemetry |
|-------------|:----:|:------:|:---------:|:-------------:|
| Base | | | | |
| YAML | ✅ | | | |
| SQLite | | ✅ | | |
| Base + Admin API + OTEL | | | ✅ | ✅ |
| YAML + Admin API + OTEL | ✅ | | ✅ | ✅ |

Все конфигурации собираются с `-Dbuild_profile=release --unity=on -Dunity_size=45`.

Тесты запускаются во всех конфигурациях через `meson test`.

#### Linux (GCC 15, Debian Sid)

Отдельный job в контейнере `debian:sid` с GCC 15. Базовая конфигурация, проверяет совместимость с актуальным компилятором.

#### Windows

| Компилятор | Способ |
|-----------|--------|
| MSVC | vcpkg + `meson setup` |
| Clang (`clang-cl`) | vcpkg + `meson setup` |
| MinGW (MSYS2 / MINGW64) | пакеты MSYS2 |
| GCC (Cygwin) | Base и YAML |
| GCC (WSL / Ubuntu 24.04) | Base и YAML |

#### macOS

Clang, базовая конфигурация. Зависимости устанавливаются через Homebrew.

#### Кэширование в CI

- **OpenTelemetry**: кэшируется собранная библиотека в `/opt/opentelemetry-cpp` по ключу версии `otel-cpp-1.24.0-ubuntu-x64`.
- **vcpkg** (Windows): кэшируются пакеты `C:\vcpkg\installed` и `C:\vcpkg\packages` по хэшу workflow-файла.
- **Cygwin**: кэшируется собранный googletest в `C:\cygwin\usr\local`.

---

### `quick-check.yml` — Fast CI Check

Запускается на каждый push в любую ветку. Собирает только базовую Linux-конфигурацию без тестов:

```
meson setup build -Dbuild_profile=test -Dbuild_tests=false --unity=on -Dunity_size=45
meson compile -C build
```

После сборки проверяется наличие и тип бинарника `build/circle`.

---

## Локальное тестирование

Перед push можно воспроизвести любую CI-конфигурацию локально.

Базовая сборка:

```bash
meson setup build -Dbuild_profile=release --unity=on -Dunity_size=45
meson compile -C build
meson test -C build
```

С YAML:

```bash
meson setup build -Dbuild_profile=release -Dyaml=system --unity=on -Dunity_size=45
meson compile -C build
meson test -C build
```

С SQLite:

```bash
meson setup build -Dbuild_profile=release -Dsqlite=system --unity=on -Dunity_size=45
meson compile -C build
meson test -C build
```

С Admin API и OpenTelemetry:

```bash
meson setup build -Dbuild_profile=release -Dadmin_api=true -Dotel=system --unity=on -Dunity_size=45
meson compile -C build
meson test -C build
```

Запуск конкретного теста:

```bash
build/tests/tests --gtest_filter="TriggersList_F.*"
```

---

## Известные особенности

### Admin API

Admin API работает **только с YML** форматом мира. Unix-сокет создаётся в директории мира (например, `small/admin_api.sock`).

### OpenTelemetry

Библиотека `opentelemetry-cpp` не поставляется в стандартных пакетах Ubuntu, поэтому в CI она собирается из исходников и кэшируется. При локальной сборке с `-Dotel=system` потребуется установить её вручную.

### Windows (MSVC / Clang)

Зависимости управляются через vcpkg. Требуется установленный Developer Command Prompt или переменные окружения MSVC.

### Cygwin

Googletest отсутствует в стандартных пакетах Cygwin и собирается из исходников. Результат кэшируется по версии (текущая: 1.14.0).

### WSL

Поведение идентично чистому Linux. Проверяется на Ubuntu 24.04.

---

## Badges

```markdown
[![Multi-Platform Build](https://github.com/bylins/mud/actions/workflows/build.yml/badge.svg)](https://github.com/bylins/mud/actions/workflows/build.yml)
[![Quick Check](https://github.com/bylins/mud/actions/workflows/quick-check.yml/badge.svg)](https://github.com/bylins/mud/actions/workflows/quick-check.yml)
```