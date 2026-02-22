# Git Hooks

This directory contains git hooks for the project.

## Installation

Run the installation script to set up hooks in your local repository:

```bash
.githooks/install.sh
```

Or configure git to use this directory for hooks (Git 2.9+):

```bash
git config core.hooksPath .githooks
```

## Available Hooks

### pre-commit

Проверяет кодировку файлов перед коммитом. Блокирует коммит если:

1. Найден символ замены U+FFFD (Unicode Replacement Character)
2. Файл должен быть KOI8-R (по .gitattributes), но находится в UTF-8
3. Найдены типичные паттерны битой кодировки

**Как исправить ошибки кодировки:**

```bash
# Вариант 1: Конвертация файла
iconv -f utf-8 -t koi8-r file_utf8.cpp > file.cpp

# Вариант 2: Редактирование через UTF-8
iconv -f koi8-r -t utf-8 file.cpp > /tmp/file_utf8.cpp
# Edit /tmp/file_utf8.cpp
iconv -f utf-8 -t koi8-r /tmp/file_utf8.cpp > file.cpp

# Вариант 3: Срочный коммит (не рекомендуется)
git commit --no-verify
```

## Adding New Hooks

1. Create executable hook file in `.githooks/` directory
2. Run `.githooks/install.sh` to install it locally
3. Commit the hook file so other developers can use it
