/* ************************************************************************
*   File: spells.cpp                                    Part of Bylins    *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "spells.h"
#include "engine/db/global_objects.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/cmd/do_hire.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/ai/mobact.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "magic.h"
#include "gameplay/skills/animal_master.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/communication/parcel.h"
#include "administration/privilege.h"
#include "engine/ui/color.h"
#include "engine/ui/cmd/do_flee.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "gameplay/mechanics/stuff.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/minions.h"

#include <cmath>

extern char cast_argument[kMaxInputLength];
extern im_type *imtypes;
extern int top_imtypes;

void weight_change_object(ObjData *obj, int weight);
int CalcBaseAc(CharData *ch);
char *diag_weapon_to_char(const CObjectPrototype *obj, int show_wear);
void SetPrecipitations(int *wtype, int startvalue, int chance1, int chance2, int chance3);
int CalcAntiSavings(CharData *ch);
void do_tell(CharData *ch, char *argument, int cmd, int subcmd);
void RemoveEquipment(CharData *ch, int pos);
int pk_action_type_summon(CharData *agressor, CharData *victim);
int pk_increment_revenge(CharData *agressor, CharData *victim);

int what_sky = kSkyCloudless;
// * Special spells appear below.

ESkill GetMagicSkillId(ESpell spell_id) {
	switch (MUD::Spell(spell_id).GetElement()) {
		case EElement::kAir: return ESkill::kAirMagic;
			break;
		case EElement::kFire: return ESkill::kFireMagic;
			break;
		case EElement::kWater: return ESkill::kWaterMagic;
			break;
		case EElement::kEarth: return ESkill::kEarthMagic;
			break;
		case EElement::kLight: return ESkill::kLightMagic;
			break;
		case EElement::kDark: return ESkill::kDarkMagic;
			break;
		case EElement::kMind: return ESkill::kMindMagic;
			break;
		case EElement::kLife: return ESkill::kLifeMagic;
			break;
		case EElement::kUndefined: [[fallthrough]];
		default: return ESkill::kUndefined;
	}
}

//Определим мин уровень для изучения спелла из книги
//req_lvl - требуемый уровень из книги
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id, int req_lvl) {
	int min_lvl = std::max(req_lvl, MUD::Class(ch->GetClass()).spells[spell_id].GetMinLevel())
		- (std::max(0, GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetSpellLvlDecrement()));

	return std::max(1, min_lvl);
}

int CalcMinSpellLvl(const CharData *ch, ESpell spell_id) {
	auto min_lvl = MUD::Class(ch->GetClass()).spells[spell_id].GetMinLevel()
		- GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetSpellLvlDecrement();

	return std::max(1, min_lvl);
}

int CalcMinRuneSpellLvl(const CharData *ch, ESpell spell_id) {
	int min_lvl;

	// (issue.runes-migrate) Read from the new rune_spells registry.
	const auto &runes = MUD::RuneSpells();
	if (auto it = runes.find(spell_id); it != runes.end()) {
		min_lvl = it->second.min_caster_level - GetRealRemort(ch) / MUD::Class(ch->GetClass()).GetSpellLvlDecrement();
	} else {
		return 999;
	}
	return std::max(1, min_lvl);
}

bool CanGetSpell(const CharData *ch, ESpell spell_id, int req_lvl) {
	if (MUD::Class(ch->GetClass()).spells.IsUnavailable(spell_id) ||
		CalcMinSpellLvl(ch, spell_id, req_lvl) > GetRealLevel(ch) ||
		MUD::Class(ch->GetClass()).spells[spell_id].GetMinRemort() > GetRealRemort(ch)) {
		return false;
	}

	return true;
};

// Функция определяет возможность изучения спелла из книги или в гильдии
bool CanGetSpell(CharData *ch, ESpell spell_id) {
	if (MUD::Class(ch->GetClass()).spells.IsUnavailable(spell_id)) {
		return false;
	}

	if (CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) ||
		MUD::Class(ch->GetClass()).spells[spell_id].GetMinRemort() > GetRealRemort(ch)) {
		return false;
	}

	return true;
};

typedef std::map<EIngredientFlag, std::string> EIngredientFlag_name_by_value_t;
typedef std::map<const std::string, EIngredientFlag> EIngredientFlag_value_by_name_t;
EIngredientFlag_name_by_value_t EIngredientFlag_name_by_value;
EIngredientFlag_value_by_name_t EIngredientFlag_value_by_name;

void init_EIngredientFlag_ITEM_NAMES() {
	EIngredientFlag_name_by_value.clear();
	EIngredientFlag_value_by_name.clear();

	EIngredientFlag_name_by_value[EIngredientFlag::kItemRunes] = "kItemRunes";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckUses] = "kItemCheckUses";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckLag] = "kItemCheckLag";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckLevel] = "kItemCheckLevel";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemDecayEmpty] = "kItemDecayEmpty";

	for (const auto &i : EIngredientFlag_name_by_value) {
		EIngredientFlag_value_by_name[i.second] = i.first;
	}
}

template<>
EIngredientFlag ITEM_BY_NAME(const std::string &name) {
	if (EIngredientFlag_name_by_value.empty()) {
		init_EIngredientFlag_ITEM_NAMES();
	}
	return EIngredientFlag_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<EIngredientFlag>(const EIngredientFlag item) {
	if (EIngredientFlag_name_by_value.empty()) {
		init_EIngredientFlag_ITEM_NAMES();
	}
	return EIngredientFlag_name_by_value.at(item);
}

void SpellCreateWater(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	int water;
	if (ch == nullptr || (obj == nullptr && victim == nullptr))
		return;
	// level = MAX(MIN(level, kLevelImplementator), 1);       - not used

	if (obj
		&& obj->get_type() == EObjType::kLiquidContainer) {
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
			// kCreateWater overrides kItemCreationFailToChar
			// with "Прекратите, ради бога, химичить.".
			SendMsgToChar(MUD::SpellMessages().GetMessage(
					ESpell::kCreateWater, ESpellMsg::kItemCreationFailToChar) + "\r\n", ch);
			return;
		} else {
			water = std::max(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0) {
				if (GET_OBJ_VAL(obj, 1) >= 0) {
					name_from_drinkcon(obj);
				}
				obj->set_val(2, LIQ_WATER);
				obj->add_val(1, water);
				// kCreateWater overrides the generic "Вы создали $o3." (kItemCreatedToChar)
				// with "Вы наполнили $o3 водой.".
				const auto &filled_msg = MUD::SpellMessages().GetMessage(
						ESpell::kCreateWater, ESpellMsg::kItemCreatedToChar);
				act(filled_msg.c_str(), false, ch, obj, nullptr, kToChar);
				name_to_drinkcon(obj, LIQ_WATER);
				weight_change_object(obj, water);
			}
		}
	}
	if (victim && !victim->IsNpc() && !victim->IsImmortal()) {
		GET_COND(victim, THIRST) = 0;
		// kCreateWater overrides kThirstToVict with "Вы полностью утолили жажду."
		// (literal text, no {intensity} expansion -- the manual path bypasses
		// CastToPoints' intensity machinery).
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kCreateWater, ESpellMsg::kThirstToVict) + "\r\n", victim);
		// The redundant "Вы напоили $N3." line to the caster was removed
		// the caster has no way to gauge the target's
		// prior thirst level, so the line conveys no real info.
	}
}

// Look up kSummonFail in `spell_id`'s sheaf (per-spell override on each summon-
// style spell, with kDefault random-variant fallback) and emit to the caster.
static void SendSummonFail(CharData *ch, ESpell spell_id) {
	SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
}
/*
#define MIN_NEWBIE_ZONE  20
#define MAX_NEWBIE_ZONE  79
#define MAX_SUMMON_TRIES 2000
*/

// Поиск комнаты для перемещающего заклинания
// ch - кого перемещают, rnum_start - первая комната диапазона, rnum_stop - последняя комната диапазона
int GetTeleportTargetRoom(CharData *ch, int rnum_start, int rnum_stop) {
	int *r_array;
	int n, i, j;
	int fnd_room = kNowhere;

	n = rnum_stop - rnum_start + 1;

	if (n <= 0) {
		return kNowhere;
	}

	r_array = (int *) malloc(n * sizeof(int));
	for (i = 0; i < n; ++i)
		r_array[i] = rnum_start + i;

	for (; n; --n) {
		j = number(0, n - 1);
		fnd_room = r_array[j];
		r_array[j] = r_array[n - 1];

		if (SECT(fnd_room) != ESector::kSecret &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kNoTeleportIn) &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) &&
			(!ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) || ch->IsImmortal()) &&
			Clan::MayEnter(ch, fnd_room, kHousePortal))
			break;
	}

	free(r_array);

	return n ? fnd_room : kNowhere;
}

