/**
\file sight.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Модуль оператора над игровыми сущностями - персонажами, предметами и комнатами.
\details Тут должен размещаться код, который перемещает и размещает персонажей, предметы и мобов.
 То есть, в комнату, из комнаты, в контейнер, в инвентарь и обратно и так далее. Либо сообщает их численность,
 составляет списки и тому подобное. Что тут нужно сделать - удалить отсюда и перенести в соответствующие модули
 код, который не относится к таким действиям. Если после этого модуль будет чересчур большим - раздеить его
 на модули объектов, персонажей и комнат/мира.
*/

//#include "handler.h"

#include "gameplay/economics/auction.h"
#include "utils/backtrace.h"
#include "utils_char_obj.inl"
#include "engine/entities/char_player.h"
#include "engine/db/world_characters.h"
#include "engine/ui/cmd/do_follow.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/economics/ext_money.h"
#include "gameplay/fight/fight.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/magic/magic.h"
#include "engine/db/obj_prototypes.h"
#include "engine/ui/color.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/classes/classes_spell_slots.h"
#include "gameplay/mechanics/depot.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/cities.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/magic/magic_rooms.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/base_stats.h"
#include "utils/utils_time.h"
#include "gameplay/classes/pc_classes.h"

using classes::CalcCircleSlotsAmount;

// local functions //
int apply_ac(CharData *ch, int eq_pos);
int apply_armour(CharData *ch, int eq_pos);
void UpdateObject(ObjData *obj, int use);
void UpdateCharObjects(CharData *ch);
bool IsWearingLight(CharData *ch);

// external functions //
void PerformDropGold(CharData *ch, int amount);
int invalid_unique(CharData *ch, const ObjData *obj);
void do_entergame(DescriptorData *d);
void DoReturn(CharData *ch, char *argument, int cmd, int subcmd);
//extern std::vector<City> Cities;
extern int global_uid;
extern void change_leader(CharData *ch, CharData *vict);
char *find_exdesc(const char *word, const ExtraDescription::shared_ptr &list);
extern void SetSkillCooldown(CharData *ch, ESkill skill, int pulses);

char *fname(const char *namelist) {
	static char holder[30];
	char *point;

	for (point = holder; a_isalpha(*namelist); namelist++, point++)
		*point = *namelist;

	*point = '\0';

	return (holder);
}

bool IsWearingLight(CharData *ch) {
	bool wear_light = false;
	for (int wear_pos = 0; wear_pos < EEquipPos::kNumEquipPos; wear_pos++) {
		if (GET_EQ(ch, wear_pos)
			&& GET_EQ(ch, wear_pos)->get_type() == EObjType::kLightSource
			&& GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2)) {
			wear_light = true;
		}
	}
	return wear_light;
}

void CheckLight(CharData *ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef) {
	if (ch->in_room == kNowhere) {
		return;
	}

	if (IsWearingLight(ch)) {
		if (was_equip == kLightNo) {
			world[ch->in_room]->light = std::max(0, world[ch->in_room]->light + koef);
		}
	} else {
		if (was_equip == kLightYes)
			world[ch->in_room]->light = std::max(0, world[ch->in_room]->light - koef);
	}

	if (AFF_FLAGGED(ch, EAffect::kSingleLight)) {
		if (was_single == kLightNo)
			world[ch->in_room]->light = std::max(0, world[ch->in_room]->light + koef);
	} else {
		if (was_single == kLightYes)
			world[ch->in_room]->light = std::max(0, world[ch->in_room]->light - koef);
	}

	if (AFF_FLAGGED(ch, EAffect::kHolyLight)) {
		if (was_holylight == kLightNo)
			world[ch->in_room]->glight = std::max(0, world[ch->in_room]->glight + koef);
	} else {
		if (was_holylight == kLightYes)
			world[ch->in_room]->glight = std::max(0, world[ch->in_room]->glight - koef);
	}

	if (AFF_FLAGGED(ch, EAffect::kHolyDark)) {
		if (was_holydark == kLightNo) {
			world[ch->in_room]->gdark = std::max(0, world[ch->in_room]->gdark + koef);
		}
	} else {
		if (was_holydark == kLightYes) {
			world[ch->in_room]->gdark = std::max(0, world[ch->in_room]->gdark - koef);
		}
	}
}

void DecreaseFeatTimer(CharData *ch, EFeat feat_id) {
	for (auto *skj = ch->timed_feat; skj; skj = skj->next) {
		if (skj->feat == feat_id) {
			if (skj->time >= 1) {
				--(skj->time);
			} else {
				ExpireTimedFeat(ch, skj);
			}
			return;
		}
	}
};

template <class TalentId>
int GetTalentTimerMod(CharData *ch, TalentId id) {
	// ABYRVALG Тут нужен цикл по способностям _персонажа_, но в настоящий момнет это невозможно из-за особенностей
	// реализации конйтенера способносткй у персонажа.
	int mod{0};
	for (const auto &feat : MUD::Feats()) {
		if (CanUseFeat(ch, feat.GetId())) {
			mod += feat.effects.GetTimerMod(id);
		}
	}
	return mod;
}

void ImposeTimedFeat(CharData *ch, TimedFeat *timed) {
	timed->time = std::max(1, timed->time + GetTalentTimerMod(ch, timed->feat));

	struct TimedFeat *timed_alloc, *skj;
	for (skj = ch->timed_feat; skj; skj = skj->next) {
		if (skj->feat == timed->feat) {
			skj->time = timed->time;
			return;
		}
	}

	CREATE(timed_alloc, 1);

	*timed_alloc = *timed;
	timed_alloc->next = ch->timed_feat;
	ch->timed_feat = timed_alloc;
}

void ExpireTimedFeat(CharData *ch, TimedFeat *timed) {
	if (ch->timed_feat == nullptr) {
		log("SYSERR: timed_feat_from_char(%s) when no timed...", GET_NAME(ch));
		return;
	}

	REMOVE_FROM_LIST(timed, ch->timed_feat);
	free(timed);
}

int IsTimedByFeat(CharData *ch, EFeat feat) {
	struct TimedFeat *hjp;

	for (hjp = ch->timed_feat; hjp; hjp = hjp->next)
		if (hjp->feat == feat)
			return (hjp->time);

	return (0);
}

/**
 * Insert an TimedSkill in a char_data structure
 */
void ImposeTimedSkill(CharData *ch, struct TimedSkill *timed) {
	timed->time = std::max(1, timed->time + GetTalentTimerMod(ch, timed->skill));

	struct TimedSkill *timed_alloc, *skj;
	for (skj = ch->timed; skj; skj = skj->next) {
		if (skj->skill == timed->skill) {
			skj->time = timed->time;
			return;
		}
	}

	CREATE(timed_alloc, 1);

	*timed_alloc = *timed;
	timed_alloc->next = ch->timed;
	ch->timed = timed_alloc;
}

void ExpireTimedSkill(CharData *ch, struct TimedSkill *timed) {
	if (ch->timed == nullptr) {
		log("SYSERR: ExpireTimedSkill(%s) when no timed...", GET_NAME(ch));
		// core_dump();
		return;
	}

	REMOVE_FROM_LIST(timed, ch->timed);
	free(timed);
}

int IsTimedBySkill(CharData *ch, ESkill id) {
	struct TimedSkill *hjp;

	for (hjp = ch->timed; hjp; hjp = hjp->next)
		if (hjp->skill == id)
			return (hjp->time);

	return (0);
}

// move a player out of a room
void RemoveCharFromRoom(CharData *ch) {
	if (ch == nullptr || ch->in_room == kNowhere) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL character or kNowhere in %s, RemoveCharFromRoom", __FILE__);
		return;
	}

	if (ch->GetEnemy() != nullptr)
		stop_fighting(ch, true);

	if (!ch->IsNpc())
		ch->set_from_room(ch->in_room);

	CheckLight(ch, kLightNo, kLightNo, kLightNo, kLightNo, -1);

	auto &people = world[ch->in_room]->people;
	people.erase(std::find(people.begin(), people.end(), ch));

	ch->in_room = kNowhere;
	ch->track_dirs = 0;
}

void PlaceCharToRoom(CharData *ch, RoomRnum room) {
	if (ch == nullptr || room < kNowhere + 1 || room > top_of_world) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p", room, top_of_world, ch);
		return;
	}

	if (!ch->IsNpc() && !Clan::MayEnter(ch, room, kHousePortal)) {
		room = ch->get_from_room();
	}

	if (!ch->IsNpc() 
			&& NORENTABLE(ch) 
			&& ROOM_FLAGGED(room, ERoomFlag::kArena) 
			&& !IS_IMMORTAL(ch)) {
		SendMsgToChar("Вы не можете попасть на арену в состоянии боевых действий!\r\n", ch);
		room = ch->get_from_room();
	}
	world[room]->people.push_front(ch);

	ch->in_room = room;
	CheckLight(ch, kLightNo, kLightNo, kLightNo, kLightNo, 1);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILHIDE);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILSNEAK);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILCAMOUFLAGE);
	if (ch->IsFlagged(EPrf::kCoderinfo)) {
		sprintf(buf,
				"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d %sЛед=%s%d "
				"%sТьма=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d%s.\r\n",
				kColorNrm, kColorBoldBlk, room,
				kColorRed, kColorBoldRed, world[room]->light,
				kColorGrn, kColorBoldGrn, world[room]->glight,
				kColorYel, kColorBoldYel, world[room]->fires,
				kColorYel, kColorBoldYel, world[room]->ices,
				kColorBlu, kColorBoldBlu, world[room]->gdark,
				kColorMag, kColorBoldCyn, weather_info.sky,
				kColorWht, kColorBoldBlk, weather_info.sunlight,
				kColorYel, kColorBoldYel, weather_info.moon_day, kColorNrm);
		SendMsgToChar(buf, ch);
	}
	// Stop fighting now, if we left.
	if (ch->GetEnemy() && ch->in_room != ch->GetEnemy()->in_room) {
		stop_fighting(ch->GetEnemy(), false);
		stop_fighting(ch, true);
	}

	if (!ch->IsNpc() && !GET_INVIS_LEV(ch)) {
		zone_table[world[room]->zone_rn].used = true;
		zone_table[world[room]->zone_rn].activity++;
	} else {
		//sventovit: здесь обрабатываются только неписи, чтобы игрок успел увидеть комнату
		//как сделать красивей я не придумал, т.к. look_at_room вызывается в act.movement а не тут
		room_spells::ProcessRoomAffectsOnEntry(ch, ch->in_room);
	}
	cities::CheckCityVisit(ch, room);
}

