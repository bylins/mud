/****************************[14~*********************************************
*   File: liquid.cpp                                   Part of Bylins    *
*   Все по жидкостям                                                     *
*                                                                        *
*  $Author$                                                      *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "liquid.h"
#include "administration/privilege.h"
#include "utils/grammar/gender.h"
#include "utils/grammar/declensions.h"

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/magic/magic.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"
#include "poison.h"

#include <cmath>
#include <limits>

#include "gameplay/magic/magic_utils.h"


const char *drinks[] = {"воды",
						"пива",
						"вина",
						"медовухи",
						"кваса",
						"самогона",
						"морсу",
						"водки",
						"браги",
						"меду",
						"молока",
						"чаю",
						"кофею",
						"КРОВИ",
						"соленой воды",
						"родниковой воды",
						"колдовского зелья",
						"красного колдовского зелья",
						"синего колдовского зелья",
						"белого колдовского зелья",
						"золотистого колдовского зелья", //20
						"черного колдовского зелья",
						"серого колдовского зелья",
						"фиолетового колдовского зелья",
						"розового колдовского зелья",
						"настойки аконита",
						"настойки скополии",
						"настойки белены",
						"настойки дурмана",
						"жгучей смеси",
						"\n"
};

// one-word alias for each drink
const char *drinknames[] = {"водой",
							"пивом",
							"вином",
							"медовухой",
							"квасом",
							"самогоном",
							"морсом",
							"водкой",
							"брагой",
							"медом",
							"молоком",
							"чаем",
							"кофе",
							"КРОВЬЮ",
							"соленой водой",
							"родниковой водой",
							"колдовским зельем",
							"красным колдовским зельем",
							"синим колдовским зельем",
							"белым колдовским зельем",
							"золотистым колдовским зельем", //20
							"черным колдовским зельем",
							"серым колдовским зельем",
							"фиолетовым колдовским зельем",
							"розовым колдовским зельем",
							"настойкой аконита",
							"настойкой скополии",
							"настойкой белены",
							"настойкой дурмана",
							"жгучей смесью",
							"\n"
};

// color of the various drinks
const char *color_liquid[] = {"прозрачной",
							  "коричневой",
							  "бордовой",
							  "золотистой",
							  "бурой",
							  "прозрачной",
							  "рубиновой",
							  "зеленой",
							  "чистой",
							  "светло зеленой",
							  "белой",
							  "коричневой",
							  "темно-коричневой",
							  "красной",
							  "прозрачной",
							  "кристально чистой",
							  "искрящейся",
							  "бордовой",
							  "лазурной",
							  "серебристой",
							  "золотистой", //20
							  "черной вязкой",
							  "бесцветной",
							  "фиолетовой",
							  "розовой",
							  "ядовитой",
							  "ядовитой",
							  "ядовитой",
							  "ядовитой",
							  "\n"
};

// effect of drinks on DRUNK, FULL, THIRST -- see values.doc
const int drink_aff[][3] = {
	{0, 1, -10},            // вода
	{2, -2, -3},            // пиво
	{5, -2, -2},            // вино
	{3, -2, -3},            // медовуха
	{1, -2, -5},            // квас
	{8, 0,
	 4},                // самогон (как и водка сушит, никак не влияет на голод можно пить хоть залейся, только запивать успевай)
	{0, -1, -8},            // морс
	{10, 0,
	 3},                // водка (водка не питье! после нее обезвоживание, пить можно сколько угодно, но сушняк будет)
	{3, -3, -3},            // брага
	{0, -2, -8},            // мед (мед тоже ж калорийный)
	{0, -3, -6},            // молоко
	{0, -1, -6},            // чай
	{0, -1, -6},            // кофе
	{0, -2, 1},            // кровь
	{0, -1, 2},            // соленая вода
	{0, 0, -13},            // родниковая вода
	{0, -1, 1},            // магическое зелье
	{0, -1, 1},            // красное магическое зелье
	{0, -1, 1},            // синее магическое зелье
	{0, -1, 1},            // белое магическое зелье
	{0, -1, 1},            // золотистое магическое зелье
	{0, -1, 1},            // черное магическое зелье
	{0, -1, 1},            // серое магическое зелье
	{0, -1, 1},            // фиолетовое магическое зелье
	{0, -1, 1},            // розовое магическое зелье
	{0, 0, 0},            // настойка аконита
	{0, 0, 0},            // настойка скополии
	{0, 0, 0},            // настойка белены
	{0, 0, 0}            // настойка дурмана
};

/**
* Зелья, перелитые в контейнеры, можно пить во время боя.
* На случай, когда придется добавлять еще пошенов, которые
* уже будут идти не подряд по номерам.
*/
bool is_potion(const ObjData *obj) {
	switch (GET_OBJ_VAL(obj, 2)) {
		case LIQ_POTION:
		case LIQ_POTION_RED:
		case LIQ_POTION_BLUE:
		case LIQ_POTION_WHITE:
		case LIQ_POTION_GOLD:
		case LIQ_POTION_BLACK:
		case LIQ_POTION_GREY:
		case LIQ_POTION_FUCHSIA:
		case LIQ_POTION_PINK: return true;
			break;
	}
	return false;
}

