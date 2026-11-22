#ifndef __MSDP_CONSTANTS_HPP_
#define __MSDP_CONSTANTS_HPP_

#include <string>

namespace msdp {
namespace constants {
extern const std::string ROOM;
extern const std::string EXPERIENCE;
extern const std::string GOLD;
extern const std::string LEVEL;
extern const std::string MAX_HIT;
extern const std::string MAX_MOVE;
extern const std::string STATE;
extern const std::string GROUP;

// Имена, принятые в MSDP у остальных мадов. Это договорённость, а не стандарт, и выполнять
// её целиком мы не обязаны -- берём то, от чего игроку польза. Данные те же, что мы и так
// отдаём, но плоскими переменными и под именем, которое сторонний клиент уже знает: текущие
// жизнь, бодрость и мана лежали у нас внутри своей таблицы STATE, максимумы -- под своими
// MAX_HIT и MAX_MOVE. Исторические имена остаются: по ним уже работают клиенты игроков.
// Денег тут намеренно нет: сколько у игрока в кошельке и в банке -- его дело, и под своим
// именем GOLD это читают только клиенты, писанные под Былины.
extern const std::string HEALTH;
extern const std::string HEALTH_MAX;
extern const std::string MOVEMENT;
extern const std::string MOVEMENT_MAX;
extern const std::string MANA;
extern const std::string MANA_MAX;
extern const std::string CHARACTER_NAME;
extern const std::string SERVER_ID;
extern const std::string SERVER_TIME;
extern const std::string ALIGNMENT;
extern const std::string EXPERIENCE_TNL;
extern const std::string WORLD_TIME;
extern const std::string OPPONENT_NAME;
extern const std::string OPPONENT_LEVEL;
extern const std::string OPPONENT_HEALTH;
extern const std::string OPPONENT_HEALTH_MAX;

constexpr char TELOPT_MSDP = 69;
}
}

#endif // __MSDP_CONSTANTS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