bool IsLabelledObjsStackable(ObjData *obj_one, ObjData *obj_two) {
	// без меток стокаются
	if (!obj_one->get_custom_label() && !obj_two->get_custom_label())
		return true;

	if (obj_one->get_custom_label() && obj_two->get_custom_label()) {
		// с разными типами меток не стокаются
		if (!obj_one->get_custom_label()->clan_abbrev != !obj_two->get_custom_label()->clan_abbrev) {
			return false;
		} else {
			// обе метки клановые один клан, текст совпадает -- стокается
			if (obj_one->get_custom_label()->clan_abbrev && obj_two->get_custom_label()->clan_abbrev
				&& !strcmp(obj_one->get_custom_label()->clan_abbrev, obj_two->get_custom_label()->clan_abbrev)
				&& obj_one->get_custom_label()->text_label && obj_two->get_custom_label()->text_label
				&& !strcmp(obj_one->get_custom_label()->text_label, obj_two->get_custom_label()->text_label)) {
				return true;
			}

			// обе метки личные, один автор, текст совпадает -- стокается
			if (obj_one->get_custom_label()->author == obj_two->get_custom_label()->author
				&& obj_one->get_custom_label()->text_label && obj_two->get_custom_label()->text_label
				&& !strcmp(obj_one->get_custom_label()->text_label, obj_two->get_custom_label()->text_label)) {
				return true;
			}
		}
	}

	return false;
}

bool IsObjsStackable(ObjData *obj_one, ObjData *obj_two) {
	if (GET_OBJ_VNUM(obj_one) != GET_OBJ_VNUM(obj_two)
		|| strcmp(obj_one->get_short_description().c_str(), obj_two->get_short_description().c_str())
		|| (obj_one->get_type() == EObjType::kLiquidContainer
			&& GET_OBJ_VAL(obj_one, 2) != GET_OBJ_VAL(obj_two, 2))
		|| (obj_one->get_type() == EObjType::kContainer
			&& (obj_one->get_contains() || obj_two->get_contains()))
		|| GET_OBJ_VNUM(obj_two) == -1
		|| (obj_one->get_type() == EObjType::kBook
			&& GET_OBJ_VAL(obj_one, 1) != GET_OBJ_VAL(obj_two, 1))
		|| !IsLabelledObjsStackable(obj_one, obj_two)) {
		return false;
	}

	return true;
}

namespace {

/**
 * Перемещаем стокающиеся предметы вверх контейнера и сверху кладем obj.
 */
void ArrangeObjs(ObjData *obj, ObjData **list_start) {
	// AL: пофиксил Ж)
	// Krodo: пофиксили третий раз, не сортируем у мобов в инве Ж)

	// begin - первый предмет в исходном списке
	// end - последний предмет в перемещаемом интервале
	// before - последний предмет перед началом интервала
	ObjData *p, *begin, *end, *before;

	obj->set_next_content(begin = *list_start);
	*list_start = obj;

	// похожий предмет уже первый в списке или список пустой
	if (!begin || IsObjsStackable(begin, obj)) {
		return;
	}

	before = p = begin;

	while (p && !IsObjsStackable(p, obj)) {
		before = p;
		p = p->get_next_content();
	}

	// нет похожих предметов
	if (!p) {
		return;
	}

	end = p;

	while (p && IsObjsStackable(p, obj)) {
		end = p;
		p = p->get_next_content();
	}

	end->set_next_content(begin);
	obj->set_next_content(before->get_next_content());
	before->set_next_content(p); // будет 0 если после перемещаемых ничего не лежало
}

} // no-name namespace

// * Инициализация уида для нового объекта.
void InitUid(ObjData *object) {
	if (GET_OBJ_VNUM(object) > 0 && // Объект не виртуальный
		object->get_unique_id() == 0)   // У объекта точно нет уида
	{
		global_uid++; // Увеличиваем глобальный счетчик уидов
		global_uid = global_uid == 0 ? 1 : global_uid; // Если произошло переполнение инта
		object->set_unique_id(global_uid); // Назначаем уид
	}
}

void PlaceObjToInventory(ObjData *object, CharData *ch) {
	unsigned int tuid;
	int inworld;

	if (!world_objects.get_by_raw_ptr(object)) {
		std::stringstream ss;
		ss << "SYSERR: Object at address 0x" << object
		   << " is not in the world but we have attempt to put it into character '" << ch->get_name()
		   << "'. Object won't be placed into character's inventory.";
		mudlog(ss.str().c_str(), NRM, kLvlImplementator, SYSLOG, true);
		debug::backtrace(runtime_config.logs(ERRLOG).handle());

		return;
	}

	int may_carry = true;
	if (object && ch) {
		if (invalid_anti_class(ch, object) || NamedStuff::check_named(ch, object, false))
			may_carry = false;
		if (!may_carry) {
			act("Вас обожгло при попытке взять $o3.", false, ch, object, nullptr, kToChar);
			act("$n попытал$u взять $o3 - и чудом не сгорел$g.", false, ch, object, nullptr, kToRoom);
			PlaceObjToRoom(object, ch->in_room);
			return;
		}
		if (!ch->IsNpc()
			|| (ch->has_master()
				&& !ch->get_master()->IsNpc())) {
			if (object && object->get_unique_id() != 0 && object->get_timer() > 0) {
				tuid = object->get_unique_id();
				inworld = 1;
				// Объект готов для проверки. Ищем в мире такой же.
				world_objects.foreach_with_vnum(GET_OBJ_VNUM(object), [&inworld, tuid, object](const ObjData::shared_ptr &i) {
					if (i->get_unique_id() == tuid // UID совпадает
						&& i->get_timer() > 0  // Целенький
						&& object != i.get()) // Не оно же
					{
						inworld++;
					}
				});

				if (inworld > 1) // У объекта есть как минимум одна копия
				{
					sprintf(buf,
							"Copy detected and prepared to extract! Object %s (UID=%ld, VNUM=%d), holder %s. In world %d.",
							object->get_PName(ECase::kNom).c_str(),
							object->get_unique_id(),
							GET_OBJ_VNUM(object),
							GET_NAME(ch),
							inworld);
					mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
					act("$o0 замигал$Q и вы увидели медленно проступившие руны 'DUPE'.", false, ch, object, nullptr, kToChar);
					object->set_timer(0);
					object->set_extra_flag(EObjFlag::kNosell); // Ибо нефиг
				}
			} // Назначаем UID
			else {
				InitUid(object);
				log("%s obj_to_char %s #%d|%ld",
					GET_NAME(ch),
					object->get_PName(ECase::kNom).c_str(),
					GET_OBJ_VNUM(object),
					object->get_unique_id());
			}
		}

		if (!ch->IsNpc()
			|| (ch->has_master()
				&& !ch->get_master()->IsNpc())) {
			object->set_extra_flag(EObjFlag::kTicktimer);    // start timer unconditionally when character picks item up.
			ArrangeObjs(object, &ch->carrying);
		} else {
			// Вот эта муть, чтобы временно обойти завязку магазинов на порядке предметов в инве моба // Krodo
			object->set_next_content(ch->carrying);
			ch->carrying = object;
		}

		object->set_carried_by(ch);
		object->set_in_room(kNowhere);
		IS_CARRYING_W(ch) += object->get_weight();
		IS_CARRYING_N(ch)++;

		if (!ch->IsNpc()) {
			log("obj_to_char: %s -> %d", ch->get_name().c_str(), GET_OBJ_VNUM(object));
		}
		// set flag for crash-save system, but not on mobs!
		if (!ch->IsNpc()) {
			ch->SetFlag(EPlrFlag::kCrashSave);
		}
	} else
		log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}

// take an object from a char
void RemoveObjFromChar(ObjData *object) {
	if (!object || !object->get_carried_by()) {
		log("SYSERR: NULL object or owner passed to obj_from_char");
		return;
	}
	object->remove_me_from_contains_list(object->get_carried_by()->carrying);

	// set flag for crash-save system, but not on mobs!
	if (!object->get_carried_by()->IsNpc()) {
		object->get_carried_by()->SetFlag(EPlrFlag::kCrashSave);
		log("obj_from_char: %s -> %d", object->get_carried_by()->get_name().c_str(), GET_OBJ_VNUM(object));
	}

	IS_CARRYING_W(object->get_carried_by()) -= object->get_weight();
	IS_CARRYING_N(object->get_carried_by())--;
	object->set_carried_by(nullptr);
	object->set_next_content(nullptr);
}

bool HaveIncompatibleAlign(CharData *ch, ObjData *obj) {
	if (ch->IsNpc() || IS_IMMORTAL(ch)) {
		return false;
	}
	if (obj->has_anti_flag(EAntiFlag::kMono) && GET_RELIGION(ch) == kReligionMono) {
		return true;
	}
	if (obj->has_anti_flag(EAntiFlag::kPoly) && GET_RELIGION(ch) == kReligionPoly) {
		return true;
	}
	return false;
}

void DisplayWearMsg(CharData *ch, ObjData *obj, int position) {
	const char *wear_messages[][2] =
		{
			{"$n засветил$g $o3 и взял$g во вторую руку.",
			 "Вы зажгли $o3 и взяли во вторую руку."},

			{"$n0 надел$g $o3 на правый указательный палец.",
			 "Вы надели $o3 на правый указательный палец."},

			{"$n0 надел$g $o3 на левый указательный палец.",
			 "Вы надели $o3 на левый указательный палец."},

			{"$n0 надел$g $o3 вокруг шеи.",
			 "Вы надели $o3 вокруг шеи."},

			{"$n0 надел$g $o3 на грудь.",
			 "Вы надели $o3 на грудь."},

			{"$n0 надел$g $o3 на туловище.",
			 "Вы надели $o3 на туловище.",},

			{"$n0 водрузил$g $o3 на голову.",
			 "Вы водрузили $o3 себе на голову."},

			{"$n0 надел$g $o3 на ноги.",
			 "Вы надели $o3 на ноги."},

			{"$n0 обул$g $o3.",
			 "Вы обули $o3."},

			{"$n0 надел$g $o3 на кисти.",
			 "Вы надели $o3 на кисти."},

			{"$n0 надел$g $o3 на руки.",
			 "Вы надели $o3 на руки."},

			{"$n0 начал$g использовать $o3 как щит.",
			 "Вы начали использовать $o3 как щит."},

			{"$n0 облачил$u в $o3.",
			 "Вы облачились в $o3."},

			{"$n0 надел$g $o3 вокруг пояса.",
			 "Вы надели $o3 вокруг пояса."},

			{"$n0 надел$g $o3 вокруг правого запястья.",
			 "Вы надели $o3 вокруг правого запястья."},

			{"$n0 надел$g $o3 вокруг левого запястья.",
			 "Вы надели $o3 вокруг левого запястья."},

			{"$n0 взял$g в правую руку $o3.",
			 "Вы вооружились $o4."},

			{"$n0 взял$g $o3 в левую руку.",
			 "Вы взяли $o3 в левую руку."},

			{"$n0 взял$g $o3 в обе руки.",
			 "Вы взяли $o3 в обе руки."},

			{"$n0 начал$g использовать $o3 как колчан.",
			 "Вы начали использовать $o3 как колчан."}
		};

	act(wear_messages[position][1], false, ch, obj, nullptr, kToChar);
	act(wear_messages[position][0],
		ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed) ? false : true,
		ch, obj, nullptr, kToRoom | kToArenaListen);
}

