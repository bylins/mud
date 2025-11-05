#include "fight_hit.h"

#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/magic/magic.h"
#include "pk.h"
#include "gameplay/statistics/dps.h"
#include "gameplay/clans/house_exp.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/mechanics/poison.h"
#include "gameplay/mechanics/bonus.h"
#include "gameplay/ai/mobact.h"
#include "fight.h"
#include "engine/db/global_objects.h"
#include "utils/backtrace.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/classes/classes.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/affects/affect_data.h"
#include "utils/utils_time.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/skills/punctual_style.h"
#include "gameplay/skills/intercept.h"
#include "gameplay/skills/overhelm.h"
#include "gameplay/skills/mighthit.h"
#include "gameplay/skills/deviate.h"
#include "gameplay/skills/parry.h"
#include "gameplay/skills/multyparry.h"
#include "gameplay/skills/shield_block.h"
#include "gameplay/skills/backstab.h"

// extern
void npc_groupbattle(CharData *ch);

/**
* Расчет множителя дамаги пушки с концентрацией силы.
* Текущая: 1 + ((сила-25)*0.4 + левел*0.2)/10 * ремы/5,
* в цифрах множитель выглядит от 1 до 2.6 с равномерным
* распределением 62.5% на силу и 37.5% на уровни + штрафы до 5 ремов.
* Способность не плюсуется при железном ветре и оглушении.
*/
int CalcStrCondensationDamage(CharData *ch, ObjData * /*wielded*/, int damage) {
	if (ch->IsNpc()
		|| GetRealStr(ch) <= 25
		|| !CanUseFeat(ch, EFeat::kStrengthConcentration)
		|| ch->battle_affects.get(kEafIronWind)
		|| ch->battle_affects.get(kEafOverwhelm)) {
		return damage;
	}
	double str_mod = (GetRealStr(ch) - 25.0) * 0.4;
	double lvl_mod = GetRealLevel(ch) * 0.2;
	double rmt_mod = std::min(5, GetRealRemort(ch)) / 5.0;
	double res_mod = 1.0 + (str_mod + lvl_mod) / 10.0 * rmt_mod;

	return static_cast<int>(damage * res_mod);
}

/**
* Расчет прибавки дамаги со скрытого стиля.
* (скилл/5 + реморты*3) * (среднее/(10 + среднее/2)) * (левел/30)
*/
int CalcNoparryhitDmg(CharData *ch, ObjData *wielded) {
	if (!ch->GetSkill(ESkill::kNoParryHit)) return 0;

	double weap_dmg = (((GET_OBJ_VAL(wielded, 2) + 1) / 2.0) * GET_OBJ_VAL(wielded, 1));
	double weap_mod = weap_dmg / (10.0 + weap_dmg / 2.0);
	double level_mod = static_cast<double>(GetRealLevel(ch)) / 30.0;
	double skill_mod = static_cast<double>(ch->GetSkill(ESkill::kNoParryHit)) / 5.0;

	return static_cast<int>((skill_mod + GetRealRemort(ch) * 3.0) * weap_mod * level_mod);
}

// * При надуве выше х 1.5 в пк есть 1% того, что весь надув слетит одним ударом.
void TryRemoveExtrahits(CharData *ch, CharData *victim) {
	if (((!ch->IsNpc() && ch != victim)
		|| (ch->has_master()
			&& !ch->get_master()->IsNpc()
			&& ch->get_master() != victim))
		&& !victim->IsNpc()
		&& victim->GetPosition() != EPosition::kDead
		&& victim->get_hit() > victim->get_real_max_hit() * 1.5
		&& number(1, 100) == 5)// пусть будет 5, а то 1 по субъективным ощущениям выпадает как-то часто
	{
		victim->set_hit(victim->get_real_max_hit());
		SendMsgToChar(victim, "%s'Будь%s тощ%s аки прежде' - мелькнула чужая мысль в вашей голове.%s\r\n",
					  kColorWht, GET_CH_POLY_1(victim), GET_CH_EXSUF_1(victim), kColorNrm);
		act("Вы прервали золотистую нить, питающую $N3 жизнью.", false, ch, nullptr, victim, kToChar);
		act("$n прервал$g золотистую нить, питающую $N3 жизнью.", false, ch, nullptr, victim, kToNotVict | kToArenaListen);
	}
}

void ProcessAddshotHits(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon) {
	int prob = CalcCurrentSkill(ch, ESkill::kAddshot, ch->GetEnemy());
	int dex_mod = std::max(GetRealDex(ch) - 25, 0) * 10;
	int pc_mod =IS_CHARMICE(ch) ? 0 : 1;
	auto difficulty = MUD::Skill(ESkill::kAddshot).difficulty * 5;
	int percent = number(1, difficulty);

	TrainSkill(ch, ESkill::kAddshot, true, ch->GetEnemy());
	if (percent <= prob * 9 + dex_mod) {
		hit(ch, victim, type, weapon);
	}
	percent = number(1, difficulty);
	if (percent <= (prob * 6 + dex_mod) && ch->GetEnemy()) {
		hit(ch, victim, type, weapon);
	}
	percent = number(1, difficulty);
	if (percent <= (prob * 4 + dex_mod / 2) * pc_mod && ch->GetEnemy()) {
		hit(ch, victim, type, weapon);
	}
	percent = number(1, difficulty);
	if (percent <= (prob * 19 / 8 + dex_mod / 2) * pc_mod && ch->GetEnemy()) {
		hit(ch, victim, type, weapon);
	}
}

// бонусы/штрафы классам за юзание определенных видов оружия (редкостный бред)
void GetClassWeaponMod(ECharClass class_id, const ESkill skill, int *damroll, int *hitroll) {
	int dam = *damroll;
	int calc_thaco = *hitroll;

	switch (class_id) {
		case ECharClass::kSorcerer:
			switch (skill) {
				case ESkill::kLongBlades:
					calc_thaco += 2;
					dam -= 1;
					break;
				case ESkill::kNonstandart:
					calc_thaco += 1;
					dam -= 2;
					break;
				default: break;
			}
			break;

		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer: [[fallthrough]];
		case ECharClass::kNecromancer:
			switch (skill) {
				case ESkill::kAxes:
				case ESkill::kLongBlades: [[fallthrough]];
				case ESkill::kSpades:
					calc_thaco += 1;
					dam += 0;
					break;
				case ESkill::kTwohands:
					calc_thaco += 1;
					dam -= 3;
					break;
				default: break;
			}
			break;

		case ECharClass::kWarrior:
			switch (skill) {
				case ESkill::kShortBlades: [[fallthrough]];
				case ESkill::kPicks:
					calc_thaco += 2;
					dam += 0;
					break;
				default: break;
			}
			break;

		case ECharClass::kRanger:
			switch (skill) {
				case ESkill::kLongBlades: [[fallthrough]];
				case ESkill::kTwohands: calc_thaco += 1;
					dam += 0;
					break;
				default: break;
			}
			break;

		case ECharClass::kGuard:
		case ECharClass::kThief: [[fallthrough]];
		case ECharClass::kAssasine:
			switch (skill) {
				case ESkill::kAxes:
				case ESkill::kLongBlades:
				case ESkill::kTwohands: [[fallthrough]];
				case ESkill::kSpades:
					calc_thaco += 1;
					dam += 0;
					break;
				default: break;
			}
			break;
		case ECharClass::kMerchant:
			switch (skill) {
				case ESkill::kLongBlades: [[fallthrough]];
				case ESkill::kTwohands:
					calc_thaco += 1;
					dam += 0;
					break;
				default: break;
			}
			break;

		case ECharClass::kMagus:
			switch (skill) {
				case ESkill::kLongBlades:
				case ESkill::kTwohands: [[fallthrough]];
				case ESkill::kBows:
					calc_thaco += 1;
					dam += 0;
					break;
				default: break;
			}
			break;
		default: break;
	}

	*damroll = dam;
	*hitroll = calc_thaco;
}

// * Проверка на фит "любимое оружие".
void HitData::CheckWeapFeats(const CharData *ch, ESkill weap_skill, int &calc_thaco, int &dam) {
	switch (weap_skill) {
		case ESkill::kPunch:
			if (ch->HaveFeat(EFeat::kPunchFocus)) {
				calc_thaco -= 2;
				dam += 2;
			}
			break;
		case ESkill::kClubs:
			if (ch->HaveFeat(EFeat::kClubsFocus)) {
				calc_thaco -= 2;
				dam += 2;
			}
			break;
		case ESkill::kAxes:
			if (ch->HaveFeat(EFeat::kAxesFocus)) {
				calc_thaco -= 1;
				dam += 2;
			}
			break;
		case ESkill::kLongBlades:
			if (ch->HaveFeat(EFeat::kLongsFocus)) {
				calc_thaco -= 1;
				dam += 2;
			}
			break;
		case ESkill::kShortBlades:
			if (ch->HaveFeat(EFeat::kShortsFocus)) {
				calc_thaco -= 2;
				dam += 3;
			}
			break;
		case ESkill::kNonstandart:
			if (ch->HaveFeat(EFeat::kNonstandartsFocus)) {
				calc_thaco -= 1;
				dam += 3;
			}
			break;
		case ESkill::kTwohands:
			if (ch->HaveFeat(EFeat::kTwohandsFocus)) {
				calc_thaco -= 1;
				dam += 3;
			}
			break;
		case ESkill::kPicks:
			if (ch->HaveFeat(EFeat::kPicksFocus)) {
				calc_thaco -= 2;
				dam += 3;
			}
			break;
		case ESkill::kSpades:
			if (ch->HaveFeat(EFeat::kSpadesFocus)) {
				calc_thaco -= 1;
				dam += 2;
			}
			break;
		case ESkill::kBows:
			if (ch->HaveFeat(EFeat::kBowsFocus)) {
				calc_thaco -= 7;
				dam += 4;
			}
			break;
		default: break;
	}
}

void Appear(CharData *ch) {
	const bool appear_msg = AFF_FLAGGED(ch, EAffect::kInvisible)
		|| AFF_FLAGGED(ch, EAffect::kDisguise)
		|| AFF_FLAGGED(ch, EAffect::kHide);

	RemoveAffectFromChar(ch, ESpell::kInvisible);
	RemoveAffectFromChar(ch, ESpell::kHide);
	RemoveAffectFromChar(ch, ESpell::kSneak);
	RemoveAffectFromChar(ch, ESpell::kCamouflage);

	AFF_FLAGS(ch).unset(EAffect::kInvisible);
	AFF_FLAGS(ch).unset(EAffect::kHide);
	AFF_FLAGS(ch).unset(EAffect::kSneak);
	AFF_FLAGS(ch).unset(EAffect::kDisguise);

	if (appear_msg) {
		if (ch->IsNpc() || GetRealLevel(ch) < kLvlImmortal) {
			act("$n медленно появил$u из пустоты.", false, ch, nullptr, nullptr, kToRoom);
		} else {
			act("Вы почувствовали странное присутствие $n1.",
				false, ch, nullptr, nullptr, kToRoom);
		}
	}
}

