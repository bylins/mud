#!/usr/bin/env python3
"""Перевести файлы мира из KOI8-R в UTF-8 (issue #3787).

Идемпотентен: файл, который уже валидный UTF-8, считается переведённым и не трогается.
Это та же логика, по которой различает кодировку сам движок (native_text::from_disk_text),
поэтому повторный прогон безопасен, а прерванный — можно просто повторить.

По умолчанию только показывает, что будет сделано. Для записи нужен --apply.

    ./tools/convert_world_to_utf8.py <каталог>            # посмотреть
    ./tools/convert_world_to_utf8.py <каталог> --apply    # перевести

Двоичные файлы и файлы, не разбирающиеся ни как UTF-8, ни как KOI8-R, пропускаются
и перечисляются в конце: их надо посмотреть руками, а не переводить вслепую.
"""

import argparse
import os
import sys

# Расширения, которые в мире содержат текст. Всё остальное (в том числе .bin) не трогаем.
TEXT_SUFFIXES = {".yaml", ".yml", ".lst", ".xml", ".txt", ".hlp", ".template"}


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


def main() -> int:
    parser = argparse.ArgumentParser(description="Перевод файлов мира из KOI8-R в UTF-8")
    parser.add_argument("root", help="каталог мира")
    parser.add_argument("--apply", action="store_true", help="записать изменения (иначе только показать)")
    parser.add_argument("--quiet", action="store_true", help="не перечислять каждый файл")
    args = parser.parse_args()

    if not os.path.isdir(args.root):
        print(f"нет такого каталога: {args.root}", file=sys.stderr)
        return 2

    counts = {"utf8": 0, "koi8": 0, "binary": 0, "unknown": 0, "skipped": 0}
    unknown_files = []
    converted = 0

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
            if kind != "koi8":
                continue

            if not args.quiet:
                print(f"  {os.path.relpath(path, args.root)}")
            if args.apply:
                text = data.decode("koi8-r").encode("utf-8")
                # запись через временный файл рядом: прерывание не оставит половину файла
                temp = path + ".utf8.tmp"
                with open(temp, "wb") as handle:
                    handle.write(text)
                os.replace(temp, path)
            converted += 1

    print()
    print(f"уже в UTF-8:      {counts['utf8']}")
    print(f"{'переведено:' if args.apply else 'будет переведено:':<17} {converted}")
    print(f"не текст:         {counts['binary']}")
    print(f"пропущено (не тот суффикс): {counts['skipped']}")

    if unknown_files:
        print()
        print(f"НЕ РАЗОБРАЛ {len(unknown_files)} файлов -- посмотрите руками, они не тронуты:")
        for name in unknown_files[:20]:
            print(f"    {name}")
        if len(unknown_files) > 20:
            print(f"    ... и ещё {len(unknown_files) - 20}")
        return 1

    if not args.apply and converted:
        print()
        print("Это был просмотр. Чтобы записать, повторите с --apply")
    return 0


if __name__ == "__main__":
    sys.exit(main())
