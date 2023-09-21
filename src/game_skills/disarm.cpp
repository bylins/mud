#include "disarm.h"

#include "game_fight/pk.h"
#include "game_fight/common.h"
#include "game_fight/fight_hit.h"
#include "handler.h"
#include "utils/random.h"
#include "color.h"
#include "structs/global_objects.h"
#include "game_fight/fight.h"

// ************* DISARM PROCEDURES
void do_disarm(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kDisarm)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kDisarm)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого обезоруживаем?\r\n", ch);
		return;
	}

	if (ch == vict) {
		SendMsgToChar("Попробуйте набрать \"снять <название.оружия>\".\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (CanUseFeat(ch, EFeat::kInjure)) {
		SendMsgToChar("Пробуем ранить\r\n", ch);
		if (IS_IMPL(ch) || !ch->GetEnemy()) {
			go_injure(ch, vict);
		} else if (IsHaveNoExtraAttack(ch)) {
			act("Хорошо. Вы попытаетесь разоружить $N3.", false, ch, nullptr, vict, kToChar);
			ch->SetExtraAttack(kExtraAttackInjure, vict);
		}
	} else {
		SendMsgToChar("Сразу пытаемся обезоружить.\r\n", ch);
		if (IS_IMPL(ch) || !ch->GetEnemy()) {
			go_disarm(ch, vict);
		} else if (IsHaveNoExtraAttack(ch)) {
			act("Хорошо. Вы попытаетесь разоружить $N3.", false, ch, nullptr, vict, kToChar);
			ch->SetExtraAttack(kExtraAttackDisarm, vict);
		}
	}
}

void go_injure(CharData *ch, CharData *vict) {
	bool can_injure = true;
	SendMsgToChar("Начинаем цикл!\r\n", ch);
	for (long & ids : vict->injure_id) {
		if (ids == GET_ID(ch) && AFF_FLAGGED(vict, EAffect::kInjured)) {
			act("Не получится - $N уже понял$G, что от Вас можно ожидать всякого!",
				false, ch, nullptr, vict, kToChar);
			can_injure = false;
			break;
		}
	}
	SkillRollResult result = MakeSkillTest(ch, ESkill::kDisarm,vict);
	bool success = result.success;

	if (can_injure) {
		if (success) {
			SendMsgToChar("Вешаем аффект!\r\n", ch);
			int injure_duration = std::min((2 + GET_SKILL(ch, ESkill::kDisarm) / 20), 10);

			if (!vict->IsNpc()) {
				injure_duration *= 30;
			}
			Affect<EApply> af;
			af.type = ESpell::kLowerEffectiveness;
			af.duration = injure_duration;
			af.modifier = -std::min((GET_SKILL(ch, ESkill::kDisarm) / 10), 40);
			af.location = EApply::kPhysicDamagePercent;
			af.battleflag = kAfBattledec;
			af.bitvector = to_underlying(EAffect::kInjured);
			affect_to_char(vict, af);
			act("Вы ранили $N3! Как же $E теперь будет Вас бить?...",
				false, ch, nullptr,vict, kToChar);
			act("$N ранил$G Вас в руку! Надо немедленно дать сдачи! Может быть даже ногами...",
				false,vict, nullptr, ch, kToChar);
			act("$N ранил$G $n3. Кажется $n0 уже передумал$g драться...",
				false,vict, nullptr, ch, kToNotVict | kToArenaListen);
		} else {
			act("Вам не удалось ранить $N3 и $e продолжил$g весело бить Вас!",
				false, ch, nullptr,vict, kToChar);
			act("$N попытал$U поранить Вас, но у н$S не вышло! На радостях Вы принялись колотить $S еще веселей!",
				false,vict, nullptr, ch, kToChar);
			act("$N безуспешно попытал$U поранить $n3. Как нелепо...",
				false,vict, nullptr, ch, kToNotVict | kToArenaListen);
		}
		vict->injure_id.push_back(GET_ID(ch));
		Damage dmg(SkillDmg(ESkill::kDisarm), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(vict, EGf::kGodscurse) || GET_GOD_FLAG(ch, EGf::kGodsLike))) {
			SetSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
		}

	}
	if (!((GET_EQ(vict, EEquipPos::kWield)
		   && GET_OBJ_TYPE(GET_EQ(vict, EEquipPos::kWield)) != EObjType::kLightSource)
		  || (GET_EQ(vict, EEquipPos::kHold)
			  && GET_OBJ_TYPE(GET_EQ(vict, EEquipPos::kHold)) != EObjType::kLightSource)
		  || (GET_EQ(vict, EEquipPos::kBoths)
			  && GET_OBJ_TYPE(GET_EQ(vict, EEquipPos::kBoths)) != EObjType::kLightSource))) {
		TrainSkill(ch, ESkill::kDisarm, success, vict);
		SendMsgToChar("Вы ТОЧНО не сможете обезоружить безоружного.\r\n", ch);
		return;
	} else {
		go_disarm(ch, vict);
	}
}