void SpellRecall(CharData *ch, CharData *victim) {
	RoomRnum to_room = kNowhere, fnd_room = kNowhere;
	RoomRnum rnum_start, rnum_stop;

	if (!victim || victim->IsNpc() || ch->in_room != victim->in_room || GetRealLevel(victim) >= kLvlImmortal) {
		SendSummonFail(ch, ESpell::kWorldOfRecall);
		return;
	}

	// kNoTeleportOut moved to <blocking><room_flags> in spells.xml; CallMagic
	// fizzles before this function runs.
	if (!ch->IsGod() && AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
		SendSummonFail(ch, ESpell::kWorldOfRecall);
		return;
	}

	if (victim != ch) {
		if (group::same_group(ch, victim)) {
			if (number(1, 100) <= 5) {
				SendSummonFail(ch, ESpell::kWorldOfRecall);
				return;
			}
		} else if (!ch->IsNpc() || (ch->has_master()
			&& !ch->get_master()->IsNpc())) // игроки не в группе и  чармисы по приказу не могут реколить свитком
		{
			SendSummonFail(ch, ESpell::kWorldOfRecall);
			return;
		}

		if ((ch->IsNpc() && CalcGeneralSaving(ch, victim, ESaving::kWill, GetRealInt(ch))) || victim->IsGod()) {
			return;
		}
	}

	if ((to_room = GetRoomRnum(GET_LOADROOM(victim))) == kNowhere)
		to_room = GetRoomRnum(calc_loadroom(victim));

	if (to_room == kNowhere) {
		SendSummonFail(ch, ESpell::kWorldOfRecall);
		return;
	}

	(void) GetZoneRooms(world[to_room]->zone_rn, &rnum_start, &rnum_stop);
	fnd_room = GetTeleportTargetRoom(victim, rnum_start, rnum_stop);
	if (fnd_room == kNowhere) {
		to_room = Clan::CloseRent(to_room);
		(void) GetZoneRooms(world[to_room]->zone_rn, &rnum_start, &rnum_stop);
		fnd_room = GetTeleportTargetRoom(victim, rnum_start, rnum_stop);
	}

	if (fnd_room == kNowhere) {
		SendSummonFail(ch, ESpell::kWorldOfRecall);
		return;
	}

	if (victim->GetEnemy() && (victim != ch)) {
		if (!pk_agro_action(ch, victim->GetEnemy()))
			return;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return;
	// kWorldOfRecall overrides the kCastDisappearToRoom /
	// kCastAppearToRoom defaults with its specific "centre of room" wording.
	act(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kCastDisappearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	RemoveCharFromRoom(victim);
	PlaceCharToRoom(victim, fnd_room);
	victim->dismount();
	act(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kCastAppearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom);
	look_at_room(victim, 0);
	greet_mtrigger(victim, -1);
	greet_otrigger(victim, -1);
}

// ПРЫЖОК в рамках зоны
void SpellTeleport(CharData *ch, CharData */*victim*/) {
	RoomRnum in_room = ch->in_room, fnd_room = kNowhere;
	RoomRnum rnum_start, rnum_stop;

	// kNoTeleportOut moved to <blocking><room_flags> in spells.xml; CallMagic
	// fizzles before this function runs.
	if (!ch->IsGod() && AFF_FLAGGED(ch, EAffect::kNoTeleport)) {
		SendSummonFail(ch, ESpell::kTeleport);
		return;
	}

	GetZoneRooms(world[in_room]->zone_rn, &rnum_start, &rnum_stop);
	fnd_room = GetTeleportTargetRoom(ch, rnum_start, rnum_stop);
	if (fnd_room == kNowhere) {
		SendSummonFail(ch, ESpell::kTeleport);
		return;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return;
	// kTeleport overrides kCastDisappearToRoom / kCastAppearToRoom.
	act(MUD::SpellMessages().GetMessage(ESpell::kTeleport, ESpellMsg::kCastDisappearToRoom).c_str(),
		false, ch, nullptr, nullptr, kToRoom);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, fnd_room);
	ch->dismount();
	act(MUD::SpellMessages().GetMessage(ESpell::kTeleport, ESpellMsg::kCastAppearToRoom).c_str(),
		false, ch, nullptr, nullptr, kToRoom);
	look_at_room(ch, 0);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

void CheckAutoNosummon(CharData *ch) {
	if (ch->IsFlagged(EPrf::kAutonosummon) && ch->IsFlagged(EPrf::KSummonable)) {
		ch->UnsetFlag(EPrf::KSummonable);
		SendMsgToChar("Режим автопризыв: вы защищены от призыва.\r\n", ch);
	}
}

void SpellRelocate(CharData *ch, CharData *victim) {
	RoomRnum to_room, fnd_room;

	if (victim == nullptr)
		return;

	// kNoTeleportOut moved to <blocking><room_flags> in spells.xml; CallMagic
	// fizzles before this function runs.
	if (!ch->IsGod() && AFF_FLAGGED(ch, EAffect::kNoTeleport)) {
		SendSummonFail(ch, ESpell::kRelocate);
		return;
	}

	to_room = victim->in_room;

	if (to_room == kNowhere) {
		SendSummonFail(ch, ESpell::kRelocate);
		return;
	}

	if (!Clan::MayEnter(ch, to_room, kHousePortal)) {
		fnd_room = Clan::CloseRent(to_room);
	} else {
		fnd_room = to_room;
	}

	if (fnd_room != to_room && !ch->IsGod()) {
		SendSummonFail(ch, ESpell::kRelocate);
		return;
	}

	if (!ch->IsGod() &&
		(SECT(fnd_room) == ESector::kSecret ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kNoRelocateIn) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) || (ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) && !ch->IsImmortal()))) {
		SendSummonFail(ch, ESpell::kRelocate);
		return;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return;
//	check_auto_nosummon(victim);
	// kRelocate shares the kTeleport disappear/appear wording
	// and adds its own kCustomMsgOne caster-side "azure flash" banner.
	act(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kCastDisappearToRoom).c_str(),
		true, ch, nullptr, nullptr, kToRoom);
	SendMsgToChar(MUD::SpellMessages().GetMessage(
			ESpell::kRelocate, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, fnd_room);
	ch->dismount();
	look_at_room(ch, 0);
	act(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kCastAppearToRoom).c_str(),
		true, ch, nullptr, nullptr, kToRoom);
	SetBattleLag(ch, 2);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

// pk_unique: when non-zero, marks this portal as a PK-revenge/fight pentagram and
// carries the imposing caster's uid. Replaces the old RoomData::pkPenterUnique
// (which was per-room, ambiguous when multiple pentas land in one room, and had
// to be cleared by hand). Read by show_room_affects (Pk variant selection) and
// do_enter (entry gate); cleared automatically when the affect expires.
void AddPortalTimer(CharData *ch, RoomData *from_room, RoomRnum to_room, int time,
					long pk_unique = 0) {
//	sprintf(buf, "Добавляем портал из %d в %d", from_room->vnum, world[to_room]->vnum);
//	mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);

	Affect<room_spells::ERoomApply> af;
	af.type = ESpell::kPortalTimer;
	af.affect_type = static_cast<room_spells::ERoomAffect>(0);
	af.duration = time; //раз в 2 секунды
	af.modifier = to_room;
	af.battleflag = 0;
	af.location = room_spells::ERoomApply::kNone;
	af.caster_id = ch? ch->get_uid() : 0;
	af.apply_time = 0;
	af.pk_unique = pk_unique;
	room_spells::affect_to_room(from_room, af);
	room_spells::AddRoomToAffected(from_room);
}

void RemovePortalGate(RoomRnum rnum) {
	auto aff = room_spells::FindAffect(world[rnum], ESpell::kPortalTimer);
	const RoomRnum to_room = (*aff)->modifier;
	// kPortal sheaf kCustomMsgThree holds "Пентаграмма была
	// разрушена." -- emitted to both char and room of each affected portal endpoint.
	const auto &broken_msg = MUD::SpellMessages().GetMessage(
			ESpell::kPortal, ESpellMsg::kCustomMsgThree);

	if (aff != world[rnum]->affected.end()) {
		room_spells::RoomRemoveAffect(world[rnum], aff);
		act(broken_msg.c_str(), false, world[rnum]->first_character(), 0, 0, kToRoom);
		act(broken_msg.c_str(), false, world[rnum]->first_character(), 0, 0, kToChar);
	}
	aff = room_spells::FindAffect(world[to_room], ESpell::kPortalTimer);
	if (aff != world[to_room]->affected.end()) {
		room_spells::RoomRemoveAffect(world[to_room], aff);
		act(broken_msg.c_str(), false, world[to_room]->first_character(), 0, 0, kToRoom);
		act(broken_msg.c_str(), false, world[to_room]->first_character(), 0, 0, kToChar);
	}
}

void SpellPortal(CharData *ch, CharData *victim) {
	RoomRnum fnd_room;

	if (victim == nullptr)
		return;
	if (GetRealLevel(victim) > GetRealLevel(ch) && !victim->IsFlagged(EPrf::KSummonable) && !group::same_group(ch, victim)) {
		SendSummonFail(ch, ESpell::kPortal);
		return;
	}
	// пентить чаров <=10 уровня, нельзя так-же нельзя пентать иммов
	if (!ch->IsGod()) {
		if ((!victim->IsNpc() && GetRealLevel(victim) <= 10 && GetRealRemort(ch) < 9) || victim->IsImmortal()
			|| AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
			SendSummonFail(ch, ESpell::kPortal);
			return;
		}
	}
	if (victim->IsNpc()) {
		SendSummonFail(ch, ESpell::kPortal);
		return;
	}
	fnd_room = victim->in_room;
	if (fnd_room == kNowhere) {
		SendSummonFail(ch, ESpell::kPortal);
		return;
	}

	if (!ch->IsGod() && (SECT(fnd_room) == ESector::kSecret || ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) || ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) || ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom))) {
		SendSummonFail(ch, ESpell::kPortal);
		return;
	}

	if (ch->in_room == fnd_room) {
		SendMsgToChar("Может, вам лучше просто потоптаться на месте?\r\n", ch);
		return;
	}

	bool pkPortal = pk_action_type_summon(ch, victim) == PK_ACTION_REVENGE ||
		pk_action_type_summon(ch, victim) == PK_ACTION_FIGHT;

	if (ch->IsImmortal() || GET_GOD_FLAG(victim, EGf::kGodscurse)
		// раньше было <= PK_ACTION_REVENGE, что вызывало абьюз при пенте на чара на арене,
		// или пенте кидаемой с арены т.к. в данном случае использовалось PK_ACTION_NO которое меньше PK_ACTION_REVENGE
		|| pkPortal || ((!victim->IsNpc() || IS_CHARMICE(ch)) && victim->IsFlagged(EPrf::KSummonable))
		|| group::same_group(ch, victim)) {
		if (pkPortal) {
			pk_increment_revenge(ch, victim);
		}
		if (room_spells::IsRoomAffected(world[ch->in_room], ESpell::kPortalTimer)) {
			bool remove = false;
			for (const auto &aff : world[ch->in_room]->affected) {
				if (aff->type == ESpell::kPortalTimer ) {
					if (aff->caster_id == ch->get_uid() && aff->modifier == fnd_room) {
						remove = true;
						break;
					}
				}
			}
			if (remove)
				RemovePortalGate(ch->in_room);
		}
		// pk_unique on the affect replaces the old per-room pkPenterUnique field
		// pkPortal => imposing caster's uid; else 0.
		AddPortalTimer(ch, world[fnd_room], ch->in_room, 29, pkPortal ? ch->get_uid() : 0);

		// pentagram-appearance narration lives in kPortal's
		// sheaf -- kCustomMsgOne is the normal line, kCustomMsgTwo is the pk variant
		// (blood-tinged). Each is sent to both kToChar and kToRoom of the destination
		// endpoint's first occupant.
		const auto &dest_pentagram = MUD::SpellMessages().GetMessage(
				ESpell::kPortal, pkPortal ? ESpellMsg::kCustomMsgTwo : ESpellMsg::kCustomMsgOne);
		act(dest_pentagram.c_str(), false, world[fnd_room]->first_character(), nullptr, nullptr, kToChar);
		act(dest_pentagram.c_str(), false, world[fnd_room]->first_character(), nullptr, nullptr, kToRoom);
		CheckAutoNosummon(victim);

		// если пенту ставит имм с привилегией arena (и находясь на арене), то пента получается односторонняя
		if (privilege::CheckFlag(ch, privilege::kArenaMaster) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)) {
			return;
		}

		AddPortalTimer(ch, world[ch->in_room], fnd_room, 29, pkPortal ? ch->get_uid() : 0);

		// Caster-side pentagram (same key resolution as the destination side above).
		const auto &caster_pentagram = MUD::SpellMessages().GetMessage(
				ESpell::kPortal, pkPortal ? ESpellMsg::kCustomMsgTwo : ESpellMsg::kCustomMsgOne);
		act(caster_pentagram.c_str(), false, world[ch->in_room]->first_character(), nullptr, nullptr, kToChar);
		act(caster_pentagram.c_str(), false, world[ch->in_room]->first_character(), nullptr, nullptr, kToRoom);
	}
}

// SpellSummon follow-up: relocate any charmice in the summoned victim's
// old room to the caster's room with the same disappear/appear narration
// used for the main victim. Each charmice is sent a "you were summoned"
// notification via kCustomMsgTwo.
static void SummonFollowingCharmices(CharData *ch, CharData *victim, RoomRnum vic_room, RoomRnum ch_room) {
// призываем чармисов
for (auto *k : victim->followers) {
	if (k->in_room == vic_room) {
		if (AFF_FLAGGED(k, EAffect::kCharmed)) {
			if (!k->GetEnemy()) {
				// Charmice reuses kSummon's disappear/appear keys but with the
				// kCustomMsgThree "arrived following the master" line on arrival.
				act(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCastDisappearToRoom).c_str(),
					true, k, nullptr, nullptr, kToRoom | kToArenaListen);
				RemoveCharFromRoom(k);
				PlaceCharToRoom(k, ch_room);
				act(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCustomMsgThree).c_str(),
					true, k, nullptr, nullptr, kToRoom | kToArenaListen);
				act(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCustomMsgTwo).c_str(),
					false, ch, nullptr, k, kToVict);
			}
		}
	}
}
}

