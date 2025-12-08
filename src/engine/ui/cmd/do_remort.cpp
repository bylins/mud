//
// Created by Sventovit on 03.09.2024.
//

#include "administration/karma.h"
#include "do_follow.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/handler.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/fight/fight.h"
#include "engine/db/global_objects.h"
#include "gameplay/economics/ext_money.h"

#include <third_party_libs/fmt/include/fmt/format.h>

extern RoomRnum r_frozen_start_room;
const char *remort_msg =
	"  Если вы так настойчивы в желании начать все заново -\r\n" "наберите <перевоплотиться> полностью.\r\n";

void DoRemort(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	int i;
	const char *remort_msg2 = "$n вспыхнул$g ослепительным пламенем и пропал$g!\r\n";

	if (ch->IsNpc() || IS_IMMORTAL(ch)) {
		SendMsgToChar("Вам это, похоже, совсем ни к чему.\r\n", ch);
		return;
	}
	if (ch->get_exp() < GetExpUntilNextLvl(ch, kLvlImmortal) - 1) {
		SendMsgToChar("ЧАВО???\r\n", ch);
		return;
	}
/*	if (Remort::need_torc(ch) && !ch->IsFlagged(EPrf::kCanRemort)) {
		SendMsgToChar(ch,
					  "Вы должны подтвердить свои заслуги, пожертвовав Богам достаточное количество гривен.\r\n"
					  "%s\r\n", Remort::WHERE_TO_REMORT_STR.c_str());
		return;
	}
*/
	if (ch->get_remort() > kMaxRemort) {
		SendMsgToChar("Достигнуто максимальное количество перевоплощений.\r\n", ch);
		return;
	}
	if (NORENTABLE(ch)) {
		SendMsgToChar("Вы не можете перевоплотиться в связи с боевыми действиями.\r\n", ch);
		return;
	}
	if (!subcmd) {
		SendMsgToChar(remort_msg, ch);
		return;
	}

	one_argument(argument, arg);
	int place_of_destination;
	if (!*arg) {
		const auto msg = fmt::format("Укажите, где вы хотите заново начать свой путь:\r\n{}",
									 Birthplaces::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(ch), GET_RACE(ch))));
		SendMsgToChar(msg, ch);
		return;
	} else {
		place_of_destination = Birthplaces::ParseSelect(arg);
		if (place_of_destination == kBirthplaceUndefined) {
			place_of_destination = PlayerRace::CheckBirthPlace(GET_KIN(ch), GET_RACE(ch), arg);
			if (!Birthplaces::CheckId(place_of_destination)) {
				SendMsgToChar("Багдад далече, выберите себе местечко среди родных осин.\r\n", ch);
				return;
			}
		}
	}

	log("Remort %s", GET_NAME(ch));
	ch->remort();
	act(remort_msg2, false, ch, nullptr, nullptr, kToRoom);

	if (ch->is_morphed()) ch->reset_morph();
	ch->set_remort(ch->get_remort() + 1);
	CLR_GOD_FLAG(ch, EGf::kRemort);
	ch->inc_str(1);
	ch->inc_dex(1);
	ch->inc_con(1);
	ch->inc_wis(1);
	ch->inc_int(1);
	ch->inc_cha(1);
	if (ch->GetEnemy()) {
		stop_fighting(ch, true);
	}
	die_follower(ch);
	ch->affected.clear();
// Снимаем весь стафф
	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)) {
			PlaceObjToInventory(UnequipChar(ch, i, CharEquipFlags()), ch);
		}
	}

	while (ch->timed) {
		ExpireTimedSkill(ch, ch->timed);
	}

	while (ch->timed_feat) {
		ExpireTimedFeat(ch, ch->timed_feat);
	}
	for (const auto &feat : MUD::Feats()) {
		ch->UnsetFeat(feat.GetId());
	}

	if (ch->get_remort() >= 9 && ch->get_remort() % 3 == 0) {
		ch->clear_skills();
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_TYPE(ch, spell_id) = (IS_MANA_CASTER(ch) ? ESpellType::kRunes : 0);
			GET_SPELL_MEM(ch, spell_id) = 0;
		}
	} else {
		ch->SetSkillAfterRemort();
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (IS_MANA_CASTER(ch)) {
				GET_SPELL_TYPE(ch, spell_id) = ESpellType::kRunes;
			} else if (MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() >= 8) {
				GET_SPELL_TYPE(ch, spell_id) = ESpellType::kUnknowm;
				GET_SPELL_MEM(ch, spell_id) = 0;
			}
		}
	}

	ch->set_max_hit(10);
		ch->set_hit(10);
	ch->set_max_move(82);
	ch->set_move(82);
	ch->mem_queue.total = ch->mem_queue.stored = 0;
	ch->set_level(0);
	GET_WIMP_LEV(ch) = 0;
	GET_AC(ch) = 100;
	GET_LOADROOM(ch) = calc_loadroom(ch, place_of_destination);
	ch->UnsetFlag(EPrf::KSummonable);
	ch->UnsetFlag(EPrf::kAwake);
	ch->UnsetFlag(EPrf::kPunctual);
	ch->UnsetFlag(EPrf::kPerformPowerAttack);
	ch->UnsetFlag(EPrf::kPerformGreatPowerAttack);
	ch->UnsetFlag(EPrf::kAwake);
	ch->UnsetFlag(EPrf::kIronWind);
	ch->UnsetFlag(EPrf::kDoubleThrow);
	ch->UnsetFlag(EPrf::kTripleThrow);
	ch->UnsetFlag(EPrf::kShadowThrow);
	ch->DeleteIrrelevantRunestones();
	if (ch->get_protecting()) {
		ch->remove_protecting();
	}

	ch->ClearThisRemortStatistics();

	// забываем всех чармисов
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	if (!m->empty()) {
		auto it = m->begin();
		for (; it != m->end(); ++it) {
			it->second.currRemortAvail = 0;
		}
	}
	DoPcInit(ch, false);
	ch->save_char();
	RoomRnum load_room;
	if (ch->IsFlagged(EPlrFlag::kHelled))
		load_room = r_helled_start_room;
	else if (ch->IsFlagged(EPlrFlag::kNameDenied))
		load_room = r_named_start_room;
	else if (ch->IsFlagged(EPlrFlag::kFrozen))
		load_room = r_frozen_start_room;
	else {
		if ((load_room = GET_LOADROOM(ch)) == kNowhere)
			load_room = calc_loadroom(ch);
		load_room = GetRoomRnum(load_room);
	}
	if (load_room == kNowhere) {
		if (GetRealLevel(ch) >= kLvlImmortal)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, load_room);
	look_at_room(ch, 0);
	ch->SetFlag(EPlrFlag::kNoDelete);
	room_spells::RemoveSingleAffectFromWorld(ch, ESpell::kRuneLabel);

	// сброс всего, связанного с гривнами (замакс сохраняем)
	ch->UnsetFlag(EPrf::kCanRemort);
	ch->set_ext_money(ExtMoney::kTorcGold, 0);
	ch->set_ext_money(ExtMoney::kTorcSilver, 0);
	ch->set_ext_money(ExtMoney::kTorcBronze, 0);

	snprintf(buf, sizeof(buf),
			 "remort from %d to %d", ch->get_remort() - 1, ch->get_remort());
	snprintf(buf2, sizeof(buf2), "dest=%d", place_of_destination);
	AddKarma(ch, buf, buf2);

	act("$n вступил$g в игру.", true, ch, nullptr, nullptr, kToRoom);
	act("Вы перевоплотились! Желаем удачи!", false, ch, nullptr, nullptr, kToChar);
	affect_total(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
