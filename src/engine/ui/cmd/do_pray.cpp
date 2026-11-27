/**
\file do_pray.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief The "молиться" and "жертвовать" commands.
\details issue #2689: the command only checks and takes the offering; the effect is a service spell --
		 kOffering for a heathen's sacrifice to a god, kPrayer for a Christian's prayer to a saint. Whom
		 is the spell's selectable action (<action choice=> in spells.xml). Piety (kReligion) sets the
		 effect's strength; its timer is the prayer cooldown. What may be offered and which gift raises
		 piety is cfg/mechanics/piety.xml (piety::kValuedOfferings switches back to the old way).
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
#include "gameplay/mechanics/piety.h"
#include "gameplay/skills/skills.h"
#include "utils/utils_string.h"

#include <limits>
#include <string>

namespace {

// The candle of the old way (piety::kValuedOfferings == false); the new way reads it from piety.xml.
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

// What an offering takes from the character: an item, an amount of a currency, or the default amount
// (a Christian's candle). gift is its value for piety growth.
struct Offering {
	ObjData *item{nullptr};
	std::string currency;
	long amount{0};
	bool is_default{false};
	piety::Gift gift;
};

std::string GiftText(const Offering &offering) {
	if (offering.item) {
		return offering.item->get_PName(grammar::ECase::kAcc);
	}
	const auto &currency = currencies::FindByTextIdNoCase(offering.currency);
	return fmt::format("{} {}", offering.amount, currency.GetNameWithAmount(offering.amount, grammar::ECase::kAcc));
}

void SendOfferingMessage(CharData *ch, ESpell spell, const Offering &offering, const std::string &whom, int to) {
	const bool to_char = (to & kToChar) != 0;
	ESpellMsg key;
	if (offering.is_default) {
		key = to_char ? ESpellMsg::kDefaultOfferingToChar : ESpellMsg::kDefaultOfferingToRoom;
	} else {
		key = to_char ? ESpellMsg::kOfferingToChar : ESpellMsg::kOfferingToRoom;
	}
	std::string msg = MUD::SpellMessages().GetMessage(spell, key);
	if (msg.empty()) {
		return;
	}
	utils::ReplaceAll(msg, "{choice}", whom);
	utils::ReplaceAll(msg, "{gift}", GiftText(offering));
	act(msg.c_str(), false, ch, offering.item, nullptr, to);
}

// The old way (piety::kValuedOfferings == false): a heathen offers food or treasure, a Christian pays
// for a candle; nothing raises piety more than a plain offering.
bool ChooseLegacyOffering(CharData *ch, int subcmd, const std::string &what, Offering &offering) {
	if (!IsSacrifice(subcmd)) {
		if (currencies::GetHand(*ch, currencies::kGold) < kCandlePrice) {
			SendMsgToChar("У вас не хватит денег на свечку.\r\n", ch);
			return false;
		}
		offering.currency = currencies::kGold;
		offering.amount = kCandlePrice;
		offering.is_default = true;
		return true;
	}
	const std::string item_name = utils::ExtractFirstArgumentLower(what);
	if (item_name.empty() || !(offering.item = get_obj_in_list_vis(ch, item_name, ch->carrying))) {
		SendMsgToChar("Вы должны пожертвовать что-то стоящее.\r\n", ch);
		return false;
	}
	if (offering.item->get_type() != EObjType::kFood && offering.item->get_type() != EObjType::kTreasure) {
		SendMsgToChar("Богам неугодна эта жертва.\r\n", ch);
		return false;
	}
	return true;
}

// "<N> <currency>": an amount of an accepted currency the character holds.
bool ChooseCurrencyOffering(CharData *ch, const piety::OfferingRules &rules, const std::string &number,
							const std::string &currency_word, Offering &offering) {
	long amount{0};
	try {
		amount = std::stol(number);
	} catch (const std::exception &) {
		amount = 0;
	}
	if (amount <= 0) {
		SendMsgToChar("Укажите, сколько вы хотите пожертвовать.\r\n", ch);
		return false;
	}
	if (currency_word.empty()) {
		SendMsgToChar(fmt::format("Чего {} вы хотите пожертвовать?\r\n", amount), ch);
		return false;
	}
	const currencies::CurrencyInfo *currency = nullptr;
	if (utils::IsAbbr("coin", currency_word.c_str()) || utils::IsAbbr("кун", currency_word.c_str())
		|| !str_cmp("денег", currency_word)) {
		currency = &currencies::FindByTextIdNoCase(currencies::kGold);
	} else {
		currency = currencies::FindBySearch(currency_word);
	}
	const auto minimum = currency ? rules.CurrencyMinimum(currency->GetTextId()) : std::nullopt;
	if (!minimum) {
		SendMsgToChar("Богам неугодна эта жертва.\r\n", ch);
		return false;
	}
	if (amount < *minimum) {
		SendMsgToChar(fmt::format("Слишком скудный дар: не меньше {} {}.\r\n", *minimum,
								  currency->GetNameWithAmount(*minimum, grammar::ECase::kGen)), ch);
		return false;
	}
	if (currencies::GetHand(*ch, currency->GetTextId()) < amount) {
		SendMsgToChar("У вас нет при себе такой суммы.\r\n", ch);
		return false;
	}
	offering.currency = currency->GetTextId();
	offering.amount = amount;
	offering.gift = {piety::EMeasure::kCurrency, offering.currency, static_cast<double>(amount)};
	return true;
}

// An accepted item from the inventory, valued by its measure and its timer.
bool ChooseItemOffering(CharData *ch, const piety::OfferingRules &rules, const std::string &item_name,
						Offering &offering) {
	ObjData *item = get_obj_in_list_vis(ch, item_name, ch->carrying);
	if (!item) {
		SendMsgToChar(fmt::format("У вас нет '{}'.\r\n", item_name), ch);
		return false;
	}
	const auto measure = rules.ItemMeasure(item->get_type());
	if (!measure) {
		SendMsgToChar("Богам неугодна эта жертва.\r\n", ch);
		return false;
	}
	if (item->has_flag(EObjFlag::kNodrop) || item->has_flag(EObjFlag::kNodonate)) {
		act("Вы не можете расстаться с $o4.", false, ch, item, nullptr, kToChar);
		return false;
	}
	if (item->get_contains()) {
		act("Сначала опустошите $o3.", false, ch, item, nullptr, kToChar);
		return false;
	}
	const auto value = piety::ItemValue(item, *measure, piety::GetRules().FreshTimerShare());
	if (!value) {
		SendMsgToChar("Богам неугодна эта жертва.\r\n", ch);
		return false;
	}
	offering.item = item;
	offering.gift = {*measure, {}, *value};
	return true;
}

// The new way: an item, "<N> <currency>", or nothing for the default amount.
bool ChooseOffering(CharData *ch, int subcmd, const std::string &what, Offering &offering) {
	const auto &rules = IsSacrifice(subcmd) ? piety::GetRules().Sacrifice() : piety::GetRules().Prayer();
	std::string rest;
	const std::string first = utils::ExtractFirstArgumentLower(what, rest);
	if (first.empty()) {
		if (rules.default_currency.empty()) {
			SendMsgToChar("Вы должны пожертвовать что-то стоящее.\r\n", ch);
			return false;
		}
		if (currencies::GetHand(*ch, rules.default_currency) < rules.default_amount) {
			SendMsgToChar("У вас не хватит денег на свечку.\r\n", ch);
			return false;
		}
		offering.currency = rules.default_currency;
		offering.amount = rules.default_amount;
		offering.is_default = true;
		offering.gift = {piety::EMeasure::kCurrency, offering.currency, static_cast<double>(offering.amount)};
		return true;
	}
	if (is_number(first.c_str())) {
		return ChooseCurrencyOffering(ch, rules, first, utils::ExtractFirstArgumentLower(rest), offering);
	}
	return ChooseItemOffering(ch, rules, first, offering);
}

void TakeOffering(CharData *ch, const Offering &offering) {
	if (offering.item) {
		RemoveObjFromChar(offering.item);
		ExtractObjFromWorld(offering.item);
	} else {
		currencies::RemoveHand(*ch, offering.currency, offering.amount);
	}
}

// Piety grows from an offering that took effect. The new way: +1 when the gift reaches the threshold
// for current piety. The old way: the usual skill roll, up to piety::kLegacyGrowthLimit.
void GrowPiety(CharData *ch, const piety::Gift &gift, bool took_effect) {
	const int trained = GetTrainedSkill(ch, ESkill::kReligion);
	if constexpr (!piety::kValuedOfferings) {
		if (trained < piety::kLegacyGrowthLimit) {
			ImproveSkill(ch, ESkill::kReligion, took_effect, nullptr);
		}
		return;
	}
	const auto *tier = piety::GetRules().TierFor(trained);
	const auto threshold = tier ? tier->Threshold(gift) : std::nullopt;
	SendToTC(ch, true, true, true, "Piety: skill=%d, gift=%.1f, threshold=%s", trained, gift.value,
			 threshold ? fmt::format("{:.1f}", *threshold).c_str() : "none");
	if (!took_effect || trained <= 0 || trained >= CalcSkillMinCap(ch, ESkill::kReligion)
		|| !IsSkillTrainableInRoom(MUD::Skill(ESkill::kReligion), world[ch->in_room])
		|| !piety::GetRules().IsWorthy(trained, gift)) {
		return;
	}
	SetSkill(ch, ESkill::kReligion, trained + 1);
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

	// "<whom> [<item> | <N> <currency>]"
	std::string what;
	const std::string whom = utils::ExtractFirstArgumentLower(argument, what);

	const auto spell = CommandSpell(subcmd);
	const auto *recipient = MUD::Spell(spell).actions.FindChoice(whom);
	if (!recipient) {
		if (!whom.empty() && MUD::Spell(OtherReligionSpell(subcmd)).actions.FindChoice(whom)) {
			SendMsgToChar(IsSacrifice(subcmd) ? "Приносите жертвы только своим Богам.\r\n"
											  : "Возносите молитвы только своим Богам.\r\n", ch);
		} else {
			ListRecipients(ch, subcmd);
		}
		return;
	}
	const std::string &recipient_name = recipient->GetChoiceName();

	Offering offering;
	const bool chosen = piety::kValuedOfferings ? ChooseOffering(ch, subcmd, what, offering)
												: ChooseLegacyOffering(ch, subcmd, what, offering);
	if (!chosen) {
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

	SendOfferingMessage(ch, spell, offering, recipient_name, kToChar);
	SendOfferingMessage(ch, spell, offering, recipient_name, kToRoom | kToArenaListen);
	// The gift's value is taken before the item is gone.
	const piety::Gift gift = offering.gift;
	TakeOffering(ch, offering);

	// The marker (kPrayerful/kPietas) and the recipient's favour are the spell's affects; the favour's
	// action is selected by the recipient's name. Their strength comes from current piety and charisma,
	// whatever the gift; only a worthy gift raises piety.
	const auto result = CallMagic(ch, ch, nullptr, world[ch->in_room], spell, GetRealLevel(ch), -1.0f,
		std::numeric_limits<double>::quiet_NaN(), -1, -1, CastArgs{{recipient_name}, 0});
	GrowPiety(ch, gift, CastTookEffect(result));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
