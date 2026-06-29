#!/bin/sh
#
# Entrypoint образа циркуля.
#
# Аргументы после имени образа передаются циркулю как есть, поэтому работает
# простой запуск без перезаписи entrypoint:
#
#   docker run -d -p 127.0.0.1:4000:4000 -v /abs/world/lib:/world circle-min -d /world 4000
#   docker run ... circle-min -- -d /world 4000     # ведущий "--" тоже допустим
#   docker run ... circle-min                        # без аргументов -> circle -d $WORLD_DIR $MUD_PORT
#
# Привилегии: контейнер стартует root'ом только чтобы СБРОСИТЬ их. Циркуль
# запускается под владельцем смонтированного мира (или PUID:PGID, если задан) —
# тогда чтение идёт от нужного uid, а файлы, записанные циркулем в мир, остаются
# за тем же владельцем на хосте, а не за root/случайным uid. Без CAP/setuid в
# самом циркуле.
#
# Форсирование конфига (admin_api + otel для combined-стека) выполняется только
# при FORCE_CONFIG=on (по умолчанию off — минимальный образ его не трогает).
set -eu

# Разрешаем ведущий "--" (docker run img -- -d /world 4000).
if [ "${1:-}" = "--" ]; then
	shift
fi

WORLD_DIR="${WORLD_DIR:-/world}"
MUD_PORT="${MUD_PORT:-4000}"
SOCKET_REL="${ADMIN_SOCKET_REL:-run/admin_api.sock}"

if [ ! -d "$WORLD_DIR" ]; then
	echo "[entrypoint] FATAL: каталог мира не найден: $WORLD_DIR" >&2
	echo "[entrypoint]        смонтируйте его: -v /abs/path/world/lib:/world" >&2
	exit 1
fi

# Циркуль пишет syslog и пр. относительно рабочего каталога. CWD по умолчанию /
# (root-only), и непривилегированный процесс туда писать не может — уходим в мир,
# он принадлежит тому же uid, под которым побежит circle.
cd "$WORLD_DIR"

# Кто будет владельцем процесса: PUID/PGID при наличии, иначе владелец /world.
RUNNER=""
if [ "$(id -u)" = "0" ]; then
	uid="${PUID:-$(stat -c '%u' "$WORLD_DIR")}"
	gid="${PGID:-$(stat -c '%g' "$WORLD_DIR")}"
	if [ "$uid" = "0" ]; then
		echo "[entrypoint] WARN: владелец $WORLD_DIR — root, разделения привилегий нет." >&2
		echo "[entrypoint]       смонтируйте мир, принадлежащий не-root юзеру, или задайте PUID/PGID." >&2
	else
		echo "[entrypoint] circle побежит под uid:gid $uid:$gid (владелец $WORLD_DIR)" >&2
		RUNNER="su-exec $uid:$gid"
	fi
fi

# Опциональный форс конфига (combined-стек). По умолчанию выключен.
case "$(printf '%s' "${FORCE_CONFIG:-0}" | tr 'A-Z' 'a-z')" in
	1 | true | yes | on)
		echo "[entrypoint] FORCE_CONFIG=on — настраиваю мир (admin_api + otel-only)" >&2
		# shellcheck disable=SC2086
		$RUNNER python3 /usr/local/bin/configure_world.py "$WORLD_DIR" "$SOCKET_REL"
		;;
esac

if [ "$#" -gt 0 ]; then
	set -- circle "$@"
else
	set -- circle -d "$WORLD_DIR" "$MUD_PORT"
fi

echo "[entrypoint] exec: ${RUNNER:+$RUNNER }$*" >&2
# shellcheck disable=SC2086
exec $RUNNER "$@"
