#include "warcry.h"

#include "handler.h"
#include "color.h"
#include "game_magic/magic_utils.h"
#include "game_magic/spells_info.h"

void do_warcry(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int spellnum, cnt;

	if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (!ch->get_skill(ESkill::kWarcry)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		SendMsgToChar("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}
	
	std::string wc_name(argument);
	// обрезаем лишние пробелы, чтобы нормально обработать "warcry of ..."
	wc_name.erase(std::find_if_not(wc_name.rbegin(), wc_name.rend(), isspace).base(), wc_name.end());
	wc_name.erase(wc_name.begin(), std::find_if_not(wc_name.begin(), wc_name.end(), isspace));

	if (wc_name.empty()) {
		sprintf(buf, "Вам доступны :\r\n");
		for (cnt = spellnum = 1; spellnum <= kSpellLast; spellnum++) {
			const char *realname = spell_info[spellnum].name
									   && *spell_info[spellnum].name ? spell_info[spellnum].name :
								   spell_info[spellnum].syn
									   && *spell_info[spellnum].syn
								   ? spell_info[spellnum].syn
								   : nullptr;

			if (realname
				&& IS_SET(spell_info[spellnum].routines, kMagWarcry)
				&& ch->get_skill(ESkill::kWarcry) >= spell_info[spellnum].mana_change) {
				if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellKnow | kSpellTemp))
					continue;
				sprintf(buf + strlen(buf), "%s%2d%s) %s%s%s\r\n",
						KGRN, cnt++, KNRM,
						spell_info[spellnum].violent ? KIRED : KIGRN, realname, KNRM);
			}
		}
		SendMsgToChar(buf, ch);
		return;
	}

	if (wc_name.length() > 3 && wc_name.substr(0, 3) == "of ") {
		wc_name = "warcry of " + wc_name.substr(3, std::string::npos);
	} else {
		wc_name = "клич " + wc_name;
	}

	spellnum = FixNameAndFindSpellNum(wc_name);

	// Unknown warcry
	if (spellnum < 1 || spellnum > kSpellLast
		|| (ch->get_skill(ESkill::kWarcry) < spell_info[spellnum].mana_change)
		|| !IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellKnow | kSpellTemp)) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	// в текущем варианте кличи можно кастить только без указания цели
	// если в будущем добавятся кличи на цель - необходимо менять формат команды
	// например как сделано с заклинаниями - брать клич в !!
	// либо гарантировать что все кличи будут всегда из 1 слова

	if (spell_info[spellnum].targets != kTarIgnore) {
		std::stringstream str_log;
		str_log << "Для клича #" << spellnum << ", установлены некорректные цели: " << spell_info[spellnum].targets;
		mudlog(str_log.str(), BRF, kLvlGod, SYSLOG, true);
		SendMsgToChar("Вы ничего не смогли выкрикнуть. Обратитесь к богам.\r\n", ch);
		return;
	}

	struct TimedSkill timed;
	timed.skill = ESkill::kWarcry;
	timed.time = IsTimedBySkill(ch, ESkill::kWarcry) + kHoursPerWarcry;

	if (timed.time > kHoursPerDay) {
		SendMsgToChar("Вы охрипли и не можете кричать.\r\n", ch);
		return;
	}

	if (GET_MOVE(ch) < spell_info[spellnum].mana_max) {
		SendMsgToChar("У вас не хватит сил для этого.\r\n", ch);
		return;
	}

	SaySpell(ch, spellnum, nullptr, nullptr);

	if (CallMagic(ch, nullptr, nullptr, nullptr, spellnum, GetRealLevel(ch)) >= 0) {
		if (!IS_IMMORTAL(ch)) {
			if (ch->get_wait() <= 0) {
				SetWaitState(ch, kPulseViolence);
			}
			ImposeTimedSkill(ch, &timed);
			GET_MOVE(ch) -= spell_info[spellnum].mana_max;
		}
		TrainSkill(ch, ESkill::kWarcry, true, nullptr);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
