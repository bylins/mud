//Modified by Svetodar 14.09.2023

#include "strangle.h"
#include "chopoff.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "utils/random.h"
#include "protect.h"
#include "gameplay/mechanics/damage.h"

#include <cmath>

void do_strangle(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetSkill(ESkill::kStrangle)) {
		SendMsgToChar("Вы не умеете этого.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого вы жаждете удавить?\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (!check_pkill(ch, vict, arg)) {
		return;
	}

	do_strangle(ch, vict);
}

void do_strangle(CharData *ch, CharData *vict) {
	if (!ch->GetSkill(ESkill::kStrangle)) {
		log("ERROR: вызов удавки для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->HasCooldown(ESkill::kGlobalCooldown)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}

	if (ch == vict->GetEnemy()) {
		act("Не получится сделать это незаметно - $N уже сражается с вами.", false, ch, nullptr, vict, kToChar);
		return;
	}

	if (IS_UNDEAD(vict) || GET_RACE(vict) == ENpcRace::kFish ||
		GET_RACE(vict) == ENpcRace::kPlant || GET_RACE(vict) == ENpcRace::kConstruct) {
		SendMsgToChar("Вы бы еще верстовой столб удавить попробовали...\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Воспользуйтесь услугами княжеского палача. Постоянным клиентам - скидки!\r\n", ch);
		return;
	}

	if (IsAffectedBySpellWithCasterId(ch, vict, ESpell::kStrangle)) {
		act("Не получится - $N уже понял$G, что от Вас можно ожидать всякого!",
			false, ch, nullptr, vict, kToChar);
		return;
	}
	go_strangle(ch, vict);
}

void go_strangle(CharData *ch, CharData *vict) {
	if (AFF_FLAGGED(ch, EAffect::kStopRight) || IsUnableToAct(ch)) {
		SendMsgToChar("Сейчас у вас не получится выполнить этот прием.\r\n", ch);
		return;
	}

	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}
//	if (vict->purged()) {
//		return;
//	}
	vict = TryToFindProtector(vict, ch);
	if (!pk_agro_action(ch, vict)) {
		return;
	}

	act("Вы попытались накинуть удавку на шею $N2.\r\n", false, ch, nullptr, vict, kToChar);

	SkillRollResult result = MakeSkillTest(ch, ESkill::kStrangle,vict);
	bool success = result.success;
//Формула дамага - (удавить/10)% от хп моба (максимум 10%) + удавить*2. Рандом - +/- 25%. Сделал отдельными переменными для удобочитаемости, иначе фиг разберёшь формулу.
	int hp_percent_dam = ceil(vict->get_max_hit() * 0.03);
	auto skill_strangle = ch->GetSkill(ESkill::kStrangle);
	int hp_percent_dam2 = (vict->get_max_hit() / 100) * (skill_strangle / 10);
	int flat_damage = skill_strangle * 1.5;
	int dam = number(ceil(std::min(hp_percent_dam, hp_percent_dam2) + flat_damage) * 1.25,
					ceil(std::min(hp_percent_dam, hp_percent_dam2) + flat_damage) / 1.25);
	int strangle_duration = 5;

	if (!vict->IsNpc()) {
		strangle_duration *= 30;
	}

	Affect<EApply> af;
	af.type = ESpell::kStrangle;
	af.duration = strangle_duration;
	af.battleflag = kNone;
	af.bitvector = to_underlying(EAffect::kStrangled);
	af.caster_id = ch->get_uid();

	TrainSkill(ch, ESkill::kStrangle, success, vict);
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kStrangle), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
	} else {
		if (AFF_FLAGGED(vict, EAffect::kStrangled)) {
			dam = number(ceil((flat_damage * 1.25)), ceil((flat_damage / 1.25)));
		}
		int silence_duration = 2 + std::min(skill_strangle/25, 10);

		if (!vict->IsNpc()) {
			silence_duration *= 30;
		}

		Affect<EApply> af2;
		af2.type = ESpell::kSilence;
		af2.duration = silence_duration;
		af2.modifier = -skill_strangle/3;
		af2.location = EApply::kMagicDamagePercent;
		af2.battleflag = kAfBattledec;
		af2.bitvector = to_underlying(EAffect::kSilence);
		affect_to_char(vict, af2);

		Damage dmg(SkillDmg(ESkill::kStrangle), dam, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kIgnoreArmor);
		dmg.flags.set(fight::kIgnoreBlink);
		dmg.Process(ch, vict);
		if (vict->GetPosition() > EPosition::kDead) {
			SetWait(vict, 2, true);
			if (vict->IsOnHorse()) {
				act("Рванув на себя, $N стащил$G Вас на землю.",
					false, vict, nullptr, ch, kToChar);
				act("Рванув на себя, Вы стащили $n3 на землю.",
					false, vict, nullptr, ch, kToVict);
				act("Рванув на себя, $N стащил$G $n3 на землю.",
					false, vict, nullptr, ch, kToNotVict | kToArenaListen);
				vict->DropFromHorse();
			}
		}
		SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
	}
	affect_to_char(vict, af);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