namespace drinkcon {

ObjVal::EValueKey init_spell_num(int num) {
	return num == 1
		   ? ObjVal::EValueKey::kPotionSpell1Num
		   : (num == 2
			  ? ObjVal::EValueKey::kPotionSpell2Num
			  : ObjVal::EValueKey::kPotionSpell3Num);
}

ObjVal::EValueKey init_spell_lvl(int num) {
	return num == 1
		   ? ObjVal::EValueKey::kPotionSpell1Lvl
		   : (num == 2
			  ? ObjVal::EValueKey::kPotionSpell2Lvl
			  : ObjVal::EValueKey::kPotionSpell3Lvl);
}

void reset_potion_values(CObjectPrototype *obj) {
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSpell1Num, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSpell1Lvl, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSpell2Num, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSpell2Lvl, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSpell3Num, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSpell3Lvl, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionPotency, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionBrewRoll, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSkill, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionStat, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kLiquidTimer, -1);
	obj->SetPotionValueKey(ObjVal::EValueKey::kLiquidPoison, -1);
}

/// уровень в зельях (GET_OBJ_VAL(from_obj, 0)) пока один на все заклы
bool copy_value(const CObjectPrototype *from_obj, CObjectPrototype *to_obj, int num) {
	if (GET_OBJ_VAL(from_obj, num) > 0) {
		to_obj->SetPotionValueKey(init_spell_num(num), GET_OBJ_VAL(from_obj, num));
		to_obj->SetPotionValueKey(init_spell_lvl(num), GET_OBJ_VAL(from_obj, 0));
		return true;
	}
	return false;
}

/// заполнение values емкости (to_obj) из зелья (from_obj)
void copy_potion_values(const CObjectPrototype *from_obj, CObjectPrototype *to_obj) {
	reset_potion_values(to_obj);
	bool copied = false;

	for (int i = 1; i <= 3; ++i) {
		if (copy_value(from_obj, to_obj, i)) {
			copied = true;
		}
	}

	if (copied) {
		to_obj->SetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum, GET_OBJ_VNUM(from_obj));
		// issue.potion-hotfix: the brewed-in strength is one per potion (not per spell), so carry the
		// potency + frozen noise roll once, letting a poured container cast exactly like the source.
		to_obj->SetPotionValueKey(ObjVal::EValueKey::kPotionPotency,
								  from_obj->GetPotionValueKey(ObjVal::EValueKey::kPotionPotency));
		to_obj->SetPotionValueKey(ObjVal::EValueKey::kPotionBrewRoll,
								  from_obj->GetPotionValueKey(ObjVal::EValueKey::kPotionBrewRoll));
		to_obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSkill,
								  from_obj->GetPotionValueKey(ObjVal::EValueKey::kPotionSkill));
		to_obj->SetPotionValueKey(ObjVal::EValueKey::kPotionStat,
								  from_obj->GetPotionValueKey(ObjVal::EValueKey::kPotionStat));
	}
}

} // namespace drinkcon

using namespace drinkcon;

