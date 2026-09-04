#!/usr/bin/env python3
"""Перевести файлы состояния сервера (lib/state) из KOI8-R в UTF-8 (issue #3787).

Пара к tools/convert_cfg_to_utf8.py, но разбор построчный, а не пофайловый, и вот почему.
Списки в state/ движок читает через native_text::from_disk_line, то есть решает о кодировке
для КАЖДОЙ строки отдельно: валидная UTF-8 строка считается уже переведённой, всё остальное
разбирается как KOI8-R. Скрипт повторяет ровно это правило, поэтому:

  * файл, переведённый наполовину, доводится до конца, а не портится повторным переводом
    (так может выглядеть список, в который движок дописывал строки уже нативными);
  * результат совпадает с тем, что прочитает движок -- расхождения между конвертером и
    загрузчиком быть не может по построению.

Двоичные файлы (statistics/mob_stat.bin и всё, где есть нулевой байт) не трогаются.

ВАЖНО: сервер должен быть остановлен. Эти файлы пишутся в рантайме, и сохранение поверх
свежепереведённого файла старым бинарём вернёт его в KOI8-R.

    ./tools/convert_state_to_utf8.py lib/state            # посмотреть
    ./tools/convert_state_to_utf8.py lib/state --apply    # перевести
"""

import argparse
import os
import re
import sys

# Объявление кодировки правим только в шапке XML (unique_mobs.xml, statistics/*.xml).
DECLARATION = re.compile(rb'^(<\?xml[^>]*encoding\s*=\s*")([^"]+)(")', re.IGNORECASE)


def convert_line(line: bytes) -> tuple[bytes, str]:
    """Перевести одну строку. Возвращает (строка, что с ней): ascii/native/recoded."""
    try:
        line.decode('ascii')
        return line, 'ascii'
    except UnicodeDecodeError:
        pass
    try:
        line.decode('utf-8')
        return line, 'native'          # уже переведена -- не трогаем
    except UnicodeDecodeError:
        pass
    return line.decode('koi8-r').encode('utf-8'), 'recoded'


def fix_declaration(data: bytes) -> tuple[bytes, bool]:
    """encoding="koi8-r" -> encoding="utf-8" в шапке XML."""
    match = DECLARATION.match(data)
    if not match or match.group(2).lower() in (b'utf-8', b'utf8'):
        return data, False
    return DECLARATION.sub(rb'\g<1>utf-8\g<3>', data, count=1), True


def write_atomically(path: str, data: bytes) -> None:
    """Запись через временный файл рядом: прерывание не оставит половину файла."""
    temp = path + '.utf8.tmp'
    with open(temp, 'wb') as handle:
        handle.write(data)
    os.replace(temp, path)


def main() -> int:
    parser = argparse.ArgumentParser(description='Перевод файлов состояния из KOI8-R в UTF-8')
    parser.add_argument('root', help='каталог состояния (обычно lib/state)')
    parser.add_argument('--apply', action='store_true', help='записать изменения (иначе только показать)')
    parser.add_argument('--quiet', action='store_true', help='не перечислять каждый файл')
    args = parser.parse_args()

    if not os.path.isdir(args.root):
        print(f'нет такого каталога: {args.root}', file=sys.stderr)
        return 2

    binary = touched = already = 0
    native_lines = 0

    for dirpath, _, names in os.walk(args.root):
        for name in sorted(names):
            path = os.path.join(dirpath, name)
            relative = os.path.relpath(path, args.root)
            try:
                with open(path, 'rb') as handle:
                    data = handle.read()
            except OSError as error:
                print(f'  не прочитать {relative}: {error}', file=sys.stderr)
                continue

            if b'\0' in data:
                binary += 1
                if not args.quiet:
                    print(f'  {relative}  [двоичный, пропущен]')
                continue

            data, declaration_fixed = fix_declaration(data)
            out = bytearray()
            recoded = native = 0
            for index, line in enumerate(data.split(b'\n')):
                if index:
                    out += b'\n'
                tail = b''
                if line.endswith(b'\r'):          # CRLF: перевод не должен его съесть
                    line, tail = line[:-1], b'\r'
                converted, what = convert_line(line)
                out += converted + tail
                if what == 'recoded':
                    recoded += 1
                elif what == 'native':
                    native += 1

            if not recoded and not declaration_fixed:
                already += 1
                continue
            native_lines += native   # считаем только там, где что-то меняли

            marks = []
            if recoded:
                marks.append(f'строк переведено: {recoded}')
            if native:
                marks.append(f'уже нативных: {native}')
            if declaration_fixed:
                marks.append('шапка')
            touched += 1
            if not args.quiet:
                print(f'  {relative}  [{", ".join(marks)}]')
            if args.apply:
                write_atomically(path, bytes(out))

    print()
    print(f"{'переведено файлов:' if args.apply else 'будет переведено файлов:':<26} {touched}")
    print(f"{'уже в UTF-8:':<26} {already}")
    print(f"{'двоичных (пропущено):':<26} {binary}")
    if native_lines:
        print()
        print(f'Внутри переведённых файлов {native_lines} строк уже были нативными -- они оставлены как есть.')

    if not args.apply and touched:
        print()
        print('Это был просмотр. Чтобы записать, повторите с --apply')
        print('Сервер при этом должен быть остановлен.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
