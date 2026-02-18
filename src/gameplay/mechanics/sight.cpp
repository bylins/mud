/**
\file sight.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Mechanics of sight.
*/

#include "sight.h"

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/db/description.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"
#include "engine/core/utils_char_obj.inl"
#include "dungeons.h"
#include "gameplay/crafting/mining.h"
#include "depot.h"
#include "liquid.h"
#include "engine/ui/modify.h"
#include "engine/ui/mapsystem.h"
#include "gameplay/clans/house.h"
#include "gameplay/skills/pick.h"
#include "engine/db/obj_prototypes.h"
#include "stable_objs.h"
#include "weather.h"
#include "gameplay/mechanics/illumination.h"
#include "gameplay/mechanics/awake.h"

#define TAG_NIGHT       "<night>"
#define TAG_DAY         "<day>"
#define TAG_WINTERNIGHT "<winternight>"
#define TAG_WINTERDAY   "<winterday>"
#define TAG_SPRINGNIGHT "<springnight>"
#define TAG_SPRINGDAY   "<springday>"
#define TAG_SUMMERNIGHT "<summernight>"
#define TAG_SUMMERDAY   "<summerday>"
#define TAG_AUTUMNNIGHT "<autumnnight>"
#define TAG_AUTUMNDAY   "<autumnday>"

#define MAX_FIRES 6
const char *Fires[MAX_FIRES] = {"тлеет небольшая кучка угольков",
								"тлеет небольшая кучка угольков",
								"еле-еле теплится огонек",
								"догорает небольшой костер",
								"весело трещит костер",
								"ярко пылает костер"
};

const char *ObjState[8][2] = {{"рассыпается", "рассыпается"},
							  {"плачевно", "в плачевном состоянии"},
							  {"плохо", "в плохом состоянии"},
							  {"неплохо", "в неплохом состоянии"},
							  {"средне", "в рабочем состоянии"},
							  {"хорошо", "в хорошем состоянии"},
							  {"очень хорошо", "в очень хорошем состоянии"},
							  {"великолепно", "в великолепном состоянии"}
};

void do_auto_exits(CharData *ch);
void show_extend_room(const char *description, CharData *ch);
void list_char_to_char_thing(const RoomData::people_t &list, CharData *ch);
int paste_description(char *string, const char *tag, int need);
void show_room_affects(CharData *ch, const char *name_affects[], const char *name_self_affects[]);
bool quest_item(ObjData *obj);
void look_at_char(CharData *i, CharData *ch);
std::string AddLeadingStringSpace(const std::string& text);
char *diag_obj_to_char(ObjData *obj, int mode);
void ListOneChar(CharData *i, CharData *ch, ESkill mode);

char *diag_weapon_to_char(const CObjectPrototype *obj, int show_wear);
std::string diag_armor_type_to_char(const ObjData *obj);
char *diag_timer_to_char(const ObjData *obj);
bool put_delim(std::stringstream &out, bool delim);
char *diag_uses_to_char(ObjData *obj, CharData *ch);
char *diag_shot_to_char(ObjData *obj, CharData *ch);
const char *diag_obj_timer(const ObjData *obj);

void look_at_room(CharData *ch, int ignore_brief, bool msdp_mode) {
	if (!ch->desc)
		return;
	if (is_dark(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !CanUseFeat(ch, EFeat::kDarkReading)) {
		SendMsgToChar("Слишком темно...\r\n", ch);
		show_glow_objs(ch);
		return;
	} else if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы все еще слепы...\r\n", ch);
		return;
	} else if (ch->GetPosition() < EPosition::kSleep) {
		return;
	}
	if (msdp_mode) {
		ch->desc->msdp_report("ROOM");
	}
	if (ch->IsFlagged(EPrf::kDrawMap) && !ch->IsFlagged(EPrf::kBlindMode)) {
		MapSystem::print_map(ch);
	} else if (ch->desc->snoop_by
		&& ch->desc->snoop_by->snoop_with_map
		&& ch->desc->snoop_by->character) {
		ch->map_print_to_snooper(ch->desc->snoop_by->character.get());
	}

	SendMsgToChar(kColorBoldCyn, ch);

	if (!ch->IsNpc() && (ch->IsFlagged(EPrf::kRoomFlags) || InTestZone(ch))) {
		// иммам рандомная * во флагах ломает мапер грят
		const bool has_flag = ROOM_FLAGGED(ch->in_room, ERoomFlag::kBfsMark) ? true : false;
		world[ch->in_room]->unset_flag(ERoomFlag::kBfsMark);

		world[ch->in_room]->flags_sprint(buf, ";");
		snprintf(buf2, kMaxStringLength, "[%5d] %s [%s]", GET_ROOM_VNUM(ch->in_room), world[ch->in_room]->name, buf);
		SendMsgToChar(buf2, ch);

		if (has_flag) {
			world[ch->in_room]->set_flag(ERoomFlag::kBfsMark);
		}
	} else {
		if (ch->IsFlagged(EPrf::kMapper) && !ch->IsFlagged(EPlrFlag::kScriptWriter)
			&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kMoMapper)) {
			RoomVnum rvn;
			if (zone_table[world[ch->in_room]->zone_rn].copy_from_zone > 0) {
				rvn = zone_table[world[ch->in_room]->zone_rn].copy_from_zone * 100 + world[ch->in_room]->vnum % 100;
			} else {
				rvn =  world[ch->in_room]->vnum;
			}
			sprintf(buf2, "%s [%d]", world[ch->in_room]->name, rvn);
			SendMsgToChar(buf2, ch);
		} else
			SendMsgToChar(world[ch->in_room]->name, ch);
	}

	SendMsgToChar(kColorNrm, ch);
	SendMsgToChar("\r\n", ch);

	if (is_dark(ch->in_room) && !ch->IsFlagged(EPrf::kHolylight)) {
		SendMsgToChar("Слишком темно...\r\n", ch);
	} else if ((!ch->IsNpc() && !ch->IsFlagged(EPrf::kBrief)) || ignore_brief || ROOM_FLAGGED(ch->in_room, ERoomFlag::kDeathTrap)) {
		show_extend_room(RoomDescription::show_desc(world[ch->in_room]->description_num).c_str(), ch);
	}

	if (ch->IsFlagged(EPrf::kAutoexit) && !ch->IsFlagged(EPlrFlag::kScriptWriter)) {
		do_auto_exits(ch);
	}

	// Отображаем аффекты комнаты. После автовыходов чтобы не ломать популярный маппер.
	if (AFF_FLAGGED(ch, EAffect::kDetectMagic) || IS_IMMORTAL(ch)) {
		show_room_affects(ch, room_aff_invis_bits, room_self_aff_invis_bits);
	} else {
		show_room_affects(ch, room_aff_visib_bits, room_aff_visib_bits);
	}

	// now list characters & objects
	if (world[ch->in_room]->fires) {
		sprintf(buf, "%sВ центре %s.%s\r\n", kColorRed, Fires[MIN(world[ch->in_room]->fires, MAX_FIRES - 1)], kColorNrm);
		SendMsgToChar(buf, ch);
	}
	if (room_spells::IsRoomAffected(world[ch->in_room], ESpell::kPortalTimer)) {
		for (const auto &aff : world[ch->in_room]->affected) {
			if (aff->type == ESpell::kPortalTimer && aff->bitvector != room_spells::ERoomAffect::kNoPortalExit) {
				if (IS_GOD(ch)) {
					sprintf(buf, "&BЛазурная пентаграмма ярко сверкает здесь. (время: %d, куда: %d)&n\r\n",
							aff->duration,  world[aff->modifier]->vnum);
				} else {
					if (world[ch->in_room]->pkPenterUnique) {
						sprintf(buf, "%sЛазурная пентаграмма %sс кровавым отблеском%s ярко сверкает здесь.%s\r\n",
								kColorBoldBlu, kColorBoldRed, kColorBoldBlu, kColorNrm);
					} else {
						sprintf(buf, "&BЛазурная пентаграмма ярко сверкает здесь.&n\r\n");
					}
				}
				SendMsgToChar(buf, ch);
			}
		}
	}

	if (world[ch->in_room]->holes) {
		const int ar = roundup(world[ch->in_room]->holes / kHolesTime);
		sprintf(buf, "%sЗдесь выкопана ямка глубиной примерно в %i аршин%s.%s\r\n",
				kColorYel, ar, (ar == 1 ? "" : (ar < 5 ? "а" : "ов")), (kColorNrm));
		SendMsgToChar(buf, ch);
	}

	if (ch->in_room != kNowhere && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoWeather)
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kIndoors)) {
		*buf = '\0';
		switch (real_sector(ch->in_room)) {
			case ESector::kFieldSnow:
			case ESector::kForestSnow:
			case ESector::kHillsSnow:
			case ESector::kMountainSnow:
				sprintf(buf, "%sСнежный ковер лежит у вас под ногами.%s\r\n",
						kColorWht, kColorNrm);
				break;
			case ESector::kFieldRain:
			case ESector::kForestRain:
			case ESector::kHillsRain:
				sprintf(buf,
						"%sВы просто увязаете в грязи.%s\r\n",
						kColorBoldBlk,
						kColorNrm);
				break;
			case ESector::kThickIce:
				sprintf(buf,
						"%sУ вас под ногами толстый лед.%s\r\n",
						kColorBoldBlu,
						kColorNrm);
				break;
			case ESector::kNormalIce:
				sprintf(buf, "%sУ вас под ногами достаточно толстый лед.%s\r\n",
						kColorBoldBlu, kColorNrm);
				break;
			case ESector::kThinIce:
				sprintf(buf, "%sТоненький ледок вот-вот проломится под вами.%s\r\n",
						kColorBoldCyn, kColorNrm);
				break;
		};
		if (*buf) {
			SendMsgToChar(buf, ch);
		}
	}
	SendMsgToChar("&Y&q", ch);
	MUD::Runestones().ShowRunestone(ch);
	list_obj_to_char(world[ch->in_room]->contents, ch, 0, false);
	list_char_to_char_thing(world[ch->in_room]->people, ch);  //добавим отдельный вызов если моб типа предмет выводим желтым
	SendMsgToChar("&R&q", ch);
	list_char_to_char(world[ch->in_room]->people, ch);
	SendMsgToChar("&Q&n", ch);

	// вход в новую зону
	if (!ch->IsNpc()) {
		ZoneRnum inroom = world[ch->in_room]->zone_rn;
		if (zone_table[world[ch->get_from_room()]->zone_rn].vnum != zone_table[inroom].vnum) {
			if (ch->IsFlagged(EPrf::kShowZoneNameOnEnter))
				print_zone_info(ch);
			if ((ch->GetLevel() < kLvlImmortal) && !ch->get_master())
				++zone_table[inroom].traffic;
			if (zone_table[inroom].first_enter.empty()) {
				zone_table[inroom].first_enter = ch->get_master() ? ch->get_master()->get_name() : ch->get_name();
			}
			if (zone_table[world[ch->get_from_room()]->zone_rn].vnum >= dungeons::kZoneStartDungeons
					&& zone_table[inroom].vnum < dungeons::kZoneStartDungeons) {
				SendMsgToChar("&GВы покинули зазеркалье.\r\n&n", ch);
//				dungeons::SwapObjectDungeon(ch); перенесено на закрытие данжа
			}
		}
	}
}

