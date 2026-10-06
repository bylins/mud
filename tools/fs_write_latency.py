#!/usr/bin/env python3
"""Замер задержек записи мелкого файла -- проба диска под сейвы (issue #4014).

Зачем. В профиле save_char одна из двух записей сейва регулярно стоит 20-35 мс,
хотя файлы крошечные, диск свободен и смонтирован обычно (rw,relatime). Проба
делает ровно то же, что делает сейв -- открывает файл с усечением, пишет пару
сотен байт, закрывает, -- и показывает распределение задержек. Движок при этом не
участвует, так что ответ получается однозначный.

Как читать результат:

  * медиана в десятки микросекунд и ни одного выброса -- запись в этом каталоге
    дешёвая, и задержку в сейве создаёт что-то своё: соседние потоки записи,
    конкуренция за диск, работа самого движка;
  * редкие выбросы по 20-35 мс -- дело не в движке: закрытие файла попадает в
    окно коммита журнала ext4 (по умолчанию раз в пять секунд). Лечится на уровне
    ввода-вывода: уносить запись сейвов с игрового потока, писать реже, менять
    параметр commit= у монтирования.

Файл-проба создаётся в указанном каталоге и удаляется в конце, поэтому каталог
нужен тот самый, в который пишет сервер -- задержки у разных файловых систем и
устройств разные.

Запуск:
  tools/fs_write_latency.py /home/mud/mud/lib/userdata/accounts
  tools/fs_write_latency.py --count 2000 --size 2048 /home/mud/mud/lib/userdata/chardata/characters
  tools/fs_write_latency.py --fsync /home/mud/mud/lib        # с принудительным сбросом на диск
"""

import argparse
import os
import sys
import time


def percentile(values, share):
	"""Значение квантиля в уже отсортированном списке (share от 0 до 1)."""
	if not values:
		return 0.0
	index = min(len(values) - 1, max(0, int(round(share * (len(values) - 1)))))
	return values[index]


def measure(path, count, size, do_fsync):
	"""Делает count записей по size байт, возвращает список задержек в секундах."""
	payload = "x" * size
	delays = []
	for _ in range(count):
		start = time.perf_counter()
		with open(path, "w") as handle:
			handle.write(payload)
			if do_fsync:
				handle.flush()
				os.fsync(handle.fileno())
		delays.append(time.perf_counter() - start)
	return delays


def main():
	parser = argparse.ArgumentParser(
		description="Проба задержек записи мелкого файла в указанном каталоге (issue #4014).")
	parser.add_argument("directory", nargs="?", default=".",
						help="каталог, в который пишет сервер (по умолчанию текущий)")
	parser.add_argument("--count", type=int, default=500, help="сколько записей сделать (по умолчанию 500)")
	parser.add_argument("--size", type=int, default=200, help="размер записи в байтах (по умолчанию 200)")
	parser.add_argument("--threshold", type=float, default=0.02,
						help="порог, выше которого задержка считается выбросом, в секундах (по умолчанию 0.02)")
	parser.add_argument("--fsync", action="store_true",
						help="сбрасывать файл на диск принудительно (сейв этого не делает -- для сравнения)")
	args = parser.parse_args()

	if not os.path.isdir(args.directory):
		print(f"Нет такого каталога: {args.directory}", file=sys.stderr)
		return 1

	path = os.path.join(args.directory, "_fs_write_latency_probe")
	started = time.perf_counter()
	try:
		delays = measure(path, args.count, args.size, args.fsync)
	except OSError as error:
		print(f"Не вышло писать в {path}: {error}", file=sys.stderr)
		return 1
	finally:
		try:
			os.unlink(path)
		except OSError:
			pass
	wall = time.perf_counter() - started

	delays.sort()
	outliers = [d for d in delays if d > args.threshold]
	print(f"каталог      : {args.directory}")
	print(f"записей      : {args.count} по {args.size} байт"
		  + (" (с fsync)" if args.fsync else ""))
	print(f"медиана      : {percentile(delays, 0.5) * 1000:.3f} мс")
	print(f"p90 / p99    : {percentile(delays, 0.9) * 1000:.3f} / {percentile(delays, 0.99) * 1000:.3f} мс")
	print(f"максимум     : {delays[-1] * 1000:.3f} мс")
	print(f"выбросов     : {len(outliers)} из {args.count} дольше {args.threshold * 1000:.0f} мс")
	print(f"всего времени: {wall:.3f} с")
	if outliers:
		worst = ", ".join(f"{d * 1000:.1f}" for d in sorted(outliers, reverse=True)[:10])
		print(f"худшие (мс)  : {worst}")
	return 0


if __name__ == "__main__":
	sys.exit(main())

# vim: ts=4 sw=4 tw=0 noet syntax=python :