// \todo Это часть системы сетов, туда и надо вынести.
int GetFlagDataByCharClass(const CharData *ch) {
	if (ch == nullptr) {
		return 0;
	}

	return flag_data_by_num(ch->IsNpc() ? kNumPlayerClasses * kNumKins : to_underlying(ch->GetClass())
		+ kNumPlayerClasses * GET_KIN(ch));
}

unsigned int ActivateStuff(CharData *ch, ObjData *obj, id_to_set_info_map::const_iterator it,
						   int pos, const CharEquipFlags& equip_flags, unsigned int set_obj_qty) {
	const bool no_cast = equip_flags.test(CharEquipFlag::no_cast);
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);
	std::string::size_type delim;

	if (pos < EEquipPos::kNumEquipPos) {
		set_info::const_iterator set_obj_info;

		if (GET_EQ(ch, pos) && GET_EQ(ch, pos)->has_flag(EObjFlag::KSetItem) &&
			(set_obj_info = it->second.find(GET_OBJ_VNUM(GET_EQ(ch, pos)))) != it->second.end()) {
			unsigned int oqty = ActivateStuff(ch, obj, it, pos + 1,
											  (show_msg ? CharEquipFlag::show_msg : CharEquipFlags())
												  | (no_cast ? CharEquipFlag::no_cast : CharEquipFlags()),
											  set_obj_qty + 1);
			qty_to_camap_map::const_iterator qty_info = set_obj_info->second.upper_bound(oqty);
			qty_to_camap_map::const_iterator old_qty_info = GET_EQ(ch, pos) == obj ?
															set_obj_info->second.begin() :
															set_obj_info->second.upper_bound(oqty - 1);

			while (qty_info != old_qty_info) {
				class_to_act_map::const_iterator class_info;

				qty_info--;
				unique_bit_flag_data item;
				const auto flags = GetFlagDataByCharClass(ch);
				item.set(flags);
				if ((class_info = qty_info->second.find(item)) != qty_info->second.end()) {
					if (GET_EQ(ch, pos) != obj) {
						for (int i = 0; i < kMaxObjAffect; i++) {
							affect_modify(ch,
										  GET_EQ(ch, pos)->get_affected(i).location,
										  GET_EQ(ch, pos)->get_affected(i).modifier,
										  static_cast<EAffect>(0),
										  false);
						}

						if (ch->in_room != kNowhere) {
							for (const auto &i : weapon_affect) {
								if (i.aff_bitvector == 0
									|| !GET_EQ(ch, pos)->GetEWeaponAffect(i.aff_pos)) {
									continue;
								}
								affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(i.aff_bitvector), false);
							}
						}
					}

					std::string act_msg = GET_EQ(ch, pos)->activate_obj(class_info->second);
					delim = act_msg.find('\n');

					if (show_msg) {
						act(act_msg.substr(0, delim).c_str(), false, ch, GET_EQ(ch, pos), nullptr, kToChar);
						act(act_msg.erase(0, delim + 1).c_str(),
							ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed) ? false : true,
							ch, GET_EQ(ch, pos), nullptr, kToRoom);
					}

					for (int i = 0; i < kMaxObjAffect; i++) {
						affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location,
									  GET_EQ(ch, pos)->get_affected(i).modifier, static_cast<EAffect>(0), true);
					}

					if (ch->in_room != kNowhere) {
						for (const auto &i : weapon_affect) {
							if (i.aff_spell == ESpell::kUndefined || !GET_EQ(ch, pos)->GetEWeaponAffect(i.aff_pos)) {
								continue;
							}
							if (!no_cast) {
								if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic)) {
									act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
										false, ch, GET_EQ(ch, pos), nullptr, kToRoom);
									act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
										false, ch, GET_EQ(ch, pos), nullptr, kToChar);
								} else {
									CastAffect(GetRealLevel(ch), ch, ch, i.aff_spell);
								}
							}
						}
					}

					return oqty;
				}
			}

			if (GET_EQ(ch, pos) == obj) {
				for (int i = 0; i < kMaxObjAffect; i++) {
					affect_modify(ch,
								  obj->get_affected(i).location,
								  obj->get_affected(i).modifier,
								  static_cast<EAffect>(0),
								  true);
				}

				if (ch->in_room != kNowhere) {
					for (const auto &i : weapon_affect) {
						if (i.aff_spell == ESpell::kUndefined || !obj->GetEWeaponAffect(i.aff_pos)) {
							continue;
						}
						if (!no_cast) {
							if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic)) {
								act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
									false, ch, obj, nullptr, kToRoom);
								act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
									false, ch, obj, nullptr, kToChar);
							} else {
								CastAffect(GetRealLevel(ch), ch, ch, i.aff_spell);
							}
						}
					}
				}
			}

			return oqty;
		} else
			return ActivateStuff(ch, obj, it, pos + 1,
								 (show_msg ? CharEquipFlag::show_msg : CharEquipFlags())
									 | (no_cast ? CharEquipFlag::no_cast : CharEquipFlags()),
								 set_obj_qty);
	} else
		return set_obj_qty;
}

bool CheckArmorType(CharData *ch, ObjData *obj) {
	if (obj->get_type() == EObjType::kLightArmor && !CanUseFeat(ch, EFeat::kWearingLightArmor)) {
		act("Для использования $o1 требуется способность 'легкие доспехи'.",
			false, ch, obj, nullptr, kToChar);
		return false;
	}

	if (obj->get_type() == EObjType::kMediumArmor && !CanUseFeat(ch, EFeat::kWearingMediumArmor)) {
		act("Для использования $o1 требуется способность 'средние доспехи'.",
			false, ch, obj, nullptr, kToChar);
		return false;
	}

	if (obj->get_type() == EObjType::kHeavyArmor && !CanUseFeat(ch, EFeat::kWearingHeavyArmor)) {
		act("Для использования $o1 требуется способность 'тяжелые доспехи'.",
			false, ch, obj, nullptr, kToChar);
		return false;
	}

	return true;
}

void EquipObj(CharData *ch, ObjData *obj, int pos, const CharEquipFlags& equip_flags) {
	int was_lgt = AFF_FLAGGED(ch, EAffect::kSingleLight) ? kLightYes : kLightNo,
		was_hlgt = AFF_FLAGGED(ch, EAffect::kHolyLight) ? kLightYes : kLightNo,
		was_hdrk = AFF_FLAGGED(ch, EAffect::kHolyDark) ? kLightYes : kLightNo,
		was_lamp = false;

	const bool no_cast = equip_flags.test(CharEquipFlag::no_cast);
	const bool skip_total = equip_flags.test(CharEquipFlag::skip_total);
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);

	if (pos < 0 || pos >= EEquipPos::kNumEquipPos) {
		log("SYSERR: equip_char(%s,%d) in unknown pos...", GET_NAME(ch), pos);
		return;
	}

	if (GET_EQ(ch, pos)) {
		log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch), obj->get_short_description().c_str());
		return;
	}
	//if (obj->carried_by) {
	//	log("SYSERR: EQUIP: %s - Obj is carried_by when equip.", OBJN(obj, ch, ECase::kNom));
	//	return;
	//}
	if (obj->get_in_room() != kNowhere) {
		log("SYSERR: EQUIP: %s - Obj is in_room when equip.", OBJN(obj, ch, ECase::kNom));
		return;
	}

	if (invalid_anti_class(ch, obj) || invalid_unique(ch, obj)) {
		act("Вас обожгло при попытке использовать $o3.", false, ch, obj, nullptr, kToChar);
		act("$n попытал$u использовать $o3 - и чудом не обгорел$g.", false, ch, obj, nullptr, kToRoom);
		if (obj->get_carried_by()) {
			RemoveObjFromChar(obj);
		}
		PlaceObjToRoom(obj, ch->in_room);
		CheckObjDecay(obj);
		return;
	} else if ((!ch->IsNpc() || IS_CHARMICE(ch)) && obj->has_flag(EObjFlag::kNamed)
		&& NamedStuff::check_named(ch, obj, true)) {
		if (!NamedStuff::wear_msg(ch, obj))
			SendMsgToChar("Просьба не трогать! Частная собственность!\r\n", ch);
		if (!obj->get_carried_by()) {
			PlaceObjToInventory(obj, ch);
		}
		return;
	}

	if ((!ch->IsNpc() && HaveIncompatibleAlign(ch, obj)) || invalid_no_class(ch, obj) ||
		(AFF_FLAGGED(ch, EAffect::kCharmed) && (obj->has_flag(EObjFlag::kSharpen) ||
		obj->has_flag(EObjFlag::kArmored)))) {
		act("$o0 явно не предназначен$A для вас.", false, ch, obj, nullptr, kToChar);
		act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", false, ch, obj, nullptr, kToRoom);
		if (!obj->get_carried_by()) {
			PlaceObjToInventory(obj, ch);
		}
		return;
	}

	if (!ch->IsNpc() || IS_CHARMICE(ch)) {
		CharData *master = IS_CHARMICE(ch) && ch->has_master() ? ch->get_master() : ch;
		if ((obj->get_auto_mort_req() >= 0) && (obj->get_auto_mort_req() > GetRealRemort(master))
			&& !IS_IMMORTAL(master)) {
			SendMsgToChar(master, "Для использования %s требуется %d %s.\r\n",
						  obj->get_PName(ECase::kGen).c_str(),
						  obj->get_auto_mort_req(),
						  GetDeclensionInNumber(obj->get_auto_mort_req(), EWhat::kRemort));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.", false, ch, obj, nullptr, kToRoom);
			if (!obj->get_carried_by()) {
				PlaceObjToInventory(obj, ch);
			}
			return;
		} else if ((obj->get_auto_mort_req() < -1) && (abs(obj->get_auto_mort_req()) < GetRealRemort(master))
			&& !IS_IMMORTAL(master)) {
			SendMsgToChar(master, "Максимально количество перевоплощений для использования %s равно %d.\r\n",
						  obj->get_PName(ECase::kGen).c_str(),
						  abs(obj->get_auto_mort_req()));
			act("$n попытал$u использовать $o3, но у н$s ничего не получилось.",
				false, ch, obj, nullptr, kToRoom);
			if (!obj->get_carried_by()) {
				PlaceObjToInventory(obj, ch);
			}
			return;
		}

	}

	if (obj->get_carried_by()) {
		RemoveObjFromChar(obj);
	}

	was_lamp = IsWearingLight(ch);
	GET_EQ(ch, pos) = obj;
	obj->set_worn_by(ch);
	obj->set_worn_on(pos);
	obj->set_next_content(nullptr);
	ch->check_aggressive = true;

	if (show_msg) {
		DisplayWearMsg(ch, obj, pos);
		if (obj->has_flag(EObjFlag::kNamed)) {
			NamedStuff::wear_msg(ch, obj);
		}
	}

	if (ch->in_room == kNowhere) {
		log("SYSERR: ch->in_room = kNowhere when equipping char %s.", GET_NAME(ch));
	}

	auto it = ObjData::set_table.begin();
	if (obj->has_flag(EObjFlag::KSetItem)) {
		for (; it != ObjData::set_table.end(); it++) {
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				ActivateStuff(ch, obj, it, 0,
							  (show_msg ? CharEquipFlag::show_msg : CharEquipFlags())
								  | (no_cast ? CharEquipFlag::no_cast : CharEquipFlags()), 0);
				break;
			}
		}
	}

	if (!obj->has_flag(EObjFlag::KSetItem) || it == ObjData::set_table.end()) {
		for (int j = 0; j < kMaxObjAffect; j++) {
			affect_modify(ch,
						  obj->get_affected(j).location,
						  obj->get_affected(j).modifier,
						  static_cast<EAffect>(0),
						  true);
		}

		if (ch->in_room != kNowhere) {
			for (const auto &j : weapon_affect) {
				if (j.aff_spell == ESpell::kUndefined || !obj->GetEWeaponAffect(j.aff_pos)) {
					continue;
				}

				if (!no_cast) {
					if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic)) {
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							false, ch, obj, nullptr, kToRoom);
						act("Магия $o1 потерпела неудачу и развеялась по воздуху.",
							false, ch, obj, nullptr, kToChar);
					} else {
						CastAffect(GetRealLevel(ch), ch, ch, j.aff_spell);
					}
				}
			}
		}
	}

	if (!skip_total) {
		if (obj_sets::is_set_item(obj)) {
			ch->obj_bonus().update(ch);
		}
		affect_total(ch);
		CheckLight(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
	}

	// Раз показываем сообщение, значит, предмет надевает сам персонаж
	// А вообще эта порнография из-за того, что одна функция используется с кучей флагов в разных вариантах
	if (show_msg && ch->GetEnemy() && (obj->get_type() == EObjType::kWeapon || pos == kShield)) {
		SetSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
	}
}

