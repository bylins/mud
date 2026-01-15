//
// Created by Sventovit on 08.05.2022.
//

#include "administration/accounts.h"
#include "administration/ban.h"
#include "administration/privilege.h"
#include "engine/ui/cmd/do_features.h"
#include "engine/ui/cmd/do_skills.h"
#include "engine/ui/cmd/do_spells.h"
#include "gameplay/communication/parcel.h"
#include "gameplay/communication/mail.h"
#include "gameplay/mechanics/depot.h"
#include "engine/scripting/dg_event.h"
#include "gameplay/economics/shop_ext.h"
#include "gameplay/economics/ext_money.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/glory_const.h"
#include "gameplay/mechanics/glory_misc.h"
#include "engine/core/handler.h"
#include "engine/ui/modify.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/statistics/mob_stat.h"
#include "engine/db/global_objects.h"
#include "utils/file_crc.h"
#include "engine/entities/char_player.h"
#include "gameplay/statistics/money_drop.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/statistics/zone_exp.h"
#include "engine/db/player_index.h"

#include <third_party_libs/fmt/include/fmt/format.h>

extern void print_rune_stats(CharData *ch);
void do_shops_list(CharData *ch);

void show_apply(CharData *ch, CharData *vict) {
	ObjData *obj = nullptr;
	for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if ((obj = GET_EQ(vict, i))) {
			SendMsgToChar(ch, "Предмет: %s (%d)\r\n", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_VNUM(obj));
			// Update weapon applies
			for (int j = 0; j < kMaxObjAffect; j++) {
				if (GET_EQ(vict, i)->get_affected(j).modifier != 0) {
					SendMsgToChar(ch, "Добавляет (apply): %s, модификатор: %d\r\n",
								  apply_types[(int) GET_EQ(vict, i)->get_affected(j).location], GET_EQ(vict, i)->get_affected(j).modifier);
				}
			}
		}
	}
}

void ShowClassInfo(CharData *ch, const std::string &class_name, const std::string &params) {
	if (class_name.empty()) {
		SendMsgToChar("Формат: show class [имя класса] [stats|skills|spells|feats|параметры|умения|заклинания|способности]", ch);
		return;
	}

	auto class_id = FindAvailableCharClassId(class_name);
	if (class_id == ECharClass::kUndefined) {
		SendMsgToChar("Неизвестное имя класса.", ch);
		return;
	}

	std::ostringstream out;
	if (params.empty()) {
		MUD::Class(class_id).Print(ch, out);
	} else {
		MUD::Class(class_id).PrintHeader(out);
		if (utils::IsAbbr(params, "stats") || utils::IsAbbr(params, "параметры")) {
			MUD::Class(class_id).PrintBaseStatsTable(ch, out);
		} else if (utils::IsAbbr(params, "skills") || utils::IsAbbr(params, "умения")) {
			MUD::Class(class_id).PrintSkillsTable(ch, out);
		} else if (utils::IsAbbr(params, "spells") || utils::IsAbbr(params, "заклинания")) {
			MUD::Class(class_id).PrintSpellsTable(ch, out);
		} else if (utils::IsAbbr(params, "feats") || utils::IsAbbr(params, "способности")) {
			MUD::Class(class_id).PrintFeatsTable(ch, out);
		}
	}
	page_string(ch->desc, out.str());
}

void ShowSpellInfo(CharData *ch, const std::string &spell_name) {
	if (spell_name.size() > 100) {
		SendMsgToChar("Превышена максимальная длина строки в запросе.\r\n", ch);
		return;
	}
	if (spell_name.empty()) {
		SendMsgToChar("Формат: show spellinfo [название заклинания]", ch);
		return;
	}

	// Это очень тупо, но иначе надо переписывать всю команду. Пока некогда.
	auto name_copy = spell_name;
	auto id = FixNameAndFindSpellId(name_copy);
	if (id == ESpell::kUndefined) {
		SendMsgToChar("Неизвестное название заклинания.", ch);
		return;
	}

	std::ostringstream out;
	MUD::Spell(id).Print(ch, out);
	page_string(ch->desc, out.str());
}

void ShowFeatInfo(CharData *ch, const std::string &name) {
	if (name.size() > 100) {
		SendMsgToChar("Превышена максимальная длина строки в запросе.\r\n", ch);
		return;
	}
	if (name.empty()) {
		SendMsgToChar("Формат: show featinfo [название способности]", ch);
		return;
	}

	auto id = FixNameAndFindFeatId(name);
	if (id == EFeat::kUndefined) {
		SendMsgToChar("Неизвестное название способности.", ch);
		return;
	}

	std::ostringstream out;
	MUD::Feat(id).Print(ch, out);
	page_string(ch->desc, out.str());
}

