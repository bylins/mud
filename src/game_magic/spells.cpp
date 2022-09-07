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
#include "structs/global_objects.h"
#include "cmd/follow.h"
#include "cmd/hire.h"
#include "depot.h"
#include "game_fight/mobact.h"
#include "handler.h"
#include "house.h"
#include "liquid.h"
#include "magic.h"
#include "obj_prototypes.h"
#include "communication/parcel.h"
#include "administration/privilege.h"
#include "color.h"
#include "game_skills/townportal.h"
#include "cmd/flee.h"
#include "stuff.h"
#include "utils/utils_char_obj.inl"

extern char cast_argument[kMaxInputLength];
extern im_type *imtypes;
extern int top_imtypes;

void weight_change_object(ObjData *obj, int weight);
int compute_armor_class(CharData *ch);
char *diag_weapon_to_char(const CObjectPrototype *obj, int show_wear);
void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
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
		&& GET_OBJ_TYPE(obj) == EObjType::kLiquidContainer) {
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
			SendMsgToChar("Прекратите, ради бога, химичить.\r\n", ch);
			return;
		} else {
			water = std::max(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0) {
				if (GET_OBJ_VAL(obj, 1) >= 0) {
					name_from_drinkcon(obj);
				}
				obj->set_val(2, LIQ_WATER);
				obj->add_val(1, water);
				act("Вы наполнили $o3 водой.", false, ch, obj, nullptr, kToChar);
				name_to_drinkcon(obj, LIQ_WATER);
				weight_change_object(obj, water);
			}
		}
	}
	if (victim && !victim->IsNpc() && !IS_IMMORTAL(victim)) {
		GET_COND(victim, THIRST) = 0;
		SendMsgToChar("Вы полностью утолили жажду.\r\n", victim);
		if (victim != ch) {
			act("Вы напоили $N3.", false, ch, nullptr, victim, kToChar);
		}
	}
}

#define SUMMON_FAIL "Попытка перемещения не удалась.\r\n"
#define SUMMON_FAIL2 "Ваша жертва устойчива к этому.\r\n"
#define SUMMON_FAIL3 "Магический кокон, окружающий вас, помешал заклинанию сработать правильно.\r\n"
#define SUMMON_FAIL4 "Ваша жертва в бою, подождите немного.\r\n"
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
			(!ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) || IS_IMMORTAL(ch)) &&
			Clan::MayEnter(ch, fnd_room, kHousePortal))
			break;
	}

	free(r_array);

	return n ? fnd_room : kNowhere;
}

void SpellRecall(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/) {
	RoomRnum to_room = kNowhere, fnd_room = kNowhere;
	RoomRnum rnum_start, rnum_stop;

	if (!victim || victim->IsNpc() || ch->in_room != IN_ROOM(victim) || GetRealLevel(victim) >= kLvlImmortal) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (!IS_GOD(ch)
		&& (ROOM_FLAGGED(IN_ROOM(victim), ERoomFlag::kNoTeleportOut) || AFF_FLAGGED(victim, EAffect::kNoTeleport))) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (victim != ch) {
		if (same_group(ch, victim)) {
			if (number(1, 100) <= 5) {
				SendMsgToChar(SUMMON_FAIL, ch);
				return;
			}
		} else if (!ch->IsNpc() || (ch->has_master()
			&& !ch->get_master()->IsNpc())) // игроки не в группе и  чармисы по приказу не могут реколить свитком
		{
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}

		if ((ch->IsNpc() && CalcGeneralSaving(ch, victim, ESaving::kWill, GetRealInt(ch))) || IS_GOD(victim)) {
			return;
		}
	}

	if ((to_room = real_room(GET_LOADROOM(victim))) == kNowhere)
		to_room = real_room(calc_loadroom(victim));

	if (to_room == kNowhere) {
		SendMsgToChar(SUMMON_FAIL, ch);
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
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (victim->GetEnemy() && (victim != ch)) {
		if (!pk_agro_action(ch, victim->GetEnemy()))
			return;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return;
	act("$n исчез$q.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	ExtractCharFromRoom(victim);
	PlaceCharToRoom(victim, fnd_room);
	victim->dismount();
	act("$n появил$u в центре комнаты.", true, victim, nullptr, nullptr, kToRoom);
	look_at_room(victim, 0);
	greet_mtrigger(victim, -1);
	greet_otrigger(victim, -1);
}

// ПРЫЖОК в рамках зоны
void SpellTeleport(int /* level */, CharData *ch, CharData */*victim*/, ObjData */*obj*/) {
	RoomRnum in_room = ch->in_room, fnd_room = kNowhere;
	RoomRnum rnum_start, rnum_stop;

	if (!IS_GOD(ch) && (ROOM_FLAGGED(in_room, ERoomFlag::kNoTeleportOut) || AFF_FLAGGED(ch, EAffect::kNoTeleport))) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	GetZoneRooms(world[in_room]->zone_rn, &rnum_start, &rnum_stop);
	fnd_room = GetTeleportTargetRoom(ch, rnum_start, rnum_stop);
	if (fnd_room == kNowhere) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return;
	act("$n медленно исчез$q из виду.", false, ch, nullptr, nullptr, kToRoom);
	ExtractCharFromRoom(ch);
	PlaceCharToRoom(ch, fnd_room);
	ch->dismount();
	act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
	look_at_room(ch, 0);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

void CheckAutoNosummon(CharData *ch) {
	if (PRF_FLAGGED(ch, EPrf::kAutonosummon) && PRF_FLAGGED(ch, EPrf::KSummonable)) {
		PRF_FLAGS(ch).unset(EPrf::KSummonable);
		SendMsgToChar("Режим автопризыв: вы защищены от призыва.\r\n", ch);
	}
}

void SpellRelocate(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/) {
	RoomRnum to_room, fnd_room;

	if (victim == nullptr)
		return;

	if (!IS_GOD(ch)) {
		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoTeleportOut)) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}

		if (AFF_FLAGGED(ch, EAffect::kNoTeleport)) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}
	}

	to_room = IN_ROOM(victim);

	if (to_room == kNowhere) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (!Clan::MayEnter(ch, to_room, kHousePortal)) {
		fnd_room = Clan::CloseRent(to_room);
	} else {
		fnd_room = to_room;
	}

	if (fnd_room != to_room && !IS_GOD(ch)) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (!IS_GOD(ch) &&
		(SECT(fnd_room) == ESector::kSecret ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kNoRelocateIn) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) || (ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) && !IS_IMMORTAL(
			ch)))) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return;
//	check_auto_nosummon(victim);
	act("$n медленно исчез$q из виду.", true, ch, nullptr, nullptr, kToRoom);
	SendMsgToChar("Лазурные сполохи пронеслись перед вашими глазами.\r\n", ch);
	ExtractCharFromRoom(ch);
	PlaceCharToRoom(ch, fnd_room);
	ch->dismount();
	look_at_room(ch, 0);
	act("$n медленно появил$u откуда-то.", true, ch, nullptr, nullptr, kToRoom);
	SetWaitState(ch, 2 * kPulseViolence);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

void SpellPortal(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/) {
	RoomRnum to_room, fnd_room;

	if (victim == nullptr)
		return;
	if (GetRealLevel(victim) > GetRealLevel(ch) && !PRF_FLAGGED(victim, EPrf::KSummonable) && !same_group(ch, victim)) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}
	// пентить чаров <=10 уровня, нельзя так-же нельзя пентать иммов
	if (!IS_GOD(ch)) {
		if ((!victim->IsNpc() && GetRealLevel(victim) <= 10 && GetRealRemort(ch) < 9) || IS_IMMORTAL(victim)
			|| AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}
	}
	if (victim->IsNpc()) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	fnd_room = IN_ROOM(victim);
	if (fnd_room == kNowhere) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (!IS_GOD(ch) && (SECT(fnd_room) == ESector::kSecret || ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) ||
		ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) || ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) ||
		ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) || ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom))) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (ch->in_room == fnd_room) {
		SendMsgToChar("Может, вам лучше просто потоптаться на месте?\r\n", ch);
		return;
	}

	if (world[fnd_room]->portal_time) {
		if (world[world[fnd_room]->portal_room]->portal_room == fnd_room
			&& world[world[fnd_room]->portal_room]->portal_time)
			decay_portal(world[fnd_room]->portal_room);
		decay_portal(fnd_room);
	}
	if (world[ch->in_room]->portal_time) {
		if (world[world[ch->in_room]->portal_room]->portal_room == ch->in_room
			&& world[world[ch->in_room]->portal_room]->portal_time)
			decay_portal(world[ch->in_room]->portal_room);
		decay_portal(ch->in_room);
	}
	bool pkPortal = pk_action_type_summon(ch, victim) == PK_ACTION_REVENGE ||
		pk_action_type_summon(ch, victim) == PK_ACTION_FIGHT;

	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(victim, EGf::kGodscurse)
		// раньше было <= PK_ACTION_REVENGE, что вызывало абьюз при пенте на чара на арене,
		// или пенте кидаемой с арены т.к. в данном случае использовалось PK_ACTION_NO которое меньше PK_ACTION_REVENGE
		|| pkPortal || ((!victim->IsNpc() || IS_CHARMICE(ch)) && PRF_FLAGGED(victim, EPrf::KSummonable))
		|| same_group(ch, victim)) {
		if (pkPortal) {
			pk_increment_revenge(ch, victim);
		}

		to_room = ch->in_room;
		world[fnd_room]->portal_room = to_room;
		world[fnd_room]->portal_time = 1;
		if (pkPortal) world[fnd_room]->pkPenterUnique = GET_UNIQUE(ch);

		if (pkPortal) {
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.",
				false, world[fnd_room]->first_character(), nullptr, nullptr, kToChar);
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.",
				false, world[fnd_room]->first_character(), nullptr, nullptr, kToRoom);
		} else {
			act("Лазурная пентаграмма возникла в воздухе.",
				false, world[fnd_room]->first_character(), nullptr, nullptr, kToChar);
			act("Лазурная пентаграмма возникла в воздухе.",
				false, world[fnd_room]->first_character(), nullptr, nullptr, kToRoom);
		}
		CheckAutoNosummon(victim);

		// если пенту ставит имм с привилегией arena (и находясь на арене), то пента получается односторонняя
		if (privilege::CheckFlag(ch, privilege::kArenaMaster) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)) {
			return;
		}

		world[to_room]->portal_room = fnd_room;
		world[to_room]->portal_time = 1;
		if (pkPortal) world[to_room]->pkPenterUnique = GET_UNIQUE(ch);

		if (pkPortal) {
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.",
				false, world[to_room]->first_character(), nullptr, nullptr, kToChar);
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.",
				false, world[to_room]->first_character(), nullptr, nullptr, kToRoom);
		} else {
			act("Лазурная пентаграмма возникла в воздухе.",
				false, world[to_room]->first_character(), nullptr, nullptr, kToChar);
			act("Лазурная пентаграмма возникла в воздухе.",
				false, world[to_room]->first_character(), nullptr, nullptr, kToRoom);
		}
	}
}