void show_glow_objs(CharData *ch) {
	unsigned cnt = 0;
	for (ObjData *obj = world[ch->in_room]->contents;
		 obj; obj = obj->get_next_content()) {
		if (obj->has_flag(EObjFlag::kGlow)) {
			++cnt;
			if (cnt > 1) {
				break;
			}
		}
	}
	if (!cnt) return;

	const char *str = cnt > 1 ?
					  "Вы видите очертания каких-то блестящих предметов.\r\n" :
					  "Вы видите очертания какого-то блестящего предмета.\r\n";
	SendMsgToChar(str, ch);
}

void show_extend_room(const char *const description, CharData *ch) {
	int found = false;
	char string[kMaxStringLength], *pos;

	if (!description || !*description)
		return;

	strcpy(string, description);
	if ((pos = strchr(string, '<')))
		*pos = '\0';
	strcpy(buf, string);
	if (pos)
		*pos = '<';

	found = found || paste_description(string, TAG_WINTERNIGHT,
									   (weather_info.season == ESeason::kWinter
										   && (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark)));
	found = found || paste_description(string, TAG_WINTERDAY,
									   (weather_info.season == ESeason::kWinter
										   && (weather_info.sunlight == kSunRise
											   || weather_info.sunlight == kSunLight)));
	found = found || paste_description(string, TAG_SPRINGNIGHT,
									   (weather_info.season == ESeason::kSpring
										   && (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark)));
	found = found || paste_description(string, TAG_SPRINGDAY,
									   (weather_info.season == ESeason::kSpring
										   && (weather_info.sunlight == kSunRise
											   || weather_info.sunlight == kSunLight)));
	found = found || paste_description(string, TAG_SUMMERNIGHT,
									   (weather_info.season == ESeason::kSummer
										   && (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark)));
	found = found || paste_description(string, TAG_SUMMERDAY,
									   (weather_info.season == ESeason::kSummer
										   && (weather_info.sunlight == kSunRise
											   || weather_info.sunlight == kSunLight)));
	found = found || paste_description(string, TAG_AUTUMNNIGHT,
									   (weather_info.season == ESeason::kAutumn
										   && (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark)));
	found = found || paste_description(string, TAG_AUTUMNDAY,
									   (weather_info.season == ESeason::kAutumn
										   && (weather_info.sunlight == kSunRise
											   || weather_info.sunlight == kSunLight)));
	found = found || paste_description(string, TAG_NIGHT,
									   (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark));
	found = found || paste_description(string, TAG_DAY,
									   (weather_info.sunlight == kSunRise || weather_info.sunlight == kSunLight));

	// Trim any LF/CRLF at the end of description
	pos = buf + strlen(buf);
	while (pos > buf && *--pos == '\n') {
		*pos = '\0';
		if (pos > buf && *(pos - 1) == '\r')
			*--pos = '\0';
	}

	SendMsgToChar(buf, ch);
	SendMsgToChar("\r\n", ch);
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 * \return флаг если смотрим в клан-сундук, чтобы после осмотра не смотреть второй раз по look_in_obj
 */
bool look_at_target(CharData *ch, char *arg, int subcmd) {
	int bits, found = false, fnum, i = 0;
	CharData *found_char = nullptr;
	ObjData *found_obj = nullptr;
	char *desc, *what, whatp[kMaxInputLength], where[kMaxInputLength];
	int where_bits = EFind::kObjInventory | EFind::kObjRoom | EFind::kObjEquip | EFind::kCharInRoom | EFind::kObjExtraDesc;

	if (!ch->desc) {
		return false;
	}

	if (!*arg) {
		SendMsgToChar("На что вы так мечтаете посмотреть?\r\n", ch);
		return false;
	}

	half_chop(arg, whatp, where);
	what = whatp;

	if (isname(where, "земля комната room ground"))
		where_bits = EFind::kObjRoom | EFind::kCharInRoom;
	else if (isname(where, "инвентарь inventory"))
		where_bits = EFind::kObjInventory;
	else if (isname(where, "экипировка equipment"))
		where_bits = EFind::kObjEquip;

	// для townportal
	if (isname(whatp, "камень") && MUD::Runestones().ViewRunestone(ch, where_bits)) {
		return false;
	}

	bits = generic_find(what, where_bits, ch, &found_char, &found_obj);
	// Is the target a character?
	if (found_char != nullptr) {
		if (subcmd == kScmdLookHide && !check_moves(ch, kLookhideMoves))
			return false;
		look_at_char(found_char, ch);
		if (ch != found_char) {
			if (subcmd == kScmdLookHide && ch->GetSkill(ESkill::kPry) > 0) {
				fnum = number(1, MUD::Skill(ESkill::kPry).difficulty);
				found = CalcCurrentSkill(ch, ESkill::kPry, found_char);
				TrainSkill(ch, ESkill::kPry, found < fnum, found_char);
				if (!IS_IMMORTAL(ch))
					SetWaitState(ch, 1 * kBattleRound);
				if (found >= fnum && (fnum < 100 || IS_IMMORTAL(ch)) && !IS_IMMORTAL(found_char))
					return false;
			}
			if (CAN_SEE(found_char, ch))
				act("$n оглядел$g вас с головы до пят.", true, ch, nullptr, found_char, kToVict);
			act("$n посмотрел$g на $N3.", true, ch, nullptr, found_char, kToNotVict);
		}
		return false;
	}

	// Strip off "number." from 2.foo and friends.
	if (!(fnum = get_number(&what))) {
		SendMsgToChar("Что осматриваем?\r\n", ch);
		return false;
	}
	// заглянуть в пентаграмму
	i = 0;
	if (isname(whatp, "пентаграмма") && IS_SET(where_bits, EFind::kObjRoom)) {
		RoomRnum to_room = kNowhere;
		bool one_way = false;

		for (const auto &aff : world[ch->in_room]->affected) {
			if (aff->type == ESpell::kPortalTimer) {
				if (aff->bitvector == room_spells::ERoomAffect::kNoPortalExit) {
					SendMsgToChar("Похоже, этого здесь нет!\r\n", ch);
					return false;
				}
				if (++i == fnum) {
					to_room = aff->modifier;
				}
				for (const auto &aff : world[to_room]->affected) {
					if (aff->type == ESpell::kPortalTimer) {
						if (aff->bitvector == room_spells::ERoomAffect::kNoPortalExit) {
							one_way = true;
						}
					}
				}
			}
		}
		if (to_room != kNowhere) {
			const auto r = ch->in_room;
			SendMsgToChar("Приблизившись к пентаграмме, вы осторожно заглянули в нее.\r\n\r\n", ch);
			act("$n0 осторожно заглянул$g в пентаграмму.\r\n", true, ch, nullptr, nullptr, kToRoom);
			if (!one_way) {
				SendMsgToChar("Яркий свет, идущий с противоположного конца прохода, застилает вам глаза.\r\n\r\n", ch);
				return false;
			}
			ch->in_room = to_room;
			look_at_room(ch, 1);
			ch->in_room = r;
			return false;
		}
	}
	i = 0;
	// Does the argument match an extra desc in the room?
	if ((desc = find_exdesc(what, world[ch->in_room]->ex_description)) != nullptr && ++i == fnum) {
		page_string(ch->desc, desc, false);
		return false;
	}

	// If an object was found back in generic_find
	if (bits && (found_obj != nullptr)) {

		if (Clan::ChestShow(found_obj, ch)) {
			return true;
		}
		if (ClanSystem::show_ingr_chest(found_obj, ch)) {
			return true;
		}
		if (Depot::is_depot(found_obj)) {
			Depot::show_depot(ch);
			return true;
		}

		// Собственно изменение. Вместо проверки "if (!found)" юзается проверка
		// наличия описания у объекта, найденного функцией "generic_find"
		if (!(desc = find_exdesc(what, found_obj->get_ex_description())) 
				|| ((found_obj->get_type() == EObjType::kNote) && !found_obj->get_action_description().empty())) {
			show_obj_to_char(found_obj, ch, 5, true, 1);    // Show no-description
		} else {
			SendMsgToChar(desc, ch);
			show_obj_to_char(found_obj, ch, 6, true, 1);    // Find hum, glow etc
		}

		*buf = '\0';
		obj_info(ch, found_obj, buf);
		SendMsgToChar(buf, ch);
	} else
		SendMsgToChar("Похоже, этого здесь нет!\r\n", ch);

	return false;
}

void look_at_char(CharData *i, CharData *ch) {
	int j, found, push_count = 0;
	ObjData *tmp_obj, *push = nullptr;

	if (!ch->desc)
		return;

	if (!i->player_data.description.empty()) {
		if (i->IsNpc())
			SendMsgToChar(ch, " * %s", i->player_data.description.c_str());
		else
			SendMsgToChar(ch, "*\r\n%s*\r\n", AddLeadingStringSpace(i->player_data.description).c_str());
	} else if (!i->IsNpc()) {
		strcpy(buf, "\r\nЭто");
		if (IS_FEMALE(i)) {
			if (GET_HEIGHT(i) <= 151) {
				if (GET_WEIGHT(i) >= 140)
					strcat(buf, " маленькая плотная дамочка.\r\n");
				else if (GET_WEIGHT(i) >= 125)
					strcat(buf, " маленькая женщина.\r\n");
				else
					strcat(buf, " миниатюрная дамочка.\r\n");
			} else if (GET_HEIGHT(i) <= 159) {
				if (GET_WEIGHT(i) >= 145)
					strcat(buf, " невысокая плотная мадам.\r\n");
				else if (GET_WEIGHT(i) >= 130)
					strcat(buf, " невысокая женщина.\r\n");
				else
					strcat(buf, " изящная леди.\r\n");
			} else if (GET_HEIGHT(i) <= 165) {
				if (GET_WEIGHT(i) >= 145)
					strcat(buf, " среднего роста женщина.\r\n");
				else
					strcat(buf, " среднего роста изящная красавица.\r\n");
			} else if (GET_HEIGHT(i) <= 175) {
				if (GET_WEIGHT(i) >= 150)
					strcat(buf, " высокая дородная баба.\r\n");
				else if (GET_WEIGHT(i) >= 135)
					strcat(buf, " высокая стройная женщина.\r\n");
				else
					strcat(buf, " высокая изящная женщина.\r\n");
			} else {
				if (GET_WEIGHT(i) >= 155)
					strcat(buf, " очень высокая крупная дама.\r\n");
				else if (GET_WEIGHT(i) >= 140)
					strcat(buf, " очень высокая стройная женщина.\r\n");
				else
					strcat(buf, " очень высокая худощавая женщина.\r\n");
			}
		} else {
			if (GET_HEIGHT(i) <= 165) {
				if (GET_WEIGHT(i) >= 170)
					strcat(buf, " маленький, похожий на колобок, мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 150)
					strcat(buf, " маленький плотный мужчина.\r\n");
				else
					strcat(buf, " маленький плюгавенький мужичонка.\r\n");
			} else if (GET_HEIGHT(i) <= 175) {
				if (GET_WEIGHT(i) >= 175)
					strcat(buf, " невысокий коренастый крепыш.\r\n");
				else if (GET_WEIGHT(i) >= 160)
					strcat(buf, " невысокий крепкий мужчина.\r\n");
				else
					strcat(buf, " невысокий худощавый мужчина.\r\n");
			} else if (GET_HEIGHT(i) <= 185) {
				if (GET_WEIGHT(i) >= 180)
					strcat(buf, " среднего роста коренастый мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 165)
					strcat(buf, " среднего роста крепкий мужчина.\r\n");
				else
					strcat(buf, " среднего роста худощавый мужчина.\r\n");
			} else if (GET_HEIGHT(i) <= 195) {
				if (GET_WEIGHT(i) >= 185)
					strcat(buf, " высокий крупный мужчина.\r\n");
				else if (GET_WEIGHT(i) >= 170)
					strcat(buf, " высокий стройный мужчина.\r\n");
				else
					strcat(buf, " длинный, худощавый мужчина.\r\n");
			} else {
				if (GET_WEIGHT(i) >= 190)
					strcat(buf, " огромный мужик.\r\n");
				else if (GET_WEIGHT(i) >= 180)
					strcat(buf, " очень высокий, крупный амбал.\r\n");
				else
					strcat(buf, " длиннющий, похожий на жердь мужчина.\r\n");
			}
		}
		SendMsgToChar(buf, ch);
	} else
		act("\r\nНичего необычного в $n5 вы не заметили.", false, i, nullptr, ch, kToVict);

	if (AFF_FLAGGED(i, EAffect::kHaemorrhage)) {
		act("$n покрыт$a кровоточащими ранами!", false, i, nullptr, ch, kToVict);
	}
	if (AFF_FLAGGED(i, EAffect::kLacerations)) {
		act("$n покрыт$a рваными ранами!", false, i, nullptr, ch, kToVict);
	}

	if (AFF_FLAGGED(i, EAffect::kCharmed)
		&& i->get_master() == ch) {
		if (i->low_charm()) {
			act("$n скоро перестанет следовать за вами.", false, i, nullptr, ch, kToVict);
		} else {
			for (const auto &aff : i->affected) {
				if (aff->type == ESpell::kCharm) {
					sprintf(buf,
							IS_POLY(i) ? "$n будут слушаться вас еще %d %s." : "$n будет слушаться вас еще %d %s.",
							aff->duration/2,
							GetDeclensionInNumber(aff->duration/2, EWhat::kHour));
					act(buf, false, i, nullptr, ch, kToVict);
					break;
				}
			}
		}
	}

	if (IS_HORSE(i)
		&& i->get_master() == ch) {
		strcpy(buf, "\r\nЭто ваш скакун. Он ");
		if (GET_HORSESTATE(i) <= 0)
			strcat(buf, "загнан.\r\n");
		else if (GET_HORSESTATE(i) <= 20)
			strcat(buf, "весь в мыле.\r\n");
		else if (GET_HORSESTATE(i) <= 80)
			strcat(buf, "в хорошем состоянии.\r\n");
		else
			strcat(buf, "выглядит совсем свежим.\r\n");
		SendMsgToChar(buf, ch);
	};

	diag_char_to_char(i, ch);

	found = false;
	for (j = 0; !found && j < EEquipPos::kNumEquipPos; j++) {
		if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
			found = true;
	}
	if (found) {
		SendMsgToChar("\r\n", ch);
		act("$n одет$a :", false, i, nullptr, ch, kToVict);
		for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
			if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
				SendMsgToChar(where[j], ch);
				if (i->has_master()
					&& i->IsNpc()) {
					show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i->get_master(), 1);
				} else {
					show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i, 1);
				}
			}
		}
	}

	if (ch != i && (ch->GetSkill(ESkill::kPry) || IS_IMMORTAL(ch))) {
		found = false;
		act("\r\nВы попытались заглянуть в $s ношу:", false, i, nullptr, ch, kToVict);
		for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->get_next_content()) {
			if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 30) < GetRealLevel(ch))) {
				if (!push) {
					push = tmp_obj;
					push_count = 1;
				} else if (!IsObjsStackable(tmp_obj, push)
					|| GET_OBJ_VNUM(push) == 100) {
					show_obj_to_char(push, ch, 1, ch == i, push_count);
					push = tmp_obj;
					push_count = 1;
				} else
					push_count++;
				found = true;
			}
		}
		if (push && push_count)
			show_obj_to_char(push, ch, 1, ch == i, push_count);
		if (!found)
			SendMsgToChar("...и ничего не обнаружили.\r\n", ch);
	}
}