unsigned int DeactivateStuff(CharData *ch, ObjData *obj, id_to_set_info_map::const_iterator it,
							 int pos, const CharEquipFlags& equip_flags, unsigned int set_obj_qty) {
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);
	std::string::size_type delim;

	if (pos < EEquipPos::kNumEquipPos) {
		set_info::const_iterator set_obj_info;

		if (GET_EQ(ch, pos)
			&& GET_EQ(ch, pos)->has_flag(EObjFlag::KSetItem)
			&& (set_obj_info = it->second.find(GET_OBJ_VNUM(GET_EQ(ch, pos)))) != it->second.end()) {
			unsigned int oqty =
				DeactivateStuff(ch, obj, it, pos + 1, (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()),
								set_obj_qty + 1);
			qty_to_camap_map::const_iterator old_qty_info = set_obj_info->second.upper_bound(oqty);
			qty_to_camap_map::const_iterator qty_info = GET_EQ(ch, pos) == obj ?
														set_obj_info->second.begin() :
														set_obj_info->second.upper_bound(oqty - 1);

			while (old_qty_info != qty_info) {
				old_qty_info--;
				unique_bit_flag_data flags1;
				flags1.set(GetFlagDataByCharClass(ch));
				class_to_act_map::const_iterator class_info = old_qty_info->second.find(flags1);
				if (class_info != old_qty_info->second.end()) {
					while (qty_info != set_obj_info->second.begin()) {
						qty_info--;
						unique_bit_flag_data flags2;
						flags2.set(GetFlagDataByCharClass(ch));
						class_to_act_map::const_iterator class_info2 = qty_info->second.find(flags2);
						if (class_info2 != qty_info->second.end()) {
							for (int i = 0; i < kMaxObjAffect; i++) {
								affect_modify(ch,
											  GET_EQ(ch, pos)->get_affected(i).location,
											  GET_EQ(ch, pos)->get_affected(i).modifier,
											  static_cast<EAffect>(0),
											  false);
							}

							if (ch->in_room != kNowhere) {
								for (const auto &i : weapon_affect) {
									if (i.aff_bitvector == 0
										|| !GET_EQ(ch, pos)->GetEWeaponAffect(i.aff_pos)) {
										continue;
									}
									affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(i.aff_bitvector), false);
								}
							}

							std::string act_msg = GET_EQ(ch, pos)->activate_obj(class_info2->second);
							delim = act_msg.find('\n');

							if (show_msg) {
								act(act_msg.substr(0, delim).c_str(), false, ch, GET_EQ(ch, pos), nullptr, kToChar);
								act(act_msg.erase(0, delim + 1).c_str(),
									ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed) ? false : true,
									ch, GET_EQ(ch, pos), nullptr, kToRoom);
							}

							for (int i = 0; i < kMaxObjAffect; i++) {
								affect_modify(ch,
											  GET_EQ(ch, pos)->get_affected(i).location,
											  GET_EQ(ch, pos)->get_affected(i).modifier,
											  static_cast<EAffect>(0),
											  true);
							}

							if (ch->in_room != kNowhere) {
								for (const auto &i : weapon_affect) {
									if (i.aff_bitvector == 0
										|| !GET_EQ(ch, pos)->GetEWeaponAffect(i.aff_pos)) {
										continue;
									}
									affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(i.aff_bitvector), true);
								}
							}

							return oqty;
						}
					}

					for (int i = 0; i < kMaxObjAffect; i++) {
						affect_modify(ch, GET_EQ(ch, pos)->get_affected(i).location,
									  GET_EQ(ch, pos)->get_affected(i).modifier, static_cast<EAffect>(0), false);
					}

					if (ch->in_room != kNowhere) {
						for (const auto &i : weapon_affect) {
							if (i.aff_bitvector == 0
								|| !GET_EQ(ch, pos)->GetEWeaponAffect(i.aff_pos)) {
								continue;
							}
							affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(i.aff_bitvector), false);
						}
					}

					std::string deact_msg = GET_EQ(ch, pos)->deactivate_obj(class_info->second);
					delim = deact_msg.find('\n');

					if (show_msg) {
						act(deact_msg.substr(0, delim).c_str(), false, ch, GET_EQ(ch, pos), nullptr, kToChar);
						act(deact_msg.erase(0, delim + 1).c_str(),
							ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed) ? false : true,
							ch, GET_EQ(ch, pos), nullptr, kToRoom);
					}

					if (GET_EQ(ch, pos) != obj) {
						for (int i = 0; i < kMaxObjAffect; i++) {
							affect_modify(ch,
										  GET_EQ(ch, pos)->get_affected(i).location,
										  GET_EQ(ch, pos)->get_affected(i).modifier,
										  static_cast<EAffect>(0),
										  true);
						}

						if (ch->in_room != kNowhere) {
							for (const auto &i : weapon_affect) {
								if (i.aff_bitvector == 0 ||
									!GET_EQ(ch, pos)->GetEWeaponAffect(i.aff_pos)) {
									continue;
								}
								affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(i.aff_bitvector), true);
							}
						}
					}

					return oqty;
				}
			}

			if (GET_EQ(ch, pos) == obj) {
				for (int i = 0; i < kMaxObjAffect; i++) {
					affect_modify(ch,
								  obj->get_affected(i).location,
								  obj->get_affected(i).modifier,
								  static_cast<EAffect>(0),
								  false);
				}

				if (ch->in_room != kNowhere) {
					for (const auto &i : weapon_affect) {
						if (i.aff_bitvector == 0
							|| !obj->GetEWeaponAffect(i.aff_pos)) {
							continue;
						}
						affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(i.aff_bitvector), false);
					}
				}

				obj->deactivate_obj(activation());
			}

			return oqty;
		} else {
			return DeactivateStuff(ch,
								   obj,
								   it,
								   pos + 1,
								   (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()),
								   set_obj_qty);
		}
	} else {
		return set_obj_qty;
	}
}

ObjData *UnequipChar(CharData *ch, int pos, const CharEquipFlags& equip_flags) {
	int was_lgt = AFF_FLAGGED(ch, EAffect::kSingleLight) ? kLightYes : kLightNo,
		was_hlgt = AFF_FLAGGED(ch, EAffect::kHolyLight) ? kLightYes : kLightNo,
		was_hdrk = AFF_FLAGGED(ch, EAffect::kHolyDark) ? kLightYes : kLightNo, was_lamp = false;

	const bool skip_total = equip_flags.test(CharEquipFlag::skip_total);
	const bool show_msg = equip_flags.test(CharEquipFlag::show_msg);

	if (pos < 0 || pos >= EEquipPos::kNumEquipPos) {
		log("SYSERR: UnequipChar(%s,%d) - unused pos...", GET_NAME(ch), pos);
		return nullptr;
	}

	ObjData *obj = GET_EQ(ch, pos);
	if (nullptr == obj) {
		log("SYSERR: UnequipChar(%s,%d) - no equip...", GET_NAME(ch), pos);
		return nullptr;
	}

	was_lamp = IsWearingLight(ch);

	if (ch->in_room == kNowhere)
		log("SYSERR: ch->in_room = kNowhere when unequipping char %s.", GET_NAME(ch));

	auto it = ObjData::set_table.begin();
	if (obj->has_flag(EObjFlag::KSetItem))
		for (; it != ObjData::set_table.end(); it++)
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				DeactivateStuff(ch, obj, it, 0, (show_msg ? CharEquipFlag::show_msg : CharEquipFlags()), 0);
				break;
			}

	if (!obj->has_flag(EObjFlag::KSetItem) || it == ObjData::set_table.end()) {
		for (int j = 0; j < kMaxObjAffect; j++) {
			affect_modify(ch,
						  obj->get_affected(j).location,
						  obj->get_affected(j).modifier,
						  static_cast<EAffect>(0),
						  false);
		}

		if (ch->in_room != kNowhere) {
			for (const auto &j : weapon_affect) {
				if (j.aff_bitvector == 0 || !obj->GetEWeaponAffect(j.aff_pos)) {
					continue;
				}
				if (ch->IsNpc()
					&& AFF_FLAGGED(&mob_proto[GET_MOB_RNUM(ch)], static_cast<EAffect>(j.aff_bitvector))) {
					continue;
				}
				affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(j.aff_bitvector), false);
			}
		}

		if ((obj->has_flag(EObjFlag::KSetItem)) && (SetSystem::is_big_set(obj)))
			obj->deactivate_obj(activation());
	}

	GET_EQ(ch, pos) = nullptr;
	obj->set_worn_by(nullptr);
	obj->set_worn_on(kNowhere);
	obj->set_next_content(nullptr);

	if (!skip_total) {
		if (obj_sets::is_set_item(obj)) {
			if (obj->get_activator().first) {
				obj_sets::print_off_msg(ch, obj);
			}
			ch->obj_bonus().update(ch);
		}
		obj->set_activator(false, 0);
		obj->remove_set_bonus();

		affect_total(ch);
		CheckLight(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
	}

	return (obj);
}

