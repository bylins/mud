#ifndef BYLINS_COMMON_H
#define BYLINS_COMMON_H

#include "engine/entities/char_data.h"

inline bool IsUnableToAct(CharData *ch) {
	return (AFF_FLAGGED(ch, EAffect::kStopFight) ||
	AFF_FLAGGED(ch, EAffect::kMagicStopFight) ||
	AFF_FLAGGED(ch, EAffect::kHold));
}

// Урон после частичной защиты, которую игра называет "немного" (уклонился, отклонил, отразил):
// две трети. Держим одной названной функцией, а не выражением на месте: четыре умения считали
// это каждое у себя, и один рефакторинг разом превратил все четыре в полный промах (#2764 ->
// `dam *= 10 / 15`, где целочисленное деление даёт ноль).
inline int SlightDefenceDamage(int dam) {
	return dam * 10 / 15;
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