void list_char_to_char_thing(const RoomData::people_t &list, CharData *ch) {
	for (const auto i : list) {
		if (ch != i) {
			if (GET_RACE(i) == ENpcRace::kConstruct) {
				ListOneChar(i, ch, ESkill::kAny);
			}
		}
	}
}

void show_room_affects(CharData *ch, const char *name_affects[], const char *name_self_affects[]) {
	std::ostringstream buffer;
	
	for (const auto &af : world[ch->in_room]->affected) {
		switch (af->type) {
			case ESpell::kLight:
				if (af->caster_id == ch->get_uid() && *name_self_affects[0] != '\0') {
					buffer << name_self_affects[0] << "\r\n";
				} else if (*name_affects[0] != '\0') {
					buffer << name_affects[0] << "\r\n";
				}
				break;
			case ESpell::kDeadlyFog:
				if (af->caster_id == ch->get_uid() && *name_self_affects[1] != '\0') {
					buffer << name_self_affects[1] << "\r\n";
				} else if (*name_affects[1] != '\0') {
					buffer << name_affects[1] << "\r\n";
				}
				break;
			case ESpell::kRuneLabel:                // 1 << 2
				if (af->caster_id == ch->get_uid() && *name_self_affects[2] != '\0') {
					buffer << name_self_affects[2] << "\r\n";
				} else if (*name_affects[2] != '\0') {
					buffer << name_affects[2] << "\r\n";
				}
				break;
			case ESpell::kForbidden:
				if (af->caster_id == ch->get_uid() && *name_self_affects[3] != '\0') {
					buffer << name_self_affects[3] << "\r\n";
				} else if (*name_affects[3] != '\0') {
					buffer << name_affects[3] << "\r\n";
				}
				break;
			case ESpell::kHypnoticPattern:
				if (af->caster_id == ch->get_uid() && *name_self_affects[4] != '\0') {
					buffer << name_self_affects[4] << "\r\n";
				} else if (*name_affects[4] != '\0') {
					buffer << name_affects[4] << "\r\n";
				}
				break;
			case ESpell::kBlackTentacles:
				if (af->caster_id == ch->get_uid() && *name_self_affects[5] != '\0') {
					buffer << name_self_affects[5] << "\r\n";
				} else if (*name_affects[5] != '\0') {
					buffer << name_affects[5] << "\r\n";
				}
				break;
			case ESpell::kMeteorStorm:
				if (af->caster_id == ch->get_uid() && *name_self_affects[6] != '\0') {
					buffer << name_self_affects[6] << "\r\n";
				} else if (*name_affects[6] != '\0') {
					buffer << name_affects[6] << "\r\n";
				}
				break;
			case ESpell::kThunderstorm:
				if (af->caster_id == ch->get_uid() && *name_self_affects[7] != '\0') {
					buffer << name_self_affects[7] << "\r\n";
				} else if (*name_affects[7] != '\0') {
						buffer << name_affects[7] << "\r\n";
				}
				break;
			case ESpell::kPortal:
				//выводится в look_at_room
				break;
			default: 
				log("SYSERR: Unknown room (#%d) spell type: %d", world[ch->in_room]->vnum, to_underlying(af->type));
				break;
		}
	}

	auto affects = buffer.str();
	if (!affects.empty()) {
		affects.append("\r\n");
		SendMsgToChar(affects.c_str(), ch);
	}
}

int paste_description(char *string, const char *tag, int need) {
	if (!*string || !*tag) {
		return (false);
	}

	char *pos = str_str(string, tag);
	if (!pos) {
		return false;
	}
	if (!need) {
/*		*pos = '\0';
		if ((pos = str_str(pos + 1, tag)))
			strcat(buf, pos + strlen(tag));
*/
		return false;
	}

	for (; *pos && *pos != '>'; pos++);

	if (*pos) {
		pos++;
	}

	if (*pos == 'R') {
		pos++;
		buf[0] = '\0';
	}

	strcat(buf, pos);
	pos = str_str(buf, tag);
	if (pos) {
		*pos = '\0';
	}

	return (true);
}

void do_auto_exits(CharData *ch) {
	int door, slen = 0;
	char buf[kMaxInputLength];

	*buf = '\0';
	for (door = 0; door < EDirection::kMaxDirNum; door++) {
		// Наконец-то добавлена отрисовка в автовыходах закрытых дверей
		if (EXIT(ch, door) && EXIT(ch, door)->to_room() != kNowhere) {
			if (EXIT_FLAGGED(EXIT(ch, door), EExitFlag::kClosed)) {
				slen += sprintf(buf + slen, "(%c) ", LOWER(*dirs[door]));
			} else if (!EXIT_FLAGGED(EXIT(ch, door), EExitFlag::kHidden)) {
				if (world[EXIT(ch, door)->to_room()]->zone_rn == world[ch->in_room]->zone_rn) {
					slen += sprintf(buf + slen, "%c ", LOWER(*dirs[door]));
				} else {
					slen += sprintf(buf + slen, "%c ", UPPER(*dirs[door]));
				}
			}
		}
	}
	sprintf(buf2, "%s[ Exits: %s]%s\r\n", kColorCyn, *buf ? buf : "None! ", kColorNrm);

	SendMsgToChar(buf2, ch);
}

void list_obj_to_char(ObjData *list, CharData *ch, int mode, int show) {
	ObjData *i, *push = nullptr;
	bool found = false;
	int push_count = 0;
	std::ostringstream buffer;
	long cost = 0, count = 0;

	bool clan_chest = false;
	if (mode == 1 && (show == 3 || show == 4)) {
		clan_chest = true;
	}

	for (i = list; i; i = i->get_next_content()) {
		if (CAN_SEE_OBJ(ch, i)) {
			if (!push) {
				push = i;
				push_count = 1;
			} else if ((!IsObjsStackable(i, push))
				|| (quest_item(i))) {
				if (clan_chest) {
					buffer << show_obj_to_char(push, ch, mode, show, push_count);
					count += push_count;
					cost += push->get_rent_on() * push_count;
				} else
					show_obj_to_char(push, ch, mode, show, push_count);
				push = i;
				push_count = 1;
			} else
				push_count++;
			found = true;
		}
	}
	if (push && push_count) {
		if (clan_chest) {
			buffer << show_obj_to_char(push, ch, mode, show, push_count);
			count += push_count;
			cost += push->get_rent_on() * push_count;
		} else
			show_obj_to_char(push, ch, mode, show, push_count);
	}
	if (!found && show) {
		if (show == 1)
			SendMsgToChar(" Внутри ничего нет.\r\n", ch);
		else if (show == 2)
			SendMsgToChar(" Вы ничего не несете.\r\n", ch);
		else if (show == 3) {
			SendMsgToChar(" Пусто...\r\n", ch);
			return;
		}
	}
	if (clan_chest)
		page_string(ch->desc, buffer.str());
}