int get_number(char **name) {
	int i, res;
	char *ppos;

	if ((ppos = strchr(*name, '.')) != nullptr) {
		for (i = 0; *name + i != ppos; i++) {
			if (!a_isdigit(*(*name + i))) {
				return 1;
			}
		}
		res = atoi(*name);
		memmove(*name, ppos + 1, strlen(ppos));
		return res;
	}
	return 1;
}

int get_number(std::string &name) {
	std::string::size_type pos = name.find('.');

	if (pos != std::string::npos) {
		for (std::string::size_type i = 0; i != pos; i++)
			if (!a_isdigit(name[i]))
				return (1);
		int res = atoi(name.substr(0, pos).c_str());
		name.erase(0, pos + 1);
		return (res);
	}
	return (1);
}

/**
 * Search an object in list by rnum.
 * @param rnum - object rnum.
 * @param list - given list.
 * @return - ptr to found obj or nullptr.
 */
ObjData *GetObjByRnumInContent(ObjRnum rnum, ObjData *list) {
	ObjData *i;
	for (i = list; i; i = i->get_next_content()) {
		if (i->get_rnum() == rnum) {
			return (i);
		}
	}

	return nullptr;
}

/**
 * Search an object in list by vnum.
 * @param vnum - object vnum.
 * @param list - given list.
 * @return - ptr to found obj or nullptr.
 */
ObjData *GetObjByVnumInContent(ObjVnum vnum, ObjData *list) {
	ObjData *i;
	for (i = list; i; i = i->get_next_content()) {
		if (i->get_vnum() == vnum) {
			return (i);
		}
	}

	return nullptr;
}

/**
 * Search the entire world for an object by virtual number.
 * @param vnum - object vnum.
 * @return - ptr to found obj or nullptr.
 */
ObjData *SearchObjByRnum(ObjRnum rnum) {
	const auto result = world_objects.find_first_by_rnum(rnum);
	return result.get();
}

// search a room for a char, and return a pointer if found..  //
CharData *SearchCharInRoomByName(const char *name, RoomRnum room) {
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, name);
	const int number = get_number(&tmp);
	if (0 == number) {
		return nullptr;
	}

	int j = 0;
	for (const auto i : world[room]->people) {
		if (isname(tmp, i->GetCharAliases())) {
			if (++j == number) {
				return i;
			}
		}
	}

	return nullptr;
}

const int kMoneyDestroyTimer = 60;
const int kDeathDestroyTimer = 5;
const int kRoomDestroyTimer = 10;
const int kScriptDestroyTimer = 10; // * !!! Never set less than ONE * //

/**
* put an object in a room
* Ахтунг, не надо тут экстрактить шмотку, если очень хочется - проверяйте и правьте 50 вызовов
* по коду, т.к. нигде оно нифига не проверяется на валидность после этой функции.
* \return 0 - невалидный объект или комната, 1 - все ок
*/
bool PlaceObjToRoom(ObjData *object, RoomRnum room) {
//	int sect = 0;
	if (!object || room < kFirstRoom || room > top_of_world) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to PlaceObjToRoom. (Room #%d/%d, obj %p)",
			room, top_of_world, object);
		return false;
	} 
	ArrangeObjs(object, &world[room]->contents);
	if (world[room]->vnum % 100 == 99 && zone_table[world[room]->zone_rn].vnum < dungeons::kZoneStartDungeons) {
		if (!(object->has_flag(EObjFlag::kAppearsDay)
				|| object->has_flag(EObjFlag::kAppearsFullmoon)
				|| object->has_flag(EObjFlag::kAppearsNight))) {
			debug::backtrace(runtime_config.logs(SYSLOG).handle());
			sprintf(buf, "Попытка поместить объект в виртуальную комнату: objvnum %d, objname %s, roomvnum %d, создан coredump", 
					object->get_vnum(), object->get_PName(ECase::kNom).c_str(), world[room]->vnum);
			mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		}
	}
	object->set_in_room(room);
	object->set_carried_by(nullptr);
	object->set_worn_by(nullptr);
	if (ROOM_FLAGGED(room, ERoomFlag::kNoItem)) {
		object->set_extra_flag(EObjFlag::kDecay);
	}
	if (object->get_script()->has_triggers()) {
		object->set_destroyer(kScriptDestroyTimer);
	} else if (object->get_type() == EObjType::kMoney) {
		object->set_destroyer(kMoneyDestroyTimer);
	} else if (ROOM_FLAGGED(room, ERoomFlag::kDeathTrap)) {
		object->set_destroyer(kDeathDestroyTimer);
	} else if (!IS_CORPSE(object)) {
		object->set_destroyer(kRoomDestroyTimer);
	}
	return true;
}

/**
 * Функция для удаления обьектов после лоада в комнату.
 * @param object - проверяемый объект.
 * @return - true - если посыпался, false - если остался
 */
bool CheckObjDecay(ObjData *object,  bool need_extract) {
	int room, sect;
	room = object->get_in_room();

	if (room == kNowhere) {
		return false;
	}
	sect = real_sector(room);

	if (((sect == ESector::kWaterSwim || sect == ESector::kWaterNoswim) &&
		!object->has_flag(EObjFlag::kSwimming) &&
		!object->has_flag(EObjFlag::kFlying) &&
		!IS_CORPSE(object))) {
		act("$o0 медленно утонул$G.",
			false, world[room]->first_character(), object, nullptr, kToRoom);
		act("$o0 медленно утонул$G.",
			false, world[room]->first_character(), object, nullptr, kToChar);
		if (need_extract) {
			ExtractObjFromWorld(object);
		}
		return true;
	}

	if (((sect == ESector::kOnlyFlying) && !IS_CORPSE(object) && !object->has_flag(EObjFlag::kFlying))) {

		act("$o0 упал$G вниз.",
			false, world[room]->first_character(), object, nullptr, kToRoom);
		act("$o0 упал$G вниз.",
			false, world[room]->first_character(), object, nullptr, kToChar);
		if (need_extract) {
			ExtractObjFromWorld(object);
		}
		return true;
	}

	if (object->has_flag(EObjFlag::kDecay) ||
		(object->has_flag(EObjFlag::kZonedacay) &&
		object->get_vnum_zone_from() != zone_table[world[room]->zone_rn].vnum)) {
		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", false,
			world[room]->first_character(), object, nullptr, kToRoom);
		act("$o0 рассыпал$U в мелкую пыль, которую развеял ветер.", false,
			world[room]->first_character(), object, nullptr, kToChar);
		if (need_extract) {
			ExtractObjFromWorld(object);
		}
		return true;
	}

	return false;
}

void RemoveObjFromRoom(ObjData *object) {
	if (!object || object->get_in_room() == kNowhere) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to RemoveObjFromRoom",
			object, object->get_in_room());
		return;
	}

	object->remove_me_from_contains_list(world[object->get_in_room()]->contents);

	object->set_in_room(kNowhere);
	object->set_next_content(nullptr);
}

void PlaceObjIntoObj(ObjData *obj, ObjData *obj_to) {
	ObjData *tmp_obj;

	if (!obj || !obj_to || obj == obj_to) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to PlaceObjIntoObj.",
			obj, obj, obj_to);
		return;
	}

	auto list = obj_to->get_contains();
	ArrangeObjs(obj, &list);
	obj_to->set_contains(list);
	obj->set_in_obj(obj_to);

	for (tmp_obj = obj->get_in_obj(); tmp_obj->get_in_obj(); tmp_obj = tmp_obj->get_in_obj()) {
		tmp_obj->add_weight(obj->get_weight());
	}

	// top level object.  Subtract weight from inventory if necessary.
	tmp_obj->add_weight(obj->get_weight());
	if (tmp_obj->get_carried_by()) {
		IS_CARRYING_W(tmp_obj->get_carried_by()) += obj->get_weight();
	}
}

// remove an object from an object
void RemoveObjFromObj(ObjData *obj) {
	if (obj->get_in_obj() == nullptr) {
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
		mudlog("SYSERR: trying to illegally extract obj from obj.");
		return;
	}
	auto obj_from = obj->get_in_obj();
	auto head = obj_from->get_contains();
	obj->remove_me_from_contains_list(head);
	obj_from->set_contains(head);

	// Subtract weight from containers container
	auto temp = obj->get_in_obj();
	for (; temp->get_in_obj(); temp = temp->get_in_obj()) {
		temp->set_weight(std::max(1, temp->get_weight() - obj->get_weight()));
	}

	// Subtract weight from char that carries the object
	temp->set_weight(std::max(1, temp->get_weight() - obj->get_weight()));
	if (temp->get_carried_by()) {
		IS_CARRYING_W(temp->get_carried_by()) = std::max(1, temp->get_carried_by()->GetCarryingWeight() - obj->get_weight());
	}

	obj->set_in_obj(nullptr);
	obj->set_next_content(nullptr);
}

// Set all carried_by to point to new owner
void object_list_new_owner(ObjData *list, CharData *ch) {
	if (list) {
		object_list_new_owner(list->get_contains(), ch);
		object_list_new_owner(list->get_next_content(), ch);
		list->set_carried_by(ch);
	}
}

RoomVnum get_room_where_obj(ObjData *obj, bool deep) {
	if (GET_ROOM_VNUM(obj->get_in_room()) != kNowhere) {
		return GET_ROOM_VNUM(obj->get_in_room());
	} else if (obj->get_in_obj() && !deep) {
		return get_room_where_obj(obj->get_in_obj(), true);
	} else if (obj->get_carried_by()) {
		return GET_ROOM_VNUM(obj->get_carried_by()->in_room);
	} else if (obj->get_worn_by()) {
		return GET_ROOM_VNUM(obj->get_worn_by()->in_room);
	}

	return kNowhere;
}

