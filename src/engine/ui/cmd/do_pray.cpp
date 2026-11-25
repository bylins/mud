/**
\file do_pray.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief The "молиться" and "жертвовать" commands.
\details issue #2689: the command only checks and takes the offering; the effect is a service spell --
		 kOffering for a heathen, who sacrifices food or treasure to a god, kPrayer for a Christian, who
		 prays to a saint and lights a candle. Whom is the spell's selectable action (<action choice=>
		 in spells.xml). Piety (kReligion) sets the effect's strength and grows with every offering;
		 its timer is the prayer cooldown.
*/

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "engine/ui/cmd/do_pray.h"
#include "administration/privilege.h"
#include "gameplay/economics/currencies.h"

#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/obj_handler.h"
#include "engine/core/target_resolver.h"
#include "gameplay/abilities/timed_abilities.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/inventory.h"
#include "gameplay/skills/skills.h"
#include "utils/utils_string.h"

#include <limits>

namespace {

constexpr int kCandlePrice{10};
constexpr int kPrayerCooldownHours{12};

bool IsSacrifice(int subcmd) {
	return subcmd == SCMD_DONATE;
}

// Who may use the command: heathens sacrifice, Christians pray.
int CommandReligion(int subcmd) {
	return IsSacrifice(subcmd) ? kReligionPoly : kReligionMono;
}

// The spell the command casts, and the other religion's one.
ESpell CommandSpell(int subcmd) {
	return IsSacrifice(subcmd) ? ESpell::kOffering : ESpell::kPrayer;
}

ESpell OtherReligionSpell(int subcmd) {
	return IsSacrifice(subcmd) ? ESpell::kPrayer : ESpell::kOffering;
}

void ListRecipients(CharData *ch, int subcmd) {
	const auto spell = CommandSpell(subcmd);
	std::string out = MUD::SpellMessages().GetMessage(spell, ESpellMsg::kUnknownChoice);
	utils::ReplaceAll(out, "{choices}", fmt::format("{}", fmt::join(MUD::Spell(spell).actions.ChoiceNames(), ", ")));
	out += IsSacrifice(subcmd) ? "\r\nУкажите, кому и что вы хотите жертвовать.\r\n"
							   : "\r\nУкажите, кому вы хотите вознести молитву.\r\n";
	SendMsgToChar(out, ch);
}

void SendOfferingMessage(CharData *ch, ESpell spell, ESpellMsg key, const std::string &whom, ObjData *gift,
						 int to) {
	std::string msg = MUD::SpellMessages().GetMessage(spell, key);
	if (msg.empty()) {
		return;
	}
	utils::ReplaceAll(msg, "{choice}", whom);
	act(msg.c_str(), false, ch, gift, nullptr, to);
}

} // namespace

void do_pray(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	if (ch->IsNpc() || (subcmd != SCMD_PRAY && subcmd != SCMD_DONATE)) {
		return;
	}

	if (!privilege::IsImmortal(ch) && GET_RELIGION(ch) != CommandReligion(subcmd)) {
		SendMsgToChar("Не кощунствуйте!\r\n", ch);
		return;
	}

	if (IsSacrifice(subcmd) && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kForPoly)) {
		SendMsgToChar("Найдите подходящее место для вашей жертвы.\r\n", ch);
		return;
	}
	if (!IsSacrifice(subcmd) && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kForMono)) {
		SendMsgToChar("Это место не обладает необходимой святостью.\r\n", ch);
		return;
	}

	char whom[kMaxInputLength];
	char gift_name[kMaxStringLength];
	half_chop(argument, whom, gift_name);

	const auto spell = CommandSpell(subcmd);
	const auto *recipient = MUD::Spell(spell).actions.FindChoice(whom);
	if (!recipient) {
		if (*whom && MUD::Spell(OtherReligionSpell(subcmd)).actions.FindChoice(whom)) {
			SendMsgToChar(IsSacrifice(subcmd) ? "Приносите жертвы только своим Богам.\r\n"
											  : "Возносите молитвы только своим Богам.\r\n", ch);
		} else {
			ListRecipients(ch, subcmd);
		}
		return;
	}
	const std::string &recipient_name = recipient->GetChoiceName();

	ObjData *gift = nullptr;
	if (IsSacrifice(subcmd)) {
		if (!*gift_name || !(gift = get_obj_in_list_vis(ch, gift_name, ch->carrying))) {
			SendMsgToChar("Вы должны пожертвовать что-то стоящее.\r\n", ch);
			return;
		}
		if (gift->get_type() != EObjType::kFood && gift->get_type() != EObjType::kTreasure) {
			SendMsgToChar("Богам неугодна эта жертва.\r\n", ch);
			return;
		}
	} else if (currencies::GetHand(*ch, currencies::kGold) < kCandlePrice) {
		SendMsgToChar("У вас не хватит денег на свечку.\r\n", ch);
		return;
	}

	if (!privilege::IsImmortal(ch) && (IsTimedBySkill(ch, ESkill::kReligion)
		|| IsAffected(ch, EAffect::kPrayerful) || IsAffected(ch, EAffect::kPietas))) {
		SendMsgToChar("Вы не можете так часто взывать к Богам.\r\n", ch);
		return;
	}

	TimedSkill timed;
	timed.skill = ESkill::kReligion;
	timed.time = kPrayerCooldownHours;
	ImposeTimedSkill(ch, &timed);

	SendOfferingMessage(ch, spell, ESpellMsg::kOfferingToChar, recipient_name, gift, kToChar);
	SendOfferingMessage(ch, spell, ESpellMsg::kOfferingToRoom, recipient_name, gift, kToRoom | kToArenaListen);
	if (gift) {
		RemoveObjFromChar(gift);
		ExtractObjFromWorld(gift);
	} else {
		currencies::RemoveHand(*ch, currencies::kGold, kCandlePrice);
	}

	// The marker (kPrayerful/kPietas) and the recipient's favour are the spell's affects; the favour's
	// action is selected by the recipient's name. Their strength comes from piety and charisma. Every
	// offering trains piety, in temples too.
	const auto result = CallMagic(ch, ch, nullptr, world[ch->in_room], spell, GetRealLevel(ch), -1.0f,
		std::numeric_limits<double>::quiet_NaN(), -1, -1, recipient_name);
	ImproveSkill(ch, ESkill::kReligion, CastTookEffect(result), nullptr);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