void list_char_to_char(const RoomData::people_t &list, CharData *ch) {
	for (const auto i : list) {
		if (ch != i) {
			if (HERE(i) && (GET_RACE(i) != ENpcRace::kConstruct)
				&& (CAN_SEE(ch, i)
					|| awaking(i, kAwHide | kAwInvis | kAwCamouflage))) {
				ListOneChar(i, ch, ESkill::kAny);
			} else if (is_dark(i->in_room)
				&& i->in_room == ch->in_room
				&& !CAN_SEE_IN_DARK(ch)
				&& AFF_FLAGGED(i, EAffect::kInfravision)) {
				SendMsgToChar("Пара светящихся глаз смотрит на вас.\r\n", ch);
			}
		}
	}
}

void look_in_direction(CharData *ch, int dir, int info_is) {
	int count = 0, probe, percent;
	RoomData::exit_data_ptr rdata;

	if (CAN_GO(ch, dir)
		|| (EXIT(ch, dir)
			&& EXIT(ch, dir)->to_room() != kNowhere)) {
		rdata = EXIT(ch, dir);
		count += sprintf(buf, "%s%s:%s ", kColorYel, dirs_rus[dir], kColorNrm);
		if (EXIT_FLAGGED(rdata, EExitFlag::kClosed)) {
			if (rdata->keyword) {
				count += sprintf(buf + count, " закрыто (%s).\r\n", rdata->keyword);
			} else {
				count += sprintf(buf + count, " закрыто (вероятно дверь).\r\n");
			}

			const int skill_pick = ch->GetSkill(ESkill::kPickLock);
			if (EXIT_FLAGGED(rdata, EExitFlag::kLocked) && skill_pick) {
				if (EXIT_FLAGGED(rdata, EExitFlag::kPickroof)) {
					count += sprintf(buf + count - 2,
									 "%s вы никогда не сможете ЭТО взломать!%s\r\n",
									 kColorBoldCyn,
									 kColorNrm);
				} else if (EXIT_FLAGGED(rdata, EExitFlag::kBrokenLock)) {
					count += sprintf(buf + count - 2, "%s Замок сломан... %s\r\n", kColorRed, kColorNrm);
				} else {
					const PickProbabilityInformation &pbi = get_pick_probability(ch, rdata->lock_complexity);
					count += sprintf(buf + count - 2, "%s\r\n", pbi.text.c_str());
				}
			}

			SendMsgToChar(buf, ch);
			return;
		}

		if (is_dark(rdata->to_room())) {
			count += sprintf(buf + count, " слишком темно.\r\n");
			SendMsgToChar(buf, ch);
			if (info_is & EXIT_SHOW_LOOKING) {
				SendMsgToChar("&R&q", ch);
				count = 0;
				for (const auto tch : world[rdata->to_room()]->people) {
					percent = number(1, MUD::Skill(ESkill::kLooking).difficulty);
					probe = CalcCurrentSkill(ch, ESkill::kLooking, tch);
					TrainSkill(ch, ESkill::kLooking, probe >= percent, tch);
					if (HERE(tch) && INVIS_OK(ch, tch) && probe >= percent
						&& (percent < 100 || IS_IMMORTAL(ch))) {
						// Если моб не вещь и смотрящий не им
						if (GET_RACE(tch) != ENpcRace::kConstruct || IS_IMMORTAL(ch)) {
							ListOneChar(tch, ch, ESkill::kLooking);
							count++;
						}
					}
				}

				if (!count) {
					SendMsgToChar("Вы ничего не смогли разглядеть!\r\n", ch);
				}
				SendMsgToChar("&Q&n", ch);
			}
		} else {
			if (!rdata->general_description.empty()) {
				count += sprintf(buf + count, "%s\r\n", rdata->general_description.c_str());
			} else {
				count += sprintf(buf + count, "%s\r\n", world[rdata->to_room()]->name);
			}
			SendMsgToChar(buf, ch);
			SendMsgToChar("&R&q", ch);
			list_char_to_char(world[rdata->to_room()]->people, ch);
			SendMsgToChar("&Q&n", ch);
		}
	} else if (info_is & EXIT_SHOW_WALL)
		SendMsgToChar("И что вы там мечтаете увидеть?\r\n", ch);
}

bool quest_item(ObjData *obj) {
	if ((obj->has_flag(EObjFlag::kNodecay)) && (!(CAN_WEAR(obj, EWearFlag::kTake)))) {
		return true;
	}
	return false;
}

void look_in_obj(CharData *ch, char *arg) {
	ObjData *obj = nullptr;
	CharData *dummy = nullptr;
	char whatp[kMaxInputLength], where[kMaxInputLength];
	int amt, bits;
	int where_bits = EFind::kObjInventory | EFind::kObjRoom | EFind::kObjEquip;

	if (!*arg)
		SendMsgToChar("Смотреть во что?\r\n", ch);
	else
		half_chop(arg, whatp, where);

	if (isname(where, "земля комната room ground"))
		where_bits = EFind::kObjRoom;
	else if (isname(where, "инвентарь inventory"))
		where_bits = EFind::kObjInventory;
	else if (isname(where, "экипировка equipment"))
		where_bits = EFind::kObjEquip;

	bits = generic_find(arg, where_bits, ch, &dummy, &obj);

	if ((obj == nullptr) || !bits) {
		sprintf(buf, "Вы не видите здесь '%s'.\r\n", arg);
		SendMsgToChar(buf, ch);
	} else if (obj->get_type() != EObjType::kLiquidContainer
		&& obj->get_type() != EObjType::kFountain
		&& obj->get_type() != EObjType::kContainer) {
		SendMsgToChar("Ничего в нем нет!\r\n", ch);
	} else {
		if (Clan::ChestShow(obj, ch)) {
			return;
		}
		if (ClanSystem::show_ingr_chest(obj, ch)) {
			return;
		}
		if (Depot::is_depot(obj)) {
			Depot::show_depot(ch);
			return;
		}

		if (obj->get_type() == EObjType::kContainer) {
			if (OBJVAL_FLAGGED(obj, EContainerFlag::kShutted)) {
				act("Закрыт$A.", false, ch, obj, nullptr, kToChar);
				const int skill_pick = ch->GetSkill(ESkill::kPickLock);
				int count = sprintf(buf, "Заперт%s.", GET_OBJ_SUF_6(obj));
				if (OBJVAL_FLAGGED(obj, EContainerFlag::kLockedUp) && skill_pick) {
					if (OBJVAL_FLAGGED(obj, EContainerFlag::kUncrackable))
						count += sprintf(buf + count,
										 "%s Вы никогда не сможете ЭТО взломать!%s\r\n",
										 kColorBoldCyn,
										 kColorNrm);
					else if (OBJVAL_FLAGGED(obj, EContainerFlag::kLockIsBroken))
						count += sprintf(buf + count, "%s Замок сломан... %s\r\n", kColorRed, kColorNrm);
					else {
						const PickProbabilityInformation &pbi = get_pick_probability(ch, GET_OBJ_VAL(obj, 3));
						count += sprintf(buf + count, "%s\r\n", pbi.text.c_str());
					}
					SendMsgToChar(buf, ch);
				}
			} else {
				SendMsgToChar(OBJN(obj, ch, ECase::kNom), ch);
				switch (bits) {
					case EFind::kObjInventory: SendMsgToChar("(в руках)\r\n", ch);
						break;
					case EFind::kObjRoom: SendMsgToChar("(на земле)\r\n", ch);
						break;
					case EFind::kObjEquip: SendMsgToChar("(в амуниции)\r\n", ch);
						break;
					default: SendMsgToChar("(неведомо где)\r\n", ch);
						break;
				}
				if (!obj->get_contains())
					SendMsgToChar(" Внутри ничего нет.\r\n", ch);
				else {
					if (GET_OBJ_VAL(obj, 0) > 0 && bits != EFind::kObjRoom) {
						/* amt - индекс массива из 6 элементов (0..5) с описанием наполненности
						   с помощью нехитрых мат. преобразований мы получаем соотношение веса и максимального объема контейнера,
						   выраженные числами от 0 до 5. (причем 5 будет лишь при полностью полном контейнере)
						*/
						amt = std::clamp((obj->get_weight() * 100) / (GET_OBJ_VAL(obj, 0) * 20), 0, 5);
						sprintf(buf, "Заполнен%s содержимым %s:\r\n", GET_OBJ_SUF_6(obj), fullness[amt]);
						SendMsgToChar(buf, ch);
					}
					list_obj_to_char(obj->get_contains(), ch, 1, bits != EFind::kObjRoom);
				}
			}
		} else {
			// item must be a fountain or drink container
			SendMsgToChar(ch, "%s.\r\n", drinkcon::daig_filling_drink(obj, ch));

		}
	}
}

void skip_hide_on_look(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kHide) &&
		((!ch->GetSkill(ESkill::kPry) ||
			((number(1, 100) -
				CalcCurrentSkill(ch, ESkill::kPry, nullptr) - 2 * (ch->get_wis() - 9)) > 0)))) {
		RemoveAffectFromChar(ch, ESpell::kHide);
		AFF_FLAGS(ch).unset(EAffect::kHide);
		SendMsgToChar("Вы прекратили прятаться.\r\n", ch);
		act("$n прекратил$g прятаться.", false, ch, nullptr, nullptr, kToRoom);
	}
}

