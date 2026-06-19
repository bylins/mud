# syntax=docker/dockerfile:1
#
# Образ игрового сервера Bylins (circle). Собирается ИЗ ЛОКАЛЬНОГО исходника.
# Опциональные возможности включаются build-аргументами:
#   WITH_YAML=true       YAML-формат мира (по умолчанию включён)
#   WITH_SQLITE=false    SQLite-формат мира (по умолчанию выключен)
#   WITH_ADMIN_API=true  Admin API (Unix-сокет) — нужен для web-админки
#   WITH_OTEL=true       OpenTelemetry (метрики/трейсы/логи по OTLP HTTP)
# Legacy-формат мира поддерживается всегда, отдельного флага не требует.
#
# Сборка:
#   docker build -t mud-server .
#   docker build -t mud-server --build-arg WITH_OTEL=false .   # без телеметрии (быстрее)
#
# Запуск (turnkey: на вход — каталог мира; entrypoint форсирует admin_api+otel-only):
#   docker run -d --network host \
#     -v /path/to/world:/world \
#     --name mud mud-server
# Циркуль запускается с host-сетью: игровой порт сразу наружу, а OTLP-endpoint
# берётся из configuration.xml мира (localhost:<порт> бьёт в коллектор этого
# сервера). Форсирование отключается через -e FORCE_CONFIG=0.

ARG BUILD_TYPE=release
ARG OTEL_VERSION=1.24.0

# ───────────────────────── Этап 1: сборка ─────────────────────────
FROM alpine:3.23 AS builder
ARG BUILD_TYPE
ARG OTEL_VERSION
ARG WITH_OTEL=true
ARG WITH_ADMIN_API=true
ARG WITH_YAML=true
ARG WITH_SQLITE=false

RUN apk add --no-cache \
    build-base make meson ninja git cmake samurai \
    zlib-dev openssl-dev curl-dev expat-dev pkgconf gettext

# opentelemetry-cpp (OTLP HTTP, статические либы) из исходников с системными
# protobuf/abseil/nlohmann. Отдельный слой — кешируется, не пересобирается при
# правках циркуля. Конфигурация проверена против protobuf 31.x из Alpine 3.23.
RUN if [ "$WITH_OTEL" = "true" ]; then \
      apk add --no-cache protobuf-dev abseil-cpp-dev nlohmann-json linux-headers && \
      git clone --depth 1 -b v${OTEL_VERSION} --recursive \
          https://github.com/open-telemetry/opentelemetry-cpp /src/otel && \
      cmake -S /src/otel -B /src/otel/build -G Ninja \
          -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
          -DWITH_OTLP_HTTP=ON -DWITH_OTLP_GRPC=OFF \
          -DOPENTELEMETRY_INSTALL=ON \
          -DBUILD_TESTING=OFF -DWITH_EXAMPLES=OFF -DWITH_FUNC_TESTS=OFF && \
      cmake --build /src/otel/build -j"$(nproc)" --target install && \
      rm -rf /src/otel ; \
    fi

# SQLite-загрузчик (опционально) использует системную библиотеку.
RUN if [ "$WITH_SQLITE" = "true" ]; then apk add --no-cache sqlite-dev; fi

WORKDIR /mud/mud
COPY . /mud/mud

RUN OTEL_OPT=$([ "$WITH_OTEL" = "true" ] && echo system || echo disabled); \
    ADMIN_OPT=$([ "$WITH_ADMIN_API" = "true" ] && echo true || echo false); \
    YAML_OPT=$([ "$WITH_YAML" = "true" ] && echo builtin || echo disabled); \
    SQLITE_OPT=$([ "$WITH_SQLITE" = "true" ] && echo system || echo disabled); \
    meson setup build \
        -Dbuild_tests=false -Dbuild_profile=${BUILD_TYPE} \
        --unity=on -Dunity_size=45 \
        -Dadmin_api=${ADMIN_OPT} -Dotel=${OTEL_OPT} \
        -Dyaml=${YAML_OPT} -Dsqlite=${SQLITE_OPT} && \
    meson compile -C build circle:executable

# ───────────────────────── Этап 2: рантайм ─────────────────────────
FROM alpine:3.23
ARG WITH_OTEL=true
ARG WITH_SQLITE=false

# Рантайм-зависимости: динамические библиотеки циркуля + python3 для entrypoint.
# protobuf/abseil подтягиваются otel-эксportёром (Alpine отдаёт их shared-либами).
RUN apk add --no-cache libstdc++ openssl curl zlib expat python3 && \
    if [ "$WITH_OTEL" = "true" ]; then apk add --no-cache protobuf abseil-cpp; fi && \
    if [ "$WITH_SQLITE" = "true" ]; then apk add --no-cache sqlite-libs; fi

COPY --from=builder /mud/mud/build/circle /usr/local/bin/circle
COPY --from=builder /mud/mud/tools/docker/entrypoint.py /usr/local/bin/entrypoint.py

# Значения по умолчанию (переопределяются окружением/compose).
ENV WORLD_DIR=/world \
    MUD_PORT=4000 \
    FORCE_CONFIG=1 \
    ADMIN_SOCKET_REL=run/admin_api.sock

EXPOSE 4000
VOLUME /world

ENTRYPOINT ["python3", "/usr/local/bin/entrypoint.py"]