void ShowMobClassInfo(CharData *ch, const std::string &mob_class_name) {
	if (mob_class_name.size() > 100) {
		SendMsgToChar("Превышена максимальная длина строки в запросе.\r\n", ch);
		return;
	}
	if (mob_class_name.empty()) {
		SendMsgToChar("Формат: show mobclass [название класса моба]", ch);
		return;
	}

	auto mob_class_id = FindAvailableMobClassId(ch, mob_class_name);
	if (mob_class_id == EMobClass::kUndefined) {
		SendMsgToChar("Неизвестное название класса моба.", ch);
		return;
	}

	std::ostringstream out;
	MUD::MobClass(mob_class_id).Print(ch, out);
	page_string(ch->desc, out.str());
}

void ShowAbilityInfo(CharData *ch, const std::string &name) {
	if (name.size() > 100) {
		SendMsgToChar("Превышена максимальная длина строки в запросе.\r\n", ch);
		return;
	}
	if (name.empty()) {
		SendMsgToChar("Формат: show abilityinfo [название навыка]", ch);
		return;
	}

	auto id = FixNameAndFindAbilityId(name);
	if (id == abilities::EAbility::kUndefined) {
		SendMsgToChar("Неизвестное название навыка.", ch);
		return;
	}

	std::ostringstream out;
	MUD::Ability(id).Print(ch, out);
	page_string(ch->desc, out.str());
}

void ShowGuildInfo(CharData *ch, const std::string &guild_locator) {
	std::ostringstream out;
	if (guild_locator.empty()) {
		for (const auto &guild : MUD::Guilds()) {
			guild.Print(ch, out);
		}
	} else {
		Vnum guild_vnum{-1};
		try {
			guild_vnum = std::stoi(guild_locator);
		} catch (...) {
			SendMsgToChar("Формат: show guild [vnum]", ch);
			return;
		}

		const auto &guild = MUD::Guild(guild_vnum);
		if (guild.GetId() == info_container::kUndefinedVnum) {
			SendMsgToChar("Гильдии с таким vnum не существует.", ch);
			return;
		}
		guild.Print(ch, out);
	}

	page_string(ch->desc, out.str());
}

void ShowCurrencyInfo(CharData *ch, const std::string &locator) {
	std::ostringstream out;
	if (locator.empty()) {
		for (const auto &item : MUD::Currencies()) {
			item.Print(ch, out);
		}
	} else {
		Vnum vnum{-1};
		try {
			vnum = std::stoi(locator);
		} catch (...) {
			SendMsgToChar("Формат: show guild [vnum]", ch);
			return;
		}

		const auto &item = MUD::Currency(vnum);
		if (item.GetId() == info_container::kUndefinedVnum) {
			SendMsgToChar("Элемента с таким vnum не существует.", ch);
			return;
		}
		item.Print(ch, out);
	}

	page_string(ch->desc, out.str());
}

namespace {

bool sort_by_zone_mob_level(int rnum1, int rnum2) {
	return !(zone_table[mob_index[rnum1].zone].mob_level < zone_table[mob_index[rnum2].zone].mob_level);
}

void print_mob_bosses(CharData *ch, bool lvl_sort) {
	std::vector<int> tmp_list;
	for (int i = 0; i <= top_of_mobt; ++i) {
		if (mob_proto[i].get_role(static_cast<unsigned>(EMobClass::kBoss))) {
			tmp_list.push_back(i);
		}
	}
	if (lvl_sort) {
		std::sort(tmp_list.begin(), tmp_list.end(), sort_by_zone_mob_level);
	}

	int cnt = 0;
	std::string out(
		"                          имя моба [ср.уровень мобов в зоне][vnum моба] имя зоны\r\n"
		"--------------------------------------------------------------------------------\r\n");

	for (int mob_rnum : tmp_list) {
		std::string zone_name_str = !zone_table[mob_index[mob_rnum].zone].name.empty() ?
									zone_table[mob_index[mob_rnum].zone].name : "EMPTY";

		const auto mob = mob_proto + mob_rnum;
		const auto vnum = GET_MOB_VNUM(mob);
		out += fmt::format("{:<3} {:<31}s [{:<2}][{:<6}] {:<31}s\r\n",
							  ++cnt,
							  mob->get_name_str().substr(0, 31),
							  zone_table[mob_index[mob_rnum].zone].mob_level,
							  vnum,
							  zone_name_str.substr(0, 31));
	}
	page_string(ch->desc, out);
}
} // namespace

