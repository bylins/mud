/**
 \authors Created by Sventovit
 \date 17.01.2022.
 \brief Умение "горное дело" - заголовок.
*/

#ifndef BYLINS_SRC_CRAFTS_MINING_H_
#define BYLINS_SRC_CRAFTS_MINING_H_

#include "engine/boot/cfg_manager.h"

#include <vector>

class CharData;

const int kHolesTime = 1;

namespace mining {

// Один ранг камня: настоящий самоцвет (stone_vnum), его стеклянная подделка
// (glass_vnum) и минимальный навык, при котором камень начинает выпадать
// (он же шанс выпадения). Ранги хранятся по возрастанию навыка.
struct DigStone {
	int rank = 0;
	int stone_vnum = 0;
	int glass_vnum = 0;
	int skill = 0;
};

// Конфиг умения "горное дело" (cfg/mechanics/digging.xml). Значения по умолчанию
// могут быть изменены при чтении файла.
struct DiggingCfg {
	int hole_max_deep = 10;       // максимальная глубина ямки
	int tool_crash_chance = 2;    // шанс инструменту сломаться
	int treasure_chance = 30000;  // шанс выкопать клад
	int jackpot_vnum = 99958;     // шкатулка Пандоры
	int jackpot_chance = 180000;  // шанс выкопать шкатулку Пандоры
	int mob_chance = 300;         // шанс выкопать моба
	int trash_chance = 200;       // шанс выкопать всякую фигню
	int lag = 2;                  // лаг (в коде умножается на PULSE_VIOLENCE)
	int skill_divisor = 3;        // на столько делится скилл, чтобы сложнее копалось
	int glass_chance = 3;         // шанс выкопать драг.камень, иначе стекло
	int moves_expense = 10;       // мувов на один копок

	std::vector<DigStone> stones; // ранги камней (по возрастанию навыка)
	std::vector<int> mob_vnums;   // мобы, которых можно выкопать
	std::vector<int> trash_vnums; // мусор, который можно выкопать
};

extern DiggingCfg dig_cfg;

// Загрузка cfg/mechanics/digging.xml через cfg_manager (boot + reload digging).
class DiggingLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

} // namespace mining

void do_dig(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CRAFTS_MINING_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
