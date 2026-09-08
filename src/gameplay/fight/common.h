#ifndef BYLINS_COMMON_H
#define BYLINS_COMMON_H

#include "engine/entities/char_data.h"

inline bool IsUnableToAct(CharData *ch) {
	return (AFF_FLAGGED(ch, EAffect::kStopFight) ||
	AFF_FLAGGED(ch, EAffect::kMagicStopFight) ||
	AFF_FLAGGED(ch, EAffect::kHold));
}

int IsHaveNoExtraAttack(CharData *ch);

void SetWait(CharData *ch, int waittime, int wait_if_fight);
void SetSkillCooldown(CharData *ch, ESkill skill, int pulses);
void SetSkillCooldownInFight(CharData *ch, ESkill skill, int pulses);
/**
 * Найти цель боевого умения в комнате. Без аргумента берётся текущий противник.
 *
 * Форма с target_name отдаёт разобранное имя наружу -- его ждёт check_pkill, требующий от
 * игрока полного имени жертвы. Раньше имя передавалось через глобальный буфер arg, который
 * FindVictim заполняла как побочный эффект (#3807).
 */
CharData *FindVictim(CharData *ch, const std::string &argument, std::string &target_name);
CharData *FindVictim(CharData *ch, const std::string &argument);

#endif //BYLINS_COMMON_H
