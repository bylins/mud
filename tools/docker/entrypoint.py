#!/usr/bin/env python3
"""
Entrypoint для образа циркуля (combined-стек circle + web-admin).

По умолчанию форсирует в configuration.xml смонтированного мира то, что нужно
для турнкей-запуска:
  - admin_api: enabled=true, socket_path в выделенном каталоге (run/admin_api.sock);
  - telemetry: enabled=true, logs mode=otel-only.

OTLP-endpoint по умолчанию НЕ трогаем — он берётся из конфига мира как есть.
Циркуль запущен с network_mode=host, поэтому localhost:<порт> из конфига бьёт
прямо в локальный приёмник этого сервера. На agent-хостах локальный приём сидит
на стандартном порту (4318), и конфиг мира уже верен. Исключение — gateway-хост:
там стандартный 4318 занят ВНЕШНИМ TLS-приёмником, а локальный plaintext-приём —
на 4319. Чтобы один и тот же образ работал на обоих, на gateway-хосте задают
OTEL_LOCAL_PORT (=4319), и entrypoint переписывает порт OTLP-endpoint'ов под него.
Без этой переменной поведение прежнее.

Форсирование отключается переменной FORCE_CONFIG=0 (тогда берётся конфиг мира как есть).

Правки делаются на уровне байт (latin-1), чтобы не трогать KOI8-R и сохранить
комментарии. Оригинал один раз сохраняется в configuration.xml.orig.
"""
import os
import re
import sys


def log(msg):
    print(f"[entrypoint] {msg}", flush=True)


def env_bool(name, default):
    val = os.environ.get(name)
    if val is None:
        return default
    return val.strip().lower() in ("1", "true", "yes", "on")


def force_block(text, tag, body):
    """Заменяет содержимое <tag>...</tag>; если блока нет — вставляет body перед </configuration>."""
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

    # --- admin_api: enabled + выделенный каталог сокета ---
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

    # --- telemetry: enabled + otel-only (порт endpoint'а — см. OTEL_LOCAL_PORT ниже).
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

    # Опционально переписать порт OTLP-endpoint'ов под локальный приёмник хоста.
    # По умолчанию (OTEL_LOCAL_PORT не задан) порт берётся из конфига мира — на
    # agent-хостах стандартный 4318 уже верен. На gateway-хосте 4318 занят внешним
    # TLS-приёмником, а локальный plaintext — на 4319; там задают OTEL_LOCAL_PORT,
    # и тот же образ работает прозрачно на любом сервере.
    local_port = os.environ.get("OTEL_LOCAL_PORT", "").strip()
    if local_port and not local_port.isdigit():
        log(f"ВНИМАНИЕ: OTEL_LOCAL_PORT={local_port!r} не число — порт не изменён")
        local_port = ""
    if local_port:
        text, port_found = edit_within(
            text,
            "telemetry",
            [(r"((?:localhost|127\.0\.0\.1):)\d+", rf"\g<1>{local_port}")],
        )
        if port_found:
            log(f"telemetry: OTLP-порт переписан на {local_port} (OTEL_LOCAL_PORT)")
        else:
            log("ВНИМАНИЕ: <telemetry> нет — OTEL_LOCAL_PORT не применён")

    if text == original:
        log("конфиг уже соответствует — изменений нет")
        return

    orig_backup = cfg_path + ".orig"
    if not os.path.exists(orig_backup):
        open(orig_backup, "wb").write(raw)
        log(f"оригинал сохранён в {orig_backup}")
    open(cfg_path, "wb").write(text.encode("latin-1"))
    log(f"конфиг обновлён: admin_api=on socket={socket_rel} telemetry logs=otel-only"
        + (f" otlp-port={local_port}" if local_port else " (endpoint из конфига мира)"))


def main():
    world_dir = os.environ.get("WORLD_DIR", "/world")
    port = os.environ.get("MUD_PORT", "4000")
    socket_rel = os.environ.get("ADMIN_SOCKET_REL", "run/admin_api.sock")

    if not os.path.isdir(world_dir):
        log(f"FATAL: каталог мира не найден: {world_dir} (смонтируйте его и задайте WORLD_DIR)")
        sys.exit(1)

    if env_bool("FORCE_CONFIG", True):
        log("FORCE_CONFIG=on — гарантирую admin_api + otel-only")
        force_config(os.path.join(world_dir, "cfg", "configuration.xml"), socket_rel)
    else:
        log("FORCE_CONFIG=off — использую configuration.xml мира как есть")

    cmd = ["circle", "-d", world_dir, port]
    log("exec: " + " ".join(cmd))
    os.execvp(cmd[0], cmd)


if __name__ == "__main__":
    main()
