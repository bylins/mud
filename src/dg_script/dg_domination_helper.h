#pragma once

#include "dg_scripts.h"

// всё это можно реализовать и на dg скрипте, но работает оно крайне медленно
// читает следующие переменные из текущего контекста: round, mobs_count, debug_messages
// список мобов и комнат:
// "hares", "hares_rooms"
// "falcons", "falcons_rooms"
// "bears", "bears_rooms"
// "lions", "lions_rooms"
// "wolves", "wolves_rooms"
// "lynxes", "lynxes_rooms"
// "bulls", "bulls_rooms"
// "snakes", "snakes_rooms"
// после чтения всех переменные загружает мобов по комнатам согласно mobs_count
void process_arena_round(Script *sc, Trigger *trig, char *cmd);

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