void SpellSummon(CharData *ch, CharData *victim) {
	RoomRnum ch_room, vic_room;

	if (ch == nullptr || victim == nullptr || ch == victim) {
		return;
	}
	if (!victim->desc) {
		SendSummonFail(ch, ESpell::kSummon);
	}
	ch_room = ch->in_room;
	vic_room = victim->in_room;

	if (ch_room == kNowhere || vic_room == kNowhere) {
		SendSummonFail(ch, ESpell::kSummon);
		ch->send_to_TC(true, true, true, "Цель в Nowhere\r\n");
		return;
	}

	if (ch->IsNpc() && victim->IsNpc()) {
		ch->send_to_TC(true, true, true, "Да ты МОБ!!!!!\r\n");
		SendSummonFail(ch, ESpell::kSummon);
		return;
	}

	if (victim->IsImmortal()) {
		if (ch->IsNpc() || (!ch->IsNpc() && GetRealLevel(ch) < GetRealLevel(victim))) {
			ch->send_to_TC(true, true, true, "Неположено сие деяние!\r\n");
			SendSummonFail(ch, ESpell::kSummon);
			return;
		}
	}

	if (!ch->IsNpc() && victim->IsNpc()) {
		if (victim->get_master() != ch) {
			ch->send_to_TC(true, true, true, "Чармис не ваш\r\n");
			SendSummonFail(ch, ESpell::kSummon);
			return;
		}
	}

	if (!ch->IsImmortal()) {
		if (!ch->IsNpc() || IS_CHARMICE(ch)) {
			if (AFF_FLAGGED(ch, EAffect::kGodsShield)) {
				ch->send_to_TC(true, true, true, "Чармис под зб\r\n");
				SendMsgToChar(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kResurrectProtected) + "\r\n", ch);
				return;
			}
			if (!victim->IsFlagged(EPrf::KSummonable) && !group::same_group(ch, victim)) {
				ch->send_to_TC(true, true, true, "Чармис не в вашей группе\r\n");
				SendMsgToChar(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kResurrectNoPower) + "\r\n", ch);
				return;
			}
			if (NORENTABLE(victim) && !IS_CHARMICE(ch)) {
				ch->send_to_TC(true, true, true, "Ваша жертва совсем не рентабельна!\r\n");
				SendSummonFail(ch, ESpell::kSummon);
				return;
			}
			if (victim->GetEnemy()
				|| victim->GetPosition() < EPosition::kRest) {
				ch->send_to_TC(true, true, true, "Чармис сражается или дрыхнет\r\n");
				SendMsgToChar(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCustomMsgFour) + "\r\n", ch);
				return;
			}
		}
		if (victim->get_wait() > 0) {
			ch->send_to_TC(true, true, true, "Чармис в лаге\r\n");
			SendSummonFail(ch, ESpell::kSummon);
			return;
		}

		if (ROOM_FLAGGED(ch_room, ERoomFlag::kNoSummonOut)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kDeathTrap)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kSlowDeathTrap)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kTunnel)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kNoBattle)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kGodsRoom)
			|| SECT(ch->in_room) == ESector::kSecret
			|| (!group::same_group(ch, victim)
				&& (ROOM_FLAGGED(ch_room, ERoomFlag::kPeaceful) || ROOM_FLAGGED(ch_room, ERoomFlag::kArena)))) {
			ch->send_to_TC(true, true, true, "Чармис в носуммоне\r\n");
			SendSummonFail(ch, ESpell::kSummon);
			return;
		}
		// отдельно проверку на клан комнаты, своих чармисов призвать можем ()
		if (!Clan::MayEnter(victim, ch_room, kHousePortal) && !(victim->has_master()) && (victim->get_master() != ch)) {
			ch->send_to_TC(true, true, true, "Чармис доступ в замок запрещен\r\n");
			SendSummonFail(ch, ESpell::kSummon);
			return;
		}

		if (!ch->IsNpc()) {
			if (ROOM_FLAGGED(vic_room, ERoomFlag::kNoSummonOut)
				|| ROOM_FLAGGED(vic_room, ERoomFlag::kGodsRoom)
				|| !Clan::MayEnter(ch, vic_room, kHousePortal)
				|| AFF_FLAGGED(victim, EAffect::kNoTeleport)
				|| (!group::same_group(ch, victim)
					&& (ROOM_FLAGGED(vic_room, ERoomFlag::kTunnel) || ROOM_FLAGGED(vic_room, ERoomFlag::kArena)))) {
				ch->send_to_TC(true, true, true, "Чармис в носуммоне\r\n");
				SendSummonFail(ch, ESpell::kSummon);
				return;
			}
		} else {
			if (ROOM_FLAGGED(vic_room, ERoomFlag::kNoSummonOut) || AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
				// block notice on kSummon's sheaf as kCustomMsgOne.
				SendMsgToChar(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
				return;
			}
		}

		if (ch->IsNpc() && number(1, 100) < 30) {
			return;
		}
	}
	if (!enter_wtrigger(world[ch_room], ch, -1)) {
		ch->send_to_TC(true, true, true, "Чармис призыв запрещен триггером\r\n");
		return;
	}
	// kSummon overrides kCastDisappearToRoom (vic disappearing
	// from old room) and kCastAppearToRoom (vic arriving in caster's room). kCustomMsgTwo
	// is the to-vict notification.
	act(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kCastDisappearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	RemoveCharFromRoom(victim);
	PlaceCharToRoom(victim, ch_room);
	CheckAutoNosummon(victim);
	victim->SetPosition(EPosition::kStand);
	act(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kCastAppearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	act(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kCustomMsgTwo).c_str(),
		false, ch, nullptr, victim, kToVict);
	victim->dismount();
	look_at_room(victim, 0);
	SummonFollowingCharmices(ch, victim, vic_room, ch_room);
	greet_mtrigger(victim, -1);
	greet_otrigger(victim, -1);
}

void SpellLocateObject(int level, CharData *ch, CharData* /*victim*/, ObjData *obj) {
	/*
	   * FIXME: This is broken.  The spell parser routines took the argument
	   * the player gave to the spell and located an object with that keyword.
	   * Since we're passed the object and not the keyword we can only guess
	   * at what the player originally meant to search for. -gg
	   */
	if (!obj) {
		return;
	}

	char name[kMaxInputLength];
	bool bloody_corpse = false;
	strcpy(name, cast_argument);

	int tmp_lvl = (ch->IsGod()) ? 300 : level;
	int count = tmp_lvl;
	const auto result = world_objects.find_if_and_dec_number([ch, name, &bloody_corpse](const ObjData::shared_ptr &i) {
		const auto obj_ptr = world_objects.get_by_raw_ptr(i.get());
		if (!obj_ptr) {
			sprintf(buf, "SYSERR: Illegal object iterator while locate");
			mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);

			return false;
		}

		bloody_corpse = false;
		if (!ch->IsGod()) {
			if (number(1, 100) > (40 + std::max((GetRealInt(ch) - 25) * 2, 0))) {
				return false;
			}

			if (IS_CORPSE(i)) {
				bloody_corpse = CatchBloodyCorpse(i.get());
				if (!bloody_corpse) {
					return false;
				}
			}
		}

		if (i->has_flag(EObjFlag::kNolocate) && i->get_carried_by() != ch) {
			// !локейт стаф может локейтить только имм или тот кто его держит
			return false;
		}

		if (SECT(i->get_in_room()) == ESector::kSecret) {
			return false;
		}

		if (i->get_carried_by()) {
			const auto carried_by = i->get_carried_by();
			const auto carried_by_ptr = character_list.get_character_by_address(carried_by);

			if (!carried_by_ptr) {
				sprintf(buf, "SYSERR: Illegal carried_by ptr. Создана кора для исследований");
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
				return false;
			}

			if (!ValidRnum(carried_by->in_room)) {
				sprintf(buf,
						"SYSERR: Illegal room %d, char %s. Создана кора для исследований",
						carried_by->in_room,
						carried_by->get_name().c_str());
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
				return false;
			}

			if (SECT(carried_by->in_room) == ESector::kSecret || carried_by->IsImmortal()) {
				return false;
			}
		}

		if (!isname(name, i->get_aliases())) {
			return false;
		}
		std::string locate_msg;

		if (i->get_carried_by()) {
			const auto carried_by = i->get_carried_by();
			const auto same_zone = world[ch->in_room]->zone_rn == world[carried_by->in_room]->zone_rn;
			if (!carried_by->IsNpc() || same_zone || bloody_corpse) {
				sprintf(buf, "%s наход%sся у %s в инвентаре.\r\n", i->get_short_description().c_str(),
						GET_OBJ_POLY_1(ch, i), PERS(carried_by, ch, 1));
			} else {
				return false;
			}
		} else if (i->get_in_room() != kNowhere && i->get_in_room()) {
			const auto room = i->get_in_room();
			const auto same_zone = world[ch->in_room]->zone_rn == world[room]->zone_rn;
			if (same_zone) {
				sprintf(buf, "%s наход%sся в комнате '%s'\r\n",
						i->get_short_description().c_str(), GET_OBJ_POLY_1(ch, i), world[room]->name);
			} else {
				return false;
			}
		} else if (i->get_in_obj()) {
			if (Clan::is_clan_chest(i->get_in_obj())) {
				return false; // шоб не забивало локейт на мобах/плеерах - по кланам проходим ниже отдельно
			} else {
				if (!ch->IsGod()) {
					if (i->get_in_obj()->get_carried_by()) {
						if (i->get_in_obj()->get_carried_by()->IsNpc() && i->has_flag(EObjFlag::kNolocate)) {
							return false;
						}
					}
					if (i->get_in_obj()->get_in_room() != kNowhere
						&& i->get_in_obj()->get_in_room()) {
						if (i->has_flag(EObjFlag::kNolocate) && !bloody_corpse) {
							return false;
						}
					}
					if (i->get_in_obj()->get_worn_by()) {
						const auto worn_by = i->get_in_obj()->get_worn_by();
						if (worn_by->IsNpc() && i->has_flag(EObjFlag::kNolocate) && !bloody_corpse) {
							return false;
						}
					}
				}
				sprintf(buf, "%s наход%sся в %s.\r\n",
						i->get_short_description().c_str(),
						GET_OBJ_POLY_1(ch, i),
						i->get_in_obj()->get_PName(ECase::kPre).c_str());
			}
		} else if (i->get_worn_by()) {
			const auto worn_by = i->get_worn_by();
			const auto same_zone = world[ch->in_room]->zone_rn == world[worn_by->in_room]->zone_rn;
			if (!worn_by->IsNpc() || same_zone || bloody_corpse) {
				sprintf(buf, "%s надет%s на %s.\r\n", i->get_short_description().c_str(),
						GET_OBJ_SUF_6(i), PERS(worn_by, ch, 3));
			} else {
				return false;
			}
		} else if (!(locate_msg = Depot::PrintSpellLocateObject(ch, i.get())).empty()) {
			SendMsgToChar(locate_msg.c_str(), ch);
			return true;
		} else if (!(locate_msg = Parcel::PrintSpellLocateObject(ch, i.get())).empty()) {
			SendMsgToChar(locate_msg.c_str(), ch);
			return true;
		} else {
			sprintf(buf, "Местоположение %s неопределимо.\r\n", OBJN(i.get(), ch, ECase::kGen));
		}
		SendMsgToChar(buf, ch);
		return true;
	}, count);

	int j = count;
	if (j > 0) {
		j = Clan::print_spell_locate_object(ch, j, std::string(name));
	}

	if (j == tmp_lvl) {
		// "nothing felt" on kLocateObject's sheaf as kCustomMsgOne.
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kLocateObject, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
	}
}

