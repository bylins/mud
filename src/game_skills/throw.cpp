#include "throw.h"

#include "action_targeting.h"
#include "game_abilities/abilities_rollsystem.h"

#include "game_fight/pk.h"
#include "game_fight/fight.h"
#include "game_fight/fight_hit.h"
#include "handler.h"
#include "protect.h"
#include "game_fight/common.h"

// ************* THROW PROCEDURES

// Временная костыльная функция до реализации встроенного механизма работы сайдскиллов
// TODO Не забыть реализовать постоянный механизм
void PerformShadowThrowSideAbilities(AbilitySystem::TechniqueRoll &technique) {
	ObjData *weapon = GET_EQ(technique.GetActor(), technique.GetWeaponEquipPosition());
	if (!weapon) {
		return;
	}

	auto feature_id{EFeat::kUndefined};
	std::string to_char, to_vict, to_room;
	void (*DoSideAction)(AbilitySystem::TechniqueRoll &technique);
	Bitvector mob_no_flag = EMobFlag::kMobDeleted;

	switch (static_cast<ESkill>(weapon->get_skill())) {
		case ESkill::kSpades:
			mob_no_flag = EMobFlag::kNoBash;
			feature_id = EFeat::kShadowSpear;
			to_char = "Попадание копья повалило $n3 наземь.";
			to_vict =
				"Копье $N1 попало вам в колено. Вы рухнули наземь! Кажется, ваши приключения сейчас закончатся...";
			to_room = "Копье $N1 сбило $n3 наземь!";
			DoSideAction = ([](AbilitySystem::TechniqueRoll &technique) {
				if (technique.GetRival()->IsOnHorse()) { //если на лошади - падение с лагом 3
					technique.GetRival()->drop_from_horse();
				} else { // иначе просто садится на попу с лагом 2
					GET_POS(technique.GetRival()) = std::min(GET_POS(technique.GetRival()), EPosition::kSit);
					SetWait(technique.GetRival(), 2, false);
				}
			});
			break;
		case ESkill::kShortBlades:
		case ESkill::kPicks:
			mob_no_flag = EMobFlag::kNoSilence;
			feature_id = EFeat::kShadowDagger;
			to_char = "Меткое попадание вашего кинжала заставило $n3 умолкнуть.";
			to_vict = "Бросок $N1 угодил вам в горло. Вы прикусили язык!";
			to_room = "Меткое попадание $N1 заставило $n3 умолкнуть!";
			DoSideAction = ([](AbilitySystem::TechniqueRoll &technique) {
				Affect<EApply> af;
				af.type = kSpellBattle;
				af.bitvector = to_underlying(EAffect::kSilence);
				af.duration = CalcDuration(technique.GetRival(), 2, GetRealLevel(technique.GetActor()), 9, 6, 2);
				af.battleflag = kAfBattledec | kAfPulsedec;
				ImposeAffect(technique.GetRival(), af, false, false, false, false);
			});
			break;
		case ESkill::kClubs:mob_no_flag = EMobFlag::kNoOverwhelm;
			feature_id = EFeat::kShadowClub;
			to_char = "Попадание булавы ошеломило $n3.";
			to_vict = "Брошенная $N4 булава врезалась вам в лоб! Какие красивые звёздочки вокруг...";
			to_room = "Попадание булавы $N1 ошеломило $n3!";
			DoSideAction = ([](AbilitySystem::TechniqueRoll &technique) {
				Affect<EApply> af;
				af.type = kSpellBattle;
				af.bitvector = to_underlying(EAffect::kStopFight);
				af.duration = CalcDuration(technique.GetRival(), 3, 0, 0, 0, 0);
				af.battleflag = kAfBattledec | kAfPulsedec;
				ImposeAffect(technique.GetRival(), af, false, false, false, false);
				SetWait(technique.GetRival(), 3, false);
			});
			break;
		default:
			feature_id = EFeat::kUndefined;
			break;
	};

	if (!IsAbleToUseFeat(technique.GetActor(), feature_id)) {
		return;
	};
	AbilitySystem::TechniqueRoll side_roll;
	side_roll.Init(technique.GetActor(), feature_id, technique.GetRival());
	if (side_roll.IsSuccess() && !MOB_FLAGGED(technique.GetRival(), mob_no_flag)) {
		act(to_char.c_str(), false, technique.GetRival(), nullptr, technique.GetActor(), kToVict);
		act(to_vict.c_str(), false, technique.GetRival(), nullptr, technique.GetActor(), kToChar);
		act(to_room.c_str(), false, technique.GetRival(), nullptr, technique.GetActor(), kToNotVict | kToArenaListen);
		DoSideAction(technique);
	}
};

