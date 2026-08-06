#ifndef BYLINS_GRAPH_H
#define BYLINS_GRAPH_H

class CharData;

// complain: жаловаться в лог, если путь не найден. По умолчанию молчим -- ненайденный путь
// это штатный исход почти для всех вызовов (моб выслеживает обидчика, тот ушел в другую зону
// или за !трек). Жалуется только ходьба по маршруту: там недостижимая точка -- ошибка билдера.
int find_first_step(RoomRnum src, RoomRnum target, CharData *ch, bool complain = false);

#endif //BYLINS_GRAPH_H
