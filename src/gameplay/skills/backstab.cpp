#include "backstab.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/core/handler.h"
#include "protect.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/damage.h"

int GetBackstabMultiplier(int level);
int CalcCritBackstabPercent(CharData *ch);
double CalcCritBackstabMultiplier(CharData *ch, CharData *victim);

// делегат обработки команды заколоть
void DoBackstab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetSkill(ESkill::kBackstab) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	CharData *vict = get_char_vis(ch, arg, EFind::kCharInRoom);
	if (!vict) {
		SendMsgToChar("Кого вы так сильно ненавидите, что хотите заколоть?\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	do_backstab(ch, vict);
}

void do_backstab(CharData *ch, CharData *vict) {
	if (ch->GetSkill(ESkill::kBackstab) < 1) {
		log("ERROR: вызов стаба для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->HasCooldown(ESkill::kBackstab)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (ch->IsOnHorse()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Вы, определенно, садомазохист!\r\n", ch);
		return;
	}

	if (!GET_EQ(ch, EEquipPos::kWield) && (!ch->IsNpc() || IS_CHARMICE(ch))) {
		SendMsgToChar("Требуется держать оружие в правой руке.\r\n", ch);
		return;
	}

	if ((!ch->IsNpc() || IS_CHARMICE(ch)) && GET_OBJ_VAL(GET_EQ(ch, EEquipPos::kWield), 3) != fight::type_pierce) {
		SendMsgToChar("ЗаКОЛоть можно только КОЛющим оружием!\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kStopRight) || IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (vict->GetEnemy() && !CanUseFeat(ch, EFeat::kThieveStrike)) {
		SendMsgToChar("Ваша цель слишком быстро движется - вы можете пораниться!\r\n", ch);
		return;
	}

	GoBackstab(ch, vict);
}

// *********************** BACKSTAB VICTIM
// Проверка на стаб в бою происходит до вызова этой функции
void GoBackstab(CharData *ch, CharData *vict) {

	if (ch->IsHorsePrevents())
		return;
//	if (vict->purged()) {
//		return;
//	}

	vict = TryToFindProtector(vict, ch);

	if (!pk_agro_action(ch, vict))
		return;

	if ((vict->IsFlagged(EMobFlag::kAware) && AWAKE(vict)) && !IS_GOD(ch)) {
		act("Вы заметили, что $N попытал$u вас заколоть!", false, vict, nullptr, ch, kToChar);
		act("$n заметил$g вашу попытку заколоть $s!", false, vict, nullptr, ch, kToVict);
		act("$n заметил$g попытку $N1 заколоть $s!", false, vict, nullptr, ch, kToNotVict | kToArenaListen);
		hit(ch, vict, ESkill::kUndefined,
			AFF_FLAGGED(ch, EAffect::kStopRight) ? fight::kOffHand : fight::kMainHand);
		return;
	}

	bool success = false;
///*	
	if (ch->IsFlagged(EPrf::kTester)) {
		SkillRollResult result = MakeSkillTest(ch, ESkill::kBackstab, vict);
		success = result.success;
	} else 
//*/
{
		int percent = number(1, MUD::Skill(ESkill::kBackstab).difficulty);
		int prob = CalcCurrentSkill(ch, ESkill::kBackstab, vict);

		if (CanUseFeat(ch, EFeat::kShadowStrike)) {
			prob = prob + prob * 20 / 100;
		};

		if (vict->GetEnemy()) {
			prob = prob * (GetRealDex(ch) + 50) / 100;
		}

		if (AFF_FLAGGED(ch, EAffect::kHide)) {
			prob += 5;
		}
		if (AFF_FLAGGED(vict, EAffect::kHold)) {
			prob = prob * 5 / 4;
		}
		if (GET_GOD_FLAG(vict, EGf::kGodscurse)) {
			prob = percent;
		}
		if (GET_GOD_FLAG(vict, EGf::kGodsLike) || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
			prob = 0;
		}
		success = percent <= prob;
		SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kBackstab).name, percent, prob, success);
	}

	TrainSkill(ch, ESkill::kBackstab, success, vict);
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kBackstab), fight::kZeroDmg, fight::kPhysDmg, ch->equipment[EEquipPos::kWield]);
		dmg.Process(ch, vict);
	} else {
		hit(ch, vict, ESkill::kBackstab, fight::kMainHand);
	}
	SetWait(ch, 1, true);
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
	SetSkillCooldownInFight(ch, ESkill::kBackstab, 2);
}