ECastResult cast_potion_spell(CharData *ch, ObjData *obj, int num) {
	const auto spell_id = static_cast<ESpell>(obj->GetPotionValueKey(init_spell_num(num)));
	if (spell_id <= ESpell::kUndefined) {
		return ECastResult::kSuccess;
	}
	// issue.potion-hotfix P3: strength is brewed in, not taken from the drinker -- the crafted
	// kPotionPotency, or (non-crafted) the value a fixed-skill potion-maker would brew (PotionPotency).
	// kPotionBrewRoll is the frozen brew-luck z replayed by CalcNoisyAmount; absent (non-crafted) ->
	// NaN -> drawn at cast. One potency + roll for the whole potion.
	const float potency = PotionPotency(obj, spell_id);
	const int brew_roll = obj->GetPotionValueKey(ObjVal::EValueKey::kPotionBrewRoll);
	const double noise_z = (brew_roll > 0)
		? static_cast<double>(brew_roll) / ObjVal::kBrewRollScale - ObjVal::kBrewRollBias
		: std::numeric_limits<double>::quiet_NaN();
	return CallMagic(ch, ch, nullptr, world[ch->in_room], spell_id, 0, potency, noise_z, PotionCastSkill(obj));
}

int TryCastSpellsFromLiquid(CharData *ch, ObjData *jar) {
	if (is_potion(jar) && jar->GetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum) >= 0) {
		act("$n выпил$g зелья из $o1.", true, ch, jar, 0, kToRoom);
		SendMsgToChar(ch, "Вы выпили зелья из %s.\r\n", OBJN(jar, ch, grammar::ECase::kGen));

		//не очень понятно, но так было
		// issue.potion-hotfix: all three slots are spells again (potency/roll live in their own keys)
		for (int i = 1; i <= 3; ++i)
			if (cast_potion_spell(ch, jar, i) != ECastResult::kSuccess)
				break;

		SetBattleLag(ch, 1);
		jar->dec_weight();
		// все выпито
		jar->dec_val(1);

		if (GET_OBJ_VAL(jar, 1) <= 0
			&& jar->get_type() != EObjType::kFountain) {
			name_from_drinkcon(jar);
			jar->set_spec_param(0);
			reset_potion_values(jar);
		}
		TryDrinkPoison(ch, jar, 0);
		return 1;
	}
	return 0;
}

void drinkcon::generate_drinkcon_name(ObjData *to_obj, ESpell spell_id) {
	switch (spell_id) {
		// восстановление (красное) //
		case ESpell::kResfresh:
		case ESpell::kGroupRefresh: to_obj->set_val(2, LIQ_POTION_RED);
			name_to_drinkcon(to_obj, LIQ_POTION_RED);
			break;
			// насыщение (синее) //
		case ESpell::kFullFeed:
		case ESpell::kCommonMeal: to_obj->set_val(2, LIQ_POTION_BLUE);
			name_to_drinkcon(to_obj, LIQ_POTION_BLUE);
			break;
			// детекты (белое) //
		case ESpell::kDetectInvis:
		case ESpell::kAllSeeingEye:
		case ESpell::kDetectMagic:
		case ESpell::kMagicalGaze:
		case ESpell::kDetectPoison:
		case ESpell::kSnakeEyes:
		case ESpell::kDetectAlign:
		case ESpell::kGroupSincerity:
		case ESpell::kSenseLife:
		case ESpell::kEyeOfGods:
		case ESpell::kInfravision:
		case ESpell::kSightOfDarkness: to_obj->set_val(2, LIQ_POTION_WHITE);
			name_to_drinkcon(to_obj, LIQ_POTION_WHITE);
			break;
			// защитные (золотистое) //
		case ESpell::kArmor:
		case ESpell::kGroupArmor:
		case ESpell::kCloudly: to_obj->set_val(2, LIQ_POTION_GOLD);
			name_to_drinkcon(to_obj, LIQ_POTION_GOLD);
			break;
			// восстанавливающие здоровье (черное) //
		case ESpell::kCureCritic:
		case ESpell::kCureLight:
		case ESpell::kHeal:
		case ESpell::kGroupHeal:
		case ESpell::kCureSerious: to_obj->set_val(2, LIQ_POTION_BLACK);
			name_to_drinkcon(to_obj, LIQ_POTION_BLACK);
			break;
			// снимающее вредные аффекты (серое) //
		case ESpell::kCureBlind:
		case ESpell::kRemoveCurse:
		case ESpell::kRemoveHold:
		case ESpell::kRemoveSilence:
		case ESpell::kCureFever:
		case ESpell::kRemoveDeafness:
		case ESpell::kRemovePoison: to_obj->set_val(2, LIQ_POTION_GREY);
			name_to_drinkcon(to_obj, LIQ_POTION_GREY);
			break;
			// прочие полезности (фиолетовое) //
		case ESpell::kInvisible:
		case ESpell::kGroupInvisible:
		case ESpell::kStrength:
		case ESpell::kGroupStrength:
		case ESpell::kFly:
		case ESpell::kGroupFly:
		case ESpell::kBless:
		case ESpell::kGroupBless:
		case ESpell::kHaste:
		case ESpell::kGroupHaste:
		case ESpell::kStoneSkin:
		case ESpell::kStoneWall:
		case ESpell::kBlink:
		case ESpell::kExtraHits:
		case ESpell::kWaterbreath: to_obj->set_val(2, LIQ_POTION_FUCHSIA);
			name_to_drinkcon(to_obj, LIQ_POTION_FUCHSIA);
			break;
		case ESpell::kPrismaticAura:
		case ESpell::kGroupPrismaticAura:
		case ESpell::kAirAura:
		case ESpell::kEarthAura:
		case ESpell::kFireAura:
		case ESpell::kIceAura: to_obj->set_val(2, LIQ_POTION_PINK);
			name_to_drinkcon(to_obj, LIQ_POTION_PINK);
			break;
		default: to_obj->set_val(2, LIQ_POTION);
			name_to_drinkcon(to_obj, LIQ_POTION);    // добавляем новый синоним //
	}
}

