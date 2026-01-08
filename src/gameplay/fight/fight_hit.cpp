#include "fight_hit.h"

#include "engine/ui/color.h"
#include "gameplay/magic/magic.h"
#include "pk.h"
#include "gameplay/statistics/dps.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/mechanics/poison.h"
#include "gameplay/ai/mobact.h"
#include "fight.h"
#include "engine/db/global_objects.h"
#include "utils/backtrace.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/classes/pc_classes.h"
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
#include "gameplay/skills/ironwind.h"
#include "gameplay/mechanics/armor.h"
#include "gameplay/skills/addshot.h"

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

int HitData::ProcessExtradamage(CharData *ch, CharData *victim) {
	if (!check_valid_chars(ch, victim, __FILE__, __LINE__)) {
		return 0;
	}

	const int memorized_dam = dam;
	dam = std::max(0, dam);
	ProcessMighthit(ch, victim, *this);
	ProcessOverhelm(ch, victim, *this);
	ProcessPoisonedWeapom(ch, victim, *this);
	ProcessToxicMob(ch, victim, *this);
	ProcessPunctualStyle(ch, victim, *this);
	auto dmg = GenerateExtradamage(memorized_dam);
	return dmg.Process(ch, victim);
}

Damage HitData::GenerateExtradamage(int initial_dmg) {
	// Если удар парирован, необходимо все равно ввязаться в драку.
	// Вызывается damage с отрицательным уроном
	dam = (initial_dmg >= 0 ? dam : -1);
	Damage dmg(SkillDmg(skill_num), dam, fight::kPhysDmg, wielded);
	dmg.hit_type = hit_type;
	dmg.dam_critic = dam_critic;
	dmg.flags = GetFlags();
	dmg.ch_start_pos = ch_start_pos;
	dmg.victim_start_pos = victim_start_pos;
	return dmg;
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

	if (wielded && wielded->get_type() == EObjType::kWeapon) {
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
			if (!CanUseFeat(ch, EFeat::kBully)) {
				calc_thaco += 4;
			} else {
				calc_thaco -= 3;
			}
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
void HitData::CalcCircumstantialAc(CharData *victim) {
	victim_ac = CalcTotalAc(victim, victim_ac);
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
		dam *= 1.0 + (ch->GetSkill(ESkill::kRiding) - 100) / 500.0;
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
	if (victim->get_extracted_list()) { //уже раз убит и в списке на удаление
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
		if (victim->get_extracted_list()) { //уже раз убит и в списке на удаление
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
	hit_params.CalcCircumstantialAc(victim);
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

// Обработка доп.атак
void ProcessExtrahits(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon) {
	if (!ch || ch->purged()) {
		log("SYSERROR: ch = %s (%s:%d)", ch ? (ch->purged() ? "purged" : "true") : "false", __FILE__, __LINE__);
		return;
	}
	if (victim->get_extracted_list()) { //уже раз убит и в списке на удаление
		return;
	}

	ProcessIronWindHits(ch, weapon);
	ProcessMultyShotHits(ch, victim, type, weapon);
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
	// И с чего бы такое счастье именно обладателям луков и допа?
	// Кстати, к ремортам оно каким боком?
//	if (CanUseFeat(ch, EFeat::kBowsFocus) && ch->GetSkill(ESkill::kAddshot)) {
//		bonus *= 3;
//	}
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
