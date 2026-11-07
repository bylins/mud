#!/usr/bin/env python3
"""Перенос внумов в данных игроков после смены номера зоны.

Переносят зоны примерно раз в десять лет, так что к следующему разу не помнит никто.
Поэтому здесь подробно: и зачем, и где именно лежат внумы, и на чем легко обжечься.

Что делают ПОМИМО этого скрипта (сам он правит только userdata):

  1. мир -- каталог зоны, ее внумы, zones/index.yaml, выходы и триггеры чужих зон;
  2. lib/cfg -- shops.xml, specials.xml, global_drop.xml и cases.xml (последний НЕ
     отслеживается git, на боевом его правят руками);
  3. клановые файлы, если переезжает клан-замок: Rent:, ChestRoom:, Guard: и шапки
     досок Clan: -- их правит этот скрипт, но знать про них надо;
  4. замена по миру должна быть ЯКОРЁНАЯ, по целому числу: голый поиск "829" портит
     97829 в зоне 978, 12829 в 128 и top_room 58299 в 582.

Самое неприятное -- числа той же формы, которые внумами НЕ являются. Проверено на
живых данных: у зоны 400 из 351 вхождения снаружи 349 оказались литералом 40000 на
полях experience, timer, bonus, cost и rent_off; "eval questnum 40007" в триггере --
номер квеста из сейвов игроков, его смена позволила бы сдать квест заново всем, кто
его сдал; в userdata та же форма сидит в Tmer, Ctmr, Curr и Plyd.

ЧТО ЛОМАЕТСЯ при переносе рабочей зоны -- это и забывается в первую очередь. Цифры
ниже настоящие, с боевого архива от 07.11.2026, когда переехали 400, 405, 406 и 829:

  вещи у игроков          17 экземпляров 13 прототипов у 15 владельцев, из них один
                          клановый сундук. Прототипа больше нет, и вещь при загрузке
                          не создается -- то есть пропадает;
  списки убитых мобов     8094 записи по 63 мобам, до 428 игроков на одного моба.
                          Отбрасываются молча, счет по зоне у всех обнуляется;
  комната выхода          9 персонажей стояли в ренте переехавшего клан-замка
                          (Room: 40616). Внума больше нет;
  лог                     "неизвестный прототип объекта" без номера и без стека, по
                          которому не понять ничего (поправлено в bylins/mud#4155).

Вещи из служебной зоны терять не жалко, из рабочей -- жалко: там носильные предметы.
Так что решение простое: служебную зону переносим без скрипта, рабочую -- со скриптом,
и это требует остановки мада, потому что пфайлы правятся на диске.

Нужен ли скрипт вообще. Если зона была служебной, предметов у игроков почти нет, и
данные чистятся сами: опись хранилища пропускает ненайденный внум (depot.cpp), общий
загрузчик вещей пишет "Object (V) N does not exist in database" и вещь не создает
(obj_save.cpp), а MobMax::load отбрасывает запись о неизвестном мобе молча. При
следующей записи файла мертвых строк в нем уже нет. Скрипт нужен, когда вещи терять
нельзя -- то есть когда переезжает рабочая зона.

Зачем. Перенос зоны правит мир и конфиги, но не userdata: предметы зоны остаются
у игроков в инвентаре, ренте, хранилище и клановых сундуках, а их прототипов больше
нет. Движок на такой внум отвечает "неизвестный прототип объекта", а загрузка вещи
пишет "Object (V) N does not exist in database" и ВЫБРАСЫВАЕТ вещь.

Правим только те поля, которые действительно держат внум:

  *.obj  -- строка "#<внум>" (прототип и родитель) и "Ozne: <номер зоны>~"
  *.player -- "Room: <внум комнаты>"
  clans/<к>/<к> -- "Rent:", "ChestRoom:", "Guard:"
  clans/<к>/*.board -- "Clan: <внум>" в шапке

НЕ трогаем поля, где число той же формы -- не внум: Tmer, Ctmr, Curr, Cost, Maxx,
Weig, Ouid, Plyd, опыт и деньги. Списки убитых мобов в .player не правим тоже:
счетчик по несуществующему мобу безвреден, а строки там без имени поля, и спутать
их с чем-то другим легко. Все такие места печатаются в отчет -- глянуть глазами.

Запуск:
  renumber_userdata.py --userdata lib/userdata 400:397 405:399 406:398 829:407
  ... и тем же самым с --apply, когда отчет устроит.
"""

import argparse
import pathlib
import re
import shutil
import sys
import time
from collections import Counter

# Поля, которые правим. Ключ -- как поле выглядит в файле, значение -- что в нем лежит.
OBJ_VNUM_LINE = re.compile(r'^#(\d+)\s*$')
OBJ_ZONE_LINE = re.compile(r'^(Ozne:\s*)(\d+)(~?)\s*$')
ROOM_LINE = re.compile(r'^(Room\s*:\s*)(\d+)\s*$')
CLAN_LINES = re.compile(r'^(Rent:|ChestRoom:|Guard:)(\s*)(\d+)\s*$')
BOARD_CLAN = re.compile(r'(\bClan:\s*)(\d+)\b')


