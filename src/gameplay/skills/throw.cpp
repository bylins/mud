#include "throw.h"

#include "engine/core/action_targeting.h"
#include "gameplay/abilities/abilities_rollsystem.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/core/handler.h"
#include "protect.h"
#include "gameplay/fight/common.h"
#include "gameplay/mechanics/damage.h"

// ************* THROW PROCEDURES

// Временная костыльная функция до реализации встроенного механизма работы сайдскиллов
// TODO Не забыть реализовать постоянный механизм
void PerformShadowThrowSideAbilities(abilities_roll::TechniqueRoll &technique) {
	ObjData *weapon = GET_EQ(technique.GetActor(), technique.GetWeaponEquipPosition());
	if (!weapon) {
		return;
	}

	auto ability_id{abilities::EAbility::kUndefined};
	// У персонажей пока нет абилок и надо как-то утанавливать соответствие абилки и фита
	auto feat_id{EFeat::kUndefined};
	std::string to_char, to_vict, to_room;
	void (*DoSideAction)(abilities_roll::TechniqueRoll &technique){nullptr};
	auto mob_no_flag = EMobFlag::kMobDeleted;

	switch (static_cast<ESkill>(weapon->get_spec_param())) {
		case ESkill::kSpades:
			mob_no_flag = EMobFlag::kNoBash;
			ability_id = abilities::EAbility::kShadowSpear;
			feat_id = EFeat::kShadowSpear;
			to_char = "Попадание копья повалило $n3 наземь.";
			to_vict =
				"Копье $N1 попало вам в колено. Вы рухнули наземь! Кажется, ваши приключения сейчас закончатся...";
			to_room = "Копье $N1 сбило $n3 наземь!";
			DoSideAction = ([](abilities_roll::TechniqueRoll &technique) {
				if (technique.GetRival()->IsOnHorse()) { //если на лошади - падение с лагом 3
					technique.GetRival()->DropFromHorse();
				} else { // иначе просто садится на попу с лагом 2
					auto pos = std::min(technique.GetRival()->GetPosition(), EPosition::kSit);
					technique.GetRival()->SetPosition(pos);
					SetWait(technique.GetRival(), 2, false);
				}
			});
			break;
		case ESkill::kShortBlades:
		case ESkill::kPicks:
			mob_no_flag = EMobFlag::kNoSilence;
			ability_id = abilities::EAbility::kShadowDagger;
			feat_id = EFeat::kShadowDagger;
			to_char = "Меткое попадание вашего кинжала заставило $n3 умолкнуть.";
			to_vict = "Бросок $N1 угодил вам в горло. Вы прикусили язык!";
			to_room = "Меткое попадание $N1 заставило $n3 умолкнуть!";
			DoSideAction = ([](abilities_roll::TechniqueRoll &technique) {
				Affect<EApply> af;
				af.type = ESpell::kBattle;
				af.bitvector = to_underlying(EAffect::kSilence);
				af.duration = CalcDuration(technique.GetRival(), 2, GetRealLevel(technique.GetActor()), 9, 6, 2);
				af.battleflag = kAfBattledec | kAfPulsedec;
				ImposeAffect(technique.GetRival(), af, false, false, false, false);
			});
			break;
		case ESkill::kClubs:mob_no_flag = EMobFlag::kNoOverwhelm;
			ability_id = abilities::EAbility::kShadowClub;
			feat_id = EFeat::kShadowClub;
			to_char = "Попадание булавы ошеломило $n3.";
			to_vict = "Брошенная $N4 булава врезалась вам в лоб! Какие красивые звёздочки вокруг...";
			to_room = "Попадание булавы $N1 ошеломило $n3!";
			DoSideAction = ([](abilities_roll::TechniqueRoll &technique) {
				Affect<EApply> af;
				af.type = ESpell::kBattle;
				af.bitvector = to_underlying(EAffect::kStopFight);
				af.duration = CalcDuration(technique.GetRival(), 3, 0, 0, 0, 0);
				af.battleflag = kAfBattledec | kAfPulsedec;
				ImposeAffect(technique.GetRival(), af, false, false, false, false);
				SetWait(technique.GetRival(), 3, false);
			});
			break;
		default:
			ability_id = abilities::EAbility::kUndefined;
			break;
	};

	if (!CanUseFeat(technique.GetActor(), feat_id)) {
		return;
	};
	abilities_roll::TechniqueRoll side_roll;
	side_roll.Init(technique.GetActor(), ability_id, technique.GetRival());
	if (DoSideAction && side_roll.IsSuccess() && !technique.GetRival()->IsFlagged(mob_no_flag)) {
		act(to_char.c_str(), false, technique.GetRival(), nullptr, technique.GetActor(), kToVict);
		act(to_vict.c_str(), false, technique.GetRival(), nullptr, technique.GetActor(), kToChar);
		act(to_room.c_str(), false, technique.GetRival(), nullptr, technique.GetActor(), kToNotVict | kToArenaListen);
		DoSideAction(technique);
	}
};

