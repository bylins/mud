# Локальное тестирование GitHub Actions

## Установка act

**act** — инструмент для локального запуска GitHub Actions workflows.

### Linux
```bash
# Через curl (рекомендуется)
curl -s https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# Или через apt
sudo apt install act

# Проверка установки
act --version
```

### Требования
- Docker должен быть установлен и запущен
- ~2–3 GB места для Docker-образов

---

## Использование

### Просмотр доступных workflows и jobs
```bash
# Список всех workflows
act -l

# Список jobs в конкретном workflow
act -l -W .github/workflows/quick-check.yml
```

### Запуск quick-check (самый быстрый способ проверить сборку)
```bash
act -j quick-build -W .github/workflows/quick-check.yml
```

### Запуск Linux matrix из build.yml

`build.yml` использует reusable workflows (`workflow_call`), которые act поддерживает ограниченно. Надёжнее запускать подчинённые workflow напрямую:

```bash
# Базовая конфигурация
act -j matrix -W .github/workflows/_linux.yml

# Конкретная матричная конфигурация
act -j matrix -W .github/workflows/_linux.yml --matrix config.name:"Base"
act -j matrix -W .github/workflows/_linux.yml --matrix config.name:"YAML"
act -j matrix -W .github/workflows/_linux.yml --matrix config.name:"SQLite"
act -j matrix -W .github/workflows/_linux.yml --matrix config.name:"Base + Admin API + OTEL"
act -j matrix -W .github/workflows/_linux.yml --matrix config.name:"YAML + Admin API + OTEL"

# GCC 15 (в контейнере debian:sid)
act -j gcc15 -W .github/workflows/_linux.yml

# Coverage
act -j coverage -W .github/workflows/_linux.yml
```

### Dry-run (без реального выполнения)
```bash
act -n
act -n -W .github/workflows/_linux.yml
```

### Debug mode
```bash
act -v
act -v -W .github/workflows/_linux.yml
```

---

## Ограничения

act не может запустить все jobs:

- Windows jobs (MSVC, Clang, MinGW) — требуют Windows runner
- Cygwin — требует Windows
- WSL — требует Windows
- MacOS — требует MacOS runner
- Linux jobs — работают полностью
- GCC 15 (`debian:sid` контейнер) — работает, но образ скачивается отдельно
- Coverage job — требует `lcov` внутри образа

---

## Рекомендуемый порядок проверки

**1. Быстрая проверка (dry-run, ~5 секунд)**
```bash
act -n -W .github/workflows/quick-check.yml
```

**2. Полная быстрая сборка (~2–5 минут)**
```bash
act -j quick-build -W .github/workflows/quick-check.yml
```

**3. Конкретная Linux-конфигурация**
```bash
act -j matrix -W .github/workflows/_linux.yml --matrix config.name:"YAML"
```

---

## Ускорение

После первого запуска Docker-образы кэшируются локально:
- Первый запуск: ~5–10 минут (скачивание образа)
- Последующие: ~2–3 минуты

В `.actrc` можно зафиксировать образ:
```
-P ubuntu-latest=catthehacker/ubuntu:act-latest
```

---

## Примеры

### Проверка изменений в workflow перед push
```bash
# 1. Редактируем .github/workflows/_linux.yml
# 2. Тестируем локально
act -j matrix -W .github/workflows/_linux.yml --matrix config.name:"Base"

# 3. Если всё ОК — коммитим
git add .github/workflows/
git commit -m "ci: update linux matrix"
git push
```

### Отладка падающего job
```bash
act -j matrix -W .github/workflows/_linux.yml --matrix config.name:"YAML" -v
```

---

## Troubleshooting

### Docker permission denied
```bash
sudo usermod -aG docker $USER
newgrp docker
```

### Нехватка места
```bash
docker system prune -a
```

### Job не запускается
```bash
# Проверить синтаксис workflow
act -n -W .github/workflows/_linux.yml

# Проверить что Docker запущен
docker ps
```

---

## Полезные ссылки

- act documentation: https://github.com/nektos/act
- Docker images для act: https://github.com/catthehacker/docker_images
- GitHub Actions docs: https://docs.github.com/en/actions