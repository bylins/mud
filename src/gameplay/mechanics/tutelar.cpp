/**
\file tutelar.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 05.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "gameplay/skills/skills.h"
#include "utils/utils.h"
#include "gameplay/mechanics/minions.h"
#include "engine/db/db.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/skills/resque.h"

void SummonTutelar(CharData *ch) {
	MobVnum mob_num = 108;
	//int modifier = 0;
	CharData *mob = nullptr;
	struct FollowerType *k, *k_next;

	auto eff_cha = get_effective_cha(ch);

	for (k = ch->followers; k; k = k_next) {
		k_next = k->next;
		if (k->follower->IsFlagged(EMobFlag::kTutelar)) {
			stop_follower(k->follower, kSfCharmlost);
		}
	}

	float base_success = 26.0;
	float additional_success_for_charisma = 1.5; // 50 at 16 charisma, 101 at 50 charisma

	if (number(1, 100) > floorf(base_success + additional_success_for_charisma * eff_cha)) {
		SendMsgToRoom("Яркая вспышка света! Несколько белых перьев кружась легли на землю...", ch->in_room, true);
		return;
	};
	if (!(mob = ReadMobile(mob_num, kVirtual))) {
		SendMsgToChar("Вы точно не помните, как создать данного монстра.\r\n", ch);
		return;
	}

	int base_hp = 360;
	int additional_hp_for_charisma = 40;
	//float base_shields = 0.0;
	// 0.72 shield at 16 charisma, 1 shield at 23 charisma. 45 for 2 shields
	//float additional_shields_for_charisma = 0.0454;
	float base_awake = 2;
	float additional_awake_for_charisma = 4; // 64 awake on 16 charisma, 202 awake at 50 charisma
	float base_multiparry = 2;
	float additional_multiparry_for_charisma = 2; // 34 multiparry on 16 charisma, 102 multiparry at 50 charisma;
	float base_rescue = 20.0;
	float additional_rescue_for_charisma = 2.5; // 60 rescue at 16 charisma, 135 rescue at 50 charisma;
	float base_heal = 0;
	float additional_heal_for_charisma = 0.12; // 1 heal at 16 charisma,  6 heal at 50 charisma;
	float base_ttl = 10.0;
	float additional_ttl_for_charisma = 0.25; // 14 min at 16 chsrisma, 22 min at 50 charisma;
	float base_ac = 100;
	float additional_ac_for_charisma = -2.5; //
	float base_armour = 0;
	float additional_armour_for_charisma = 0.5; // 8 armour for 16 charisma, 25 armour for 50 charisma

	ClearMinionTalents(mob);
	Affect<EApply> af;
	af.type = ESpell::kCharm;
	af.duration = CalcDuration(mob, floorf(base_ttl + additional_ttl_for_charisma * eff_cha), 0, 0, 0, 0);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = 0;
	af.bitvector = to_underlying(EAffect::kHelper);
	affect_to_char(mob, af);

	af.bitvector = to_underlying(EAffect::kFly);
	affect_to_char(mob, af);

	af.bitvector = to_underlying(EAffect::kInfravision);
	affect_to_char(mob, af);

	af.bitvector = to_underlying(EAffect::kSanctuary);
	affect_to_char(mob, af);

	//Set shields
	float base_shields = 0.0;
	float additional_shields_for_charisma = 0.0454; // 0.72 shield at 16 charisma, 1 shield at 23 charisma. 45 for 2 shields
	int count_shields = base_shields + floorf(eff_cha * additional_shields_for_charisma);
	if (count_shields > 0) {
		mob->SetFlag(EMobFlag::kNoHold);
		af.bitvector = to_underlying(EAffect::kAirShield);
		affect_to_char(mob, af);
	}
	if (count_shields > 1) {
		af.bitvector = to_underlying(EAffect::kIceShield);
		affect_to_char(mob, af);
	}
	if (count_shields > 2) {
		af.bitvector = to_underlying(EAffect::kFireShield);
		affect_to_char(mob, af);
	}

	if (IS_FEMALE(ch)) {
		mob->set_sex(EGender::kMale);
		mob->SetCharAliases("Небесный защитник");
		mob->player_data.PNames[ECase::kNom] = "Небесный защитник";
		mob->player_data.PNames[ECase::kGen] = "Небесного защитника";
		mob->player_data.PNames[ECase::kDat] = "Небесному защитнику";
		mob->player_data.PNames[ECase::kAcc] = "Небесного защитника";
		mob->player_data.PNames[ECase::kIns] = "Небесным защитником";
		mob->player_data.PNames[ECase::kPre] = "Небесном защитнике";
		mob->set_npc_name("Небесный защитник");
		mob->player_data.long_descr = str_dup("Небесный защитник летает тут.\r\n");
		mob->player_data.description = str_dup("Сияющая призрачная фигура о двух крылах.\r\n");
	} else {
		mob->set_sex(EGender::kFemale);
		mob->SetCharAliases("Небесная защитница");
		mob->player_data.PNames[ECase::kNom] = "Небесная защитница";
		mob->player_data.PNames[ECase::kGen] = "Небесной защитницы";
		mob->player_data.PNames[ECase::kDat] = "Небесной защитнице";
		mob->player_data.PNames[ECase::kAcc] = "Небесную защитницу";
		mob->player_data.PNames[ECase::kIns] = "Небесной защитницей";
		mob->player_data.PNames[ECase::kPre] = "Небесной защитнице";
		mob->set_npc_name("Небесная защитница");
		mob->player_data.long_descr = str_dup("Небесная защитница летает тут.\r\n");
		mob->player_data.description = str_dup("Сияющая призрачная фигура о двух крылах.\r\n");
	}

	float additional_str_for_charisma = 0.6875;
	float additional_dex_for_charisma = 1.0;
	float additional_con_for_charisma = 1.0625;
	float additional_int_for_charisma = 1.5625;
	float additional_wis_for_charisma = 0.6;
	float additional_cha_for_charisma = 1.375;

	mob->set_str(1 + floorf(additional_str_for_charisma * eff_cha));
	mob->set_dex(1 + floorf(additional_dex_for_charisma * eff_cha));
	mob->set_con(1 + floorf(additional_con_for_charisma * eff_cha));
	mob->set_int(std::max(50, 1 + static_cast<int>(floorf(additional_int_for_charisma * eff_cha)))); //кап 50
	mob->set_wis(1 + floorf(additional_wis_for_charisma * eff_cha));
	mob->set_cha(1 + floorf(additional_cha_for_charisma * eff_cha));

	GET_WEIGHT(mob) = 150;
	GET_HEIGHT(mob) = 200;
	GET_SIZE(mob) = 65;

	GET_HR(mob) = 1;
	GET_AC(mob) = floorf(base_ac + additional_ac_for_charisma * eff_cha);
	GET_DR(mob) = 0;
	GET_ARMOUR(mob) = floorf(base_armour + additional_armour_for_charisma * eff_cha);

	mob->mob_specials.damnodice = 1;
	mob->mob_specials.damsizedice = 1;
	mob->mob_specials.extra_attack = 0;

	mob->set_exp(0);

	mob->set_max_hit(floorf(base_hp + additional_hp_for_charisma * eff_cha));
	mob->set_hit(mob->get_max_hit());
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;

	mob->SetPosition(EPosition::kStand);
	GET_DEFAULT_POS(mob) = EPosition::kStand;

	mob->set_skill(ESkill::kRescue, floorf(base_rescue + additional_rescue_for_charisma * eff_cha));
	mob->set_skill(ESkill::kAwake, floorf(base_awake + additional_awake_for_charisma * eff_cha));
	mob->set_skill(ESkill::kMultiparry, floorf(base_multiparry + additional_multiparry_for_charisma * eff_cha));

	int base_spell = 2 + count_shields;

	SET_SPELL_MEM(mob, ESpell::kCureBlind, base_spell);
	SET_SPELL_MEM(mob, ESpell::kRemoveHold, base_spell);
	SET_SPELL_MEM(mob, ESpell::kRemovePoison, base_spell);
	SET_SPELL_MEM(mob, ESpell::kHeal, floorf(base_heal + additional_heal_for_charisma * eff_cha));
	mob->mob_specials.have_spell = true;
	if (mob->GetSkill(ESkill::kAwake)) {
		mob->SetFlag(EPrf::kAwake);
	}
	GET_LIKES(mob) = 100;
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;
	mob->SetFlag(EMobFlag::kCorpse);
	mob->SetFlag(EMobFlag::kTutelar);
	mob->SetFlag(EMobFlag::kLightingBreath);
	mob->set_level(GetRealLevel(ch));
	PlaceCharToRoom(mob, ch->in_room);
	if (IS_FEMALE(mob)) {
		act("Небесная защитница появилась в яркой вспышке света!",
			true, mob, nullptr, nullptr, kToRoom | kToArenaListen);
	} else {
		act("Небесный защитник появился в яркой вспышке света!",
			true, mob, nullptr, nullptr, kToRoom | kToArenaListen);
	}
	ch->add_follower(mob);
}

void CheckTutelarSelfSacrfice(CharData *ch, CharData *victim) {
	// если виктим в группе с кем-то с ангелом - вместо смерти виктима умирает ангел
	if (victim->get_hit() <= 0
		&& !victim->IsNpc()
		&& AFF_FLAGGED(victim, EAffect::kGroup)) {
		// make copy of people because keeper might be removed from this list inside the loop
		const auto people = world[victim->in_room]->people;
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

void TryToRescueWithTutelar(CharData *ch) {
	struct FollowerType *k, *k_next;
	for (k = ch->followers; k; k = k_next) {
		k_next = k->next;
		if (AFF_FLAGGED(k->follower, EAffect::kHelper)
			&& k->follower->IsFlagged(EMobFlag::kTutelar)
			&& !k->follower->GetEnemy()
			&& k->follower->in_room == ch->in_room
			&& CAN_SEE(k->follower, ch)
			&& AWAKE(k->follower)
			&& !IsUnableToAct(k->follower)
			&& k->follower->get_wait() <= 0
			&& k->follower->GetPosition() >= EPosition::kFight) {
			for (const auto vict : world[ch->in_room]->people) {
				if (vict->GetEnemy() == ch
					&& vict != ch
					&& vict != k->follower) {
					if (k->follower->GetSkill(ESkill::kRescue)) {
						go_rescue(k->follower, ch, vict);
					}
					break;
				}
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