int check_potion_spell(ObjData *from_obj, ObjData *to_obj, int num) {
	const auto spell = init_spell_num(num);
	const auto level = init_spell_lvl(num);

	if (GET_OBJ_VAL(from_obj, num) != to_obj->GetPotionValueKey(spell)) {
		// не совпали заклы
		return 0;
	}
	if (GET_OBJ_VAL(from_obj, 0) < to_obj->GetPotionValueKey(level)) {
		// переливаемое зелье ниже уровня закла в емкости
		return -1;
	}
	return 1;
}

/// \return 1 - можно переливать
///         0 - нельзя смешивать разные зелья
///        -1 - попытка перелить зелье с меньшим уровнем закла
int drinkcon::check_equal_potions(ObjData *from_obj, ObjData *to_obj) {
	// емкость с уже перелитым ранее зельем
	if (to_obj->GetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum) > 0
		&& GET_OBJ_VNUM(from_obj) != to_obj->GetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum)) {
		return 0;
	}
	// совпадение заклов и не меньшего уровня
	for (int i = 1; i <= 3; ++i) {
		if (GET_OBJ_VAL(from_obj, i) > 0) {
			int result = check_potion_spell(from_obj, to_obj, i);
			if (result <= 0) {
				return result;
			}
		}
	}
	return 1;
}

/// см check_equal_drinkcon()
int check_drincon_spell(ObjData *from_obj, ObjData *to_obj, int num) {
	const auto spell = init_spell_num(num);
	const auto level = init_spell_lvl(num);

	if (from_obj->GetPotionValueKey(spell) != to_obj->GetPotionValueKey(spell)) {
		// не совпали заклы
		return 0;
	}
	if (from_obj->GetPotionValueKey(level) < to_obj->GetPotionValueKey(level)) {
		// переливаемое зелье ниже уровня закла в емкости
		return -1;
	}
	return 1;
}

/// Временная версия check_equal_potions для двух емкостей, пока в пошенах
/// еще нет values с заклами/уровнями.
/// \return 1 - можно переливать
///         0 - нельзя смешивать разные зелья
///        -1 - попытка перелить зелье с меньшим уровнем закла
int drinkcon::check_equal_drinkcon(ObjData *from_obj, ObjData *to_obj) {
	// совпадение заклов и не меньшего уровня (и в том же порядке, ибо влом)
	for (int i = 1; i <= 3; ++i) {
		if (GET_OBJ_VAL(from_obj, i) > 0) {
			int result = check_drincon_spell(from_obj, to_obj, i);
			if (result <= 0) {
				return result;
			}
		}
	}
	return 1;
}

