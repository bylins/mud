/**
 \file spell_locate_object.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellLocateObject manual-cast handler (extracted from spells.cpp).
*/

#include <fmt/format.h>

#include "gameplay/handlers/spell_handlers.h"
#include "administration/privilege.h"
#include "utils/grammar/gender.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/fight/pk.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/clans/house.h"
#include "utils/utils.h"
#include "gameplay/communication/parcel.h"
#include "engine/core/utils_char_obj.inl"

namespace handlers {

EStageResult SpellLocateObject(ActionContext &ctx) {
	const int level = abs(ctx.level);
	CharData *ch = ctx.caster();
	// Цель заклинанию не нужна: ищем по строке запроса. Раньше её всё равно искали
	// заранее, гоняя весь список предметов мира впустую и отбирая по другим
	// правилам, чем здесь (issue #3924).
	char name[kMaxInputLength];
	bool bloody_corpse = false;
	strcpy(name, cast_argument);

	const bool is_god = privilege::IsGod(ch);
	const int tmp_lvl = is_god ? 300 : level;
	int count = tmp_lvl;
	// matched -- предметы с таким именем, которые разрешено искать: по нулю совпадений
	// видно, что игрок ввёл бессмыслицу или опечатался. shown -- сколько реально показано.
	int matched = 0;
	int shown = 0;
	const auto locate = [&](const ObjData::shared_ptr &i) -> bool {
		bloody_corpse = false;

		// Сначала запреты. Запрещённое к поиску не считается совпадением, иначе заклинание
		// выдавало бы, что предмет с таким именем где-то существует.
		if (i->has_flag(EObjFlag::kNolocate) && i->get_carried_by() != ch) {
			// !локейт стаф может локейтить только имм или тот кто его держит
			return false;
		}

		if (SECT(i->get_in_room()) == ESector::kSecret) {
			return false;
		}

		// Предфильтр перед isname: тот разбирает строки посимвольно и на полном
		// списке предметов мира стоит ощутимо дороже поиска подстроки (issue #3924).
		if (!MayMatchName(name, i->get_aliases()) || !isname(name, i->get_aliases())) {
			return false;
		}

		if (i->get_carried_by()) {
			const auto carried_by = i->get_carried_by();
			const auto carried_by_ptr = character_list.get_character_by_address(carried_by);

			if (!carried_by_ptr) {
				mudlog("SYSERR: Illegal carried_by ptr. Создана кора для исследований",
					   BRF, kLvlImplementator, SYSLOG, true);
				return false;
			}

			if (!ValidRnum(carried_by->in_room)) {
				mudlog(fmt::format("SYSERR: Illegal room {}, char {}. Создана кора для исследований",
								   carried_by->in_room, carried_by->get_name()),
					   BRF, kLvlImplementator, SYSLOG, true);
				return false;
			}

			if (SECT(carried_by->in_room) == ESector::kSecret || privilege::IsImmortal(carried_by)) {
				return false;
			}
		}

		if (!is_god && IS_CORPSE(i)) {
			bloody_corpse = bloody::CatchBloodyCorpse(i.get());
			if (!bloody_corpse) {
				return false;
			}
		}

		++matched;

		// Шанс -- после имени: вероятность для каждого предмета та же, что была, а
		// совпадение уже учтено и неудачный бросок не выглядит бессмыслицей.
		if (!is_god && number(1, 100) > (40 + std::max((GetRealInt(ch) - 25) * 2, 0))) {
			return false;
		}

		std::string locate_msg;
		std::string where;

		if (i->get_carried_by()) {
			const auto carried_by = i->get_carried_by();
			const auto same_zone = world[ch->in_room]->zone_rn == world[carried_by->in_room]->zone_rn;
			if (!carried_by->IsNpc() || same_zone || bloody_corpse) {
				where = fmt::format("{} наход{}ся у {} в инвентаре.\r\n", i->get_short_description(),
									grammar::ObjPluralVerbEnding((i)->get_sex()), sight::PersonName(carried_by, ch, 1));
			} else {
				return false;
			}
		} else if (i->get_in_room() != kNowhere && i->get_in_room()) {
			const auto room = i->get_in_room();
			const auto same_zone = world[ch->in_room]->zone_rn == world[room]->zone_rn;
			if (same_zone) {
				// Имя комнаты бывает нулевым, а fmt на нуле бросает исключение
				where = fmt::format("{} наход{}ся в комнате '{}'\r\n",
									i->get_short_description(), grammar::ObjPluralVerbEnding((i)->get_sex()),
									world[room]->name ? world[room]->name : "");
			} else {
				return false;
			}
		} else if (i->get_in_obj()) {
			if (Clan::is_clan_chest(i->get_in_obj())) {
				return false; // шоб не забивало локейт на мобах/плеерах - по кланам проходим ниже отдельно
			} else {
				if (!privilege::IsGod(ch)) {
					if (i->get_in_obj()->get_carried_by()) {
						if (i->get_in_obj()->get_carried_by()->IsNpc() && i->has_flag(EObjFlag::kNolocate)) {
							return false;
						}
					}
					if (i->get_in_obj()->get_in_room() != kNowhere
						&& i->get_in_obj()->get_in_room()) {
						if (i->has_flag(EObjFlag::kNolocate) && !bloody_corpse) {
							return false;
						}
					}
					if (i->get_in_obj()->get_worn_by()) {
						const auto worn_by = i->get_in_obj()->get_worn_by();
						if (worn_by->IsNpc() && i->has_flag(EObjFlag::kNolocate) && !bloody_corpse) {
							return false;
						}
					}
				}
				where = fmt::format("{} наход{}ся в {}.\r\n",
									i->get_short_description(),
									grammar::ObjPluralVerbEnding((i)->get_sex()),
									i->get_in_obj()->get_PName(grammar::ECase::kPre));
			}
		} else if (i->get_worn_by()) {
			const auto worn_by = i->get_worn_by();
			const auto same_zone = world[ch->in_room]->zone_rn == world[worn_by->in_room]->zone_rn;
			if (!worn_by->IsNpc() || same_zone || bloody_corpse) {
				where = fmt::format("{} надет{} на {}.\r\n", i->get_short_description(),
									grammar::ObjSexEnding((i)->get_sex(), 6), sight::PersonName(worn_by, ch, 3));
			} else {
				return false;
			}
		} else if (!(locate_msg = Depot::PrintSpellLocateObject(ch, i.get())).empty()) {
			SendMsgToChar(locate_msg.c_str(), ch);
			return true;
		} else if (!(locate_msg = Parcel::PrintSpellLocateObject(ch, i.get())).empty()) {
			SendMsgToChar(locate_msg.c_str(), ch);
			return true;
		} else {
			where = fmt::format("Местоположение {} неопределимо.\r\n", OBJN(i.get(), ch, grammar::ECase::kGen));
		}
		SendMsgToChar(where, ch);
		return true;
	};
	world_objects.find_if_and_dec_number([&](const ObjData::shared_ptr &i) {
		if (locate(i)) {
			++shown;
			return true;
		}
		return false;
	}, count);

	// Лимит считаем по показанному: счётчик find_if_and_dec_number не опускается ниже
	// единицы, и хранилища дружин добавляли одну находку сверх уровня.
	int clan_matched = 0;
	if (const int remaining = tmp_lvl - shown; remaining > 0) {
		shown += remaining - Clan::print_spell_locate_object(ch, remaining, name, clan_matched);
	}

	if (matched + clan_matched == 0) {
		// Такого предмета нет вовсе -- опечатка или бессмыслица.
		SendMsgToChar("Тяжеловато найти цель вашего заклинания!\r\n", ch);
	} else if (shown == 0) {
		// "nothing felt" on kLocateObject's sheaf as kCustomMsgOne.
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kLocateObject, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
