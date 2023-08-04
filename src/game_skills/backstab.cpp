#include "backstab.h"

#include "game_fight/pk.h"
#include "game_fight/fight.h"
#include "game_fight/common.h"
#include "game_fight/fight_hit.h"
#include "game_fight/fight_start.h"
#include "handler.h"
#include "protect.h"
#include "structs/global_objects.h"

// делегат обработки команды заколоть
void do_backstab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetSkill(ESkill::kBackstab) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
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

	if (GET_POS(ch) < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	CharData *vict = get_char_vis(ch, arg, EFind::kCharInRoom);
	if (!vict) {
		SendMsgToChar("Кого вы так сильно ненавидите, что хотите заколоть?\r\n", ch);
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

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;
	go_backstab(ch, vict);
}

// *********************** BACKSTAB VICTIM
// Проверка на стаб в бою происходит до вызова этой функции
void go_backstab(CharData *ch, CharData *vict) {

	if (ch->IsHorsePrevents())
		return;

	vict = TryToFindProtector(vict, ch);

	if (!pk_agro_action(ch, vict))
		return;

	if ((MOB_FLAGGED(vict, EMobFlag::kAware) && AWAKE(vict)) && !IS_GOD(ch)) {
		act("Вы заметили, что $N попытал$u вас заколоть!", false, vict, nullptr, ch, kToChar);
		act("$n заметил$g вашу попытку заколоть $s!", false, vict, nullptr, ch, kToVict);
		act("$n заметил$g попытку $N1 заколоть $s!", false, vict, nullptr, ch, kToNotVict | kToArenaListen);
		hit(ch, vict, ESkill::kUndefined,
			AFF_FLAGGED(ch, EAffect::kStopRight) ? fight::kOffHand : fight::kMainHand);
		return;
	}

	bool success = false;
///*	
	if (PRF_FLAGGED(ch, EPrf::kTester)) {
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
		CalcBackstabDamage(ch, vict);
	}
	SetWait(ch, 1, true);
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
	SetSkillCooldownInFight(ch, ESkill::kBackstab, 2);
}
void CalcBackstabDamage(CharData *ch, CharData *vict) {
	HitData hit_params;
	hit_params.skill_num = ESkill::kBackstab;
	hit_params.weapon = fight::kMainHand;
	hit_params.init(ch, vict);
	hit_params.reset_flag(fight::kCritHit);
	hit_params.set_flag(fight::kIgnoreFireShield);
	if (CanUseFeat(ch, EFeat::kThieveStrike)) {
		hit_params.set_flag(fight::kIgnoreSanct);
		hit_params.set_flag(fight::kIgnoreBlink);
		hit_params.set_flag(fight::kIgnoreArmor);
	} else if (CanUseFeat(ch, EFeat::kShadowStrike)) {
		hit_params.set_flag(fight::kIgnoreArmor);
	} else {
		hit_params.set_flag(fight::kHalfIgnoreArmor);
	}
	hit_params.calc_damage(ch);
	if (CanUseFeat(ch, EFeat::kShadowStrike)) {
		hit_params.dam *= backstab_mult(GetRealLevel(ch)) * (1.0 + ch->GetSkill(ESkill::kNoParryHit) / 200.0);
	} else if (CanUseFeat(ch, EFeat::kThieveStrike)) {
		if (vict->GetEnemy()) {
			hit_params.dam *= backstab_mult(GetRealLevel(ch));
		} else {
			hit_params.dam *= backstab_mult(GetRealLevel(ch)) * 1.3;
		}
	} else {
		hit_params.dam *= backstab_mult(GetRealLevel(ch));
	}
	Damage dmg(SkillDmg(ESkill::kBackstab), hit_params.dam, fight::kPhysDmg, hit_params.wielded);
	dmg.flags = hit_params.get_flags();
	dmg.Process(ch, vict);
	alt_equip(ch, hit_params.weapon_pos, hit_params.dam, 10);
	return;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
