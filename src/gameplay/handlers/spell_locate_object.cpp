/**
 \file spell_locate_object.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellLocateObject manual-cast handler (extracted from spells.cpp).
*/

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
	ObjData *obj = ctx.ovict;
	/*
	   * FIXME: This is broken.  The spell parser routines took the argument
	   * the player gave to the spell and located an object with that keyword.
	   * Since we're passed the object and not the keyword we can only guess
	   * at what the player originally meant to search for. -gg
	   */
	if (!obj) {
		return EStageResult::kSuccess;
	}

	char name[kMaxInputLength];
	bool bloody_corpse = false;
	strcpy(name, cast_argument);

	int tmp_lvl = (privilege::IsGod(ch)) ? 300 : level;
	int count = tmp_lvl;
	const auto result = world_objects.find_if_and_dec_number([ch, name, &bloody_corpse](const ObjData::shared_ptr &i) {
		const auto obj_ptr = world_objects.get_by_raw_ptr(i.get());
		if (!obj_ptr) {
			sprintf(buf, "SYSERR: Illegal object iterator while locate");
			mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);

			return false;
		}

		bloody_corpse = false;
		if (!privilege::IsGod(ch)) {
			if (number(1, 100) > (40 + std::max((GetRealInt(ch) - 25) * 2, 0))) {
				return false;
			}

			if (IS_CORPSE(i)) {
				bloody_corpse = bloody::CatchBloodyCorpse(i.get());
				if (!bloody_corpse) {
					return false;
				}
			}
		}

		if (i->has_flag(EObjFlag::kNolocate) && i->get_carried_by() != ch) {
			// !локейт стаф может локейтить только имм или тот кто его держит
			return false;
		}

		if (SECT(i->get_in_room()) == ESector::kSecret) {
			return false;
		}

		if (i->get_carried_by()) {
			const auto carried_by = i->get_carried_by();
			const auto carried_by_ptr = character_list.get_character_by_address(carried_by);

			if (!carried_by_ptr) {
				sprintf(buf, "SYSERR: Illegal carried_by ptr. Создана кора для исследований");
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
				return false;
			}

			if (!ValidRnum(carried_by->in_room)) {
				sprintf(buf,
						"SYSERR: Illegal room %d, char %s. Создана кора для исследований",
						carried_by->in_room,
						carried_by->get_name().c_str());
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
				return false;
			}

			if (SECT(carried_by->in_room) == ESector::kSecret || privilege::IsImmortal(carried_by)) {
				return false;
			}
		}

		if (!isname(name, i->get_aliases())) {
			return false;
		}
		std::string locate_msg;

		if (i->get_carried_by()) {
			const auto carried_by = i->get_carried_by();
			const auto same_zone = world[ch->in_room]->zone_rn == world[carried_by->in_room]->zone_rn;
			if (!carried_by->IsNpc() || same_zone || bloody_corpse) {
				sprintf(buf, "%s наход%sся у %s в инвентаре.\r\n", i->get_short_description().c_str(),
						grammar::ObjPluralVerbEnding((i)->get_sex()), sight::PersonName(carried_by, ch, 1));
			} else {
				return false;
			}
		} else if (i->get_in_room() != kNowhere && i->get_in_room()) {
			const auto room = i->get_in_room();
			const auto same_zone = world[ch->in_room]->zone_rn == world[room]->zone_rn;
			if (same_zone) {
				sprintf(buf, "%s наход%sся в комнате '%s'\r\n",
						i->get_short_description().c_str(), grammar::ObjPluralVerbEnding((i)->get_sex()), world[room]->name);
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
				sprintf(buf, "%s наход%sся в %s.\r\n",
						i->get_short_description().c_str(),
						grammar::ObjPluralVerbEnding((i)->get_sex()),
						i->get_in_obj()->get_PName(grammar::ECase::kPre).c_str());
			}
		} else if (i->get_worn_by()) {
			const auto worn_by = i->get_worn_by();
			const auto same_zone = world[ch->in_room]->zone_rn == world[worn_by->in_room]->zone_rn;
			if (!worn_by->IsNpc() || same_zone || bloody_corpse) {
				sprintf(buf, "%s надет%s на %s.\r\n", i->get_short_description().c_str(),
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
			sprintf(buf, "Местоположение %s неопределимо.\r\n", OBJN(i.get(), ch, grammar::ECase::kGen));
		}
		SendMsgToChar(buf, ch);
		return true;
	}, count);

	int j = count;
	if (j > 0) {
		j = Clan::print_spell_locate_object(ch, j, std::string(name));
	}

	if (j == tmp_lvl) {
		// "nothing felt" on kLocateObject's sheaf as kCustomMsgOne.
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kLocateObject, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
