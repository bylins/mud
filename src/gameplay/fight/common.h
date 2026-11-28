#ifndef BYLINS_COMMON_H
#define BYLINS_COMMON_H

#include <algorithm>

#include "engine/entities/char_data.h"
#include "utils/random.h"

inline bool IsUnableToAct(CharData *ch) {
	return (AFF_FLAGGED(ch, EAffect::kStopFight) ||
	AFF_FLAGGED(ch, EAffect::kMagicStopFight) ||
	AFF_FLAGGED(ch, EAffect::kHold));
}

// A group member standing in formation (kSkirmisher) who can actually cover the group right now: able
// to act and on their feet.
inline bool IsActiveSkirmisher(CharData *ch) {
	return ch->IsFlagged(EPrf::kSkirmisher) && !IsUnableToAct(ch) && ch->GetPosition() >= EPosition::kFight;
}

// Урон после частичной защиты, которую игра называет "немного" (уклонился, отклонил, отразил):
// две трети. Держим одной названной функцией, а не выражением на месте: четыре умения считали
// это каждое у себя, и один рефакторинг разом превратил все четыре в полный промах (#2764 ->
// `dam *= 10 / 15`, где целочисленное деление даёт ноль).
inline int SlightDefenceDamage(int dam) {
	return dam * 10 / 15;
}

// Урон с процентным бонусом атакующего (`percent_physdam_add`, `percent_spellpower_add`), одинаково
// для физики и магии и в обе стороны: отрицательный процент режет урон, но не ниже нуля. Физ. урон
// раньше применял процент, только если тот больше нуля, и все минусы к нему (белена, покалеченная
// конечность, костяная дань) не делали ничего (#4095).

// Для удара: кубик 1..2p процентов (в среднем ~p), для минуса -- зеркально, -(1..2|p|).
inline int RollDamagePercent(int dam, int percent) {
	if (percent == 0) {
		return dam;
	}
	const int rolled = percent > 0 ? number(1, percent * 2) : -number(1, -percent * 2);
	return std::max(0, dam + static_cast<int>(dam * (rolled / 100.0)));
}

// Для показа (счёт, опознание): средний результат броска, ровно p процентов.
inline int AverageDamagePercent(int dam, int percent) {
	return std::max(0, dam + static_cast<int>(dam * (percent / 100.0)));
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
