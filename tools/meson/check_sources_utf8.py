#!/usr/bin/env python3
"""Убедиться, что исходники в рабочем дереве -- UTF-8 (issue #3681).

Флип на UTF-8 сделан снятием working-tree-encoding с /src и /tests в .gitattributes,
а это преобразование времени checkout'а. Дерево, выкаченное до флипа, при переключении
на ветку остаётся в KOI8-R: содержимое блоба не изменилось, и git молча оставляет файлы
как есть -- git status показывает чистое дерево.

Собранный из такого дерева бинарь получает кои-восьмые строковые литералы, а движок
считает весь текст нативным UTF-8. Дальше расходится всё: сравнения имён, доски,
кодировка лога, а конверсия на диск гонит такие литералы через словарь транслита.

Чинится принудительной перевыкачкой:  rm -rf src tests && git checkout -- src tests
"""

import sys
from pathlib import Path

SUFFIXES = {".cpp", ".h", ".hpp", ".cc", ".inl"}


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    bad = []
    for directory in ("src", "tests"):
        base = root / directory
        if not base.is_dir():
            continue
        for path in base.rglob("*"):
            if path.suffix not in SUFFIXES or not path.is_file():
                continue
            try:
                path.read_bytes().decode("utf-8")
            except UnicodeDecodeError:
                bad.append(path.relative_to(root))

    if not bad:
        return 0

    print(
        f"исходники не в UTF-8: {len(bad)} файлов.\n"
        "\n"
        "Похоже, рабочее дерево выкачено до флипа на UTF-8 (issue #3681): флип сделан\n"
        "снятием working-tree-encoding в .gitattributes, а это преобразование времени\n"
        "checkout'а. При переключении ветки git оставляет такие файлы в KOI8-R и считает\n"
        "дерево чистым -- git status ничего не покажет. Собранный отсюда бинарь получит\n"
        "кои-восьмые строковые литералы, а движок считает весь текст нативным UTF-8.\n"
        "\n"
        "Лечится перевыкачкой:\n"
        "    rm -rf src tests && git checkout -- src tests\n"
        "\n"
        "Например: " + ", ".join(str(x) for x in sorted(bad)[:4])
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
