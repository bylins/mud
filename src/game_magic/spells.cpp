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

#include <boost/format.hpp>

#include "structs/global_objects.h"
#include "cmd/follow.h"
#include "cmd/hire.h"
#include "depot.h"
#include "fightsystem/mobact.h"
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

ESkill GetMagicSkillId(int spellnum) {
	switch (spell_info[spellnum].spell_class) {
		case kTypeAir: return ESkill::kAirMagic;
			break;
		case kTypeFire: return ESkill::kFireMagic;
			break;
		case kTypeWater: return ESkill::kWaterMagic;
			break;
		case kTypeEarth: return ESkill::kEarthMagic;
			break;
		case kTypeLight: return ESkill::kLightMagic;
			break;
		case kTypeDark: return ESkill::kDarkMagic;
			break;
		case kTypeMind: return ESkill::kMindMagic;
			break;
		case kTypeLife: return ESkill::kLifeMagic;
			break;
		case kTypeNeutral: [[fallthrough]];
		default: return ESkill::kIncorrect;
	}
}

//Определим мин уровень для изучения спелла из книги
//req_lvl - требуемый уровень из книги
int CalcMinSpellLevel(CharData *ch, int spellnum, int req_lvl) {
	int min_lvl = MAX(req_lvl, BASE_CAST_LEV(spell_info[spellnum], ch))
		- (MAX(GET_REAL_REMORT(ch) - MIN_CAST_REM(spell_info[spellnum], ch), 0) / 3);

	return MAX(1, min_lvl);
}

bool IsAbleToGetSpell(CharData *ch, int spellnum, int req_lvl) {
	if (CalcMinSpellLevel(ch, spellnum, req_lvl) > GetRealLevel(ch)
		|| MIN_CAST_REM(spell_info[spellnum], ch) > GET_REAL_REMORT(ch))
		return false;

	return true;
};