void go_disarm(CharData *ch, CharData *vict) {
	ObjData *wielded = GET_EQ(vict, EEquipPos::kWield) ? GET_EQ(vict, EEquipPos::kWield) :
					   GET_EQ(vict, EEquipPos::kBoths), *helded = GET_EQ(vict, EEquipPos::kHold);

	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (!((wielded && GET_OBJ_TYPE(wielded) != EObjType::kLightSource)
		|| (helded && GET_OBJ_TYPE(helded) != EObjType::kLightSource))) {
		SendMsgToChar("Вы не можете обезоружить безоружного.\r\n", ch);
		return;
	}
	int pos = 0;
	if (number(1, 100) > 30) {
		pos = wielded ? (GET_EQ(vict, EEquipPos::kBoths) ? EEquipPos::kBoths : EEquipPos::kWield) : EEquipPos::kHold;
	} else {
		pos = helded ? EEquipPos::kHold : (GET_EQ(vict, EEquipPos::kBoths) ? EEquipPos::kBoths : EEquipPos::kWield);
	}

	if (!pos || !GET_EQ(vict, pos))
		return;
	if (!pk_agro_action(ch, vict))
		return;
	SkillRollResult result = MakeSkillTest(ch, ESkill::kDisarm,vict);
	bool success = result.success;
	int lag;

	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(vict, EGf::kGodscurse) || GET_GOD_FLAG(ch, EGf::kGodsLike))
		success = true;
	if (IS_IMMORTAL(vict) || GET_GOD_FLAG(ch, EGf::kGodscurse) || GET_GOD_FLAG(vict, EGf::kGodsLike)
		|| CanUseFeat(vict, EFeat::kStrongClutch))
		success = false;

	TrainSkill(ch, ESkill::kDisarm, success, vict);
	if (!success || GET_EQ(vict, pos)->has_flag(EObjFlag::kNodisarm)) {
		SendMsgToChar(ch,
					  "%sВы не сумели обезоружить %s...%s\r\n",
					  CCWHT(ch, C_NRM),
					  GET_PAD(vict, 3),
					  CCNRM(ch, C_NRM));
		lag = 2;
	} else {
		wielded = GET_EQ(vict, pos);
		SendMsgToChar(ch, "%sВы ловко выбили %s из рук %s!%s\r\n",
					  CCIBLU(ch, C_NRM), wielded->get_PName(3).c_str(), GET_PAD(vict, 1), CCNRM(ch, C_NRM));
		SendMsgToChar(vict, "Ловкий удар %s выбил %s%s из ваших рук.\r\n",
					  GET_PAD(ch, 1), wielded->get_PName(3).c_str(), char_get_custom_label(wielded, vict).c_str());
		act("$n ловко выбил$g $o3 из рук $N1.", true, ch, wielded, vict, kToNotVict | kToArenaListen);
		UnequipChar(vict, pos, CharEquipFlags());
		SetSkillCooldown(ch, ESkill::kGlobalCooldown, vict->IsNpc() ? 1 : 2);
		lag = 2;
		if (ROOM_FLAGGED(IN_ROOM(vict), ERoomFlag::kArena) || (!IS_MOB(vict)) || vict->has_master()) {
			PlaceObjToInventory(wielded, vict);
		} else {
			PlaceObjToRoom(wielded, IN_ROOM(vict));
			CheckObjDecay(wielded);
		};
	}
	appear(ch);
	Damage dmg(SkillDmg(ESkill::kDisarm), fight::kZeroDmg, fight::kPhysDmg, nullptr);
	dmg.Process(ch, vict);
	SetSkillCooldown(ch, ESkill::kDisarm, lag);
	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
