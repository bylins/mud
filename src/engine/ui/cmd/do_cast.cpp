#include "do_cast.h"

#include "engine/entities/char_data.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/classes/classes_spell_slots.h"
#include "gameplay/magic/spells_info.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"

auto FindSubstituteSpellId(CharData *ch, ESpell spell_id) {
	static const std::set<ESpell> healing_spells{
		ESpell::kCureLight,
		ESpell::kCureSerious,
		ESpell::kCureCritic,
		ESpell::kHeal};

	auto subst_spell_id{ESpell::kUndefined};
	if (CanUseFeat(ch, EFeat::kSpellSubstitute) && healing_spells.contains(spell_id)) {
		for (const auto &test_spell : MUD::Class(ch->GetClass()).spells) {
			auto test_spell_id = test_spell.GetId();
			if (GET_SPELL_MEM(ch, test_spell_id) &&
				MUD::Class(ch->GetClass()).spells[test_spell_id].GetCircle() ==
					MUD::Class(ch->GetClass()).spells[spell_id].GetCircle()) {
				subst_spell_id = test_spell_id;
				break;
			}
		}
	}

	return subst_spell_id;
}


/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to CastSpell().
 */
void DoCast(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed))
		return;
	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kGlobalCooldown)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}
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
	if (!*argument) {
		SendMsgToChar("ЧТО вы хотите колдовать?\r\n", ch);
		return;
	}
	auto spell_name = strtok(argument, "'*!");
	if (!str_cmp(spell_name, argument)) {
		SendMsgToChar("Название заклинания должно быть заключено в символы : * или !\r\n", ch);
		return;
	}
	const auto spell_id = FixNameAndFindSpellId(spell_name);
	if (spell_id == ESpell::kUndefined) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}
	if (const auto spell = MUD::Class(ch->GetClass()).spells[spell_id];
		(!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp | ESpellType::kKnow) ||
		GetRealRemort(ch) < spell.GetMinRemort()) &&
		(GetRealLevel(ch) < kLvlGreatGod) && !ch->IsNpc()) {
		if (GetRealLevel(ch) < CalcMinSpellLvl(ch, spell_id)
			|| classes::CalcCircleSlotsAmount(ch, spell.GetCircle()) <= 0) {
			SendMsgToChar("Рано еще вам бросаться такими словами!\r\n", ch);
			return;
		} else {
			SendMsgToChar("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
			return;
		}
	}

	auto substitute_spell_id{ESpell::kUndefined};
	if (!GET_SPELL_MEM(ch, spell_id) && !IS_IMMORTAL(ch)) {
		substitute_spell_id = FindSubstituteSpellId(ch, spell_id);
		if (substitute_spell_id == ESpell::kUndefined) {
			SendMsgToChar("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
			return;
		}
	} else {
		substitute_spell_id = spell_id;
	}


	if (auto target_name = strtok(nullptr, "\0"); target_name != nullptr) {
		one_argument(target_name, arg);
	} else {
		*arg = '\0';
	}

	CharData *tch;
	ObjData *tobj;
	RoomData *troom;
	auto target = FindCastTarget(spell_id, arg, ch, &tch, &tobj, &troom);
	if (target && (tch == ch) && MUD::Spell(spell_id).IsViolent()) {
		SendMsgToChar("Лекари не рекомендуют использовать ЭТО на себя!\r\n", ch);
		return;
	}
	if (!target) {
		SendMsgToChar("Тяжеловато найти цель вашего заклинания!\r\n", ch);
		return;
	}
	if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp) &&
		ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		SendMsgToChar("На данной арене вы можете колдовать только временные заклинания!\r\n", ch);
		return;
	}

	ch->SetCast(ESpell::kUndefined, ESpell::kUndefined, nullptr, nullptr, nullptr);
	if (!CalcCastSuccess(ch, tch, ESaving::kStability, spell_id)) {
		if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)))
			SetWaitState(ch, kBattleRound);
		if (GET_SPELL_MEM(ch, substitute_spell_id)) {
			GET_SPELL_MEM(ch, substitute_spell_id)--;
		}
		if (!ch->IsNpc() && !IS_IMMORTAL(ch) && ch->IsFlagged(EPrf::kAutomem)) {
			MemQ_remember(ch, substitute_spell_id);
		}
		affect_total(ch);
		if (!tch || !SendSkillMessages(0, ch, tch, spell_id)) {
			SendMsgToChar("Вы не смогли сосредоточиться!\r\n", ch);
		}
	} else {
		if (ch->GetEnemy() && !IS_IMPL(ch)) {
			ch->SetCast(spell_id, substitute_spell_id, tch, tobj, troom);
			sprintf(buf, "Вы приготовились применить заклинание %s'%s'%s%s.\r\n",
					kColorCyn, MUD::Spell(spell_id).GetCName(), kColorNrm,
					tch == ch ? " на себя" : tch ? " на $N3" : tobj ? " на $o3" : troom ? " на всех" : "");
			act(buf, false, ch, tobj, tch, kToChar);
		} else if (CastSpell(ch, tch, tobj, troom, spell_id, substitute_spell_id) >= 0) {
			if (!(IS_IMMORTAL(ch) || ch->get_wait() > 0))
				SetWaitState(ch, kBattleRound);
		} else if (ch->get_wait() == 0)
			SetWaitState(ch, 1);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