/// копирование полей заклинаний при переливании из емкости с зельем в пустую емкость
void drinkcon::spells_to_drinkcon(ObjData *from_obj, ObjData *to_obj) {
	// инит заклов
	for (int i = 1; i <= 3; ++i) {
		const auto spell = init_spell_num(i);
		const auto level = init_spell_lvl(i);
		to_obj->SetPotionValueKey(spell, from_obj->GetPotionValueKey(spell));
		to_obj->SetPotionValueKey(level, from_obj->GetPotionValueKey(level));
	}
	// сохранение инфы о первоначальном источнике зелья
	const int proto_vnum = from_obj->GetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum) > 0
						   ? from_obj->GetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum)
						   : GET_OBJ_VNUM(from_obj);
	to_obj->SetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum, proto_vnum);
	// issue.potion-hotfix: carry the FULL brewed stats, not just the spells + proto vnum. A brewed
	// potion's strength is its maker skill/stat (+ the frozen roll); the vnum alone can't recover it.
	for (const auto key : {ObjVal::EValueKey::kPotionSkill, ObjVal::EValueKey::kPotionStat,
						   ObjVal::EValueKey::kPotionPotency, ObjVal::EValueKey::kPotionBrewRoll}) {
		to_obj->SetPotionValueKey(key, from_obj->GetPotionValueKey(key));
	}
}

// issue.potion-hotfix: pouring one potion (n_from units) into another (n_to units) blends the two
// makers' inputs by quantity -- e.g. 18 units at skill 100 mixed with 2 at skill 80 gives
// (18*100 + 2*80)/20 = 98; the key stat blends the same way, and the re-derived potency follows.
// The spells must already match (the caller checks). Uses each side's EFFECTIVE value so an authored
// potion (no stored skill/stat) contributes its authored maker (80/25), not zero.
void drinkcon::mix_potion_values(ObjData *to_obj, const ObjData *from_obj, int n_to, int n_from) {
	if (n_to + n_from <= 0) {
		return;
	}
	const int skill = (n_to * PotionCastSkill(to_obj) + n_from * PotionCastSkill(from_obj)) / (n_to + n_from);
	const int stat  = (n_to * PotionCastStat(to_obj)  + n_from * PotionCastStat(from_obj))  / (n_to + n_from);
	to_obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSkill, std::max(1, skill));
	to_obj->SetPotionValueKey(ObjVal::EValueKey::kPotionStat, std::max(1, stat));
}

int drinkcon::age_contents(ObjData *obj) {
	if (!obj) {
		return 0;
	}
	const auto type = obj->get_type();
	if (type != EObjType::kLiquidContainer && type != EObjType::kFood) {
		return 0;
	}
	int timer = obj->GetPotionValueKey(ObjVal::EValueKey::kLiquidTimer);
	if (timer <= 0) {
		return 0;    // 0 / unset -> no freshness clock (never spoils, or already spoiled)
	}
	timer -= 1;
	obj->SetPotionValueKey(ObjVal::EValueKey::kLiquidTimer, timer);
	if (timer == 0) {
		spoil_potion(obj);
	}
	return timer;
}

void drinkcon::spoil_potion(ObjData *obj) {
	if (!obj) {
		return;
	}
	// Poison level = a fixed percentage of the power the potion has RIGHT NOW (before it is weakened),
	// read from its primary spell's potency. Compute it first, then halve the power inputs so the next
	// spoilage cycle leaves an even weaker -- eventually inert -- potion.
	const auto spell = static_cast<ESpell>(obj->GetPotionValueKey(ObjVal::EValueKey::kPotionSpell1Num));
	float power = 0.0f;
	if (spell > ESpell::kUndefined) {
		power = PotionPotency(obj, spell);
	}
	int poison = 0;
	if (power > 0.0f) {
		poison = std::max(1, static_cast<int>(std::lround(power * kSpoiledPoisonPercent / 100.0)));
	}
	obj->SetPotionValueKey(ObjVal::EValueKey::kLiquidPoison, poison);

	// Halve the effective maker skill/stat and store them EXPLICITLY. PotionCastSkill/Stat fall back to
	// the authored default only when the key is ABSENT, so once stored a value keeps halving toward 0.
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionSkill, PotionCastSkill(obj) / 2);
	obj->SetPotionValueKey(ObjVal::EValueKey::kPotionStat,  PotionCastStat(obj)  / 2);
}