std::string print_zone_enters(ZoneRnum zone) {
	bool found{false};
	char tmp[128];

	snprintf(tmp, sizeof(tmp),
			 "\r\nВходы в зону %3d:\r\n", zone_table[zone].vnum);
	std::string out(tmp);

	for (int n = kFirstRoom; n <= top_of_world; n++) {
		if (world[n]->zone_rn != zone) {
			for (int dir = 0; dir < EDirection::kMaxDirNum; dir++) {
				if (world[n]->dir_option[dir]
					&& world[world[n]->dir_option[dir]->to_room()]->zone_rn == zone
					&& world[world[n]->dir_option[dir]->to_room()]->vnum > 0) {
					snprintf(tmp, sizeof(tmp),
							 "  Номер комнаты:%5d Направление:%6s Вход в комнату:%5d\r\n",
							 world[n]->vnum, dirs_rus[dir],
							 world[world[n]->dir_option[dir]->to_room()]->vnum);
					out += tmp;
					found = true;
				}
			}
		}
	}
	if (!found) {
		out += "Входов в зону не обнаружено.\r\n";
	}
	return out;
}

std::string print_zone_exits(ZoneRnum zone) {
	bool found = false;
	char tmp[128];

	snprintf(tmp, sizeof(tmp),
			 "\r\nВыходы из зоны %3d:\r\n", zone_table[zone].vnum);
	std::string out(tmp);

	for (int n = kFirstRoom; n <= top_of_world; n++) {
		if (world[n]->zone_rn == zone) {
			for (int dir = 0; dir < EDirection::kMaxDirNum; dir++) {
				if (world[n]->dir_option[dir]
					&& world[world[n]->dir_option[dir]->to_room()]->zone_rn != zone
					&& world[world[n]->dir_option[dir]->to_room()]->vnum > 0) {
					snprintf(tmp, sizeof(tmp),
							 "  Номер комнаты:%5d Направление:%6s Выход в комнату:%5d\r\n",
							 world[n]->vnum, dirs_rus[dir],
							 world[world[n]->dir_option[dir]->to_room()]->vnum);
					out += tmp;
					found = true;
				}
			}
		}
	}
	if (!found) {
		out += "Выходов из зоны не обнаружено.\r\n";
	}
	return out;
}

// single zone printing fn used by "show zone" so it's not repeated in the
// code 3 times ... -je, 4/6/93

void print_zone_to_buf(char **bufptr, ZoneRnum zone) {
	const size_t BUFFER_SIZE = 1024;
	int rfirst, rlast;
	GetZoneRooms(zone, &rfirst, &rlast);
	char tmpstr[BUFFER_SIZE];
	snprintf(tmpstr, BUFFER_SIZE,
			 "%3d %s\r\n"
			 "Уровнь зоны %2d, Средний уровень мобов: %2d; Type: %-20.20s; Age: %3d; Reset: %3d (%1d)(%1d)\r\n"
			 "First: %5d, Top: %5d %s %s; ResetIdle: %s; Занято: %s; Активность: %.2f; Группа: %2d; \r\n"
			 "Автор: %s, количество репопов зоны (с перезагрузки): %d, всего посещений: %d, вход в зону: %d\r\n",
			 zone_table[zone].vnum,
			 zone_table[zone].name.c_str(),
			 zone_table[zone].level,
			 zone_table[zone].mob_level,
			 zone_types[zone_table[zone].type].name,
			 zone_table[zone].age, zone_table[zone].lifespan,
			 zone_table[zone].reset_mode,
			 (zone_table[zone].reset_mode == 3) ? (CanBeReset(zone) ? 1 : 0) : (IsZoneEmpty(zone) ? 1 : 0),
			 world[rfirst]->vnum,
			 world[rlast]->vnum,
			 zone_table[zone].under_construction ? "&GТестовая!&n" : " ",
			 zone_table[zone].locked ? "&RРедактирование запрещено!&n" : " ",
			 zone_table[zone].reset_idle ? "Y" : "N",
			 zone_table[zone].used ? "Y" : "N",
			 (double) zone_table[zone].activity / 1000,
			 zone_table[zone].group,
			 !zone_table[zone].author.empty() ? zone_table[zone].author.c_str() : "неизвестен",
			 zone_table[zone].count_reset,
			 zone_table[zone].traffic,
			 zone_table[zone].entrance);
	*bufptr = str_add(*bufptr, tmpstr);
/*
	if (!zone_table[zone].first_enter.empty()) {
		snprintf(tmpstr, BUFFER_SIZE, "Первый вошедший после ресета: %s\r\n", zone_table[zone].first_enter.c_str());
		*bufptr = str_add(*bufptr, tmpstr);
	}
*/
	if (zone_table[zone].copy_from_zone > 0) {
		snprintf(tmpstr, BUFFER_SIZE, "Зона прародитель: (%d) %s\r\n",
				 zone_table[zone].copy_from_zone, zone_table[GetZoneRnum(zone_table[zone].copy_from_zone)].name.c_str());
		*bufptr = str_add(*bufptr, tmpstr);
	}
}

struct show_struct {
	const char *cmd = nullptr;
	const char level = 0;
};

