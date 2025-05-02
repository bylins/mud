# использование:
# docker build -t mud-server --build-arg BUILD_TYPE=Test .
# docker run -d -p 4000:4000 -e MUD_PORT=4000 -v ./lib:/mud/lib --name mud mud-server

# Этап 1: Сборка проекта
FROM alpine:3.20 AS builder

RUN apk add --no-cache \
    build-base \
    make \
    cmake \
    git \
    openssl-dev \
    curl-dev \
    expat-dev \
    gettext

WORKDIR /mud
RUN git clone --recurse-submodules https://github.com/bylins/mud mud

WORKDIR /mud/mud
RUN git checkout alpine
RUN cp -r lib.template/* lib
RUN mkdir build
WORKDIR /mud/mud/build

ARG BUILD_TYPE=Test
ENV BUILD_TYPE=${BUILD_TYPE}
RUN cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. && \
    make -j$(nproc)

# Этап 2: Итоговый контейнер для запуска
FROM alpine:3.20

RUN apk add --no-cache \
    libstdc++ \
    openssl \
    curl \
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