void SpellSummon(int /*level*/, CharData *ch, CharData *victim, ObjData */*obj*/) {
	RoomRnum ch_room, vic_room;
	struct FollowerType *k, *k_next;

	if (ch == nullptr || victim == nullptr || ch == victim) {
		return;
	}

	ch_room = ch->in_room;
	vic_room = IN_ROOM(victim);

	if (ch_room == kNowhere || vic_room == kNowhere) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (ch->IsNpc() && victim->IsNpc()) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	if (IS_IMMORTAL(victim)) {
		if (ch->IsNpc() || (!ch->IsNpc() && GetRealLevel(ch) < GetRealLevel(victim))) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}
	}

	if (!ch->IsNpc() && victim->IsNpc()) {
		if (victim->get_master() != ch  //поправим это, тут и так понято что чармис ()
			|| victim->GetEnemy()
			|| GET_POS(victim) < EPosition::kRest) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}
	}

	if (!IS_IMMORTAL(ch)) {
		if (!ch->IsNpc() || IS_CHARMICE(ch)) {
			if (AFF_FLAGGED(ch, EAffect::kShield)) {
				SendMsgToChar(SUMMON_FAIL3, ch);
				return;
			}
			if (!PRF_FLAGGED(victim, EPrf::KSummonable) && !same_group(ch, victim)) {
				SendMsgToChar(SUMMON_FAIL2, ch);
				return;
			}
			if (NORENTABLE(victim)) {
				SendMsgToChar(SUMMON_FAIL, ch);
				return;
			}
			if (victim->GetEnemy()) {
				SendMsgToChar(SUMMON_FAIL4, ch);
				return;
			}
		}
		if (victim->get_wait() > 0) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}
		if (!ch->IsNpc() && !victim->IsNpc() && GetRealLevel(victim) <= 10) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}

		if (ROOM_FLAGGED(ch_room, ERoomFlag::kNoSummonOut)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kDeathTrap)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kSlowDeathTrap)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kTunnel)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kNoBattle)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kGodsRoom)
			|| SECT(ch->in_room) == ESector::kSecret
			|| (!same_group(ch, victim)
				&& (ROOM_FLAGGED(ch_room, ERoomFlag::kPeaceful) || ROOM_FLAGGED(ch_room, ERoomFlag::kArena)))) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}
		// отдельно проверку на клан комнаты, своих чармисов призвать можем ()
		if (!Clan::MayEnter(victim, ch_room, kHousePortal) && !(victim->has_master()) && (victim->get_master() != ch)) {
			SendMsgToChar(SUMMON_FAIL, ch);
			return;
		}

		if (!ch->IsNpc()) {
			if (ROOM_FLAGGED(vic_room, ERoomFlag::kNoSummonOut)
				|| ROOM_FLAGGED(vic_room, ERoomFlag::kGodsRoom)
				|| !Clan::MayEnter(ch, vic_room, kHousePortal)
				|| AFF_FLAGGED(victim, EAffect::kNoTeleport)
				|| (!same_group(ch, victim)
					&& (ROOM_FLAGGED(vic_room, ERoomFlag::kTunnel) || ROOM_FLAGGED(vic_room, ERoomFlag::kArena)))) {
				SendMsgToChar(SUMMON_FAIL, ch);
				return;
			}
		} else {
			if (ROOM_FLAGGED(vic_room, ERoomFlag::kNoSummonOut) || AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
				SendMsgToChar(SUMMON_FAIL, ch);
				return;
			}
		}

		if (ch->IsNpc() && number(1, 100) < 30) {
			return;
		}
	}
	if (!enter_wtrigger(world[ch_room], ch, -1))
		return;
	act("$n растворил$u на ваших глазах.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	ExtractCharFromRoom(victim);
	PlaceCharToRoom(victim, ch_room);
	victim->dismount();
	act("$n прибыл$g по вызову.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	act("$n призвал$g вас!", false, ch, nullptr, victim, kToVict);
	CheckAutoNosummon(victim);
	GET_POS(victim) = EPosition::kStand;
	look_at_room(victim, 0);
	// призываем чармисов
	for (k = victim->followers; k; k = k_next) {
		k_next = k->next;
		if (IN_ROOM(k->follower) == vic_room) {
			if (AFF_FLAGGED(k->follower, EAffect::kCharmed)) {
				if (!k->follower->GetEnemy()) {
					act("$n растворил$u на ваших глазах.",
						true, k->follower, nullptr, nullptr, kToRoom | kToArenaListen);
					ExtractCharFromRoom(k->follower);
					PlaceCharToRoom(k->follower, ch_room);
					act("$n прибыл$g за хозяином.",
						true, k->follower, nullptr, nullptr, kToRoom | kToArenaListen);
					act("$n призвал$g вас!", false, ch, nullptr, k->follower, kToVict);
				}
			}
		}
	}
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

	int tmp_lvl = (IS_GOD(ch)) ? 300 : level;
	int count = tmp_lvl;
	const auto result = world_objects.find_if_and_dec_number([&](const ObjData::shared_ptr &i) {
		const auto obj_ptr = world_objects.get_by_raw_ptr(i.get());
		if (!obj_ptr) {
			sprintf(buf, "SYSERR: Illegal object iterator while locate");
			mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);

			return false;
		}

		bloody_corpse = false;
		if (!IS_GOD(ch)) {
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

			if (!VALID_RNUM(IN_ROOM(carried_by))) {
				sprintf(buf,
						"SYSERR: Illegal room %d, char %s. Создана кора для исследований",
						IN_ROOM(carried_by),
						carried_by->get_name().c_str());
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);
				return false;
			}

			if (SECT(IN_ROOM(carried_by)) == ESector::kSecret || IS_IMMORTAL(carried_by)) {
				return false;
			}
		}

		if (!isname(name, i->get_aliases())) {
			return false;
		}
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
				if (!IS_GOD(ch)) {
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
						i->get_in_obj()->get_PName(5).c_str());
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
		} else {
			sprintf(buf, "Местоположение %s неопределимо.\r\n", OBJN(i.get(), ch, 1));
		}

//		CAP(buf); issue #59
		SendMsgToChar(buf, ch);

		return true;
	}, count);

	int j = count;
	if (j > 0) {
		j = Clan::print_spell_locate_object(ch, j, std::string(name));
	}

	if (j > 0) {
		j = Depot::print_spell_locate_object(ch, j, std::string(name));
	}

	if (j > 0) {
		j = Parcel::print_spell_locate_object(ch, j, std::string(name));
	}

	if (j == tmp_lvl) {
		SendMsgToChar("Вы ничего не чувствуете.\r\n", ch);
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
	struct FollowerType *k;
	int cha_summ = 0, reformed_hp_summ = 0;
	bool undead_in_group = false, living_in_group = false;

	for (k = ch->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->follower, EAffect::kCharmed)
			&& k->follower->get_master() == ch) {
			cha_summ++;
			//hp_summ += GET_REAL_MAX_HIT(k->ch);
			reformed_hp_summ += GetReformedCharmiceHp(ch, k->follower, spell_id);
// Проверка на тип последователей -- некрасиво, зато эффективно
			if (MOB_FLAGGED(k->follower, EMobFlag::kCorpse)) {
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
	if (victim == nullptr || ch == nullptr)
		return;

	if (victim == ch)
		SendMsgToChar("Вы просто очарованы своим внешним видом!\r\n", ch);
	else if (!victim->IsNpc()) {
		SendMsgToChar("Вы не можете очаровать реального игрока!\r\n", ch);
		if (!pk_agro_action(ch, victim))
			return;
	} else if (!IS_IMMORTAL(ch)
		&& (AFF_FLAGGED(victim, EAffect::kSanctuary) || MOB_FLAGGED(victim, EMobFlag::kProtect)))
		SendMsgToChar("Ваша жертва освящена Богами!\r\n", ch);
	else if (!IS_IMMORTAL(ch) && (AFF_FLAGGED(victim, EAffect::kShield) || MOB_FLAGGED(victim, EMobFlag::kProtect)))
		SendMsgToChar("Ваша жертва защищена Богами!\r\n", ch);
	else if (!IS_IMMORTAL(ch) && MOB_FLAGGED(victim, EMobFlag::kNoCharm))
		SendMsgToChar("Ваша жертва устойчива к этому!\r\n", ch);
	else if (AFF_FLAGGED(ch, EAffect::kCharmed))
		SendMsgToChar("Вы сами очарованы кем-то и не можете иметь последователей.\r\n", ch);
	else if (AFF_FLAGGED(victim, EAffect::kCharmed)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressive)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressiveMono)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressivePoly)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressiveDay)
		|| MOB_FLAGGED(victim, EMobFlag::kAggressiveNight)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressiveFullmoon)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressiveWinter)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressiveSpring)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressiveSummer)
		|| MOB_FLAGGED(victim, EMobFlag::kAgressiveAutumn))
		SendMsgToChar("Ваша магия потерпела неудачу.\r\n", ch);
	else if (IS_HORSE(victim))
		SendMsgToChar("Это боевой скакун, а не хухры-мухры.\r\n", ch);
	else if (victim->GetEnemy() || GET_POS(victim) < EPosition::kRest)
		act("$M сейчас, похоже, не до вас.", false, ch, nullptr, victim, kToChar);
	else if (circle_follow(victim, ch))
		SendMsgToChar("Следование по кругу запрещено.\r\n", ch);
	else if (!IS_IMMORTAL(ch)
		&& CalcGeneralSaving(ch, victim, ESaving::kWill, (GetRealCha(ch) - 10) * 4 + GetRealRemort(ch) * 3)) //предлагаю завязать на каст
		SendMsgToChar("Ваша магия потерпела неудачу.\r\n", ch);
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
			mobRemember(victim, ch);
		}

		if (MOB_FLAGGED(victim, EMobFlag::kNoGroup))
			MOB_FLAGS(victim).unset(EMobFlag::kNoGroup);

		RemoveAffectFromChar(victim, ESpell::kCharm);
		ch->add_follower(victim);
		Affect<EApply> af;
		af.type = ESpell::kCharm;

		if (GetRealInt(victim) > GetRealInt(ch)) {
			af.duration = CalcDuration(victim, GetRealCha(ch), 0, 0, 0, 0);
		} else {
			af.duration = CalcDuration(victim, GetRealCha(ch) + number(1, 10) + GetRealRemort(ch) * 2, 0, 0, 0, 0);
		}

		af.modifier = 0;
		af.location = EApply::kNone;
		af.bitvector = to_underlying(EAffect::kCharmed);
		af.battleflag = 0;
		affect_to_char(victim, af);
		// резервируем место под фит ()
		if (CanUseFeat(ch, EFeat::kAnimalMaster) &&
		GET_RACE(victim) == 104) {
			act("$N0 обрел$G часть вашей магической силы, и стал$G намного опаснее...",
				false, ch, nullptr, victim, kToChar);
			act("$N0 обрел$G часть магической силы $n1.",
				false, ch, nullptr, victim, kToRoom | kToArenaListen);
			// начинаем модификации victim
			// создаем переменные модификаторов
			int r_cha = GetRealCha(ch);
			int perc = ch->GetSkill(GetMagicSkillId(ESpell::kCharm));
			ch->send_to_TC(false, true, false, "Значение хари:  %d.\r\n", r_cha);
			ch->send_to_TC(false, true, false, "Значение скила магии: %d.\r\n", perc);
			
			// вычисляем % владения умений у victim
			k_skills = floorf(0.8*r_cha + 0.5*perc);
			ch->send_to_TC(false, true, false, "Владение скилом: %d.\r\n", k_skills);
			// === Формируем новые статы ===
			// Устанавливаем на виктим флаг маг-сумон (маг-зверь)
			af.bitvector = to_underlying(EAffect::kHelper);
			affect_to_char(victim, af);
			MOB_FLAGS(victim).set(EMobFlag::kSummoned);
			// Модифицируем имя в зависимости от хари
			static char descr[kMaxStringLength];
			int gender;
			// ниже идет просто порнуха
			// по идее могут быть случаи "огромная огромная макака" или "громадная большая собака"
			// как бороться думаю
			// Sventovit:
			// Для начала - вынести повторяющийся много раз кусок кода в функцию и вызывать её.
			// Также вынести отсюда стену строковых констант.
			// Тело функции в идеале должно занимать не более трех строк, никак не 30 экранов.
			// У функции в идеале не должно быть параметров. В допустимом пределе - три параметра.
			// Если их больше - программист что-то не так делает.
			// А бороться - просто сравнивать добавляемое слово, если оно уже есть - то не добавлять.
			// Варнинги по поводу неявного приведения типов тоже стоит почистить.
			const char *state[4][9][6] = {
							{  						
							{"крепкие",  "крепких", "крепким", "крепкие", "крепкими", "крепких"},
							{"сильные",  "сильных", "сильным", "сильные", "сильными", "сильных"},
							{"упитанные",  "упитанных", "упитанным", "упитанных", "упитанными", "упитанных"},
							{"крупные",  "крупные", "крупным", "крупные", "крупными", "крупных"},
							{"большые",  "большые", "большым", "большых", "большыми", "большых"},
							{"громадные", "громадные", "громадным", "громадные", "громадными", "громадных"},
							{"огромные",  "огромных", "огромным", "огромные", "огромными", "огромных"},
							{"исполинские",  "исполинские", "исполинским", "исполинские", "исполинскими", "исполинских"},
							{"гигантские" ,"гигантские", "гигантские", "гигантские", "гигантские", "гигантские"},
							},
			 				{ // род ОН
							{"крепкий",  "крепкого", "крепкому", "крепкого", "крепким", "крепком"},
							{"сильный",  "сильного", "сильному", "сильного", "сильным", "сильном"},
							{"упитанный",  "упитанного", "упитанному", "упитанного", "упитанным", "упитанном"},
							{"крупный",  "крупного", "крупному", "крупного", "крупным", "крупном"},
							{"большой",  "большого", "большому", "большого", "большым", "большом"},
							{"громадный",  "громадного", "громадному", "громадного", "громадным", "громадном"},
							{"огромный",  "огромного", "огромному", "огромного", "огромным", "огромном"},
							{"исполинский",  "исполинского", "исполинскому", "исполинского", "исполинским", "исполинском"},
							{"гигантский",  "гигантского", "гигантскому", "гигантского", "гигантским", "гигантском"},
							},
			 				{ // род ОНА
							{"крепкая", "крепкой", "крепкой", "крепкую", "крепкой", "крепкой"},
							{"сильная", "сильной", "сильной", "сильную", "сильной", "сильной"},
							{"упитанная", "упитанной", "упитанной", "упитанную", "упитанной", "упитанной"},
							{"крупная",  "крупной", "крупной", "крупную", "крупной", "крупной"},
							{"большая", "большой", "большой", "большую", "большой", "большой"},
							{"громадная",  "громадной", "громадной", "громадную", "громадной", "громадной"},
							{"огромная",  "огромной", "огромной", "огромную", "огромной", "огромной"},
							{"исполинская", "исполинской", "исполинской", "исполинскую", "исполинской", "исполинской"},
							{"гигантская",  "гигантской", "гигантской", "гигантскую", "гигантской", "гигантской"},
							},
			 				{  // род ОНО
							{"крепкое", "крепкое", "крепкому", "крепкое", "крепким", "крепком"},
							{"сильное",  "сильное", "сильному", "сильное", "сильным", "сильном"},
							{"упитанное","упитанное", "упитанному", "упитанное", "упитанным", "упитанном"},
							{"крупное", "крупное", "крупному", "крупное", "крупным", "крупном"},
							{"большое",  "большое", "большому", "большое", "большым", "большом"},
							{"громадное", "громадное", "громадному", "громадное", "громадным", "громадном"},
							{"огромное",  "огромное", "огромному", "огромное", "огромным", "огромном"},
							{"исполинское",  "исполинское", "исполинскому", "исполинское", "исполинским", "исполинском"},
							{"гигантское" , "гигантское", "гигантскому", "гигантское", "гигантским", "гигантском"},
							}
							};
			//проверяем GENDER 
			switch ((victim)->get_sex()) {
					case EGender::kNeutral:
					gender = 0;
					break;
					case EGender::kMale:
					gender = 1;
					break;
					case EGender::kFemale:
					gender = 2;
					break;
					default:
					gender = 3;
					break;
			}
 		// 1 при 10-19, 2 при 20-29 , 3 при 30-39....
			int adj = r_cha/10;
			sprintf(descr, "%s %s %s", state[gender][adj - 1][0], GET_PAD(victim, 0), GET_NAME(victim));
			victim->SetCharAliases(descr);
			sprintf(descr, "%s %s", state[gender][adj - 1][0], GET_PAD(victim, 0));
			victim->set_npc_name(descr);
			sprintf(descr, "%s %s", state[gender][adj - 1][0], GET_PAD(victim, 0));
			victim->player_data.PNames[0] = std::string(descr);
			sprintf(descr, "%s %s", state[gender][adj - 1][1], GET_PAD(victim, 1));
			victim->player_data.PNames[1] = std::string(descr);
			sprintf(descr, "%s %s", state[gender][adj - 1][2], GET_PAD(victim, 2));
			victim->player_data.PNames[2] = std::string(descr);
			sprintf(descr, "%s %s", state[gender][adj - 1][3], GET_PAD(victim, 3));
			victim->player_data.PNames[3] = std::string(descr);
			sprintf(descr, "%s %s", state[gender][adj - 1][4], GET_PAD(victim, 4));
			victim->player_data.PNames[4] = std::string(descr);
			sprintf(descr, "%s %s", state[gender][adj - 1][5], GET_PAD(victim, 5));
			victim->player_data.PNames[5] = std::string(descr);
				
			// прибавка хитов по формуле: 1/3 хп_хозяина + 12*лвл_хоз + 4*обая_хоз + 1.5*%магии_хоз
			GET_MAX_HIT(victim) += floorf(GET_MAX_HIT(ch)*0.33 + GetRealLevel(ch)*12 + r_cha*4 + perc*1.5);
			GET_HIT(victim) = GET_MAX_HIT(victim);
			// статы
			victim->set_int(floorf((r_cha*0.2 + perc*0.15)));
			victim->set_dex(floorf((r_cha*0.3 + perc*0.15)));
			victim->set_str(floorf((r_cha*0.3 + perc*0.15)));
			victim->set_con(floorf((r_cha*0.3 + perc*0.15)));
			victim->set_wis(floorf((r_cha*0.2 + perc*0.15)));
			victim->set_cha(floorf((r_cha*0.2 + perc*0.15)));
			// боевые показатели
			GET_INITIATIVE(victim) = floorf(k_skills/4.0);	// инициатива
			GET_MORALE(victim) = floorf(k_skills/5.0); 		// удача
			GET_HR(victim) = floorf(r_cha/3.5 + perc/10.0);  // попадание
			GET_AC(victim) = -floorf(r_cha/5.0 + perc/15.0); // АС
			GET_DR(victim) = floorf(r_cha/6.0 + perc/20.0);  // дамрол
			GET_ARMOUR(victim) = floorf(r_cha/4.0 + perc/10.0); // броня
			 // почему-то не работает
			if (GetRealRemort(ch) > 12) {
				GET_AR(victim) = (GET_AR(victim) + GetRealRemort(ch) - 12);
				GET_MR(victim) = (GET_MR(victim) + GetRealRemort(ch) - 12);
				GET_PR(victim) = (GET_PR(victim) + GetRealRemort(ch) - 12);
			}
			// спелы не работают пока 
			// SET_SPELL_MEM(victim, SPELL_CURE_BLIND, 1); // -?
			// SET_SPELL_MEM(victim, SPELL_REMOVE_DEAFNESS, 1); // -?
			// SET_SPELL_MEM(victim, SPELL_REMOVE_HOLD, 1); // -?
			// SET_SPELL_MEM(victim, SPELL_REMOVE_POISON, 1); // -?
			// SET_SPELL_MEM(victim, SPELL_HEAL, 1);

			//NPC_FLAGS(victim).set(NPC_WIELDING); // тут пока закомитим
			GET_LIKES(victim) = 10 + r_cha; // устанавливаем возможность авто применения умений
			
			// создаем кубики и доп атаки (пока без + а просто сет)
			victim->mob_specials.damnodice = floorf((r_cha*1.3 + perc*0.2) / 5.0);
			victim->mob_specials.damsizedice = floorf((r_cha*1.2 + perc*0.1) / 11.0);
			victim->mob_specials.extra_attack = floorf((r_cha*1.2 + perc) / 120.0);
			

			// простые аффекты
			if (r_cha > 25)  {
				af.bitvector = to_underlying(EAffect::kInfravision);
				affect_to_char(victim, af);
			} 
			 if (r_cha >= 30) {
				af.bitvector = to_underlying(EAffect::kDetectInvisible);
				affect_to_char(victim, af);
			} 
			if (r_cha >= 35) {
				af.bitvector = to_underlying(EAffect::kFly);
				affect_to_char(victim, af);
			} 
			if (r_cha >= 39) {	
				af.bitvector = to_underlying(EAffect::kStoneHands);
				affect_to_char(victim, af);
			}
			
			// расщет крутых маг аффектов
			if (r_cha > 56) {
				af.bitvector = to_underlying(EAffect::kShadowCloak);
				affect_to_char(victim, af);
			} 
			
			if ((r_cha > 65) && (r_cha < 74)) {
				af.bitvector = to_underlying(EAffect::kFireShield);
			} else if ((r_cha >= 74) && (r_cha < 82)){
				af.bitvector = to_underlying(EAffect::kAirShield);
			} else if (r_cha >= 82) {
				af.bitvector = to_underlying(EAffect::kIceShield);
				affect_to_char(victim, af);
				af.bitvector = to_underlying(EAffect::kBrokenChains);
			}
			affect_to_char(victim, af);
			

			// выбираем тип бойца - рандомно из 8 вариантов
			int rnd = number(1, 8);
			switch (rnd)
			{ // готовим наборы скиллов / способностей
			case 1:
				act("Лапы $N1 увеличились в размерах и обрели огромную, дикую мощь.\nТуловище $N1 стало огромным.",
					false, ch, nullptr, victim, kToChar); // тут потом заменим на валидные фразы
				act("Лапы $N1 увеличились в размерах и обрели огромную, дикую мощь.\nТуловище $N1 стало огромным.",
					false, ch, nullptr, victim, kToRoom | kToArenaListen);
				victim->set_skill(ESkill::kHammer, k_skills);
				victim->set_skill(ESkill::kRescue, k_skills*0.8);
				victim->set_skill(ESkill::kPunch, k_skills*0.9);
				victim->set_skill(ESkill::kNoParryHit, k_skills*0.4);
				victim->set_skill(ESkill::kIntercept, k_skills*0.75);
				victim->SetFeat(EFeat::kPunchMaster);
					if (floorf(r_cha*0.9 + perc/5.0) > number(1, 150)) {
					victim->SetFeat(EFeat::kPunchFocus);
					victim->set_skill(ESkill::kStrangle, k_skills);
					victim->SetFeat(EFeat::kBerserker);
					act("&B$N0 теперь сможет просто удавить всех своих врагов.&n\n",
						false, ch, nullptr, victim, kToChar);
				}
				victim->set_str(floorf(GetRealStr(victim)*1.3));
				skill_id = ESkill::kPunch;
				break;
			case 2:
				act("Лапы $N1 удлинились и на них выросли гиганские острые когти.\nТуловище $N1 стало более мускулистым.",
					false, ch, nullptr, victim, kToChar);
				act("Лапы $N1 удлинились и на них выросли гиганские острые когти.\nТуловище $N1 стало более мускулистым.",
					false, ch, nullptr, victim, kToRoom | kToArenaListen);
				victim->set_skill(ESkill::kOverwhelm, k_skills);
				victim->set_skill(ESkill::kRescue, k_skills*0.8);
				victim->set_skill(ESkill::kTwohands, k_skills*0.95);
				victim->set_skill(ESkill::kNoParryHit, k_skills*0.4);
				victim->SetFeat(EFeat::kTwohandsMaster);
				victim->SetFeat(EFeat::kTwohandsFocus);
				if (floorf(r_cha + perc/5.0) > number(1, 150)) {
					act("&G$N0 стал$G намного более опасным хищником.&n\n",
						false, ch, nullptr, victim, kToChar);
					victim->set_skill(ESkill::kFirstAid, k_skills*0.4);
					victim->set_skill(ESkill::kParry, k_skills*0.7);
				}
				victim->set_str(floorf(GetRealStr(victim)*1.2));
				skill_id = ESkill::kTwohands;
				break;
			case 3:
				act("Когти на лапах $N1 удлинились в размерах и приобрели зеленоватый оттенок.\nДвижения $N1 стали более размытими.",
					false, ch, nullptr, victim, kToChar);
				act("Когти на лапах $N1 удлинились в размерах и приобрели зеленоватый оттенок.\nДвижения $N1 стали более размытими.",
					false, ch, nullptr, victim, kToRoom | kToArenaListen);
				victim->set_skill(ESkill::kBackstab, k_skills);
				victim->set_skill(ESkill::kRescue, k_skills*0.6);
				victim->set_skill(ESkill::kPicks, k_skills*0.75);
				victim->set_skill(ESkill::kPoisoning, k_skills*0.7);
				victim->set_skill(ESkill::kNoParryHit, k_skills*0.75);
				victim->SetFeat(EFeat::kPicksMaster);
				victim->SetFeat(EFeat::kThieveStrike);
				if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
					victim->SetFeat(EFeat::kShadowStrike);
					act("&c$N0 затаил$U в вашей тени...&n\n", false, ch, nullptr, victim, kToChar);
					
				}
				victim->set_dex(floorf(GetRealDex(victim)*1.3));
				skill_id = ESkill::kPicks;
				break;
			case 4:
				act("Рефлексы $N1 обострились и туловище раздалось в ширь.\nНа огромных лапах засияли мелкие острые коготки.",
					false, ch, nullptr, victim, kToChar);
				act("Рефлексы $N1 обострились и туловище раздалось в ширь.\nНа огромных лапах засияли мелкие острые коготки.",
					false, ch, nullptr, victim, kToRoom | kToArenaListen);
				victim->set_skill(ESkill::kAwake, k_skills);
				victim->set_skill(ESkill::kRescue, k_skills*0.85);
				victim->set_skill(ESkill::kShieldBlock, k_skills*0.75);
				victim->set_skill(ESkill::kAxes, k_skills*0.85);
				victim->set_skill(ESkill::kNoParryHit, k_skills*0.65);
				if (floorf(r_cha*0.9 + perc/5.0) > number(1, 140)) {
					victim->set_skill(ESkill::kProtect, k_skills*0.75);
					act("&WЧуткий взгяд $N1 остановился на вас и вы ощутили себя под защитой.&n\n",
						false, ch, nullptr, victim, kToChar);
					victim->set_protecting(ch);
				}
				victim->SetFeat(EFeat::kAxesMaster);
				victim->SetFeat(EFeat::kThieveStrike);
				victim->SetFeat(EFeat::kDefender);
				victim->SetFeat(EFeat::kLiveShield);
				victim->set_con(floorf(GetRealCon(victim)*1.3));
				victim->set_str(floorf(GetRealStr(victim)*1.2));
				skill_id = ESkill::kAxes;
				break;
			case 5:
				act("Движения $N1 сильно ускорились, из туловища выросло несколько новых лап.\nКоторые покрылись длинными когтями.",
					false, ch, nullptr, victim, kToChar);
				act("Движения $N1 сильно ускорились, из туловища выросло несколько новых лап.\nКоторые покрылись длинными когтями.",
					false, ch, nullptr, victim, kToRoom | kToArenaListen);
				victim->set_skill(ESkill::kUndercut, k_skills);
				victim->set_skill(ESkill::kDodge, k_skills*0.7);
				victim->set_skill(ESkill::kAddshot, k_skills*0.7);
				victim->set_skill(ESkill::kBows, k_skills*0.85);
				victim->set_skill(ESkill::kRescue, k_skills*0.65);
				victim->set_skill(ESkill::kNoParryHit, k_skills*0.5);
				victim->SetFeat(EFeat::kThieveStrike);
				victim->SetFeat(EFeat::kBowsMaster);
				if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
					af.bitvector = to_underlying(EAffect::kCloudOfArrows);
					act("&YВокруг когтей $N1 засияли яркие магические всполохи.&n\n",
						false, ch, nullptr, victim, kToChar);
					affect_to_char(victim, af);
				}
				victim->set_dex(floorf(GetRealDex(victim)*1.2));
				victim->set_str(floorf(GetRealStr(victim)*1.15));
				victim->mob_specials.extra_attack = floorf((r_cha*1.2 + perc) / 180.0); // срежем доп атаки
				skill_id = ESkill::kBows;
				break;
			case 6:
				act("Туловище $N1 увеличилось, лапы сильно удлинились.\nНа них выросли острые когти-шипы.",
					false, ch, nullptr, victim, kToChar);
				act("Туловище $N1 увеличилось, лапы сильно удлинились.\nНа них выросли острые когти-шипы.",
					false, ch, nullptr, victim, kToRoom | kToArenaListen);
				victim->set_skill(ESkill::kClubs, k_skills);
				victim->set_skill(ESkill::kThrow, k_skills*0.85);
				victim->set_skill(ESkill::kDodge, k_skills*0.7);
				victim->set_skill(ESkill::kRescue, k_skills*0.6);
				victim->set_skill(ESkill::kNoParryHit, k_skills*0.6);
				victim->SetFeat(EFeat::kClubsMaster);
				victim->SetFeat(EFeat::kDoubleThrower);
				victim->SetFeat(EFeat::kTripleThrower);
				victim->SetFeat(EFeat::kPowerThrow);
				victim->SetFeat(EFeat::kDeadlyThrow);
				if (floorf(r_cha*0.8 + perc/5.0) > number(1, 140)) {
					victim->SetFeat(EFeat::kShadowThrower);
					victim->SetFeat(EFeat::kShadowClub);
					victim->set_skill(ESkill::kDarkMagic, k_skills*0.7);
					act("&cКогти $N1 преобрели &Kчерный цвет&c, будто смерть коснулась их.&n\n",
						false, ch, nullptr, victim, kToChar);
					victim->mob_specials.extra_attack = floorf((r_cha*1.2 + perc) / 100.0);
				}
				victim->set_str(floorf(GetRealStr(victim)*1.25));
				
				skill_id = ESkill::kClubs;
			break;
			case 7:
				act("Туловище $N1 увеличилось, мышцы налились дикой силой.\nА когти на лапах удлинились и заострились.",
					false, ch, nullptr, victim, kToChar);
				act("Туловище $N1 увеличилось, мышцы налились дикой силой.\nА когти на лапах удлинились и заострились.",
					false, ch, nullptr, victim, kToRoom | kToArenaListen);
				victim->set_skill(ESkill::kLongBlades, k_skills);
				victim->set_skill(ESkill::kKick, k_skills*0.95);
				victim->set_skill(ESkill::kNoParryHit, k_skills*0.7);
				victim->set_skill(ESkill::kRescue, k_skills*0.4);
				victim->SetFeat(EFeat::kLongsMaster);
			
				if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
					victim->set_skill(ESkill::kIronwind, k_skills*0.8);
					victim->SetFeat(EFeat::kBerserker);
					act("&mДвижения $N1 сильно ускорились, и в глазах появились &Rогоньки&m безумия.&n\n",
						false, ch, nullptr, victim, kToChar);
				}
				victim->set_dex(floorf(GetRealDex(victim)*1.1));
				victim->set_str(floorf(GetRealStr(victim)*1.35));
				
				skill_id = ESkill::kLongBlades;
			break;		
			default:
				act("Рефлексы $N1 обострились, а передние лапы сильно удлинились.\nНа них выросли острые когти.",
					false, ch, nullptr, victim, kToChar);
				act("Рефлексы $N1 обострились, а передние лапы сильно удлинились.\nНа них выросли острые когти.",
					false, ch, nullptr, victim, kToRoom | kToArenaListen);
				victim->set_skill(ESkill::kParry, k_skills);
				victim->set_skill(ESkill::kRescue, k_skills*0.75);
				victim->set_skill(ESkill::kThrow, k_skills*0.95);
				victim->set_skill(ESkill::kSpades, k_skills*0.9);
				victim->set_skill(ESkill::kNoParryHit, k_skills*0.6);
				victim->SetFeat(EFeat::kLiveShield);
				victim->SetFeat(EFeat::kSpadesMaster);
								
				if (floorf(r_cha*0.9 + perc/4.0) > number(1, 140)) {
					victim->SetFeat(EFeat::kShadowThrower);
					victim->SetFeat(EFeat::kShadowSpear);
					victim->set_skill(ESkill::kDarkMagic, k_skills*0.8);
					act("&KКогти $N1 преобрели темный оттенок, будто сама тьма коснулась их.&n\n",
						false, ch, nullptr, victim, kToChar);
				}
				
				victim->SetFeat(EFeat::kDoubleThrower);
				victim->SetFeat(EFeat::kTripleThrower);
				victim->SetFeat(EFeat::kPowerThrow);
				victim->SetFeat(EFeat::kDeadlyThrow);
				victim->set_str(floorf(GetRealStr(victim)*1.2));
				victim->set_con(floorf(GetRealCon(victim)*1.2));
				skill_id = ESkill::kSpades;
				break;
			}

		}

		if (victim->helpers) {
			victim->helpers = nullptr;
		}

		act("$n покорил$g ваше сердце настолько, что вы готовы на все ради н$s.",
			false, ch, nullptr, victim, kToVict);
		if (victim->IsNpc()) {
//Eli. Раздеваемся.
			if (victim->IsNpc() && !MOB_FLAGGED(victim, EMobFlag::kSummoned)) { // только если не маг зверьки ()
				for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
					if (GET_EQ(victim, i)) {
						if (!remove_otrigger(GET_EQ(victim, i), victim)) {
							continue;
						}

						act("Вы прекратили использовать $o3.",
							false, victim, GET_EQ(victim, i), nullptr, kToChar);
						act("$n прекратил$g использовать $o3.",
							true, victim, GET_EQ(victim, i), nullptr, kToRoom);
						PlaceObjToInventory(UnequipChar(victim, i, CharEquipFlag::show_msg), victim);
					}
				}
			}
