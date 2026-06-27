/**
 \authors Created by Sventovit
 \date 17.01.2022.
 \brief Умение "ювелир" - заголовок.
*/

#ifndef BYLINS_SRC_CRAFTS_JEWELRY_H_
#define BYLINS_SRC_CRAFTS_JEWELRY_H_

#include "engine/boot/cfg_manager.h"

#include <string>
#include <unordered_map>
#include <vector>

class CharData;
class ObjData;

namespace jewelry {

// Один эффект, который можно вплавить в предмет:
//   type 1 - apply  (id = EApply,   val = модификатор)
//   type 2 - affect (id = EAffect)
//   type 3 - флаг   (id = EObjFlag)
//   type 4 - умение (id = ESkill,   val = процент)
// key - имя, по которому игрок выбирает эффект: игровое название эффекта (apply_types/
// affect_msg/extra_bits/название умения), а не внутренний id-токен. Может быть переопределено
// атрибутом alias="" в конфиге.
struct GemEffect {
	int type = 0;
	int id = 0;
	int val = 0;
	std::string key;
};

// Конфиг умения "ювелир" (cfg/craft/jewelry.xml). Значения по умолчанию могут
// быть изменены при чтении файла.
struct JewelryCfg {
	int lag = 1;                                  // лаг (умножается на PULSE_VIOLENCE)
	int skill_divisor = 1;                        // на столько делится скилл при проверке
	int skill_penalty_for_affect = 15;            // штраф к скиллу за каждый аффект/APPLY на предмете
	int target_obj_crash_percent = 10;            // шанс испортить предмет при неудаче
	int target_obj_timer_increment_percent = 10;  // на сколько % растет таймер своего предмета
	int target_obj_timer_decrement_percent = 10;  // на сколько % падает таймер чужого предмета
	int desired_min_skill = 80;                   // мин. скилл, чтобы вплавлять желаемый эффект
	int desired_foreign_min_skill = 130;          // мин. скилл, чтобы вплавлять желаемый эффект в чужой предмет
	int desired_success_offset = 50;              // смещение порога успеха при вплавлении желаемого эффекта

	std::unordered_map<int, std::vector<GemEffect>> gems;   // vnum камня -> эффекты

	[[nodiscard]] bool IsGem(int vnum) const;
	[[nodiscard]] const GemEffect *Find(int vnum, const std::string &key) const;
	[[nodiscard]] std::string RandomKey(int vnum) const;
	void Show(CharData *ch, int vnum) const;
};

extern JewelryCfg cfg;

// Загрузка cfg/craft/jewelry.xml через cfg_manager (boot + reload jewelry).
class JewelryLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

} // namespace jewelry

bool is_dig_stone(ObjData *obj);

#endif //BYLINS_SRC_CRAFTS_JEWELRY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
