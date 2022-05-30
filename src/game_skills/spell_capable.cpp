#include "game_classes/classes_spell_slots.h"
#include "handler.h"
#include "game_magic/spells_info.h"
#include "structs/global_objects.h"

// Вложить закл в клона
void DoSpellCapable(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	using classes::CalcCircleSlotsAmount;

	struct TimedFeat timed;

	if (!IS_IMPL(ch) && (ch->IsNpc() || !CanUseFeat(ch, EFeat::kSpellCapabler))) {
		SendMsgToChar("Вы не столь могущественны.\r\n", ch);
		return;
	}

	if (IsTimedByFeat(ch, EFeat::kSpellCapabler) && !IS_IMPL(ch)) {
		SendMsgToChar("Невозможно использовать это так часто.\r\n", ch);
		return;
	}

	char *s;
	if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		SendMsgToChar("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}

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

	auto spell_id = FixNameAndFindSpellId(s);
	if (spell_id == ESpell::kUndefined) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	const auto spell = MUD::Class(ch->GetClass()).spells[spell_id];
	if ((!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp | ESpellType::kKnow) ||
		GET_REAL_REMORT(ch) < spell.GetMinRemort()) &&
		(GetRealLevel(ch) < kLvlGreatGod) && (!ch->IsNpc())) {
		if (GetRealLevel(ch) < CalcMinSpellLvl(ch, spell_id) ||
			CalcCircleSlotsAmount(ch, spell.GetCircle()) <= 0) {
			SendMsgToChar("Рано еще вам бросаться такими словами!\r\n", ch);
			return;
		} else {
			SendMsgToChar("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
			return;
		}
	}

	if (!GET_SPELL_MEM(ch, spell_id) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
		return;
	}

	FollowerType *k;
	CharData *follower = nullptr;
	for (k = ch->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->follower, EAffect::kCharmed)
			&& k->follower->get_master() == ch
			&& MOB_FLAGGED(k->follower, EMobFlag::kClone)
			&& !IsAffectedBySpell(k->follower, ESpell::kCapable)
			&& ch->in_room == IN_ROOM(k->follower)) {
			follower = k->follower;
			break;
		}
	}
	if (!GET_SPELL_MEM(ch, spell_id) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
		return;
	}

	if (!follower) {
		SendMsgToChar("Хорошо бы найти подходящую цель для этого.\r\n", ch);
		return;
	}

	act("Вы принялись зачаровывать $N3.", false, ch, nullptr, follower, kToChar);
	act("$n принял$u делать какие-то пассы и что-то бормотать в сторону $N3.", false, ch, nullptr, follower, kToRoom);

	GET_SPELL_MEM(ch, spell_id)--;
	if (!ch->IsNpc() && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, EPrf::kAutomem))
		MemQ_remember(ch, spell_id);

	if (!MUD::Spell(spell_id).IsViolent() ||
		!MUD::Spell(spell_id).IsFlagged(kMagDamage) ||
		MUD::Spell(spell_id).IsFlagged(kMagMasses) ||
		MUD::Spell(spell_id).IsFlagged(kMagGroups) ||
		MUD::Spell(spell_id).IsFlagged(kMagAreas)) {
		SendMsgToChar("Вы конечно мастер, но не такой магии.\r\n", ch);
		return;
	}
	// Что-то малопонятное... что фит может делать в качестве идентификатора аффекта? Бред.
	//RemoveAffectFromChar(ch, to_underlying(EFeat::kSpellCapabler));

	timed.feat = EFeat::kSpellCapabler;

	switch (MUD::Class(ch->GetClass()).spells[spell_id].GetCircle()) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5://1-5 слоты кд 4 тика
			timed.time = 4;
			break;
		case 6:
		case 7://6-7 слоты кд 6 тиков
			timed.time = 6;
			break;
		case 8://8 слот кд 10 тиков
			timed.time = 10;
			break;
		case 9://9 слот кд 12 тиков
			timed.time = 12;
			break;
		default://10 слот или тп
			timed.time = 24;
	}
	ImposeTimedFeat(ch, &timed);

	GET_CAST_SUCCESS(follower) = GET_REAL_REMORT(ch) * 4;
	Affect<EApply> af;
	af.type = ESpell::kCapable;
	af.duration = 48;
	if (GET_REAL_REMORT(ch) > 0) {
		af.modifier = GET_REAL_REMORT(ch) * 4;//вешаецо аффект который дает +морт*4 касту
		af.location = EApply::kCastSuccess;
	} else {
		af.modifier = 0;
		af.location = EApply::kNone;
	}
	af.battleflag = 0;
	af.bitvector = 0;
	affect_to_char(follower, af);
	follower->mob_specials.capable_spell = spell_id;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
