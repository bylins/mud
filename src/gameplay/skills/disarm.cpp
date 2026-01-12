#include "disarm.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/core/handler.h"
#include "utils/random.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/damage.h"

#include <cmath>

// ************* DISARM PROCEDURES
void do_disarm(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kDisarm)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого обезоруживаем?\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	do_disarm(ch, vict);
}

void do_disarm(CharData *ch, CharData *vict) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kDisarm)) {
		log("ERROR: вызов дизарма для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}
	
	if (ch->HasCooldown(ESkill::kDisarm)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (ch == vict) {
		SendMsgToChar("Попробуйте набрать \"снять <название.оружия>\".\r\n", ch);
		return;
	}
	if (vict->purged()) {
		return;
	}

	if (CanUseFeat(ch, EFeat::kInjure)) {
		if (IsAffectedBySpellWithCasterId(ch, vict, ESpell::kNoInjure) && (!vict->HasWeapon())) {
			act("Не получится - $N уже понял$G, что от Вас можно ожидать всякого!",
				false, ch, nullptr, vict, kToChar);
		} else if (IsAffectedBySpellWithCasterId(ch, vict, ESpell::kNoInjure) && (vict->HasWeapon())) {
			if (IS_IMPL(ch) || !ch->GetEnemy()) {
				go_disarm(ch, vict);
			} else if (IsHaveNoExtraAttack(ch)) {
				act("Хорошо. Вы попытаетесь разоружить $N3.", false, ch, nullptr, vict, kToChar);
				ch->SetExtraAttack(kExtraAttackDisarm, vict);
			}
		} else {
			if (IS_IMPL(ch) || !ch->GetEnemy()) {
				go_injure(ch, vict);
			} else if (IsHaveNoExtraAttack(ch)) {
				act("Хорошо. Вы попытаетесь ранить $N3.", false, ch, nullptr, vict, kToChar);
				ch->SetExtraAttack(kExtraAttackInjure, vict);
			}
		}
	} else {
		if (!vict->HasWeapon()) {
			SendMsgToChar("Вы не сможете обезоружить безоружного!\r\n", ch);
			return;
		} else {
			if (IS_IMPL(ch) || !ch->GetEnemy()) {
				go_disarm(ch, vict);
			} else if (IsHaveNoExtraAttack(ch)) {
				act("Хорошо. Вы попытаетесь разоружить $N3.", false, ch, nullptr, vict, kToChar);
				ch->SetExtraAttack(kExtraAttackDisarm, vict);
			}
		}
	}
}

void go_injure(CharData *ch, CharData *vict) {
	SkillRollResult result = MakeSkillTest(ch, ESkill::kDisarm, vict);
	bool injure_success = result.success;

	if (injure_success) {
		int injure_duration = std::min((2 + ch->GetSkill(ESkill::kDisarm) / 20), 10);

		if (!vict->IsNpc()) {
			injure_duration *= 30;
		}

//Ввожу ДВА аффекта: 1. Короткий - собственно сам дебафф. 2. Долгий, для запрета повторного наложения.
//af.bitvector = to_underlying(EAffect::kInjured); - этот битвектор нафиг не нужен. Ввел его только для того чтобы не показывало !UNDEF! при выводе аффектов
		Affect<EApply> af;
		af.type = ESpell::kLowerEffectiveness;
		af.duration = injure_duration;
		af.modifier = -(10 + std::min((ch->GetSkill(ESkill::kDisarm) / 10), 20));
		af.location = EApply::kPhysicDamagePercent;
		af.battleflag = kAfBattledec;
		af.bitvector = to_underlying(EAffect::kInjured);
		affect_to_char(vict, af);

		act("Вы ранили $N3! Как же $E теперь будет Вас бить?...",
			false, ch, nullptr, vict, kToChar);
		act("$N ранил$G Вас в руку! Надо бы дать сдачи...",
			false, vict, nullptr, ch, kToChar);
		act("$N ранил$G $n3. Кажется $n0 уже передумал$g драться...",
			false, vict, nullptr, ch, kToNotVict | kToArenaListen);

		int dam = number(ceil(ch->GetSkill(ESkill::kDisarm) / 1.25), ceil(ch->GetSkill(ESkill::kDisarm) * 1.25))
			* GetRealLevel(ch) / 30;
		Damage dmg(SkillDmg(ESkill::kDisarm), dam, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kIgnoreBlink);
		dmg.Process(ch, vict);
	} else {
		Damage dmg(SkillDmg(ESkill::kDisarm), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kIgnoreBlink);
		dmg.Process(ch, vict);

		act("Вам не удалось ранить $N3 и $e продолжил$g весело бить Вас!",
			false, ch, nullptr, vict, kToChar);
		act("$N попытал$U поранить Вас, но у н$S не вышло! На радостях Вы принялись колотить $S еще веселей!",
			false, vict, nullptr, ch, kToChar);
		act("$N безуспешно попытал$U поранить $n3. Как нелепо...",
			false, vict, nullptr, ch, kToNotVict | kToArenaListen);
	}
	if (vict->GetPosition() != EPosition::kDead) {
		int no_injure_duration = 4;

		if (!vict->IsNpc()) {
			no_injure_duration *= 30;
		}
//Этот аффект ничего не дает, просто предотвращает повторное наложение дебаффа тем же персонажем.
		Affect<EApply> af2;
		af2.type = ESpell::kNoInjure;
		af2.duration = no_injure_duration;
		af2.battleflag = kNone;
		af2.caster_id = ch->get_uid();
		affect_to_char(vict, af2);

		if (!vict->HasWeapon()) {
			TrainSkill(ch, ESkill::kDisarm, injure_success, vict);
			SetSkillCooldown(ch, ESkill::kDisarm, 1);
			return;
		} else {
			go_disarm(ch, vict);
		}
	}
}

void go_disarm(CharData *ch, CharData *vict) {
	ObjData
		*wielded = GET_EQ(vict, EEquipPos::kWield) ? GET_EQ(vict, EEquipPos::kWield) : GET_EQ(vict, EEquipPos::kBoths),
		*helded = GET_EQ(vict, EEquipPos::kHold);

	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (!vict->HasWeapon()) {
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
	SkillRollResult result = MakeSkillTest(ch, ESkill::kDisarm, vict);
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
					  kColorWht,
					  GET_PAD(vict, 3),
					  kColorNrm);
		lag = 2;
	} else {
		wielded = GET_EQ(vict, pos);
		SendMsgToChar(ch, "%sВы ловко выбили %s из рук %s!%s\r\n",
					  kColorBoldBlu, wielded->get_PName(ECase::kAcc).c_str(), GET_PAD(vict, 1), kColorNrm);
		SendMsgToChar(vict, "Ловкий удар %s выбил %s%s из ваших рук.\r\n",
					  GET_PAD(ch, 1), wielded->get_PName(ECase::kAcc).c_str(), char_get_custom_label(wielded, vict).c_str());
		act("$n ловко выбил$g $o3 из рук $N1.", true, ch, wielded, vict, kToNotVict | kToArenaListen);
		UnequipChar(vict, pos, CharEquipFlags());
		SetSkillCooldown(ch, ESkill::kGlobalCooldown, vict->IsNpc() ? 1 : 2);
		lag = 2;
		if (ROOM_FLAGGED(vict->in_room, ERoomFlag::kArena) || (!IS_MOB(vict)) || vict->has_master()) {
			PlaceObjToInventory(wielded, vict);
		} else {
			PlaceObjToRoom(wielded, vict->in_room);
			CheckObjDecay(wielded);
		};
	}
	Damage dmg(SkillDmg(ESkill::kDisarm), fight::kZeroDmg, fight::kPhysDmg, nullptr);
	dmg.Process(ch, vict);
	SetSkillCooldown(ch, ESkill::kDisarm, lag);
	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
