# использование:
# docker build -t mud-server --build-arg BUILD_TYPE=test .
# docker run -d -p 4000:4000 -e MUD_PORT=4000 -v ./lib:/mud/lib --name mud mud-server

# Этап 1: Сборка проекта
FROM alpine:3.23 AS builder

RUN apk add --no-cache \
    build-base make cmake\
    meson ninja git \
    zlib-dev openssl-dev \
    curl-dev expat-dev \
    pkgconf gettext

WORKDIR /mud
RUN git clone https://github.com/bylins/mud mud

WORKDIR /mud/mud
RUN cp -r lib.template/* lib
RUN find subprojects -type d -empty -delete

ARG BUILD_TYPE=test
ENV BUILD_TYPE=${BUILD_TYPE}

RUN meson setup build -Dbuild_tests=false -Dbuild_profile=${BUILD_TYPE} --unity=on -Dunity_size=45
RUN meson compile -C build

# Этап 2: Итоговый контейнер для запуска
FROM alpine:3.23

RUN apk add --no-cache \
    libstdc++ \
    openssl \
    curl \
    zlib \
    expat \
    python3 \
    gdb

WORKDIR /mud
COPY --from=builder /mud/mud/build/circle /mud/circle
VOLUME /mud/lib

ARG MUD_PORT=4000
ENV MUD_PORT=${MUD_PORT}
EXPOSE ${MUD_PORT}

CMD /bin/sh -c "echo Running circle with MUD_PORT=${MUD_PORT}; ./circle ${MUD_PORT} 2>&1"