size_t find_liquid_name(const char *name) {
	std::string tmp = std::string(name);
	size_t pos, result = std::string::npos;
	for (int i = 0; strcmp(drinknames[i], "\n"); i++) {
		pos = tmp.find(drinknames[i]);
		if (pos != std::string::npos) {
			result = pos;
		}
	}
	return result;
}

void name_from_drinkcon(ObjData *obj) {
	char new_name[kMaxStringLength];
	std::string tmp;

	size_t pos = find_liquid_name(obj->get_aliases().c_str());
	if (pos == std::string::npos) return;
	tmp = obj->get_aliases().substr(0, pos - 1);

	sprintf(new_name, "%s", tmp.c_str());
	obj->set_aliases(new_name);

	pos = find_liquid_name(obj->get_short_description().c_str());
	if (pos == std::string::npos) return;
	tmp = obj->get_short_description().substr(0, pos - 3);

	sprintf(new_name, "%s", tmp.c_str());
	obj->set_short_description(new_name);

	for (int c = grammar::ECase::kFirstCase; c <= grammar::ECase::kLastCase; c++) {
		auto name_case = static_cast<grammar::ECase>(c);
		pos = find_liquid_name(obj->get_PName(name_case).c_str());
		if (pos == std::string::npos) return;
		tmp = obj->get_PName(name_case).substr(0, pos - 3);
		sprintf(new_name, "%s", tmp.c_str());
		obj->set_PName(name_case, new_name);
	}
}

void name_to_drinkcon(ObjData *obj, int type) {
	int c;
	char new_name[kMaxInputLength], potion_name[kMaxInputLength];
	if (type >= NUM_LIQ_TYPES) {
		snprintf(potion_name, kMaxInputLength, "%s", "непонятной бормотухой, сообщите БОГАМ");
	} else {
		snprintf(potion_name, kMaxInputLength, "%s", drinknames[type]);
	}

	snprintf(new_name, kMaxInputLength, "%s %s", obj->get_aliases().c_str(), potion_name);
	obj->set_aliases(new_name);
	snprintf(new_name, kMaxInputLength, "%s с %s", obj->get_short_description().c_str(), potion_name);
	obj->set_short_description(new_name);

	for (c = grammar::ECase::kFirstCase; c <= grammar::ECase::kLastCase; c++) {
		auto name_case = static_cast<grammar::ECase>(c);
		snprintf(new_name, kMaxInputLength, "%s с %s", obj->get_PName(name_case).c_str(), potion_name);
		obj->set_PName(name_case, new_name);
	}
}

std::string print_spell(const ObjData *obj, int num) {
	const auto spell = init_spell_num(num);
	const auto spell_id = static_cast<ESpell>(obj->GetPotionValueKey(spell));

	if (obj->GetPotionValueKey(spell) == -1 || !MUD::Spell(spell_id).IsValid()) {
		return "";
	}

	// issue.potion-olc-values: a magic drink shows the same maker-derived potency
	// (сила) as a potion, not the retired per-spell level.
	const int potency = static_cast<int>(PotionPotency(obj, spell_id) + 0.5f);
	char buf_[kMaxInputLength];
	snprintf(buf_, sizeof(buf_), "Содержит заклинание: %s%s (сила %d)%s\r\n",
			 kColorCyn,
			 MUD::Spell(spell_id).GetCName(),
			 potency,
			 kColorNrm);

	return buf_;
}

