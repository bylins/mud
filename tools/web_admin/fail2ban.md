# Защита админ-панели через fail2ban

При включённом Admin API сервер пишет неудачные попытки входа в админ-панель
(неверный пароль при авторизации через сайт) в отдельный лог
`accesadminpanel.log` в каталоге логов сервера (`log_dir` из конфигурации).

Формат строки (язык-нейтральный, удобен для разбора):

```
19-09-25 14:30:05 :: accesadminpanel: login failed ip=203.0.113.7 user='вася'
```

IP берётся из поля `client_ip` запроса `auth`, которое проставляет веб-панель
(`tools/web_admin`): за обратным прокси — первый адрес из `X-Forwarded-For`,
иначе `remote_addr`. Если фронт IP не передал, в логе будет `unix-socket`.

## Фильтр fail2ban

`/etc/fail2ban/filter.d/accesadminpanel.conf`:

```ini
[Definition]
failregex = ^.*accesadminpanel: login failed ip=<HOST> user=
# В логе дата в формате DD-MM-YY HH:MM:SS — это нестандартно для fail2ban,
# поэтому datepattern задаём явно.
datepattern = ^%%d-%%m-%%y %%H:%%M:%%S
```

## Jail

`/etc/fail2ban/jail.d/accesadminpanel.conf`:

```ini
[accesadminpanel]
enabled  = true
filter   = accesadminpanel
logpath  = /path/to/mud/log/accesadminpanel.log
maxretry = 5
findtime = 600
bantime  = 3600
```

Путь `logpath` укажите на реальный `accesadminpanel.log` сервера.

## Проверка фильтра

```bash
fail2ban-regex /path/to/mud/log/accesadminpanel.log \
    /etc/fail2ban/filter.d/accesadminpanel.conf
```