struct show_struct show_fields[] = {
	{"nothing", 0},        // 0
	{"zones", kLvlImmortal},    // 1
	{"player", kLvlImmortal},
	{"rent", kLvlGreatGod},
	{"stats", kLvlImmortal},
	{"errors", kLvlImplementator},    // 5
	{"death", kLvlGod},
	{"godrooms", kLvlGod},
	{"snoop", kLvlGreatGod},
	{"linkdrop", kLvlGreatGod},
	{"punishment", kLvlImmortal}, // 10
	{"paths", kLvlGreatGod},
	{"loadrooms", kLvlGreatGod},
	{"skills", kLvlImplementator},
	{"spells", kLvlImplementator},
	{"ban", kLvlImmortal}, // 15
	{"features", kLvlImplementator},
	{"glory", kLvlImplementator},
	{"crc", kLvlImmortal},
	{"affectedrooms", kLvlImmortal},
	{"money", kLvlImplementator}, // 20
	{"expgain", kLvlImplementator},
	{"runestat", kLvlGod},
	{"mobstat", kLvlImplementator},
	{"bosses", kLvlImplementator},
	{"remort", kLvlImplementator}, // 25
	{"apply", kLvlGod}, // 26
	{"worlds", kLvlImmortal},
	{"triggers", kLvlImmortal},
	{"class", kLvlImmortal},
	{"guild", kLvlImmortal}, //30
	{"currency", kLvlImmortal},
	{"spellinfo", kLvlImmortal},
	{"featinfo", kLvlImmortal},
	{"abilityinfo", kLvlImmortal},
	{"account", kLvlGod}, //35
	{"shops", kLvlGod},
	{"runeslist", kLvlGod},
	{"dungeons", kLvlGod},
	{"mobclass", kLvlGod},
	{"\n", 0}
};

std::pair<int, int> TotalMemUse(){
#ifdef __linux__
	FILE *fl;
	char name[256], line[1024];
	int mem = 0, vmem = 0, pmem = 0;
	pid_t pid = getpid();

	sprintf(name, "/proc/%d/status", pid);
	if (!(fl = fopen(name,"r"))) {
		log("Cann't open process files...");
		return std::make_pair(-1, -1);
	}
	while (get_line(fl, buf2)) {
		sscanf(buf2, "%s %d", line, &mem);
		if (!str_cmp(line, "VmRSS:")) {
			pmem = mem;
		}
		if (!str_cmp(line, "VmSize:")) {
			vmem = mem;
		}
		if (vmem > 0 && pmem > 0)
			break;
	}
	fclose(fl);
	return std::make_pair(vmem, pmem);
#else
	return std::make_pair(-1, -1);
#endif
}

void ListSpellCreate(CharData *ch) {
	int i = 0;
	for (auto it : spell_create) {
		SendMsgToChar(ch, "%3d) Rune spell [%3d] &W%-30s&n runes: %3d %3d %3d %3d level %d\r\n", 
				++i, to_underlying(it.first), MUD::Spell(it.first).GetCName(),
				it.second.runes.items[0], it.second.runes.items[1],
				it.second.runes.items[2], it.second.runes.rnumber,
				it.second.runes.min_caster_level);
	}
}