bool CatchBloodyCorpse(ObjData *l) {
	bool temp_bloody = false;
	ObjData *next_element;

	if (!l->get_contains()) {
		return false;
	}

	if (bloody::is_bloody(l->get_contains())) {
		return true;
	}

	if (!l->get_contains()->get_next_content()) {
		return false;
	}

	next_element = l->get_contains()->get_next_content();
	while (next_element) {
		if (next_element->get_contains()) {
			temp_bloody = CatchBloodyCorpse(next_element->get_contains());
			if (temp_bloody) {
				return true;
			}
		}

		if (bloody::is_bloody(next_element)) {
			return true;
		}

		next_element = next_element->get_contains();
	}

	return false;
}

void SpellCreateWeapon(int/* level*/, CharData* /*ch*/, CharData* /*victim*/, ObjData* /* obj*/) {
	//go_create_weapon(ch,nullptr,what_sky);
// отключено, так как не реализовано
}

int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id) {
	int cha_summ = 0, reformed_hp_summ = 0;
	bool undead_in_group = false, living_in_group = false;

	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kCharmed)
			&& k->get_master() == ch) {
			cha_summ++;
			//hp_summ += GET_REAL_MAX_HIT(k->ch);
			reformed_hp_summ += GetReformedCharmiceHp(ch, k, spell_id);
// Проверка на тип последователей -- некрасиво, зато эффективно
			if (k->IsFlagged(EMobFlag::kCorpse)) {
				undead_in_group = true;
			} else {
				living_in_group = true;
			}
		}
	}

	if (undead_in_group && living_in_group) {
		mudlog("SYSERR: Undead and living in group simultaniously", NRM, kLvlGod, ERRLOG, true);
		return (false);
	}

	if (spell_id == ESpell::kCharm && undead_in_group) {
		SendMsgToChar("Ваша жертва боится ваших последователей.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kCharm && living_in_group) {
		SendMsgToChar("Ваш последователь мешает вам произнести это заклинание.\r\n", ch);
		return (false);
	}

	if (spell_id == ESpell::kClone && cha_summ >= std::max(1, (GetRealLevel(ch) + 4) / 5 - 2)) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kClone && cha_summ >= (GetRealLevel(ch) + 9) / 10) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kClone &&
		reformed_hp_summ + GetReformedCharmiceHp(ch, victim, spell_id) >= CalcCharmPoint(ch, spell_id)) {
		SendMsgToChar("Вам не под силу управлять такой боевой мощью.\r\n", ch);
		return (false);
	}
	return (true);
}

void SpellCharm(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/) {
	int k_skills = 0;
	ESkill skill_id = ESkill::kUndefined;
		Affect<EApply> af;
	if (victim == nullptr || ch == nullptr)
		return;

	// Rejection narration: six of the SpellCharm reject
	// paths share semantics with existing kSummon* / kResurrect* keys; the
	// per-spell kCharm sheaf carries the charm-specific wording while the
	// kDefault texts (phrased for resurrect/summon) stay intact for those
	// callers. Four messages without a clean key match stay inline.
	auto SendCharmMsg = [ch](ESpellMsg key) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kCharm, key) + "\r\n", ch);
	};
	if (victim == ch)
		SendCharmMsg(ESpellMsg::kCustomMsgOne);  // self-cast humor; see kCharm sheaf.
	else if (!victim->IsNpc()) {
		// 3 charm one-offs migrated to kCharm's kCustomMsg slots.
		SendCharmMsg(ESpellMsg::kCustomMsgTwo);
		if (!pk_agro_action(ch, victim))
			return;
	} else if (!ch->IsImmortal()
		&& (AFF_FLAGGED(victim, EAffect::kSanctuary) || victim->IsFlagged(EMobFlag::kProtect)))
		SendCharmMsg(ESpellMsg::kResurrectConsecrated);
	else if (!ch->IsImmortal() && (AFF_FLAGGED(victim, EAffect::kGodsShield) || victim->IsFlagged(EMobFlag::kProtect)))
		SendCharmMsg(ESpellMsg::kResurrectProtected);
	else if (!ch->IsImmortal() && victim->IsFlagged(EMobFlag::kNoCharm))
		SendCharmMsg(ESpellMsg::kResurrectNoPower);
	else if (AFF_FLAGGED(ch, EAffect::kCharmed))
		SendCharmMsg(ESpellMsg::kSummonCharmed);
	else if (AFF_FLAGGED(victim, EAffect::kCharmed)
		|| victim->IsFlagged(EMobFlag::kAgressive)
		|| victim->IsFlagged(EMobFlag::kAgressiveMono)
		|| victim->IsFlagged(EMobFlag::kAgressivePoly)
		|| victim->IsFlagged(EMobFlag::kAgressiveDay)
		|| victim->IsFlagged(EMobFlag::kAggressiveNight)
		|| victim->IsFlagged(EMobFlag::kAgressiveFullmoon)
		|| victim->IsFlagged(EMobFlag::kAgressiveWinter)
		|| victim->IsFlagged(EMobFlag::kAgressiveSpring)
		|| victim->IsFlagged(EMobFlag::kAgressiveSummer)
		|| victim->IsFlagged(EMobFlag::kAgressiveAutumn))
		SendCharmMsg(ESpellMsg::kSummonFail);
	else if (IS_HORSE(victim))
		SendCharmMsg(ESpellMsg::kSummonWarhorse);
	else if (victim->GetEnemy() || victim->GetPosition() < EPosition::kRest)
		act(MUD::SpellMessages().GetMessage(ESpell::kCharm, ESpellMsg::kCustomMsgThree).c_str(),
			false, ch, nullptr, victim, kToChar);
	else if (circle_follow(victim, ch))
		SendCharmMsg(ESpellMsg::kCustomMsgFour);
	else if (!ch->IsImmortal()
		&& CalcGeneralSaving(ch, victim, ESaving::kWill, (GetRealCha(ch) - 10) * 4 + GetRealRemort(ch) * 3)) //предлагаю завязать на каст
		SendCharmMsg(ESpellMsg::kSummonFail);
	else {
		if (!CheckCharmices(ch, victim, ESpell::kCharm)) {
			return;
		}

		// Левая проверка
		if (victim->has_master()) {
			if (stop_follower(victim, kSfMasterdie)) {
				return;
			}
		}

		if (CAN_SEE(victim, ch)) {
			mob_ai::mobRemember(victim, ch);
		}

		if (victim->IsFlagged(EMobFlag::kNoGroup))
			victim->UnsetFlag(EMobFlag::kNoGroup);
		RemoveAffectFromChar(victim, ESpell::kCharm);
		if (GetRealInt(victim) > GetRealInt(ch)) {
			af.duration = CalcDuration(victim, victim, ESkill::kUndefined, GetRealCha(ch), 0, 0, 0);
		} else {
			af.duration = CalcDuration(victim, victim, ESkill::kUndefined, GetRealCha(ch) + number(1, 10) + GetRealRemort(ch) * 2, 0, 0, 0);
		}
		af.modifier = 0;
		af.location = EApply::kNone;
		af.battleflag = 0;
		af.type = ESpell::kCharm;

		// резервируем место под фит ()
		// the ~390-line AnimalMaster body moved to
		// src/gameplay/skills/animal_master.{h,cpp}; the race check now uses the
		// named ENpcRace::kAnimal constant instead of the magic number 104.
		if (CanUseFeat(ch, EFeat::kAnimalMaster) && GET_RACE(victim) == ENpcRace::kAnimal) {
			ApplyAnimalMaster(ch, victim, af, k_skills, skill_id);
		}
		victim->summon_helpers.clear();
		if (victim->IsNpc()) {
			if (!victim->IsFlagged(EMobFlag::kSummoned)) { // только если не маг зверьки ()
				for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
					if (GET_EQ(victim, i)) {
						if (!remove_otrigger(GET_EQ(victim, i), victim)) {
							continue;
						}
						act("$n прекратил$g использовать $o3.",
							true, victim, GET_EQ(victim, i), nullptr, kToRoom);
						PlaceObjToInventory(UnequipChar(victim, i, CharEquipFlag::show_msg), victim);
					}
				}
			} else {
				// запрещаем умке одевать шмот
				victim->UnsetFlag(ENpcFlag::kWielding);
				victim->UnsetFlag(ENpcFlag::kArmoring);
			}
			victim->UnsetFlag(EMobFlag::kAgressive);
			victim->UnsetFlag(EMobFlag::kSpec);
			victim->UnsetFlag(EPrf::kPunctual);
			victim->SetFlag(EMobFlag::kNoSkillTrain);
			// по идее при речарме и последующем креше можно оказаться с сейвом без шмота на чармисе -- Krodo
			if (!NPC_FLAGGED(victim, ENpcFlag::kNoMercList)) {
				MobVnum mvn = GET_MOB_VNUM(victim);

				if (mvn / 100 >=  dungeons::kZoneStartDungeons) {
					mvn = zone_table[GetZoneRnum(mvn / 100)].copy_from_zone * 100 + mvn % 100;
				}
				ch->updateCharmee(mvn, 0);
			}
			Crash_crashsave(ch);
			ch->save_char();
		}
		af.modifier = 0;
		af.location = EApply::kNone;
		af.battleflag = 0;
		af.type = ESpell::kCharm;
		af.affect_type = EAffect::kCharmed;
		affect_to_char(victim, af);
		ch->add_follower(victim);
	}
	// тут обрабатываем, если виктим маг-зверь => передаем в фунцию создание маг шмоток (цель, базовый скил, процент владения)
	if (victim->IsFlagged(EMobFlag::kSummoned)) {
		create_charmice_stuff(victim, skill_id, k_skills);
	}
}

void ShowWeapon(CharData *ch, ObjData *obj) {
	if (obj->get_type() == EObjType::kWeapon) {
		*buf = '\0';
		if (CAN_WEAR(obj, EWearFlag::kWield)) {
			sprintf(buf, "Можно взять %s в правую руку.\r\n", OBJN(obj, ch, ECase::kAcc));
		}

		if (CAN_WEAR(obj, EWearFlag::kHold)) {
			sprintf(buf + strlen(buf), "Можно взять %s в левую руку.\r\n", OBJN(obj, ch, ECase::kAcc));
		}

		if (CAN_WEAR(obj, EWearFlag::kBoth)) {
			sprintf(buf + strlen(buf), "Можно взять %s в обе руки.\r\n", OBJN(obj, ch, ECase::kAcc));
		}

		if (*buf) {
			SendMsgToChar(buf, ch);
		}
	}
}