// mode 1 show_state 3 для хранилище (4 - хранилище ингров)
const char *show_obj_to_char(ObjData *object, CharData *ch, int mode, int show_state, int how) {
	*buf = '\0';
	if ((mode < 5) && (ch->IsFlagged(EPrf::kRoomFlags) || InTestZone(ch)))
		sprintf(buf, "[%5d] ", GET_OBJ_VNUM(object));

	if (mode == 0
		&& !object->get_description().empty()) {
		strcat(buf, object->get_description().c_str());
		strcat(buf, char_get_custom_label(object, ch).c_str());
	} else if (!object->get_short_description().empty() && ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4))) {
		strcat(buf, object->get_short_description().c_str());
		strcat(buf, char_get_custom_label(object, ch).c_str());
	} else if (mode == 5) {
		if (object->get_type() == EObjType::kNote) {
			if (!object->get_action_description().empty()) {
				strcpy(buf, "Вы прочитали следующее :\r\n\r\n");
				strcat(buf, AddLeadingStringSpace(object->get_action_description()).c_str());
				strcat(buf, "\r\n");
				page_string(ch->desc, buf, 1);
			} else {
				SendMsgToChar("Чисто.\r\n", ch);
			}
			return nullptr;
		} else if (object->get_type() == EObjType::kBandage) {
			strcpy(buf, "Бинты для перевязки ран ('перевязать').\r\n");
			snprintf(buf2, kMaxStringLength, "Осталось применений: %d, восстановление: %d",
					 object->get_weight(), GET_OBJ_VAL(object, 0) * 10);
			strcat(buf, buf2);
		} else if (object->get_type() != EObjType::kLiquidContainer) {
			strcpy(buf, "Вы не видите ничего необычного.");
		} else        // ITEM_TYPE == kLiquidContainer||FOUNTAIN
		{
			strcpy(buf, "Это емкость для жидкости.");
		}
	}

	if (show_state && show_state != 3 && show_state != 4) {
		*buf2 = '\0';
		if (mode == 1 && how <= 1) {
			if (object->get_type() == EObjType::kLightSource) {
				if (GET_OBJ_VAL(object, 2) == -1)
					strcpy(buf2, " (вечный свет)");
				else if (GET_OBJ_VAL(object, 2) == 0)
					sprintf(buf2, " (погас%s)", GET_OBJ_SUF_4(object));
				else
					sprintf(buf2, " (%d %s)",
							GET_OBJ_VAL(object, 2), GetDeclensionInNumber(GET_OBJ_VAL(object, 2), EWhat::kHour));
			} else {
				if (object->timed_spell().IsSpellPoisoned() != ESpell::kUndefined) {
					sprintf(buf2, " %s*%s%s", kColorGrn,
							kColorNrm, diag_obj_to_char(object, 1));
				} else {
					sprintf(buf2, " %s ", diag_obj_to_char(object, 1));
					if (object->get_type() == EObjType::kLiquidContainer) {
						char *tmp = drinkcon::daig_filling_drink(object, ch);
						char tmp2[128];
						*tmp = LOWER(*tmp);
						sprintf(tmp2, "(%s)", tmp);
						strcat(buf2, tmp2);
					}
				}
			}
			if ((object->get_type() == EObjType::kContainer)
				&& !OBJVAL_FLAGGED(object, EContainerFlag::kShutted)) // если закрыто, содержимое не показываем
			{
				if (object->get_contains()) {
					strcat(buf2, " (есть содержимое)");
				} else {
					if (GET_OBJ_VAL(object, 3) < 1) // есть ключ для открытия, пустоту не показываем2
						sprintf(buf2 + strlen(buf2), " (пуст%s)", GET_OBJ_SUF_6(object));
				}
			}
			if ((object->get_type() == EObjType::kNote) && !object->get_action_description().empty()) {
					strcat(buf2, " (что-то накарябано)");
			}
		} else if (mode >= 2 && how <= 1) {
			std::string obj_name = OBJN(object, ch, ECase::kNom);
			obj_name[0] = UPPER(obj_name[0]);
			if (object->get_type() == EObjType::kLightSource) {
				if (GET_OBJ_VAL(object, 2) == -1) {
					sprintf(buf2, "\r\n%s дает вечный свет.", obj_name.c_str());
				} else if (GET_OBJ_VAL(object, 2) == 0) {
					sprintf(buf2, "\r\n%s погас%s.", obj_name.c_str(), GET_OBJ_SUF_4(object));
				} else {
					sprintf(buf2, "\r\n%s будет светить %d %s.", obj_name.c_str(), GET_OBJ_VAL(object, 2),
							GetDeclensionInNumber(GET_OBJ_VAL(object, 2), EWhat::kHour));
				}
			} else if (object->get_current_durability() < object->get_maximum_durability()) {
				sprintf(buf2, "\r\n%s %s.", obj_name.c_str(), diag_obj_to_char(object, 2));
			}
		}
		strcat(buf, buf2);
	}
	if (how > 1) {
		sprintf(buf + strlen(buf), " [%d]", how);
	}
	if (mode != 3 && how <= 1) {
		if (object->has_flag(EObjFlag::kInvisible)) {
			sprintf(buf2, " (невидим%s)", GET_OBJ_SUF_6(object));
			strcat(buf, buf2);
		}
		if (object->has_flag(EObjFlag::kBless)
			&& AFF_FLAGGED(ch, EAffect::kDetectAlign))
			strcat(buf, " ..голубая аура!");
		if (object->has_flag(EObjFlag::kMagic)
			&& AFF_FLAGGED(ch, EAffect::kDetectMagic))
			strcat(buf, " ..желтая аура!");
		if (object->has_flag(EObjFlag::kPoisoned)
			&& AFF_FLAGGED(ch, EAffect::kDetectPoison)) {
			sprintf(buf2, "..отравлен%s!", GET_OBJ_SUF_6(object));
			strcat(buf, buf2);
		}
		if (object->has_flag(EObjFlag::kGlow))
			strcat(buf, " ..блестит!");
		if (object->has_flag(EObjFlag::kHum) && !AFF_FLAGGED(ch, EAffect::kDeafness))
			strcat(buf, " ..шумит!");
		if (object->has_flag(EObjFlag::kFire))
			strcat(buf, " ..горит!");
		if (object->has_flag(EObjFlag::kBloody)) {
			sprintf(buf2, " %s..покрыт%s кровью!%s", kColorBoldRed, GET_OBJ_SUF_6(object), kColorNrm);
			strcat(buf, buf2);
		}
	}

	if (mode == 1) {
		// клан-сундук, выводим список разом постранично
		if (show_state == 3) {
			sprintf(buf + strlen(buf), " [%d %s]\r\n",
					object->get_rent_on() * kClanStorehouseCoeff / 100,
					GetDeclensionInNumber(object->get_rent_on() * kClanStorehouseCoeff / 100, EWhat::kMoneyA));
			return buf;
		}
			// ингры
		else if (show_state == 4) {
			sprintf(buf + strlen(buf), " [%d %s]\r\n", object->get_rent_off(),
					GetDeclensionInNumber(object->get_rent_off(), EWhat::kMoneyA));
			return buf;
		}
	}

	strcat(buf, "\r\n");
	if (mode >= 5) {
		strcat(buf, diag_weapon_to_char(object, true));
		strcat(buf, diag_armor_type_to_char(object).c_str());
		strcat(buf, diag_timer_to_char(object));
		strcat(buf, "\r\n");
		//strcat(buf, diag_uses_to_char(object, ch)); // commented by WorM перенес в obj_info чтобы заряды рун было видно на базаре/ауке
		strcat(buf, object->diag_ts_to_char().c_str());
	}
	page_string(ch->desc, buf, true);
	return nullptr;
}

void print_zone_info(CharData *ch) {
	ZoneData *zone = &zone_table[world[ch->in_room]->zone_rn];
	std::stringstream out;
	out << "\r\n" << zone->name;

	bool delim = false;
	if (!zone->is_town) {
		delim = put_delim(out, delim);
		out << "средний уровень: " << zone->mob_level;
	}
	if (zone->group > 1) {
		delim = put_delim(out, delim);
		out << "групповая на " << zone->group
			<< " " << GetDeclensionInNumber(zone->group, EWhat::kPeople);
	}
	if (delim) {
		out << ")";
	}
	out << ".\r\n";

	SendMsgToChar(out.str(), ch);
}

bool put_delim(std::stringstream &out, bool delim) {
	if (!delim) {
		out << " (";
	} else {
		out << ", ";
	}
	return true;
}

char *find_exdesc(const char *word, const ExtraDescription::shared_ptr &list) {
	for (auto i = list; i; i = i->next) {
		if (isname(word, i->keyword)) {
			return i->description;
		}
	}

	return nullptr;
}

/**
* При чтении писем и осмотре чара в его описании подставляем в начало каждой строки пробел
* (для дурных тригов), пользуясь случаем передаю привет проне!
*/
std::string AddLeadingStringSpace(const std::string& text) {
	if (!text.empty()) {
		std::string tmp(" ");
		tmp += text;
		utils::ReplaceAll(tmp, "\n", "\n ");
		return tmp;
	}
	return "";
}

char *diag_obj_to_char(ObjData *obj, int mode) {
	static char out_str[80] = "\0";
	const char *color;
	int percent;

	if (obj->get_maximum_durability() > 0)
		percent = 100 * obj->get_current_durability() / obj->get_maximum_durability();
	else
		percent = -1;

	if (percent >= 100) {
		percent = 7;
		color = kColorWht;
	} else if (percent >= 90) {
		percent = 6;
		color = kColorBoldGrn;
	} else if (percent >= 75) {
		percent = 5;
		color = kColorGrn;
	} else if (percent >= 50) {
		percent = 4;
		color = kColorBoldYel;
	} else if (percent >= 30) {
		percent = 3;
		color = kColorBoldRed;
	} else if (percent >= 15) {
		percent = 2;
		color = kColorBoldRed;
	} else if (percent > 0) {
		percent = 1;
		color = kColorRed;
	} else {
		percent = 0;
		color = kColorBoldBlk;
	}

	if (mode == 1)
		sprintf(out_str, " %s<%s>%s", color, ObjState[percent][0], kColorNrm);
	else if (mode == 2)
		strcpy(out_str, ObjState[percent][1]);
	return out_str;
}