void do_show(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int i, j, l, con;    // i, j, k to specifics?

	ZoneRnum zrn;
	ZoneVnum zvn;
	char self = 0;
	CharData *vict;
	DescriptorData *d;
	char field[kMaxInputLength], value[kMaxInputLength], value1[kMaxInputLength];
	// char bf[kMaxExtendLength];
	char *bf = nullptr;
	char rem[kMaxInputLength];

	skip_spaces(&argument);

	if (!*argument) {
		strcpy(buf, "Опции для показа:\r\n");
		for (j = 0, i = 1; show_fields[i].level; i++)
			if (privilege::HasPrivilege(ch, std::string(show_fields[i].cmd), 0, 2))
				sprintf(buf + strlen(buf), "%-15s%s", show_fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
		strcat(buf, "\r\n");
		SendMsgToChar(buf, ch);
		return;
	}

	strcpy(arg, three_arguments(argument, field, value, value1));

	for (l = 0; *(show_fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, show_fields[l].cmd, strlen(field)))
			break;

	if (!privilege::HasPrivilege(ch, std::string(show_fields[l].cmd), 0, 2)) {
		SendMsgToChar("Вы не столь могущественны, чтобы узнать это.\r\n", ch);
		return;
	}
	if (!strcmp(value, "."))
		self = 1;
	buf[0] = '\0';
	//bf[0] = '\0';
	switch (l) {
		case 1:        // zone
			// tightened up by JE 4/6/93
			if (self)
				print_zone_to_buf(&bf, world[ch->in_room]->zone_rn);
			else if (*value1 && is_number(value) && is_number(value1)) {
				// хотят зоны в диапазоне увидеть
				int found = 0;
				int zstart = atoi(value);
				int zend = atoi(value1);

				for (zrn = 0; zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
					if (zone_table[zrn].vnum >= zstart
						&& zone_table[zrn].vnum <= zend) {
						print_zone_to_buf(&bf, zrn);
						found = 1;
					}
				}

				if (!found) {
					SendMsgToChar("В заданном диапазоне зон нет.\r\n", ch);
					return;
				}
			} else if (*value && is_number(value)) {
				for (zvn = atoi(value), zrn = 0;
					 zrn < static_cast<ZoneRnum>(zone_table.size()) && zone_table[zrn].vnum != zvn;
					 zrn++) {
					/* empty loop */
				}

				if (zrn < static_cast<ZoneRnum>(zone_table.size())) {
					print_zone_to_buf(&bf, zrn);
				} else {
					SendMsgToChar("Нет такой зоны.\r\n", ch);
					return;
				}
			} else if (*value && !strcmp(value, "-g")) {
				for (zrn = 0; zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
					if (zone_table[zrn].group > 1) {
						print_zone_to_buf(&bf, zrn);
					}
				}
			} else if (*value1 && !strcmp(value, "-l") && is_number(value1)) {
				one_argument(arg, value);
				if (*value && is_number(value)) {
					// show zones -l x y
					for (zrn = 0; zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
						if (zone_table[zrn].mob_level >= atoi(value1)
							&& zone_table[zrn].mob_level <= atoi(value)) {
							print_zone_to_buf(&bf, zrn);
						}
					}
				} else {
					// show zones -l x
					for (zrn = 0; zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
						if (zone_table[zrn].mob_level == atoi(value1)) {
							print_zone_to_buf(&bf, zrn);
						}
					}
				}
			} else {
				for (zrn = 0; zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
					print_zone_to_buf(&bf, zrn);
				}
			}

			page_string(ch->desc, bf, true);
			free(bf);
			break;

		case 2: {
			if (!*value) {
				SendMsgToChar("Уточните имя.\r\n", ch);
				return;
			}
			if (!(vict = get_player_vis(ch, value, EFind::kCharInWorld))) {
				SendMsgToChar("Нет такого игрока.\r\n", ch);
				return;
			}
			sprintf(buf, "&WИнформация по игроку %s:&n (", vict->get_name().c_str());
			sprinttype(to_underlying(vict->get_sex()), genders, buf + strlen(buf));
			sprintf(buf + strlen(buf), ")&n\r\n");
			sprintf(buf + strlen(buf), "Падежи : %s/%s/%s/%s/%s/%s\r\n",
					GET_PAD(vict, 0), GET_PAD(vict, 1), GET_PAD(vict, 2),
					GET_PAD(vict, 3), GET_PAD(vict, 4), GET_PAD(vict, 5));
			if (!NAME_GOD(vict)) {
				sprintf(buf + strlen(buf), "Имя никем не одобрено!\r\n");
			} else if (NAME_GOD(vict) < 1000) {
				sprintf(buf1, "%s", GetNameById(NAME_ID_GOD(vict)).c_str());
				*buf1 = UPPER(*buf1);
				snprintf(buf + strlen(buf), kMaxStringLength, "Имя запрещено богом %s\r\n", buf1);
			} else {
				sprintf(buf1, "%s", GetNameById(NAME_ID_GOD(vict)).c_str());
				*buf1 = UPPER(*buf1);
				snprintf(buf + strlen(buf), kMaxStringLength, "Имя одобрено богом %s\r\n", buf1);
			}
			if (GetRealRemort(vict) < 4)
				sprintf(rem, "Перевоплощений: %d\r\n", GetRealRemort(vict));
			else
				sprintf(rem, "Перевоплощений: 3+\r\n");
			sprintf(buf + strlen(buf), "%s", rem);
			sprintf(buf + strlen(buf), "Уровень: %s\r\n", (GetRealLevel(vict) < 25 ? "ниже 25" : "25+"));
			const auto &title = vict->GetTitleStr();
			sprintf(buf + strlen(buf), "Титул: %s\r\n", (title.empty() ? "<Нет>" : title.c_str()));
			sprintf(buf + strlen(buf), "Описание игрока:\r\n");
			sprintf(buf + strlen(buf),
					"%s\r\n",
					(vict->player_data.description.empty() ? "<Нет>" : vict->player_data.description.c_str()));
			SendMsgToChar(buf, ch);
			// Отображаем карму.
			if (KARMA(vict)) {
				sprintf(buf, "\r\n&WИнформация по наказаниям и поощрениям:&n\r\n%s", KARMA(vict));
				SendMsgToChar(buf, ch);
			}
			break;
		}
		case 3:
			if (!*value) {
				SendMsgToChar("Уточните имя.\r\n", ch);
				return;
			}
			Crash_listrent(ch, value);
			break;
		case 4: {
			i = 0;
			j = 0;
			con = 0;
			int motion = 0;
			for (const auto &victim : character_list) {
				if (victim->IsNpc()) {
					j++;
				} else {
					if (victim->is_active()) {
						++motion;
					}
					if (CAN_SEE(ch, victim)) {
						i++;
						if (victim->desc) {
							con++;
						}
					}
				}
			}

			strcpy(buf, "Текущее состояние:\r\n");
			sprintf(buf + strlen(buf), "  Игроков в игре - %5d, соединений - %5d\r\n", i, con);
			sprintf(buf + strlen(buf), "  Всего зарегистрировано игроков - %5zd\r\n", player_table.size());
			sprintf(buf + strlen(buf), "  Мобов - %5d,  прообразов мобов - %5d\r\n", j, top_of_mobt + 1);
			sprintf(buf + strlen(buf), "  Предметов - %5zd, прообразов предметов - %5zd\r\n",
					world_objects.size(), obj_proto.size());
			sprintf(buf + strlen(buf), "  Комнат - %5d, зон - %5zd, триггеров %d\r\n", top_of_world + 1, zone_table.size(), top_of_trigt);
			sprintf(buf + strlen(buf), "  Больших буферов - %5d\r\n", iosystem::buf_largecount);
			sprintf(buf + strlen(buf),
					"  Переключенных буферов - %5d, переполненных - %5d\r\n",
					iosystem::buf_switches,
					iosystem::buf_overflows);
			auto getmem = TotalMemUse();
			sprintf(buf + strlen(buf), "  PID процесса: %d, использовано памяти: виртуальной - %d kB, физической: - %d kB\r\n", getpid(), getmem.first, getmem.second);
			sprintf(buf + strlen(buf), "  Послано байт - %lu\r\n", iosystem::number_of_bytes_written);
			sprintf(buf + strlen(buf), "  Получено байт - %lu\r\n", iosystem::number_of_bytes_read);
			sprintf(buf + strlen(buf), "  Максимальный Id - %ld\r\n", max_id.current());
			sprintf(buf + strlen(buf), "  Активность игроков (cmds/min) - %lu\r\n",
					static_cast<unsigned long>((cmd_cnt * 60) / (time(nullptr) - shutdown_parameters.get_boot_time())));
			SendMsgToChar(buf, ch);
			Depot::show_stats(ch);
			Glory::show_stats(ch);
			GloryConst::show_stats(ch);
			Parcel::show_stats(ch);
			SendMsgToChar(ch, "  Сообщений на почте: %zu\r\n", mail::get_msg_count());
			SendMsgToChar(ch, "  Передвижения: %d\r\n", motion);
			SendMsgToChar(ch, "  Потрачено кун в магазинах2 за ребут: %d\r\n", ShopExt::get_spent_today());
			mob_stat::ShowStats(ch);
			break;
		}
		case 5: {
			int k = 0;
			strcpy(buf, "Пустых выходов\r\n" "--------------\r\n");
			for (i = kFirstRoom; i <= top_of_world; i++) {
				for (j = 0; j < EDirection::kMaxDirNum; j++) {
					if (world[i]->dir_option[j]
						&& world[i]->dir_option[j]->to_room() == 0) {
						sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k,
								GET_ROOM_VNUM(i), world[i]->name);
					}
				}
			}
			page_string(ch->desc, buf, true);
		}
			break;

		case 6: strcpy(buf, "Смертельных выходов\r\n" "-------------------\r\n");
			for (i = kFirstRoom, j = 0; i <= top_of_world; i++)
				if (ROOM_FLAGGED(i, ERoomFlag::kDeathTrap))
					sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i]->name);
			page_string(ch->desc, buf, true);
			break;
		case 7: strcpy(buf, "Комнаты для богов\r\n" "-----------------\r\n");
			for (i = kFirstRoom, j = 0; i <= top_of_world; i++)
				if (ROOM_FLAGGED(i, ERoomFlag::kGodsRoom))
					sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i]->name);
			page_string(ch->desc, buf, true);
			break;
		case 8: *buf = '\0';
			SendMsgToChar("Система негласного контроля:\r\n", ch);
			SendMsgToChar("----------------------------\r\n", ch);
			for (d = descriptor_list; d; d = d->next) {
				if (d->snooping
					&& d->character
					&& d->state == EConState::kPlaying
					&& d->character->in_room != kNowhere
					&& ((CAN_SEE(ch, d->character) && GetRealLevel(ch) >= GetRealLevel(d->character))
						|| ch->IsFlagged(EPrf::kCoderinfo))) {
					sprintf(buf + strlen(buf),
							"%-10s - подслушивается %s (map %s).\r\n",
							GET_NAME(d->snooping->character),
							GET_PAD(d->character, 4),
							d->snoop_with_map ? "on" : "off");
				}
			}
			SendMsgToChar(*buf ? buf : "Никто не подслушивается.\r\n", ch);
			break;        // snoop
		case 9:        // show linkdrop
			SendMsgToChar("  Список игроков в состоянии 'link drop'\r\n", ch);
			sprintf(buf, "%-50s%-16s   %s\r\n", "   Имя", "Комната", "Бездействие (тики)");
			SendMsgToChar(buf, ch);
			i = 0;
			for (const auto &character : character_list) {
				if (IS_GOD(character) || character->IsNpc() ||
					character->desc != nullptr || character->in_room == kNowhere) {
					continue;
				}
				++i;
				sprintf(buf, "%-50s[%6d][%6d]   %d\r\n",
						character->GetNameWithTitleOrRace().c_str(), GET_ROOM_VNUM(character->in_room),
						GET_ROOM_VNUM(character->get_was_in_room()), character->char_specials.timer);
				SendMsgToChar(buf, ch);
			}
			sprintf(buf, "Всего - %d\r\n", i);
			SendMsgToChar(buf, ch);
			break;
		case 10:        // show punishment
			SendMsgToChar("  Список наказанных игроков.\r\n", ch);
			for (d = descriptor_list; d; d = d->next) {
				if (d->snooping != nullptr && d->character != nullptr)
					continue;
				if (d->state != EConState::kPlaying
					|| (GetRealLevel(ch) < GetRealLevel(d->character) && !ch->IsFlagged(EPrf::kCoderinfo)))
					continue;
				if (!CAN_SEE(ch, d->character) || d->character->in_room == kNowhere)
					continue;
				buf[0] = 0;
				if (d->character->IsFlagged(EPlrFlag::kFrozen)
					&& FREEZE_DURATION(d->character))
					sprintf(buf + strlen(buf), "Заморожен : %ld час [%s].\r\n",
							static_cast<long>((FREEZE_DURATION(d->character) - time(nullptr)) / 3600),
							FREEZE_REASON(d->character) ? FREEZE_REASON(d->character) : "-");

				if (d->character->IsFlagged(EPlrFlag::kMuted)
					&& MUTE_DURATION(d->character))
					sprintf(buf + strlen(buf), "Будет молчать : %ld час [%s].\r\n",
							static_cast<long>((MUTE_DURATION(d->character) - time(nullptr)) / 3600),
							MUTE_REASON(d->character) ? MUTE_REASON(d->character) : "-");

				if (d->character->IsFlagged(EPlrFlag::kDumbed)
					&& DUMB_DURATION(d->character))
					sprintf(buf + strlen(buf), "Будет нем : %ld час [%s].\r\n",
							static_cast<long>((DUMB_DURATION(d->character) - time(nullptr)) / 3600),
							DUMB_REASON(d->character) ? DUMB_REASON(d->character) : "-");

				if (d->character->IsFlagged(EPlrFlag::kHelled)
					&& HELL_DURATION(d->character))
					sprintf(buf + strlen(buf), "Будет в аду : %ld час [%s].\r\n",
							static_cast<long>((HELL_DURATION(d->character) - time(nullptr)) / 3600),
							HELL_REASON(d->character) ? HELL_REASON(d->character) : "-");

				if (!d->character->IsFlagged(EPlrFlag::kRegistred)
					&& UNREG_DURATION(d->character)) {
					sprintf(buf + strlen(buf), "Не сможет заходить с одного IP : %ld час [%s].\r\n",
							static_cast<long>((UNREG_DURATION(d->character) - time(nullptr)) / 3600),
							UNREG_REASON(d->character) ? UNREG_REASON(d->character) : "-");
				}

				if (buf[0]) {
					SendMsgToChar(GET_NAME(d->character), ch);
					SendMsgToChar("\r\n", ch);
					SendMsgToChar(buf, ch);
				}
			}
			break;
		case 11:        // show paths
			if (self) {
				std::string out = print_zone_exits(world[ch->in_room]->zone_rn);
				out += print_zone_enters(world[ch->in_room]->zone_rn);
				page_string(ch->desc, out);
			} else if (*value && is_number(value)) {
				for (zvn = atoi(value), zrn = 0;
					 zone_table[zrn].vnum != zvn && zrn < static_cast<ZoneRnum>(zone_table.size());
					 zrn++) {
					// empty
				}

				if (zrn < static_cast<ZoneRnum>(zone_table.size())) {
					auto out = print_zone_exits(zrn);
					out += print_zone_enters(zrn);
					page_string(ch->desc, out);
				} else {
					SendMsgToChar("Нет такой зоны.\r\n", ch);
					return;
				}
			} else {
				SendMsgToChar("Какую зону показать?\r\n", ch);
				return;
			}
			break;
		case 12:        // show loadrooms
			break;
		case 13:        // show skills
			if (!*value) {
				SendMsgToChar("Уточните имя.\r\n", ch);
				return;
			}
			if (!(vict = get_player_vis(ch, value, EFind::kCharInWorld))) {
				SendMsgToChar("Нет такого игрока.\r\n", ch);
				return;
			}
			DisplaySkills(vict, ch);
			break;
		case 14:        // show spells
			if (!*value) {
				SendMsgToChar("Уточните имя.\r\n", ch);
				return;
			}
			if (!(vict = get_player_vis(ch, value, EFind::kCharInWorld))) {
				SendMsgToChar("Нет такого игрока.\r\n", ch);
				return;
			}
			DisplaySpells(vict, ch, false);
			break;
		case 15:        //Show ban.
			if (!*value) {
				ban->ShowBannedIp(BanList::SORT_BY_DATE, ch);
				return;
			}
			ban->ShowBannedIpByMask(BanList::SORT_BY_DATE, ch, value);
			break;
		case 16:        // show features
			if (!*value) {
				SendMsgToChar("Уточните имя.\r\n", ch);
				return;
			}
			if (!(vict = get_player_vis(ch, value, EFind::kCharInWorld))) {
				SendMsgToChar("Нет такого игрока.\r\n", ch);
				return;
			}
			DisplayFeats(vict, ch, false);
			break;
		case 17:        // show glory
			GloryMisc::show_log(ch, value);
			break;
		case 18:        // show crc
			FileCRC::show(ch);
			break;
		case 19: room_spells::ShowAffectedRooms(ch);
			break;
		case 20: // money
			MoneyDropStat::print(ch);
			break;
		case 21: // expgain
			ZoneExpStat::print_gain(ch);
			break;
		case 22: // runes stat
			print_rune_stats(ch);
			break;
		case 23: { // mobstat
			if (*value && is_number(value)) {
				if (*value1 && is_number(value1)) {
					mob_stat::ShowZoneMobKillsStat(ch, atoi(value), atoi(value1));
				} else {
					mob_stat::ShowZoneMobKillsStat(ch, atoi(value), 0);
				}
			} else {
				SendMsgToChar("Формат команды: show mobstat внум-зоны <месяцев>.\r\n", ch);
			}
			break;
		}
		case 24: // bosses
			if (*value && !strcmp(value, "-l")) {
				print_mob_bosses(ch, true);
			} else {
				print_mob_bosses(ch, false);
			}
			break;
		case 25: // remort
			Remort::show_config(ch);
			break;
		case 26: { //Apply
			if (!*value) {
				SendMsgToChar("Уточните имя.\r\n", ch);
				return;
			}
			if (!(vict = get_player_vis(ch, value, EFind::kCharInWorld))) {
				SendMsgToChar("Нет такого игрока.\r\n", ch);
				return;
			}
			show_apply(ch, vict);
			break;
		}
		case 27: // worlds
			if (*value && is_number(value)) {
				print_worlds_vars(ch, atol(value));
			}
			else if (*value && !str_cmp("all", value)) {
				print_worlds_vars(ch, std::nullopt);
			} else {
				SendMsgToChar("Формат команды: show worlds номер-контекста|all.\r\n", ch);
			}
			break;
		case 28: // triggers
			print_event_list(ch);
			break;
		case 29: // class
			ShowClassInfo(ch, value, value1);
			break;
		case 30: // guild
			ShowGuildInfo(ch, value);
			break;
		case 31: // currency
			ShowCurrencyInfo(ch, value);
			break;
		case 32: // spell
			ShowSpellInfo(ch, value);
			break;
		case 33: // feat
			ShowFeatInfo(ch, value);
			break;
		case 34: // ability
			ShowAbilityInfo(ch, value);
			break;
		case 35: {// account
			if (!*value) {
				SendMsgToChar("Уточните имя.\r\n", ch);
				return;
			}
			Player t_chdata;
			Player *chdata = &t_chdata;
			if (LoadPlayerCharacter(value, chdata, ELoadCharFlags::kFindId) < 0) {
				SendMsgToChar("Нет такого игрока.\r\n", ch);
				return;
			}
			chdata->get_account()->show_players(ch);
			chdata->get_account()->show_history_logins(ch);
			break;
		}
		case 36: // shops
			do_shops_list(ch);
			break;
		case 37: // runes list
			ListSpellCreate(ch);
			break;
		case 38: // dungeons
			dungeons::ListDungeons(ch);
			break;
		case 39: // mob class
			ShowMobClassInfo(ch, value);
			break;
		default: SendMsgToChar("Извините, неверная команда.\r\n", ch);
			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
