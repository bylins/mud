//
// Created by Svetodar on 21.07.2023.
//

#include "slay.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "protect.h"
#include "engine/entities/obj_data.h"
#include "gameplay/mechanics/damage.h"

#include <cmath>

void go_slay(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch) || AFF_FLAGGED(ch, EAffect::kStopRight)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}
	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}
	if (vict->purged()) {
		return;
	}
	vict = TryToFindProtector(vict, ch);

	ObjData *GetUsedWeapon(CharData *ch, fight::AttackType AttackType);
	ObjData *obj = GetUsedWeapon(ch, fight::AttackType::kMainHand);
	int weapon_dmg = (GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1)) / 2;
	int lag;
	int dam;

	SkillRollResult result = MakeSkillTest(ch, ESkill::kSlay, vict);
	bool success = result.success;

	if (AFF_FLAGGED(vict, EAffect::kHold) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		success = result.success;
	}

	if (GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		success = !result.success;
	}

	TrainSkill(ch, ESkill::kSlay, success, vict);

	if (!success) {
		Damage dmg(SkillDmg(ESkill::kSlay), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		lag = 2;

	} else {
		dam = (number(ceil((((GetRealStr(ch) + weapon_dmg) / 1.2) * (ch->GetSkill(ESkill::kSlay) / 6)
						  + ((GetRealDamroll(ch)) * 2)) / 1.25),
					  ceil((((GetRealStr(ch) + weapon_dmg) / 1.2) * (ch->GetSkill(ESkill::kSlay) / 6)
						  + ((GetRealDamroll(ch)) * 2)) * 1.25)) *
			GetRealLevel(ch)) / 30;

		Damage dmg(SkillDmg(ESkill::kSlay), dam, fight::kPhysDmg, ch->equipment[EEquipPos::kWield]);
		dmg.flags.set(fight::kIgnoreBlink);
		dmg.Process(ch, vict);
		lag = 1;

		if (vict->GetPosition() == EPosition::kDead) {
			lag = 0;
		}

		if (AFF_FLAGGED(vict, EAffect::kGodsShield)) {
			lag = 1;
		}
	}
	switch (lag) {
		case 0: SetWait(ch, 0, true);
			break;
		case 1: SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
			break;
		case 2: SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
			break;
	}
}

void do_slay(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict = FindVictim(ch, argument);

	if (!ch->GetSkill(ESkill::kSlay)) {
		SendMsgToChar("Вы не умеете сражать, только сра... В общем, не умеете.\r\n", ch);
		return;
	}
	if (!vict) {
		SendMsgToChar("Кого же вы хотите сразить?\r\n", ch);
		return;
	}
	if (ch == vict) {
		SendMsgToChar("Сразить себя?! Вы же дружинник, а не самурай!\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kAwake)) {
		SendMsgToChar("Вы не можете потрошить врагов и при этом осторожничать!\r\n", ch);
		return;
	}
	if (!IS_IMMORTAL(ch) && !(GET_EQ(ch, EEquipPos::kWield) || GET_EQ(ch, EEquipPos::kBoths))) {
		SendMsgToChar("Для этого Вам потребуется оружие!\r\n", ch);
		return;
	}
	if (!(AFF_FLAGGED(vict, EAffect::kConfused) || IS_IMMORTAL(ch))) {
		SendMsgToChar("Это не так просто! Сначала попробуйте обескуражить противника!\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kSlay)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}
	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;
	if (IS_IMPL(ch) || !ch->GetEnemy()) {
		go_slay(ch, vict);
	} else if (IsHaveNoExtraAttack(ch)) {
		if (!ch->IsNpc())
			act("Хорошо. Вы попытаетесь сразить $N3.", false, ch, nullptr, vict, kToChar);
		ch->SetExtraAttack(kExtraAttackSlay, vict);
	}
}