void ExtractObjFromWorld(ObjData *obj, bool showlog) {
	char name[kMaxStringLength];
	ObjData *temp;
	int roomload = get_room_where_obj(obj, false);
	utils::CExecutionTimer timer;

	strcpy(name, obj->get_PName(ECase::kNom).c_str());
	if (showlog) {
		log("[Extract obj] Start for: %s vnum == %d room = %d timer == %d",
				name, GET_OBJ_VNUM(obj), roomload, obj->get_timer());
	}
	// Обработка содержимого контейнера при его уничтожении
	purge_otrigger(obj);
//	log("[Extract obj] purge_otrigger, delta %f", timer.delta().count());
	if (obj->get_contains()) {
		while (obj->get_contains()) {
			temp = obj->get_contains();
			RemoveObjFromObj(temp);
			if (obj->get_carried_by()) {
				if (obj->get_carried_by()->IsNpc()
					|| (obj->get_carried_by()->GetCarryingQuantity() >= CAN_CARRY_N(obj->get_carried_by()))) {
					PlaceObjToRoom(temp, obj->get_carried_by()->in_room);
					CheckObjDecay(temp);
				} else {
					PlaceObjToInventory(temp, obj->get_carried_by());
				}
			} else if (obj->get_worn_by() != nullptr) {
				if (obj->get_worn_by()->IsNpc()
					|| (obj->get_worn_by()->GetCarryingQuantity() >= CAN_CARRY_N(obj->get_worn_by()))) {
					PlaceObjToRoom(temp, obj->get_worn_by()->in_room);
					CheckObjDecay(temp);
				} else {
					PlaceObjToInventory(temp, obj->get_worn_by());
				}
			} else if (obj->get_in_room() != kNowhere
					&& (IS_CORPSE(obj) || temp->has_flag(EObjFlag::kTicktimer))) {
				PlaceObjToRoom(temp, obj->get_in_room());
				CheckObjDecay(temp);
			} else if (obj->get_in_obj()) {
				ExtractObjFromWorld(temp, false);
			} else {
				ExtractObjFromWorld(temp, false);
			}
		}
		if (showlog)
			log("[Extract obj] Delta for container: %s vnum == %d room = %d timer == %d delta_time %f",
					name, GET_OBJ_VNUM(obj), get_room_where_obj(obj, false), obj->get_timer(), timer.delta().count());
	}
	// Содержимое контейнера удалено

	if (obj->get_worn_by() != nullptr) {
		if (UnequipChar(obj->get_worn_by(), obj->get_worn_on(), CharEquipFlags()) != obj) {
			log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
		}
	}

	if (obj->get_in_room() != kNowhere) {
		RemoveObjFromRoom(obj);
	} else if (obj->get_carried_by()) {
		RemoveObjFromChar(obj);
	} else if (obj->get_in_obj()) {
		RemoveObjFromObj(obj);
	}

	check_auction(nullptr, obj);
	check_exchange(obj);
	const auto rnum = obj->get_rnum();
	obj_proto.dec_number(rnum);
	obj->get_script()->set_purged();
	world_objects.remove(obj);
	if (showlog) {
		log("[Extract obj] Stop, delta %f", timer.delta().count());
	}
}

void UpdateCharObjects(CharData *ch) {
	for (int wear_pos = 0; wear_pos < EEquipPos::kNumEquipPos; wear_pos++) {
		if (GET_EQ(ch, wear_pos) != nullptr) {
			if (GET_EQ(ch, wear_pos)->get_type() == EObjType::kLightSource) {
				if (GET_OBJ_VAL(GET_EQ(ch, wear_pos), 2) > 0) {
					const int i = GET_EQ(ch, wear_pos)->dec_val(2);
					if (i == 1) {
						act("$z $o замерцал$G и начал$G угасать.\r\n",
							false, ch, GET_EQ(ch, wear_pos), nullptr, kToChar);
						act("$o $n1 замерцал$G и начал$G угасать.",
							false, ch, GET_EQ(ch, wear_pos), nullptr, kToRoom);
					} else if (i == 0) {
						act("$z $o погас$Q.\r\n", false, ch, GET_EQ(ch, wear_pos), nullptr, kToChar);
						act("$o $n1 погас$Q.", false, ch, GET_EQ(ch, wear_pos), nullptr, kToRoom);
						if (ch->in_room != kNowhere) {
							if (world[ch->in_room]->light > 0)
								world[ch->in_room]->light -= 1;
						}
						if (GET_EQ(ch, wear_pos)->has_flag(EObjFlag::kDecay)) {
							ExtractObjFromWorld(GET_EQ(ch, wear_pos));
						}
					}
				}
			}
		}
	}
}

/**
* Если на мобе шмотки, одетые во время резета зоны, то при резете в случае пуржа моба - они уничтожаются с ним же.
* Если на мобе шмотки, поднятые и бывшие у игрока (таймер уже тикал), то он их при резете выкинет на землю, как обычно.
* А то при резетах например той же мавки умудрялись лутить шмот с земли, упавший с нее до того, как она сама поднимет,
* плюс этот лоад накапливался и можно было заиметь несколько шмоток сразу с нескольких резетов. -- Krodo
* \param inv - 1 сообщение о выкидывании из инвентаря, 0 - о снятии с себя
* \param zone_reset - 1 - пуржим стаф без включенных таймеров, 0 - не пуржим ничего
*/
void DropObjOnZoneReset(CharData *ch, ObjData *obj, bool inv, bool zone_reset) {
	if (zone_reset && !obj->has_flag(EObjFlag::kTicktimer)) {
		ExtractObjFromWorld(obj, false);
	} else {
		if (inv)
			act("Вы выбросили $o3 на землю.", false, ch, obj, nullptr, kToChar);
		else
			act("Вы сняли $o3 и выбросили на землю.", false, ch, obj, nullptr, kToChar);
		// Если этот моб трупа не оставит, то не выводить сообщение
		// иначе ужасно коряво смотрится в бою и в тригах
		bool msgShown = false;
		if (!ch->IsNpc() || !ch->IsFlagged(EMobFlag::kCorpse)) {
			if (inv)
				act("$n бросил$g $o3 на землю.", false, ch, obj, nullptr, kToRoom);
			else
				act("$n снял$g $o3 и бросил$g на землю.", false, ch, obj, nullptr, kToRoom);
			msgShown = true;
		}

		drop_otrigger(obj, ch, kOtrigDropInroom);

		drop_wtrigger(obj, ch);

		PlaceObjToRoom(obj, ch->in_room);
		if (!CheckObjDecay(obj) && !msgShown) {
			act("На земле остал$U лежать $o.", false, ch, obj, nullptr, kToRoom);
		}
	}
}

namespace {

void change_npc_leader(CharData *ch) {
	std::vector<CharData *> tmp_list;

	for (FollowerType *i = ch->followers; i; i = i->next) {
		if (i->follower->IsNpc()
			&& !IS_CHARMICE(i->follower)
			&& i->follower->get_master() == ch) {
			tmp_list.push_back(i->follower);
		}
	}
	if (tmp_list.empty()) {
		return;
	}

	CharData *leader = nullptr;
	for (auto i : tmp_list) {
		if (stop_follower(i, kSfSilence)) {
			continue;
		}
		if (!leader) {
			leader = i;
		} else {
			leader->add_follower_silently(i);
		}
	}
}

} // namespace

void DropEquipment(CharData *ch, bool zone_reset) {
	for (auto i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)) {
			ObjData *obj_eq = UnequipChar(ch, i, CharEquipFlags());
			if (!obj_eq) {
				continue;
			}
			remove_otrigger(obj_eq, ch);
			DropObjOnZoneReset(ch, obj_eq, false, zone_reset);
		}
	}
}
void DropInventory(CharData *ch, bool zone_reset) {
	while (ch->carrying) {
		ObjData *obj = ch->carrying;
		RemoveObjFromChar(obj);
		DropObjOnZoneReset(ch, obj, true, zone_reset);
	}
}
/**
* Extract a ch completely from the world, and leave his stuff behind
* \param zone_reset - 0 обычный пурж когда угодно (по умолчанию), 1 - пурж при резете зоны
*/
void ExtractCharFromWorld(CharData *ch, int clear_objs, bool zone_reset) {
	if (ch->purged()) {
		log("SYSERROR: double extract_char (%s:%d)", __FILE__, __LINE__);
		return;
	}
	
	if (ch->IsFlagged(EMobFlag::kMobFreed) || ch->IsFlagged(EMobFlag::kMobDeleted)) {
		return;
	}

	std::string name = GET_NAME(ch);
	DescriptorData *t_desc;
	utils::CExecutionTimer timer;

	log("[Extract char] Start function for char %s VNUM: %d", name.c_str(), GET_MOB_VNUM(ch));
	if (!ch->IsNpc() && !ch->desc) {
//		log("[Extract char] Extract descriptors");
		for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next) {
			if (t_desc->original.get() == ch) {
				DoReturn(t_desc->character.get(), nullptr, 0, 0);
			}
		}
	}

	// Forget snooping, if applicable
//	log("[Extract char] Stop snooping");
	if (ch->desc) {
		if (ch->desc->snooping) {
			ch->desc->snooping->snoop_by = nullptr;
			ch->desc->snooping = nullptr;
		}

		if (ch->desc->snoop_by) {
			iosystem::write_to_output("Ваша жертва теперь недоступна.\r\n", ch->desc->snoop_by);
			ch->desc->snoop_by->snooping = nullptr;
			ch->desc->snoop_by = nullptr;
		}
	}

	// transfer equipment to room, if any
//	log("[Extract char] Drop equipment & inventory");
	if (ch->in_room != kNowhere) {
		DropEquipment(ch, zone_reset);
		DropInventory(ch, zone_reset);
	}

	if (ch->IsNpc()) {
		// дроп гривен до изменений последователей за мобом
		ExtMoney::drop_torc(ch);
	}

	if (!ch->IsNpc()
		&& !ch->has_master()
		&& ch->followers
		&& AFF_FLAGGED(ch, EAffect::kGroup)) {
//		log("[Extract char] Change group leader");
		change_leader(ch, nullptr);
	} else if (ch->IsNpc()
		&& !IS_CHARMICE(ch)
		&& !ch->has_master()
		&& ch->followers) {
//		log("[Extract char] Changing NPC leader");
		change_npc_leader(ch);
	}

//	log("[Extract char] Die followers");
	if ((ch->followers || ch->has_master())
		&& die_follower(ch)) {
		// TODO: странно все это с пуржем в stop_follower
		//закостылил чтоб экстракт тут не делался для ch->has_master()
	}
//	log("[Extract char] Stop all fight for opponee");
	change_fighting(ch, true);