void diag_char_to_char(CharData *i, CharData *ch) {
	int percent;

	if (i->get_real_max_hit() > 0)
		percent = (100 * i->get_hit()) / i->get_real_max_hit();
	else
		percent = -1;    // How could MAX_HIT be < 1??

	strcpy(buf, PERS(i, ch, 0));
	utils::CAP(buf);

	if (percent >= 100) {
		sprintf(buf2, " невредим%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 90) {
		sprintf(buf2, " слегка поцарапан%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 75) {
		sprintf(buf2, " легко ранен%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 50) {
		sprintf(buf2, " ранен%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 30) {
		sprintf(buf2, " тяжело ранен%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 15) {
		sprintf(buf2, " смертельно ранен%s", GET_CH_SUF_6(i));
		strcat(buf, buf2);
	} else if (percent >= 0)
		strcat(buf, " в ужасном состоянии");
	else
		strcat(buf, " умирает");

	if (!i->IsOnHorse())
		switch (i->GetPosition()) {
			case EPosition::kPerish: strcat(buf, ".");
				break;
			case EPosition::kIncap: strcat(buf, IS_POLY(i) ? ", лежат без сознания." : ", лежит без сознания.");
				break;
			case EPosition::kStun: strcat(buf, IS_POLY(i) ? ", лежат в обмороке." : ", лежит в обмороке.");
				break;
			case EPosition::kSleep: strcat(buf, IS_POLY(i) ? ", спят." : ", спит.");
				break;
			case EPosition::kRest: strcat(buf, IS_POLY(i) ? ", отдыхают." : ", отдыхает.");
				break;
			case EPosition::kSit: strcat(buf, IS_POLY(i) ? ", сидят." : ", сидит.");
				break;
			case EPosition::kStand: strcat(buf, IS_POLY(i) ? ", стоят." : ", стоит.");
				break;
			case EPosition::kFight:
				if (i->GetEnemy())
					strcat(buf, IS_POLY(i) ? ", сражаются." : ", сражается.");
				else
					strcat(buf, IS_POLY(i) ? ", махают кулаками." : ", махает кулаками.");
				break;
			default: return;
				break;
		}
	else
		strcat(buf, IS_POLY(i) ? ", сидят верхом." : ", сидит верхом.");

	if (AFF_FLAGGED(ch, EAffect::kDetectPoison))
		if (AFF_FLAGGED(i, EAffect::kPoisoned)) {
			sprintf(buf2, " (отравлен%s)", GET_CH_SUF_6(i));
			strcat(buf, buf2);
		}

	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

}

//ф-ция вывода доп инфы об объекте
//buf это буфер в который дописывать инфу, в нем уже может быть что-то иначе надо перед вызовом присвоить *buf='\0'
void obj_info(CharData *ch, ObjData *obj, char buf[kMaxStringLength]) {
	int j;
	if (CanUseFeat(ch, EFeat::kSkilledTrader) || ch->IsFlagged(EPrf::kHolylight) || ch->GetSkill(ESkill::kJewelry)) {
		sprintf(buf + strlen(buf), "Материал : %s", kColorCyn);
		sprinttype(obj->get_material(), material_name, buf + strlen(buf));
		sprintf(buf + strlen(buf), "\r\n%s", kColorNrm);
	}

	if (obj->get_type() == EObjType::kMagicIngredient
		&& (CanUseFeat(ch, EFeat::kHerbalist)
			|| ch->IsFlagged(EPrf::kHolylight))) {
		for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT) && j <= top_imtypes;) {
			j++;
		}
		sprintf(buf + strlen(buf), "Это ингредиент вида '%s'.\r\n", imtypes[j].name);
		const int imquality = GET_OBJ_VAL(obj, IM_POWER_SLOT);
		if (GetRealLevel(ch) >= imquality) {
			sprintf(buf + strlen(buf), "Качество ингредиента ");
			if (imquality > 25)
				strcat(buf + strlen(buf), "наилучшее.\r\n");
			else if (imquality > 20)
				strcat(buf + strlen(buf), "отличное.\r\n");
			else if (imquality > 15)
				strcat(buf + strlen(buf), "очень хорошее.\r\n");
			else if (imquality > 10)
				strcat(buf + strlen(buf), "выше среднего.\r\n");
			else if (imquality > 5)
				strcat(buf + strlen(buf), "весьма посредственное.\r\n");
			else
				strcat(buf + strlen(buf), "хуже не бывает.\r\n");
		} else {
			strcat(buf + strlen(buf), "Вы не в состоянии определить качество этого ингредиента.\r\n");
		}
	}

	//|| EPrf::FLAGGED(ch, EPrf::HOLYLIGHT)
	if (CanUseFeat(ch, EFeat::kJeweller)) {
		sprintf(buf + strlen(buf), "Слоты : %s", kColorCyn);
		if (obj->has_flag(EObjFlag::kHasThreeSlots)) {
			strcat(buf, "доступно 3 слота\r\n");
		} else if (obj->has_flag(EObjFlag::kHasTwoSlots)) {
			strcat(buf, "доступно 2 слота\r\n");
		} else if (obj->has_flag(EObjFlag::kHasOneSlot)) {
			strcat(buf, "доступен 1 слот\r\n");
		} else {
			strcat(buf, "нет слотов\r\n");
		}
		sprintf(buf + strlen(buf), "\r\n%s", kColorNrm);
	}
	if (AUTH_CUSTOM_LABEL(obj, ch) && obj->get_custom_label()->text_label) {
		if (obj->get_custom_label()->clan_abbrev) {
			strcat(buf, "Метки дружины: ");
		} else {
			strcat(buf, "Ваши метки: ");
		}
		sprintf(buf + strlen(buf), "%s\r\n", obj->get_custom_label()->text_label);
	}
	sprintf(buf + strlen(buf), "%s", diag_uses_to_char(obj, ch));
	sprintf(buf + strlen(buf), "%s", diag_shot_to_char(obj, ch));
	if (GET_OBJ_VNUM(obj) >= DUPLICATE_MINI_SET_VNUM) {
		sprintf(buf + strlen(buf), "Светится белым сиянием.\r\n");
	}

	if (((obj->get_type() == EObjType::kLiquidContainer)
		&& (GET_OBJ_VAL(obj, 1) > 0))
		|| (obj->get_type() == EObjType::kFood)) {
		sprintf(buf1, "Качество: %s\r\n", drinkcon::diag_liquid_timer(obj));
		strcat(buf, buf1);
	}
}

void ListOneChar(CharData *i, CharData *ch, ESkill mode) {
	int sector = ESector::kCity;
	int n;
	char aura_txt[200];
	const char *positions[] =
		{
			"лежит здесь, мертвый. ",
			"лежит здесь, при смерти. ",
			"лежит здесь, без сознания. ",
			"лежит здесь, в обмороке. ",
			"спит здесь. ",
			"отдыхает здесь. ",
			"сидит здесь. ",
			"СРАЖАЕТСЯ! ",
			"стоит здесь. "
		};

	// Здесь и далее при использовании IS_POLY() - патч для отображения позиций мобов типа "они" -- Ковшегуб
	const char *poly_positions[] =
		{
			"лежат здесь, мертвые. ",
			"лежат здесь, при смерти. ",
			"лежат здесь, без сознания. ",
			"лежат здесь, в обмороке. ",
			"спят здесь. ",
			"отдыхают здесь. ",
			"сидят здесь. ",
			"СРАЖАЮТСЯ! ",
			"стоят здесь. "
		};

	if (IS_HORSE(i) && i->get_master()->IsOnHorse()) {
		if (ch == i->get_master()) {
			if (!IS_POLY(i)) {
				act("$N несет вас на своей спине.", false, ch, nullptr, i, kToChar);
			} else {
				act("$N несут вас на своей спине.", false, ch, nullptr, i, kToChar);
			}
		}

		return;
	}

	if (mode == ESkill::kLooking) {
		if (HERE(i) && INVIS_OK(ch, i) && GetRealLevel(ch) >= (i->IsNpc() ? 0 : GET_INVIS_LEV(i))) {
			if (GET_RACE(i) == ENpcRace::kConstruct && IS_IMMORTAL(ch)) {
				sprintf(buf, "Вы разглядели %s.(предмет)\r\n", GET_PAD(i, 3));
			} else {
				sprintf(buf, "Вы разглядели %s.\r\n", GET_PAD(i, 3));
			}
			SendMsgToChar(buf, ch);
		}
		return;
	}

	Bitvector mode_flags{0};
	if (!CAN_SEE(ch, i)) {
		mode_flags =
			check_awake(i, kAcheckAffects | kAcheckLight | kAcheckHumming | kAcheckGlowing | kAcheckWeight);
		*buf = 0;
		if (IS_SET(mode_flags, kAcheckAffects)) {
			REMOVE_BIT(mode_flags, kAcheckAffects);
			sprintf(buf + strlen(buf), "магический ореол%s", mode_flags ? ", " : " ");
		}
		if (IS_SET(mode_flags, kAcheckLight)) {
			REMOVE_BIT(mode_flags, kAcheckLight);
			sprintf(buf + strlen(buf), "яркий свет%s", mode_flags ? ", " : " ");
		}
		if (IS_SET(mode_flags, kAcheckGlowing)
			&& IS_SET(mode_flags, kAcheckHumming)
			&& !AFF_FLAGGED(ch, EAffect::kDeafness)) {
			REMOVE_BIT(mode_flags, kAcheckGlowing);
			REMOVE_BIT(mode_flags, kAcheckHumming);
			sprintf(buf + strlen(buf), "шум и блеск экипировки%s", mode_flags ? ", " : " ");
		}
		if (IS_SET(mode_flags, kAcheckGlowing)) {
			REMOVE_BIT(mode_flags, kAcheckGlowing);
			sprintf(buf + strlen(buf), "блеск экипировки%s", mode_flags ? ", " : " ");
		}
		if (IS_SET(mode_flags, kAcheckHumming)
			&& !AFF_FLAGGED(ch, EAffect::kDeafness)) {
			REMOVE_BIT(mode_flags, kAcheckHumming);
			sprintf(buf + strlen(buf), "шум экипировки%s", mode_flags ? ", " : " ");
		}
		if (IS_SET(mode_flags, kAcheckWeight)
			&& !AFF_FLAGGED(ch, EAffect::kDeafness)) {
			REMOVE_BIT(mode_flags, kAcheckWeight);
			sprintf(buf + strlen(buf), "бряцание металла%s", mode_flags ? ", " : " ");
		}
		strcat(buf, "выдает чье-то присутствие.\r\n");
		SendMsgToChar(utils::CAP(buf), ch);
		return;
	}

	if (i->IsNpc()
		&& !i->player_data.long_descr.empty()
		&& i->GetPosition() == GET_DEFAULT_POS(i)
		&& ch->in_room == i->in_room
		&& !AFF_FLAGGED(i, EAffect::kCharmed)
		&& !IS_HORSE(i)) {
		*buf = '\0';
		if (ch->IsFlagged(EPrf::kRoomFlags) || InTestZone(ch)) {
			sprintf(buf, "[%5d] ", GET_MOB_VNUM(i));
		}

		if (AFF_FLAGGED(ch, EAffect::kDetectMagic)
			&& !AFF_FLAGGED(ch, EAffect::kDetectAlign)) {
			if (AFF_FLAGGED(i, EAffect::kForcesOfEvil)) {
				strcat(buf, "(черная аура) ");
			}
		}
		if (AFF_FLAGGED(ch, EAffect::kDetectAlign)) {
			if (i->IsNpc()) {
				if (NPC_FLAGGED(i, ENpcFlag::kAirCreature))
					sprintf(buf + strlen(buf), "%s(аура воздуха)%s ",
							kColorBoldBlu, kColorBoldRed);
				else if (NPC_FLAGGED(i, ENpcFlag::kWaterCreature))
					sprintf(buf + strlen(buf), "%s(аура воды)%s ",
							kColorBoldCyn, kColorBoldRed);
				else if (NPC_FLAGGED(i, ENpcFlag::kFireCreature))
					sprintf(buf + strlen(buf), "%s(аура огня)%s ",
							kColorBoldMag, kColorBoldRed);
				else if (NPC_FLAGGED(i, ENpcFlag::kEarthCreature))
					sprintf(buf + strlen(buf), "%s(аура земли)%s ",
							kColorBoldGrn, kColorBoldRed);
			}
		}
		if (AFF_FLAGGED(i, EAffect::kInvisible))
			sprintf(buf + strlen(buf), "(невидим%s) ", GET_CH_SUF_6(i));
		if (AFF_FLAGGED(i, EAffect::kHide))
			sprintf(buf + strlen(buf), "(спрятал%s) ", GET_CH_SUF_2(i));
		if (AFF_FLAGGED(i, EAffect::kDisguise))
			sprintf(buf + strlen(buf), "(замаскировал%s) ", GET_CH_SUF_2(i));
		if (AFF_FLAGGED(i, EAffect::kFly))
			strcat(buf, IS_POLY(i) ? "(летят) " : "(летит) ");
		if (AFF_FLAGGED(i, EAffect::kHorse))
			strcat(buf, "(под седлом) ");

		strcat(buf, i->player_data.long_descr.c_str());
		SendMsgToChar(buf, ch);

		*aura_txt = '\0';
		if (AFF_FLAGGED(i, EAffect::kGodsShield)) {
			strcat(aura_txt, "...окутан");
			strcat(aura_txt, GET_CH_SUF_6(i));
			strcat(aura_txt, " сверкающим коконом ");
		}
		if (AFF_FLAGGED(i, EAffect::kSanctuary))
			strcat(aura_txt, IS_POLY(i) ? "...светятся ярким сиянием " : "...светится ярким сиянием ");
		else if (AFF_FLAGGED(i, EAffect::kPrismaticAura))
			strcat(aura_txt, IS_POLY(i) ? "...переливаются всеми цветами " : "...переливается всеми цветами ");
		act(aura_txt, false, i, nullptr, ch, kToVict);

		*aura_txt = '\0';
		n = 0;
		strcat(aura_txt, "...окружен");
		strcat(aura_txt, GET_CH_SUF_6(i));
		if (AFF_FLAGGED(i, EAffect::kAirShield)) {
			strcat(aura_txt, " воздушным");
			n++;
		}
		if (AFF_FLAGGED(i, EAffect::kFireShield)) {
			if (n > 0)
				strcat(aura_txt, ", огненным");
			else
				strcat(aura_txt, " огненным");
			n++;
		}
		if (AFF_FLAGGED(i, EAffect::kIceShield)) {
			if (n > 0)
				strcat(aura_txt, ", ледяным");
			else
				strcat(aura_txt, " ледяным");
			n++;
		}
		if (n == 1)
			strcat(aura_txt, " щитом ");
		else if (n > 1)
			strcat(aura_txt, " щитами ");
		if (n > 0)
			act(aura_txt, false, i, nullptr, ch, kToVict);

		if (AFF_FLAGGED(ch, EAffect::kDetectMagic)) {
			*aura_txt = '\0';
			n = 0;
			strcat(aura_txt, "...");
			if (AFF_FLAGGED(i, EAffect::kMagicGlass)) {
				if (n > 0)
					strcat(aura_txt, ", серебристая");
				else
					strcat(aura_txt, "серебристая");
				n++;
			}
			if (AFF_FLAGGED(i, EAffect::kBrokenChains)) {
				if (n > 0)
					strcat(aura_txt, ", ярко-синяя");
				else
					strcat(aura_txt, "ярко-синяя");
				n++;
			}
			if (AFF_FLAGGED(i, EAffect::kForcesOfEvil)) {
				if (n > 0)
					strcat(aura_txt, ", черная");
				else
					strcat(aura_txt, "черная");
				n++;
			}
			if (n == 1)
				strcat(aura_txt, " аура ");
			else if (n > 1)
				strcat(aura_txt, " ауры ");

			if (n > 0)
				act(aura_txt, false, i, nullptr, ch, kToVict);
		}
		*aura_txt = '\0';
		if (AFF_FLAGGED(ch, EAffect::kDetectMagic)) {
			if (AFF_FLAGGED(i, EAffect::kHold))
				strcat(aura_txt, "...парализован$a");
			if (AFF_FLAGGED(i, EAffect::kSilence) && (!AFF_FLAGGED(i, EAffect::kStrangled)))
				strcat(aura_txt, "...нем$a");
		}
		if (AFF_FLAGGED(i, EAffect::kBlind))
			strcat(aura_txt, "...слеп$a");
		if (AFF_FLAGGED(i, EAffect::kDeafness))
			strcat(aura_txt, "...глух$a");
		if (AFF_FLAGGED(i, EAffect::kStrangled) && AFF_FLAGGED(i, EAffect::kSilence))
			strcat(aura_txt, "...задыхается.");

		if (*aura_txt)
			act(aura_txt, false, i, nullptr, ch, kToVict);

		return;
	}

	if (i->IsNpc()) {
		strcpy(buf1, i->get_npc_name().c_str());
		strcat(buf1, " ");
		if (AFF_FLAGGED(i, EAffect::kHorse))
			strcat(buf1, "(под седлом) ");
		utils::CAP(buf1);
	} else {
		sprintf(buf1, "%s%s ", i->race_or_title().c_str(), i->IsFlagged(EPlrFlag::kKiller) ? " <ДУШЕГУБ>" : "");
	}

	snprintf(buf, kMaxStringLength, "%s%s", AFF_FLAGGED(i, EAffect::kCharmed) ? "*" : "", buf1);
	if (AFF_FLAGGED(i, EAffect::kInvisible))
		sprintf(buf + strlen(buf), "(невидим%s) ", GET_CH_SUF_6(i));
	if (AFF_FLAGGED(i, EAffect::kHide))
		sprintf(buf + strlen(buf), "(спрятал%s) ", GET_CH_SUF_2(i));
	if (AFF_FLAGGED(i, EAffect::kDisguise))
		sprintf(buf + strlen(buf), "(замаскировал%s) ", GET_CH_SUF_2(i));
	if (!i->IsNpc() && !i->desc)
		sprintf(buf + strlen(buf), "(потерял%s связь) ", GET_CH_SUF_1(i));
	if (!i->IsNpc() && i->IsFlagged(EPlrFlag::kWriting))
		strcat(buf, "(пишет) ");

	if (i->GetPosition() != EPosition::kFight) {
		if (i->IsOnHorse()) {
			CharData *horse = i->get_horse();
			if (horse) {
				const char *msg =
					AFF_FLAGGED(horse, EAffect::kFly) ? "летает" : "сидит";
				sprintf(buf + strlen(buf), "%s здесь верхом на %s. ",
						msg, PERS(horse, ch, 5));
			}
		} else if (IS_HORSE(i) && AFF_FLAGGED(i, EAffect::kTethered))
			sprintf(buf + strlen(buf), "привязан%s здесь. ", GET_CH_SUF_6(i));
		else if ((sector = real_sector(i->in_room)) == ESector::kOnlyFlying)
			strcat(buf, IS_POLY(i) ? "летают здесь. " : "летает здесь. ");
		else if (sector == ESector::kUnderwater)
			strcat(buf, IS_POLY(i) ? "плавают здесь. " : "плавает здесь. ");
		else if (i->GetPosition() > EPosition::kSleep && AFF_FLAGGED(i, EAffect::kFly))
			strcat(buf, IS_POLY(i) ? "летают здесь. " : "летает здесь. ");
		else if (sector == ESector::kWaterSwim || sector == ESector::kWaterNoswim)
			strcat(buf, IS_POLY(i) ? "плавают здесь. " : "плавает здесь. ");
		else
			strcat(buf,
				   IS_POLY(i) ? poly_positions[static_cast<int>(i->GetPosition())] : positions[static_cast<int>(i->GetPosition())]);
		if (AFF_FLAGGED(ch, EAffect::kDetectMagic) && i->IsNpc() && IsAffectedBySpell(i, ESpell::kCapable))
			sprintf(buf + strlen(buf), "(аура магии) ");
	} else {
		if (i->GetEnemy()) {
			strcat(buf, IS_POLY(i) ? "сражаются с " : "сражается с ");
			if (i->in_room != i->GetEnemy()->in_room)
				strcat(buf, "чьей-то тенью");
			else if (i->GetEnemy() == ch)
				strcat(buf, "ВАМИ");
			else
				strcat(buf, GET_PAD(i->GetEnemy(), 4));
			if (i->IsOnHorse())
				sprintf(buf + strlen(buf), ", сидя верхом на %s! ", PERS(i->get_horse(), ch, 5));
			else
				strcat(buf, "! ");
		} else        // NIL fighting pointer
		{
			strcat(buf, IS_POLY(i) ? "колотят по воздуху" : "колотит по воздуху");
			if (i->IsOnHorse())
				sprintf(buf + strlen(buf), ", сидя верхом на %s. ", PERS(i->get_horse(), ch, 5));
			else
				strcat(buf, ". ");
		}
	}

	if (AFF_FLAGGED(ch, EAffect::kDetectMagic)
		&& !AFF_FLAGGED(ch, EAffect::kDetectAlign)) {
		if (AFF_FLAGGED(i, EAffect::kForcesOfEvil))
			strcat(buf, "(черная аура) ");
	}
	if (AFF_FLAGGED(ch, EAffect::kDetectAlign)) {
		if (i->IsNpc()) {
			if (IS_EVIL(i)) {
				if (AFF_FLAGGED(ch, EAffect::kDetectMagic)
					&& AFF_FLAGGED(i, EAffect::kForcesOfEvil))
					strcat(buf, "(иссиня-черная аура) ");
				else
					strcat(buf, "(темная аура) ");
			} else if (IS_GOOD(i)) {
				if (AFF_FLAGGED(ch, EAffect::kDetectMagic)
					&& AFF_FLAGGED(i, EAffect::kForcesOfEvil))
					strcat(buf, "(серая аура) ");
				else
					strcat(buf, "(светлая аура) ");
			} else {
				if (AFF_FLAGGED(ch, EAffect::kDetectMagic)
					&& AFF_FLAGGED(i, EAffect::kForcesOfEvil))
					strcat(buf, "(черная аура) ");
			}
		} else {
			AddPkAuraDescription(i, aura_txt);
			strcat(buf, aura_txt);
			strcat(buf, " ");
		}
	}
	if (AFF_FLAGGED(ch, EAffect::kDetectPoison))
		if (AFF_FLAGGED(i, EAffect::kPoisoned)
			|| IsAffectedBySpell(i, ESpell::kDaturaPoison)
			|| IsAffectedBySpell(i, ESpell::kAconitumPoison)
			|| IsAffectedBySpell(i, ESpell::kScopolaPoison)
			|| IsAffectedBySpell(i, ESpell::kBelenaPoison))
			sprintf(buf + strlen(buf), "(отравлен%s) ", GET_CH_SUF_6(i));

	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	*aura_txt = '\0';
	if (AFF_FLAGGED(i, EAffect::kGodsShield)) {
		strcat(aura_txt, "...окутан");
		strcat(aura_txt, GET_CH_SUF_6(i));
		strcat(aura_txt, " сверкающим коконом ");
	}
	if (AFF_FLAGGED(i, EAffect::kSanctuary))
		strcat(aura_txt, IS_POLY(i) ? "...светятся ярким сиянием " : "...светится ярким сиянием ");
	else if (AFF_FLAGGED(i, EAffect::kPrismaticAura))
		strcat(aura_txt, IS_POLY(i) ? "...переливаются всеми цветами " : "...переливается всеми цветами ");
	act(aura_txt, false, i, nullptr, ch, kToVict);

	*aura_txt = '\0';
	n = 0;
	strcat(aura_txt, "...окружен");
	strcat(aura_txt, GET_CH_SUF_6(i));
	if (AFF_FLAGGED(i, EAffect::kAirShield)) {
		strcat(aura_txt, " воздушным");
		n++;
	}
	if (AFF_FLAGGED(i, EAffect::kFireShield)) {
		if (n > 0)
			strcat(aura_txt, ", огненным");
		else
			strcat(aura_txt, " огненным");
		n++;
	}
	if (AFF_FLAGGED(i, EAffect::kIceShield)) {
		if (n > 0)
			strcat(aura_txt, ", ледяным");
		else
			strcat(aura_txt, " ледяным");
		n++;
	}
	if (n == 1)
		strcat(aura_txt, " щитом ");
	else if (n > 1)
		strcat(aura_txt, " щитами ");
	if (n > 0)
		act(aura_txt, false, i, nullptr, ch, kToVict);
	if (AFF_FLAGGED(ch, EAffect::kDetectMagic)) {
		*aura_txt = '\0';
		n = 0;
		strcat(aura_txt, " ..");
		if (AFF_FLAGGED(i, EAffect::kMagicGlass)) {
			if (n > 0)
				strcat(aura_txt, ", серебристая");
			else
				strcat(aura_txt, "серебристая");
			n++;
		}
		if (AFF_FLAGGED(i, EAffect::kBrokenChains)) {
			if (n > 0)
				strcat(aura_txt, ", ярко-синяя");
			else
				strcat(aura_txt, "ярко-синяя");
			n++;
		}
		if (n == 1)
			strcat(aura_txt, " аура ");
		else if (n > 1)
			strcat(aura_txt, " ауры ");

		if (n > 0)
			act(aura_txt, false, i, nullptr, ch, kToVict);
	}
	*aura_txt = '\0';
	if (AFF_FLAGGED(ch, EAffect::kDetectMagic)) {
		if (AFF_FLAGGED(i, EAffect::kHold))
			strcat(aura_txt, " ...парализован$a");
		if (AFF_FLAGGED(i, EAffect::kSilence) && (!AFF_FLAGGED(i, EAffect::kStrangled)))
			strcat(aura_txt, " ...нем$a");
	}
	if (AFF_FLAGGED(i, EAffect::kBlind))
		strcat(aura_txt, " ...слеп$a");
	if (AFF_FLAGGED(i, EAffect::kDeafness))
		strcat(aura_txt, " ...глух$a");
	if (AFF_FLAGGED(i, EAffect::kStrangled) && AFF_FLAGGED(i, EAffect::kSilence))
		strcat(aura_txt, " ...задыхается");
	if (AFF_FLAGGED(i, EAffect::kCommander))
		strcat(aura_txt, " ...реет стяг над головой");
	if (*aura_txt)
		act(aura_txt, false, i, nullptr, ch, kToVict);
/*	if (IS_MANA_CASTER(i)) {
		*aura_txt = '\0';
		if (i->GetMorphSkill(ESkill::kDarkMagic) > 0)
			strcat(aura_txt, "...все сферы магии кружатся над головой");
		else if (i->GetMorphSkill(ESkill::kAirMagic) > 0)
			strcat(aura_txt, "...сферы четырех магий кружатся над головой");
		else if (i->GetMorphSkill(ESkill::kEarthMagic) > 0)
			strcat(aura_txt, "...сферы трех магий кружатся над головой");
		else if (i->GetMorphSkill(ESkill::kWaterMagic) > 0)
			strcat(aura_txt, "...сферы двух магий кружатся над головой");
		else if (i->GetMorphSkill(ESkill::kFireMagic) > 0)
			strcat(aura_txt, "...сфера огня кружит над головой");
		if (*aura_txt)
			act(aura_txt, false, i, nullptr, ch, kToVict);
	}
*/
}

char *diag_weapon_to_char(const CObjectPrototype *obj, int show_wear) {
	static char out_str[kMaxStringLength];
	int skill = 0;
	int need_str = 0;

	*out_str = '\0';
	if (obj->get_type() == EObjType::kWeapon) {
		switch (static_cast<ESkill>(obj->get_spec_param())) {
			case ESkill::kBows: skill = 1;
				break;
			case ESkill::kShortBlades: skill = 2;
				break;
			case ESkill::kLongBlades: skill = 3;
				break;
			case ESkill::kAxes: skill = 4;
				break;
			case ESkill::kClubs: skill = 5;
				break;
			case ESkill::kNonstandart: skill = 6;
				break;
			case ESkill::kTwohands: skill = 7;
				break;
			case ESkill::kPicks: skill = 8;
				break;
			case ESkill::kSpades: skill = 9;
				break;
			default: sprintf(out_str, "!! Не принадлежит к известным типам оружия - сообщите Богам (skill=%d) !!\r\n", obj->get_spec_param());
				break;
		}
		if (skill) {
			sprintf(out_str, "Принадлежит к классу \"%s\".\r\n", weapon_class[skill - 1]);
		}
	}
	if (show_wear) {
		if (CAN_WEAR(obj, EWearFlag::kFinger)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на палец.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kNeck)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на шею.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kBody)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на туловище.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kHead)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на голову.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kLegs)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на ноги.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kFeet)) {
			sprintf(out_str + strlen(out_str), "Можно обуть.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kHands)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на кисти.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kArms)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на руки.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kShoulders)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на плечи.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kWaist)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на пояс.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kQuiver)) {
			sprintf(out_str + strlen(out_str), "Можно использовать как колчан.\r\n");
		}
		if (CAN_WEAR(obj, EWearFlag::kWrist)) {
			sprintf(out_str + strlen(out_str), "Можно надеть на запястья.\r\n");
		}
		if (show_wear > 1) {
			if (CAN_WEAR(obj, EWearFlag::kShield)) {
				need_str = std::max(0, calc_str_req((obj->get_weight() + 1) / 2, STR_HOLD_W));
				sprintf(out_str + strlen(out_str),
						"Можно использовать как щит (требуется %d %s).\r\n",
						need_str,
						GetDeclensionInNumber(need_str, EWhat::kStr));
			}
			if (CAN_WEAR(obj, EWearFlag::kWield)) {
				need_str = std::max(0, calc_str_req(obj->get_weight(), STR_WIELD_W));
				sprintf(out_str + strlen(out_str),
						"Можно взять в правую руку (требуется %d %s).\r\n",
						need_str,
						GetDeclensionInNumber(need_str, EWhat::kStr));
			}
			if (CAN_WEAR(obj, EWearFlag::kHold)) {
				need_str = std::max(0, calc_str_req(obj->get_weight(), STR_HOLD_W));
				sprintf(out_str + strlen(out_str),
						"Можно взять в левую руку (требуется %d %s).\r\n",
						need_str,
						GetDeclensionInNumber(need_str, EWhat::kStr));
			}
			if (CAN_WEAR(obj, EWearFlag::kBoth)) {
				need_str = std::max(0, calc_str_req(obj->get_weight(), STR_BOTH_W));
				sprintf(out_str + strlen(out_str),
						"Можно взять в обе руки (требуется %d %s).\r\n",
						need_str,
						GetDeclensionInNumber(need_str, EWhat::kStr));
			}
		} else {
			if (CAN_WEAR(obj, EWearFlag::kShield)) {
				sprintf(out_str + strlen(out_str), "Можно использовать как щит.\r\n");
			}
			if (CAN_WEAR(obj, EWearFlag::kWield)) {
				sprintf(out_str + strlen(out_str), "Можно взять в правую руку.\r\n");
			}
			if (CAN_WEAR(obj, EWearFlag::kHold)) {
				sprintf(out_str + strlen(out_str), "Можно взять в левую руку.\r\n");
			}
			if (CAN_WEAR(obj, EWearFlag::kBoth)) {
				sprintf(out_str + strlen(out_str), "Можно взять в обе руки.\r\n");
			}
		}
	}
	return (out_str);
}

