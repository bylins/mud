#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""create_test_world.py — создаёт тестовый мир из выбранного набора зон.

Питоновская замена старого create_test_world.pl с поддержкой обоих форматов
мира:

  * legacy — классический формат CircleMUD: подпапки wld/obj/mob/zon/trg,
             файлы вида <vnum>.<ext> и текстовый index в каждой подпапке.
  * yaml   — новый формат: zones/<vnum>/{rooms,mobs,objects,triggers}/*.yaml,
             общий каталог dictionaries/ и список зон в zones/index.yaml.

Формат определяется автоматически по содержимому каталога мира либо задаётся
ключом --format. Если в одном каталоге присутствуют оба формата (как в
lib/world), формат нужно указать явно.

Логика выбора зон повторяет старый скрипт:
  зона копируется, если она есть в --include ЛИБО (её нет в --exclude И она
  попадает в диапазон --min..--max). Списки зон поддерживают диапазоны:
  «4,10-20,40» — это зоны 4, 10..20 включительно и 40.

Примеры (из корня репозитория, мир берётся из lib/world по умолчанию):
  # тестовый мир из зон 1, 40 и 820 (legacy)
  tools/create_test_world.py --format legacy --include 1,40,820 -o world.test

  # все зоны из диапазона 600..699 плюс обязательные стартовые (yaml)
  tools/create_test_world.py --format yaml --min 600 --max 699 --include 1,40 -o world.test
"""

import argparse
import os
import re
import shutil
import sys
from pathlib import Path

# Подпапки legacy-мира: имя подпапки совпадает с расширением файлов внутри неё.
LEGACY_SUBDIRS = ("wld", "obj", "mob", "zon", "trg")


def parse_zone_list(text):
    """Разбирает строку вида «4,10-20,40» в множество номеров зон."""
    zones = set()
    for part in text.split(","):
        part = part.strip()
        if not part:
            continue
        m = re.fullmatch(r"(\d+)\s*-\s*(\d+)", part)
        if m:
            lo, hi = int(m.group(1)), int(m.group(2))
            if lo > hi:
                lo, hi = hi, lo
            zones.update(range(lo, hi + 1))
        elif part.isdigit():
            zones.add(int(part))
        else:
            raise ValueError(f"не могу разобрать номер зоны: {part!r}")
    return zones


def select_zones(available, include, exclude, zmin, zmax):
    """Возвращает отсортированный список зон, которые нужно скопировать."""
    selected = set()
    for z in available:
        in_range = z not in exclude and zmin <= z <= zmax
        if z in include or in_range:
            selected.add(z)
    missing = sorted(z for z in include if z not in available)
    if missing:
        print(f"ПРЕДУПРЕЖДЕНИЕ: зоны из --include не найдены в мире: "
              f"{', '.join(map(str, missing))}", file=sys.stderr)
    return sorted(selected)


def detect_format(world_dir):
    """Определяет формат мира по содержимому каталога."""
    has_yaml = (world_dir / "zones" / "index.yaml").is_file()
    has_legacy = (world_dir / "wld").is_dir()
    if has_yaml and has_legacy:
        sys.exit("В каталоге присутствуют оба формата (legacy и yaml). "
                 "Укажите формат явно ключом --format.")
    if has_yaml:
        return "yaml"
    if has_legacy:
        return "legacy"
    sys.exit(f"Не удалось определить формат мира в {world_dir}. "
             "Укажите --format legacy|yaml.")


# --------------------------------------------------------------------------- #
# legacy
# --------------------------------------------------------------------------- #

def discover_legacy_zones(world_dir):
    """Собирает номера зон, для которых есть хоть один legacy-файл."""
    zones = set()
    for sub in LEGACY_SUBDIRS:
        sub_dir = world_dir / sub
        if not sub_dir.is_dir():
            continue
        for f in sub_dir.glob(f"*.{sub}"):
            m = re.fullmatch(rf"(\d+)\.{sub}", f.name)
            if m:
                zones.add(int(m.group(1)))
    return zones


def build_legacy(world_dir, out_dir, selected):
    for sub in LEGACY_SUBDIRS:
        src_dir = world_dir / sub
        if not src_dir.is_dir():
            continue
        dst_dir = out_dir / sub
        dst_dir.mkdir(parents=True, exist_ok=True)
        copied = []
        for z in selected:
            src_file = src_dir / f"{z}.{sub}"
            if src_file.is_file():
                shutil.copy2(src_file, dst_dir / src_file.name)
                copied.append(z)
        copied.sort()
        # Формат index как в старом perl-скрипте: список файлов и завершающие «$».
        with open(dst_dir / "index", "w", encoding="koi8-r", newline="\n") as idx:
            for z in copied:
                idx.write(f"{z}.{sub}\n")
            idx.write("$\n$\n\n")
        print(f"  {sub}: скопировано зон {len(copied)}")


# --------------------------------------------------------------------------- #
# yaml
# --------------------------------------------------------------------------- #

def discover_yaml_zones(world_dir):
    """Собирает номера зон по подкаталогам zones/<vnum>/."""
    zones = set()
    zones_dir = world_dir / "zones"
    if zones_dir.is_dir():
        for d in zones_dir.iterdir():
            if d.is_dir() and d.name.isdigit():
                zones.add(int(d.name))
    return zones


def build_yaml(world_dir, out_dir, selected):
    # Словари общие для всего мира — копируем целиком.
    src_dict = world_dir / "dictionaries"
    if src_dict.is_dir():
        shutil.copytree(src_dict, out_dir / "dictionaries", dirs_exist_ok=True)
        print("  dictionaries: скопированы")

    # Общий конфиг мира, если есть.
    cfg = world_dir / "world_config.yaml"
    if cfg.is_file():
        shutil.copy2(cfg, out_dir / "world_config.yaml")

    zones_out = out_dir / "zones"
    zones_out.mkdir(parents=True, exist_ok=True)
    copied = []
    for z in selected:
        src = world_dir / "zones" / str(z)
        if src.is_dir():
            shutil.copytree(src, zones_out / str(z), dirs_exist_ok=True)
            copied.append(z)
    copied.sort()

    # zones/index.yaml — простой список номеров зон (содержимое чисто ASCII).
    with open(zones_out / "index.yaml", "w", encoding="koi8-r", newline="\n") as f:
        f.write("zones:\n")
        for z in copied:
            f.write(f"- {z}\n")
    print(f"  zones: скопировано {len(copied)}, записан zones/index.yaml")


# --------------------------------------------------------------------------- #

def main(argv=None):
    # Скрипт лежит в tools/, мир по умолчанию — lib/world рядом с корнем репозитория.
    default_world = Path(__file__).resolve().parent.parent / "lib" / "world"
    parser = argparse.ArgumentParser(
        description="Создаёт тестовый мир из выбранного набора зон.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("-w", "--world-dir", type=Path, default=default_world,
                        help="каталог исходного мира (по умолчанию lib/world)")
    parser.add_argument("-o", "--output", type=Path, default=Path("world.test"),
                        help="каталог для тестового мира (по умолчанию ./world.test)")
    parser.add_argument("-f", "--format", choices=("auto", "legacy", "yaml"),
                        default="auto", help="формат мира (по умолчанию auto)")
    parser.add_argument("--min", type=int, default=1,
                        help="минимальный номер зоны диапазона (по умолчанию 1)")
    parser.add_argument("--max", type=int, default=0,
                        help="максимальный номер зоны диапазона (по умолчанию 0, "
                             "то есть диапазон пуст и копируются только --include)")
    parser.add_argument("--include", type=parse_zone_list, default=set(),
                        help="зоны, которые копируются всегда (напр. «1,40,820»)")
    parser.add_argument("--exclude", type=parse_zone_list, default={0},
                        help="зоны, которые не копировать (по умолчанию «0»)")
    parser.add_argument("--clean", action="store_true",
                        help="очистить каталог вывода перед копированием")
    args = parser.parse_args(argv)

    world_dir = args.world_dir.resolve()
    if not world_dir.is_dir():
        sys.exit(f"Каталог мира не найден: {world_dir}")

    fmt = args.format
    if fmt == "auto":
        fmt = detect_format(world_dir)

    out_dir = args.output
    if args.clean and out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    if fmt == "legacy":
        available = discover_legacy_zones(world_dir)
    else:
        available = discover_yaml_zones(world_dir)

    selected = select_zones(available, args.include, args.exclude, args.min, args.max)
    if not selected:
        sys.exit("Не выбрано ни одной зоны — проверьте --include/--min/--max.")

    print(f"Формат: {fmt}; исходный мир: {world_dir}")
    print(f"Выбрано зон: {len(selected)} -> {out_dir}")

    if fmt == "legacy":
        build_legacy(world_dir, out_dir, selected)
    else:
        build_yaml(world_dir, out_dir, selected)

    print("Готово.")


if __name__ == "__main__":
    main()
