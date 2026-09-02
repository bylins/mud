#!/usr/bin/env python3
"""Перевести данные игроков (lib/userdata) из KOI8-R в UTF-8 (issue #3787).

Третий и последний конвертер после cfg и state, и самый осторожный: здесь лежат сейвы
персонажей, сундуки дружин, доски и биржа.

Разбор построчный, как у convert_state_to_utf8.py и как у самого движка
(native_text::from_disk_line): валидная UTF-8 строка считается уже переведённой, всё
остальное разбирается как KOI8-R. Это не перестраховка -- в живых данных встречаются
файлы, где часть строк уже нативная (метки вещей Clbl/ClCl), и пофайловый перевод их
испортил бы.

Два формата разбираются отдельно:

  * .alias -- хранит ДЛИНЫ строк в байтах ("13\\nгг Хаджи уб=*\\n"), а читатель берёт ровно
    столько байт, сколько записано. Простая перекодировка сдвинула бы все длины и разъехала
    бы файл на первом же русском синониме, поэтому длины пересчитываются.
  * .timeobjs и всё, где есть нулевой байт -- двоичное, не трогаем.

ВАЖНО: сервер должен быть остановлен. И отдельно: перевод меняет байты файлов персонажей,
а от них считается CRC -- после перевода снимки надо сбросить (crcsavecheck), иначе каждый
вход будет поднимать имму ложную тревогу.

    ./tools/convert_userdata_to_utf8.py lib/userdata            # посмотреть
    ./tools/convert_userdata_to_utf8.py lib/userdata --apply    # перевести
"""

import argparse
import os
import sys


def convert_bytes(raw: bytes) -> tuple[bytes, bool]:
    """Перевести строку (без перевода строки). Возвращает (байты, менялось ли)."""
    try:
        raw.decode('ascii')
        return raw, False
    except UnicodeDecodeError:
        pass
    try:
        raw.decode('utf-8')
        return raw, False          # уже нативная -- не трогаем
    except UnicodeDecodeError:
        pass
    return raw.decode('koi8-r').encode('utf-8'), True


def convert_lines(data: bytes) -> tuple[bytes, int]:
    """Построчный перевод. Возвращает (данные, сколько строк переведено)."""
    out = bytearray()
    recoded = 0
    for index, line in enumerate(data.split(b'\n')):
        if index:
            out += b'\n'
        tail = b''
        if line.endswith(b'\r'):       # CRLF: перевод не должен его съесть
            line, tail = line[:-1], b'\r'
        converted, changed = convert_bytes(line)
        out += converted + tail
        recoded += changed
    return bytes(out), recoded


def convert_alias(data: bytes) -> tuple[bytes, int]:
    """Перевод файла синонимов с пересчётом байтовых длин.

    Формат (см. WriteAliases/ReadAliases в alias.cpp), повторяется до конца файла:
        <длина синонима>\\n<синоним>\\n<длина замены>\\n<замена>\\n<тип>\\n

    Бросает ValueError, если файл не разобрался: такой лучше оставить нетронутым и
    посмотреть руками, чем угадывать.
    """
    out = bytearray()
    recoded = 0
    pos = 0
    while pos < len(data):
        if data[pos:] == b'\n':                     # хвостовой перевод строки
            out += b'\n'
            break
        fields = []
        for _ in range(2):
            end = data.find(b'\n', pos)
            if end < 0:
                raise ValueError('нет длины')
            declared = int(data[pos:end])           # ValueError, если не число
            pos = end + 1
            value = data[pos:pos + declared]
            if len(value) != declared:
                raise ValueError('строка короче объявленной длины')
            pos += declared
            if data[pos:pos + 1] != b'\n':
                raise ValueError('после строки нет перевода строки')
            pos += 1
            converted, changed = convert_bytes(value)
            recoded += changed
            fields.append(converted)
        end = data.find(b'\n', pos)
        if end < 0:
            raise ValueError('нет типа синонима')
        kind = int(data[pos:end])
        pos = end + 1
        for value in fields:
            out += str(len(value)).encode('ascii') + b'\n' + value + b'\n'
        out += str(kind).encode('ascii') + b'\n'
    return bytes(out), recoded


def write_atomically(path: str, data: bytes) -> None:
    """Запись через временный файл рядом: прерывание не оставит половину файла."""
    temp = path + '.utf8.tmp'
    with open(temp, 'wb') as handle:
        handle.write(data)
    os.replace(temp, path)


def main() -> int:
    parser = argparse.ArgumentParser(description='Перевод данных игроков из KOI8-R в UTF-8')
    parser.add_argument('root', help='каталог данных (обычно lib/userdata)')
    parser.add_argument('--apply', action='store_true', help='записать изменения (иначе только показать)')
    parser.add_argument('--verbose', action='store_true', help='перечислять каждый файл')
    args = parser.parse_args()

    if not os.path.isdir(args.root):
        print(f'нет такого каталога: {args.root}', file=sys.stderr)
        return 2

    binary = touched = already = 0
    lines_recoded = 0
    aliases = 0
    unparsed = []

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
                continue

            is_alias = name.endswith('.alias')
            try:
                converted, recoded = convert_alias(data) if is_alias else convert_lines(data)
            except ValueError as error:
                unparsed.append((relative, str(error)))
                continue

            if converted == data:
                already += 1
                continue

            touched += 1
            lines_recoded += recoded
            aliases += is_alias
            if args.verbose:
                print(f'  {relative}  [строк: {recoded}]' + ('  [синонимы: длины пересчитаны]' if is_alias else ''))
            if args.apply:
                write_atomically(path, converted)

    print()
    print(f"{'переведено файлов:' if args.apply else 'будет переведено файлов:':<26} {touched}")
    print(f"{'   из них .alias:':<26} {aliases}")
    print(f"{'строк переведено:':<26} {lines_recoded}")
    print(f"{'уже в UTF-8:':<26} {already}")
    print(f"{'двоичных (пропущено):':<26} {binary}")

    if unparsed:
        print()
        print(f'НЕ РАЗОБРАЛ {len(unparsed)} файлов -- они НЕ тронуты, посмотрите руками:')
        for relative, why in unparsed[:20]:
            print(f'    {relative}: {why}')
        if len(unparsed) > 20:
            print(f'    ... и ещё {len(unparsed) - 20}')
        return 1

    if not args.apply and touched:
        print()
        print('Это был просмотр. Чтобы записать, повторите с --apply')
        print('Сервер при этом должен быть остановлен, а после -- сбросить CRC (crcsavecheck).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