void PrintBookUpgradeSkill(CharData *ch, const ObjData *obj) {
	const auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
	if (MUD::Skills().IsInvalid(skill_id)) {
		log("SYSERR: invalid skill_id: %d, ch_name=%s, ObjVnum=%d (%s %s %d)",
			GET_OBJ_VAL(obj, 1), ch->get_name().c_str(), GET_OBJ_VNUM(obj), __FILE__, __func__, __LINE__);
		return;
	}
	if (GET_OBJ_VAL(obj, 3) > 0) {
		SendMsgToChar(ch, "повышает умение \"%s\" (максимум %d)\r\n",
					  MUD::Skill(skill_id).GetName(), GET_OBJ_VAL(obj, 3));
	} else {
		SendMsgToChar(ch, "повышает умение \"%s\" (не больше максимума текущего перевоплощения)\r\n",
					  MUD::Skill(skill_id).GetName());
	}
}

// Per-type detail block of MortShowObjValues. Pulled out so the parent
// stays under the 200-line ceiling and the type-specific rendering can be
// read without scrolling past the shared header / footer code.
static void ShowObjTypeSpecificValues(const ObjData *obj, CharData *ch) {
	int i, j, drndice = 0, drsdice = 0;
	long int li;
	(void) i; (void) j; (void) li;  // some branches do not touch all of them
switch (obj->get_type()) {
	case EObjType::kScroll:
	case EObjType::kPotion: {
		std::ostringstream out;
		out << "Содержит заклинание:";
		for (auto val = 1; val < 4; ++val) {
			auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, val));
			if (MUD::Spell(spell_id).IsValid()) {
				out << " Ур. [" << GET_OBJ_VAL(obj, 0) << "] " << MUD::Spell(spell_id).GetName() << ",";
			}
		}
		if (out.str().back() == ',') {
			out.seekp(-1, out.end);
		}
		out << "\r\n";
		SendMsgToChar(out.str(), ch);
		break;
	}
	case EObjType::kWand:
	case EObjType::kStaff: sprintf(buf, "Вызывает заклинания: ");
		sprintf(buf + strlen(buf), " %s\r\n",
				MUD::Spell(static_cast<ESpell>(GET_OBJ_VAL(obj, 3))).GetCName());
		sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n",
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		SendMsgToChar(buf, ch);
		break;

	case EObjType::kWeapon: drndice = GET_OBJ_VAL(obj, 1);
		drsdice = GET_OBJ_VAL(obj, 2);
		sprintf(buf, "Наносимые повреждения '%dD%d'", drndice, drsdice);
		sprintf(buf + strlen(buf), " среднее %.1f.\r\n", ((drsdice + 1) * drndice / 2.0));
		SendMsgToChar(buf, ch);
		break;

	case EObjType::kArmor:
	case EObjType::kLightArmor:
	case EObjType::kMediumArmor:
	case EObjType::kHeavyArmor: drndice = GET_OBJ_VAL(obj, 0);
		drsdice = GET_OBJ_VAL(obj, 1);
		sprintf(buf, "защита (AC) : %d\r\n", drndice);
		SendMsgToChar(buf, ch);
		sprintf(buf, "броня       : %d\r\n", drsdice);
		SendMsgToChar(buf, ch);
		break;

	case EObjType::kBook:
		switch (GET_OBJ_VAL(obj, 0)) {
			case EBook::kSpell: {
				auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, 1));
				if (spell_id >= ESpell::kFirst && spell_id <= ESpell::kLast) {
					drndice = GET_OBJ_VAL(obj, 1);
					if (MUD::Class(ch->GetClass()).spells.IsAvailable(spell_id)) {
						drsdice = CalcMinSpellLvl(ch, spell_id, GET_OBJ_VAL(obj, 2));
					} else {
						drsdice = kLvlImplementator;
					}
					sprintf(buf, "содержит заклинание        : \"%s\"\r\n", MUD::Spell(spell_id).GetCName());
					SendMsgToChar(buf, ch);
					sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
					SendMsgToChar(buf, ch);
				}
				break;
			}
			case EBook::kSkill: {
				auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
				if (MUD::Skills().IsValid(skill_id)) {
					drndice = GET_OBJ_VAL(obj, 1);
					if (MUD::Class(ch->GetClass()).skills[skill_id].IsAvailable()) {
						drsdice = GetSkillMinLevel(ch, skill_id, GET_OBJ_VAL(obj, 2));
					} else {
						drsdice = kLvlImplementator;
					}
					sprintf(buf, "содержит секрет умения     : \"%s\"\r\n", MUD::Skill(skill_id).GetName());
					SendMsgToChar(buf, ch);
					sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
					SendMsgToChar(buf, ch);
				}
				break;
			}
			case EBook::kSkillUpgrade: PrintBookUpgradeSkill(ch, obj);
				break;

			case EBook::kReceipt: drndice = im_get_recipe(GET_OBJ_VAL(obj, 1));
				if (drndice >= 0) {
					drsdice = std::max(GET_OBJ_VAL(obj, 2), imrecipes[drndice].level);
					int count = imrecipes[drndice].remort;
					if (imrecipes[drndice].classknow[to_underlying(ch->GetClass())] != kKnownRecipe)
						drsdice = kLvlImplementator;
					sprintf(buf, "содержит рецепт отвара     : \"%s\"\r\n", imrecipes[drndice].name);
					SendMsgToChar(buf, ch);
					if (drsdice == -1 || count == -1) {
						SendMsgToChar(kColorBoldRed, ch);
						SendMsgToChar("Некорректная запись рецепта для вашего класса - сообщите Богам.\r\n", ch);
						SendMsgToChar(kColorNrm, ch);
					} else if (drsdice == kLvlImplementator) {
						sprintf(buf, "уровень изучения (количество ремортов) : %d (--)\r\n", drsdice);
						SendMsgToChar(buf, ch);
					} else {
						sprintf(buf, "уровень изучения (количество ремортов) : %d (%d)\r\n", drsdice, count);
						SendMsgToChar(buf, ch);
					}
				}
				break;

			case EBook::kFeat: {
				const auto feat_id = static_cast<EFeat>(GET_OBJ_VAL(obj, 1));
				if (MUD::Feat(feat_id).IsValid()) {
					if (CanGetFeat(ch, feat_id)) {
						drsdice = MUD::Class(ch->GetClass()).feats[feat_id].GetSlot();
					} else {
						drsdice = kLvlImplementator;
					}
					sprintf(buf, "содержит секрет способности : \"%s\"\r\n", MUD::Feat(feat_id).GetCName());
					SendMsgToChar(buf, ch);
					sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
					SendMsgToChar(buf, ch);
				}
			}
				break;

			default: SendMsgToChar(kColorBoldRed, ch);
				SendMsgToChar("НЕВЕРНО УКАЗАН ТИП КНИГИ - сообщите Богам\r\n", ch);
				SendMsgToChar(kColorNrm, ch);
				break;
		}
		break;

	case EObjType::kMagicIngredient: sprintbit(obj->get_spec_param(), ingradient_bits, buf2, sizeof(buf2));
		snprintf(buf, kMaxStringLength, "%s\r\n", buf2);
		SendMsgToChar(buf, ch);

		if (IS_SET(obj->get_spec_param(), kItemCheckUses)) {
			sprintf(buf, "можно применить %d раз\r\n", GET_OBJ_VAL(obj, 2));
			SendMsgToChar(buf, ch);
		}

		if (IS_SET(obj->get_spec_param(), kItemCheckLag)) {
			sprintf(buf, "можно применить 1 раз в %d сек", (i = GET_OBJ_VAL(obj, 0) & 0xFF));
			if (GET_OBJ_VAL(obj, 3) == 0 || GET_OBJ_VAL(obj, 3) + i < time(nullptr))
				strcat(buf, "(можно применять).\r\n");
			else {
				li = GET_OBJ_VAL(obj, 3) + i - time(nullptr);
				sprintf(buf + strlen(buf), "(осталось %ld сек).\r\n", li);
			}
			SendMsgToChar(buf, ch);
		}

		if (IS_SET(obj->get_spec_param(), kItemCheckLevel)) {
			sprintf(buf, "можно применить с %d уровня.\r\n", (GET_OBJ_VAL(obj, 0) >> 8) & 0x1F);
			SendMsgToChar(buf, ch);
		}

		if ((i = GetObjRnum(GET_OBJ_VAL(obj, 1))) >= 0) {
			sprintf(buf, "прототип %s%s%s.\r\n",
					kColorBoldCyn, obj_proto[i]->get_PName(ECase::kNom).c_str(), kColorNrm);
			SendMsgToChar(buf, ch);
		}
		break;

	case EObjType::kMagicComponent:
		for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT) && j <= top_imtypes;) {
			j++;
		}
		sprintf(buf, "Это ингредиент вида '%s%s%s'\r\n", kColorCyn, imtypes[j].name, kColorNrm);
		SendMsgToChar(buf, ch);
		i = GET_OBJ_VAL(obj, IM_POWER_SLOT);
		if (i > 45) { // тут явно опечатка была, кроме того у нас мобы и выше 40лвл
			SendMsgToChar("Вы не в состоянии определить качество этого ингредиента.\r\n", ch);
		} else {
			sprintf(buf, "Качество ингредиента ");
			if (i > 40)
				strcat(buf, "божественное.\r\n");
			else if (i > 35)
				strcat(buf, "идеальное.\r\n");
			else if (i > 30)
				strcat(buf, "наилучшее.\r\n");
			else if (i > 25)
				strcat(buf, "превосходное.\r\n");
			else if (i > 20)
				strcat(buf, "отличное.\r\n");
			else if (i > 15)
				strcat(buf, "очень хорошее.\r\n");
			else if (i > 10)
				strcat(buf, "выше среднего.\r\n");
			else if (i > 5)
				strcat(buf, "весьма посредственное.\r\n");
			else
				strcat(buf, "хуже не бывает.\r\n");
			SendMsgToChar(buf, ch);
		}
		break;

		//Информация о контейнерах (Купала)
	case EObjType::kContainer: sprintf(buf, "Максимально вместимый вес: %d.\r\n", GET_OBJ_VAL(obj, 0));
		SendMsgToChar(buf, ch);
		break;

		//Информация о емкостях (Купала)
	case EObjType::kLiquidContainer: drinkcon::identify(ch, obj);
		break;

	case EObjType::kMagicArrow:
	case EObjType::kMagicContaner: sprintf(buf, "Может вместить стрел: %d.\r\n", GET_OBJ_VAL(obj, 1));
		sprintf(buf, "Осталось стрел: %s%d&n.\r\n",
				GET_OBJ_VAL(obj, 2) > 3 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
		SendMsgToChar(buf, ch);
		break;

	default: break;
} // switch
}