//Eli закончили раздеваться.
			MOB_FLAGS(victim).unset(EMobFlag::kAgressive);
			MOB_FLAGS(victim).unset(EMobFlag::kSpec);
			PRF_FLAGS(victim).unset(EPrf::kPunctual);
			MOB_FLAGS(victim).set(EMobFlag::kNoSkillTrain);
			victim->set_skill(ESkill::kPunctual, 0);
			// по идее при речарме и последующем креше можно оказаться с сейвом без шмота на чармисе -- Krodo
			if (!NPC_FLAGGED(ch, ENpcFlag::kNoMercList))
				ch->updateCharmee(GET_MOB_VNUM(victim), 0);
			Crash_crashsave(ch);
			ch->save_char();
		}
	}
	// тут обрабатываем, если виктим маг-зверь => передаем в фунцию создание маг шмоток (цель, базовый скил, процент владения)
	if (MOB_FLAGGED(victim, EMobFlag::kSummoned)) {
		create_charmice_stuff(victim, skill_id, k_skills);
	}
}

void show_weapon(CharData *ch, ObjData *obj) {
	if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
		*buf = '\0';
		if (CAN_WEAR(obj, EWearFlag::kWield)) {
			sprintf(buf, "Можно взять %s в правую руку.\r\n", OBJN(obj, ch, 3));
		}

		if (CAN_WEAR(obj, EWearFlag::kHold)) {
			sprintf(buf + strlen(buf), "Можно взять %s в левую руку.\r\n", OBJN(obj, ch, 3));
		}

		if (CAN_WEAR(obj, EWearFlag::kBoth)) {
			sprintf(buf + strlen(buf), "Можно взять %s в обе руки.\r\n", OBJN(obj, ch, 3));
		}

		if (*buf) {
			SendMsgToChar(buf, ch);
		}
	}
}

