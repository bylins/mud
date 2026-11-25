/**
\file do_pray.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief The "молиться" and "жертвовать" commands.
\details issue #2689: the command only checks and takes the offering; the effect is the recipient's
		 service spell (cfg/mechanics/religion.xml -> spells.xml). A Christian prays to a saint and lights
		 a candle, a heathen sacrifices food or treasure to a god. Piety (kReligion) sets the effect's
		 strength and grows with every offering; its timer is the prayer cooldown.
*/

#include <fmt/format.h>

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
#include "gameplay/mechanics/religion.h"
#include "gameplay/skills/skills.h"

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

void ListRecipients(CharData *ch, int subcmd) {
	std::string out = IsSacrifice(subcmd) ? "Вы можете принести жертву:\r\n" : "Вы можете вознести молитву:\r\n";
	for (const auto &recipient : religion::RecipientsRegistry().All()) {
		if (recipient.religion == CommandReligion(subcmd)) {
			out += fmt::format("  {}\r\n", recipient.name);
		}
	}
	out += IsSacrifice(subcmd) ? "Укажите, кому и что вы хотите жертвовать.\r\n"
							   : "Укажите, кому вы хотите вознести молитву.\r\n";
	SendMsgToChar(out, ch);
}

void SendOfferingMessages(CharData *ch, ESpell spell, ObjData *gift) {
	const auto &to_char = MUD::SpellMessages().GetMessage(spell, ESpellMsg::kOfferingToChar);
	if (!to_char.empty()) {
		act(to_char.c_str(), false, ch, gift, nullptr, kToChar);
	}
	const auto &to_room = MUD::SpellMessages().GetMessage(spell, ESpellMsg::kOfferingToRoom);
	if (!to_room.empty()) {
		act(to_room.c_str(), false, ch, gift, nullptr, kToRoom | kToArenaListen);
	}
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

	const auto *recipient = religion::RecipientsRegistry().Find(whom);
	if (!recipient) {
		ListRecipients(ch, subcmd);
		return;
	}

	if (recipient->religion != CommandReligion(subcmd)) {
		SendMsgToChar(IsSacrifice(subcmd) ? "Приносите жертвы только своим Богам.\r\n"
										  : "Возносите молитвы только своим Богам.\r\n", ch);
		return;
	}

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

	SendOfferingMessages(ch, recipient->spell, gift);
	if (gift) {
		RemoveObjFromChar(gift);
		ExtractObjFromWorld(gift);
	} else {
		currencies::RemoveHand(*ch, currencies::kGold, kCandlePrice);
	}

	// The marker (kPrayerful/kPietas) and the recipient's favour are the spell's affects; their
	// strength comes from piety and charisma. Every offering trains piety, in temples too.
	const auto result = CallMagic(ch, ch, nullptr, world[ch->in_room], recipient->spell, GetRealLevel(ch));
	ImproveSkill(ch, ESkill::kReligion, CastTookEffect(result), nullptr);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
