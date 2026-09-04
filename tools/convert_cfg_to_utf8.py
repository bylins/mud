#!/usr/bin/env python3
"""Перевести конфиги (lib/cfg) из KOI8-R в UTF-8 (issue #3787).

Делает две вещи, обе идемпотентно:

  1. Переводит содержимое из KOI8-R в UTF-8. Файл, который уже валидный UTF-8, считается
     переведённым и не трогается -- это та же логика, по которой различает кодировку сам
     движок (native_text::from_disk_text), поэтому повторный прогон безопасен, а прерванный
     можно просто повторить.
  2. Правит объявление кодировки в шапке XML: encoding="koi8-r" -> encoding="utf-8".
     Это содержимое файла, а не его кодировка, поэтому у трекаемых в git файлов правка
     нужна отдельно от снятия working-tree-encoding.

По умолчанию только показывает, что будет сделано. Для записи нужен --apply.

    ./tools/convert_cfg_to_utf8.py lib/cfg            # посмотреть
    ./tools/convert_cfg_to_utf8.py lib/cfg --apply    # перевести

Файлы, не разбирающиеся ни как UTF-8, ни как KOI8-R, пропускаются и перечисляются в конце:
их надо посмотреть руками, а не переводить вслепую.
"""

import argparse
import os
import re
import sys

# Текстовые расширения конфигов. Всё остальное не трогаем.
TEXT_SUFFIXES = {".xml", ".scheme", ".lst", ".txt"}

# Объявление кодировки ищем только в шапке XML, а не по всему файлу: строка encoding="koi8-r"
# может встретиться и в тексте настроек, трогать её нельзя.
DECLARATION = re.compile(r'^(<\?xml[^>]*encoding\s*=\s*")([^"]+)(")', re.IGNORECASE)


def classify(data: bytes) -> str:
    """utf8 -- уже переведён; koi8 -- надо перевести; binary -- не текст; unknown -- не понял."""
    if b"\0" in data:
        return "binary"
    try:
        data.decode("utf-8")
        return "utf8"
    except UnicodeDecodeError:
        pass
    try:
        data.decode("koi8-r")
        return "koi8"
    except UnicodeDecodeError:
        return "unknown"


def fix_declaration(text: str) -> tuple[str, bool]:
    """Заменить кодировку в объявлении XML на utf-8. Возвращает (текст, менялось ли)."""
    match = DECLARATION.match(text)
    if not match or match.group(2).lower() in ("utf-8", "utf8"):
        return text, False
    return DECLARATION.sub(r"\g<1>utf-8\g<3>", text, count=1), True


def write_atomically(path: str, data: bytes) -> None:
    """Запись через временный файл рядом: прерывание не оставит половину файла."""
    temp = path + ".utf8.tmp"
    with open(temp, "wb") as handle:
        handle.write(data)
    os.replace(temp, path)


def main() -> int:
    parser = argparse.ArgumentParser(description="Перевод конфигов из KOI8-R в UTF-8")
    parser.add_argument("root", help="каталог конфигов (обычно lib/cfg)")
    parser.add_argument("--apply", action="store_true", help="записать изменения (иначе только показать)")
    parser.add_argument("--quiet", action="store_true", help="не перечислять каждый файл")
    args = parser.parse_args()

    if not os.path.isdir(args.root):
        print(f"нет такого каталога: {args.root}", file=sys.stderr)
        return 2

    counts = {"utf8": 0, "koi8": 0, "binary": 0, "unknown": 0, "skipped": 0}
    unknown_files = []
    recoded = 0
    redeclared = 0

    for dirpath, _, names in os.walk(args.root):
        for name in sorted(names):
            path = os.path.join(dirpath, name)
            if os.path.splitext(name)[1].lower() not in TEXT_SUFFIXES:
                counts["skipped"] += 1
                continue
            try:
                with open(path, "rb") as handle:
                    data = handle.read()
            except OSError as error:
                print(f"  не прочитать {path}: {error}", file=sys.stderr)
                continue

            kind = classify(data)
            counts[kind] += 1
            if kind == "unknown":
                unknown_files.append(os.path.relpath(path, args.root))
                continue
            if kind == "binary":
                continue

            text = data.decode("koi8-r" if kind == "koi8" else "utf-8")
            text, declaration_fixed = fix_declaration(text)
            if kind != "koi8" and not declaration_fixed:
                continue

            marks = []
            if kind == "koi8":
                recoded += 1
                marks.append("кодировка")
            if declaration_fixed:
                redeclared += 1
                marks.append("объявление")
            if not args.quiet:
                print(f"  {os.path.relpath(path, args.root)}  [{', '.join(marks)}]")
            if args.apply:
                write_atomically(path, text.encode("utf-8"))

    print()
    print(f"уже в UTF-8:       {counts['utf8']}")
    print(f"{'переведено:' if args.apply else 'будет переведено:':<18} {recoded}")
    print(f"{'шапок поправлено:' if args.apply else 'шапок поправить:':<18} {redeclared}")
    print(f"не текст:          {counts['binary']}")
    print(f"пропущено (не тот суффикс): {counts['skipped']}")

    if unknown_files:
        print()
        print(f"НЕ РАЗОБРАЛ {len(unknown_files)} файлов -- посмотрите руками, они не тронуты:")
        for name in unknown_files[:20]:
            print(f"    {name}")
        if len(unknown_files) > 20:
            print(f"    ... и ещё {len(unknown_files) - 20}")
        return 1

    if not args.apply and (recoded or redeclared):
        print()
        print("Это был просмотр. Чтобы записать, повторите с --apply")
    return 0


if __name__ == "__main__":
    sys.exit(main())
