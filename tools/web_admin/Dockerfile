# Bylins MUD Web Admin — небольшой образ для запуска web-интерфейса Admin API.
#
# Сборка (контекст — каталог tools/web_admin):
#   docker build -t mud-web-admin tools/web_admin
#
# Запуск (монтируем КАТАЛОГ с сокетом, а не сам файл — см. ниже):
#   docker run -d --name mud-web-admin \
#     -p 127.0.0.1:5000:5000 \
#     -v /opt/mud/lib/run:/mud/socket:ro \
#     --user "$(id -u):$(id -g)" \
#     mud-web-admin
#
# В configuration.xml циркуля держите сокет в выделенном каталоге, напр.:
#   <admin_api><enabled>true</enabled><socket_path>run/admin_api.sock</socket_path></admin_api>
# Каталог run/ циркуль создаёт автоматически при старте.
#
# Логи (запросы + ошибки аутентификации) идут в stdout/stderr:
#   docker logs -f mud-web-admin
#
# Почему монтируется каталог, а не файл сокета:
#   При рестарте циркуль делает unlink()+bind() и создаёт сокет с НОВЫМ inode.
#   Bind-mount одного файла фиксирует старый inode — после рестарта в контейнере
#   останется «протухший» сокет и потребуется перезапуск контейнера. Монтирование
#   каталога показывает контейнеру актуальный файл сразу после пересоздания.
#
# Сокет создаётся с правами 0600 (owner-only) — контейнер ОБЯЗАН работать под тем
# же UID, что и циркуль, поэтому почти всегда нужен --user (см. ниже).

FROM python:3.12-alpine

# Не пишем .pyc и не буферизуем вывод — иначе логи задерживаются в `docker logs`.
ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1

WORKDIR /app

# Зависимости (Flask/Werkzeug/gunicorn — чистый Python, компиляция не нужна).
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Код приложения.
COPY app.py mud_client.py ./
COPY templates ./templates
COPY static ./static

# Непривилегированный пользователь по умолчанию. На практике для доступа к сокету
# почти всегда нужно переопределить UID/GID под владельца сокета через
# `docker run --user "$(id -u):$(id -g)"`.
RUN adduser -D -u 10001 webadmin
USER webadmin

EXPOSE 5000

ENV MUD_SOCKET=/mud/socket/admin_api.sock \
    FLASK_HOST=0.0.0.0 \
    FLASK_PORT=5000 \
    LOG_LEVEL=INFO

# gunicorn: один воркер (общий секрет сессий без FLASK_SECRET_KEY), несколько
# потоков для параллельных запросов. Access- и error-лог — в stdout/stderr.
CMD ["gunicorn", \
     "--bind", "0.0.0.0:5000", \
     "--workers", "1", \
     "--threads", "4", \
     "--access-logfile", "-", \
     "--error-logfile", "-", \
     "app:app"]
