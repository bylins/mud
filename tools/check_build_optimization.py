#!/usr/bin/env python3
"""Проверить, с какой оптимизацией собраны движок и подпроекты.

Подпроекты (fmt, yaml-cpp, zlib, gtest, luajit) получают свои флаги от meson при ПЕРВОЙ
конфигурации каталога сборки. Если каталог старше, чем строка с default_options в meson.build,
то библиотека так и останется собранной без -O -- а это, например, пятикратная разница на
fmt::format, которым движок печатает почти всё.

    tools/check_build_optimization.py [каталог_сборки]     # по умолчанию build

Чинится без полной пересборки:

    meson configure <каталог> -Dfmt:optimization=2 && ninja -C <каталог>
"""

import json
import os
import sys

# Кого проверяем и какой оптимизации ждём как минимум.
WANT = {"fmt": 2, "yaml-cpp": 2, "zlib": 2, "gtest": 2, "luajit": 2, "sol2": 2,
        "emmyluacodestyle": 2}

LEVELS = {"0": 0, "1": 1, "2": 2, "3": 3, "g": 1, "s": 2, "fast": 3}


def flags_of(entry: dict) -> list:
    command = entry.get("command") or " ".join(entry.get("arguments", []))
    return [token for token in command.split() if token.startswith("-O")]


def level_of(flags: list) -> int:
    if not flags:
        return -1
    return max(LEVELS.get(flag[2:], 0) for flag in flags)


def main() -> int:
    build_dir = sys.argv[1] if len(sys.argv) > 1 else "build"
    path = os.path.join(build_dir, "compile_commands.json")
    try:
        with open(path, encoding="utf-8") as handle:
            entries = json.load(handle)
    except OSError as error:
        print(f"не прочитать {path}: {error}", file=sys.stderr)
        return 2

    seen = {}
    engine = None
    for entry in entries:
        # Путь бывает и относительным (subprojects/fmt/...), и абсолютным.
        source = entry.get("file", "").replace("\\", "/")
        marker = "subprojects/"
        index = source.find(marker)
        if index >= 0:
            name = source[index + len(marker):].split("/", 1)[0]
            seen.setdefault(name, flags_of(entry))
        elif engine is None and "libcircle" in source:
            engine = flags_of(entry)

    print(f"каталог сборки: {build_dir}")
    print(f"  {'движок':<12} {' '.join(engine) if engine else '(не найден)'}")

    bad = []
    for name, flags in sorted(seen.items()):
        want = WANT.get(name)
        mark = ""
        if want is not None and level_of(flags) < want:
            mark = "  <-- без оптимизации!"
            bad.append(name)
        print(f"  {name:<12} {' '.join(flags) if flags else '(нет -O)'}{mark}")

    if bad:
        print()
        print("Собрано без оптимизации: " + ", ".join(bad))
        print("Чинится без полной пересборки:")
        print("    meson configure %s %s && ninja -C %s"
              % (build_dir, " ".join(f"-D{name}:optimization=2" for name in bad), build_dir))
        return 1

    print()
    print("Всё в порядке.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