void MortShowObjValues(const ObjData *obj, CharData *ch, int fullness) {
	int i;
	bool found;
	bool enhansed_scroll = false;
	
	if (fullness > 399) {
		enhansed_scroll = true;
	}
	SendMsgToChar("Вы узнали следующее:\r\n", ch);
	sprintf(buf, "Предмет \"%s\", тип : ", obj->get_short_description().c_str());
	sprinttype(obj->get_type(), item_types, buf2);
	strcat(buf, buf2);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	strcpy(buf, diag_weapon_to_char(obj, 2));
	if (*buf)
		SendMsgToChar(buf, ch);

	if (fullness < 20)
		return;

	//ShowWeapon(ch, obj);

	sprintf(buf, "Вес: %d, Цена: %d, Рента: %d(%d)\r\n",
			obj->get_weight(), obj->get_cost(), obj->get_rent_off(), obj->get_rent_on());
	SendMsgToChar(buf, ch);

	if (fullness < 30)
		return;
	sprinttype(obj->get_material(), material_name, buf2);
	snprintf(buf, kMaxStringLength, "Материал : %s, макс.прочность : %d, тек.прочность : %d\r\n", buf2,
			 obj->get_maximum_durability(), obj->get_current_durability());
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 40)
		return;

	SendMsgToChar("Неудобен : ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_no_flags().sprintbits(no_bits, buf, sizeof(buf), ",", ch->IsImmortal() ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 50)
		return;

	SendMsgToChar("Недоступен : ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_anti_flags().sprintbits(anti_bits, buf, sizeof(buf), ",", ch->IsImmortal() ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (obj->get_auto_mort_req() > 0) {
		SendMsgToChar(ch, "Требует перевоплощений : %s%d%s\r\n",
					  kColorCyn, obj->get_auto_mort_req(), kColorNrm);
	} else if (obj->get_auto_mort_req() < -1) {
		SendMsgToChar(ch, "Максимальное количество перевоплощение : %s%d%s\r\n",
					  kColorCyn, abs(obj->get_minimum_remorts()), kColorNrm);
	}

	if (fullness < 60)
		return;

	SendMsgToChar("Имеет экстрафлаги: ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_extra_flags().sprintbits(extra_bits, buf, sizeof(buf), ",", ch->IsImmortal() ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);
//enhansed_scroll = true; //для теста
	if (enhansed_scroll) {
		if (stable_objs::IsTimerUnlimited(obj))
			sprintf(buf2, "Таймер: %d/нерушимо.", obj_proto[obj->get_rnum()]->get_timer());
		else
			sprintf(buf2, "Таймер: %d/%d.", obj_proto[obj->get_rnum()]->get_timer(), obj->get_timer());
		char miw[128];
		if (GetObjMIW(obj->get_rnum()) < 0) {
			sprintf(miw, "%s", "бесконечно");
		} else {
			sprintf(miw, "%d", GetObjMIW(obj->get_rnum()));
		}
		snprintf(buf, kMaxStringLength, "&GСейчас в мире : %d. На постое : %d. Макс. в мире : %s. %s&n\r\n",
				 obj_proto.total_online(obj->get_rnum()), obj_proto.stored(obj->get_rnum()), miw, buf2);
		SendMsgToChar(buf, ch);
	}
	if (fullness < 75)
		return;

	ShowObjTypeSpecificValues(obj, ch);

	if (fullness < 90) {
		return;
	}

	SendMsgToChar("Накладывает на вас аффекты: ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_affect_flags().sprintbits(weapon_affects, buf, sizeof(buf), ",", ch->IsImmortal() ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 100) {
		return;
	}

	found = false;
	for (i = 0; i < kMaxObjAffect; i++) {
		if (obj->get_affected(i).location != EApply::kNone
			&& obj->get_affected(i).modifier != 0) {
			if (!found) {
				SendMsgToChar("Дополнительные свойства :\r\n", ch);
				found = true;
			}
			print_obj_affects(ch, obj->get_affected(i));
		}
	}

	if (obj->get_type() == EObjType::kEnchant
		&& GET_OBJ_VAL(obj, 0) != 0) {
		if (!found) {
			SendMsgToChar("Дополнительные свойства :\r\n", ch);
			found = true;
		}
		SendMsgToChar(ch, "%s   %s вес предмета на %d%s\r\n", kColorCyn,
					  GET_OBJ_VAL(obj, 0) > 0 ? "увеличивает" : "уменьшает",
					  abs(GET_OBJ_VAL(obj, 0)), kColorNrm);
	}

	if (obj->has_skills()) {
		SendMsgToChar("Меняет умения :\r\n", ch);
		CObjectPrototype::skills_t skills;
		obj->get_skills(skills);
		int percent;
		for (const auto &it : skills) {
			auto skill_id = it.first;
			percent = it.second;

			if (percent == 0) // TODO: такого не должно быть?
				continue;

			sprintf(buf, "   %s%s%s%s%s%d%%%s\r\n",
					kColorCyn, MUD::Skill(skill_id).GetName(), kColorNrm,
					kColorCyn,
					percent < 0 ? " ухудшает на " : " улучшает на ", abs(percent), kColorNrm);
			SendMsgToChar(buf, ch);
		}
	}

	auto it = ObjData::set_table.begin();
	if (obj->has_flag(EObjFlag::kSetItem)) {
		for (; it != ObjData::set_table.end(); it++) {
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				sprintf(buf,
						"Часть набора предметов: %s%s%s\r\n",
						kColorNrm,
						it->second.get_name().c_str(),
						kColorNrm);
				SendMsgToChar(buf, ch);
				for (auto & vnum : it->second) {
					const int r_num = GetObjRnum(vnum.first);
					if (r_num < 0) {
						SendMsgToChar("Неизвестный объект!!!\r\n", ch);
						continue;
					}
					sprintf(buf, "   %s\r\n", obj_proto[r_num]->get_short_description().c_str());
					SendMsgToChar(buf, ch);
				}
				break;
			}
		}
	}

	if (!obj->get_enchants().empty()) {
		obj->get_enchants().print(ch);
	}
	obj_sets::print_identify(ch, obj);
}

void MortShowCharValues(CharData *victim, CharData *ch, int fullness) {
	int val0, val1, val2;

	sprintf(buf, "Имя: %s\r\n", GET_NAME(victim));
	SendMsgToChar(buf, ch);
	if (!victim->IsNpc() && victim == ch) {
		sprintf(buf, "Написание : %s/%s/%s/%s/%s/%s\r\n",
				GET_PAD(victim, 0), GET_PAD(victim, 1), GET_PAD(victim, 2),
				GET_PAD(victim, 3), GET_PAD(victim, 4), GET_PAD(victim, 5));
		SendMsgToChar(buf, ch);
	}

	if (!victim->IsNpc() && victim == ch) {
		const auto &victimAge = CalcCharAge(victim);
		sprintf(buf, "Возраст %s  : %d лет, %d месяцев, %d дней и %d часов.\r\n",
				GET_PAD(victim, 1), victimAge->year, victimAge->month, victimAge->day, victimAge->hours);
		SendMsgToChar(buf, ch);
	}
	if (fullness < 20 && ch != victim)
		return;

	val0 = GET_HEIGHT(victim);
	val1 = GET_WEIGHT(victim);
	val2 = GET_SIZE(victim);
	sprintf(buf, "Вес %d, Размер %d\r\n", val1,
			val2);
	SendMsgToChar(buf, ch);
	if (fullness < 60 && ch != victim)
		return;

	val0 = GetRealLevel(victim);
	val1 = victim->get_hit();
	val2 = victim->get_real_max_hit();
	sprintf(buf, "Уровень : %d, может выдержать повреждений : %d(%d), ", val0, val1, val2);
	SendMsgToChar(buf, ch);
	SendMsgToChar(ch, "Перевоплощений : %d\r\n", GetRealRemort(victim));
	val0 = MIN(GET_AR(victim), 100);
	val1 = MIN(GET_MR(victim), 100);
	val2 = MIN(GET_PR(victim), 100);
	sprintf(buf,
			"Защита от чар : %d, Защита от магических повреждений : %d, Защита от физических повреждений : %d\r\n",
			val0,
			val1,
			val2);
	SendMsgToChar(buf, ch);
	if (fullness < 90 && ch != victim)
		return;

	SendMsgToChar(ch, "Атака : %d, Повреждения : %d\r\n",
				  GET_HR(victim), GET_DR(victim));
	SendMsgToChar(ch, "Защита : %d, Броня : %d, Поглощение : %d\r\n",
				  CalcBaseAc(victim), GET_ARMOUR(victim), GET_ABSORBE(victim));

	if (fullness < 100 || (ch != victim && !victim->IsNpc()))
		return;

	val0 = victim->get_str();
	val1 = victim->get_int();
	val2 = victim->get_wis();
	sprintf(buf, "Сила: %d, Ум: %d, Муд: %d, ", val0, val1, val2);
	val0 = victim->get_dex();
	val1 = victim->get_con();
	val2 = victim->get_cha();
	sprintf(buf + strlen(buf), "Ловк: %d, Тел: %d, Обаян: %d\r\n", val0, val1, val2);
	SendMsgToChar(buf, ch);

	if (fullness < 120 || (ch != victim && !victim->IsNpc()))
		return;

	int found = false;
	for (const auto &aff : victim->affected) {
		if (aff->location != EApply::kNone && aff->modifier != 0) {
			if (!found) {
				SendMsgToChar("Дополнительные свойства :\r\n", ch);
				found = true;
				SendMsgToChar(kColorBoldRed, ch);
			}
			sprinttype(aff->location, apply_types, buf2);
			snprintf(buf,
					 kMaxStringLength,
					 "   %s изменяет на %s%d\r\n",
					 buf2,
					 aff->modifier > 0 ? "+" : "",
					 aff->modifier);
			SendMsgToChar(buf, ch);
		}
	}
	SendMsgToChar(kColorNrm, ch);

	SendMsgToChar("Аффекты :\r\n", ch);
	SendMsgToChar(kColorBoldCyn, ch);
	victim->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, sizeof(buf2), "\r\n", ch->IsImmortal() ? 4 : 0);
	snprintf(buf, kMaxStringLength, "%s\r\n", buf2);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);
}

void SkillIdentify(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	if (obj) {
		MortShowObjValues(obj, ch, CalcCurrentSkill(ch, ESkill::kIdentify, nullptr));
		TrainSkill(ch, ESkill::kIdentify, true, nullptr);
	} else if (victim) {
		if (GetRealLevel(victim) < 3) {
			SendMsgToChar("Вы можете опознать только персонажа, достигнувшего третьего уровня.\r\n", ch);
			return;
		}
		MortShowCharValues(victim, ch, CalcCurrentSkill(ch, ESkill::kIdentify, victim));
		TrainSkill(ch, ESkill::kIdentify, true, victim);
	}
}

void SpellFullIdentify(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	if (obj)
		MortShowObjValues(obj, ch, 400);
	else if (victim) {
		// kFullIdentify overrides kWrongTarget with the identify-specific text
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kFullIdentify, ESpellMsg::kWrongTarget) + "\r\n", ch);
			return;
	}
}