namespace drinkcon {

std::string print_spells(const ObjData *obj) {
	std::string out;
	char buf_[kMaxInputLength];

	for (int i = 1; i <= 3; ++i) {
		out += print_spell(obj, i);
	}

	if (!out.empty() && !is_potion(obj)) {
		snprintf(buf_, sizeof(buf_), "%sВНИМАНИЕ%s: тип жидкости не является зельем\r\n",
				 kColorBoldRed, kColorNrm);
		out += buf_;
	} else if (out.empty() && is_potion(obj)) {
		snprintf(buf_, sizeof(buf_), "%sВНИМАНИЕ%s: у данного зелья отсутствуют заклинания\r\n",
				 kColorBoldRed, kColorNrm);
		out += buf_;
	}

	return out;
}

void identify(CharData *ch, const ObjData *obj) {
	std::string out;
	char buf_[kMaxInputLength];
	int volume = GET_OBJ_VAL(obj, 0);
	int amount = GET_OBJ_VAL(obj, 1);

	snprintf(buf_, sizeof(buf_), "Может вместить зелья: %s%d %s%s\r\n",
			 kColorCyn,
			 volume, grammar::GetDeclensionInNumber(volume, grammar::EWhat::kGulp),
			 kColorNrm);
	out += buf_;

	// емкость не пуста
	if (amount > 0) {
		// есть какие-то заклы
		if (obj->GetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum) >= 0) {
			if (privilege::IsImmortal(ch)) {
				snprintf(buf_, sizeof(buf_), "Содержит %d %s %s (VNUM: %d).\r\n",
						 amount,
						 grammar::GetDeclensionInNumber(amount, grammar::EWhat::kGulp),
						 drinks[GET_OBJ_VAL(obj, 2)],
						 obj->GetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum));
			} else {
				snprintf(buf_, sizeof(buf_), "Содержит %d %s %s.\r\n",
						 amount,
						 grammar::GetDeclensionInNumber(amount, grammar::EWhat::kGulp),
						 drinks[GET_OBJ_VAL(obj, 2)]);
			}
			out += buf_;
			out += print_spells(obj);
		} else {
			snprintf(buf_, sizeof(buf_), "Заполнен%s %s на %d%%\r\n",
					 grammar::ObjSexEnding((obj)->get_sex(), 6),
					 drinknames[GET_OBJ_VAL(obj, 2)],
					 amount * 100 / (volume ? volume : 1));
			out += buf_;
			// чтобы выдать варнинг на тему зелья без заклов
			if (is_potion(obj)) {
				out += print_spells(obj);
			}
		}
	}
	if (amount > 0) //если что-то плескается
	{
		sprintf(buf1, "Качество: %s \r\n", diag_liquid_timer(obj)); // состояние жижки
		out += buf1;
	}
	SendMsgToChar(out, ch);
}

char *daig_filling_drink(const ObjData *obj, const CharData *ch) {
	char tmp[256];
	if (GET_OBJ_VAL(obj, 1) <= 0) {
		sprintf(buf1, "Пусто");
		return buf1;
	}
	else {
		if (GET_OBJ_VAL(obj, 0) <= 0 || GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0)) {
			sprintf(buf1, "Заполнен%s вакуумом?!", grammar::ObjSexEnding((obj)->get_sex(), 6));    // BUG
			return buf1;
		}
		else {
			const char *msg = AFF_FLAGGED(ch, EAffect::kDetectPoison)
				&& obj->GetPotionValueKey(ObjVal::EValueKey::kLiquidPoison) > 0 ? " *отравленной*" : "";
			int amt = (GET_OBJ_VAL(obj, 1) * 5) / GET_OBJ_VAL(obj, 0);
			sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, tmp);
			snprintf(buf1, kMaxStringLength,
					 "Наполнен%s %s%s%s жидкостью", grammar::ObjSexEnding((obj)->get_sex(), 6), fullness[amt], tmp, msg);
			return buf1;
		}
	}
}

const char *diag_liquid_timer(const ObjData *obj) {
	// issue.potion-hotfix: freshness now lives in kLiquidTimer (a countdown), spoilage in kLiquidPoison.
	if (obj->GetPotionValueKey(ObjVal::EValueKey::kLiquidPoison) > 0)
		return "испортилось!";
	const int tm = obj->GetPotionValueKey(ObjVal::EValueKey::kLiquidTimer);
	if (tm <= 0)   // 0 / absent -> no spoilage clock running
		return "идеальное.";
	if (tm < 1440) // сутки
		return "скоро испортится!";
	else if (tm < 10080) //неделя
		return "сомнительное.";
	else if (tm < 20160) // 2 недели
		return "выглядит свежим.";
	else if (tm < 30240) // 3 недели
		return "свежее.";
	return "идеальное.";
}

} // namespace drinkcon

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
