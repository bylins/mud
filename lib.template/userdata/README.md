# userdata/

Data about **users / accounts and their characters' cross-session mail** Б─■ the
people playing, as distinct from the character save files themselves (`plrs/`,
`plrobjs/`, Б─╕) and from server-wide state (`state/`).

Migrated here from `etc/` (issue.misc-migrate):

| Path | Read/written by | Purpose |
|------|-----------------|---------|
| `plrmail.xml` | `mail.cpp` (`MAIL_XML_FILE`) | pending (unread) in-game player mail; created/rewritten at runtime |
| `boards/*.board` | `boards.cpp` (`BOARDS_DIR`) | message boards (general/news/idea/error/god-*/Б─╕); created/rewritten at runtime |
