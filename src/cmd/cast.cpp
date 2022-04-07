#include "cast.h"

#include "entities/char_data.h"
#include "game_magic/magic_utils.h"
#include "game_classes/classes_spell_slots.h"
#include "game_magic/spells_info.h"
#include "handler.h"
#include "color.h"

/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to CastSpell().
 */
void do_cast(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	CharData *tch;
	ObjData *tobj;
	RoomData *troom;

	char *s, *t;
	int i, spellnum, spell_subst, target = 0;

	if (ch->is_npc() && AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		SendMsgToChar("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kGlobalCooldown)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!ch->affected.empty()) {
		for (const auto &aff : ch->affected) {
			if (aff->location == EApply::kPlague && number(1, 100) < 10) { // лихорадка 10% фэйл закла
				SendMsgToChar("Вас трясет лихорадка, вы не смогли сконцентрироваться на произнесении заклинания.\r\n",
							 ch);
				return;
			}
			if (aff->location == EApply::kMadness && number(1, 100) < 20) { // безумие 20% фэйл закла
				SendMsgToChar("Начав безумно кричать, вы забыли, что хотели произнести.\r\n", ch);
				return;
			}
		}
	}

	if (ch->is_morphed()) {
		SendMsgToChar("Вы не можете произносить заклинания в звериной форме.\r\n", ch);
		return;
	}

	// get: blank, spell name, target name
	s = strtok(argument, "'*!");
	if (s == nullptr) {
		SendMsgToChar("ЧТО вы хотите колдовать?\r\n", ch);
		return;
	}
	s = strtok(nullptr, "'*!");
	if (s == nullptr) {
		SendMsgToChar("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}
	t = strtok(nullptr, "\0");

	spellnum = FixNameAndFindSpellNum(s);
	spell_subst = spellnum;

	// Unknown spell
	if (spellnum < 1 || spellnum > kSpellCount) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	// Caster is lower than spell level
	if ((!IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellTemp | kSpellKnow) ||
		GET_REAL_REMORT(ch) < MIN_CAST_REM(spell_info[spellnum], ch)) &&
		(GetRealLevel(ch) < kLvlGreatGod) && !ch->is_npc()) {
		if (GetRealLevel(ch) < MIN_CAST_LEV(spell_info[spellnum], ch)
			|| GET_REAL_REMORT(ch) < MIN_CAST_REM(spell_info[spellnum], ch)
			|| PlayerClass::CalcCircleSlotsAmount(ch,
												  spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
				<= 0) {
			SendMsgToChar("Рано еще вам бросаться такими словами!\r\n", ch);
			return;
		} else {
			SendMsgToChar("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
			return;
		}
	}

	// Caster havn't slot
	if (!GET_SPELL_MEM(ch, spellnum) && !IS_IMMORTAL(ch)) {
		if (IsAbleToUseFeat(ch, EFeat::kSpellSubstitute)
			&& (spellnum == kSpellCureLight || spellnum == kSpellCureSerious
				|| spellnum == kSpellCureCritic || spellnum == kSpellHeal)) {
			for (i = 1; i <= kSpellCount; i++) {
				if (GET_SPELL_MEM(ch, i) &&
					spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] ==
						spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
					spell_subst = i;
					break;
				}
			}
			if (i > kSpellCount) {
				SendMsgToChar("У вас нет заученных заклинаний этого круга.\r\n", ch);
				return;
			}
		} else {
			SendMsgToChar("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
			return;
		}
	}

	// Find the target
	if (t != nullptr)
		one_argument(t, arg);
	else
		*arg = '\0';

	target = FindCastTarget(spellnum, arg, ch, &tch, &tobj, &troom);

	if (target && (tch == ch) && spell_info[spellnum].violent) {
		SendMsgToChar("Лекари не рекомендуют использовать ЭТО на себя!\r\n", ch);
		return;
	}
	if (!target) {
		SendMsgToChar("Тяжеловато найти цель вашего заклинания!\r\n", ch);
		return;
	}
	if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellTemp) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		SendMsgToChar("На данной арене вы можете колдовать только временные заклинания!\r\n", ch);
		return;
	}

	// You throws the dice and you takes your chances.. 101% is total failure
	// Чтобы в бой не вступал с уже взведенной заклинашкой !!!
	ch->set_cast(0, 0, 0, 0, 0);
	if (!CalcCastSuccess(ch, tch, ESaving::kStability, spellnum)) {
		if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)))
			SetWaitState(ch, kPulseViolence);
		if (GET_SPELL_MEM(ch, spell_subst)) {
			GET_SPELL_MEM(ch, spell_subst)--;
		}
		if (!ch->is_npc() && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, EPrf::kAutomem))
			MemQ_remember(ch, spell_subst);
		//log("[DO_CAST->AFFECT_TOTAL] Start");
		affect_total(ch);
		//log("[DO_CAST->AFFECT_TOTAL] Stop");
		if (!tch || !SendSkillMessages(0, ch, tch, spellnum))
			SendMsgToChar("Вы не смогли сосредоточиться!\r\n", ch);
	} else        // cast spell returns 1 on success; subtract mana & set waitstate
	{
		if (ch->get_fighting() && !IS_IMPL(ch)) {
			ch->set_cast(spellnum, spell_subst, tch, tobj, troom);
			sprintf(buf,
					"Вы приготовились применить заклинание %s'%s'%s%s.\r\n",
					CCCYN(ch, C_NRM), spell_info[spellnum].name, CCNRM(ch, C_NRM),
					tch == ch ? " на себя" : tch ? " на $N3" : tobj ? " на $o3" : troom ? " на всех" : "");
			act(buf, false, ch, tobj, tch, kToChar);
		} else if (CastSpell(ch, tch, tobj, troom, spellnum, spell_subst) >= 0) {
			if (!(IS_IMMORTAL(ch) || ch->get_wait() > 0))
				SetWaitState(ch, kPulseViolence);
		}
	}
}