//	log("[Extract char] Remove char from room");
	if (ch->in_room != kNowhere) {
		RemoveCharFromRoom(ch);
	}

	// pull the char from the list
	ch->SetFlag(EMobFlag::kMobDeleted);

	if (ch->desc && ch->desc->original) {
		DoReturn(ch, nullptr, 0, 0);
	}

	const bool is_npc = ch->IsNpc();
	if (!is_npc) {
//		log("[Extract char] All save for PC");
		check_auction(ch, nullptr);
		ch->save_char();
		//удаляются рент-файлы, если только персонаж не ушел в ренту
		Crash_delete_crashfile(ch);
	} else {
//		log("[Extract char] All clear for NPC");
		if ((GET_MOB_RNUM(ch) >= 0) && !ch->IsFlagged(EMobFlag::kSummoned)) {
			mob_index[GET_MOB_RNUM(ch)].total_online--;
		}
	}
	chardata_by_uid.erase(ch->get_uid());
	bool left_in_game = false;
	if (!is_npc
		&& ch->desc != nullptr) {
		ch->desc->state = EConState::kMenu;
		iosystem::write_to_output(MENU, ch->desc);
		if (!ch->IsNpc() && NORENTABLE(ch) && clear_objs) {
			ch->zero_wait();
			do_entergame(ch->desc);
			left_in_game = true;
		}
	}
	if (ch->get_protecting()) {
		ch->remove_protecting();
	}
	if (!ch->who_protecting.empty()) {
		auto protecting_copy = ch->who_protecting;
		for (auto &it : protecting_copy) {
			it->remove_protecting();
		}
	}
	if (!left_in_game) {
		character_list.remove(ch);
	}

	log("[Extract char] Stop function for char %s, delta %f", name.c_str(), timer.delta().count());
}

/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */

// ищем по имени в character list
CharData *get_player_of_name(const char *name) {
	for (const auto &i : character_list) {
		if (i->IsNpc()) {
			continue;
		}
		if (!isname(name, i->GetCharAliases())) {
			continue;
		}
		return i.get();
	}
	return nullptr;
}

CharData *get_player_vis(CharData *ch, const char *name, int inroom) {
	DescriptorData *d;
	for (d = descriptor_list; d; d = d->next) {
		if (d->state != EConState::kPlaying) {
			continue;
		}
		if (!HERE(d->character))
			continue;
		if ((inroom & EFind::kCharInRoom) && d->character->in_room != ch->in_room)
			continue;
		if (!CAN_SEE_CHAR(ch, d->character))
			continue;
		if (!isname(name, d->character->GetCharAliases())) {
			continue;
		}
		return d->character.get();
	}
	return nullptr;
}

CharData *get_player_pun(CharData *ch, const char *name, int inroom) {
	for (const auto &i : character_list) {
		if (i->IsNpc())
			continue;
		if ((inroom & EFind::kCharInRoom) && i->in_room != ch->in_room)
			continue;
		if (!isname(name, i->GetCharAliases())) {
			continue;
		}
		return i.get();
	}
	return nullptr;
}

CharData *get_char_room_vis(CharData *ch, const char *name) {
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;
	// JE 7/18/94 :-) :-)
	if (!str_cmp(name, "self")
		|| !str_cmp(name, "me")
		|| !str_cmp(name, "я")
		|| !str_cmp(name, "меня")
		|| !str_cmp(name, "себя")) {
		return (ch);
	}
	// 0.<name> means PC with name
	strl_cpy(tmp, name, kMaxInputLength);
	const int number = get_number(&tmp);
	if (0 == number) {
		return get_player_vis(ch, tmp, EFind::kCharInRoom);
	}
	int j = 0;
	for (const auto i : world[ch->in_room]->people) {
		if (HERE(i) && CAN_SEE(ch, i)
			&& isname(tmp, i->GetCharAliases())) {
			if (++j == number) {
				return i;
			}
		}
	}
	return nullptr;
}

CharData *get_char_vis(CharData *ch, const char *name, int where) {
	CharData *i;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	// check the room first
	if (where == EFind::kCharInRoom) {
		return get_char_room_vis(ch, name);
	} else if (where == EFind::kCharInWorld) {
		if ((i = get_char_room_vis(ch, name)) != nullptr) {
			return (i);
		}

		strcpy(tmp, name);
		const int number = get_number(&tmp);
		if (0 == number) {
			return get_player_vis(ch, tmp, EFind::kCharInRoom);
		}
		if (number == 1) {
			CharData *tmp_ch = get_player_vis(ch, tmp, EFind::kCharInWorld);
			if (tmp_ch != nullptr) {
				return tmp_ch;
			}
		}
		int j = 0;
		for (const auto &target : character_list) {
			if (HERE(target) && CAN_SEE(ch, target)
				&& isname(tmp, target->GetCharAliases())) {
				if (++j == number) {
					return target.get();
				}
			}
		}
	}
	return nullptr;
}

ObjData *get_obj_in_list_vis(CharData *ch, const char *name, ObjData *list, bool locate_item) {
	ObjData *i;
	int j = 0, number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return (nullptr);

	//Запретим локейт 2. 3. n. стафин
	if (number > 1 && locate_item)
		return (nullptr);

	for (i = list; i && (j <= number); i = i->get_next_content()) {
		if (i->get_extracted_list()) {
			continue;
		}
		if (isname(tmp, i->get_aliases())
			|| CHECK_CUSTOM_LABEL(tmp, i, ch)) {
			if (CAN_SEE_OBJ(ch, i)) {
				// sprintf(buf,"Show obj %d %s %x ", number, i->name, i);
				// SendMsgToChar(buf,ch);
				if (!locate_item) {
					if (++j == number)
						return (i);
				} else {
					if (try_locate_obj(ch, i))
						return (i);
					else
						continue;
				}
			}
		}
	}

	return (nullptr);
}

class ExitLoopException : std::exception {};

ObjData *get_obj_vis_and_dec_num(CharData *ch,
								 const char *name,
								 ObjData *list,
								 std::unordered_set<unsigned int> &id_obj_set,
								 int &number) {
	for (auto item = list; item != nullptr; item = item->get_next_content()) {
		if (CAN_SEE_OBJ(ch, item)) {
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch)) {
				if (--number == 0) {
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}

	return nullptr;
}

ObjData *get_obj_vis_and_dec_num(CharData *ch,
								 const char *name,
								 ObjData *equip[],
								 std::unordered_set<unsigned int> &id_obj_set,
								 int &number) {
	for (auto i = 0; i < EEquipPos::kNumEquipPos; ++i) {
		auto item = equip[i];
		if (item && CAN_SEE_OBJ(ch, item)) {
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch)) {
				if (--number == 0) {
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}

	return nullptr;
}

// search the entire world for an object, and return a pointer
ObjData *get_obj_vis(CharData *ch, const char *name) {
	int number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, name);
	number = get_number(&tmp);
	if (number < 1) {
		return nullptr;
	}

	auto id_obj_set = std::unordered_set<unsigned int>();

	//Scan in equipment
	auto obj = get_obj_vis_and_dec_num(ch, tmp, ch->equipment, id_obj_set, number);
	if (obj) {
		return obj;
	}

	//Scan in carried items
	obj = get_obj_vis_and_dec_num(ch, tmp, ch->carrying, id_obj_set, number);
	if (obj) {
		return obj;
	}

	//Scan in room
	obj = get_obj_vis_and_dec_num(ch, tmp, world[ch->in_room]->contents, id_obj_set, number);
	if (obj) {
		return obj;
	}

	//Scan charater's in room
	for (const auto &vict : world[ch->in_room]->people) {
		if (ch->get_uid() == vict->get_uid()) {
			continue;
		}

		//Scan in equipment
		obj = get_obj_vis_and_dec_num(ch, tmp, vict->equipment, id_obj_set, number);
		if (obj) {
			return obj;
		}

		//Scan in carried items
		obj = get_obj_vis_and_dec_num(ch, tmp, vict->carrying, id_obj_set, number);
		if (obj) {
			return obj;
		}
	}

	// ok.. no luck yet. scan the entire obj list except already found
	const WorldObjects::predicate_f predicate = [&ch, &tmp, &id_obj_set](const ObjData::shared_ptr &i) -> bool {
		const auto result = CAN_SEE_OBJ(ch, i.get())
			&& (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i.get(), ch))
			&& (id_obj_set.count(i.get()->get_id()) == 0);
		return result;
	};
	obj = world_objects.find_if(predicate, number).get();
		if (obj) {
		return obj;
	}
	obj = Depot::find_obj_from_depot_and_dec_number(tmp, number);
		if (obj) {
		return obj;
	}
	return nullptr;
}

// search the entire world for an object, and return a pointer
ObjData *get_obj_vis_for_locate(CharData *ch, const char *name) {
	ObjData *i;
	int number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	// scan items carried //
	if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != nullptr) {
		return i;
	}

	// scan room //
	if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room]->contents)) != nullptr) {
		return i;
	}

	strcpy(tmp, name);
	number = get_number(&tmp);
	if (number != 1) {
		return nullptr;
	}

	// ok.. no luck yet. scan the entire obj list   //
	const WorldObjects::predicate_f locate_predicate = [&](const ObjData::shared_ptr &i) -> bool {
		const auto result = CAN_SEE_OBJ(ch, i.get())
			&& (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i.get(), ch))
			&& try_locate_obj(ch, i.get());
		return result;
	};

	return world_objects.find_if(locate_predicate).get();
}

bool try_locate_obj(CharData *ch, ObjData *i) {
	if (IS_CORPSE(i) || IS_GOD(ch)) //имм может локейтить и можно локейтить трупы
	{
		return true;
	} else if (i->has_flag(EObjFlag::kNolocate)) //если флаг !локейт и ее нет в комнате/инвентаре - пропустим ее
	{
		return false;
	} else if (i->get_carried_by() && i->get_carried_by()->IsNpc()) {
		if (world[i->get_carried_by()->in_room]->zone_rn
			== world[ch->in_room]->zone_rn) //шмотки у моба можно локейтить только в одной зоне
		{
			return true;
		} else {
			return false;
		}
	} else if (i->get_in_room() != kNowhere && i->get_in_room()) {
		if (world[i->get_in_room()]->zone_rn
			== world[ch->in_room]->zone_rn) //шмотки в клетке можно локейтить только в одной зоне
		{
			return true;
		} else {
			return false;
		}
	} else if (i->get_worn_by() && i->get_worn_by()->IsNpc()) {
		if (world[i->get_worn_by()->in_room]->zone_rn == world[ch->in_room]->zone_rn) {
			return true;
		} else {
			return false;
		}
	} else if (i->get_in_obj()) {
		if (Clan::is_clan_chest(i->get_in_obj())) {
			return true;
		} else {
			const auto in_obj = i->get_in_obj();
			if (in_obj->get_carried_by()) {
				if (in_obj->get_carried_by()->IsNpc()) {
					if (world[in_obj->get_carried_by()->in_room]->zone_rn == world[ch->in_room]->zone_rn) {
						return true;
					} else {
						return false;
					}
				} else {
					return true;
				}
			} else if (in_obj->get_in_room() != kNowhere && in_obj->get_in_room()) {
				if (world[in_obj->get_in_room()]->zone_rn == world[ch->in_room]->zone_rn) {
					return true;
				} else {
					return false;
				}
			} else if (in_obj->get_worn_by()) {
				const auto worn_by = i->get_in_obj()->get_worn_by();
				if (worn_by->IsNpc()) {
					if (world[worn_by->in_room]->zone_rn == world[ch->in_room]->zone_rn) {
						return true;
					} else {
						return false;
					}
				} else {
					return true;
				}
			} else {
				return true;
			}
		}
	} else {
		return true;
	}
}

