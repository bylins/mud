#!/usr/bin/env python3
"""
Форсирование configuration.xml смонтированного мира под combined-стек
(circle + web-admin): admin_api=on (сокет в run/), telemetry=on (otel-only).

Вызывается из docker-entrypoint.sh ТОЛЬКО при FORCE_CONFIG=on. Сам циркуль
здесь не запускается — этим занимается entrypoint после дропа привилегий.

OTLP-endpoint НЕ трогаем — берётся из конфига мира как есть (при network_mode=host
localhost:<порт> бьёт в коллектор этого сервера).

Правки делаются побайтово (latin-1), чтобы не трогать KOI8-R и сохранить
комментарии. Оригинал один раз сохраняется в configuration.xml.orig.

Usage: configure_world.py <world_dir> [socket_rel]
"""
import os
import re
import sys


def log(msg):
    print(f"[configure-world] {msg}", flush=True)


def force_block(text, tag, body):
    """Заменяет содержимое <tag>...</tag>; если блока нет — вставляет перед </configuration>."""
    pattern = re.compile(rf"<{tag}>.*?</{tag}>", re.DOTALL)
    if pattern.search(text):
        return pattern.sub(body, text, count=1)
    log(f"<{tag}> отсутствует — добавляю блок")
    return text.replace("</configuration>", body + "\n</configuration>", 1)


def edit_within(text, tag, edits):
    """Применяет регэкс-правки edits=[(pattern, repl)] только внутри блока <tag>...</tag>."""
    pattern = re.compile(rf"(<{tag}>)(.*?)(</{tag}>)", re.DOTALL)
    m = pattern.search(text)
    if not m:
        return text, False
    inner = m.group(2)
    for pat, repl in edits:
        inner = re.sub(pat, repl, inner, flags=re.DOTALL)
    return text[: m.start()] + m.group(1) + inner + m.group(3) + text[m.end():], True


def force_config(cfg_path, socket_rel):
    if not os.path.exists(cfg_path):
        log(f"ВНИМАНИЕ: {cfg_path} не найден — форсирование пропущено")
        return

    raw = open(cfg_path, "rb").read()
    text = raw.decode("latin-1")  # byte-preserving
    original = text

    admin_block = (
        "\t<admin_api>\n"
        "\t\t<enabled>true</enabled>\n"
        f"\t\t<socket_path>{socket_rel}</socket_path>\n"
        "\t</admin_api>"
    )
    text, found = edit_within(
        text,
        "admin_api",
        [
            (r"<enabled>.*?</enabled>", "<enabled>true</enabled>"),
            (r"<socket_path>.*?</socket_path>", f"<socket_path>{socket_rel}</socket_path>"),
        ],
    )
    if not found:
        text = force_block(text, "admin_api", admin_block)

    text, found = edit_within(
        text,
        "telemetry",
        [
            (r"<enabled>.*?</enabled>", "<enabled>true</enabled>"),
            (r"<mode>.*?</mode>", "<mode>otel-only</mode>"),
        ],
    )
    if not found:
        log("ВНИМАНИЕ: <telemetry> в конфиге мира нет — телеметрия не настроена, "
            "форсирую только admin_api")

    if text == original:
        log("конфиг уже соответствует — изменений нет")
        return

    orig_backup = cfg_path + ".orig"
    if not os.path.exists(orig_backup):
        open(orig_backup, "wb").write(raw)
        log(f"оригинал сохранён в {orig_backup}")
    open(cfg_path, "wb").write(text.encode("latin-1"))
    log(f"конфиг обновлён: admin_api=on socket={socket_rel} telemetry logs=otel-only "
        "(endpoint оставлен из конфига мира)")


def main():
    world_dir = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("WORLD_DIR", "/world")
    socket_rel = sys.argv[2] if len(sys.argv) > 2 else os.environ.get("ADMIN_SOCKET_REL", "run/admin_api.sock")
    force_config(os.path.join(world_dir, "misc", "configuration.xml"), socket_rel)


if __name__ == "__main__":
    main()