std::string diag_armor_type_to_char(const ObjData *obj) {
	if (obj->get_type() == EObjType::kLightArmor) {
		return "Легкий тип доспехов.\r\n";
	}
	if (obj->get_type() == EObjType::kMediumArmor) {
		return "Средний тип доспехов.\r\n";
	}
	if (obj->get_type() == EObjType::kHeavyArmor) {
		return "Тяжелый тип доспехов.\r\n";
	}
	return "";
}

char *diag_timer_to_char(const ObjData *obj) {
	static char out_str[kMaxStringLength];
	*out_str = 0;
	sprintf(out_str, "Состояние: %s.", diag_obj_timer(obj));
	return (out_str);
}

char *diag_uses_to_char(ObjData *obj, CharData *ch) {
	static char out_str[kMaxStringLength];

	*out_str = 0;
	if (obj->get_type() == EObjType::kIngredient
		&& IS_SET(obj->get_spec_param(), kItemCheckUses)
		&& IS_MANA_CASTER(ch)) {
		int i = -1;
		if ((i = GetObjRnum(GET_OBJ_VAL(obj, 1))) >= 0) {
			sprintf(out_str, "Прототип: %s%s%s.\r\n",
					kColorBoldCyn, obj_proto[i]->get_PName(ECase::kNom).c_str(), kColorNrm);
		}
		sprintf(out_str + strlen(out_str), "Осталось применений: %s%d&n.\r\n",
				GET_OBJ_VAL(obj, 2) > 100 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
	}
	return (out_str);
}

char *diag_shot_to_char(ObjData *obj, CharData *ch) {
	static char out_str[kMaxStringLength];

	*out_str = 0;
	if (obj->get_type() == EObjType::kMagicContaner
		&& (ch->GetClass() == ECharClass::kRanger || ch->GetClass() == ECharClass::kCharmer || IS_MANA_CASTER(ch))) {
		sprintf(out_str + strlen(out_str), "Осталось стрел: %s%d&n.\r\n",
				GET_OBJ_VAL(obj, 2) > 3 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
	}
	return (out_str);
}

// Чтобы можно было получить только строку состяния
const char *diag_obj_timer(const ObjData *obj) {
	int prot_timer;
	if (obj->get_rnum() != kNothing) {
		if (stable_objs::IsTimerUnlimited(obj)) {
			return "нерушимо";
		}

		if (obj->get_craft_timer() > 0) {
			prot_timer = obj->get_craft_timer();// если вещь скрафчена, смотрим ее таймер а не у прототипа
		} else {
			prot_timer = obj_proto[obj->get_rnum()]->get_timer();
		}

		if (!prot_timer) {
			return "Прототип предмета имеет нулевой таймер!\r\n";
		}

		const int tm = (obj->get_timer() * 100 / prot_timer); // если вещь скрафчена, смотрим ее таймер а не у прототипа
		return print_obj_state(tm);
	}
	return "";
}

const char *print_obj_state(int tm_pct) {
	if (tm_pct < 20)
		return "ужасно";
	else if (tm_pct < 40)
		return "скоро сломается";
	else if (tm_pct < 60)
		return "плоховато";
	else if (tm_pct < 80)
		return "средне";
		//else if (tm_pct <=100) // у только что созданной шмотки значение 100% первый тик, потому <=
		//	return "идеально";
	else if (tm_pct < 1000) // проблема крафта, на хаймортах таймер больще прототипа
		return "идеально";
	else return "нерушимо";
}

// Это не совсем механика зрения, нужен отдельный модуль на тему "кто кого (не)видит"
// Но пока эти функции распиханы по всему коду, начиная с handler, по этому временно - тут
void Appear(CharData *ch) {
	const bool appear_msg = AFF_FLAGGED(ch, EAffect::kInvisible)
		|| AFF_FLAGGED(ch, EAffect::kDisguise)
		|| AFF_FLAGGED(ch, EAffect::kHide);

	RemoveAffectFromChar(ch, ESpell::kInvisible);
	RemoveAffectFromChar(ch, ESpell::kHide);
	RemoveAffectFromChar(ch, ESpell::kSneak);
	RemoveAffectFromChar(ch, ESpell::kCamouflage);

	AFF_FLAGS(ch).unset(EAffect::kInvisible);
	AFF_FLAGS(ch).unset(EAffect::kHide);
	AFF_FLAGS(ch).unset(EAffect::kSneak);
	AFF_FLAGS(ch).unset(EAffect::kDisguise);

	if (appear_msg) {
		if (ch->IsNpc() || GetRealLevel(ch) < kLvlImmortal) {
			act("$n медленно появил$u из пустоты.", false, ch, nullptr, nullptr, kToRoom);
		} else {
			act("Вы почувствовали странное присутствие $n1.",
				false, ch, nullptr, nullptr, kToRoom);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