ObjData *get_object_in_equip_vis(CharData *ch, const char *arg, ObjData *equipment[], int *j) {
	int l, number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, arg);
	if (!(number = get_number(&tmp)))
		return (nullptr);

	for ((*j) = 0, l = 0; (*j) < EEquipPos::kNumEquipPos; (*j)++) {
		if (equipment[(*j)]) {
			if (CAN_SEE_OBJ(ch, equipment[(*j)])) {
				if (isname(tmp, equipment[(*j)]->get_aliases())
					|| CHECK_CUSTOM_LABEL(tmp, equipment[(*j)], ch)) {
					if (++l == number) {
						return equipment[(*j)];
					}
				}
			}
		}
	}

	return (nullptr);
}

/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be nullptr if no character was found, otherwise points
 * **tar_obj Will be nullptr if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(char *arg, Bitvector bitvector, CharData *ch, CharData **tar_ch, ObjData **tar_obj) {
	char name[kMaxInputLength];

	*tar_ch = nullptr;
	*tar_obj = nullptr;

	ObjData *i;
	int l, number, j = 0;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	if (strlen(arg) > 256)
		return 0;
	one_argument(arg, name);

	if (!*name)
		return (0);

	if (IS_SET(bitvector, EFind::kCharInRoom))    // Find person in room
	{
		if ((*tar_ch = get_char_vis(ch, name, EFind::kCharInRoom)) != nullptr)
			return (EFind::kCharInRoom);
	}
	if (IS_SET(bitvector, EFind::kCharInWorld)) {
		if ((*tar_ch = get_char_vis(ch, name, EFind::kCharInWorld)) != nullptr)
			return (EFind::kCharInWorld);
	}
	if (IS_SET(bitvector, EFind::kObjWorld)) {
		if ((*tar_obj = get_obj_vis(ch, name)))
			return (EFind::kObjWorld);
	}

	// Начало изменений. (с) Дмитрий ака dzMUDiST ака

// Переписан код, обрабатывающий параметры EFind::kObjEquip | EFind::kObjInventory | EFind::kObjRoom
// В итоге поиск объекта просиходит в " инветаре - комнате - экипировке" согласно
// общему количеству имеющихся "созвучных" предметов.
// Старый код закомментирован и подан в конце изменений.

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return 0;
	if (IS_SET(bitvector, EFind::kObjInventory)) {
		for (i = ch->carrying; i && (j <= number); i = i->get_next_content()) {
			if (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, EFind::kObjExtraDesc)
					&& find_exdesc(tmp, i->get_ex_description()))) {
				if (CAN_SEE_OBJ(ch, i)) {
					if (++j == number) {
						*tar_obj = i;
						return (EFind::kObjInventory);
					}
				}
			}
		}
	}
	if (IS_SET(bitvector, EFind::kObjRoom)) {
		for (i = world[ch->in_room]->contents;
			 i && (j <= number); i = i->get_next_content()) {
			if (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, EFind::kObjExtraDesc)
					&& find_exdesc(tmp, i->get_ex_description()))) {
				if (CAN_SEE_OBJ(ch, i)) {
					if (++j == number) {
						*tar_obj = i;
						return (EFind::kObjRoom);
					}
				}
			}
		}
	}
	if (IS_SET(bitvector, EFind::kObjEquip)) {
		for (l = 0; l < EEquipPos::kNumEquipPos; l++) {
			if (GET_EQ(ch, l) && CAN_SEE_OBJ(ch, GET_EQ(ch, l))) {
				if (isname(tmp, GET_EQ(ch, l)->get_aliases())
					|| CHECK_CUSTOM_LABEL(tmp, GET_EQ(ch, l), ch)
					|| (IS_SET(bitvector, EFind::kObjExtraDesc)
						&& find_exdesc(tmp, GET_EQ(ch, l)->get_ex_description()))) {
					if (++j == number) {
						*tar_obj = GET_EQ(ch, l);
						return (EFind::kObjEquip);
					}
				}
			}
		}
	}
	return (0);
}

// a function to scan for "all" or "all.x"
int find_all_dots(char *arg) {
	char tmpname[kMaxInputLength];

	if (!str_cmp(arg, "all") || !str_cmp(arg, "все")) {
		return (kFindAll);
	} else if (!strn_cmp(arg, "all.", 4) || !strn_cmp(arg, "все.", 4)) {
		strl_cpy(tmpname, arg + 4, kMaxInputLength);
		strl_cpy(arg, tmpname, kMaxInputLength);
		return (kFindAlldot);
	} else {
		return (kFindIndiv);
	}
}

RoomRnum FindRoomRnum(CharData *ch, char *rawroomstr, int trig) {
	RoomVnum tmp;
	RoomRnum location;
	CharData *target_mob;
	ObjData *target_obj;
	char roomstr[kMaxInputLength];

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		SendMsgToChar("Укажите номер или название комнаты.\r\n", ch);
		return (kNowhere);
	}
	if (a_isdigit(*roomstr) && !strchr(roomstr, '.')) {
		tmp = atoi(roomstr);
		if ((location = GetRoomRnum(tmp)) == kNowhere) {
			SendMsgToChar("Нет комнаты с таким номером.\r\n", ch);
			return (kNowhere);
		}
	} else if ((target_mob = get_char_vis(ch, roomstr, EFind::kCharInWorld)) != nullptr) {
		location = target_mob->in_room;
	} else if ((target_obj = get_obj_vis(ch, roomstr)) != nullptr) {
		if (target_obj->get_in_room() != kNowhere) {
			location = target_obj->get_in_room();
		} else {
			SendMsgToChar("Этот объект вам недоступен.\r\n", ch);
			return (kNowhere);
		}
	} else {
		SendMsgToChar("В округе нет похожего предмета или создания.\r\n", ch);
		return (kNowhere);
	}

	// a location has been found -- if you're < GRGOD, check restrictions.
	if (!IS_GRGOD(ch) && !ch->IsFlagged(EPrf::kCoderinfo)) {
		if (ROOM_FLAGGED(location, ERoomFlag::kGodsRoom) && GetRealLevel(ch) < kLvlGreatGod) {
			SendMsgToChar("Вы не столь божественны, чтобы получить доступ в эту комнату!\r\n", ch);
			return (kNowhere);
		}
		if (ROOM_FLAGGED(location, ERoomFlag::kNoTeleportIn) && trig != 1) {
			SendMsgToChar("В комнату не телепортировать!\r\n", ch);
			return (kNowhere);
		}
		if (!Clan::MayEnter(ch, location, kHousePortal)) {
			SendMsgToChar("Частная собственность - посторонним в ней делать нечего!\r\n", ch);
			return (kNowhere);
		}
	}
	return (location);
}

int IsEquipInMetall(CharData *ch) {
	int i, wgt = 0;

	if (ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed))
		return (false);
	if (IS_GOD(ch))
		return (false);

	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)
			&& ObjSystem::is_armor_type(GET_EQ(ch, i))
			&& GET_EQ(ch, i)->get_material() <= EObjMaterial::kPreciousMetel) {
			wgt += GET_EQ(ch, i)->get_weight();
		}
	}

	if (wgt > GetRealStr(ch))
		return (true);

	return (false);
}

// * Берется минимальная цена ренты шмотки, не важно, одетая она будет или снятая.
int get_object_low_rent(ObjData *obj) {
	int rent = obj->get_rent_off() > obj->get_rent_on() ? obj->get_rent_on() : obj->get_rent_off();
	return rent;
}

// Функция проверяет может ли ch нести предмет obj и загружает предмет
// в инвентарь игрока или в комнату, где игрок находится
void can_carry_obj(CharData *ch, ObjData *obj) {
	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
		SendMsgToChar("Вы не можете нести столько предметов.", ch);
		PlaceObjToRoom(obj, ch->in_room);
		CheckObjDecay(obj);
	} else {
		if (obj->get_weight() + ch->GetCarryingWeight() > CAN_CARRY_W(ch)) {
			sprintf(buf, "Вам слишком тяжело нести еще и %s.", obj->get_PName(ECase::kAcc).c_str());
			SendMsgToChar(buf, ch);
			PlaceObjToRoom(obj, ch->in_room);
			// obj_decay(obj);
		} else {
			PlaceObjToInventory(obj, ch);
		}
	}
}

int num_pc_in_room(RoomData *room) {
	int i = 0;
	for (const auto ch : room->people) {
		if (!ch->IsNpc()) {
			i++;
		}
	}

	return i;
}

int check_moves(CharData *ch, int how_moves) {
	if (IS_IMMORTAL(ch) || ch->IsNpc())
		return (true);
	if (ch->get_move() < how_moves) {
		SendMsgToChar("Вы слишком устали.\r\n", ch);
		return (false);
	}
	ch->set_move(ch->get_move() - how_moves);
	return (true);
}

int real_sector(int room) {
	int sector = SECT(room);

	if (ROOM_FLAGGED(room, ERoomFlag::kNoWeather))
		return sector;
	switch (sector) {
		case ESector::kInside:
		case ESector::kCity:
		case ESector::kOnlyFlying:
		case ESector::kUnderwater:
		case ESector::kSecret:
		case ESector::kStoneroad:
		case ESector::kRoad:
		case ESector::kWildroad: return sector;
			break;
		case ESector::kField:
			if (world[room]->weather.snowlevel > 20)
				return ESector::kFieldSnow;
			else if (world[room]->weather.rainlevel > 20)
				return ESector::kFieldRain;
			else
				return ESector::kField;
			break;
		case ESector::kForest:
			if (world[room]->weather.snowlevel > 20)
				return ESector::kForestSnow;
			else if (world[room]->weather.rainlevel > 20)
				return ESector::kForestRain;
			else
				return ESector::kForest;
			break;
		case ESector::kHills:
			if (world[room]->weather.snowlevel > 20)
				return ESector::kHillsSnow;
			else if (world[room]->weather.rainlevel > 20)
				return ESector::kHillsRain;
			else
				return ESector::kHills;
			break;
		case ESector::kMountain:
			if (world[room]->weather.snowlevel > 20)
				return ESector::kMountainSnow;
			else
				return ESector::kMountain;
			break;
		case ESector::kWaterSwim:
		case ESector::kWaterNoswim:
			if (world[room]->weather.icelevel > 30)
				return ESector::kThickIce;
			else if (world[room]->weather.icelevel > 20)
				return ESector::kNormalIce;
			else if (world[room]->weather.icelevel > 10)
				return ESector::kThinIce;
			else
				return sector;
			break;
	}
	return ESector::kInside;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