void print_book_uprgd_skill(CharData *ch, const ObjData *obj) {
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

void mort_show_obj_values(const ObjData *obj, CharData *ch, int fullness, bool enhansed_scroll) {
	int i, found, drndice = 0, drsdice = 0, j;
	long int li;

	SendMsgToChar("Вы узнали следующее:\r\n", ch);
	sprintf(buf, "Предмет \"%s\", тип : ", obj->get_short_description().c_str());
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
	strcat(buf, buf2);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	strcpy(buf, diag_weapon_to_char(obj, 2));
	if (*buf)
		SendMsgToChar(buf, ch);

	if (fullness < 20)
		return;

	//show_weapon(ch, obj);

	sprintf(buf, "Вес: %d, Цена: %d, Рента: %d(%d)\r\n",
			GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_RENTEQ(obj));
	SendMsgToChar(buf, ch);

	if (fullness < 30)
		return;
	sprinttype(obj->get_material(), material_name, buf2);
	snprintf(buf, kMaxStringLength, "Материал : %s, макс.прочность : %d, тек.прочность : %d\r\n", buf2,
			 obj->get_maximum_durability(), obj->get_current_durability());
	SendMsgToChar(buf, ch);
	SendMsgToChar(CCNRM(ch, C_NRM), ch);

	if (fullness < 40)
		return;

	SendMsgToChar("Неудобен : ", ch);
	SendMsgToChar(CCCYN(ch, C_NRM), ch);
	obj->get_no_flags().sprintbits(no_bits, buf, ",", IS_IMMORTAL(ch) ? 4 : 0);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);
	SendMsgToChar(CCNRM(ch, C_NRM), ch);

	if (fullness < 50)
		return;

	SendMsgToChar("Недоступен : ", ch);
	SendMsgToChar(CCCYN(ch, C_NRM), ch);
	obj->get_anti_flags().sprintbits(anti_bits, buf, ",", IS_IMMORTAL(ch) ? 4 : 0);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);
	SendMsgToChar(CCNRM(ch, C_NRM), ch);

	if (obj->get_auto_mort_req() > 0) {
		SendMsgToChar(ch, "Требует перевоплощений : %s%d%s\r\n",
					  CCCYN(ch, C_NRM), obj->get_auto_mort_req(), CCNRM(ch, C_NRM));
	} else if (obj->get_auto_mort_req() < -1) {
		SendMsgToChar(ch, "Максимальное количество перевоплощение : %s%d%s\r\n",
					  CCCYN(ch, C_NRM), abs(obj->get_minimum_remorts()), CCNRM(ch, C_NRM));
	}

	if (fullness < 60)
		return;

	SendMsgToChar("Имеет экстрафлаги: ", ch);
	SendMsgToChar(CCCYN(ch, C_NRM), ch);
	GET_OBJ_EXTRA(obj).sprintbits(extra_bits, buf, ",", IS_IMMORTAL(ch) ? 4 : 0);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);
	SendMsgToChar(CCNRM(ch, C_NRM), ch);