// message for doing damage with a weapon
void Damage::SendDmgMsg(CharData *ch, CharData *victim) const {
	int dam_msgnum;
	int w_type = msg_num;

	static const struct dam_weapon_type {
		const char *to_room;
		const char *to_char;
		const char *to_victim;
	} dam_weapons[] =
		{

			// use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes")

			{
				"$n попытал$u #W $N3, но промахнул$u.",    // 0: 0      0 //
				"Вы попытались #W $N3, но промахнулись.",
				"$n попытал$u #W вас, но промахнул$u."
			}, {
				"$n легонько #w$g $N3.",    //  1..5 1 //
				"Вы легонько #wи $N3.",
				"$n легонько #w$g вас."
			}, {
				"$n слегка #w$g $N3.",    //  6..11  2 //
				"Вы слегка #wи $N3.",
				"$n слегка #w$g вас."
			}, {
				"$n #w$g $N3.",    //  12..18   3 //
				"Вы #wи $N3.",
				"$n #w$g вас."
			}, {
				"$n #w$g $N3.",    // 19..26  4 //
				"Вы #wи $N3.",
				"$n #w$g вас."
			}, {
				"$n сильно #w$g $N3.",    // 27..35  5 //
				"Вы сильно #wи $N3.",
				"$n сильно #w$g вас."
			}, {
				"$n очень сильно #w$g $N3.",    //  36..45 6  //
				"Вы очень сильно #wи $N3.",
				"$n очень сильно #w$g вас."
			}, {
				"$n чрезвычайно сильно #w$g $N3.",    //  46..55  7 //
				"Вы чрезвычайно сильно #wи $N3.",
				"$n чрезвычайно сильно #w$g вас."
			}, {
				"$n БОЛЬНО #w$g $N3.",    //  56..96   8 //
				"Вы БОЛЬНО #wи $N3.",
				"$n БОЛЬНО #w$g вас."
			}, {
				"$n ОЧЕНЬ БОЛЬНО #w$g $N3.",    //    97..136  9  //
				"Вы ОЧЕНЬ БОЛЬНО #wи $N3.",
				"$n ОЧЕНЬ БОЛЬНО #w$g вас."
			}, {
				"$n ЧРЕЗВЫЧАЙНО БОЛЬНО #w$g $N3.",    //   137..176  10 //
				"Вы ЧРЕЗВЫЧАЙНО БОЛЬНО #wи $N3.",
				"$n ЧРЕЗВЫЧАЙНО БОЛЬНО #w$g вас."
			}, {
				"$n НЕВЫНОСИМО БОЛЬНО #w$g $N3.",    //    177..216  11 //
				"Вы НЕВЫНОСИМО БОЛЬНО #wи $N3.",
				"$n НЕВЫНОСИМО БОЛЬНО #w$g вас."
			}, {
				"$n ЖЕСТОКО #w$g $N3.",    //    217..256  13 //
				"Вы ЖЕСТОКО #wи $N3.",
				"$n ЖЕСТОКО #w$g вас."
			}, {
				"$n УЖАСНО #w$g $N3.",    //    257..296  13 //
				"Вы УЖАСНО #wи $N3.",
				"$n УЖАСНО #w$g вас."
			}, {
				"$n УБИЙСТВЕННО #w$g $N3.",    //    297..400  15 //
				"Вы УБИЙСТВЕННО #wи $N3.",
				"$n УБИЙСТВЕННО #w$g вас."
			}, {
				"$n ИЗУВЕРСКИ #w$g $N3.",    //    297..400  15 //
				"Вы ИЗУВЕРСКИ #wи $N3.",
				"$n ИЗУВЕРСКИ #w$g вас."
			}, {
				"$n СМЕРТЕЛЬНО #w$g $N3.",    // 400+  16 //
				"Вы СМЕРТЕЛЬНО #wи $N3.",
				"$n СМЕРТЕЛЬНО #w$g вас."
			}
		};

	if (w_type >= kTypeHit && w_type < kTypeMagic)
		w_type -= kTypeHit;    // Change to base of table with text //
	else
		w_type = kTypeHit;

	if (dam == 0)
		dam_msgnum = 0;
	else if (dam <= 5)
		dam_msgnum = 1;
	else if (dam <= 11)
		dam_msgnum = 2;
	else if (dam <= 18)
		dam_msgnum = 3;
	else if (dam <= 26)
		dam_msgnum = 4;
	else if (dam <= 35)
		dam_msgnum = 5;
	else if (dam <= 45)
		dam_msgnum = 6;
	else if (dam <= 56)
		dam_msgnum = 7;
	else if (dam <= 96)
		dam_msgnum = 8;
	else if (dam <= 136)
		dam_msgnum = 9;
	else if (dam <= 176)
		dam_msgnum = 10;
	else if (dam <= 216)
		dam_msgnum = 11;
	else if (dam <= 256)
		dam_msgnum = 12;
	else if (dam <= 296)
		dam_msgnum = 13;
	else if (dam <= 400)
		dam_msgnum = 14;
	else if (dam <= 800)
		dam_msgnum = 15;
	else
		dam_msgnum = 16;
	// damage message to onlookers
	char *buf_ptr = replace_string(dam_weapons[dam_msgnum].to_room,
								   attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	if (brief_shields_.empty()) {
		act(buf_ptr, false, ch, nullptr, victim, kToNotVict | kToArenaListen);
	} else {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "%s%s", buf_ptr, brief_shields_.c_str());
		act(buf_, false, ch, nullptr, victim, kToNotVict | kToArenaListen | kToBriefShields);
		act(buf_ptr, false, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
	}

	// damage message to damager
	SendMsgToChar(ch, "%s", dam ? "&Y&q" : "&y&q");
	if (!brief_shields_.empty() && ch->IsFlagged(EPrf::kBriefShields)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "%s%s",
				 replace_string(dam_weapons[dam_msgnum].to_char,
								attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
				 brief_shields_.c_str());
		act(buf_, false, ch, nullptr, victim, kToChar);
	} else {
		buf_ptr = replace_string(dam_weapons[dam_msgnum].to_char,
								 attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
		act(buf_ptr, false, ch, nullptr, victim, kToChar);
	}
	SendMsgToChar("&Q&n", ch);

	// damage message to damagee
	SendMsgToChar("&R&q", victim);
	if (!brief_shields_.empty() && victim->IsFlagged(EPrf::kBriefShields)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "%s%s",
				 replace_string(dam_weapons[dam_msgnum].to_victim,
								attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
				 brief_shields_.c_str());
		act(buf_, false, ch, nullptr, victim, kToVict | kToSleep);
	} else {
		buf_ptr = replace_string(dam_weapons[dam_msgnum].to_victim,
								 attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
		act(buf_ptr, false, ch, nullptr, victim, kToVict | kToSleep);
	}
	SendMsgToChar("&Q&n", victim);
}

bool Damage::CalcMagisShieldsDmgAbsoption(CharData *ch, CharData *victim) {
	if (dam <= 0) {
		return false;
	}

	// отражение части маг дамага от зеркала
	if (AFF_FLAGGED(victim, EAffect::kMagicGlass)
		&& dmg_type == fight::kMagicDmg) {
		int pct = 6;
		if (victim->IsNpc() && !IS_CHARMICE(victim)) {
			pct += 2;
			if (victim->get_role(MOB_ROLE_BOSS)) {
				pct += 2;
			}
		}
		// дамаг обратки
		const int mg_damage = dam * pct / 100;
		if (mg_damage > 0
			&& victim->GetEnemy()
			&& victim->GetPosition() > EPosition::kStun
			&& victim->in_room != kNowhere) {
			flags.set(fight::kDrawBriefMagMirror);
			Damage dmg(SpellDmg(ESpell::kMagicGlass), mg_damage, fight::kUndefDmg);
			dmg.flags.set(fight::kNoFleeDmg);
			dmg.flags.set(fight::kMagicReflect);
			dmg.Process(victim, ch);
		}
	}

	// обработка щитов, см Damage::post_init_shields()
	if (flags[fight::kVictimFireShield]
		&& !flags[fight::kCritHit]) {
		if (dmg_type == fight::kPhysDmg
			&& !flags[fight::kIgnoreFireShield]) {
			int pct = 15;
			if (victim->IsNpc() && !IS_CHARMICE(victim)) {
				pct += 5;
				if (victim->get_role(MOB_ROLE_BOSS)) {
					pct += 5;
				}
			}
			fs_damage = dam * pct / 100;
		} else {
			act("Огненный щит вокруг $N1 ослабил вашу атаку.",
				false, ch, nullptr, victim, kToChar | kToNoBriefShields);
			act("Огненный щит принял часть повреждений на себя.",
				false, ch, nullptr, victim, kToVict | kToNoBriefShields);
			act("Огненный щит вокруг $N1 ослабил атаку $n1.",
				true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		}
		flags.set(fight::kDrawBriefFireShield);
		dam -= (dam * number(30, 50) / 100);
	}

	// если критический удар (не точка и стаб) и есть щит - 95% шанс в молоко
	// критическим считается любой удар который вложиля в определенные границы
	if (dam
		&& flags[fight::kCritHit] && flags[fight::kVictimIceShield]
		&& !dam_critic
		&& spell_id != ESpell::kPoison
		&& number(0, 100) < 94) {
		act("Ваше меткое попадания частично утонуло в ледяной пелене вокруг $N1.",
			false, ch, nullptr, victim, kToChar | kToNoBriefShields);
		act("Меткое попадание частично утонуло в ледяной пелене щита.",
			false, ch, nullptr, victim, kToVict | kToNoBriefShields);
		act("Ледяной щит вокруг $N1 частично поглотил меткое попадание $n1.",
			true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);

		flags.reset(fight::kCritHit);
		if (dam > 0) dam -= (dam * number(30, 50) / 100);
	}
		//шоб небуло спама модернизировал условие
	else if (dam > 0
		&& flags[fight::kVictimIceShield]
		&& !flags[fight::kCritHit]) {
		flags.set(fight::kDrawBriefIceShield);
		act("Ледяной щит вокруг $N1 смягчил ваш удар.",
			false, ch, nullptr, victim, kToChar | kToNoBriefShields);
		act("Ледяной щит принял часть удара на себя.",
			false, ch, nullptr, victim, kToVict | kToNoBriefShields);
		act("Ледяной щит вокруг $N1 смягчил удар $n1.",
			true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		dam -= (dam * number(30, 50) / 100);
	}

	if (dam > 0
		&& flags[fight::kVictimAirShield]
		&& !flags[fight::kCritHit]) {
		flags.set(fight::kDrawBriefAirShield);
		act("Воздушный щит вокруг $N1 ослабил ваш удар.",
			false, ch, nullptr, victim, kToChar | kToNoBriefShields);
		act("Воздушный щит смягчил удар $n1.",
			false, ch, nullptr, victim, kToVict | kToNoBriefShields);
		act("Воздушный щит вокруг $N1 ослабил удар $n1.",
			true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
		dam -= (dam * number(30, 50) / 100);
	}

	return false;
}

void Damage::CalcArmorDmgAbsorption(CharData *victim) {
	// броня на физ дамаг
	if (dam > 0 && dmg_type == fight::kPhysDmg) {
		DamageEquipment(victim, kNowhere, dam, 50);
		if (!flags[fight::kCritHit] && !flags[fight::kIgnoreArmor]) {
			// 50 брони = 50% снижение дамага
			int max_armour = 50;
			if (CanUseFeat(victim, EFeat::kImpregnable) && victim->IsFlagged(EPrf::kAwake)) {
				// непробиваемый в осторожке - до 75 брони
				max_armour = 75;
			}
			int tmp_dam = dam * std::max(0, std::min(max_armour, GET_ARMOUR(victim))) / 100;
			// ополовинивание брони по флагу скила
			if (tmp_dam >= 2 && flags[fight::kHalfIgnoreArmor]) {
				tmp_dam /= 2;
			}
			dam -= tmp_dam;
			// крит удар умножает дамаг, если жертва без призмы и без лед.щита
		}
	}
}

/**
 * Обработка поглощения физ урона.
 * \return true - полное поглощение
 */
bool Damage::CalcDmgAbsorption(CharData *ch, CharData *victim) {
	if (dmg_type == fight::kPhysDmg
		&& skill_id < ESkill::kFirst
		&& spell_id < ESpell::kUndefined
		&& dam > 0
		&& GET_ABSORBE(victim) > 0) {
		// шансы поглощения: непробиваемый в осторожке 15%, остальные 10%
		int chance = 10 + GetRealRemort(victim) / 3;
		if (CanUseFeat(victim, EFeat::kImpregnable)
			&& victim->IsFlagged(EPrf::kAwake)) {
			chance += 5;
		}
		// физ урон - прямое вычитание из дамага
		if (number(1, 100) <= chance) {
			dam -= GET_ABSORBE(victim) / 2;
			if (dam <= 0) {
				act("Ваши доспехи полностью поглотили удар $n1.",
					false, ch, nullptr, victim, kToVict);
				act("Доспехи $N1 полностью поглотили ваш удар.",
					false, ch, nullptr, victim, kToChar);
				act("Доспехи $N1 полностью поглотили удар $n1.",
					true, ch, nullptr, victim, kToNotVict | kToArenaListen);
				return true;
			}
		}
	}
	if (dmg_type == fight::kMagicDmg
		&& dam > 0
		&& GET_ABSORBE(victim) > 0
		&& !flags[fight::kIgnoreAbsorbe]) {
// маг урон - по 1% за каждые 2 абсорба, максимум 25% (цифры из mag_damage)
		int absorb = std::min(GET_ABSORBE(victim) / 2, 25);
		dam -= dam * absorb / 100;
	}
	return false;
}

void Damage::SendCritHitMsg(CharData *ch, CharData *victim) {
	// Блочить мессагу крита при ледяном щите вроде нелогично,
	// так что добавил отдельные сообщения для ледяного щита (Купала)
	if (!flags[fight::kVictimIceShield]) {
		sprintf(buf, "&G&qВаше меткое попадание тяжело ранило %s.&Q&n\r\n",
				PERS(victim, ch, 3));
	} else {
		sprintf(buf, "&B&qВаше меткое попадание утонуло в ледяной пелене щита %s.&Q&n\r\n",
				PERS(victim, ch, 1));
	}

	SendMsgToChar(buf, ch);

	if (!flags[fight::kVictimIceShield]) {
		sprintf(buf, "&r&qМеткое попадание %s тяжело ранило вас.&Q&n\r\n",
				PERS(ch, victim, 1));
	} else {
		sprintf(buf, "&r&qМеткое попадание %s утонуло в ледяной пелене вашего щита.&Q&n\r\n",
				PERS(ch, victim, 1));
	}

	SendMsgToChar(buf, victim);
	// Закомментил чтобы не спамило, сделать потом в виде режима
	//act("Меткое попадание $N1 заставило $n3 пошатнуться.", true, victim, nullptr, ch, TO_NOTVICT);
}

void UpdateDpsStatistics(CharData *ch, int real_dam, int over_dam) {
	if (!ch->IsNpc()) {
		ch->dps_add_dmg(DpsSystem::PERS_DPS, real_dam, over_dam);
/*		log("DmetrLog. Name(player): %s, class: %d, remort:%d, level:%d, dmg: %d, over_dmg:%d",
			GET_NAME(ch),
			to_underlying(ch->GetClass()),
			GetRealRemort(ch),
			GetRealLevel(ch),
			real_dam,
			over_dam);
*/
		if (AFF_FLAGGED(ch, EAffect::kGroup)) {
			CharData *leader = ch->has_master() ? ch->get_master() : ch;
			leader->dps_add_dmg(DpsSystem::GROUP_DPS, real_dam, over_dam, ch);
		}
	} else if (IS_CHARMICE(ch)
		&& ch->has_master()) {
		ch->get_master()->dps_add_dmg(DpsSystem::PERS_CHARM_DPS, real_dam, over_dam, ch);
/*		if (!ch->get_master()->IsNpc()) {
			log("DmetrLog. Name(charmice): %s, name(master): %s, class: %d, remort: %d, level: %d, dmg: %d, over_dmg:%d",
				ch->get_name().c_str(), ch->get_master()->get_name().c_str(), to_underlying(ch->get_master()->GetClass()),
				GetRealRemort(ch->get_master()), GetRealLevel(ch->get_master()), real_dam, over_dam);
		}
*/
		if (AFF_FLAGGED(ch->get_master(), EAffect::kGroup)) {
			CharData *leader = ch->get_master()->has_master() ? ch->get_master()->get_master() : ch->get_master();
			leader->dps_add_dmg(DpsSystem::GROUP_CHARM_DPS, real_dam, over_dam, ch);
		}
	}
}

void CheckTutelarSelfSacrfice(CharData *ch, CharData *victim) {
	// если виктим в группе с кем-то с ангелом - вместо смерти виктима умирает ангел
	if (victim->get_hit() <= 0
		&& !victim->IsNpc()
		&& AFF_FLAGGED(victim, EAffect::kGroup)) {
		const auto people =
			world[victim->in_room]->people;    // make copy of people because keeper might be removed from this list inside the loop
		for (const auto keeper : people) {
			if (keeper->IsNpc()
				&& keeper->IsFlagged(EMobFlag::kTutelar)
				&& keeper->has_master()
				&& AFF_FLAGGED(keeper->get_master(), EAffect::kGroup)) {
				CharData *keeper_leader = keeper->get_master()->has_master()
										   ? keeper->get_master()->get_master()
										   : keeper->get_master();
				CharData *victim_leader = victim->has_master()
										   ? victim->get_master()
										   : victim;

				if ((keeper_leader == victim_leader) && (may_kill_here(keeper->get_master(), ch, nullptr))) {
					if (!pk_agro_action(keeper->get_master(), ch)) {
						return;
					}
					log("angel_sacrifice: Nmae (ch): %s, Name(charmice): %s, name(victim): %s", GET_NAME(ch), GET_NAME(keeper), GET_NAME(victim));

					SendMsgToChar(victim, "%s пожертвовал%s своей жизнью, вытаскивая вас с того света!\r\n",
								  GET_PAD(keeper, 0), GET_CH_SUF_1(keeper));
					snprintf(buf, kMaxStringLength, "%s пожертвовал%s своей жизнью, вытаскивая %s с того света!",
							 GET_PAD(keeper, 0), GET_CH_SUF_1(keeper), GET_PAD(victim, 3));
					act(buf, false, victim, nullptr, nullptr, kToRoom | kToArenaListen);

					ExtractCharFromWorld(keeper, 0);
					victim->set_hit(std::min(300, victim->get_max_hit() / 2));
				}
			}
		}
	}
}

void UpdatePkLogs(CharData *ch, CharData *victim) {
	ClanPkLog::check(ch, victim);
	sprintf(buf2, "%s killed by %s at %s [%d] ", GET_NAME(victim), GET_NAME(ch),
			victim->in_room != kNowhere ? world[victim->in_room]->name : "kNowhere", GET_ROOM_VNUM(victim->in_room));
	mudlog(buf2, CMP, kLvlImmortal, SYSLOG, true);

	if ((!ch->IsNpc()
		|| (ch->has_master()
			&& !ch->get_master()->IsNpc()))
		&& NORENTABLE(victim)
		&& !ROOM_FLAGGED(victim->in_room, ERoomFlag::kArena)) {
		mudlog(buf2, BRF, kLvlImplementator, SYSLOG, false);
		if (ch->IsNpc()
			&& (AFF_FLAGGED(ch, EAffect::kCharmed) || IS_HORSE(ch))
			&& ch->has_master()
			&& !ch->get_master()->IsNpc()) {
			sprintf(buf2, "%s is following %s.", GET_NAME(ch), GET_PAD(ch->get_master(), 2));
			mudlog(buf2, BRF, kLvlImplementator, SYSLOG, true);
		}
	}
}

void Damage::ProcessBlink(CharData *ch, CharData *victim) {
	if (flags[fight::kIgnoreBlink] || flags[fight::kCritLuck])
		return;
	ubyte blink = 0;
	// даже в случае попадания можно уклониться мигалкой
	if (dmg_type == fight::kMagicDmg) {
		if (AFF_FLAGGED(victim, EAffect::kCloudly) || victim->add_abils.percent_spell_blink_mag > 0) {
			if (victim->IsNpc()) {
				blink = GetRealLevel(victim) + GetRealRemort(victim);
			} else if(victim->add_abils.percent_spell_blink_mag > 0) {
				blink = victim->add_abils.percent_spell_blink_mag;
			} else {
				blink = 10;
			}
		}
	} else if(dmg_type == fight::kPhysDmg) {
		if (AFF_FLAGGED(victim, EAffect::kBlink) || victim->add_abils.percent_spell_blink_phys > 0) {
			if (victim->IsNpc()) {
				blink = GetRealLevel(victim) + GetRealRemort(victim);
			} else if (victim->add_abils.percent_spell_blink_phys > 0) {
				blink = victim->add_abils.percent_spell_blink_phys;
			} else {
				blink = 10;
			}
		}
	}
	if(blink < 1)
		return;
//	ch->send_to_TC(false, true, false, "Шанс мигалки равен == %d процентов.\r\n", blink);
//	victim->send_to_TC(false, true, false, "Шанс мигалки равен == %d процентов.\r\n", blink);
	int bottom = 1;
	if (ch->calc_morale() > number(1, 100)) // удача
		bottom = 10;
	if (number(bottom, blink) >= number(1, 100)) {
		sprintf(buf, "%sНа мгновение вы исчезли из поля зрения противника.%s\r\n",
				kColorBoldBlk, kColorNrm);
		SendMsgToChar(buf, victim);
		act("$n исчез$q из вашего поля зрения.", true, victim, nullptr, ch, kToVict);
		act("$n исчез$q из поля зрения $N1.", true, victim, nullptr, ch, kToNotVict);
		dam = 0;
		fs_damage = 0;
		return;
	}
}

void Damage::ProcessDeath(CharData *ch, CharData *victim) const {
	CharData *killer = nullptr;

	if (victim->IsNpc() || victim->desc) {
		if (victim == ch && victim->in_room != kNowhere) {
			if (spell_id == ESpell::kPoison) {
				for (const auto poisoner : world[victim->in_room]->people) {
					if (poisoner != victim
						&& GET_UID(poisoner) == victim->poisoner) {
						killer = poisoner;
					}
				}
			} else if (msg_num == kTypeSuffering) {
				for (const auto attacker : world[victim->in_room]->people) {
					if (attacker->GetEnemy() == victim) {
						killer = attacker;
					}
				}
			}
		}

		if (ch != victim) {
			killer = ch;
		}
	}
	if (killer) {
		if (AFF_FLAGGED(killer, EAffect::kGroup)) {
			// т.к. помечен флагом AFF_GROUP - точно PC
			group_gain(killer, victim);
		} else if ((AFF_FLAGGED(killer, EAffect::kCharmed)
			|| killer->IsFlagged(EMobFlag::kTutelar)
			|| killer->IsFlagged(EMobFlag::kMentalShadow))
			&& killer->has_master())
			// killer - зачармленный NPC с хозяином
		{
			// по логике надо бы сделать, что если хозяина нет в клетке, но
			// кто-то из группы хозяина в клетке, то опыт накинуть согруппам,
			// которые рядом с убившим моба чармисом.
			if (AFF_FLAGGED(killer->get_master(), EAffect::kGroup)
				&& killer->in_room == killer->get_master()->in_room) {
				// Хозяин - PC в группе => опыт группе
				group_gain(killer->get_master(), victim);
			} else if (killer->in_room == killer->get_master()->in_room) {
				perform_group_gain(killer->get_master(), victim, 1, 100);
			}
			// else
			// А хозяина то рядом не оказалось, все чармису - убрано
			// нефиг абьюзить чарм  group::perform_group_gain( killer, victim, 1, 100 );
		} else {
			// Просто NPC или PC сам по себе
			perform_group_gain(killer, victim, 1, 100);
		}
	}

	// в сислог иммам идут только смерти в пк (без арен)
	// в файл пишутся все смерти чаров
	// если чар убит палачем то тоже не спамим
	if (!victim->IsNpc() && !(killer && killer->IsFlagged(EPrf::kExecutor))) {
		UpdatePkLogs(ch, victim);

		for (const auto &ch_vict : world[ch->in_room]->people) {
			//Мобы все кто присутствовал при смерти игрока забывают
			if (IS_IMMORTAL(ch_vict))
				continue;
			if (!HERE(ch_vict))
				continue;
			if (!ch_vict->IsNpc())
				continue;
			if (ch_vict->IsFlagged(EMobFlag::kMemory)) {
				mob_ai::mobForget(ch_vict, victim);
			}
		}

	}

	if (killer) {
		ch = killer;
	}
	die(victim, ch);
}

ObjData *GetUsedWeapon(CharData *ch, fight::AttackType AttackType) {
	ObjData *UsedWeapon = nullptr;

	if (AttackType == fight::AttackType::kMainHand) {
		if (!(UsedWeapon = GET_EQ(ch, EEquipPos::kWield)))
			UsedWeapon = GET_EQ(ch, EEquipPos::kBoths);
	} else if (AttackType == fight::AttackType::kOffHand)
		UsedWeapon = GET_EQ(ch, EEquipPos::kHold);

	return UsedWeapon;
}

void Damage::ZeroInit() {
	dam = 0;
	dam_critic = 0;
	fs_damage = 0;
	element = EElement::kUndefined;
	dmg_type = -1;
	skill_id = ESkill::kUndefined;
	spell_id = ESpell::kUndefined;
	hit_type = -1;
	msg_num = -1;
	ch_start_pos = EPosition::kUndefined;
	victim_start_pos = EPosition::kUndefined;
	wielded = nullptr;
}

/**
 * Разный инит щитов у мобов и чаров.
 * У мобов работают все 3 щита, у чаров только 1 рандомный на текущий удар.
 */
void Damage::SetPostInitShieldFlags(CharData *victim) {
	if (victim->IsNpc() && !IS_CHARMICE(victim)) {
		if (AFF_FLAGGED(victim, EAffect::kFireShield)) {
			flags.set(fight::kVictimFireShield);
		}

		if (AFF_FLAGGED(victim, EAffect::kIceShield)) {
			flags.set(fight::kVictimIceShield);
		}

		if (AFF_FLAGGED(victim, EAffect::kAirShield)) {
			flags.set(fight::kVictimAirShield);
		}
	} else {
		enum { FIRESHIELD, ICESHIELD, AIRSHIELD };
		std::vector<int> shields;

		if (AFF_FLAGGED(victim, EAffect::kFireShield)) {
			shields.push_back(FIRESHIELD);
		}

		if (AFF_FLAGGED(victim, EAffect::kAirShield)) {
			shields.push_back(AIRSHIELD);
		}

		if (AFF_FLAGGED(victim, EAffect::kIceShield)) {
			shields.push_back(ICESHIELD);
		}

		if (shields.empty()) {
			return;
		}

		int shield_num = number(0, static_cast<int>(shields.size() - 1));

		if (shields[shield_num] == FIRESHIELD) {
			flags.set(fight::kVictimFireShield);
		} else if (shields[shield_num] == AIRSHIELD) {
			flags.set(fight::kVictimAirShield);
		} else if (shields[shield_num] == ICESHIELD) {
			flags.set(fight::kVictimIceShield);
		}
	}
}

void Damage::PerformPostInit(CharData *ch, CharData *victim) {
	if (msg_num == -1) {
		// ABYRVALG тут нужно переделать на взятие сообщения из структуры абилок
		if (MUD::Skills().IsValid(skill_id)) {
			msg_num = to_underlying(skill_id) + kTypeHit;
		} else if (spell_id > ESpell::kUndefined) {
			msg_num = to_underlying(spell_id);
		} else if (hit_type >= 0) {
			msg_num = hit_type + kTypeHit;
		} else {
			msg_num = kTypeHit;
		}
	}

	if (ch_start_pos == EPosition::kUndefined) {
		ch_start_pos = ch->GetPosition();
	}

	if (victim_start_pos == EPosition::kUndefined) {
		victim_start_pos = victim->GetPosition();
	}

	SetPostInitShieldFlags(victim);
}

// обработка щитов, зб, поглощения, сообщения для огн. щита НЕ ЗДЕСЬ
// возвращает сделанный дамаг
int Damage::Process(CharData *ch, CharData *victim) {
	PerformPostInit(ch, victim);
	if (!check_valid_chars(ch, victim, __FILE__, __LINE__)) {
		return 0;
	}
	if (victim->in_room == kNowhere || ch->in_room == kNowhere || ch->in_room != victim->in_room) {
		log("SYSERR: Attempt to damage '%s' in room kNowhere by '%s'.",
			GET_NAME(victim), GET_NAME(ch));
		return 0;
	}
	if (victim->GetPosition() <= EPosition::kDead) {
		log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
			GET_NAME(victim), GET_ROOM_VNUM(victim->in_room), GET_NAME(ch));
		die(victim, nullptr);
		return 0;
	}
	if ((ch->IsNpc() && ch->IsFlagged(EMobFlag::kNoFight))
		|| (victim->IsNpc() && victim->IsFlagged(EMobFlag::kNoFight))) {
		return 0;
	}
	if (dam > 0) {
		if (IS_GOD(victim)) {
			dam = 0;
		} else if (IS_IMMORTAL(victim) || GET_GOD_FLAG(victim, EGf::kGodsLike)) {
			dam /= 4;
		} else if (GET_GOD_FLAG(victim, EGf::kGodscurse)) {
			dam *= 2;
		}
	}

	mob_ai::update_mob_memory(ch, victim);
	Appear(ch);
	Appear(victim);

	// If you attack a pet, it hates your guts
	if (!group::same_group(ch, victim)) {
		check_agro_follower(ch, victim);
	}
	if (victim != ch) {
		if (ch->GetPosition() > EPosition::kStun && (ch->GetEnemy() == nullptr)) {
			if (!pk_agro_action(ch, victim)) {
				return 0;
			}
			SetFighting(ch, victim);
			npc_groupbattle(ch);
		}
		// Start the victim fighting the attacker
		if (victim->GetPosition() > EPosition::kDead && (victim->GetEnemy() == nullptr)) {
			SetFighting(victim, ch);
			npc_groupbattle(victim);
		}

		// лошадь сбрасывает седока при уроне
		if (ch->IsOnHorse() && ch->get_horse() == victim) {
			victim->DropFromHorse();
		} else if (victim->IsOnHorse() && victim->get_horse() == ch) {
			ch->DropFromHorse();
		}
	}

	if (dam < 0 || ch->in_room == kNowhere || victim->in_room == kNowhere || ch->in_room != victim->in_room) {
		return 0;
	}

	if (ch->GetPosition() <= EPosition::kIncap) {
		return 0;
	}

	if (dam >= 2) {
		if (AFF_FLAGGED(victim, EAffect::kPrismaticAura) && !flags[fight::kIgnorePrism]) {
			if (dmg_type == fight::kPhysDmg) {
				dam *= 2;
			} else if (dmg_type == fight::kMagicDmg) {
				dam /= 2;
			}
		}
		if (AFF_FLAGGED(victim, EAffect::kSanctuary) && !flags[fight::kIgnoreSanct]) {
			if (dmg_type == fight::kPhysDmg) {
				dam /= 2;
			} else if (dmg_type == fight::kMagicDmg) {
				dam *= 2;
			}
		}

		if (victim->IsNpc() && Bonus::is_bonus_active(Bonus::EBonusType::BONUS_DAMAGE)) {
			dam *= Bonus::get_mult_bonus();
		}
	}
	//учет резистов для магического урона
	if (dmg_type == fight::kMagicDmg) {
		if (spell_id > ESpell::kUndefined) {
			dam = ApplyResist(victim, GetResistType(spell_id), dam);
		} else {
			dam = ApplyResist(victim, GetResisTypeWithElement(element), dam);
		};
	};

	// учет положения атакующего и жертвы
	// Include a damage multiplier if victim isn't ready to fight:
	// Position sitting  1.5 x normal
	// Position resting  2.0 x normal
	// Position sleeping 2.5 x normal
	// Position stunned  3.0 x normal
	// Position incap    3.5 x normal
	// Position mortally 4.0 x normal
	// Note, this is a hack because it depends on the particular
	// values of the POSITION_XXX constants.

	// физ дамага атакера из сидячего положения
	if (ch_start_pos < EPosition::kFight && dmg_type == fight::kPhysDmg) {
		dam -= dam * (EPosition::kFight - ch_start_pos) / 4;
	}

	// дамаг не увеличивается если:
	// на жертве есть воздушный щит
	// атака - каст моба (в mage_damage увеличение дамага от позиции было только у колдунов)
	if (victim_start_pos < EPosition::kFight
		&& !flags[fight::kVictimAirShield]
		&& !(dmg_type == fight::kMagicDmg
			&& ch->IsNpc())) {
		dam += dam * (EPosition::kFight - victim_start_pos) / 4;
	}

	// прочие множители

	if (AFF_FLAGGED(victim, EAffect::kHold) && dmg_type == fight::kPhysDmg) {
		if (ch->IsNpc() && !IS_CHARMICE(ch)) {
			dam = dam * 15 / 10;
		} else {
			dam = dam * 125 / 100;
		}
	}

	if (!victim->IsNpc() && IS_CHARMICE(ch)) {
		dam = dam * 8 / 10;
	}

	if (GET_PR(victim) && dmg_type == fight::kPhysDmg) {
		int ResultDam = dam - (dam * GET_PR(victim) / 100);
		ch->send_to_TC(false, true, false,
					   "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
		victim->send_to_TC(false, true, false,
						   "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
		dam = ResultDam;
	}
	if (!IS_IMMORTAL(ch) && AFF_FLAGGED(victim, EAffect::kGodsShield)) {
		if (skill_id == ESkill::kBash) {
			SendSkillMessages(dam, ch, victim, msg_num);
		}
		if (ch != victim) {
			act("Магический кокон полностью поглотил удар $N1.", false, victim, nullptr, ch, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил ваш удар.", false, ch, nullptr, victim, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил удар $n1.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		} else {
			act("Магический кокон полностью поглотил повреждения.", false, ch, nullptr, nullptr, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил повреждения.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		}
		return 0;
	}
	// щиты, броня, поглощение
	if (victim != ch) {
		bool shield_full_absorb = CalcMagisShieldsDmgAbsoption(ch, victim);
		CalcArmorDmgAbsorption(victim);
		bool armor_full_absorb = CalcDmgAbsorption(ch, victim);
		if (flags[fight::kCritHit] && (GetRealLevel(victim) >= 5 || !ch->IsNpc())
			&& !AFF_FLAGGED(victim, EAffect::kPrismaticAura)
			&& !flags[fight::kVictimIceShield]) {
			int tmpdam = std::min(victim->get_real_max_hit() / 8, dam * 2);
			tmpdam = ApplyResist(victim, EResist::kVitality, dam);
			dam = std::max(dam, tmpdam); //крит
		}
		// полное поглощение
		if (shield_full_absorb || armor_full_absorb) {
			return 0;
		}
		if (dam > 0)
			ProcessBlink(ch, victim);
	}

	// Внутри magic_shields_dam вызывается dmg::proccess, если чар там умрет, то будет креш
	if (!(ch && victim) || (ch->purged() || victim->purged())) {
		log("Death from CalcMagisShieldsDmgAbsoption");
		return 0;
	}

	if (victim->IsFlagged(EMobFlag::kProtect)) {
		if (victim != ch) {
			act("$n находится под защитой Богов.", false, victim, nullptr, nullptr, kToRoom);
		}
		return 0;
	}
	// внутри есть !боевое везение!, для какого типа дамага - не знаю
	DamageActorParameters params(ch, victim, dam);
	handle_affects(params);
	dam = params.damage;
	DamageVictimParameters params1(ch, victim, dam);
	handle_affects(params1);
	dam = params1.damage;

	// обратка от зеркал/огненного щита
	if (flags[fight::kMagicReflect]) {
		// ограничение для зеркал на 40% от макс хп кастера
		dam = std::min(dam, victim->get_max_hit() * 4 / 10);
		// чтобы не убивало обраткой
		dam = std::min(dam, victim->get_hit() - 1);
	}

	dam = std::clamp(dam, 0, kMaxHits);
	if (dam >= 0) {
		if (dmg_type == fight::kPhysDmg) {
			if (!damage_mtrigger(ch, victim, dam, MUD::Skill(skill_id).GetName(), 1, wielded))
				return 0;
		} else if (dmg_type == fight::kMagicDmg) {
			if (!damage_mtrigger(ch, victim, dam, MUD::Spell(spell_id).GetCName(), 0, wielded))
				return 0;
		} else if (dmg_type == fight::kPoisonDmg) {
			if (!damage_mtrigger(ch, victim, dam, MUD::Spell(spell_id).GetCName(), 2, wielded))
				return 0;
		}
	}
	gain_battle_exp(ch, victim, dam);

	// real_dam так же идет в обратку от огн.щита
	int real_dam = dam;
	int over_dam = 0;

	if (dam > victim->get_hit() + 11) {
		real_dam = victim->get_hit() + 11;
		over_dam = dam - real_dam;
	}
	// собственно нанесение дамага
	victim->set_hit(victim->get_hit() - dam);
	victim->send_to_TC(false, true, true, "&MПолучен урон = %d&n\r\n", dam);
	ch->send_to_TC(false, true, true, "&MПрименен урон = %d&n\r\n", dam);
	if (dmg_type == fight::kPhysDmg && GET_GOD_FLAG(ch, EGf::kSkillTester) && skill_id != ESkill::kUndefined) {
		log("SKILLTEST:;%s;skill;%s;damage;%d;Luck;%s", GET_NAME(ch), MUD::Skill(skill_id).GetName(), dam, flags[fight::kCritLuck] ? "yes" : "no");
	}
	// если на чармисе вампир
	if (AFF_FLAGGED(ch, EAffect::kVampirism)) {
		ch->set_hit(std::clamp(ch->get_hit() + std::max(1, dam / 10),
								 ch->get_hit(), ch->get_real_max_hit() + ch->get_real_max_hit() * GetRealLevel(ch) / 10));
		// если есть родство душ, то чару отходит по 5% от дамаги к хп
		if (ch->has_master()) {
			if (CanUseFeat(ch->get_master(), EFeat::kSoulLink)) {
				ch->get_master()->set_hit(std::max(ch->get_master()->get_hit(),
					std::min(ch->get_master()->get_hit() + std::max(1, dam / 20 ),
						ch->get_master()->get_real_max_hit() +
						ch->get_master()->get_real_max_hit() *
						GetRealLevel(ch->get_master()) / 10)));
			}
		}
	}
	// запись в дметр фактического и овер дамага
	UpdateDpsStatistics(ch, real_dam, over_dam);
	// запись дамага в список атакеров
	if (victim->IsNpc()) {
		victim->add_attacker(ch, ATTACKER_DAMAGE, real_dam);
	}
	// попытка спасти жертву через ангела
	CheckTutelarSelfSacrfice(ch, victim);

	// обновление позиции после удара и ангела
	update_pos(victim);
	// если вдруг виктим сдох после этого, то произойдет креш, поэтому вставил тут проверочку
	if (!(ch && victim) || (ch->purged() || victim->purged())) {
		log("Error in fight_hit, function process()\r\n");
		return 0;
	}
	// если у чара есть жатва жизни
	if (CanUseFeat(victim, EFeat::kHarvestOfLife)) {
		if (victim->GetPosition() == EPosition::kDead) {
			int souls = victim->get_souls();
			if (souls >= 10) {
				victim->set_hit(0 + souls * 10);
				update_pos(victim);
				SendMsgToChar("&GДуши спасли вас от смерти!&n\r\n", victim);
				victim->set_souls(0);
			}
		}
	}
	// сбивание надува черноков //
	if (spell_id != ESpell::kPoison && dam > 0 && !flags[fight::kMagicReflect]) {
		TryRemoveExtrahits(ch, victim);
	}

	if (dam && flags[fight::kCritHit] && !dam_critic && spell_id != ESpell::kPoison) {
		SendCritHitMsg(ch, victim);
	}

	// инит Damage::brief_shields_
	if (flags.test(fight::kDrawBriefFireShield)
		|| flags.test(fight::kDrawBriefAirShield)
		|| flags.test(fight::kDrawBriefIceShield)
		|| flags.test(fight::kDrawBriefMagMirror)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "&n (%s%s%s%s)",
				 flags.test(fight::kDrawBriefFireShield) ? "&R*&n" : "",
				 flags.test(fight::kDrawBriefAirShield) ? "&C*&n" : "",
				 flags.test(fight::kDrawBriefIceShield) ? "&W*&n" : "",
				 flags.test(fight::kDrawBriefMagMirror) ? "&M*&n" : "");
		brief_shields_ = buf_;
	}
	// сообщения об ударах //
	if (MUD::Skills().IsValid(skill_id) || spell_id > ESpell::kUndefined || hit_type < 0) {
		// скилл, спелл, необычный дамаг
		SendSkillMessages(dam, ch, victim, msg_num, brief_shields_);
	} else {
		// простой удар рукой/оружием
		if (victim->GetPosition() == EPosition::kDead || dam == 0) {
			if (!SendSkillMessages(dam, ch, victim, msg_num, brief_shields_)) {
				SendDmgMsg(ch, victim);
			}
		} else {
			SendDmgMsg(ch, victim);
		}
	}
	/// Use SendMsgToChar -- act() doesn't send message if you are DEAD.
	char_dam_message(dam, ch, victim, flags[fight::kNoFleeDmg]);


	// Проверить, что жертва все еще тут. Может уже сбежала по трусости.
	// Думаю, простой проверки достаточно.
	// Примечание, если сбежал в FIRESHIELD,
	// то обратного повреждения по атакующему не будет
	if (ch->in_room != victim->in_room) {
		return dam;
	}

	// Stop someone from fighting if they're stunned or worse
	/*if ((victim->GetPosition() <= EPosition::kStun)
		&& (victim->GetEnemy() != NULL))
	{
		stop_fighting(victim, victim->GetPosition() <= EPosition::kDead);
	} */
	utils::CSteppedProfiler round_profiler(fmt::format("process death"), 0.01);
	round_profiler.next_step("Start");

	// жертва умирает //
	if (victim->GetPosition() == EPosition::kDead) {
		ProcessDeath(ch, victim);
		return -1;
	}
	// обратка от огненного щита
	if (fs_damage > 0
		&& victim->GetEnemy()
		&& victim->GetPosition() > EPosition::kStun
		&& victim->in_room != kNowhere) {
		Damage dmg(SpellDmg(ESpell::kFireShield), fs_damage, fight::kUndefDmg);
		dmg.flags.set(fight::kNoFleeDmg);
		dmg.flags.set(fight::kMagicReflect);
		dmg.Process(victim, ch);
	}
	return dam;
}

int HitData::ProcessExtradamage(CharData *ch, CharData *victim) {
	if (!check_valid_chars(ch, victim, __FILE__, __LINE__)) {
		return 0;
	}

	const int mem_dam = dam;
	if (dam < 0) {
		dam = 0;
	}

	if (ch->battle_affects.get(kEafHammer) && ch->get_wait() <= 0) {
		//* богатырский молот //
		// в эти условия ничего добавлять не надо, иначе kEafHammer не снимется
		// с моба по ходу боя, если он не может по каким-то причинам смолотить
		ProcessMighthit(ch, victim, *this);
	} else if (ch->battle_affects.get(kEafOverwhelm) && ch->get_wait() <= 0) {
		//* оглушить  аналогично молоту, все доп условия добавляются внутри
		ProcessOverhelm(ch, victim, *this);
	}
		//* яды со скила отравить //
	else if (!victim->IsFlagged(EMobFlag::kProtect)
		&& dam
		&& wielded
		&& wielded->has_timed_spell()
		&& ch->GetSkill(ESkill::kPoisoning)) {
		TryPoisonWithWeapom(ch, victim, wielded->timed_spell().IsSpellPoisoned());
	}
		//* травящий ядом моб //
	else if (dam
		&& ch->IsNpc()
		&& NPC_FLAGGED(ch, ENpcFlag::kToxic)
		&& !AFF_FLAGGED(ch, EAffect::kCharmed)
		&& ch->get_wait() <= 0
		&& !AFF_FLAGGED(victim, EAffect::kPoisoned)
		&& number(0, 100) < GET_LIKES(ch) + GetRealLevel(ch) - GetRealLevel(victim)
		&& !CalcGeneralSaving(ch, victim, ESaving::kCritical, -GetRealCon(victim))) {
		poison_victim(ch, victim, std::max(1, GetRealLevel(ch) - GetRealLevel(victim)) * 10);
	}
		//* точный стиль //
	else if (dam && GetFlags()[fight::kCritHit] && dam_critic) {
		auto punctual_result = ProcessPunctualHit(ch, victim, dam, dam_critic);
		dam = punctual_result.dmg;
		dam_critic = punctual_result.dmg_critical;
		SetFlag(fight::kIgnoreBlink);

	}

	// Если удар парирован, необходимо все равно ввязаться в драку.
	// Вызывается damage с отрицательным уроном
	dam = mem_dam >= 0 ? dam : -1;

	Damage dmg(SkillDmg(skill_num), dam, fight::kPhysDmg, wielded);
	dmg.hit_type = hit_type;
	dmg.dam_critic = dam_critic;
	dmg.flags = GetFlags();
	dmg.ch_start_pos = ch_start_pos;
	dmg.victim_start_pos = victim_start_pos;

	return dmg.Process(ch, victim);
}

/**
 * Инициализация всех нужных первичных полей (отделены в хедере), после
 * этой функции уже начинаются подсчеты собсна хитролов/дамролов и прочего.
 */
void HitData::Init(CharData *ch, CharData *victim) {
	// Find weapon for attack number weapon //
	if (weapon == fight::AttackType::kMainHand) {
		if (!(wielded = GET_EQ(ch, EEquipPos::kWield))) {
			wielded = GET_EQ(ch, EEquipPos::kBoths);
			weapon_pos = EEquipPos::kBoths;
		}
	} else if (weapon == fight::AttackType::kOffHand) {
		wielded = GET_EQ(ch, EEquipPos::kHold);
		weapon_pos = EEquipPos::kHold;
		if (!wielded) { // удар второй рукой
			weap_skill = ESkill::kLeftHit;
// зачем вычисления и спам в лог если ниже переопределяется
//			weap_skill_is = CalcCurrentSkill(ch, weap_skill, victim);
			TrainSkill(ch, weap_skill, true, victim);
		}
	}

	if (wielded
		&& wielded->get_type() == EObjType::kWeapon) {
		// для всех типов атак скилл берется из пушки, если она есть
		weap_skill = static_cast<ESkill>(wielded->get_spec_param());
	} else {
		// удар голыми руками
		weap_skill = ESkill::kPunch;
	}
	if (skill_num == ESkill::kUndefined) {
		TrainSkill(ch, weap_skill, true, victim);
		SkillRollResult result = MakeSkillTest(ch, weap_skill, victim);
		weap_skill_is = result.SkillRate;
		if (result.CritLuck) {
			SendMsgToChar(ch, "Вы удачно поразили %s в уязвимое место.\r\n", victim->player_data.PNames[ECase::kAcc].c_str());
			act("$n поразил$g вас в уязвимое место.", true, ch, nullptr, victim, kToVict);
			act("$n поразил$g $N3 в уязвимое место.", true, ch, nullptr, victim, kToNotVict);
			SetFlag(fight::kCritLuck);
			SetFlag(fight::kIgnoreSanct);
			SetFlag(fight::kHalfIgnoreArmor);
			SetFlag(fight::kIgnoreAbsorbe);
		}
	}
	//* обработка ESkill::kNoParryHit //
	if (skill_num == ESkill::kUndefined && ch->GetSkill(ESkill::kNoParryHit)) {
		int tmp_skill = CalcCurrentSkill(ch, ESkill::kNoParryHit, victim);
		bool success = tmp_skill >= number(1, MUD::Skill(ESkill::kNoParryHit).difficulty);
		TrainSkill(ch, ESkill::kNoParryHit, success, victim);
		if (success) {
			hit_no_parry = true;
		}
	}

	if (ch->battle_affects.get(kEafOverwhelm) || ch->battle_affects.get(kEafHammer)) {
		hit_no_parry = true;
	}

	if (wielded && wielded->get_type() == EObjType::kWeapon) {
		hit_type = GET_OBJ_VAL(wielded, 3);
	} else {
		weapon_pos = 0;
		if (ch->IsNpc()) {
			hit_type = ch->mob_specials.attack_type;
		}
	}

	// позиции сражающихся до применения скилов и прочего, что может их изменить
	ch_start_pos = ch->GetPosition();
	victim_start_pos = victim->GetPosition();
}

/**
 * Подсчет статичных хитролов у чара, не меняющихся от рандома типа train_skill
 * (в том числе weap_skill_is) или параметров противника.
 * Предполагается, что в итоге это пойдет в 'счет все' через что-то вроде
 * TestSelfHitroll() в данный момент.
 */
void HitData::CalcBaseHitroll(CharData *ch) {
	if (skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab) {
		if (wielded
			&& wielded->get_type() == EObjType::kWeapon
			&& !ch->IsNpc()) {
			// Apply HR for light weapon
			int percent = 0;
			switch (weapon_pos) {
				case EEquipPos::kWield: percent = (str_bonus(GetRealStr(ch), STR_WIELD_W) - wielded->get_weight() + 1) / 2;
					break;
				case EEquipPos::kHold: percent = (str_bonus(GetRealStr(ch), STR_HOLD_W) - wielded->get_weight() + 1) / 2;
					break;
				case EEquipPos::kBoths:
					percent = (str_bonus(GetRealStr(ch), STR_WIELD_W) +
						str_bonus(GetRealStr(ch), STR_HOLD_W) - wielded->get_weight() + 1) / 2;
					break;
			}
			calc_thaco -= std::min(3, std::max(percent, 0));
		} else if (!ch->IsNpc()) {
			// кулаками у нас полагается бить только богатырям :)
			if (!CanUseFeat(ch, EFeat::kBully))
				calc_thaco += 4;
			else    // а богатырям положен бонус за отсутствие оружия
				calc_thaco -= 3;
		}
	}

	CheckWeapFeats(ch, weap_skill, calc_thaco, dam);

	if (ch->battle_affects.get(kEafOverwhelm) || ch->battle_affects.get(kEafHammer)) {
		calc_thaco -= std::max(0, (ch->GetSkill(weap_skill) - 70) / 8);
	}

	//    AWAKE style - decrease hitroll
	if (ch->battle_affects.get(kEafAwake)
		&& !CanUseFeat(ch, EFeat::kShadowStrike)
		&& skill_num != ESkill::kThrow
		&& skill_num != ESkill::kBackstab) {
		if (CanPerformAutoblock(ch)) {
			// осторожка со щитом в руках у дружа с блоком - штрафы на хитролы (от 0 до 10)
			calc_thaco += ch->GetSkill(ESkill::kAwake) * 5 / 100;
		} else {
			// здесь еще были штрафы на дамаг через деление, но положительного дамага
			// на этом этапе еще нет, так что делили по сути нули
			calc_thaco += ((ch->GetSkill(ESkill::kAwake) + 9) / 10) + 2;
		}
	}

	if (!ch->IsNpc() && skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab) {
		// Casters use weather, int and wisdom
		if (IsCaster(ch)) {
			/*	  calc_thaco +=
				    (10 -
				     complex_skill_modifier (ch, kAny, GAPPLY_ESkill::SKILL_SUCCESS,
							     10));
			*/
			calc_thaco -= (int) ((GetRealInt(ch) - 13) / GetRealLevel(ch));
			calc_thaco -= (int) ((GetRealWis(ch) - 13) / GetRealLevel(ch));
		}
	}

	if (AFF_FLAGGED(ch, EAffect::kBless)) {
		calc_thaco -= 4;
	}
	if (AFF_FLAGGED(ch, EAffect::kCurse)) {
		calc_thaco += 6;
		dam -= 5;
	}

	// Учет мощной и прицельной атаки
	if (ch->IsFlagged(EPrf::kPerformPowerAttack) && CanUseFeat(ch, EFeat::kPowerAttack)) {
		calc_thaco += 2;
	} else if (ch->IsFlagged(EPrf::kPerformGreatPowerAttack) && CanUseFeat(ch, EFeat::kGreatPowerAttack)) {
		calc_thaco += 4;
	} else if (ch->IsFlagged(EPrf::kPerformAimingAttack) && CanUseFeat(ch, EFeat::kAimingAttack)) {
		calc_thaco -= 2;
	} else if (ch->IsFlagged(EPrf::kPerformGreatAimingAttack) && CanUseFeat(ch, EFeat::kGreatAimingAttack)) {
		calc_thaco -= 4;
	}

	// Calculate the THAC0 of the attacker
	if (!ch->IsNpc()) {
		calc_thaco += GetThac0(ch->GetClass(), GetRealLevel(ch));
	} else {
		// штраф мобам по рекомендации Триглава
		calc_thaco += (25 - GetRealLevel(ch) / 3);
	}

	//Вычисляем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);

	calc_thaco -= static_cast<int>(GET_REAL_HR(ch) * p_hitroll);
	calc_thaco -= static_cast<int>(str_bonus(GetRealStr(ch), STR_TO_HIT) * p_hitroll);
	if ((skill_num == ESkill::kThrow
		|| skill_num == ESkill::kBackstab)
		&& wielded
		&& wielded->get_type() == EObjType::kWeapon) {
		if (skill_num == ESkill::kBackstab) {
			calc_thaco -= std::max(0, (ch->GetSkill(ESkill::kSneak) + ch->GetSkill(ESkill::kHide) - 100) / 30);
		}
	} else {
		calc_thaco += 4;
	}

	if (IsAffectedBySpell(ch, ESpell::kBerserk)) {
		if (AFF_FLAGGED(ch, EAffect::kBerserk)) {
			calc_thaco -= (12 * ((ch->get_real_max_hit() / 2) - ch->get_hit()) / ch->get_real_max_hit());
		}
	}

}

/**
 * Соответственно подсчет динамических хитролов, не попавших в calc_base_hr()
 * Все, что меняется от раза к разу или при разных противниках
 * При любом изменении содержимого нужно поддерживать адекватность CalcStaticHitroll()
 */
void HitData::CalcCircumstantialHitroll(CharData *ch, CharData *victim) {
	//считаем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);
	// штраф в размере 1 хитролла за каждые
	// недокачанные 10% скилла "удар левой рукой"

	if (weapon == fight::AttackType::kOffHand
		&& skill_num != ESkill::kThrow
		&& skill_num != ESkill::kBackstab
		&& !(wielded && wielded->get_type() == EObjType::kWeapon)
		&& !ch->IsNpc()) {
			calc_thaco += std::max(0, (CalcSkillMinCap(victim, ESkill::kLeftHit) - CalcCurrentSkill(ch, ESkill::kLeftHit, victim)) / 10);
	}

	// courage
	if (IsAffectedBySpell(ch, ESpell::kCourage)) {
		int range = number(1, MUD::Skill(ESkill::kCourage).difficulty + ch->get_real_max_hit() - ch->get_hit());
		int prob = CalcCurrentSkill(ch, ESkill::kCourage, victim);
		TrainSkill(ch, ESkill::kCourage, prob > range, victim);
		if (prob > range) {
			dam += ((ch->GetSkill(ESkill::kCourage) + 19) / 20);
			calc_thaco -= static_cast<int>(((ch->GetSkill(ESkill::kCourage) + 9.0) / 20.0) * p_hitroll);
		}
	}

	// Horse modifier for attacker
	if (!ch->IsNpc() && skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab && ch->IsOnHorse()) {
		TrainSkill(ch, ESkill::kRiding, true, victim);
		calc_thaco += 10 - ch->GetSkill(ESkill::kRiding) / 20;
	}

	// not can see (blind, dark, etc)
	if (!CAN_SEE(ch, victim))
		calc_thaco += (CanUseFeat(ch, EFeat::kBlindFight) ? 2 : ch->IsNpc() ? 6 : 10);
	if (!CAN_SEE(victim, ch))
		calc_thaco -= (CanUseFeat(victim, EFeat::kBlindFight) ? 2 : 8);

	// "Dirty" methods for battle
	if (skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab) {
		int prob = (ch->GetSkill(weap_skill) + cha_app[GetRealCha(ch)].illusive) -
			(victim->GetSkill(weap_skill) + int_app[GetRealInt(victim)].observation);
		if (prob >= 30 && !victim->battle_affects.get(kEafAwake)
			&& (ch->IsNpc() || !ch->battle_affects.get(kEafPunctual))) {
			calc_thaco -= static_cast<int>((ch->GetSkill(weap_skill) -
				victim->GetSkill(weap_skill) > 60 ? 2 : 1) * p_hitroll);
			if (!victim->IsNpc())
				dam += (prob >= 70 ? 3 : (prob >= 50 ? 2 : 1));
		}
	}

	// AWAKE style for victim
	if (victim->battle_affects.get(kEafAwake)
		&& !AFF_FLAGGED(victim, EAffect::kStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kMagicStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kHold)) {
		bool success = CalcCurrentSkill(ch, ESkill::kAwake, victim)
			>= number(1, MUD::Skill(ESkill::kAwake).difficulty);
		if (success) {
			// > и зачем так? кто балансом занимается поправте.
			// воткнул мин. разницу, чтобы анализаторы не ругались
			dam -= ch->IsNpc() ? 5 : 4;
			calc_thaco += ch->IsNpc() ? 4 : 2;
		}
		TrainSkill(victim, ESkill::kAwake, success, ch);
	}

	// скилл владения пушкой или голыми руками
	if (weap_skill_is <= 80)
		calc_thaco -= static_cast<int>((weap_skill_is / 20.0) * p_hitroll);
	else if (weap_skill_is <= 160)
		calc_thaco -= static_cast<int>((4.0 + (weap_skill_is - 80.0) / 10.0) * p_hitroll);
	else
		calc_thaco -= static_cast<int>((12.0 + (weap_skill_is - 160.0) / 5.0) * p_hitroll);
}

// * Версия calc_rand_hr для показа по 'счет', без рандомов и статов жертвы.
void HitData::CalcStaticHitroll(CharData *ch) {
	//считаем штраф за голод
	float p_hitroll = ch->get_cond_penalty(P_HITROLL);
	// штраф в размере 1 хитролла за каждые
	// недокачанные 10% скилла "удар левой рукой"
	if (weapon == fight::AttackType::kOffHand
		&& skill_num != ESkill::kThrow
		&& skill_num != ESkill::kBackstab
		&& !(wielded && wielded->get_type() == EObjType::kWeapon)
		&& !ch->IsNpc()) {
		calc_thaco += (MUD::Skill(ESkill::kLeftHit).difficulty - ch->GetSkill(ESkill::kLeftHit)) / 10;
	}

	// courage
	if (IsAffectedBySpell(ch, ESpell::kCourage)) {
		dam += ((ch->GetSkill(ESkill::kCourage) + 19) / 20);
		calc_thaco -= static_cast<int>(((ch->GetSkill(ESkill::kCourage) + 9.0) / 20.0) * p_hitroll);
	}

	// Horse modifier for attacker
	if (!ch->IsNpc() && skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab && ch->IsOnHorse()) {
		int prob = ch->GetSkill(ESkill::kRiding);
		int range = MUD::Skill(ESkill::kRiding).difficulty / 2;

		dam += ((prob + 19) / 10);

		if (range > prob)
			calc_thaco += ((range - prob) + 19 / 20);
		else
			calc_thaco -= ((prob - range) + 19 / 20);
	}

	// скилл владения пушкой или голыми руками
	if (ch->GetSkill(weap_skill) <= 80)
		calc_thaco -= static_cast<int>((ch->GetSkill(weap_skill) / 20.0) * p_hitroll);
	else if (ch->GetSkill(weap_skill) <= 160)
		calc_thaco -= static_cast<int>((4.0 + (ch->GetSkill(weap_skill) - 80.0) / 10.0) * p_hitroll);
	else
		calc_thaco -= static_cast<int>((12.0 + (ch->GetSkill(weap_skill) - 160.0) / 5.0) * p_hitroll);
}

// * Подсчет армор класса жертвы.
void HitData::CalcAc(CharData *victim) {
	// Calculate the raw armor including magic armor.  Lower AC is better.
	victim_ac += CalcAC(victim);
	victim_ac /= 10;
	//считаем штраф за голод
	if (!victim->IsNpc() && victim_ac < 5) { //для голодных
		int p_ac = static_cast<int>((1 - victim->get_cond_penalty(P_AC)) * 40);
		if (p_ac) {
			if (victim_ac + p_ac > 5) {
				victim_ac = 5;
			} else {
				victim_ac += p_ac;
			}
		}
	}

	if (victim->GetPosition() < EPosition::kFight)
		victim_ac += 4;
	if (victim->GetPosition() < EPosition::kRest)
		victim_ac += 3;
	if (AFF_FLAGGED(victim, EAffect::kHold))
		victim_ac += 4;
	if (AFF_FLAGGED(victim, EAffect::kCrying))
		victim_ac += 4;
}

void HitData::ProcessDefensiveAbilities(CharData *ch, CharData *victim) {
	ProcessIntercept(ch, *this);
	ProcessDeviate(ch, victim, *this);
	ProcessParry(ch, victim, *this);
	ProcessMultyparry(ch, victim, *this);
	ProcessShieldBlock(ch, victim, *this);
}

/**
 * В данный момент:
 * добавление дамролов с пушек
 * добавление дамага от концентрации силы
 */
void HitData::AddWeaponDmg(CharData *ch, bool need_dice) {
	int damroll;
 	if (need_dice) {
		if (GetFlags()[fight::kCritLuck]) {
			damroll = GET_OBJ_VAL(wielded, 1) *  GET_OBJ_VAL(wielded, 2);
		} else {
			damroll = RollDices(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
		}
	} else {
		damroll = (GET_OBJ_VAL(wielded, 1) * GET_OBJ_VAL(wielded, 2) + GET_OBJ_VAL(wielded, 1)) / 2;
	}
	if (ch->IsNpc()
		&& !AFF_FLAGGED(ch, EAffect::kCharmed)
		&& !(ch->IsFlagged(EMobFlag::kTutelar) || ch->IsFlagged(EMobFlag::kMentalShadow))) {
		damroll *= kMobDamageMult;
	} else {
		damroll = std::min(damroll, damroll * wielded->get_current_durability() / std::max(1, wielded->get_maximum_durability()));
	}

	damroll = CalcStrCondensationDamage(ch, wielded, damroll);
	dam += std::max(1, damroll);
}

void HitData::AddBareHandsDmg(CharData *ch, bool need_dice) {

	if (AFF_FLAGGED(ch, EAffect::kStoneHands)) {
		if (GetFlags()[fight::kCritLuck]) {
			dam += 8 + GetRealRemort(ch) / 2;
		} else {
			dam += need_dice ? RollDices(2, 4) + GetRealRemort(ch) / 2  : 5 + GetRealRemort(ch) / 2;
		}
		if (CanUseFeat(ch, EFeat::kBully)) {
			dam += GetRealLevel(ch) / 5;
			dam += std::max(0, GetRealStr(ch) - 25);
		}
	} else {
		if (GetFlags()[fight::kCritLuck]) {
			dam  += 3;
		} else {
			dam += need_dice? number(1, 3) : 2;
		}
	}
	// Мультипликатор повреждений без оружия и в перчатках (линейная интерполяция)
	// <вес перчаток> <увеличение>
	// 0  50%
	// 5 100%
	// 10 150%
	// 15 200%
	// НА МОЛОТ НЕ ВЛИЯЕТ
	if (!ch->battle_affects.get(kEafHammer)
		|| GetFlags()[fight::kCritHit]) //в метком молоте идет учет перчаток
	{
		int modi = 10 * (5 + (GET_EQ(ch, EEquipPos::kHands) ? std::min(GET_EQ(ch, EEquipPos::kHands)->get_weight(), 18)
															: 0)); //вес перчаток больше 18 не учитывается
		if (ch->IsNpc() || CanUseFeat(ch, EFeat::kBully)) {
			modi = std::max(100, modi);
		}
		dam = modi * dam / 100;
	}
}

// * Расчет шанса на критический удар (не точкой).
void HitData::CalcCritHitChance(CharData *ch) {
	dam_critic = 0;
	int calc_critic = 0;

	// Маги, волхвы и чармисы не умеют критать //
	if ((!ch->IsNpc() && !IsMage(ch) && !IS_MAGUS(ch))
			|| (ch->IsNpc() && (!AFF_FLAGGED(ch, EAffect::kCharmed)
			&& !AFF_FLAGGED(ch, EAffect::kHelper)))) {
		calc_critic = std::min(ch->GetSkill(weap_skill), 70);
		if (CanUseFeat(ch, FindWeaponMasterFeat(weap_skill))) {
			calc_critic += std::max(0, ch->GetSkill(weap_skill) - 70);
		}
		if (CanUseFeat(ch, EFeat::kThieveStrike)) {
			calc_critic += ch->GetSkill(ESkill::kBackstab);
		}
		if (!ch->IsNpc()) {
			calc_critic += static_cast<int>(ch->GetSkill(ESkill::kPunctual) / 2);
			calc_critic += static_cast<int>(ch->GetSkill(ESkill::kNoParryHit) / 3);
		}
		calc_critic += GetRealLevel(ch) + GetRealRemort(ch);
	}
	if (number(0, 2000) < calc_critic) {
		SetFlag(fight::kCritHit);
	} else {
		ResetFlag(fight::kCritHit);
	}
}
int HitData::CalcDmg(CharData *ch, bool need_dice) {
	if (ch->IsFlagged(EPrf::kExecutor)) {
		SendMsgToChar(ch, "&YСкилл: %s. Дамага без бонусов == %d&n\r\n", MUD::Skill(weap_skill).GetName(), dam);
	}
	if (ch->GetSkill(weap_skill) == 0) {
		calc_thaco += (50 - std::min(50, GetRealInt(ch))) / 3;
		dam -= (50 - std::min(50, GetRealInt(ch))) / 6;
	} else {
		GetClassWeaponMod(ch->GetClass(), weap_skill, &dam, &calc_thaco);
	}
	if (ch->GetSkill(weap_skill) >= 60) { //от уровня скилла
		dam += ((ch->GetSkill(weap_skill) - 50) / 10);
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага с уровнем скилла == %d&n\r\n", dam);
	}
	// Учет мощной и прицельной атаки
	if (ch->IsFlagged(EPrf::kPerformPowerAttack) && CanUseFeat(ch, EFeat::kPowerAttack)) {
		dam += 5;
	} else if (ch->IsFlagged(EPrf::kPerformGreatPowerAttack) && CanUseFeat(ch, EFeat::kGreatPowerAttack)) {
		dam += 10;
	} else if (ch->IsFlagged(EPrf::kPerformAimingAttack) && CanUseFeat(ch, EFeat::kAimingAttack)) {
		dam -= 5;
	} else if (ch->IsFlagged(EPrf::kPerformGreatAimingAttack) && CanUseFeat(ch, EFeat::kGreatAimingAttack)) {
		dam -= 10;
	}
	if (ch->IsFlagged(EPrf::kExecutor))
		SendMsgToChar(ch, "&YДамага с учетом перков мощная-улучш == %d&n\r\n", dam);
	// courage
	if (IsAffectedBySpell(ch, ESpell::kCourage)) {
		int range = number(1, MUD::Skill(ESkill::kCourage).difficulty + ch->get_real_max_hit() - ch->get_hit());
		int prob = CalcCurrentSkill(ch, ESkill::kCourage, ch);
		if (prob > range) {
			dam += ((ch->GetSkill(ESkill::kCourage) + 19) / 20);
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага с бухлом == %d&n\r\n", dam);
		}
	}
/*	// Horse modifier for attacker
	if (!ch->IsNpc() && skill_num != ESkill::kThrow && skill_num != ESkill::kBackstab && ch->IsOnHorse()) {
		int prob = ch->get_skill(ESkill::kRiding);
		dam += ((prob + 19) / 10);
		SendMsgToChar(ch, "&YДамага с учетом лошади == %d&n\r\n", dam);
	}
*/
	// обработка по факту попадания
	if (skill_num < ESkill::kFirst) {
		dam += GetAutoattackDamroll(ch, ch->GetSkill(weap_skill));
	if (ch->IsFlagged(EPrf::kExecutor))
		SendMsgToChar(ch, "&YДамага +дамролы автоатаки == %d&n\r\n", dam);
	} else {
		dam += GetRealDamroll(ch);
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага +дамролы скилла== %d&n\r\n", dam);
	}
	if (CanUseFeat(ch, EFeat::kFInesseShot) || CanUseFeat(ch, EFeat::kWeaponFinesse)) {
		dam += str_bonus(GetRealDex(ch), STR_TO_DAM);
	} else {
		dam += str_bonus(GetRealStr(ch), STR_TO_DAM);
	}
	if (ch->IsFlagged(EPrf::kExecutor))
		SendMsgToChar(ch, "&YДамага с бонусами от силы или ловкости == %d str_bonus == %d str == %d&n\r\n", dam, str_bonus(
						  GetRealStr(ch), STR_TO_DAM),
					  GetRealStr(ch));
	// оружие/руки и модификаторы урона скилов, с ними связанных
	if (wielded && wielded->get_type() == EObjType::kWeapon) {
		AddWeaponDmg(ch, need_dice);
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага +кубики оружия дамага == %d вооружен %s vnum %d&n\r\n", dam,
						  wielded->get_PName(ECase::kGen).c_str(), GET_OBJ_VNUM(wielded));
		if (GET_EQ(ch, EEquipPos::kBoths) && weap_skill != ESkill::kBows) { //двуруч множим на 2
			dam *= 2;
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага двуручем множим на 2 == %d&n\r\n", dam);
		}
		// скрытый удар
		int tmp_dam = CalcNoparryhitDmg(ch, wielded);
		if (tmp_dam > 0) {
			// 0 раунд и стаб = 70% скрытого, дальше раунд * 0.4 (до 5 раунда)
			int round_dam = tmp_dam * 7 / 10;
			if (CanUseFeat(ch, EFeat::kSnakeRage)) {
				if (ch->round_counter >= 1 && ch->round_counter <= 3) {
					dam *= ch->round_counter;
				}
			}
			if (skill_num == ESkill::kBackstab || ch->round_counter <= 0) {
				dam += round_dam;
			} else {
				dam += round_dam*std::min(3, ch->round_counter);
			}
			if (ch->IsFlagged(EPrf::kExecutor))
				SendMsgToChar(ch, "&YДамага от скрытого удара == %d&n\r\n", dam);
		}
	} else {
		AddBareHandsDmg(ch, need_dice);
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага руками == %d&n\r\n", dam);
	}
	if (ch->IsFlagged(EPrf::kExecutor))
		SendMsgToChar(ch, "&YДамага после расчета руки или оружия == %d&n\r\n", dam);

	if (ch->battle_affects.get(kEafIronWind)) {
		dam += ch->GetSkill(ESkill::kIronwind) / 2;
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага после расчета железного ветра == %d&n\r\n", dam);
	}

	if (IsAffectedBySpell(ch, ESpell::kBerserk)) {
		if (AFF_FLAGGED(ch, EAffect::kBerserk)) {
			dam = (dam*std::max(150, 150 + GetRealLevel(ch) +
				RollDices(0, GetRealRemort(ch)) * 2)) / 100;
			if (ch->IsFlagged(EPrf::kExecutor))
				SendMsgToChar(ch, "&YДамага с учетом берсерка== %d&n\r\n", dam);
		}
	}
	if (ch->IsNpc()) { // урон моба из олц
		dam += RollDices(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
	}

	if (ch->GetSkill(ESkill::kRiding) > 100 && ch->IsOnHorse()) {
		dam *= static_cast<int>(1.0 + (ch->GetSkill(ESkill::kRiding) - 100) / 500.0);
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага с учетом лошади (при скилле 200 +20 процентов)== %d&n\r\n", dam);
	}

	if (ch->add_abils.percent_physdam_add > 0) {
		int tmp;
		if (need_dice) {
			tmp = dam * (number(1, ch->add_abils.percent_physdam_add * 2) / 100.0); 
			dam += tmp;
		} else {
			tmp = dam * (ch->add_abils.percent_physdam_add / 100.0);
			dam += tmp;
		}
		if (ch->IsFlagged(EPrf::kExecutor))
			SendMsgToChar(ch, "&YДамага c + процентами дамаги== %d, добавили = %d процентов &n\r\n", dam, tmp);
	}
	//режем дамаг от голода
	dam *= ch->get_cond_penalty(P_DAMROLL);
	if (ch->IsFlagged(EPrf::kExecutor))
		SendMsgToChar(ch, "&YДамага с бонусами итого == %d&n\r\n", dam);
	return dam;

}

ESpell GetSpellIdByBreathflag(CharData *ch) {
	if (ch->IsFlagged((EMobFlag::kFireBreath))) {
		return ESpell::kFireBreath;
	} else if (ch->IsFlagged((EMobFlag::kGasBreath))) {
		return ESpell::kGasBreath;
	} else if (ch->IsFlagged((EMobFlag::kFrostBreath))) {
		return ESpell::kFrostBreath;
	} else if (ch->IsFlagged((EMobFlag::kAcidBreath))) {
		return ESpell::kAcidBreath;
	} else if (ch->IsFlagged((EMobFlag::kLightingBreath))) {
		return ESpell::kLightingBreath;
	}

	return ESpell::kUndefined;
}

/**
* обработка ударов оружием, санка, призма, стили, итд.
*/
void hit(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon) {
	if (!victim) {
		return;
	}
	if (!ch || ch->purged() || victim->purged()) {
		log("SYSERROR: ch = %s, victim = %s (%s:%d)",
			ch ? (ch->purged() ? "purged" : "true") : "false",
			victim->purged() ? "purged" : "true", __FILE__, __LINE__);
		return;
	}
	// Do some sanity checking, in case someone flees, etc.
	if (ch->in_room != victim->in_room || ch->in_room == kNowhere) {
		if (ch->GetEnemy() && ch->GetEnemy() == victim) {
			stop_fighting(ch, true);
		}
		return;
	}
	// Stand awarness mobs
	if (CAN_SEE(victim, ch)
		&& !victim->GetEnemy()
		&& ((victim->IsNpc() && (victim->get_hit() < victim->get_max_hit()
			|| victim->IsFlagged(EMobFlag::kAware)))
			|| AFF_FLAGGED(victim, EAffect::kAwarness))
		&& !AFF_FLAGGED(victim, EAffect::kHold) && victim->get_wait() <= 0) {
		set_battle_pos(victim);
	}

	// дышащий моб может оглушить, и нанесёт физ.дамаг!!
	if (type == ESkill::kUndefined) {
		ESpell spell_id;
		spell_id = GetSpellIdByBreathflag(ch);
		if (spell_id != ESpell::kUndefined) { // защита от падения
			if (!ch->GetEnemy())
				SetFighting(ch, victim);
			if (!victim->GetEnemy())
				SetFighting(victim, ch);
			// AOE атаки всегда магические. Раздаём каждому игроку и уходим.
			if (ch->IsFlagged(EMobFlag::kAreaAttack)) {
				const auto
					people = world[ch->in_room]->people;    // make copy because inside loop this list might be changed.
				for (const auto &tch : people) {
					if (IS_IMMORTAL(tch) || ch->in_room == kNowhere || tch->in_room == kNowhere)
						continue;
					if (tch != ch && !group::same_group(ch, tch)) {
						CastDamage(GetRealLevel(ch), ch, tch, spell_id);
					}
				}
				return;
			}
			// а теперь просто дышащие
			CastDamage(GetRealLevel(ch), ch, victim, spell_id);
			return;
		}
	}

	//go_autoassist(ch);

	// старт инициализации полей для удара
	HitData hit_params;
	//конвертация скиллов, которые предполагают вызов hit()
	//c тип_андеф, в тип_андеф.
	hit_params.skill_num = type != ESkill::kOverwhelm && type != ESkill::kHammer ? type : ESkill::kUndefined;
	hit_params.weapon = weapon;
	hit_params.Init(ch, victim);
	//  дополнительный маг. дамаг независимо от попадания физ. атаки
	if (AFF_FLAGGED(ch, EAffect::kCloudOfArrows)
		&& hit_params.skill_num == ESkill::kUndefined
		&& (ch->GetEnemy() 
		|| (!ch->battle_affects.get(kEafHammer) && !ch->battle_affects.get(kEafOverwhelm)))) {
		// здесь можно получить спурженного victim, но ch не умрет от зеркала
		if (ch->IsNpc()) {
			CastDamage(GetRealLevel(ch), ch, victim, ESpell::kMagicMissile);
		} else {
			CastDamage(1, ch, victim, ESpell::kMagicMissile);
		}
		if (ch->purged() || victim->purged()) { // вдруг помер
			return;
		}
		if (ch->in_room != victim->in_room) {  //если сбег по трусости
			return;
		}
		auto skillnum = GetMagicSkillId(ESpell::kCloudOfArrows);
		TrainSkill(ch, skillnum, true, victim);
	}
	// вычисление хитролов/ац
	hit_params.CalcBaseHitroll(ch);
	hit_params.CalcCircumstantialHitroll(ch, victim);
	hit_params.CalcAc(victim);
	hit_params.CalcDmg(ch); // попытка все собрать в кучу
	// рандом разброс базового дамага для красоты
	if (hit_params.dam > 0) {
		int min_rnd = hit_params.dam - hit_params.dam / 4;
		int max_rnd = hit_params.dam + hit_params.dam / 4;
		hit_params.dam = std::max(1, number(min_rnd, max_rnd));
	}
	if (hit_params.skill_num  == ESkill::kUndefined && !hit_params.GetFlags()[fight::kCritLuck]) { //автоатака
		const int victim_lvl_miss = GetRealLevel(victim) + GetRealRemort(victim);
		const int ch_lvl_miss = GetRealLevel(ch) + GetRealRemort(ch);

		// собсно выяснение попали или нет
		if (victim_lvl_miss - ch_lvl_miss <= 5 || (!ch->IsNpc() && !victim->IsNpc())) {
			// 5% шанс промазать, если цель в пределах 5 уровней или пвп случай
			if ((number(1, 100) <= 5)) {
				hit_params.dam = 0;
				hit_params.ProcessExtradamage(ch, victim);
				hitprcnt_mtrigger(victim);
				return;
			}
		} else {
			// шанс промазать = разнице уровней и мортов
			const int diff = victim_lvl_miss - ch_lvl_miss;
			if (number(1, 100) <= diff) {
				hit_params.dam = 0;
				hit_params.ProcessExtradamage(ch, victim);
				hitprcnt_mtrigger(victim);
				return;
			}
		}
		// всегда есть 5% вероятность попасть (diceroll == 20)
		if ((hit_params.diceroll < 20 && AWAKE(victim))
			&& hit_params.calc_thaco - hit_params.diceroll > hit_params.victim_ac) {
			hit_params.dam = 0;
			hit_params.ProcessExtradamage(ch, victim);
			hitprcnt_mtrigger(victim);
			return;
		}
	}
	// расчет критических ударов
	hit_params.CalcCritHitChance(ch);
	// зовется до DamageEquipment, чтобы не абузить повреждение пушек
	if (hit_params.weapon_pos) {
		DamageEquipment(ch, hit_params.weapon_pos, hit_params.dam, 10);
	}

	if (ProcessBackstab(ch, victim, hit_params)) {
		return;
	}

	if (hit_params.skill_num == ESkill::kThrow) {
		hit_params.SetFlag(fight::kIgnoreFireShield);
		hit_params.dam *= (CalcCurrentSkill(ch, ESkill::kThrow, victim) + 10) / 10;
		if (ch->IsNpc()) {
			hit_params.dam = std::min(300, hit_params.dam);
		}
		hit_params.dam = ApplyResist(victim, EResist::kVitality, hit_params.dam);
		hit_params.ProcessExtradamage(ch, victim);
		return;
	}
	//Рассчёт шанса точного стиля:
	if (!IS_CHARMICE(ch) && ch->battle_affects.get(kEafPunctual) && ch->punctual_wait <= 0 && ch->get_wait() <= 0
		&& (hit_params.diceroll >= 18 - AFF_FLAGGED(victim, EAffect::kHold))) {
		SkillRollResult result = MakeSkillTest(ch, ESkill::kPunctual, victim);
		bool success = result.success;
		TrainSkill(ch, ESkill::kPunctual, success, victim);
		if (!IS_IMMORTAL(ch)) {
			PUNCTUAL_WAIT_STATE(ch, 1 * kBattleRound);
		}
		if (success && (hit_params.calc_thaco - hit_params.diceroll < hit_params.victim_ac - 5
			|| result.CritLuck)) {
			hit_params.SetFlag(fight::kCritHit);
			hit_params.dam_critic = CalcPunctualCritDmg(ch, victim, hit_params.wielded);
			ch->send_to_TC(false, true, false, "&CДамага точки равна = %d&n\r\n", hit_params.dam_critic);
			victim->send_to_TC(false, true, false, "&CДамага точки равна = %d&n\r\n", hit_params.dam_critic);
			if (!IS_IMMORTAL(ch)) {
				PUNCTUAL_WAIT_STATE(ch, 2 * kBattleRound);
			}
			CallMagic(ch, victim, nullptr, nullptr, ESpell::kPaladineInspiration, GetRealLevel(ch));
		}
	}

	// обнуляем флаги, если у нападающего есть лаг
	if ((ch->battle_affects.get(kEafOverwhelm) || ch->battle_affects.get(kEafHammer)) && ch->get_wait() > 0) {
		ch->battle_affects.unset(kEafOverwhelm);
		ch->battle_affects.unset(kEafHammer);
	}

	// обработка защитных скилов (захват, уклон, парир, веер, блок)
	hit_params.ProcessDefensiveAbilities(ch, victim);

	// итоговый дамаг
	ch->send_to_TC(false, true, true, "&CНанёс: Регуляр дамаг = %d&n\r\n", hit_params.dam);
	victim->send_to_TC(false, true, true, "&CПолучил: Регуляр дамаг = %d&n\r\n", hit_params.dam);
	int made_dam = hit_params.ProcessExtradamage(ch, victim);

	//Обнуление лага, когда виктим убит с применением
	//оглушить или молотить. Чтобы все это было похоже на
	//действие скиллов экстраатак(пнуть, сбить и т.д.)
/*
	if (ch->get_wait() > 0
		&& made_dam == -1
		&& (type == ESkill::kOverwhelm
			|| type == ESkill::kHammer))
	{
		ch->set_wait(0u);
	} */
	if (made_dam == -1) {
		if (type == ESkill::kOverwhelm) {
			ch->setSkillCooldown(ESkill::kOverwhelm, 0u);
		} else if (type == ESkill::kHammer) {
			ch->setSkillCooldown(ESkill::kHammer, 0u);
		}
	}

	// check if the victim has a hitprcnt trigger
	if (made_dam != -1) {
		// victim is not dead after hit
		hitprcnt_mtrigger(victim);
	}
}

void ProcessIronWindHits(CharData *ch, fight::AttackType weapon) {
	int percent = 0, prob = 0, div = 0, moves = 0;
	/*
	первая дополнительная атака правой наносится 100%
	вторая дополнительная атака правой начинает наноситься с 80%+ скилла, но не более чем с 80% вероятностью
	первая дополнительная атака левой начинает наноситься сразу, но не более чем с 80% вероятностью
	вторая дополнительная атака левей начинает наноситься с 170%+ скилла, но не более чем с 30% вероятности
	*/
	if (ch->IsFlagged(EPrf::kIronWind)) {
		percent = ch->GetSkill(ESkill::kIronwind);
		moves = ch->get_max_move() / (6 + std::max(10, percent) / 10);
		prob = ch->battle_affects.get(kEafIronWind);
		if (prob && !check_moves(ch, moves)) {
			ch->battle_affects.unset(kEafIronWind);
		} else if (!prob && (ch->get_move() > moves)) {
			ch->battle_affects.set(kEafIronWind);
		};
	};
	if (ch->battle_affects.get(kEafIronWind)) {
		TrainSkill(ch, ESkill::kIronwind, true, ch->GetEnemy());
		if (weapon == fight::kMainHand) {
			div = 100 + std::min(80, std::max(1, percent - 80));
			prob = 100;
		} else {
			div = std::min(80, percent + 10);
			prob = 80 - std::min(30, std::max(0, percent - 170));
		};
		while (div > 0) {
			if (number(1, 100) < div)
				hit(ch, ch->GetEnemy(), ESkill::kUndefined, weapon);
			div -= prob;
		};
	};
}

// Обработка доп.атак
void ProcessExtrahits(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? (ch->purged() ? "purged" : "true") : "false", __FILE__, __LINE__);
		return;
	}
	// обрабатываем доп.атаки от железного ветра
	ProcessIronWindHits(ch, weapon);

	ObjData *wielded = nullptr;
	wielded = GetUsedWeapon(ch, weapon);
	if (wielded
		&& !GET_EQ(ch, EEquipPos::kShield)
		&& static_cast<ESkill>(wielded->get_spec_param()) == ESkill::kBows
		&& GET_EQ(ch, EEquipPos::kBoths)) {
		// Лук в обеих руках - юзаем доп. или двойной выстрел
		if (CanUseFeat(ch, EFeat::kDoubleShot) && !ch->GetSkill(ESkill::kAddshot)
			&& std::min(850, 200 + ch->GetSkill(ESkill::kBows) * 4 + GetRealDex(ch) * 5) >= number(1, 1000)) {
			hit(ch, ch->GetEnemy(), type, weapon);
		} else if (ch->GetSkill(ESkill::kAddshot) > 0) {
			ProcessAddshotHits(ch, victim, type, weapon);
		}
	}
	hit(ch, victim, type, weapon);
}

int CalcPcDamrollBonus(CharData *ch) {
	const int kMaxRemortForDamrollBonus = 35;
	const int kRemortDamrollBonus[kMaxRemortForDamrollBonus + 1] =
		{0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8};
	int bonus = 0;
	if (IS_VIGILANT(ch) || IS_GUARD(ch) || IS_RANGER(ch)) {
		bonus = kRemortDamrollBonus[std::min(kMaxRemortForDamrollBonus, GetRealRemort(ch))];
	}
	if (CanUseFeat(ch, EFeat::kBowsFocus) && ch->GetSkill(ESkill::kAddshot)) {
		bonus *= 3;
	}
	return bonus;
}

int CalcNpcDamrollBonus(CharData *ch) {
	if (GetRealLevel(ch) > kStrongMobLevel) {
		return static_cast<int>(GetRealLevel(ch) * number(100, 200) / 100.0);
	}
	return 0;
}

/**
* еще есть рандом дамролы, в данный момент максимум 30d127
*/
int GetRealDamroll(CharData *ch) {
	if (ch->IsNpc() && !IS_CHARMICE(ch)) {
		return std::max(0, GET_DR(ch) + GET_DR_ADD(ch) + CalcNpcDamrollBonus(ch));
	}

	int bonus = CalcPcDamrollBonus(ch);
	return std::clamp(GET_DR(ch) + GET_DR_ADD(ch) + 2 * bonus, -50, (IS_MORTIFIER(ch) ? 100 : 50) + 2 * bonus);
}

int GetAutoattackDamroll(CharData *ch, int weapon_skill) {
	if (ch->IsNpc() && !IS_CHARMICE(ch)) {
		return std::max(0, GET_DR(ch) + GET_DR_ADD(ch) + CalcNpcDamrollBonus(ch));
	}
	return std::min(GET_DR(ch) + GET_DR_ADD(ch) + 2 * CalcPcDamrollBonus(ch), weapon_skill / 2); //попробюуем так
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