bool ProcessBackstab(CharData *ch, CharData *victim, HitData &hit_data) {
	if (hit_data.skill_num != ESkill::kBackstab) {
		return false;
	}
	hit_data.ResetFlag(fight::kCritHit);
	hit_data.SetFlag(fight::kIgnoreFireShield);
	if (CanUseFeat(ch, EFeat::kThieveStrike)) {
		hit_data.SetFlag(fight::kIgnoreSanct);
		hit_data.SetFlag(fight::kIgnoreBlink);
		hit_data.SetFlag(fight::kIgnoreArmor);
	} else if (CanUseFeat(ch, EFeat::kShadowStrike)) {
		hit_data.SetFlag(fight::kIgnoreArmor);
	} else {
		hit_data.SetFlag(fight::kHalfIgnoreArmor);
	}
	if (CanUseFeat(ch, EFeat::kShadowStrike)) {
		hit_data.dam *=
			GetBackstabMultiplier(GetRealLevel(ch)) * (1.0 + ch->GetSkill(ESkill::kNoParryHit) / 200.0);
	} else if (CanUseFeat(ch, EFeat::kThieveStrike)) {
		if (victim->GetEnemy()) {
			hit_data.dam *= GetBackstabMultiplier(GetRealLevel(ch));
		} else {
			hit_data.dam *= GetBackstabMultiplier(GetRealLevel(ch)) * 1.3;
		}
	} else {
		hit_data.dam *= GetBackstabMultiplier(GetRealLevel(ch));
	}

	if (!ch->IsNpc()
		&& CanUseFeat(ch, EFeat::kShadowStrike)
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		&& !(AFF_FLAGGED(victim, EAffect::kGodsShield) && !(victim->IsFlagged(EMobFlag::kProtect)))
		&& (number(1, 100) <= 6 * ch->get_cond_penalty(P_HITROLL))
		&& !victim->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
		victim->set_hit(1);
		hit_data.dam = victim->get_hit() + fight::kLethalDmg;
		SendMsgToChar(ch, "&GПрямо в сердце, насмерть!&n\r\n");
		hit_data.SetFlag(fight::kIgnoreBlink);
		hit_data.ProcessExtradamage(ch, victim);
		return true;
	}

	if (!ch->IsNpc()
		&& (number(1, 100) < CalcCritBackstabPercent(ch) * ch->get_cond_penalty(P_HITROLL))
		&& (!CalcGeneralSaving(ch, victim, ESaving::kCritical, CalculateSkillRate(ch, ESkill::kBackstab, victim)))) {
		hit_data.dam = static_cast<int>(hit_data.dam * CalcCritBackstabMultiplier(ch, victim));
		if ((hit_data.dam > 0)
			&& !AFF_FLAGGED(victim, EAffect::kGodsShield)
			&& !(victim->IsFlagged(EMobFlag::kProtect))) {
			SendMsgToChar("&GПрямо в сердце!&n\r\n", ch);
			hit_data.SetFlag(fight::kIgnoreBlink);
		}
	}

	hit_data.dam = ApplyResist(victim, EResist::kVitality, hit_data.dam);
	// режем стаб
	if (CanUseFeat(ch, EFeat::kShadowStrike) && !ch->IsNpc()) {
		hit_data.dam = std::min(8000 + GetRealRemort(ch) * 20 * GetRealLevel(ch), hit_data.dam);
	}

	ch->send_to_TC(false, true, false, "&CДамага стаба равна = %d&n\r\n", hit_data.dam);
	victim->send_to_TC(false, true, false, "&RДамага стаба  равна = %d&n\r\n", hit_data.dam);
	hit_data.ProcessExtradamage(ch, victim);
	return true;
}

int GetBackstabMultiplier(int level) {
	if (level <= 0)
		return 1;    // level 0 //
	else if (level <= 5)
		return 2;    // level 1 - 5 //
	else if (level <= 10)
		return 3;    // level 6 - 10 //
	else if (level <= 15)
		return 4;    // level 11 - 15 //
	else if (level <= 20)
		return 5;    // level 16 - 20 //
	else if (level <= 25)
		return 6;    // level 21 - 25 //
	else if (level <= 30)
		return 7;    // level 26 - 30 //
	else
		return 10;
}

/**
* Процент прохождения крит.стаба = скилл/11 + (декса-20)/(декса/30) для вовровского удара,
* для остального в учет только ловку
* при 74х мортах максимальный скилл заколоть будет около 500. декса 90 ессно - получается 75% критстабов.
* попробуем скилл/15 + (декса - 20) / (декса/20) - получается около 50% критстабов.
* TO DO.. еще удачу есть план добавить в расчет шанса критстаба
*/
int CalcCritBackstabPercent(CharData *ch) {
	double percent = ((GetRealDex(ch) -20) / (GetRealDex(ch) / 20.0));
	if (CanUseFeat(ch, EFeat::kThieveStrike)) {
		percent += (ch->GetSkill(ESkill::kBackstab) / 15.0);
	}
	return static_cast<int>(percent);
}

/*
 *  Расчет множителя критстаба.
* против игроков только у воров.
 */
double CalcCritBackstabMultiplier(CharData *ch, CharData *victim) {
	double bs_coeff = 1.0;
	if (victim->IsNpc()) {
		if (CanUseFeat(ch, EFeat::kThieveStrike)) {
			bs_coeff *= ch->GetSkill(ESkill::kBackstab) / 15.0;
		} else {
			bs_coeff *= ch->GetSkill(ESkill::kBackstab) / 25.0;
		}
		if (CanUseFeat(ch, EFeat::kShadowStrike) && (ch->GetSkill(ESkill::kNoParryHit))) {
			bs_coeff *= (1 + (ch->GetSkill(ESkill::kNoParryHit) * 0.00125));
		}
	} else if (CanUseFeat(ch, EFeat::kThieveStrike)) {
		if (victim->GetEnemy()) {
			bs_coeff *= (1.0 + (ch->GetSkill(ESkill::kBackstab) * 0.00225));
		} else {
			bs_coeff *= (1.0 + (ch->GetSkill(ESkill::kBackstab) * 0.00350));
		}
	}

	return std::max(1.0, bs_coeff);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