//enhansed_scroll = true; //для теста
	if (enhansed_scroll) {
		if (check_unlimited_timer(obj))
			sprintf(buf2, "Таймер: %d/нерушимо.", obj_proto[GET_OBJ_RNUM(obj)]->get_timer());
		else
			sprintf(buf2, "Таймер: %d/%d.", obj_proto[GET_OBJ_RNUM(obj)]->get_timer(), obj->get_timer());
		char miw[128];
		if (GET_OBJ_MIW(obj) < 0) {
			sprintf(miw, "%s", "бесконечно");
		} else {
			sprintf(miw, "%d", GET_OBJ_MIW(obj));
		}
		snprintf(buf, kMaxStringLength, "&GСейчас в мире : %d. На постое : %d. Макс. в мире : %s. %s&n\r\n",
				 obj_proto.CountInWorld(GET_OBJ_RNUM(obj)), obj_proto.stored(GET_OBJ_RNUM(obj)), miw, buf2);
		SendMsgToChar(buf, ch);
	}
	if (fullness < 75)
		return;

	switch (GET_OBJ_TYPE(obj)) {
		case EObjType::kScroll:
		case EObjType::kPotion: {
			std::ostringstream out;
			out << "Содержит заклинание: ";
			for (auto val = 1; val < 4; ++val) {
				auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, val));
				if (MUD::Spell(spell_id).IsValid()) {
					out << MUD::Spell(spell_id).GetName();
					if (val < 3) {
						out << ", ";
					} else {
						out << ".";
					}
				}
			}
			out << std::endl;
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
				case EBook::kSkillUpgrade: print_book_uprgd_skill(ch, obj);
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
							SendMsgToChar(CCIRED(ch, C_NRM), ch);
							SendMsgToChar("Некорректная запись рецепта для вашего класса - сообщите Богам.\r\n", ch);
							SendMsgToChar(CCNRM(ch, C_NRM), ch);
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

				default: SendMsgToChar(CCIRED(ch, C_NRM), ch);
					SendMsgToChar("НЕВЕРНО УКАЗАН ТИП КНИГИ - сообщите Богам\r\n", ch);
					SendMsgToChar(CCNRM(ch, C_NRM), ch);
					break;
			}
			break;

		case EObjType::kIngredient: sprintbit(GET_OBJ_SKILL(obj), ingradient_bits, buf2);
			snprintf(buf, kMaxStringLength, "%s\r\n", buf2);
			SendMsgToChar(buf, ch);

			if (IS_SET(GET_OBJ_SKILL(obj), kItemCheckUses)) {
				sprintf(buf, "можно применить %d раз\r\n", GET_OBJ_VAL(obj, 2));
				SendMsgToChar(buf, ch);
			}

			if (IS_SET(GET_OBJ_SKILL(obj), kItemCheckLag)) {
				sprintf(buf, "можно применить 1 раз в %d сек", (i = GET_OBJ_VAL(obj, 0) & 0xFF));
				if (GET_OBJ_VAL(obj, 3) == 0 || GET_OBJ_VAL(obj, 3) + i < time(nullptr))
					strcat(buf, "(можно применять).\r\n");
				else {
					li = GET_OBJ_VAL(obj, 3) + i - time(nullptr);
					sprintf(buf + strlen(buf), "(осталось %ld сек).\r\n", li);
				}
				SendMsgToChar(buf, ch);
			}

			if (IS_SET(GET_OBJ_SKILL(obj), kItemCheckLevel)) {
				sprintf(buf, "можно применить с %d уровня.\r\n", (GET_OBJ_VAL(obj, 0) >> 8) & 0x1F);
				SendMsgToChar(buf, ch);
			}

			if ((i = real_object(GET_OBJ_VAL(obj, 1))) >= 0) {
				sprintf(buf, "прототип %s%s%s.\r\n",
						CCICYN(ch, C_NRM), obj_proto[i]->get_PName(0).c_str(), CCNRM(ch, C_NRM));
				SendMsgToChar(buf, ch);
			}
			break;

		case EObjType::kMagicIngredient:
			for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT) && j <= top_imtypes;) {
				j++;
			}
			sprintf(buf, "Это ингредиент вида '%s%s%s'\r\n", CCCYN(ch, C_NRM), imtypes[j].name, CCNRM(ch, C_NRM));
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

	if (fullness < 90) {
		return;
	}

	SendMsgToChar("Накладывает на вас аффекты: ", ch);
	SendMsgToChar(CCCYN(ch, C_NRM), ch);
	obj->get_affect_flags().sprintbits(weapon_affects, buf, ",", IS_IMMORTAL(ch) ? 4 : 0);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);
	SendMsgToChar(CCNRM(ch, C_NRM), ch);

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

	if (GET_OBJ_TYPE(obj) == EObjType::kEnchant
		&& GET_OBJ_VAL(obj, 0) != 0) {
		if (!found) {
			SendMsgToChar("Дополнительные свойства :\r\n", ch);
			found = true;
		}
		SendMsgToChar(ch, "%s   %s вес предмета на %d%s\r\n", CCCYN(ch, C_NRM),
					  GET_OBJ_VAL(obj, 0) > 0 ? "увеличивает" : "уменьшает",
					  abs(GET_OBJ_VAL(obj, 0)), CCNRM(ch, C_NRM));
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
					CCCYN(ch, C_NRM), MUD::Skill(skill_id).GetName(), CCNRM(ch, C_NRM),
					CCCYN(ch, C_NRM),
					percent < 0 ? " ухудшает на " : " улучшает на ", abs(percent), CCNRM(ch, C_NRM));
			SendMsgToChar(buf, ch);
		}
	}

	auto it = ObjData::set_table.begin();
	if (obj->has_flag(EObjFlag::KSetItem)) {
		for (; it != ObjData::set_table.end(); it++) {
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				sprintf(buf,
						"Часть набора предметов: %s%s%s\r\n",
						CCNRM(ch, C_NRM),
						it->second.get_name().c_str(),
						CCNRM(ch, C_NRM));
				SendMsgToChar(buf, ch);
				for (auto & vnum : it->second) {
					const int r_num = real_object(vnum.first);
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

#define IDENT_SELF_LEVEL 6

void mort_show_char_values(CharData *victim, CharData *ch, int fullness) {
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
		sprintf(buf,
				"Возраст %s  : %d лет, %d месяцев, %d дней и %d часов.\r\n",
				GET_PAD(victim, 1), age(victim)->year, age(victim)->month,
				age(victim)->day, age(victim)->hours);
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
	val1 = GET_HIT(victim);
	val2 = GET_REAL_MAX_HIT(victim);
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
				  compute_armor_class(victim), GET_ARMOUR(victim), GET_ABSORBE(victim));

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
				SendMsgToChar(CCIRED(ch, C_NRM), ch);
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
	SendMsgToChar(CCNRM(ch, C_NRM), ch);

	SendMsgToChar("Аффекты :\r\n", ch);
	SendMsgToChar(CCICYN(ch, C_NRM), ch);
	victim->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, "\r\n", IS_IMMORTAL(ch) ? 4 : 0);
	snprintf(buf, kMaxStringLength, "%s\r\n", buf2);
	SendMsgToChar(buf, ch);
	SendMsgToChar(CCNRM(ch, C_NRM), ch);
}

void SkillIdentify(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	bool full = false;
	if (obj) {
		mort_show_obj_values(obj, ch, CalcCurrentSkill(ch, ESkill::kIdentify, nullptr), full);
		TrainSkill(ch, ESkill::kIdentify, true, nullptr);
	} else if (victim) {
		if (GetRealLevel(victim) < 3) {
			SendMsgToChar("Вы можете опознать только персонажа, достигнувшего третьего уровня.\r\n", ch);
			return;
		}
		mort_show_char_values(victim, ch, CalcCurrentSkill(ch, ESkill::kIdentify, victim));
		TrainSkill(ch, ESkill::kIdentify, true, victim);
	}
}


void SpellFullIdentify(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	bool full = true;
	if (obj)
		mort_show_obj_values(obj, ch, 100, full);
	else if (victim) {
		SendMsgToChar("С помощью магии нельзя опознать другое существо.\r\n", ch);
			return;
	}
}

void SpellIdentify(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	bool full = false;
	if (obj)
		mort_show_obj_values(obj, ch, 100, full);
	else if (victim) {
		if (victim != ch) {
			SendMsgToChar("С помощью магии нельзя опознать другое существо.\r\n", ch);
			return;
		}
		if (GetRealLevel(victim) < 3) {
			SendMsgToChar("Вы можете опознать себя только достигнув третьего уровня.\r\n", ch);
			return;
		}
		mort_show_char_values(victim, ch, 100);
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
				create_rainsnow(&sky_type, kWeatherLightrain, 0, 50, 50);
			} else if (time_info.month >= EMonth::kDecember || time_info.month <= EMonth::kFebruary) {
				sky_info = "Повалил снег.";
				create_rainsnow(&sky_type, kWeatherLightsnow, 0, 50, 50);
			} else if (time_info.month == EMonth::kMarch || time_info.month == EMonth::kNovember) {
				if (weather_info.temperature > 2) {
					sky_info = "Начался проливной дождь.";
					create_rainsnow(&sky_type, kWeatherLightrain, 0, 50, 50);
				} else {
					sky_info = "Повалил снег.";
					create_rainsnow(&sky_type, kWeatherLightsnow, 0, 50, 50);
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
	if (PRF_FLAGGED(ch, EPrf::kAwake))
		modi = modi - 50;
	if (AFF_FLAGGED(victim, EAffect::kBless))
		modi -= 25;

	if (!MOB_FLAGGED(victim, EMobFlag::kNoFear) && !CalcGeneralSaving(ch, victim, ESaving::kWill, modi))
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
	if (PRF_FLAGGED(ch, EPrf::kAwake))
		modi = modi - 50;

	if (ch == victim || !CalcGeneralSaving(ch, victim, ESaving::kWill, CALC_SUCCESS(modi, 33))) {
		for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_MEM(victim, spell_id) = 0;
		}
		victim->caster_level = 0;
		SendMsgToChar("Внезапно вы осознали, что у вас напрочь отшибло память.\r\n", victim);
	} else
		SendMsgToChar(NOEFFECT, ch);
}

// накачка хитов
void do_sacrifice(CharData *ch, int dam) {
//MZ.overflow_fix
	GET_HIT(ch) = MAX(GET_HIT(ch), MIN(GET_HIT(ch) + MAX(1, dam), GET_REAL_MAX_HIT(ch)
		+ GET_REAL_MAX_HIT(ch) * GetRealLevel(ch) / 10));
//-MZ.overflow_fix
	update_pos(ch);
}

void SpellSacrifice(int/* level*/, CharData *ch, CharData *victim, ObjData* /*obj*/) {
	int dam, d0 = GET_HIT(victim);
	struct FollowerType *f;

	// Высосать жизнь - некроманы - уровень 18 круг 6й (5)
	// *** мин 54 макс 66 (330)

	if (IS_IMMORTAL(victim) || victim == ch || IS_CHARMICE(victim)) {
		SendMsgToChar(NOEFFECT, ch);
		return;
	}

	dam = CastDamage(GetRealLevel(ch), ch, victim, ESpell::kSacrifice);
	// victim может быть спуржен

	if (dam < 0)
		dam = d0;
	if (dam > d0)
		dam = d0;
	if (dam <= 0)
		return;

	do_sacrifice(ch, dam);
	if (!ch->IsNpc()) {
		for (f = ch->followers; f; f = f->next) {
			if (f->follower->IsNpc()
				&& AFF_FLAGGED(f->follower, EAffect::kCharmed)
				&& MOB_FLAGGED(f->follower, EMobFlag::kCorpse)
				&& ch->in_room == IN_ROOM(f->follower)) {
				do_sacrifice(f->follower, dam);
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
			if (!MOB_FLAGGED(tch, EMobFlag::kCorpse)
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

		CastAffect(GetRealLevel(ch), ch, tch, ESpell::kHolystrike);
		CastDamage(GetRealLevel(ch), ch, tch, ESpell::kHolystrike);
	}

	act(msg2, false, ch, nullptr, nullptr, kToChar);
	act(msg2, false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

	ObjData *o = nullptr;
	do {
		for (o = world[ch->in_room]->contents; o; o = o->get_next_content()) {
			if (!IS_CORPSE(o)) {
				continue;
			}

			ExtractObjFromWorld(o);

			break;
		}
	} while (o);
}

void SpellSummonAngel(int/* level*/, CharData *ch, CharData* /*victim*/, ObjData* /*obj*/) {
	MobVnum mob_num = 108;
	//int modifier = 0;
	CharData *mob = nullptr;
	struct FollowerType *k, *k_next;

	auto eff_cha = get_effective_cha(ch);

	for (k = ch->followers; k; k = k_next) {
		k_next = k->next;
		if (MOB_FLAGGED(k->follower,
						EMobFlag::kTutelar))    //SendMsgToChar("Боги не обратили на вас никакого внимания!\r\n", ch);
		{
			//return;
			//пуржим старого ангела
			stop_follower(k->follower, kSfCharmlost);
		}
	}

	float base_success = 26.0;
	float additional_success_for_charisma = 1.5; // 50 at 16 charisma, 101 at 50 charisma

	if (number(1, 100) > floorf(base_success + additional_success_for_charisma * eff_cha)) {
		SendMsgToRoom("Яркая вспышка света! Несколько белых перьев кружась легли на землю...", ch->in_room, true);
		return;
	};
	if (!(mob = read_mobile(-mob_num, VIRTUAL))) {
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

	ClearCharTalents(mob);
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
		mob->player_data.PNames[0] = "Небесный защитник";
		mob->player_data.PNames[1] = "Небесного защитника";
		mob->player_data.PNames[2] = "Небесному защитнику";
		mob->player_data.PNames[3] = "Небесного защитника";
		mob->player_data.PNames[4] = "Небесным защитником";
		mob->player_data.PNames[5] = "Небесном защитнике";
		mob->set_npc_name("Небесный защитник");
		mob->player_data.long_descr = str_dup("Небесный защитник летает тут.\r\n");
		mob->player_data.description = str_dup("Сияющая призрачная фигура о двух крылах.\r\n");
	} else {
		mob->set_sex(EGender::kFemale);
		mob->SetCharAliases("Небесная защитница");
		mob->player_data.PNames[0] = "Небесная защитница";
		mob->player_data.PNames[1] = "Небесной защитницы";
		mob->player_data.PNames[2] = "Небесной защитнице";
		mob->player_data.PNames[3] = "Небесную защитницу";
		mob->player_data.PNames[4] = "Небесной защитницей";
		mob->player_data.PNames[5] = "Небесной защитнице";
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

	GET_MAX_HIT(mob) = floorf(base_hp + additional_hp_for_charisma * eff_cha);
	GET_HIT(mob) = GET_MAX_HIT(mob);
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;

	GET_POS(mob) = EPosition::kStand;
	GET_DEFAULT_POS(mob) = EPosition::kStand;

	mob->set_skill(ESkill::kRescue, floorf(base_rescue + additional_rescue_for_charisma * eff_cha));
	mob->set_skill(ESkill::kAwake, floorf(base_awake + additional_awake_for_charisma * eff_cha));
	mob->set_skill(ESkill::kMultiparry, floorf(base_multiparry + additional_multiparry_for_charisma * eff_cha));
	int base_spell = 2;
	SET_SPELL_MEM(mob, ESpell::kCureBlind, base_spell);
	SET_SPELL_MEM(mob, ESpell::kRemoveHold, base_spell);
	SET_SPELL_MEM(mob, ESpell::kRemovePoison, base_spell);
	SET_SPELL_MEM(mob, ESpell::kHeal, floorf(base_heal + additional_heal_for_charisma * eff_cha));

	if (mob->GetSkill(ESkill::kAwake)) {
		PRF_FLAGS(mob).set(EPrf::kAwake);
	}

	GET_LIKES(mob) = 100;
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;

	MOB_FLAGS(mob).set(EMobFlag::kCorpse);
	MOB_FLAGS(mob).set(EMobFlag::kTutelar);
	MOB_FLAGS(mob).set(EMobFlag::kLightingBreath);

	mob->set_level(GetRealLevel(ch));
	PlaceCharToRoom(mob, ch->in_room);
	ch->add_follower(mob);
	
	if (IS_FEMALE(mob)) {
		act("Небесная защитница появилась в яркой вспышке света!",
			true, mob, nullptr, nullptr, kToRoom | kToArenaListen);
	} else {
		act("Небесный защитник появился в яркой вспышке света!",
			true, mob, nullptr, nullptr, kToRoom | kToArenaListen);
	}
}

void SpellVampirism(int/* level*/, CharData* /*ch*/, CharData* /*victim*/, ObjData* /*obj*/) {
}

void SpellMentalShadow(int/* level*/, CharData *ch, CharData* /*victim*/, ObjData* /*obj*/) {
	// подготовка контейнера для создания заклинания ментальная тень
	// все предложения пишем мад почтой

	MobVnum mob_num = kMobMentalShadow;

	CharData *mob = nullptr;
	struct FollowerType *k, *k_next;
	for (k = ch->followers; k; k = k_next) {
		k_next = k->next;
		if (MOB_FLAGGED(k->follower, EMobFlag::kMentalShadow)) {
			stop_follower(k->follower, false);
		}
	}
	auto eff_int = get_effective_int(ch);
	int hp = 100;
	int hp_per_int = 15;
	float base_ac = 100;
	float additional_ac = -1.5;
	if (eff_int < 26 && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Головные боли мешают работать!\r\n", ch);
		return;
	};

	if (!(mob = read_mobile(-mob_num, VIRTUAL))) {
		SendMsgToChar("Вы точно не помните, как создать данного монстра.\r\n", ch);
		return;
	}
	Affect<EApply> af;
	af.type = ESpell::kCharm;
	af.duration = CalcDuration(mob, 5 + (int) VPOSI<float>((get_effective_int(ch) - 16.0) / 2, 0, 50), 0, 0, 0, 0);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.bitvector = to_underlying(EAffect::kHelper);
	af.battleflag = 0;
	affect_to_char(mob, af);
	
	GET_MAX_HIT(mob) = floorf(hp + hp_per_int * (eff_int - 20) + GET_HIT(ch)/4);
	GET_HIT(mob) = GET_MAX_HIT(mob);
	GET_AC(mob) = floorf(base_ac + additional_ac * eff_int);
	// Добавление заклов и аффектов в зависимости от интелекта кудеса
	if (eff_int >= 28 && eff_int < 32) {
     	SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 1);
	} else if (eff_int >= 32 && eff_int < 38) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 1);
		af.bitvector = to_underlying(EAffect::kShadowCloak);
		affect_to_char(mob, af);

	} else if(eff_int >= 38 && eff_int < 44) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 2);
		af.bitvector = to_underlying(EAffect::kShadowCloak);
		affect_to_char(mob, af);
		
	} else if(eff_int >= 44) {
		SET_SPELL_MEM(mob, ESpell::kRemoveSilence, 3);
		af.bitvector = to_underlying(EAffect::kShadowCloak);
		affect_to_char(mob, af);
		af.bitvector = to_underlying(EAffect::kBrokenChains);
		affect_to_char(mob, af);
	}
	if (mob->GetSkill(ESkill::kAwake)) {
		PRF_FLAGS(mob).set(EPrf::kAwake);
	}
	mob->set_level(GetRealLevel(ch));
	MOB_FLAGS(mob).set(EMobFlag::kCorpse);
	MOB_FLAGS(mob).set(EMobFlag::kMentalShadow);
	PlaceCharToRoom(mob, IN_ROOM(ch));
	ch->add_follower(mob);
	mob->set_protecting(ch);
	
	act("Мимолётное наваждение воплотилось в призрачную тень.",
		true, mob, nullptr, nullptr, kToRoom | kToArenaListen);
}

std::map<int /* vnum */, int /* count */> rune_list;

void add_rune_stats(CharData *ch, int vnum, int spelltype) {
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

	if (IS_SET(GET_OBJ_SKILL(obj), kItemCheckUses)) {
		obj->dec_val(2);
		if (GET_OBJ_VAL(obj, 2) <= 0
			&& IS_SET(GET_OBJ_SKILL(obj), kItemDecayEmpty)) {
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

	if (((obj_num = real_object(items->rnumber)) < 0 &&
		spell_type != ESpellType::kItemCast && spell_type != ESpellType::kRunes) ||
		((item0 = real_object(items->items[0])) +
			(item1 = real_object(items->items[1])) + (item2 = real_object(items->items[2])) < -2)) {
		if (showrecipe)
			SendMsgToChar("Боги хранят в секрете этот рецепт.\n\r", ch);
		return (false);
	}

	if (!showrecipe)
		return (true);
	else {
		strcpy(buf, "Вам потребуется :\r\n");
		if (item0 >= 0) {
			strcat(buf, CCIRED(ch, C_NRM));
			strcat(buf, obj_proto[item0]->get_PName(0).c_str());
			strcat(buf, "\r\n");
		}
		if (item1 >= 0) {
			strcat(buf, CCIYEL(ch, C_NRM));
			strcat(buf, obj_proto[item1]->get_PName(0).c_str());
			strcat(buf, "\r\n");
		}
		if (item2 >= 0) {
			strcat(buf, CCIGRN(ch, C_NRM));
			strcat(buf, obj_proto[item2]->get_PName(0).c_str());
			strcat(buf, "\r\n");
		}
		if (obj_num >= 0 && (spell_type == ESpellType::kItemCast || spell_type == ESpellType::kRunes)) {
			strcat(buf, CCIBLU(ch, C_NRM));
			strcat(buf, obj_proto[obj_num]->get_PName(0).c_str());
			strcat(buf, "\r\n");
		}

		strcat(buf, CCNRM(ch, C_NRM));
		if (spell_type == ESpellType::kItemCast || spell_type == ESpellType::kRunes) {
			strcat(buf, "для создания магии '");
			strcat(buf, MUD::Spell(spell_id).GetCName());
			strcat(buf, "'.");
		} else {
			strcat(buf, "для создания ");
			strcat(buf, obj_proto[obj_num]->get_PName(1).c_str());
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
		&& GET_OBJ_TYPE(obj) != EObjType::kIngredient) {
		return false;
	}

	if (GET_OBJ_TYPE(obj) == EObjType::kIngredient) {
		if ((!IS_SET(GET_OBJ_SKILL(obj), kItemRunes) && spelltype == ESpellType::kRunes)
			|| (IS_SET(GET_OBJ_SKILL(obj), kItemRunes) && spelltype != ESpellType::kRunes)) {
			return false;
		}
	}

	if (IS_SET(GET_OBJ_SKILL(obj), kItemCheckUses)
		&& GET_OBJ_VAL(obj, 2) <= 0) {
		return false;
	}

	if (IS_SET(GET_OBJ_SKILL(obj), kItemCheckLag)) {
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

	if (IS_SET(GET_OBJ_SKILL(obj), kItemCheckLevel)) {
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

int CheckRecipeItems(CharData *ch, ESpell spell_id, ESpellType spell_type, int extract, const CharData *targ) {
	ObjData *obj0 = nullptr, *obj1 = nullptr, *obj2 = nullptr, *obj3 = nullptr, *objo = nullptr;
	int item0 = -1, item1 = -1, item2 = -1, item3 = -1;
	int create = 0, obj_num = -1, percent = 0, num = 0;
	auto skill_id{ESkill::kUndefined};
	struct SpellCreateItem *items;

	if (spell_id <= ESpell::kUndefined) {
		return false;
	}
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

	if (((spell_type == ESpellType::kRunes || spell_type == ESpellType::kItemCast) &&
		(item3 = items->rnumber) +
			(item0 = items->items[0]) +
			(item1 = items->items[1]) +
			(item2 = items->items[2]) < -3)
		|| ((spell_type == ESpellType::kScrollCast
			|| spell_type == ESpellType::kWandCast
			|| spell_type == ESpellType::kPotionCast)
			&& ((obj_num = items->rnumber) < 0
				|| (item0 = items->items[0]) + (item1 = items->items[1])
					+ (item2 = items->items[2]) < -2))) {
		return (false);
	}

	const int item0_rnum = item0 >= 0 ? real_object(item0) : -1;
	const int item1_rnum = item1 >= 0 ? real_object(item1) : -1;
	const int item2_rnum = item2 >= 0 ? real_object(item2) : -1;
	const int item3_rnum = item3 >= 0 ? real_object(item3) : -1;

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
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj0->get_PName(3).c_str());
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj0, 1), spell_type);
		}

		if (item1 == -2) {
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj1->get_PName(3).c_str());
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj1, 1), spell_type);
		}

		if (item2 == -2) {
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj2->get_PName(3).c_str());
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj2, 1), spell_type);
		}

		if (item3 == -2) {
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj3->get_PName(3).c_str());
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj3, 1), spell_type);
		}

		strcat(buf, CCNRM(ch, C_NRM));

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
						PRF_FLAGGED(ch, EPrf::kCompact) ? "" : "\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n сложил$g руны, которые вспыхнули ярким пламенем.",
					true, ch, nullptr, nullptr, kToRoom);
				sprintf(buf, "$n сложил$g руны в заклинание '%s'%s%s.",
						MUD::Spell(spell_id).GetCName(),
						(targ && targ != ch ? " на " : ""),
						(targ && targ != ch ? GET_PAD(targ, 1) : ""));
				act(buf, true, ch, nullptr, nullptr, kToArenaListen);
				auto magic_skill = GetMagicSkillId(spell_id);
				if (MUD::Skills().IsValid(magic_skill)) {
					TrainSkill(ch, magic_skill, true, nullptr);
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
	if (!IS_GRGOD(ch)) {
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