// TODO: Перенести подобную логику в модуль абилок, разделить уровни
void PerformWeaponThrow(AbilitySystem::TechniqueRoll &technique, Damage &damage) {
	damage.dam = fight::kZeroDmg;
	if (technique.IsSuccess()) {
		damage.dam = technique.CalcDamage();
		if (technique.IsCriticalSuccess()) {
			SendMsgToChar("&GВ яблочко!&n\r\n", technique.GetActor());
			damage.flags.set(fight::kIgnoreArmor);
			damage.flags.set(fight::kCritHit);
		};
		if (IsTimedByFeat(technique.GetActor(), EFeat::kShadowThrower)) {
			DecreaseFeatTimer(technique.GetActor(), EFeat::kShadowThrower);
		};
		if (technique.GetAbilityId() == EFeat::kShadowThrower) {
			PerformShadowThrowSideAbilities(technique);
		};
	} else {
		if (technique.IsCriticalFail()) {
			ObjData *weapon = UnequipChar(technique.GetActor(), technique.GetWeaponEquipPosition(), CharEquipFlags());
			if (weapon) {
				PlaceObjToInventory(weapon, technique.GetActor());
				SendMsgToChar(technique.GetActor(), "&BВы выронили %s!&n\r\n", GET_OBJ_PNAME(weapon, 3).c_str());
			};
		};
	};
	damage.Process(technique.GetActor(), technique.GetRival());
};

void go_throw(CharData *ch, CharData *victim) {

	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	// TODO: Возможно, стоит добавить простой тест на добавление целей.
	int victims_amount = 1 + PRF_FLAGGED(ch, EPrf::kDoubleThrow) + 2 * PRF_FLAGGED(ch, EPrf::kTripleThrow);

	auto technique_id = EFeat::kThrowWeapon;
	auto dmg_type = fight::kPhysDmg;
	if (PRF_FLAGGED(ch, EPrf::kShadowThrow)) {
		SendMsgToChar("Рукоять оружия в вашей руке налилась неестественным холодом.\r\n", ch);
		act("Оружие в руках $n1 окружила призрачная дымка.",
			true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		technique_id = EFeat::kShadowThrower;
		dmg_type = fight::kMagicDmg;
		TimedFeat timed;
		timed.feat = EFeat::kShadowThrower;
		timed.time = 6;
		ImposeTimedFeat(ch, &timed);
		PRF_FLAGS(ch).unset(EPrf::kShadowThrow);
	}
	AbilitySystem::TechniqueRoll roll;
	Damage damage(SkillDmg(ESkill::kThrow), fight::kZeroDmg, dmg_type, nullptr); //х3 как тут с оружием
	damage.magic_type = kTypeDark;

	ActionTargeting::FoesRosterType
		roster{ch, victim, [](CharData *ch, CharData *victim) { return CAN_SEE(ch, victim); }};
	for (auto target : roster) {
		target = TryToFindProtector(target, ch);
		roll.Init(ch, technique_id, target);
		if (roll.IsWrongConditions()) {
			roll.SendDenyMsgToActor();
			break;
		};
		PerformWeaponThrow(roll, damage);
		--victims_amount;
		if (ch->purged() || victims_amount == 0) {
			break;
		};
	};

	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
	if (technique_id == EFeat::kThrowWeapon) {
		SetSkillCooldownInFight(ch, ESkill::kThrow, 3);
	}
}

void do_throw(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	//Svent TODO: Не забыть убрать заглушку после дописывания навыков
	if (!ch->get_skill(ESkill::kThrow)) {
		SendMsgToChar("Вы принялись метать икру. Это единственное, что вы умеете метать.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kThrow)) {
		SendMsgToChar("Так и рука отвалится, нужно передохнуть.\r\n", ch);
		return;
	};
/*
	if (!IS_IMPL(ch) && !can_use_feat(ch, EFeat::kThrowWeapon)) {
			SendMsgToChar("Вы не умеете этого.\r\n", ch);
			return;
	}
*/
	CharData *victim = FindVictim(ch, argument);
	if (!victim) {
		SendMsgToChar("В кого мечем?\r\n", ch);
		return;
	}

	if (ch == victim) {
		SendMsgToChar("Вы начали метаться как белка в колесе.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, victim, argument)) {
		return;
	}
	if (!check_pkill(ch, victim, arg)) {
		return;
	}

	if (subcmd == SCMD_SHADOW_THROW) {
		if (IsTimedByFeat(ch, EFeat::kShadowThrower)) {
			SendMsgToChar("Не стоит так часто беспокоить тёмные силы.\r\n", ch);
			return;
		}
		PRF_FLAGS(ch).set(EPrf::kShadowThrow);
	};

	if (IS_IMPL(ch) || !ch->GetEnemy()) {
		go_throw(ch, victim);
	} else {
		if (IsHaveNoExtraAttack(ch)) {
			act("Хорошо. Вы попытаетесь метнуть оружие в $N3.",
				false, ch, nullptr, victim, kToChar);
			ch->SetExtraAttack(kExtraAttackThrow, victim);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