// TODO: Перенести подобную логику в модуль абилок, разделить уровни
void PerformWeaponThrow(abilities_roll::TechniqueRoll &technique, Damage &damage) {
	damage.dam = fight::kZeroDmg;
	if (technique.IsSuccess()) {
		damage.dam = technique.CalcDamage();
		if (technique.IsCriticalSuccess()) {
			SendMsgToChar("&GВ яблочко!&n\r\n", technique.GetActor());
			damage.flags.set(fight::kIgnoreArmor);
			damage.flags.set(fight::kCritHit);
			damage.flags.set(fight::kIgnoreBlink);
		};
		if (IsTimedByFeat(technique.GetActor(), EFeat::kShadowThrower)) {
			DecreaseFeatTimer(technique.GetActor(), EFeat::kShadowThrower);
		};
		if (technique.GetAbilityId() == abilities::EAbility::kShadowThrower) {
			PerformShadowThrowSideAbilities(technique);
		};
	} else {
		if (technique.IsCriticalFail()) {
			ObjData *weapon = UnequipChar(technique.GetActor(), technique.GetWeaponEquipPosition(), CharEquipFlags());
			if (weapon) {
				PlaceObjToInventory(weapon, technique.GetActor());
				SendMsgToChar(technique.GetActor(), "&BВы выронили %s!&n\r\n", weapon->get_PName(ECase::kAcc).c_str());
			};
		};
	};
	damage.Process(technique.GetActor(), technique.GetRival());
};

void GoThrow(CharData *ch, CharData *victim) {

	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	// TODO: Возможно, стоит добавить простой тест на добавление целей.
	int victims_amount = 1 + ch->IsFlagged(EPrf::kDoubleThrow) + 2 * ch->IsFlagged(EPrf::kTripleThrow);

	auto technique_id = abilities::EAbility::kThrowWeapon;
	auto dmg_type = fight::kPhysDmg;
	if (ch->IsFlagged(EPrf::kShadowThrow)) {
		SendMsgToChar("Рукоять оружия в вашей руке налилась неестественным холодом.\r\n", ch);
		act("Оружие в руках $n1 окружила призрачная дымка.",
			true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		technique_id = abilities::EAbility::kShadowThrower;
		dmg_type = fight::kMagicDmg;
		TimedFeat timed;
		timed.feat = EFeat::kShadowThrower;
		timed.time = 6;
		ImposeTimedFeat(ch, &timed);
		ch->UnsetFlag(EPrf::kShadowThrow);
	}
	abilities_roll::TechniqueRoll roll;
	Damage damage(SkillDmg(ESkill::kThrow), fight::kZeroDmg, dmg_type, nullptr); //х3 как тут с оружием
	damage.element = EElement::kDark;

	ActionTargeting::FoesRosterType
		roster{ch, victim, [](CharData *ch, CharData *victim) { return CAN_SEE(ch, victim); }};
	for (auto target : roster) {
		if (target->purged() || target->in_room == kNowhere)
			continue;
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
	if (technique_id == abilities::EAbility::kThrowWeapon) {
		SetSkillCooldownInFight(ch, ESkill::kThrow, 3);
	}
}

void DoThrow(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	//Svent TODO: Не забыть убрать заглушку после дописывания навыков
	if (!ch->GetSkill(ESkill::kThrow)) {
		SendMsgToChar("Вы принялись метать икру. Это единственное, что вы умеете метать.\r\n", ch);
		return;
	}

	if (subcmd == kScmdShadowThrow && !CanUseFeat(ch, EFeat::kShadowThrower)) {
		SendMsgToChar("Вы этого не умеете.\r\n", ch);
		return;
	};

	CharData *victim = FindVictim(ch, argument);
	if (!victim) {
		SendMsgToChar("В кого мечем?\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, victim, argument)) {
		return;
	}
	if (!check_pkill(ch, victim, arg)) {
		return;
	}

	if (subcmd == kScmdShadowThrow) {
		if (IsTimedByFeat(ch, EFeat::kShadowThrower)) {
			SendMsgToChar("Не стоит так часто беспокоить тёмные силы.\r\n", ch);
			return;
		}
		ch->SetFlag(EPrf::kShadowThrow);
	};

	DoThrow(ch, victim);
}

void DoThrow(CharData *ch, CharData *victim) {
	if (!ch->GetSkill(ESkill::kThrow)) {
		log("ERROR: вызов метнуть для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->HasCooldown(ESkill::kThrow)) {
		SendMsgToChar("Так и рука отвалится, нужно передохнуть.\r\n", ch);
		return;
	};

	if (ch == victim) {
		SendMsgToChar("Вы начали метаться как белка в колесе.\r\n", ch);
		return;
	}

	if (IS_IMPL(ch) || !ch->GetEnemy()) {
		GoThrow(ch, victim);
	} else {
		if (IsHaveNoExtraAttack(ch)) {
			act("Хорошо. Вы попытаетесь метнуть оружие в $N3.", false, ch, nullptr, victim, kToChar);
			ch->SetExtraAttack(kExtraAttackThrow, victim);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