void SpellIdentify(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	if (obj)
		MortShowObjValues(obj, ch, 100);
	else if (victim) {
		if (GET_GOD_FLAG(ch, EGf::kAllowTesterMode) && (world[ch->in_room]->vnum / 100 >= dungeons::kZoneStartDungeons)) {
			do_stat_character(ch, victim);
			return;
		}
		if (victim != ch) {
			// kIdentify overrides kWrongTarget with the identify-specific text
			SendMsgToChar(MUD::SpellMessages().GetMessage(
					ESpell::kIdentify, ESpellMsg::kWrongTarget) + "\r\n", ch);
			return;
		}
		if (GetRealLevel(victim) < 3) {
			// low-level self-identify rejection on kIdentify's sheaf.
			SendMsgToChar(MUD::SpellMessages().GetMessage(
					ESpell::kIdentify, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
			return;
		}
		MortShowCharValues(victim, ch, 100);
	}
}

void SpellControlWeather(int/* level*/, CharData *ch, CharData* /*victim*/, ObjData* /*obj*/) {
	const char *sky_info = nullptr;
	int i, duration, zone, sky_type = 0;

	if (what_sky > kSkyLightning)
		what_sky = kSkyLightning;

	switch (what_sky) {
		case kSkyCloudless: sky_info = "Небо покрылось облаками.";
			break;
		case kSkyCloudy: sky_info = "Небо покрылось тяжелыми тучами.";
			break;
		case kSkyRaining:
			if (time_info.month >= EMonth::kMay && time_info.month <= EMonth::kOctober) {
				sky_info = "Начался проливной дождь.";
				SetPrecipitations(&sky_type, kWeatherLightrain, 0, 50, 50);
			} else if (time_info.month >= EMonth::kDecember || time_info.month <= EMonth::kFebruary) {
				sky_info = "Повалил снег.";
				SetPrecipitations(&sky_type, kWeatherLightsnow, 0, 50, 50);
			} else if (time_info.month == EMonth::kMarch || time_info.month == EMonth::kNovember) {
				if (weather_info.temperature > 2) {
					sky_info = "Начался проливной дождь.";
					SetPrecipitations(&sky_type, kWeatherLightrain, 0, 50, 50);
				} else {
					sky_info = "Повалил снег.";
					SetPrecipitations(&sky_type, kWeatherLightsnow, 0, 50, 50);
				}
			}
			break;
		case kSkyLightning: sky_info = "На небе не осталось ни единого облачка.";
			break;
		default: break;
	}

	if (sky_info) {
		duration = std::max(GetRealLevel(ch) / 8, 2);
		zone = world[ch->in_room]->zone_rn;
		for (i = kFirstRoom; i <= top_of_world; i++)
			if (world[i]->zone_rn == zone && SECT(i) != ESector::kInside && SECT(i) != ESector::kCity) {
				world[i]->weather.sky = what_sky;
				world[i]->weather.weather_type = sky_type;
				world[i]->weather.duration = duration;
				if (world[i]->first_character()) {
					act(sky_info, false, world[i]->first_character(), nullptr, nullptr, kToRoom | kToArenaListen);
					act(sky_info, false, world[i]->first_character(), nullptr, nullptr, kToChar);
				}
			}
	}
}

void SpellFear(int/* level*/, CharData *ch, CharData *victim, ObjData* /*obj*/) {
	int modi = 0;
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, ESpell::kFear);
		if (!pk_agro_action(ch, victim))
			return;
	}
	if (!ch->IsNpc() && (GetRealLevel(ch) > 10))
		modi += (GetRealLevel(ch) - 10);
	if (ch->IsFlagged(EPrf::kAwake))
		modi = modi - 50;
	if (AFF_FLAGGED(victim, EAffect::kBless))
		modi -= 25;

	if (!victim->IsFlagged(EMobFlag::kNoFear) && !CalcGeneralSaving(ch, victim, ESaving::kWill, modi))
		GoFlee(victim);
}

void SpellEnergydrain(int/* level*/, CharData *ch, CharData *victim, ObjData* /*obj*/) {
	// истощить энергию - круг 28 уровень 9 (1)
	// для всех
	int modi = 0;
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, ESpell::kEnergyDrain);
		if (!pk_agro_action(ch, victim))
			return;
	}
	if (!ch->IsNpc() && (GetRealLevel(ch) > 10))
		modi += (GetRealLevel(ch) - 10);
	if (ch->IsFlagged(EPrf::kAwake))
		modi = modi - 50;

	if (ch == victim || !CalcGeneralSaving(ch, victim, ESaving::kWill, modi)) {
		for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_MEM(victim, spell_id) = 0;
		}
		victim->caster_level = 0;
		SendMsgToChar("Внезапно вы осознали, что у вас напрочь отшибло память.\r\n", victim);
	} else
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kEnergyDrain, ESpellMsg::kNoeffect) + "\r\n", ch);
}

void do_sacrifice(CharData *ch, int dam) {
	ch->set_hit(std::max(ch->get_hit(), std::min(ch->get_hit() + std::max(1, dam), ch->get_real_max_hit()
		+ ch->get_real_max_hit() * GetRealLevel(ch) / 10)));
	update_pos(ch);
}

void SpellSacrifice(int/* level*/, CharData *ch, CharData *victim, ObjData* /*obj*/) {
	int dam, d0 = victim->get_hit();

	// Высосать жизнь - некроманы - уровень 18 круг 6й (5)
	// *** мин 54 макс 66 (330)

	if (victim->IsImmortal() || victim == ch || IS_CHARMICE(victim)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSacrifice, ESpellMsg::kNoeffect) + "\r\n", ch);
		return;
	}

	CastContext sac_ctx(ch, ESpell::kSacrifice, GetRealLevel(ch), {}, {});
	sac_ctx.cvict = victim;
	dam = CastDamage(sac_ctx);
	// victim может быть спуржен

	if (dam < 0)
		dam = d0;
	if (dam > d0)
		dam = d0;
	if (dam <= 0)
		return;

	do_sacrifice(ch, dam);
	if (!ch->IsNpc()) {
		for (auto *f : ch->followers) {
			if (f->IsNpc()
				&& AFF_FLAGGED(f, EAffect::kCharmed)
				&& f->IsFlagged(EMobFlag::kCorpse)
				&& ch->in_room == f->in_room) {
				do_sacrifice(f, dam);
			}
		}
	}
}

void SpellHolystrike(int/* level*/, CharData *ch, CharData* /*victim*/, ObjData* /*obj*/) {
	const char *msg1 = "Земля под вами засветилась и всех поглотил плотный туман.";
	const char *msg2 = "Вдруг туман стал уходить обратно в землю, забирая с собой тела поверженных.";

	act(msg1, false, ch, nullptr, nullptr, kToChar);
	act(msg1, false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

	const auto people_copy = world[ch->in_room]->people;
	for (const auto tch : people_copy) {
		if (tch->IsNpc()) {
			if (!tch->IsFlagged(EMobFlag::kCorpse)
				&& GET_RACE(tch) != ENpcRace::kZombie
				&& GET_RACE(tch) != ENpcRace::kBoggart) {
				continue;
			}
		} else {
			//Чуток нелогично, но раз зомби гоняет -- сам немного мертвяк. :)
			//Тут сам спелл бредовый... Но пока на скорую руку.
			if (!CanUseFeat(tch, EFeat::kZombieDrover)) {
				continue;
			}
		}

		// per-target order is Damage -> Unaffect -> Affect (matches
		// CallMagic's stage order). kHolystrike's <unaffect> dispels kEviless on the target and
		// returns kBreak, which skips the hold imposition for that just-dispelled minion. Damage
		// always lands first, so the high-damage replacement for the old instant_death still
		// bites kEviless minions on the way through.
		CastContext hs_ctx(ch, ESpell::kHolystrike, GetRealLevel(ch), {}, {});
		hs_ctx.cvict = tch;
		CastDamage(hs_ctx);
		if (CastUnaffects(hs_ctx) == EStageResult::kBreak) {
			continue;
		}
		CastAffect(hs_ctx);
	}

	act(msg2, false, ch, nullptr, nullptr, kToChar);
	act(msg2, false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

	ObjData *o = nullptr;
	do {
		for (auto o : world[ch->in_room]->contents) {
			if (!IS_CORPSE(o)) {
				continue;
			}

			ExtractObjFromWorld(o);

			break;
		}
	} while (o);
}

void SpellVampirism(int/* level*/, CharData* /*ch*/, CharData* /*victim*/, ObjData* /*obj*/) {
}

void SpellMentalShadow(CharData *ch) {
	// подготовка контейнера для создания заклинания ментальная тень
	// все предложения пишем мад почтой

	MobVnum mob_num = kMobMentalShadow;

	CharData *mob = nullptr;
	auto followers_copy = ch->followers;
	for (auto *k : followers_copy) {
		if (k->IsFlagged(EMobFlag::kMentalShadow)) {
			stop_follower(k, false);
		}
	}
	auto eff_int = get_effective_int(ch);
	int hp = 100;
	int hp_per_int = 15;
	float base_ac = 100;
	float additional_ac = -1.5;
	if (eff_int < 26 && !ch->IsImmortal()) {
		// low-Int rejection on kMentalShadow's sheaf as kCustomMsgOne.
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kMentalShadow, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
		return;
	};

	if (!(mob = ReadMobile(mob_num, kVirtual))) {
		// kSummonNoProto kDefault already carries this exact text -- the sheaf lookup
		// returns it without any per-spell override needed.
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kMentalShadow, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return;
	}
	Affect<EApply> af;
	af.type = ESpell::kCharm;
	af.duration = CalcDuration(mob, mob, ESkill::kUndefined, 5 + (int) VPOSI<float>((get_effective_int(ch) - 16.0) / 2, 0, 50), 0, 0, 0);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.affect_type = EAffect::kHelper;
	af.battleflag = 0;
	affect_to_char(mob, af);
	
	mob->set_max_hit(floorf(hp + hp_per_int * (eff_int - 20) + ch->get_hit()/4));
	mob->set_hit(mob->get_max_hit());
	GET_AC(mob) = floorf(base_ac + additional_ac * eff_int);
	// Добавление заклов и аффектов в зависимости от интелекта кудеса
	if (eff_int >= 28 && eff_int < 32) {
     	SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 1);
	} else if (eff_int >= 32 && eff_int < 38) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 1);
		af.affect_type = EAffect::kShadowCloak;
		affect_to_char(mob, af);
		mob->mob_specials.have_spell = true;
	} else if(eff_int >= 38 && eff_int < 44) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 2);
		af.affect_type = EAffect::kShadowCloak;
		affect_to_char(mob, af);
		mob->mob_specials.have_spell = true;
	
	} else if(eff_int >= 44) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 3);
		af.affect_type = EAffect::kShadowCloak;
		affect_to_char(mob, af);
		af.affect_type = EAffect::kBrokenChains;
		affect_to_char(mob, af);
		mob->mob_specials.have_spell = true;
	}
	if (mob->GetSkill(ESkill::kAwake)) {
		mob->SetFlag(EPrf::kAwake);
	}
	mob->set_level(GetRealLevel(ch));
	mob->SetFlag(EMobFlag::kCorpse);
	mob->SetFlag(EMobFlag::kMentalShadow);
	PlaceCharToRoom(mob, ch->in_room);
	ch->add_follower(mob);
	mob->set_protecting(ch);
	
	// kMentalShadow overrides kSummonToRoom (whose kDefault sheaf carries 9
	// random-failure variants used by kClone-style spells) with its single
	// success line.
	const auto &shadow_msg = MUD::SpellMessages().GetMessage(
			ESpell::kMentalShadow, ESpellMsg::kSummonToRoom);
	act(shadow_msg.c_str(), true, mob, nullptr, nullptr, kToRoom | kToArenaListen);
}

std::map<int /* vnum */, int /* count */> rune_list;

void AddRuneStats(CharData *ch, int vnum, int spelltype) {
	if (ch->IsNpc() || ESpellType::kRunes != spelltype) {
		return;
	}
	std::map<int, int>::iterator i = rune_list.find(vnum);
	if (rune_list.end() != i) {
		i->second += 1;
	} else {
		rune_list[vnum] = 1;
	}
}