// Функция определяет возможность изучения спелла из книги или в гильдии
bool IsAbleToGetSpell(CharData *ch, int spellnum) {
	if (MIN_CAST_LEV(spell_info[spellnum], ch) > GetRealLevel(ch)
		|| MIN_CAST_REM(spell_info[spellnum], ch) > GET_REAL_REMORT(ch))
		return false;

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
			water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
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

		if ((ch->IsNpc() && CalcGeneralSaving(ch, victim, ESaving::kWill, GET_REAL_INT(ch))) || IS_GOD(victim)) {
			return;
		}
	}

	if ((to_room = real_room(GET_LOADROOM(victim))) == kNowhere)
		to_room = real_room(calc_loadroom(victim));

	if (to_room == kNowhere) {
		SendMsgToChar(SUMMON_FAIL, ch);
		return;
	}

	(void) get_zone_rooms(world[to_room]->zone_rn, &rnum_start, &rnum_stop);
	fnd_room = GetTeleportTargetRoom(victim, rnum_start, rnum_stop);
	if (fnd_room == kNowhere) {
		to_room = Clan::CloseRent(to_room);
		(void) get_zone_rooms(world[to_room]->zone_rn, &rnum_start, &rnum_stop);
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

	get_zone_rooms(world[in_room]->zone_rn, &rnum_start, &rnum_stop);
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
			ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) || (ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) && !IS_IMMORTAL(ch)))) {
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
		if ((!victim->IsNpc() && GetRealLevel(victim) <= 10 && GET_REAL_REMORT(ch) < 9) || IS_IMMORTAL(victim)
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
	struct Follower *k, *k_next;

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
		if (victim->get_master() != ch  //поправим это, тут и так понято что чармис (Кудояр)
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
		// отдельно проверку на клан комнаты, своих чармисов призвать можем (Кудояр)
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
		if (IN_ROOM(k->ch) == vic_room) {
			if (AFF_FLAGGED(k->ch, EAffect::kCharmed)) {
				if (!k->ch->GetEnemy()) {
					act("$n растворил$u на ваших глазах.",
						true, k->ch, nullptr, nullptr, kToRoom | kToArenaListen);
					ExtractCharFromRoom(k->ch);
					PlaceCharToRoom(k->ch, ch_room);
					act("$n прибыл$g за хозяином.",
						true, k->ch, nullptr, nullptr, kToRoom | kToArenaListen);
					act("$n призвал$g вас!", false, ch, nullptr, k->ch, kToVict);
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
			if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0))) {
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

int CheckCharmices(CharData *ch, CharData *victim, int spellnum) {
	struct Follower *k;
	int cha_summ = 0, reformed_hp_summ = 0;
	bool undead_in_group = false, living_in_group = false;

	for (k = ch->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->ch, EAffect::kCharmed)
			&& k->ch->get_master() == ch) {
			cha_summ++;
			//hp_summ += GET_REAL_MAX_HIT(k->ch);
			reformed_hp_summ += get_reformed_charmice_hp(ch, k->ch, spellnum);
// Проверка на тип последователей -- некрасиво, зато эффективно
			if (MOB_FLAGGED(k->ch, EMobFlag::kCorpse)) {
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

	if (spellnum == kSpellCharm && undead_in_group) {
		SendMsgToChar("Ваша жертва боится ваших последователей.\r\n", ch);
		return (false);
	}

	if (spellnum != kSpellCharm && living_in_group) {
		SendMsgToChar("Ваш последователь мешает вам произнести это заклинание.\r\n", ch);
		return (false);
	}

	if (spellnum == kSpellClone && cha_summ >= MAX(1, (GetRealLevel(ch) + 4) / 5 - 2)) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}

	if (spellnum != kSpellClone && cha_summ >= (GetRealLevel(ch) + 9) / 10) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}

	if (spellnum != kSpellClone &&
		reformed_hp_summ + get_reformed_charmice_hp(ch, victim, spellnum) >= get_player_charms(ch, spellnum)) {
		SendMsgToChar("Вам не под силу управлять такой боевой мощью.\r\n", ch);
		return (false);
	}
	return (true);
}

void SpellCharm(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/) {
	int k_skills = 0;
	ESkill skill_id = ESkill::kIncorrect;
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
		&& CalcGeneralSaving(ch, victim, ESaving::kWill, (GET_REAL_CHA(ch) - 10) * 4 + GET_REAL_REMORT(ch) * 3)) //предлагаю завязать на каст
		SendMsgToChar("Ваша магия потерпела неудачу.\r\n", ch);
	else {
		if (!CheckCharmices(ch, victim, kSpellCharm)) {
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

		affect_from_char(victim, kSpellCharm);
		ch->add_follower(victim);
		Affect<EApply> af;
		af.type = kSpellCharm;

		if (GET_REAL_INT(victim) > GET_REAL_INT(ch)) {
			af.duration = CalcDuration(victim, GET_REAL_CHA(ch), 0, 0, 0, 0);
		} else {
			af.duration = CalcDuration(victim, GET_REAL_CHA(ch) + number(1, 10) + GET_REAL_REMORT(ch) * 2, 0, 0, 0, 0);
		}

		af.modifier = 0;
		af.location = EApply::kNone;
		af.bitvector = to_underlying(EAffect::kCharmed);
		af.battleflag = 0;
		affect_to_char(victim, af);
		// резервируем место под фит (Кудояр)
		if (IsAbleToUseFeat(ch, EFeat::kAnimalMaster) &&
		GET_RACE(victim) == 104) {
			act("$N0 обрел$G часть вашей магической силы, и стал$G намного опаснее...",
				false, ch, nullptr, victim, kToChar);
			act("$N0 обрел$G часть магической силы $n1.",
				false, ch, nullptr, victim, kToRoom | kToArenaListen);
			// начинаем модификации victim
			// создаем переменные модификаторов
			int r_cha = GET_REAL_CHA(ch);
			int perc = ch->get_skill(GetMagicSkillId(kSpellCharm));
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
					case ESex::kNeutral:
					gender = 0;
					break;
					case ESex::kMale:
					gender = 1;
					break;
					case ESex::kFemale:
					gender = 2;
					break;
					default:
					gender = 3;
					break;
			}
 		// 1 при 10-19, 2 при 20-29 , 3 при 30-39....
			int adj = r_cha/10;
			sprintf(descr, "%s %s %s", state[gender][adj - 1][0], GET_PAD(victim, 0), GET_NAME(victim));
			victim->set_pc_name(descr);
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
			if (GET_REAL_REMORT(ch) > 12) {
				GET_AR(victim) = (GET_AR(victim) + GET_REAL_REMORT(ch) - 12);
				GET_MR(victim) = (GET_MR(victim) + GET_REAL_REMORT(ch) - 12);
				GET_PR(victim) = (GET_PR(victim) + GET_REAL_REMORT(ch) - 12);
			}
			// спелы не работают пока 
			// SET_SPELL(victim, SPELL_CURE_BLIND, 1); // -?
			// SET_SPELL(victim, SPELL_REMOVE_DEAFNESS, 1); // -?
			// SET_SPELL(victim, SPELL_REMOVE_HOLD, 1); // -?
			// SET_SPELL(victim, SPELL_REMOVE_POISON, 1); // -?
			// SET_SPELL(victim, SPELL_HEAL, 1);

			//NPC_FLAGS(victim).set(NPC_WIELDING); // тут пока закомитим
			GET_LIKES(victim) = 10 + r_cha; // устанавливаем возможность авто применения умений
			
			// создаем кубики и доп атаки (пока без + а просто сет)
			victim->mob_specials.damnodice = floorf((r_cha*1.3 + perc*0.2) / 5.0);
			victim->mob_specials.damsizedice = floorf((r_cha*1.2 + perc*0.1) / 11.0);
			victim->mob_specials.ExtraAttack = floorf((r_cha*1.2 + perc) / 120.0);
			

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
				SET_FEAT(victim, EFeat::kPunchMaster);
					if (floorf(r_cha*0.9 + perc/5.0) > number(1, 150)) {
					SET_FEAT(victim, EFeat::kPunchFocus);
					victim->set_skill(ESkill::kStrangle, k_skills);
					SET_FEAT(victim, EFeat::kBerserker);
					act("&B$N0 теперь сможет просто удавить всех своих врагов.&n\n",
						false, ch, nullptr, victim, kToChar);
				}
				victim->set_str(floorf(GET_REAL_STR(victim)*1.3));
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
				SET_FEAT(victim, EFeat::kTwohandsMaster);
				SET_FEAT(victim, EFeat::kTwohandsFocus);
				if (floorf(r_cha + perc/5.0) > number(1, 150)) {
					SET_FEAT(victim, EFeat::kRelatedToMagic);
					act("&G$N0 стал$G намного более опасным хищником.&n\n",
						false, ch, nullptr, victim, kToChar);
					victim->set_skill(ESkill::kFirstAid, k_skills*0.4);
					victim->set_skill(ESkill::kParry, k_skills*0.7);
				}
				victim->set_str(floorf(GET_REAL_STR(victim)*1.2));
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
				SET_FEAT(victim, EFeat::kPicksMaster);
				SET_FEAT(victim, EFeat::kThieveStrike);
				if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
					SET_FEAT(victim, EFeat::kShadowStrike);
					act("&c$N0 затаил$U в вашей тени...&n\n", false, ch, nullptr, victim, kToChar);
					
				}
				victim->set_dex(floorf(GET_REAL_DEX(victim)*1.3));		
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
				SET_FEAT(victim, EFeat::kAxesMaster);
				SET_FEAT(victim, EFeat::kThieveStrike);
				SET_FEAT(victim, EFeat::kDefender);
				SET_FEAT(victim, EFeat::kLiveShield);
				victim->set_con(floorf(GET_REAL_CON(victim)*1.3));
				victim->set_str(floorf(GET_REAL_STR(victim)*1.2));
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
				SET_FEAT(victim, EFeat::kThieveStrike);
				SET_FEAT(victim, EFeat::kBowsMaster);
				if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
					af.bitvector = to_underlying(EAffect::kCloudOfArrows);
					act("&YВокруг когтей $N1 засияли яркие магические всполохи.&n\n",
						false, ch, nullptr, victim, kToChar);
					affect_to_char(victim, af);
				}
				victim->set_dex(floorf(GET_REAL_DEX(victim)*1.2));
				victim->set_str(floorf(GET_REAL_STR(victim)*1.15));
				victim->mob_specials.ExtraAttack = floorf((r_cha*1.2 + perc) / 180.0); // срежем доп атаки
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
				SET_FEAT(victim, EFeat::kClubsMaster);
				SET_FEAT(victim, EFeat::kThrowWeapon);
				SET_FEAT(victim, EFeat::kDoubleThrower);
				SET_FEAT(victim, EFeat::kTripleThrower);
				SET_FEAT(victim, EFeat::kPowerThrow);
				SET_FEAT(victim, EFeat::kDeadlyThrow);
				if (floorf(r_cha*0.8 + perc/5.0) > number(1, 140)) {
					SET_FEAT(victim, EFeat::kShadowThrower);
					SET_FEAT(victim, EFeat::kShadowClub);
					victim->set_skill(ESkill::kDarkMagic, k_skills*0.7);
					act("&cКогти $N1 преобрели &Kчерный цвет&c, будто смерть коснулась их.&n\n",
						false, ch, nullptr, victim, kToChar);
					victim->mob_specials.ExtraAttack = floorf((r_cha*1.2 + perc) / 100.0);
				}
				victim->set_str(floorf(GET_REAL_STR(victim)*1.25));
				
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
				SET_FEAT(victim, EFeat::kLongsMaster);
			
				if (floorf(r_cha*0.8 + perc/5.0) > number(1, 150)) {
					victim->set_skill(ESkill::kIronwind, k_skills*0.8);
					SET_FEAT(victim, EFeat::kBerserker);
					act("&mДвижения $N1 сильно ускорились, и в глазах появились &Rогоньки&m безумия.&n\n",
						false, ch, nullptr, victim, kToChar);
				}
				victim->set_dex(floorf(GET_REAL_DEX(victim)*1.1));
				victim->set_str(floorf(GET_REAL_STR(victim)*1.35));
				
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
				SET_FEAT(victim, EFeat::kLiveShield);
				SET_FEAT(victim, EFeat::kSpadesMaster);
								
				if (floorf(r_cha*0.9 + perc/4.0) > number(1, 140)) {
					SET_FEAT(victim, EFeat::kShadowThrower);
					SET_FEAT(victim, EFeat::kShadowSpear);
					victim->set_skill(ESkill::kDarkMagic, k_skills*0.8);
					act("&KКогти $N1 преобрели темный оттенок, будто сама тьма коснулась их.&n\n",
						false, ch, nullptr, victim, kToChar);
				}
				
				SET_FEAT(victim, EFeat::kThrowWeapon);
				SET_FEAT(victim, EFeat::kDoubleThrower);
				SET_FEAT(victim, EFeat::kTripleThrower);
				SET_FEAT(victim, EFeat::kPowerThrow);
				SET_FEAT(victim, EFeat::kDeadlyThrow);
				victim->set_str(floorf(GET_REAL_STR(victim)*1.2));
				victim->set_con(floorf(GET_REAL_CON(victim)*1.2));
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
			if (victim->IsNpc() && !MOB_FLAGGED(victim, EMobFlag::kSummoned)) { // только если не маг зверьки (Кудояр)
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
					  MUD::Skills()[skill_id].GetName(), GET_OBJ_VAL(obj, 3));
	} else {
		SendMsgToChar(ch, "повышает умение \"%s\" (не больше максимума текущего перевоплощения)\r\n",
					  MUD::Skills()[skill_id].GetName());
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
		case EObjType::kPorion: sprintf(buf, "Содержит заклинание: ");
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) <= kSpellCount)
				sprintf(buf + strlen(buf), " %s", GetSpellName(GET_OBJ_VAL(obj, 1)));
			if (GET_OBJ_VAL(obj, 2) >= 1 && GET_OBJ_VAL(obj, 2) <= kSpellCount)
				sprintf(buf + strlen(buf), ", %s", GetSpellName(GET_OBJ_VAL(obj, 2)));
			if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) <= kSpellCount)
				sprintf(buf + strlen(buf), ", %s", GetSpellName(GET_OBJ_VAL(obj, 3)));
			strcat(buf, "\r\n");
			SendMsgToChar(buf, ch);
			break;

		case EObjType::kWand:
		case EObjType::kStaff: sprintf(buf, "Вызывает заклинания: ");
			if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) <= kSpellCount)
				sprintf(buf + strlen(buf), " %s\r\n", GetSpellName(GET_OBJ_VAL(obj, 3)));
			sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
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
				case EBook::kSpell:
					if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) <= kSpellCount) {
						drndice = GET_OBJ_VAL(obj, 1);
						if (MIN_CAST_REM(spell_info[GET_OBJ_VAL(obj, 1)], ch) > GET_REAL_REMORT(ch))
							drsdice = 34;
						else
							drsdice = CalcMinSpellLevel(ch, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
						sprintf(buf, "содержит заклинание        : \"%s\"\r\n", spell_info[drndice].name);
						SendMsgToChar(buf, ch);
						sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
						SendMsgToChar(buf, ch);
					}
					break;

				case EBook::kSkill: {
					auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
					if (MUD::Skills().IsValid(skill_id)) {
						drndice = GET_OBJ_VAL(obj, 1);
						if (MUD::Classes()[ch->get_class()].skills.HasItem(skill_id)) {
							drsdice = GetSkillMinLevel(ch, skill_id, GET_OBJ_VAL(obj, 2));
						} else {
							drsdice = kLvlImplementator;
						}
						sprintf(buf, "содержит секрет умения     : \"%s\"\r\n", MUD::Skills()[skill_id].GetName());
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
						drsdice = MAX(GET_OBJ_VAL(obj, 2), imrecipes[drndice].level);
						int count = imrecipes[drndice].remort;
						if (imrecipes[drndice].classknow[(int) GET_CLASS(ch)] != KNOW_RECIPE)
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
					if (feat_id >= EFeat::kFirstFeat && feat_id <= EFeat::kLastFeat) {
						if (IsAbleToGetFeat(ch, feat_id)) {
							drsdice = feat_info[feat_id].slot[(int) GET_CLASS(ch)][(int) GET_KIN(ch)];
						} else {
							drsdice = kLvlImplementator;
						}
						sprintf(buf, "содержит секрет способности : \"%s\"\r\n", GetFeatName(feat_id));
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
					CCCYN(ch, C_NRM), MUD::Skills()[skill_id].GetName(), CCNRM(ch, C_NRM),
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
	SendMsgToChar(ch, "Перевоплощений : %d\r\n", GET_REAL_REMORT(victim));
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
		duration = MAX(GetRealLevel(ch) / 8, 2);
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
		if (!pk_agro_action(ch, victim))
			return;
	}
	if (!ch->IsNpc() && (GetRealLevel(ch) > 10))
		modi += (GetRealLevel(ch) - 10);
	if (PRF_FLAGGED(ch, EPrf::kAwake))
		modi = modi - 50;

	if (ch == victim || !CalcGeneralSaving(ch, victim, ESaving::kWill, CALC_SUCCESS(modi, 33))) {
		int i;
		for (i = 0; i <= kSpellCount; GET_SPELL_MEM(victim, i++) = 0);
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
	struct Follower *f;

	// Высосать жизнь - некроманы - уровень 18 круг 6й (5)
	// *** мин 54 макс 66 (330)

	if (IS_IMMORTAL(victim) || victim == ch || IS_CHARMICE(victim)) {
		SendMsgToChar(NOEFFECT, ch);
		return;
	}

	dam = mag_damage(GetRealLevel(ch), ch, victim, kSpellSacrifice, ESaving::kStability);
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
			if (f->ch->IsNpc()
				&& AFF_FLAGGED(f->ch, EAffect::kCharmed)
				&& MOB_FLAGGED(f->ch, EMobFlag::kCorpse)
				&& ch->in_room == IN_ROOM(f->ch)) {
				do_sacrifice(f->ch, dam);
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
			if (!IsAbleToUseFeat(tch, EFeat::kZombieDrover)) {
				continue;
			}
		}

		mag_affects(GetRealLevel(ch), ch, tch, kSpellHolystrike, ESaving::kStability);
		mag_damage(GetRealLevel(ch), ch, tch, kSpellHolystrike, ESaving::kStability);
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
	struct Follower *k, *k_next;

	auto eff_cha = get_effective_cha(ch);

	for (k = ch->followers; k; k = k_next) {
		k_next = k->next;
		if (MOB_FLAGGED(k->ch,
						EMobFlag::kTutelar))    //SendMsgToChar("Боги не обратили на вас никакого внимания!\r\n", ch);
		{
			//return;
			//пуржим старого ангела
			stop_follower(k->ch, kSfCharmlost);
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

	clear_char_skills(mob);
	Affect<EApply> af;
	af.type = kSpellCharm;
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
		mob->set_sex(ESex::kMale);
		mob->set_pc_name("Небесный защитник");
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
		mob->set_sex(ESex::kFemale);
		mob->set_pc_name("Небесная защитница");
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
	mob->set_int(MAX(50, 1 + floorf(additional_int_for_charisma * eff_cha))); //кап 50
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
	mob->mob_specials.ExtraAttack = 0;

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
	SET_SPELL(mob, kSpellCureBlind, base_spell);
	SET_SPELL(mob, kSpellRemoveHold, base_spell);
	SET_SPELL(mob, kSpellRemovePoison, base_spell);
	SET_SPELL(mob, kSpellHeal, floorf(base_heal + additional_heal_for_charisma * eff_cha));

	if (mob->get_skill(ESkill::kAwake)) {
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
	struct Follower *k, *k_next;
	for (k = ch->followers; k; k = k_next) {
		k_next = k->next;
		if (MOB_FLAGGED(k->ch, EMobFlag::kMentalShadow)) {
			stop_follower(k->ch, false);
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
	af.type = kSpellCharm;
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
     	SET_SPELL(mob, kSpellRemoveSilence, 1);
	} else if (eff_int >= 32 && eff_int < 38) {
		SET_SPELL(mob, kSpellRemoveSilence, 1);
		af.bitvector = to_underlying(EAffect::kShadowCloak);
		affect_to_char(mob, af);

	} else if(eff_int >= 38 && eff_int < 44) {
		SET_SPELL(mob, kSpellRemoveSilence, 2);
		af.bitvector = to_underlying(EAffect::kShadowCloak);
		affect_to_char(mob, af);
		
	} else if(eff_int >= 44) {
		SET_SPELL(mob, kSpellRemoveSilence, 3);
		af.bitvector = to_underlying(EAffect::kShadowCloak);
		affect_to_char(mob, af);
		af.bitvector = to_underlying(EAffect::kBrokenChains);
		affect_to_char(mob, af);
	}
	if (mob->get_skill(ESkill::kAwake)) {
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

std::string get_wear_off_text(ESpell spell)
{
	static const std::map<ESpell, std::string> spell_to_text {
		{kSpellArmor, "Вы почувствовали себя менее защищенно."},
		{kSpellTeleport, "!Teleport!"},
		{kSpellBless, "Вы почувствовали себя менее доблестно."},
		{kSpellBlindness, "Вы вновь можете видеть."},
		{kSpellBurningHands, "!Burning Hands!"},
		{kSpellCallLighting, "!Call Lightning"},
		{kSpellCharm, "Вы подчиняетесь теперь только себе."},
		{kSpellChillTouch, "Вы отметили, что силы вернулись к вам."},
		{kSpellClone, "!Clone!"},
		{kSpellIceBolts, "!Color Spray!"},
		{kSpellControlWeather, "!Control Weather!"},
		{kSpellCreateFood, "!Create Food!"},
		{kSpellCreateWater, "!Create Water!"},
		{kSpellCureBlind, "!Cure Blind!"},
		{kSpellCureCritic, "!Cure Critic!"},
		{kSpellCureLight, "!Cure Light!"},
		{kSpellCurse, "Вы почувствовали себя более уверенно."},
		{kSpellDetectAlign, "Вы более не можете определять наклонности."},
		{kSpellDetectInvis, "Вы не в состоянии больше видеть невидимых."},
		{kSpellDetectMagic, "Вы не в состоянии более определять магию."},
		{kSpellDetectPoison, "Вы не в состоянии более определять яды."},
		{kSpellDispelEvil, "!Dispel Evil!"},
		{kSpellEarthquake, "!Earthquake!"},
		{kSpellEnchantWeapon, "!Enchant Weapon!"},
		{kSpellEnergyDrain, "!Energy Drain!"},
		{kSpellFireball, "!Fireball!"},
		{kSpellHarm, "!Harm!"},
		{kSpellHeal, "!Heal!"},
		{kSpellInvisible, "Вы вновь видимы."},
		{kSpellLightingBolt, "!Lightning Bolt!"},
		{kSpellLocateObject, "!Locate object!"},
		{kSpellMagicMissile, "!Magic Missile!"},
		{kSpellPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellProtectFromEvil, "Вы вновь ощущаете страх перед тьмой."},
		{kSpellRemoveCurse, "!Remove Curse!"},
		{kSpellSanctuary, "Белая аура вокруг вашего тела угасла."},
		{kSpellShockingGasp, "!Shocking Grasp!"},
		{kSpellSleep, "Вы не чувствуете сонливости."},
		{kSpellStrength, "Вы чувствуете себя немного слабее."},
		{kSpellSummon, "!Summon!"},
		{kSpellPatronage, "Вы утратили покровительство высших сил."},
		{kSpellWorldOfRecall, "!Word of Recall!"},
		{kSpellRemovePoison, "!Remove Poison!"},
		{kSpellSenseLife, "Вы больше не можете чувствовать жизнь."},
		{kSpellAnimateDead, "!Animate Dead!"},
		{kSpellDispelGood, "!Dispel Good!"},
		{kSpellGroupArmor, "!Group Armor!"},
		{kSpellGroupHeal, "!Group Heal!"},
		{kSpellGroupRecall, "!Group Recall!"},
		{kSpellInfravision, "Вы больше не можете видеть ночью."},
		{kSpellWaterwalk, "Вы больше не можете ходить по воде."},
		{kSpellCureSerious, "!SPELL CURE SERIOUS!"},
		{kSpellGroupStrength, "!SPELL GROUP STRENGTH!"},
		{kSpellHold, "К вам вернулась способность двигаться."},
		{kSpellPowerHold, "!SPELL POWER HOLD!"},
		{kSpellMassHold, "!SPELL MASS HOLD!"},
		{kSpellFly, "Вы приземлились на землю."},
		{kSpellBrokenChains, "Вы вновь стали уязвимы для оцепенения."},
		{kSpellNoflee, "Вы опять можете сбежать с поля боя."},
		{kSpellCreateLight, "!SPELL CREATE LIGHT!"},
		{kSpellDarkness, "Облако тьмы, окружающее вас, спало."},
		{kSpellStoneSkin, "Ваша кожа вновь стала мягкой и бархатистой."},
		{kSpellCloudly, "Ваши очертания приобрели отчетливость."},
		{kSpellSllence, "Теперь вы можете болтать, все что думаете."},
		{kSpellLight, "Ваше тело перестало светиться."},
		{kSpellChainLighting, "!SPELL CHAIN LIGHTNING!"},
		{kSpellFireBlast, "!SPELL FIREBLAST!"},
		{kSpellGodsWrath, "!SPELL IMPLOSION!"},
		{kSpellWeaknes, "Силы вернулись к вам."},
		{kSpellGroupInvisible, "!SPELL GROUP INVISIBLE!"},
		{kSpellShadowCloak, "Ваша теневая мантия замерцала и растаяла."},
		{kSpellAcid, "!SPELL ACID!"},
		{kSpellRepair, "!SPELL REPAIR!"},
		{kSpellEnlarge, "Ваши размеры стали прежними."},
		{kSpellFear, "!SPELL FEAR!"},
		{kSpellSacrifice, "!SPELL SACRIFICE!"},
		{kSpellWeb, "Магическая сеть, покрывавшая вас, исчезла."},
		{kSpellBlink, "Вы перестали мигать."},
		{kSpellRemoveHold, "!SPELL REMOVE HOLD!"},
		{kSpellCamouflage, "Вы стали вновь похожи сами на себя."},
		{kSpellPowerBlindness, "!SPELL POWER BLINDNESS!"},
		{kSpellMassBlindness, "!SPELL MASS BLINDNESS!"},
		{kSpellPowerSilence, "!SPELL POWER SIELENCE!"},
		{kSpellExtraHits, "!SPELL EXTRA HITS!"},
		{kSpellResurrection, "!SPELL RESSURECTION!"},
		{kSpellMagicShield, "Ваш волшебный щит рассеялся."},
		{kSpellForbidden, "Магия, запечатывающая входы, пропала."},
		{kSpellMassSilence, "!SPELL MASS SIELENCE!"},
		{kSpellRemoveSilence, "!SPELL REMOVE SIELENCE!"},
		{kSpellDamageLight, "!SPELL DAMAGE LIGHT!"},
		{kSpellDamageSerious, "!SPELL DAMAGE SERIOUS!"},
		{kSpellDamageCritic, "!SPELL DAMAGE CRITIC!"},
		{kSpellMassCurse, "!SPELL MASS CURSE!"},
		{kSpellArmageddon, "!SPELL ARMAGEDDON!"},
		{kSpellGroupFly, "!SPELL GROUP FLY!"},
		{kSpellGroupBless, "!SPELL GROUP BLESS!"},
		{kSpellResfresh, "!SPELL REFRESH!"},
		{kSpellStunning, "!SPELL STUNNING!"},
		{kSpellHide, "Вы стали заметны окружающим."},
		{kSpellSneak, "Ваши передвижения стали заметны."},
		{kSpellDrunked, "Кураж прошел. Мама, лучше бы я умер$q вчера."},
		{kSpellAbstinent, "А головка ваша уже не болит."},
		{kSpellFullFeed, "Вам снова захотелось жареного, да с дымком."},
		{kSpellColdWind, "Вы согрелись и подвижность вернулась к вам."},
		{kSpellBattle, "К вам вернулась способность нормально сражаться."},
		{kSpellHaemorrhage, "Ваши кровоточащие раны затянулись."},
		{kSpellCourage, "Вы успокоились."},
		{kSpellWaterbreath, "Вы более не способны дышать водой."},
		{kSpellSlowdown, "Медлительность исчезла."},
		{kSpellHaste, "Вы стали более медлительны."},
		{kSpellMassSlow, "!SPELL MASS SLOW!"},
		{kSpellGroupHaste, "!SPELL MASS HASTE!"},
		{kSpellGodsShield, "Голубой кокон вокруг вашего тела угас."},
		{kSpellFever, "Лихорадка прекратилась."},
		{kSpellCureFever, "!SPELL CURE PLAQUE!"},
		{kSpellAwareness, "Вы стали менее внимательны."},
		{kSpellReligion, "Вы утратили расположение Богов."},
		{kSpellAirShield, "Ваш воздушный щит исчез."},
		{kSpellPortal, "!PORTAL!"},
		{kSpellDispellMagic, "!DISPELL MAGIC!"},
		{kSpellSummonKeeper, "!SUMMON KEEPER!"},
		{kSpellFastRegeneration, "Живительная сила покинула вас."},
		{kSpellCreateWeapon, "!CREATE WEAPON!"},
		{kSpellFireShield, "Огненный щит вокруг вашего тела исчез."},
		{kSpellRelocate, "!RELOCATE!"},
		{kSpellSummonFirekeeper, "!SUMMON FIREKEEPER!"},
		{kSpellIceShield, "Ледяной щит вокруг вашего тела исчез."},
		{kSpellIceStorm, "Ваши мышцы оттаяли и вы снова можете двигаться."},
		{kSpellLessening, "Ваши размеры вновь стали прежними."},
		{kSpellShineFlash, "!SHINE LIGHT!"},
		{kSpellMadness, "Безумие боя отпустило вас."},
		{kSpellGroupMagicGlass, "!GROUP MAGICGLASS!"},
		{kSpellCloudOfArrows, "Облако стрел вокруг вас рассеялось."},
		{kSpellVacuum, "!VACUUM!"},
		{kSpellMeteorStorm, "Последний громовой камень грянул в землю и все стихло."},
		{kSpellStoneHands, "Ваши руки вернулись к прежнему состоянию."},
		{kSpellMindless, "Ваш разум просветлел."},
		{kSpellPrismaticAura, "Призматическая аура вокруг вашего тела угасла."},
		{kSpellEviless, "Силы зла оставили вас."},
		{kSpellAirAura, "Воздушная аура вокруг вас исчезла."},
		{kSpellFireAura, "Огненная аура вокруг вас исчезла."},
		{kSpellIceAura, "Ледяная аура вокруг вас исчезла."},
		{kSpellShock, "!SHOCK!"},
		{kSpellMagicGlass, "Вы вновь чувствительны к магическим поражениям."},
		{kSpellGroupSanctuary, "!SPELL GROUP SANCTUARY!"},
		{kSpellGroupPrismaticAura, "!SPELL GROUP PRISMATICAURA!"},
		{kSpellDeafness, "Вы вновь можете слышать."},
		{kSpellPowerDeafness, "!SPELL_POWER_DEAFNESS!"},
		{kSpellRemoveDeafness, "!SPELL_REMOVE_DEAFNESS!"},
		{kSpellMassDeafness, "!SPELL_MASS_DEAFNESS!"},
		{kSpellDustStorm, "!SPELL_DUSTSTORM!"},
		{kSpellEarthfall, "!SPELL_EARTHFALL!"},
		{kSpellSonicWave, "!SPELL_SONICWAVE!"},
		{kSpellHolystrike, "!SPELL_HOLYSTRIKE!"},
		{kSpellSumonAngel, "!SPELL_SPELL_ANGEL!"},
		{kSpellMassFear, "!SPELL_SPELL_MASS_FEAR!"},
		{kSpellFascination, "Ваша красота куда-то пропала."},
		{kSpellCrying, "Ваша душа успокоилась."},
		{kSpellOblivion, "!SPELL_OBLIVION!"},
		{kSpellBurdenOfTime, "!SPELL_BURDEN_OF_TIME!"},
		{kSpellGroupRefresh, "!SPELL_GROUP_REFRESH!"},
		{kSpellPeaceful, "Смирение в вашей душе вдруг куда-то исчезло."},
		{kSpellMagicBattle, "К вам вернулась способность нормально сражаться."},
		{kSpellBerserk, "Неистовство оставило вас."},
		{kSpellStoneBones, "!stone bones!"},
		{kSpellRoomLight, "Колдовской свет угас."},
		{kSpellPoosinedFog, "Порыв ветра развеял ядовитый туман."},
		{kSpellThunderstorm, "Ветер прогнал грозовые тучи."},
		{kSpellLightWalk, "Ваши следы вновь стали заметны."},
		{kSpellFailure, "Удача вновь вернулась к вам."},
		{kSpellClanPray, "Магические чары ослабели со временем и покинули вас."},
		{kSpellGlitterDust, "Покрывавшая вас блестящая пыль осыпалась и растаяла в воздухе."},
		{kSpellScream, "Леденящий душу испуг отпустил вас."},
		{kSpellCatGrace, "Ваши движения утратили прежнюю колдовскую ловкость."},
		{kSpellBullBody, "Ваше телосложение вновь стало обычным."},
		{kSpellSnakeWisdom, "Вы утратили навеянную магией мудрость."},
		{kSpellGimmicry, "Навеянная магией хитрость покинула вас."},
		{kSpellWarcryOfChallenge, "!SPELL_WC_OF_CHALLENGE!"},
		{kSpellWarcryOfMenace, ""},
		{kSpellWarcryOfRage, "!SPELL_WC_OF_RAGE!"},
		{kSpellWarcryOfMadness, ""},
		{kSpellWarcryOfThunder, "!SPELL_WC_OF_THUNDER!"},
		{kSpellWarcryOfDefence, "Действие клича 'призыв к обороне' закончилось."},
		{kSpellWarcryOfBattle, "Действие клича битвы закончилось."},
		{kSpellWarcryOfPower, "Действие клича мощи закончилось."},
		{kSpellWarcryOfBless, "Действие клича доблести закончилось."},
		{kSpellWarcryOfCourage, "Действие клича отваги закончилось."},
		{kSpellRuneLabel, "Магические письмена на земле угасли."},
		{kSpellAconitumPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellScopolaPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellBelenaPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellDaturaPoison, "В вашей крови не осталось ни капельки яда."},
		{kSpellTimerRestore, "SPELL_TIMER_REPAIR"},
		{kSpellLucky, "!SPELL_LACKY!"},
		{kSpellBandage, "Вы аккуратно перевязали свои раны."},
		{kSpellNoBandage, "Вы снова можете перевязывать свои раны."},
		{kSpellCapable, "!SPELL_CAPABLE!"},
		{kSpellStrangle, "Удушье отпустило вас, и вы вздохнули полной грудью."},
		{kSpellRecallSpells, "Вам стало не на чем концентрироваться."},
		{kSpellHypnoticPattern, "Плывший в воздухе огненный узор потускнел и растаял струйками дыма."},
		{kSpellSolobonus, "Одна из наград прекратила действовать."},
		{kSpellVampirism, "!SPELL_VAMPIRE!"},
		{kSpellRestoration, "!SPELLS_RESTORATION!"},
		{kSpellDeathAura, "Силы нави покинули вас."},
		{kSpellRecovery, "!SPELL_RECOVERY!"},
		{kSpellMassRecovery, "!SPELL_MASS_RECOVERY!"},
		{kSpellAuraOfEvil, "Аура зла больше не помогает вам."},
		{kSpellMentalShadow, "!SPELL_MENTAL_SHADOW!"},
		{kSpellBlackTentacles, "Жуткие черные руки побледнели и расплылись зловонной дымкой."},
		{kSpellWhirlwind, "!SPELL_WHIRLWIND!"},
		{kSpellIndriksTeeth, "Каменные зубы исчезли, возвратив способность двигаться."},
		{kSpellAcidArrow, "!SPELL_MELFS_ACID_ARROW!"},
		{kSpellThunderStone, "!SPELL_THUNDERSTONE!"},
		{kSpellClod, "!SPELL_CLOD!"},
		{kSpellExpedient, "Эффект боевого приема завершился."},
		{kSpellSightOfDarkness, "!SPELL SIGHT OF DARKNESS!"},
		{kSpellGroupSincerity, "!SPELL GENERAL SINCERITY!"},
		{kSpellMagicalGaze, "!SPELL MAGICAL GAZE!"},
		{kSpellAllSeeingEye, "!SPELL ALL SEEING EYE!"},
		{kSpellEyeOfGods, "!SPELL EYE OF GODS!"},
		{kSpellBreathingAtDepth, "!SPELL BREATHING AT DEPTH!"},
		{kSpellGeneralRecovery, "!SPELL GENERAL RECOVERY!"},
		{kSpellCommonMeal, "!SPELL COMMON MEAL!"},
		{kSpellStoneWall, "!SPELL STONE WALL!"},
		{kSpellSnakeEyes, "!SPELL SNAKE EYES!"},
		{kSpellEarthAura, "Матушка земля забыла про Вас."},
		{kSpellGroupProtectFromEvil, "Вы вновь ощущаете страх перед тьмой."},
		{kSpellArrowsFire, "!NONE"},
		{kSpellArrowsWater, "!NONE"},
		{kSpellArrowsEarth, "!NONE"},
		{kSpellArrowsAir, "!NONE"},
		{kSpellArrowsDeath, "!NONE"},
		{kSpellPaladineInspiration, "*Боевое воодушевление угасло, а с ним и вся жажда подвигов!"},
		{kSpellDexterity, "Вы стали менее шустрым."},
		{kSpellGroupBlink, "!NONE"},
		{kSpellGroupCloudly, "!NONE"},
		{kSpellGroupAwareness, "!NONE"},
		{kSpellWatctyOfExpirence, "Действие клича 'обучение' закончилось."},
		{kSpellWarcryOfLuck, "Действие клича 'везение' закончилось."},
		{kSpellWarcryOfPhysdamage, "Действие клича 'точность' закончилось."},
		{kSpellMassFailure, "Удача снова повернулась к вам лицом... и залепила пощечину."},
		{kSpellSnare, "Покрывавшие вас сети колдовской западни растаяли."},
		{kSpellQUest, "Наложенные на вас чары рассеялись."},
		{kSpellExpedientFail, "Вы восстановили равновесие."}
	};

	if (!spell_to_text.count(spell)) {
		std::stringstream log_text;
		log_text << "!нет сообщения при спадении аффекта под номером: " << static_cast<int>(spell) << "!";
		//return log_text.str().c_str();
		return log_text.str();
	}

	return spell_to_text.at(spell);
}

// TODO:refactor and replate int spell by ESpell
std::optional<CastPhraseList> get_cast_phrase(int spell)
{
	// маппинг заклинания в текстовую пару [для язычника, для христианина]
	static const std::map<ESpell, CastPhraseList> cast_to_text {
		{kSpellArmor, {"буде во прибежище", "... Он - помощь наша и защита наша."}},
		{kSpellTeleport, {"несите ветры", "... дух поднял меня и перенес меня."}},
		{kSpellBless, {"истягну умь крепостию", "... даст блага просящим у Него."}},
		{kSpellBlindness, {"Чтоб твои зенки вылезли!", "... поразит тебя Господь слепотою."}},
		{kSpellBurningHands, {"узри огонь!", "... простер руку свою к огню."}},
		{kSpellCallLighting, {"Разрази тебя Перун!", "... и путь для громоносной молнии."}},
		{kSpellCharm, {"умь полонить", "... слушай пастыря сваего, и уразумей."}},
		{kSpellChillTouch, {"хладну персты воскладаше", "... которые черны от льда."}},
		{kSpellClone, {"пусть будет много меня", "... и плодились, и весьма умножились."}},
		{kSpellIceBolts, {"хлад и мраз исторгнути", "... и из воды делается лед."}},
		{kSpellControlWeather, {"стихия подкоряшися", "... власть затворить небо, чтобы не шел дождь."}},
		{kSpellCreateFood, {"будовати снедь", "... это хлеб, который Господь дал вам в пищу."}},
		{kSpellCreateWater, {"напоиши влагой", "... и потекло много воды."}},
		{kSpellCureBlind, {"зряще узрите", "... и прозрят из тьмы и мрака глаза слепых."}},
		{kSpellCureCritic, {"гой еси", "... да зарубцуются гноища твои."}},
		{kSpellCureLight, {"малейше целити раны", "... да затянутся раны твои."}},
		{kSpellCurse, {"порча", "... проклят ты пред всеми скотами."}},
		{kSpellDetectAlign, {"узряще норов", "... и отделит одних от других, как пастырь отделяет овец от козлов."}},
		{kSpellDetectInvis, {"взор мечетный", "... ибо нет ничего тайного, что не сделалось бы явным."}},
		{kSpellDetectMagic, {"зряще ворожбу", "... покажись, ересь богопротивная."}},
		{kSpellDetectPoison, {"зряще трутизну", "... по плодам их узнаете их."}},
		{kSpellDispelEvil, {"долой нощи", "... грешников преследует зло, а праведникам воздается добром."}},
		{kSpellEarthquake, {"земля тутнет", "... в тот же час произошло великое землетрясение."}},
		{kSpellEnchantWeapon, {"ницовати стружие", "... укрепи сталь Божьим перстом."}},
		{kSpellEnergyDrain, {"преторгоста", "... да иссякнут соки, питающие тело."}},
		{kSpellFireball, {"огненну солнце", "... да ниспадет огонь с неба, и пожрет их."}},
		{kSpellHarm, {"згола скверна", "... и жестокою болью во всех костях твоих."}},
		{kSpellHeal, {"згола гой еси", "... тебе говорю, встань."}},
		{kSpellInvisible, {"низовати мечетно", "... ибо видимое временно, а невидимое вечно."}},
		{kSpellLightingBolt, {"грянет гром", "... и были громы и молнии."}},
		{kSpellLocateObject, {"рища, летая умом под облакы", "... ибо всякий просящий получает, и ищущий находит."}},
		{kSpellMagicMissile, {"ворожья стрела", "... остры стрелы Твои."}},
		{kSpellPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellProtectFromEvil, {"супостат нощи", "... свет, который в тебе, да убоится тьма."}},
		{kSpellRemoveCurse, {"изыде порча", "... да простятся тебе прегрешения твои."}},
		{kSpellSanctuary, {"иже во святых", "... буде святым, аки Господь наш свят."}},
		{kSpellShockingGasp, {"воскладше огненну персты", "... и дано буде жечь врагов огнем."}},
		{kSpellSleep, {"иже дремлет", "... на веки твои тяжесть покладет."}},
		{kSpellStrength, {"будет силен", "... и человек разумный укрепляет силу свою."}},
		{kSpellSummon, {"кличу-велю", "... и послали за ним и призвали его."}},
		{kSpellPatronage, {"ибо будет угоден Богам", "... ибо спасет людей Своих от грехов их."}},
		{kSpellWorldOfRecall, {"с глаз долой исчезни", "... ступай с миром."}},
		{kSpellRemovePoison, {"изыде трутизна", "... именем Божьим, изгнати сгниенье из тела."}},
		{kSpellSenseLife, {"зряще живота", "... ибо нет ничего сокровенного, что не обнаружилось бы."}},
		{kSpellAnimateDead, {"живот изо праха створисте", "... и земля извергнет мертвецов."}},
		{kSpellDispelGood, {"свет сгинь", "... и тьма свет накроет."}},
		{kSpellGroupArmor, {"прибежище други", "... ибо кто Бог, кроме Господа, и кто защита, кроме Бога нашего?"}},
		{kSpellGroupHeal, {"други, гой еси", "... вам говорю, встаньте."}},
		{kSpellGroupRecall, {"исчезните с глаз моих", "... вам говорю, ступайте с миром."}},
		{kSpellInfravision, {"в нощи зряще", "...ибо ни днем, ни ночью сна не знают очи."}},
		{kSpellWaterwalk, {"по воде аки по суху", "... поднимись и ввергнись в море."}},
		{kSpellCureSerious, {"целите раны", "... да уменьшатся раны твои."}},
		{kSpellGroupStrength, {"други сильны", "... и даст нам Господь силу."}},
		{kSpellHold, {"аки околел", "... замри."}},
		{kSpellPowerHold, {"згола аки околел", "... замри надолго."}},
		{kSpellMassHold, {"их окалеть", "... замрите."}},
		{kSpellFly, {"летать зегзицею", "... и полетел, и понесся на крыльях ветра."}},
		{kSpellBrokenChains, {"вериги и цепи железные изорву", "... и цепи упали с рук его."}},
		{kSpellNoflee, {"опуташа в путины железны", "... да поищешь уйти, да не возможешь."}},
		{kSpellCreateLight, {"будовати светоч", "... да будет свет."}},
		{kSpellDarkness, {"тьмою прикрыты", "... тьма покроет землю."}},
		{kSpellStoneSkin, {"буде тверд аки камень", "... твердость ли камней твердость твоя?"}},
		{kSpellCloudly, {"мгла покрыла", "... будут как утренний туман."}},
		{kSpellSllence, {"типун тебе на язык!", "... да замкнутся уста твои."}},
		{kSpellLight, {"буде аки светоч", "... и да воссияет над ним свет!"}},
		{kSpellChainLighting, {"глаголят небеса", "... понесутся меткие стрелы молний из облаков."}},
		{kSpellFireBlast, {"створисте огненну струя", "... и ввергне их в озеро огненное."}},
		{kSpellGodsWrath, {"гнев божиа не минути", "... и воспламенится гнев Господа, и Он скоро истребит тебя."}},
		{kSpellWeaknes, {"буде чахнуть", "... и силу могучих ослабляет."}},
		{kSpellGroupInvisible, {"други, низовати мечетны",
								"... возвещай всем великую силу Бога. И, сказав сие, они стали невидимы."}},
		{kSpellShadowCloak, {"будут тени и туман, и мрак ночной", "... распростираются вечерние тени."}},
		{kSpellAcid, {"жги аки смола горячая", "... подобно мучению от скорпиона."}},
		{kSpellRepair, {"будь целым, аки прежде", "... заделаю трещины в ней и разрушенное восстановлю."}},
		{kSpellEnlarge, {"возросши к небу", "... и плоть выросла."}},
		{kSpellFear, {"падоша в тернии", "... убойся того, кто по убиении ввергнет в геенну."}},
		{kSpellSacrifice, {"да коснется тебя Чернобог", "... плоть твоя и тело твое будут истощены."}},
		{kSpellWeb, {"сети ловчи", "... терны и сети на пути коварного."}},
		{kSpellBlink, {"от стрел укрытие и от меча оборона", "...да защитит он себя."}},
		{kSpellRemoveHold, {"буде быстр аки прежде", "... встань, и ходи."}},
		{kSpellCamouflage, {"", ""}},
		{kSpellPowerBlindness, {"згола застить очеса", "... поразит тебя Господь слепотою навечно."}},
		{kSpellMassBlindness, {"их очеса непотребны", "... и Он поразил их слепотою."}},
		{kSpellPowerSilence, {"згола не прерчет", "... исходящее из уст твоих, да не осквернит слуха."}},
		{kSpellExtraHits, {"буде полон здоровья", "... крепкое тело лучше несметного богатства."}},
		{kSpellResurrection, {"воскресе из мертвых", "... оживут мертвецы Твои, восстанут мертвые тела!"}},
		{kSpellMagicShield, {"и ворога оберегись", "... руками своими да защитит он себя"}},
		{kSpellForbidden, {"вороги не войдут", "... ибо положена печать, и никто не возвращается."}},
		{kSpellMassSilence, {"их уста непотребны", "... да замкнутся уста ваши."}},
		{kSpellRemoveSilence, {"глаголите", "... слова из уст мудрого - благодать."}},
		{kSpellDamageLight, {"падош", "... будет чувствовать боль."}},
		{kSpellDamageSerious, {"скверна", "... постигнут тебя муки."}},
		{kSpellDamageCritic, {"сильна скверна", "... боль и муки схватили."}},
		{kSpellMassCurse, {"порча их", "... прокляты вы пред всеми скотами."}},
		{kSpellArmageddon, {"суд божиа не минути", "... какою мерою мерите, такою отмерено будет и вам."}},
		{kSpellGroupFly, {"крыла им створисте", "... и все летающие по роду их."}},
		{kSpellGroupBless, {"други, наполнися ратнаго духа", "... блажены те, слышащие слово Божие."}},
		{kSpellResfresh, {"буде свеж", "... не будет у него ни усталого, ни изнемогающего."}},
		{kSpellStunning, {"да обратит тебя Чернобог в мертвый камень!", "... и проклял его именем Господним."}},
		{kSpellHide, {"", ""}},
		{kSpellSneak, {"", ""}},
		{kSpellDrunked, {"", ""}},
		{kSpellAbstinent, {"", ""}},
		{kSpellFullFeed, {"брюхо полно", "... душа больше пищи, и тело - одежды."}},
		{kSpellColdWind, {"веют ветры", "... подует северный холодный ветер."}},
		{kSpellBattle, {"", ""}},
		{kSpellHaemorrhage, {"", ""}},
		{kSpellCourage, {"", ""}},
		{kSpellWaterbreath, {"не затвори темне березе", "... дух дышит, где хочет."}},
		{kSpellSlowdown, {"немочь", "...и помедлил еще семь дней других."}},
		{kSpellHaste, {"скор аки ястреб", "... поднимет его ветер и понесет, и он быстро побежит от него."}},
		{kSpellMassSlow, {"тернии им", "... загорожу путь их тернами."}},
		{kSpellGroupHaste, {"быстры аки ястребов стая", "... и они быстры как серны на горах."}},
		{kSpellGodsShield, {"Живый в помощи Вышняго", "... благословен буде Грядый во имя Господне."}},
		{kSpellFever, {"нутро снеде", "... и сделаются жестокие и кровавые язвы."}},
		{kSpellCureFever, {"Навь, очисти тело", "... хочу, очистись."}},
		{kSpellAwareness, {"око недреманно", "... не дам сна очам моим и веждам моим - дремания."}},
		{kSpellReligion, {"", ""}},
		{kSpellAirShield, {"Стрибог, даруй прибежище", "... защита от ветра и покров от непогоды."}},
		{kSpellPortal, {"буде путь короток", "... входите во врата Его."}},
		{kSpellDispellMagic, {"изыде ворожба", "... выйди, дух нечистый."}},
		{kSpellSummonKeeper, {"Сварог, даруй защитника", "... и благословен защитник мой!"}},
		{kSpellFastRegeneration, {"заживет, аки на собаке", "... нет богатства лучше телесного здоровья."}},
		{kSpellCreateWeapon, {"будовати стружие", "...вооружите из себя людей на войну"}},
		{kSpellFireShield, {"Хорс, даруй прибежище", "... душа горячая, как пылающий огонь."}},
		{kSpellRelocate, {"Стрибог, укажи путь...", "... указывай им путь, по которому они должны идти."}},
		{kSpellSummonFirekeeper, {"Дажьбог, даруй защитника", "... Ангел Мой с вами, и он защитник душ ваших."}},
		{kSpellIceShield, {"Морена, даруй прибежище", "... а снег и лед выдерживали огонь и не таяли."}},
		{kSpellIceStorm, {"торже, яко вихор", "... и град, величиною в талант, падет с неба."}},
		{kSpellLessening, {"буде мал аки мышь", "... плоть на нем пропадает."}},
		{kSpellShineFlash, {"засти очи им", "... свет пламени из средины огня."}},
		{kSpellMadness, {"згола яростен", "... и ярость его загорелась в нем."}},
		{kSpellGroupMagicGlass, {"гладь воды отразит", "... воздай им по делам их, по злым поступкам их."}},
		{kSpellCloudOfArrows, {"и будут стрелы молний, и зарницы в высях",
							   "... соберу на них бедствия и истощу на них стрелы Мои."}},
		{kSpellVacuum, {"Умри!", "... и услышав слова сии - пал бездыханен."}},
		{kSpellMeteorStorm, {"идти дождю стрелами", "... и камни, величиною в талант, падут с неба."}},
		{kSpellStoneHands, {"сильны велетов руки", "... рука Моя пребудет с ним, и мышца Моя укрепит его."}},
		{kSpellMindless, {"разум аки мутный омут", "... и безумие его с ним."}},
		{kSpellPrismaticAura, {"окружен радугой", "... явится радуга в облаке."}},
		{kSpellEviless, {"зло творяще", "... и ты воздашь им злом."}},
		{kSpellAirAura, {"Мать-земля, даруй защиту.", "... поклон тебе матушка земля."}},
		{kSpellFireAura, {"Сварог, даруй защиту.", "... и огонь низводит с неба."}},
		{kSpellIceAura, {"Морена, даруй защиту.", "... текущие холодные воды."}},
		{kSpellShock, {"будет слеп и глух, аки мертвец", "... кто делает или глухим, или слепым."}},
		{kSpellMagicGlass, {"Аз воздам!", "... и воздам каждому из вас."}},
		{kSpellGroupSanctuary, {"иже во святых, други", "... будьте святы, аки Господь наш свят."}},
		{kSpellGroupPrismaticAura, {"други, буде окружены радугой", "... взгляни на радугу, и прославь Сотворившего ее."}},
		{kSpellDeafness, {"оглохни", "... и глухота поразит тебя."}},
		{kSpellPowerDeafness, {"да застит уши твои", "... и будь глухим надолго."}},
		{kSpellRemoveDeafness, {"слушай глас мой", "... услышь слово Его."}},
		{kSpellMassDeafness, {"будьте глухи", "... и не будут слышать уши ваши."}},
		{kSpellDustStorm, {"пыль поднимется столбами", "... и пыль поглотит вас."}},
		{kSpellEarthfall, {"пусть каменья падут", "... и обрушатся камни с небес."}},
		{kSpellSonicWave, {"да невзлюбит тебя воздух", "... и даже воздух покарает тебя."}},
		{kSpellHolystrike, {"Велес, упокой мертвых",
							"... и предоставь мертвым погребать своих мертвецов."}},
		{kSpellSumonAngel, {"Боги, даруйте защитника", "... дабы уберег он меня от зла."}},
		{kSpellMassFear, {"Поврещу сташивые души их в скарядие!", "... и затмил ужас разум их."}},
		{kSpellFascination, {"Да пребудет с тобой вся краса мира!", "... и омолодил он, и украсил их."}},
		{kSpellCrying, {"Будут слезы твои, аки камень на сердце",
						"... и постигнет твой дух угнетение вечное."}},
		{kSpellOblivion, {"будь живот аки буява с шерстнями.",
						  "... опадет на тебя чернь страшная."}},
		{kSpellBurdenOfTime, {"Яко небытие нещадно к вам, али время вернулось вспять.",
							  "... и время не властно над ними."}},
		{kSpellGroupRefresh, {"Исполняше други силою!",
							  "...да не останется ни обделенного, ни обессиленного."}},
		{kSpellPeaceful, {"Избавь речь свою от недобрых слов, а ум - от крамольных мыслей.",
						  "... любите врагов ваших и благотворите ненавидящим вас."}},
		{kSpellMagicBattle, {"", ""}},
		{kSpellBerserk, {"", ""}},
		{kSpellStoneBones, {"Обращу кости их в твердый камень.",
							"...и тот, кто упадет на камень сей, разобьется."}},
		{kSpellRoomLight, {"Да буде СВЕТ !!!", "...ибо сказал МОНТЕР !!!"}},
		{kSpellPoosinedFog, {"Порчу воздух !!!", "...и зловонное дыхание его."}},
		{kSpellThunderstorm, {"Абие велий вихрь деяти!",
							  "...творит молнии при дожде, изводит ветер из хранилищ Своих."}},
		{kSpellLightWalk, {"", ""}},
		{kSpellFailure, {"аще доля зла и удача немилостива", ".. и несчастен, и жалок, и нищ."}},
		{kSpellClanPray, {"", ""}},
		{kSpellGlitterDust, {"зрети супостат охабиша", "...и бросали пыль на воздух."}},
		{kSpellScream, {"язвень голки уведати", "...но в полночь раздался крик."}},
		{kSpellCatGrace, {"ристати споро", "...и не уязвит враг того, кто скор."}},
		{kSpellBullBody, {"руци яре ворога супротив", "...и мощь звериная жила в теле его."}},
		{kSpellSnakeWisdom, {"веси и зрети стези отай", "...и даровал мудрость ему."}},
		{kSpellGimmicry, {"клюка вящего улучити", "...ибо кто познал ум Господень?"}},
		{kSpellWarcryOfChallenge, {"Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!",
								   "Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!"}},
		{kSpellWarcryOfMenace, {"Покрошу-изувечу, душу выну и в блины закатаю!",
								"Покрошу-изувечу, душу выну и в блины закатаю!"}},
		{kSpellWarcryOfRage, {"Не отступим, други, они наше сало сперли!",
							  "Не отступим, други, они наше сало сперли!"}},
		{kSpellWarcryOfMadness, {"Всех убью, а сам$g останусь!", "Всех убью, а сам$g останусь!"}},
		{kSpellWarcryOfThunder, {"Шоб вас приподняло, да шлепнуло!!!", "Шоб вас приподняло да шлепнуло!!!"}},
		{kSpellWarcryOfDefence, {"В строй други, защитим животами Русь нашу!",
								 "В строй други, защитим животами Русь нашу!"}},
		{kSpellWarcryOfBattle, {"Дер-ржать строй, волчьи хвосты!", "Дер-ржать строй, волчьи хвосты!"}},
		{kSpellWarcryOfPower, {"Сарынь на кичку!", "Сарынь на кичку!"}},
		{kSpellWarcryOfBless, {"Стоять крепко! За нами Киев, Подол и трактир с пивом!!!",
							   "Стоять крепко! За нами Киев, Подол и трактир с пивом!!!"}},
		{kSpellWarcryOfCourage, {"Орлы! Будем биться как львы!", "Орлы! Будем биться как львы!"}},
		{kSpellRuneLabel, {"...пьсати черты и резы.", "...и Сам отошел от них на вержение камня."}},
		{kSpellAconitumPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellScopolaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellBelenaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellDaturaPoison, {"трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле."}},
		{kSpellTimerRestore, {"", ""}},
		{kSpellLucky, {"", ""}},
		{kSpellBandage, {"", ""}},
		{kSpellNoBandage, {"", ""}},
		{kSpellCapable, {"", ""}},
		{kSpellStrangle, {"", ""}},
		{kSpellRecallSpells, {"", ""}},
		{kSpellHypnoticPattern, {"ажбо супостаты блазнити да клюковати",
								 "...и утроба его приготовляет обман."}},
		{kSpellSolobonus, {"", ""}},
		{kSpellVampirism, {"", ""}},
		{kSpellRestoration, {"Да прими вид прежний, якой был.",
							 ".. Воззри на предмет сей Отче и верни ему силу прежнюю."}},
		{kSpellDeathAura, {"Надели силою своею Навь, дабы собрать урожай тебе.",
						   "...налякай ворогов наших и покарай их немощью."}},
		{kSpellRecovery, {"Обрасти плотью сызнова.", "... прости Господи грехи, верни плоть созданию."}},
		{kSpellMassRecovery, {"Обрастите плотью сызнова.",
							  "... прости Господи грехи, верни плоть созданиям."}},
		{kSpellAuraOfEvil, {"Возьми личину зла для жатвы славной.", "Надели силой злою во благо."}},
		{kSpellMentalShadow, {"Силою мысли защиту будую себе.",
							  "Даруй Отче защиту, силой разума воздвигнутую."}},
		{kSpellBlackTentacles, {"Ато егоже руци попасти.",
								"И он не знает, что мертвецы там и что в глубине..."}},
		{kSpellWhirlwind, {"Вждати бурю обло створити.", "И поднялась великая буря..."}},
		{kSpellIndriksTeeth, {"Идеже индрика зубы супостаты изъмати.",
							  "Есть род, у которого зубы - мечи и челюсти - ножи..."}},
		{kSpellAcidArrow, {"Варно сожжет струя!",
						   "...и на коже его сделаются как бы язвы проказы"}},
		{kSpellThunderStone, {"Небесе тутнет!", "...и взял оттуда камень, и бросил из пращи."}},
		{kSpellClod, {"Онома утес низринется!",
					  "...доколе камень не оторвался от горы без содействия рук."}},
		{kSpellExpedient, {"!Применил боевой прием!", "!use battle expedient!"}},
		{kSpellSightOfDarkness, {"Что свет, что тьма - глазу одинаково.",
								 "Станьте зрячи в тьме кромешной!"}},
		{kSpellGroupSincerity, {"...да не скроются намерения.",
								"И узрим братья намерения окружающих."}},
		{kSpellMagicalGaze, {"Узрим же все, что с магией навкруги нас.",
							 "Покажи, Спаситель, магические силы братии."}},
		{kSpellAllSeeingEye, {"Все тайное станет явным.",
							  "Не спрячется, не скроется, ни заяц, ни блоха."}},
		{kSpellEyeOfGods, {"Осязаемое откройся взору!",
						   "Да не скроется от взора вашего, ни одна живая душа."}},
		{kSpellBreathingAtDepth, {"Аки стайка рыбок, плывите вместе.",
								  "Что в воде, что на земле, воздух свежим будет."}},
		{kSpellGeneralRecovery, {"...дабы пройти вместе не одну сотню верст",
								 "Сохрани Отче от усталости детей своих!"}},
		{kSpellCommonMeal, {"Благодарите богов за хлеб и соль!",
							"...дабы не осталось голодающих на свете белом"}},
		{kSpellStoneWall, {"Станем други крепки як николы!", "Укрепим тела наши перед битвой!"}},
		{kSpellSnakeEyes, {"Что яд, а что мед. Не обманемся!",
						   "...и самый сильный яд станет вам виден."}},
		{kSpellEarthAura, {"Велес, даруй защиту.", "... земля благословенна твоя."}},
		{kSpellGroupProtectFromEvil, {"други, супостат нощи",
									  "други, свет который в нас, да убоится тьма."}},
		{kSpellArrowsFire, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellArrowsWater, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellArrowsEarth, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellArrowsAir, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellArrowsDeath, {"!магический выстрел!", "!use battle expedient!"}},
		{kSpellPaladineInspiration, {"", ""}},
		{kSpellDexterity, {"будет ловким", "... и человек разумный укрепляет ловкость свою."}},
		{kSpellGroupBlink, {"защити нас от железа разящего", "... ни стрела, ни меч не пронзят печень вашу."}},
		{kSpellGroupCloudly, {"огрожу беззакония их туманом",
							  "...да защитит и покроет рассветная пелена тела ваши."}},
		{kSpellGroupAwareness, {"буде вежды ваши открыты", "... и забота о ближнем отгоняет сон от очей их."}},
		{kSpellWatctyOfExpirence, {"найдем новизну в рутине сражений!", "найдем новизну в рутине сражений!"}},
		{kSpellWarcryOfLuck, {"и пусть удача будет нашей спутницей!", "и пусть удача будет нашей спутницей!"}},
		{kSpellWarcryOfPhysdamage, {"бей в глаз, не порти шкуру", "бей в глаз, не порти шкуру."}},
		{kSpellMassFailure, {"...отче Велес, очи отвержеши!",
							 "...надежда тщетна: не упадешь ли от одного взгляда его?"}},
		{kSpellSnare, {"Заклинати поврещение в сети заскопиены!",
					   "...будет трапеза их сетью им, и мирное пиршество их - западнею."}},
		 {kSpellExpedientFail, {"!Провалил боевой прием!",
						"!Провалил боевой прием!"}}
	};

	if (!cast_to_text.count(static_cast<ESpell>(spell))) {
		return std::nullopt;
	}

	return cast_to_text.at(static_cast<ESpell>(spell));
}

typedef std::map<ESpell, std::string> ESpell_name_by_value_t;
typedef std::map<const std::string, ESpell> ESpell_value_by_name_t;
ESpell_name_by_value_t ESpell_name_by_value;
ESpell_value_by_name_t ESpell_value_by_name;
void init_ESpell_ITEM_NAMES() {
	ESpell_value_by_name.clear();
	ESpell_name_by_value.clear();

	ESpell_name_by_value[ESpell::kIncorrect] = "kSpellNoSpell";
	ESpell_name_by_value[ESpell::kSpellArmor] = "kSpellArmor";
	ESpell_name_by_value[ESpell::kSpellTeleport] = "kSpellTeleport";
	ESpell_name_by_value[ESpell::kSpellBless] = "kSpellBless";
	ESpell_name_by_value[ESpell::kSpellBlindness] = "kSpellBlindness";
	ESpell_name_by_value[ESpell::kSpellBurningHands] = "kSpellBurningHands";
	ESpell_name_by_value[ESpell::kSpellCallLighting] = "kSpellCallLighting";
	ESpell_name_by_value[ESpell::kSpellCharm] = "kSpellCharm";
	ESpell_name_by_value[ESpell::kSpellChillTouch] = "kSpellChillTouch";
	ESpell_name_by_value[ESpell::kSpellClone] = "kSpellClone";
	ESpell_name_by_value[ESpell::kSpellIceBolts] = "kSpellIceBolts";
	ESpell_name_by_value[ESpell::kSpellControlWeather] = "kSpellControlWeather";
	ESpell_name_by_value[ESpell::kSpellCreateFood] = "kSpellCreateFood";
	ESpell_name_by_value[ESpell::kSpellCreateWater] = "kSpellCreateWater";
	ESpell_name_by_value[ESpell::kSpellCureBlind] = "kSpellCureBlind";
	ESpell_name_by_value[ESpell::kSpellCureCritic] = "kSpellCureCritic";
	ESpell_name_by_value[ESpell::kSpellCureLight] = "kSpellCureLight";
	ESpell_name_by_value[ESpell::kSpellCurse] = "kSpellCurse";
	ESpell_name_by_value[ESpell::kSpellDetectAlign] = "kSpellDetectAlign";
	ESpell_name_by_value[ESpell::kSpellDetectInvis] = "kSpellDetectInvis";
	ESpell_name_by_value[ESpell::kSpellDetectMagic] = "kSpellDetectMagic";
	ESpell_name_by_value[ESpell::kSpellDetectPoison] = "kSpellDetectPoison";
	ESpell_name_by_value[ESpell::kSpellDispelEvil] = "kSpellDispelEvil";
	ESpell_name_by_value[ESpell::kSpellEarthquake] = "kSpellEarthquake";
	ESpell_name_by_value[ESpell::kSpellEnchantWeapon] = "kSpellEnchantWeapon";
	ESpell_name_by_value[ESpell::kSpellEnergyDrain] = "kSpellEnergyDrain";
	ESpell_name_by_value[ESpell::kSpellFireball] = "kSpellFireball";
	ESpell_name_by_value[ESpell::kSpellHarm] = "kSpellHarm";
	ESpell_name_by_value[ESpell::kSpellHeal] = "kSpellHeal";
	ESpell_name_by_value[ESpell::kSpellInvisible] = "kSpellInvisible";
	ESpell_name_by_value[ESpell::kSpellLightingBolt] = "kSpellLightingBolt";
	ESpell_name_by_value[ESpell::kSpellLocateObject] = "kSpellLocateObject";
	ESpell_name_by_value[ESpell::kSpellMagicMissile] = "kSpellMagicMissile";
	ESpell_name_by_value[ESpell::kSpellPoison] = "kSpellPoison";
	ESpell_name_by_value[ESpell::kSpellProtectFromEvil] = "kSpellProtectFromEvil";
	ESpell_name_by_value[ESpell::kSpellRemoveCurse] = "kSpellRemoveCurse";
	ESpell_name_by_value[ESpell::kSpellSanctuary] = "kSpellSanctuary";
	ESpell_name_by_value[ESpell::kSpellShockingGasp] = "kSpellShockingGasp";
	ESpell_name_by_value[ESpell::kSpellSleep] = "kSpellSleep";
	ESpell_name_by_value[ESpell::kSpellStrength] = "kSpellStrength";
	ESpell_name_by_value[ESpell::kSpellSummon] = "kSpellSummon";
	ESpell_name_by_value[ESpell::kSpellPatronage] = "kSpellPatronage";
	ESpell_name_by_value[ESpell::kSpellWorldOfRecall] = "kSpellWorldOfRecall";
	ESpell_name_by_value[ESpell::kSpellRemovePoison] = "kSpellRemovePoison";
	ESpell_name_by_value[ESpell::kSpellSenseLife] = "kSpellSenseLife";
	ESpell_name_by_value[ESpell::kSpellAnimateDead] = "kSpellAnimateDead";
	ESpell_name_by_value[ESpell::kSpellDispelGood] = "kSpellDispelGood";
	ESpell_name_by_value[ESpell::kSpellGroupArmor] = "kSpellGroupArmor";
	ESpell_name_by_value[ESpell::kSpellGroupHeal] = "kSpellGroupHeal";
	ESpell_name_by_value[ESpell::kSpellGroupRecall] = "kSpellGroupRecall";
	ESpell_name_by_value[ESpell::kSpellInfravision] = "kSpellInfravision";
	ESpell_name_by_value[ESpell::kSpellWaterwalk] = "kSpellWaterwalk";
	ESpell_name_by_value[ESpell::kSpellCureSerious] = "kSpellCureSerious";
	ESpell_name_by_value[ESpell::kSpellGroupStrength] = "kSpellGroupStrength";
	ESpell_name_by_value[ESpell::kSpellHold] = "kSpellHold";
	ESpell_name_by_value[ESpell::kSpellPowerHold] = "kSpellPowerHold";
	ESpell_name_by_value[ESpell::kSpellMassHold] = "kSpellMassHold";
	ESpell_name_by_value[ESpell::kSpellFly] = "kSpellFly";
	ESpell_name_by_value[ESpell::kSpellBrokenChains] = "kSpellBrokenChains";
	ESpell_name_by_value[ESpell::kSpellNoflee] = "kSpellNoflee";
	ESpell_name_by_value[ESpell::kSpellCreateLight] = "kSpellCreateLight";
	ESpell_name_by_value[ESpell::kSpellDarkness] = "kSpellDarkness";
	ESpell_name_by_value[ESpell::kSpellStoneSkin] = "kSpellStoneSkin";
	ESpell_name_by_value[ESpell::kSpellCloudly] = "kSpellCloudly";
	ESpell_name_by_value[ESpell::kSpellSllence] = "kSpellSllence";
	ESpell_name_by_value[ESpell::kSpellLight] = "kSpellLight";
	ESpell_name_by_value[ESpell::kSpellChainLighting] = "kSpellChainLighting";
	ESpell_name_by_value[ESpell::kSpellFireBlast] = "kSpellFireBlast";
	ESpell_name_by_value[ESpell::kSpellGodsWrath] = "kSpellGodsWrath";
	ESpell_name_by_value[ESpell::kSpellWeaknes] = "kSpellWeaknes";
	ESpell_name_by_value[ESpell::kSpellGroupInvisible] = "kSpellGroupInvisible";
	ESpell_name_by_value[ESpell::kSpellShadowCloak] = "kSpellShadowCloak";
	ESpell_name_by_value[ESpell::kSpellAcid] = "kSpellAcid";
	ESpell_name_by_value[ESpell::kSpellRepair] = "kSpellRepair";
	ESpell_name_by_value[ESpell::kSpellEnlarge] = "kSpellEnlarge";
	ESpell_name_by_value[ESpell::kSpellFear] = "kSpellFear";
	ESpell_name_by_value[ESpell::kSpellSacrifice] = "kSpellSacrifice";
	ESpell_name_by_value[ESpell::kSpellWeb] = "kSpellWeb";
	ESpell_name_by_value[ESpell::kSpellBlink] = "kSpellBlink";
	ESpell_name_by_value[ESpell::kSpellRemoveHold] = "kSpellRemoveHold";
	ESpell_name_by_value[ESpell::kSpellCamouflage] = "kSpellCamouflage";
	ESpell_name_by_value[ESpell::kSpellPowerBlindness] = "kSpellPowerBlindness";
	ESpell_name_by_value[ESpell::kSpellMassBlindness] = "kSpellMassBlindness";
	ESpell_name_by_value[ESpell::kSpellPowerSilence] = "kSpellPowerSilence";
	ESpell_name_by_value[ESpell::kSpellExtraHits] = "kSpellExtraHits";
	ESpell_name_by_value[ESpell::kSpellResurrection] = "kSpellResurrection";
	ESpell_name_by_value[ESpell::kSpellMagicShield] = "kSpellMagicShield";
	ESpell_name_by_value[ESpell::kSpellForbidden] = "kSpellForbidden";
	ESpell_name_by_value[ESpell::kSpellMassSilence] = "kSpellMassSilence";
	ESpell_name_by_value[ESpell::kSpellRemoveSilence] = "kSpellRemoveSilence";
	ESpell_name_by_value[ESpell::kSpellDamageLight] = "kSpellDamageLight";
	ESpell_name_by_value[ESpell::kSpellDamageSerious] = "kSpellDamageSerious";
	ESpell_name_by_value[ESpell::kSpellDamageCritic] = "kSpellDamageCritic";
	ESpell_name_by_value[ESpell::kSpellMassCurse] = "kSpellMassCurse";
	ESpell_name_by_value[ESpell::kSpellArmageddon] = "kSpellArmageddon";
	ESpell_name_by_value[ESpell::kSpellGroupFly] = "kSpellGroupFly";
	ESpell_name_by_value[ESpell::kSpellGroupBless] = "kSpellGroupBless";
	ESpell_name_by_value[ESpell::kSpellResfresh] = "kSpellResfresh";
	ESpell_name_by_value[ESpell::kSpellStunning] = "kSpellStunning";
	ESpell_name_by_value[ESpell::kSpellHide] = "kSpellHide";
	ESpell_name_by_value[ESpell::kSpellSneak] = "kSpellSneak";
	ESpell_name_by_value[ESpell::kSpellDrunked] = "kSpellDrunked";
	ESpell_name_by_value[ESpell::kSpellAbstinent] = "kSpellAbstinent";
	ESpell_name_by_value[ESpell::kSpellFullFeed] = "kSpellFullFeed";
	ESpell_name_by_value[ESpell::kSpellColdWind] = "kSpellColdWind";
	ESpell_name_by_value[ESpell::kSpellBattle] = "kSpellBattle";
	ESpell_name_by_value[ESpell::kSpellHaemorrhage] = "kSpellHaemorragis";
	ESpell_name_by_value[ESpell::kSpellCourage] = "kSpellCourage";
	ESpell_name_by_value[ESpell::kSpellWaterbreath] = "kSpellWaterbreath";
	ESpell_name_by_value[ESpell::kSpellSlowdown] = "kSpellSlowdown";
	ESpell_name_by_value[ESpell::kSpellHaste] = "kSpellHaste";
	ESpell_name_by_value[ESpell::kSpellMassSlow] = "kSpellMassSlow";
	ESpell_name_by_value[ESpell::kSpellGroupHaste] = "kSpellGroupHaste";
	ESpell_name_by_value[ESpell::kSpellGodsShield] = "kSpellGodsShield";
	ESpell_name_by_value[ESpell::kSpellFever] = "kSpellFever";
	ESpell_name_by_value[ESpell::kSpellCureFever] = "kSpellCureFever";
	ESpell_name_by_value[ESpell::kSpellAwareness] = "kSpellAwareness";
	ESpell_name_by_value[ESpell::kSpellReligion] = "kSpellReligion";
	ESpell_name_by_value[ESpell::kSpellAirShield] = "kSpellAirShield";
	ESpell_name_by_value[ESpell::kSpellPortal] = "kSpellPortal";
	ESpell_name_by_value[ESpell::kSpellDispellMagic] = "kSpellDispellMagic";
	ESpell_name_by_value[ESpell::kSpellSummonKeeper] = "kSpellSummonKeeper";
	ESpell_name_by_value[ESpell::kSpellFastRegeneration] = "kSpellFastRegeneration";
	ESpell_name_by_value[ESpell::kSpellCreateWeapon] = "kSpellCreateWeapon";
	ESpell_name_by_value[ESpell::kSpellFireShield] = "kSpellFireShield";
	ESpell_name_by_value[ESpell::kSpellRelocate] = "kSpellRelocate";
	ESpell_name_by_value[ESpell::kSpellSummonFirekeeper] = "kSpellSummonFirekeeper";
	ESpell_name_by_value[ESpell::kSpellIceShield] = "kSpellIceShield";
	ESpell_name_by_value[ESpell::kSpellIceStorm] = "kSpellIceStorm";
	ESpell_name_by_value[ESpell::kSpellLessening] = "kSpellLessening";
	ESpell_name_by_value[ESpell::kSpellShineFlash] = "kSpellShineFlash";
	ESpell_name_by_value[ESpell::kSpellMadness] = "kSpellMadness";
	ESpell_name_by_value[ESpell::kSpellGroupMagicGlass] = "kSpellGroupMagicGlass";
	ESpell_name_by_value[ESpell::kSpellCloudOfArrows] = "kSpellCloudOfArrows";
	ESpell_name_by_value[ESpell::kSpellVacuum] = "kSpellVacuum";
	ESpell_name_by_value[ESpell::kSpellMeteorStorm] = "kSpellMeteorStorm";
	ESpell_name_by_value[ESpell::kSpellStoneHands] = "kSpellStoneHands";
	ESpell_name_by_value[ESpell::kSpellMindless] = "kSpellMindless";
	ESpell_name_by_value[ESpell::kSpellPrismaticAura] = "kSpellPrismaticAura";
	ESpell_name_by_value[ESpell::kSpellEviless] = "kSpellEviless";
	ESpell_name_by_value[ESpell::kSpellAirAura] = "kSpellAirAura";
	ESpell_name_by_value[ESpell::kSpellFireAura] = "kSpellFireAura";
	ESpell_name_by_value[ESpell::kSpellIceAura] = "kSpellIceAura";
	ESpell_name_by_value[ESpell::kSpellShock] = "kSpellShock";
	ESpell_name_by_value[ESpell::kSpellMagicGlass] = "kSpellMagicGlass";
	ESpell_name_by_value[ESpell::kSpellGroupSanctuary] = "kSpellGroupSanctuary";
	ESpell_name_by_value[ESpell::kSpellGroupPrismaticAura] = "kSpellGroupPrismaticAura";
	ESpell_name_by_value[ESpell::kSpellDeafness] = "kSpellDeafness";
	ESpell_name_by_value[ESpell::kSpellPowerDeafness] = "kSpellPowerDeafness";
	ESpell_name_by_value[ESpell::kSpellRemoveDeafness] = "kSpellRemoveDeafness";
	ESpell_name_by_value[ESpell::kSpellMassDeafness] = "kSpellMassDeafness";
	ESpell_name_by_value[ESpell::kSpellDustStorm] = "kSpellDustStorm";
	ESpell_name_by_value[ESpell::kSpellEarthfall] = "kSpellEarthfall";
	ESpell_name_by_value[ESpell::kSpellSonicWave] = "kSpellSonicWave";
	ESpell_name_by_value[ESpell::kSpellHolystrike] = "kSpellHolystrike";
	ESpell_name_by_value[ESpell::kSpellSumonAngel] = "kSpellSumonAngel";
	ESpell_name_by_value[ESpell::kSpellMassFear] = "kSpellMassFear";
	ESpell_name_by_value[ESpell::kSpellFascination] = "kSpellFascination";
	ESpell_name_by_value[ESpell::kSpellCrying] = "kSpellCrying";
	ESpell_name_by_value[ESpell::kSpellOblivion] = "kSpellOblivion";
	ESpell_name_by_value[ESpell::kSpellBurdenOfTime] = "kSpellBurdenOfTime";
	ESpell_name_by_value[ESpell::kSpellGroupRefresh] = "kSpellGroupRefresh";
	ESpell_name_by_value[ESpell::kSpellPeaceful] = "kSpellPeaceful";
	ESpell_name_by_value[ESpell::kSpellMagicBattle] = "kSpellMagicBattle";
	ESpell_name_by_value[ESpell::kSpellBerserk] = "kSpellBerserk";
	ESpell_name_by_value[ESpell::kSpellStoneBones] = "kSpellStoneBones";
	ESpell_name_by_value[ESpell::kSpellRoomLight] = "kSpellRoomLight";
	ESpell_name_by_value[ESpell::kSpellPoosinedFog] = "kSpellPoosinedFog";
	ESpell_name_by_value[ESpell::kSpellThunderstorm] = "kSpellThunderstorm";
	ESpell_name_by_value[ESpell::kSpellLightWalk] = "kSpellLightWalk";
	ESpell_name_by_value[ESpell::kSpellFailure] = "kSpellFailure";
	ESpell_name_by_value[ESpell::kSpellClanPray] = "kSpellClanPray";
	ESpell_name_by_value[ESpell::kSpellGlitterDust] = "kSpellGlitterDust";
	ESpell_name_by_value[ESpell::kSpellScream] = "kSpellScream";
	ESpell_name_by_value[ESpell::kSpellCatGrace] = "kSpellCatGrace";
	ESpell_name_by_value[ESpell::kSpellBullBody] = "kSpellBullBody";
	ESpell_name_by_value[ESpell::kSpellSnakeWisdom] = "kSpellSnakeWisdom";
	ESpell_name_by_value[ESpell::kSpellGimmicry] = "kSpellGimmicry";
	ESpell_name_by_value[ESpell::kSpellWarcryOfChallenge] = "kSpellWarcryOfChallenge";
	ESpell_name_by_value[ESpell::kSpellWarcryOfMenace] = "kSpellWarcryOfMenace";
	ESpell_name_by_value[ESpell::kSpellWarcryOfRage] = "kSpellWarcryOfRage";
	ESpell_name_by_value[ESpell::kSpellWarcryOfMadness] = "kSpellWarcryOfMadness";
	ESpell_name_by_value[ESpell::kSpellWarcryOfThunder] = "kSpellWarcryOfThunder";
	ESpell_name_by_value[ESpell::kSpellWarcryOfDefence] = "kSpellWarcryOfDefence";
	ESpell_name_by_value[ESpell::kSpellWarcryOfBattle] = "kSpellWarcryOfBattle";
	ESpell_name_by_value[ESpell::kSpellWarcryOfPower] = "kSpellWarcryOfPower";
	ESpell_name_by_value[ESpell::kSpellWarcryOfBless] = "kSpellWarcryOfBless";
	ESpell_name_by_value[ESpell::kSpellWarcryOfCourage] = "kSpellWarcryOfCourage";
	ESpell_name_by_value[ESpell::kSpellRuneLabel] = "kSpellRuneLabel";
	ESpell_name_by_value[ESpell::kSpellAconitumPoison] = "kSpellAconitumPoison";
	ESpell_name_by_value[ESpell::kSpellScopolaPoison] = "kSpellScopolaPoison";
	ESpell_name_by_value[ESpell::kSpellBelenaPoison] = "kSpellBelenaPoison";
	ESpell_name_by_value[ESpell::kSpellDaturaPoison] = "kSpellDaturaPoison";
	ESpell_name_by_value[ESpell::kSpellTimerRestore] = "kSpellTimerRestore";
	ESpell_name_by_value[ESpell::kSpellLucky] = "kSpellLucky";
	ESpell_name_by_value[ESpell::kSpellBandage] = "kSpellBandage";
	ESpell_name_by_value[ESpell::kSpellNoBandage] = "kSpellNoBandage";
	ESpell_name_by_value[ESpell::kSpellCapable] = "kSpellCapable";
	ESpell_name_by_value[ESpell::kSpellStrangle] = "kSpellStrangle";
	ESpell_name_by_value[ESpell::kSpellRecallSpells] = "kSpellRecallSpells";
	ESpell_name_by_value[ESpell::kSpellHypnoticPattern] = "kSpellHypnoticPattern";
	ESpell_name_by_value[ESpell::kSpellSolobonus] = "kSpellSolobonus";
	ESpell_name_by_value[ESpell::kSpellVampirism] = "kSpellVampirism";
	ESpell_name_by_value[ESpell::kSpellRestoration] = "kSpellRestoration";
	ESpell_name_by_value[ESpell::kSpellDeathAura] = "kSpellDeathAura";
	ESpell_name_by_value[ESpell::kSpellRecovery] = "kSpellRecovery";
	ESpell_name_by_value[ESpell::kSpellMassRecovery] = "kSpellMassRecovery";
	ESpell_name_by_value[ESpell::kSpellAuraOfEvil] = "kSpellAuraOfEvil";
	ESpell_name_by_value[ESpell::kSpellMentalShadow] = "kSpellMentalShadow";
	ESpell_name_by_value[ESpell::kSpellBlackTentacles] = "kSpellBlackTentacles";
	ESpell_name_by_value[ESpell::kSpellWhirlwind] = "kSpellWhirlwind";
	ESpell_name_by_value[ESpell::kSpellIndriksTeeth] = "kSpellIndriksTeeth";
	ESpell_name_by_value[ESpell::kSpellAcidArrow] = "kSpellAcidArrow";
	ESpell_name_by_value[ESpell::kSpellThunderStone] = "kSpellThunderStone";
	ESpell_name_by_value[ESpell::kSpellClod] = "kSpellClod";
	ESpell_name_by_value[ESpell::kSpellExpedient] = "kSpellExpedient";
	ESpell_name_by_value[ESpell::kSpellSightOfDarkness] = "kSpellSightOfDarkness";
	ESpell_name_by_value[ESpell::kSpellGroupSincerity] = "kSpellGroupSincerity";
	ESpell_name_by_value[ESpell::kSpellMagicalGaze] = "kSpellMagicalGaze";
	ESpell_name_by_value[ESpell::kSpellAllSeeingEye] = "kSpellAllSeeingEye";
	ESpell_name_by_value[ESpell::kSpellEyeOfGods] = "kSpellEyeOfGods";
	ESpell_name_by_value[ESpell::kSpellBreathingAtDepth] = "kSpellBreathingAtDepth";
	ESpell_name_by_value[ESpell::kSpellGeneralRecovery] = "kSpellGeneralRecovery";
	ESpell_name_by_value[ESpell::kSpellCommonMeal] = "kSpellCommonMeal";
	ESpell_name_by_value[ESpell::kSpellStoneWall] = "kSpellStoneWall";
	ESpell_name_by_value[ESpell::kSpellSnakeEyes] = "kSpellSnakeEyes";
	ESpell_name_by_value[ESpell::kSpellEarthAura] = "kSpellEarthAura";
	ESpell_name_by_value[ESpell::kSpellGroupProtectFromEvil] = "kSpellGroupProtectFromEvil";
	ESpell_name_by_value[ESpell::kSpellArrowsFire] = "kSpellArrowsFire";
	ESpell_name_by_value[ESpell::kSpellArrowsWater] = "kSpellArrowsWater";
	ESpell_name_by_value[ESpell::kSpellArrowsEarth] = "kSpellArrowsEarth";
	ESpell_name_by_value[ESpell::kSpellArrowsAir] = "kSpellArrowsAir";
	ESpell_name_by_value[ESpell::kSpellArrowsDeath] = "kSpellArrowsDeath";
	ESpell_name_by_value[ESpell::kSpellPaladineInspiration] = "kSpellPaladineInspiration";
	ESpell_name_by_value[ESpell::kSpellDexterity] = "kSpellDexterity";
	ESpell_name_by_value[ESpell::kSpellGroupBlink] = "kSpellGroupBlink";
	ESpell_name_by_value[ESpell::kSpellGroupCloudly] = "kSpellGroupCloudly";
	ESpell_name_by_value[ESpell::kSpellGroupAwareness] = "kSpellGroupAwareness";
	ESpell_name_by_value[ESpell::kSpellMassFailure] = "kSpellMassFailure";
	ESpell_name_by_value[ESpell::kSpellSnare] = "kSpellSnare";
	ESpell_name_by_value[ESpell::kSpellExpedientFail] = "kSpellExpedientFail";

	for (const auto &i : ESpell_name_by_value) {
		ESpell_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ESpell>(const ESpell item) {
	if (ESpell_name_by_value.empty()) {
		init_ESpell_ITEM_NAMES();
	}
	return ESpell_name_by_value.at(item);
}

template<>
ESpell ITEM_BY_NAME(const std::string &name) {
	if (ESpell_name_by_value.empty()) {
		init_ESpell_ITEM_NAMES();
	}
	return ESpell_value_by_name.at(name);
}

std::map<int /* vnum */, int /* count */> rune_list;

void add_rune_stats(CharData *ch, int vnum, int spelltype) {
	if (ch->IsNpc() || kSpellRunes != spelltype) {
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
	} else if (spelltype != kSpellRunes) {
		extract = true;
	}

	if (extract) {
		if (spelltype == kSpellRunes) {
			snprintf(buf, kMaxStringLength, "$o%s рассыпал$U у вас в руках.",
					 char_get_custom_label(obj, ch).c_str());
			act(buf, false, ch, obj, nullptr, kToChar);
		}
		ExtractObjFromChar(obj);
		ExtractObjFromWorld(obj);
	}
}

int CheckRecipeValues(CharData *ch, int spellnum, int spelltype, int showrecipe) {
	int item0 = -1, item1 = -1, item2 = -1, obj_num = -1;
	struct SpellCreateItem *items;

	if (spellnum <= 0 || spellnum > kSpellCount)
		return (false);
	if (spelltype == kSpellItems) {
		items = &spell_create[spellnum].items;
	} else if (spelltype == kSpellPotion) {
		items = &spell_create[spellnum].potion;
	} else if (spelltype == kSpellWand) {
		items = &spell_create[spellnum].wand;
	} else if (spelltype == kSpellScroll) {
		items = &spell_create[spellnum].scroll;
	} else if (spelltype == kSpellRunes) {
		items = &spell_create[spellnum].runes;
	} else
		return (false);

	if (((obj_num = real_object(items->rnumber)) < 0 &&
		spelltype != kSpellItems && spelltype != kSpellRunes) ||
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
		if (obj_num >= 0 && (spelltype == kSpellItems || spelltype == kSpellRunes)) {
			strcat(buf, CCIBLU(ch, C_NRM));
			strcat(buf, obj_proto[obj_num]->get_PName(0).c_str());
			strcat(buf, "\r\n");
		}

		strcat(buf, CCNRM(ch, C_NRM));
		if (spelltype == kSpellItems || spelltype == kSpellRunes) {
			strcat(buf, "для создания магии '");
			strcat(buf, GetSpellName(spellnum));
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

	if (spelltype == kSpellRunes
		&& GET_OBJ_TYPE(obj) != EObjType::kIngredient) {
		return false;
	}

	if (GET_OBJ_TYPE(obj) == EObjType::kIngredient) {
		if ((!IS_SET(GET_OBJ_SKILL(obj), kItemRunes) && spelltype == kSpellRunes)
			|| (IS_SET(GET_OBJ_SKILL(obj), kItemRunes) && spelltype != kSpellRunes)) {
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
		if (GET_OBJ_VAL(obj, 3) + num - 5 * GET_REAL_REMORT(ch) >= time(nullptr))
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
		if (GetRealLevel(ch) + GET_REAL_REMORT(ch) < num)
			return false;
	}

	return true;
}

int CheckRecipeItems(CharData *ch, int spellnum, int spelltype, int extract, const CharData *targ) {
	ObjData *obj0 = nullptr, *obj1 = nullptr, *obj2 = nullptr, *obj3 = nullptr, *objo = nullptr;
	int item0 = -1, item1 = -1, item2 = -1, item3 = -1;
	int create = 0, obj_num = -1, percent = 0, num = 0;
	ESkill skill_id = ESkill::kIncorrect;
	struct SpellCreateItem *items;

	if (spellnum <= 0
		|| spellnum > kSpellCount) {
		return (false);
	}
	if (spelltype == kSpellItems) {
		items = &spell_create[spellnum].items;
	} else if (spelltype == kSpellPotion) {
		items = &spell_create[spellnum].potion;
		skill_id = ESkill::kCreatePotion;
		create = 1;
	} else if (spelltype == kSpellWand) {
		items = &spell_create[spellnum].wand;
		skill_id = ESkill::kCreateWand;
		create = 1;
	} else if (spelltype == kSpellScroll) {
		items = &spell_create[spellnum].scroll;
		skill_id = ESkill::kCreateScroll;
		create = 1;
	} else if (spelltype == kSpellRunes) {
		items = &spell_create[spellnum].runes;
	} else {
		return (false);
	}

	if (((spelltype == kSpellRunes || spelltype == kSpellItems) &&
		(item3 = items->rnumber) +
			(item0 = items->items[0]) +
			(item1 = items->items[1]) +
			(item2 = items->items[2]) < -3)
		|| ((spelltype == kSpellScroll
			|| spelltype == kSpellWand
			|| spelltype == kSpellPotion)
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
			&& mag_item_ok(ch, obj, spelltype)) {
			obj0 = obj;
			item0 = -2;
			objo = obj0;
			num++;
		} else if (item1 >= 0 && item1_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item1_rnum], 1)
			&& mag_item_ok(ch, obj, spelltype)) {
			obj1 = obj;
			item1 = -2;
			objo = obj1;
			num++;
		} else if (item2 >= 0 && item2_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item2_rnum], 1)
			&& mag_item_ok(ch, obj, spelltype)) {
			obj2 = obj;
			item2 = -2;
			objo = obj2;
			num++;
		} else if (item3 >= 0 && item3_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item3_rnum], 1)
			&& mag_item_ok(ch, obj, spelltype)) {
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
		if (spelltype == kSpellRunes) {
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
				percent = number(1, MUD::Skills()[skill_id].difficulty);
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
			add_rune_stats(ch, GET_OBJ_VAL(obj0, 1), spelltype);
		}

		if (item1 == -2) {
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj1->get_PName(3).c_str());
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj1, 1), spelltype);
		}

		if (item2 == -2) {
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj2->get_PName(3).c_str());
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj2, 1), spelltype);
		}

		if (item3 == -2) {
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj3->get_PName(3).c_str());
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj3, 1), spelltype);
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
			if (spelltype == kSpellItems) {
				strcat(buf, "и создали магическую смесь.\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n смешал$g что-то в своей ноше.\r\n"
					"Вы почувствовали резкий запах.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			} else if (spelltype == kSpellRunes) {
				sprintf(buf + strlen(buf),
						"котор%s вспыхнул%s ярким светом.%s",
						num > 1 ? "ые" : GET_OBJ_SUF_3(objo), num > 1 ? "и" : GET_OBJ_SUF_1(objo),
						PRF_FLAGGED(ch, EPrf::kCompact) ? "" : "\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n сложил$g руны, которые вспыхнули ярким пламенем.",
					true, ch, nullptr, nullptr, kToRoom);
				sprintf(buf, "$n сложил$g руны в заклинание '%s'%s%s.",
						GetSpellName(spellnum),
						(targ && targ != ch ? " на " : ""),
						(targ && targ != ch ? GET_PAD(targ, 1) : ""));
				act(buf, true, ch, nullptr, nullptr, kToArenaListen);
				auto magic_skill = GetMagicSkillId(spellnum);
				if (MUD::Skills().IsValid(magic_skill)) {
					TrainSkill(ch, magic_skill, true, nullptr);
				}
			}
		}
		extract_item(ch, obj0, spelltype);
		extract_item(ch, obj1, spelltype);
		extract_item(ch, obj2, spelltype);
		extract_item(ch, obj3, spelltype);
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
	std::string out(
		"Rune stats:\r\n"
		"vnum -> count\r\n"
		"--------------\r\n");
	for (std::multimap<int, int>::const_reverse_iterator i = tmp_list.rbegin(),
			 iend = tmp_list.rend(); i != iend; ++i) {
		out += boost::str(boost::format("%1% -> %2%\r\n") % i->second % i->first);
	}
	SendMsgToChar(out, ch);
}

void print_rune_log() {
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
			 iend = rune_list.end(); i != iend; ++i) {
		log("RuneUsed: %d %d", i->first, i->second);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