def build_maps(pairs):
    """Из пар 400:397 делаем две таблицы: по внумам сущностей и по номерам зон."""
    zones, ents = {}, {}
    for p in pairs:
        old, new = (int(x) for x in p.split(':'))
        zones[old] = new
        ents[old] = new
    return zones, ents


def move_entity(vnum, ents):
    """Внум сущности: первые цифры -- зона, последние две -- номер внутри зоны."""
    zone, rel = divmod(vnum, 100)
    return ents[zone] * 100 + rel if zone in ents else None


def patch_file(path, zones, ents, stats, report):
    try:
        text = path.read_text(encoding='utf-8', errors='surrogateescape')
    except OSError as e:
        report.append(f'!! не прочитан {path}: {e}')
        return None

    kind = path.suffix
    out, changed = [], 0
    for lineno, line in enumerate(text.split('\n'), 1):
        new_line = line

        m = OBJ_VNUM_LINE.match(line)
        if m:
            moved = move_entity(int(m.group(1)), ents)
            if moved is not None:
                new_line = f'#{moved}'

        if new_line == line:
            m = OBJ_ZONE_LINE.match(line)
            if m and int(m.group(2)) in zones:
                new_line = f'{m.group(1)}{zones[int(m.group(2))]}{m.group(3)}'

        if new_line == line:
            m = ROOM_LINE.match(line)
            if m:
                moved = move_entity(int(m.group(2)), ents)
                if moved is not None:
                    new_line = f'{m.group(1)}{moved}'

        if new_line == line:
            m = CLAN_LINES.match(line)
            if m:
                moved = move_entity(int(m.group(3)), ents)
                if moved is not None:
                    new_line = f'{m.group(1)}{m.group(2)}{moved}'

        if new_line == line and '.board' in path.name:
            def sub_clan(mm):
                moved = move_entity(int(mm.group(2)), ents)
                return f'{mm.group(1)}{moved}' if moved is not None else mm.group(0)
            new_line = BOARD_CLAN.sub(sub_clan, line)

        if new_line != line:
            changed += 1
            stats[f'{kind or path.name}'] += 1
        out.append(new_line)

    return '\n'.join(out) if changed else None


def scan_leftovers(path, ents, leftovers):
    """Числа старой формы на полях, которые мы намеренно не правим."""
    pat = re.compile(r'(?<![0-9])(' + '|'.join(str(z) for z in ents) + r')(\d\d)(?![0-9])')
    try:
        text = path.read_text(encoding='utf-8', errors='surrogateescape')
    except OSError:
        return
    for line in text.split('\n'):
        if not pat.search(line):
            continue
        field = re.match(r'\s*([A-Za-z_]+)\s*:', line)
        name = field.group(1) if field else ('#прототип' if line.startswith('#') else 'строка-без-поля')
        if name in ('Room', 'Ozne', 'Rent', 'ChestRoom', 'Guard', '#прототип'):
            continue
        leftovers[(path.suffix or path.name, name)] += 1


def main():
    ap = argparse.ArgumentParser(description='Перенос внумов в userdata после смены номера зоны.')
    ap.add_argument('pairs', nargs='+', metavar='СТАРАЯ:НОВАЯ', help='например 829:407')
    ap.add_argument('--userdata', required=True, help='каталог userdata')
    ap.add_argument('--apply', action='store_true', help='писать изменения (по умолчанию только отчет)')
    ap.add_argument('--backup', default=None, help='куда сложить копию перед правкой')
    args = ap.parse_args()

    root = pathlib.Path(args.userdata)
    if not root.is_dir():
        sys.exit(f'нет каталога {root}')
    zones, ents = build_maps(args.pairs)

    if args.apply:
        dst = pathlib.Path(args.backup or f'{root}.before-renumber-{time.strftime("%Y%m%d-%H%M%S")}')
        if dst.exists():
            sys.exit(f'{dst} уже есть, выберите другое имя')
        print(f'копия: {dst}')
        shutil.copytree(root, dst)

    stats, leftovers, report = Counter(), Counter(), []
    touched = 0
    for path in sorted(root.rglob('*')):
        if not path.is_file():
            continue
        patched = patch_file(path, zones, ents, stats, report)
        if patched is not None:
            touched += 1
            if args.apply:
                path.write_text(patched, encoding='utf-8', errors='surrogateescape')
        scan_leftovers(path, ents, leftovers)

    print(f'\nпереносы: ' + ', '.join(f'{o}->{n}' for o, n in sorted(zones.items())))
    print(f'файлов затронуто: {touched}')
    print('правок по виду файла:')
    for k, v in sorted(stats.items()):
        print(f'   {k}: {v}')
    if not stats:
        print('   нет -- либо уже перенесено, либо не тот каталог')
    if leftovers:
        print('\nчисла старой формы на полях, которые НЕ правим (глянуть глазами):')
        for (kind, field), n in sorted(leftovers.items()):
            print(f'   {kind} [{field}]: {n}')
    for line in report:
        print(line)
    if not args.apply:
        print('\nэто пробный прогон; повторите с --apply, когда отчет устроит')


if __name__ == '__main__':
    main()