void extract_item(CharData *ch, ObjData *obj, int spelltype) {
	int extract = false;
	if (!obj) {
		return;
	}

	obj->set_val(3, time(nullptr));

	if (IS_SET(obj->get_spec_param(), kItemCheckUses)) {
		obj->dec_val(2);
		if (GET_OBJ_VAL(obj, 2) <= 0
			&& IS_SET(obj->get_spec_param(), kItemDecayEmpty)) {
			extract = true;
		}
	} else if (spelltype != ESpellType::kRunes) {
		extract = true;
	}

	if (extract) {
		if (spelltype == ESpellType::kRunes) {
			snprintf(buf, kMaxStringLength, "$o%s рассыпал$U у вас в руках.",
					 char_get_custom_label(obj, ch).c_str());
			act(buf, false, ch, obj, nullptr, kToChar);
		}
		RemoveObjFromChar(obj);
		ExtractObjFromWorld(obj);
	}
}

int CheckRecipeValues(CharData *ch, ESpell spell_id, ESpellType spell_type, int showrecipe) {
	int item0 = -1, item1 = -1, item2 = -1, obj_num = -1;
	struct SpellCreateItem *items;

	if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
		return (false);
	}
	if (!spell_create.contains(spell_id)) {
		SendMsgToChar("Вам не доступно это заклинание.\r\n", ch);
		return (false);
	}
	if (spell_type == ESpellType::kItemCast) {
		items = &spell_create[spell_id].items;
	} else if (spell_type == ESpellType::kPotionCast) {
		items = &spell_create[spell_id].potion;
	} else if (spell_type == ESpellType::kWandCast) {
		items = &spell_create[spell_id].wand;
	} else if (spell_type == ESpellType::kScrollCast) {
		items = &spell_create[spell_id].scroll;
	} else if (spell_type == ESpellType::kRunes) {
		items = &spell_create[spell_id].runes;
	} else
		return (false);

	if (((obj_num = GetObjRnum(items->rnumber)) < 0 &&
		spell_type != ESpellType::kItemCast && spell_type != ESpellType::kRunes) ||
		((item0 = GetObjRnum(items->items[0])) +
			(item1 = GetObjRnum(items->items[1])) + (item2 = GetObjRnum(items->items[2])) < -2)) {
		if (showrecipe)
			SendMsgToChar("Боги хранят в секрете этот рецепт.\n\r", ch);
		return (false);
	}

	if (!showrecipe)
		return (true);
	else {
		strcpy(buf, "Вам потребуется :\r\n");
		if (item0 >= 0) {
			strcat(buf, kColorBoldRed);
			strcat(buf, obj_proto[item0]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (item1 >= 0) {
			strcat(buf, kColorBoldYel);
			strcat(buf, obj_proto[item1]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (item2 >= 0) {
			strcat(buf, kColorBoldGrn);
			strcat(buf, obj_proto[item2]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (obj_num >= 0 && (spell_type == ESpellType::kItemCast || spell_type == ESpellType::kRunes)) {
			strcat(buf, kColorBoldBlu);
			strcat(buf, obj_proto[obj_num]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}

		strcat(buf, kColorNrm);
		if (spell_type == ESpellType::kItemCast || spell_type == ESpellType::kRunes) {
			strcat(buf, "для создания магии '");
			strcat(buf, MUD::Spell(spell_id).GetCName());
			strcat(buf, "'.");
		} else {
			strcat(buf, "для создания ");
			strcat(buf, obj_proto[obj_num]->get_PName(ECase::kGen).c_str());
		}
		act(buf, false, ch, nullptr, nullptr, kToChar);
	}

	return (true);
}

/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
bool mag_item_ok(CharData *ch, ObjData *obj, int spelltype) {
	int num = 0;

	if (spelltype == ESpellType::kRunes
		&& obj->get_type() != EObjType::kMagicIngredient) {
		return false;
	}

	if (obj->get_type() == EObjType::kMagicIngredient) {
		if ((!IS_SET(obj->get_spec_param(), kItemRunes) && spelltype == ESpellType::kRunes)
			|| (IS_SET(obj->get_spec_param(), kItemRunes) && spelltype != ESpellType::kRunes)) {
			return false;
		}
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckUses)
		&& GET_OBJ_VAL(obj, 2) <= 0) {
		return false;
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckLag)) {
		num = 0;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag1S))
			num += 1;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag2S))
			num += 2;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag4S))
			num += 4;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag8S))
			num += 8;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag16S))
			num += 16;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag32S))
			num += 32;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag64S))
			num += 64;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag128S))
			num += 128;
		if (GET_OBJ_VAL(obj, 3) + num - 5 * GetRealRemort(ch) >= time(nullptr))
			return false;
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckLevel)) {
		num = 0;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel1))
			num += 1;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel2))
			num += 2;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel4))
			num += 4;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel8))
			num += 8;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel16))
			num += 16;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel32))
			num += 32;
		if (GetRealLevel(ch) + GetRealRemort(ch) < num)
			return false;
	}

	return true;
}

int CheckRecipeItems(CharData *ch, ESpell spell_id, ESpellType spell_type, int extract, CharData *tch) {
	ObjData *obj0 = nullptr, *obj1 = nullptr, *obj2 = nullptr, *obj3 = nullptr, *objo = nullptr;
	int item0 = -1, item1 = -1, item2 = -1, item3 = -1;
	int create = 0, obj_num = -1, percent = 0, num = 0;
	auto skill_id{ESkill::kUndefined};
	struct SpellCreateItem *items;

	if (spell_id <= ESpell::kUndefined) {
		return false;
	}
	if (!spell_create.contains(spell_id))
		return false;
	if (spell_type == ESpellType::kItemCast) {
		items = &spell_create[spell_id].items;
	} else if (spell_type == ESpellType::kPotionCast) {
		items = &spell_create[spell_id].potion;
		skill_id = ESkill::kCreatePotion;
		create = 1;
	} else if (spell_type == ESpellType::kWandCast) {
		items = &spell_create[spell_id].wand;
		skill_id = ESkill::kCreateWand;
		create = 1;
	} else if (spell_type == ESpellType::kScrollCast) {
		items = &spell_create[spell_id].scroll;
		skill_id = ESkill::kCreateScroll;
		create = 1;
	} else if (spell_type == ESpellType::kRunes) {
		items = &spell_create[spell_id].runes;
	} else {
		return (false);
	}
	item3 = items->rnumber;
	item0 = items->items[0];
	item1 = items->items[1];
	item2 = items->items[2];
	const int item0_rnum = item0 >= 0 ? GetObjRnum(item0) : -1;
	const int item1_rnum = item1 >= 0 ? GetObjRnum(item1) : -1;
	const int item2_rnum = item2 >= 0 ? GetObjRnum(item2) : -1;
	const int item3_rnum = item3 >= 0 ? GetObjRnum(item3) : -1;

	for (auto obj = ch->carrying; obj; obj = obj->get_next_content()) {
		if (item0 >= 0 && item0_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item0_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr)); //в вал3 сохраним время когда мы заюзали руну
			obj0 = obj;
			item0 = -2;
			objo = obj0;
			num++;
		} else if (item1 >= 0 && item1_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item1_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj1 = obj;
			item1 = -2;
			objo = obj1;
			num++;
		} else if (item2 >= 0 && item2_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item2_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj2 = obj;
			item2 = -2;
			objo = obj2;
			num++;
		} else if (item3 >= 0 && item3_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item3_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj3 = obj;
			item3 = -2;
			objo = obj3;
			num++;
		}
	}

	if (!objo ||
		(items->items[0] >= 0 && item0 >= 0) ||
		(items->items[1] >= 0 && item1 >= 0) ||
		(items->items[2] >= 0 && item2 >= 0) || (items->rnumber >= 0 && item3 >= 0)) {
		return (false);
	}

	if (extract) {
		if (spell_type == ESpellType::kRunes) {
			strcpy(buf, "Вы сложили ");
		} else {
			strcpy(buf, "Вы взяли ");
		}

		ObjData::shared_ptr obj;
		if (create) {
			obj = world_objects.create_from_prototype_by_vnum(obj_num);
			if (!obj) {
				return false;
			} else {
				percent = number(1, MUD::Skill(skill_id).difficulty);
				auto prob = CalcCurrentSkill(ch, skill_id, nullptr);

				if (MUD::Skills().IsValid(skill_id) && percent > prob) {
					percent = -1;
				}
			}
		}

		if (item0 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj0->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj0, 1), spell_type);
		}

		if (item1 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj1->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj1, 1), spell_type);
		}

		if (item2 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj2->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj2, 1), spell_type);
		}

		if (item3 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj3->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj3, 1), spell_type);
		}

		strcat(buf, kColorNrm);

		if (create) {
			if (percent >= 0) {
				strcat(buf, " и создали $o3.");
				act(buf, false, ch, obj.get(), nullptr, kToChar);
				act("$n создал$g $o3.", false, ch, obj.get(), nullptr, kToRoom | kToArenaListen);
				PlaceObjToInventory(obj.get(), ch);
			} else {
				strcat(buf, " и попытались создать $o3.\r\n" "Ничего не вышло.");
				act(buf, false, ch, obj.get(), nullptr, kToChar);
				ExtractObjFromWorld(obj.get());
			}
		} else {
			if (spell_type == ESpellType::kItemCast) {
				strcat(buf, "и создали магическую смесь.\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n смешал$g что-то в своей ноше.\r\n"
					"Вы почувствовали резкий запах.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			} else if (spell_type == ESpellType::kRunes) {
				sprintf(buf + strlen(buf),
						"котор%s вспыхнул%s ярким светом.%s",
						num > 1 ? "ые" : GET_OBJ_SUF_3(objo), num > 1 ? "и" : GET_OBJ_SUF_1(objo),
						ch->IsFlagged(EPrf::kCompact) ? "" : "\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n сложил$g руны, которые вспыхнули ярким пламенем.",
					true, ch, nullptr, nullptr, kToRoom);
				sprintf(buf, "$n сложил$g руны в заклинание '%s'%s%s.",
						MUD::Spell(spell_id).GetCName(),
						(tch && tch != ch ? " на " : ""),
						(tch && tch != ch ? GET_PAD(tch, 1) : ""));
				act(buf, true, ch, nullptr, nullptr, kToArenaListen);
				auto magic_skill = GetMagicSkillId(spell_id);
				if (MUD::Skills().IsValid(magic_skill)) {
					TrainSkill(ch, magic_skill, true, tch);
				}
			}
		}
		extract_item(ch, obj0, spell_type);
		extract_item(ch, obj1, spell_type);
		extract_item(ch, obj2, spell_type);
		extract_item(ch, obj3, spell_type);
	}
	return (true);
}

void print_rune_stats(CharData *ch) {
	if (!ch->IsGrGod()) {
		SendMsgToChar(ch, "Только для иммов 33+.\r\n");
		return;
	}

	std::multimap<int, int> tmp_list;
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
			 iend = rune_list.end(); i != iend; ++i) {
		tmp_list.insert(std::make_pair(i->second, i->first));
	}
	std::stringstream out;
	out << "Rune stats:\r\n" << "vnum -> count\r\n" << "--------------\r\n";

	for (std::multimap<int, int>::const_reverse_iterator i = tmp_list.rbegin(),
			 iend = tmp_list.rend(); i != iend; ++i) {
		
		out << i->second << " -> " << i->first << "\r\n";
	}
	SendMsgToChar(out.str(), ch);
}

void print_rune_log() {
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
			 iend = rune_list.end(); i != iend; ++i) {
		log("RuneUsed: %d %d", i->first, i->second);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
