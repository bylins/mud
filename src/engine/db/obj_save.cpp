/*   File: objsave.cpp                                     Part of Bylins  *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

// * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>

#include "engine/core/char_handler.h"
#include "gameplay/mechanics/equipment.h"
#include "obj_save.h"
#include "gameplay/mechanics/groups.h"
#include "administration/privilege.h"
#include "engine/db/global_objects.h"
#include "gameplay/economics/currencies.h"
#include "utils/grammar/gender.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/ai/special_messages.h"

#include <fmt/format.h>

#include "world_objects.h"
#include "world_characters.h"
#include "obj_prototypes.h"
#include "engine/ui/color.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/mechanics/liquid.h"
#include "utils/file_crc.h"
#include "gameplay/mechanics/named_stuff.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/weather.h"
#include "utils/utils_time.h"
#include "player_index.h"

#include <sys/stat.h>
#include <fmt/format.h>
#include "engine/observability/helpers.h"
#include "engine/observability/metrics.h"
#include "utils/tracing/trace_manager.h"

#include <unordered_map>

// #3440: кэш хешей содержимого файла объектов по uid игрока. Для периодического
// crash-сейва (RENT_CRASH) позволяет пропустить дисковую запись, если набор
// предметов не изменился. Заполняется только при реальной записи файла.
static std::unordered_map<long, std::size_t> g_obj_crash_save_hash;

const int LOC_INVENTORY = 0;
//const int MAX_BAG_ROWS = 5;
//const int ITEM_DESTROYED = 100;

// header block for rent files.  BEWARE: Changing it will ruin rent files  //

extern IndexData *mob_index;
extern DescriptorData *descriptor_list;
extern int rent_file_timeout, crash_file_timeout;
extern int free_crashrent_period;
extern int free_rent;
extern RoomRnum r_helled_start_room;
extern RoomRnum r_named_start_room;
#include "gameplay/ai/subcmd_resolver.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/sight.h"
extern RoomRnum r_unreg_start_room;

#define RENTCODE(number) (player_table[(number)].timer->rent.rentcode)
#define GET_INDEX(ch) (GetPlayerTablePosByName(GET_NAME(ch)))

// Extern functions
void do_tell(CharData *ch, char *argument, int cmd, int subcmd);
//int receptionist(CharData *ch, void *me, int cmd, char *argument);
int invalid_no_class(CharData *ch, const ObjData *obj);
int invalid_anti_class(CharData *ch, const ObjData *obj);
int invalid_unique(CharData *ch, const ObjData *obj);
int min_rent_cost(CharData *ch);
extern bool IsObjFromSystemZone(ObjVnum vnum);
// local functions
void Crash_extract_norent_eq(CharData *ch);
int auto_equip(CharData *ch, ObjData *obj, int location);
int Crash_report_unrentables(CharData *ch, CharData *recep, ObjData *obj);
void Crash_report_rent(CharData *ch, CharData *recep, ObjData *obj,
					   int *cost, long *n_items, int rentshow, int factor, int equip, int recursive);
void Crash_save(std::stringstream &write_buffer, int iplayer, ObjData *obj, int location, int savetype);
void Crash_rent_deadline(CharData *ch, CharData *recep, long cost);
void Crash_restore_weight(ObjData *obj);
void Crash_extract_objs(ObjData *obj);
int Crash_is_unrentable(CharData *ch, ObjData *obj);
void Crash_extract_norents(CharData *ch, ObjData *obj);
int Crash_calculate_rent(ObjData *obj);
int Crash_calculate_rent_eq(ObjData *obj);
void Crash_save_all_rent();
int Crash_calc_charmee_items(CharData *ch);

#define DIV_CHAR  '#'
#define END_CHAR  '$'
#define END_LINE  '\n'
#define END_LINES '~'
#define COM_CHAR  '*'

// Rent codes
enum {
	RENT_UNDEF,    // не используется
	RENT_CRASH,    // регулярный автосейв на случай креша
	RENT_RENTED,   // зарентился сам
	RENT_CRYO,     // не используется
	RENT_FORCED,   // заренчен на ребуте
	RENT_TIMEDOUT, // заренчен после idle_rent_time
};

// issue #3568: все вызывающие передают буфер размера kMaxStringLength, поэтому
// пишем не дальше него -- иначе строка из файла длиннее буфера затирала память
// за границей (порча кучи/стека). При переполнении обрезаем и пишем в лог.
int get_buf_line(char **source, char *target) {
	char *otarget = target;
	char *const limit = target + kMaxStringLength - 1;    // резерв под '\0'
	bool truncated = false;
	int empty = true;

	*target = '\0';
	for (; **source && **source != DIV_CHAR && **source != END_CHAR; (*source)++) {
		if (**source == '\r')
			continue;
		if (**source == END_LINE) {
			if (empty || *otarget == COM_CHAR) {
				target = otarget;
				*target = '\0';
				continue;
			}
			(*source)++;
			return (true);
		}
		if (target >= limit) {    // граница буфера: дочитываем строку, но не пишем
			if (!truncated) {
				truncated = true;
				char msg[256];
				snprintf(msg, sizeof(msg),
						"SYSERR: get_buf_line: строка обрезана до %d байт (предотвращено переполнение буфера)",
						kMaxStringLength);
				mudlog(msg, NRM, kLvlGod, SYSLOG, true);   // #3568: в mudlog -- боги видят сразу
			}
			continue;
		}
		*target = **source;
		if (!isspace(static_cast<unsigned char>(*target++)))
			empty = false;
		*target = '\0';
	}
	return (false);
}

int get_buf_lines(char **source, char *target) {
	char *const limit = target + kMaxStringLength - 1;    // резерв под '\0'
	bool truncated = false;

	*target = '\0';
	for (; **source && **source != DIV_CHAR && **source != END_CHAR; (*source)++) {
		if (**source == END_LINES) {
			(*source)++;
			if (**source == '\r')
				(*source)++;
			if (**source == END_LINE)
				(*source)++;
			return (true);
		}
		if (target >= limit) {    // граница буфера: дочитываем блок, но не пишем
			if (!truncated) {
				truncated = true;
				char msg[256];
				snprintf(msg, sizeof(msg),
						"SYSERR: get_buf_lines: блок обрезан до %d байт (предотвращено переполнение буфера)",
						kMaxStringLength);
				mudlog(msg, NRM, kLvlGod, SYSLOG, true);   // #3568: в mudlog -- боги видят сразу
			}
			continue;
		}
		*(target++) = **source;
		*target = '\0';
	}
	return (false);
}

// Данная процедура выбирает предмет из буфера.
// с поддержкой нового формата вещей игроков [от 10.12.04].
ObjData::shared_ptr read_one_object_new(char **data, int *error) {
	char buffer[kMaxStringLength];
	char read_line[kMaxStringLength];
	int t[2];
	int vnum;
	ObjRnum rnum = -1;    // vnum->rnum считаем один раз: для создания и для проверки прототипа
	ObjData::shared_ptr object;

	// Станем на начало предмета
	for (; **data != DIV_CHAR; (*data)++) {
		if (!**data || **data == END_CHAR) {
			*error = 1;
			return object;
		}
	}
	// Пропустим #
	(*data)++;
	// Считаем vnum предмета
	if (!get_buf_line(data, buffer)) {
		*error = 2;
		return object;
	}
	vnum = atoi(buffer);
	if (!vnum) {
		*error = 3;
		return object;
	}
	// Если без прототипа, создаем новый. Иначе читаем прототип и возвращаем NULL,
	// если прототип не существует.
	if (vnum < 0) {
		object = world_objects.create_blank();
	} else {
		rnum = GetObjRnum(vnum);
		if (rnum < 0) {
			log("Object (V) %d does not exist in database.", vnum);
			*error = 5;
			return nullptr;
		}
		object = world_objects.create_from_prototype_by_rnum(rnum);
		if (!object) {
			*error = 5;
			return nullptr;
		}
	}
	// Далее найденные параметры присваиваем к прототипу
	while (get_buf_lines(data, buffer)) {
		ExtractTagFromArgument(buffer, read_line);

		//if (read_line != NULL) read_line cannot be NULL because it is a local array
		{
			// Чтобы не портился прототип вещи некоторые параметры изменяются лишь у вещей с vnum < 0
			if (!strcmp(read_line, "Alia")) {
				*error = 6;
				object->set_aliases(buffer);
			} else if (!strcmp(read_line, "Prnt")) {
			// 13.05.2024 можно удалить через месяц
			} else if (!strcmp(read_line, "Pad0")) {
				*error = 7;
				object->set_PName(grammar::ECase::kNom, buffer);
				object->set_short_description(buffer);
			} else if (!strcmp(read_line, "Pad1")) {
				*error = 8;
				object->set_PName(grammar::ECase::kGen, buffer);
			} else if (!strcmp(read_line, "Pad2")) {
				*error = 9;
				object->set_PName(grammar::ECase::kDat, buffer);
			} else if (!strcmp(read_line, "Pad3")) {
				*error = 10;
				object->set_PName(grammar::ECase::kAcc, buffer);
			} else if (!strcmp(read_line, "Pad4")) {
				*error = 11;
				object->set_PName(grammar::ECase::kIns, buffer);
			} else if (!strcmp(read_line, "Pad5")) {
				*error = 12;
				object->set_PName(grammar::ECase::kPre, buffer);
			} else if (!strcmp(read_line, "Desc")) {
				*error = 13;
				object->set_description(buffer);
			} else if (!strcmp(read_line, "ADsc")) {
				*error = 14;
				if (!strcmp(buffer, "NULL")) {
					object->set_action_description("");
				} else {
					object->set_action_description(buffer);
				}
			} else if (!strcmp(read_line, "Lctn")) {
				*error = 5;
				object->set_worn_on(atoi(buffer));
			} else if (!strcmp(read_line, "Skil")) {
				*error = 15;
				int tmp_a_, tmp_b_;
				if (sscanf(buffer, "%d %d", &tmp_a_, &tmp_b_) != 2) {
					continue;
				}
				object->set_skill(static_cast<ESkill>(tmp_a_), tmp_b_);
			} else if (!strcmp(read_line, "Maxx")) {
				*error = 16;
				object->set_maximum_durability(atoi(buffer));
			} else if (!strcmp(read_line, "Curr")) {
				*error = 17;
				object->set_current_durability(atoi(buffer));
			} else if (!strcmp(read_line, "Mter")) {
				*error = 18;
				object->set_material(static_cast<EObjMaterial>(atoi(buffer)));
			} else if (!strcmp(read_line, "Sexx")) {
				*error = 19;
				object->set_sex(static_cast<EGender>(atoi(buffer)));
			} else if (!strcmp(read_line, "Tmer")) {
				*error = 20;
				const int saved_timer = atoi(buffer);
				// Защита от мусорных таймеров (UNLIMITED_TIMER, INT_MAX и
				// прочие следы старых багов decay_manager в otransform/
				// oedit/обмена шмотками - см. #3213). При парсинге мира
				// таймеры > 99999 уже клампятся, тут делаем то же для
				// сохранений игроков. Если в пфайле плохое значение,
				// оставляем то, что пришло из прототипа в
				// create_from_prototype_by_vnum выше. Отрицательные значения
				// валидны (например, -1 означает "таймер не уменьшается").
				if (saved_timer <= 99999) {
					object->set_timer(saved_timer);
				}
			} else if (!strcmp(read_line, "Spll")) {
				*error = 21;
				// issue #3581: obj->spell -- мёртвое поле; тег игнорируем (совместимость со старыми рентами).
			} else if (!strcmp(read_line, "Levl")) {
				*error = 22;
				// issue #3581: obj->spell/level -- мёртвая пара; тег игнорируем (совместимость со старыми рентами).
			} else if (!strcmp(read_line, "Affs")) {
				*error = 23;
				object->SetWeaponAffectFlags(clear_flags);
				object->load_affect_flags(buffer);
			} else if (!strcmp(read_line, "Anti")) {
				*error = 24;
				object->set_anti_flags(clear_flags);
				object->load_anti_flags(buffer);
			} else if (!strcmp(read_line, "Nofl")) {
				*error = 25;
				object->set_no_flags(clear_flags);
				object->load_no_flags(buffer);
			} else if (!strcmp(read_line, "Extr")) {
				*error = 26;
				object->set_extra_flags(clear_flags);
				object->load_extra_flags(buffer);
			} else if (!strcmp(read_line, "Wear")) {
				*error = 27;
				ObjData::wear_flags_t wear_flags = 0;
				asciiflag_conv(buffer, &wear_flags);
				object->set_wear_flags(wear_flags);
			} else if (!strcmp(read_line, "Type")) {
				*error = 28;
				object->set_type(static_cast<EObjType>(atoi(buffer)));
			} else if (!strcmp(read_line, "Val0")) {
				*error = 29;
				object->set_val(0, atoi(buffer));
			} else if (!strcmp(read_line, "Val1")) {
				*error = 30;
				object->set_val(1, atoi(buffer));
			} else if (!strcmp(read_line, "Val2")) {
				*error = 31;
				object->set_val(2, atoi(buffer));
			} else if (!strcmp(read_line, "Val3")) {
				*error = 32;
				object->set_val(3, atoi(buffer));
			} else if (!strcmp(read_line, "Weig")) {
				*error = 33;
				int wt = atoi(buffer);
				if (wt != 0) { //временно 17,09,25
					object->set_weight(wt);
				}
			} else if (!strcmp(read_line, "Cost")) {
				*error = 34;
				object->set_cost(atoi(buffer));
			} else if (!strcmp(read_line, "Rent")) {
				*error = 35;
				object->set_rent_off(atoi(buffer));
			} else if (!strcmp(read_line, "RntQ")) {
				*error = 36;
				object->set_rent_on(atoi(buffer));
			} else if (!strcmp(read_line, "Ownr")) {
				*error = 37;
				object->set_owner(atoi(buffer));
			} else if (!strcmp(read_line, "Mker")) {
				*error = 38;
				object->set_crafter_uid(atoi(buffer));
			} else if (!strcmp(read_line, "Afc0")) {
				*error = 40;
				sscanf(buffer, "%d %d", t, t + 1);
				object->set_affected(0, static_cast<EApply>(t[0]), t[1]);
			} else if (!strcmp(read_line, "Afc1")) {
				*error = 41;
				sscanf(buffer, "%d %d", t, t + 1);
				object->set_affected(1, static_cast<EApply>(t[0]), t[1]);
			} else if (!strcmp(read_line, "Afc2")) {
				*error = 42;
				sscanf(buffer, "%d %d", t, t + 1);
				object->set_affected(2, static_cast<EApply>(t[0]), t[1]);
			} else if (!strcmp(read_line, "Afc3")) {
				*error = 43;
				sscanf(buffer, "%d %d", t, t + 1);
				object->set_affected(3, static_cast<EApply>(t[0]), t[1]);
			} else if (!strcmp(read_line, "Afc4")) {
				*error = 44;
				sscanf(buffer, "%d %d", t, t + 1);
				object->set_affected(4, static_cast<EApply>(t[0]), t[1]);
			} else if (!strcmp(read_line, "Afc5")) {
				*error = 45;
				sscanf(buffer, "%d %d", t, t + 1);
				object->set_affected(5, static_cast<EApply>(t[0]), t[1]);
			} else if (!strcmp(read_line, "Afc6")) {
				*error = 456;
				sscanf(buffer, "%d %d", t, t + 1);
				object->set_affected(6, static_cast<EApply>(t[0]), t[1]);
			} else if (!strcmp(read_line, "Afc7")) {
				*error = 457;
				sscanf(buffer, "%d %d", t, t + 1);
				object->set_affected(7, static_cast<EApply>(t[0]), t[1]);
			} else if (!strcmp(read_line, "Edes")) {
				*error = 46;
				ExtraDescription new_descr;
				new_descr.keyword = buffer;
				// Маркер "None" в старых рентах означал "описаний нет"; больше не
				// пишется -- просто пропускаем (описания прототипа не трогаем).
				if (new_descr.keyword != "None") {
					if (!get_buf_lines(data, buffer)) {
						*error = 47;
						return (object);
					}
					new_descr.description = buffer;
					// Мерж по ключу: объект -- копия прототипа, в ренте лежат только
					// отличия. Есть описание с таким ключом -- меняем его текст,
					// нет -- добавляем новое.
					auto &descs = object->ex_descriptions();
					ExtraDescription *same_key = nullptr;
					for (auto &d : descs) {
						if (!str_cmp(d.keyword.c_str(), new_descr.keyword.c_str())) {
							same_key = &d;
							break;
						}
					}
					if (same_key) {
						same_key->description = std::move(new_descr.description);
					} else {
						descs.push_back(std::move(new_descr));
					}
				}
			} else if (!strcmp(read_line, "Ouid")) {
				*error = 48;
				object->set_unique_id(atoi(buffer));
			} else if (!strcmp(read_line, "TSpl")) {
				*error = 49;
				std::stringstream text(buffer);
				std::string tmp_buf;

				while (std::getline(text, tmp_buf)) {
					utils::Trim(tmp_buf);
					if (tmp_buf.empty() || tmp_buf[0] == '~') {
						break;
					}
					if (sscanf(tmp_buf.c_str(), "%d %d", t, t + 1) != 2) {
						*error = 50;
						return object;
					}
					object->add_timed_spell(static_cast<ESpell>(t[0]), t[1]);
				}
			} else if (!strcmp(read_line, "Mort")) {
				*error = 51;
				object->set_minimum_remorts(atoi(buffer));
			} else if (!strcmp(read_line, "DGsc")) {
				*error = 67;
				object->set_dgscript_field(buffer);
			} else if (!strcmp(read_line, "Ench")) {
				ObjectEnchant::enchant tmp_aff;
				std::stringstream text(buffer);
				std::string tmp_buf;

				while (std::getline(text, tmp_buf)) {
					utils::Trim(tmp_buf);
					if (tmp_buf.empty() || tmp_buf[0] == '~') {
						break;
					}
					switch (tmp_buf[0]) {
						case 'I': tmp_aff.name_ = tmp_buf.substr(1);
							utils::Trim(tmp_aff.name_);
							if (tmp_aff.name_.empty()) {
								tmp_aff.name_ = "<null>";
							}
							break;
						case 'T':
							if (sscanf(tmp_buf.c_str(), "T %d", &tmp_aff.type_) != 1) {
								*error = 52;
								return object;
							}
							break;
						case 'A': {
							obj_affected_type tmp_affected;
							int location = 0;
							if (sscanf(tmp_buf.c_str(), "A %d %d", &location, &tmp_affected.modifier) != 2) {
								*error = 53;
								return object;
							}
							tmp_affected.location = static_cast<EApply>(location);
							tmp_aff.affected_.push_back(tmp_affected);
							break;
						}
						case 'F':
							if (sscanf(tmp_buf.c_str(), "F %s", buf2) != 1) {
								*error = 54;
								return object;
							}
							tmp_aff.affects_flags_.from_string(buf2);
							break;
						case 'E':
							if (sscanf(tmp_buf.c_str(), "E %s", buf2) != 1) {
								*error = 55;
								return object;
							}
							tmp_aff.extra_flags_.from_string(buf2);
							break;
						case 'N':
							if (sscanf(tmp_buf.c_str(), "N %s", buf2) != 1) {
								*error = 56;
								return object;
							}
							tmp_aff.no_flags_.from_string(buf2);
							break;
						case 'W':
							if (sscanf(tmp_buf.c_str(), "W %d", &tmp_aff.weight_) != 1) {
								*error = 57;
								return object;
							}
							break;
						case 'B':
							if (sscanf(tmp_buf.c_str(), "B %d", &tmp_aff.ndice_) != 1) {
								*error = 58;
								return object;
							}
							break;
						case 'C':
							if (sscanf(tmp_buf.c_str(), "C %d", &tmp_aff.sdice_) != 1) {
								*error = 59;
								return object;
							}
							break;
					}
				}

				object->add_enchant(tmp_aff);
			} else if (!strcmp(read_line, "Clbl")) // текст метки
			{
				*error = 60;
				if (!object->get_custom_label()) {
					object->set_custom_label(new custom_label());
				}
				object->get_custom_label()->text_label = buffer;
			} else if (!strcmp(read_line, "ClID")) // id чара
			{
				*error = 61;
				if (!object->get_custom_label()) {
					object->set_custom_label(new custom_label());
				}

				object->get_custom_label()->author = atoi(buffer);
				if (object->get_custom_label()->author > 0) {
					for (std::size_t i = 0; i < player_table.size(); i++) {
						if (player_table[i].uid() == object->get_custom_label()->author) {
							object->get_custom_label()->author_mail = player_table[i].mail;
							break;
						}
					}
				}
			} else if (!strcmp(read_line, "ClCl")) // аббревиатура клана
			{
				*error = 62;
				if (!object->get_custom_label()) {
					object->set_custom_label(new custom_label());
				}
				object->get_custom_label()->clan_abbrev = buffer;
			} else if (!strcmp(read_line, "Vals")) {
				*error = 63;
				if (!object->init_values_from_file(buffer)) {
					return object;
				}
			} else if (!strcmp(read_line, "Rnme")) {
				*error = 64;
				int tmp_int = atoi(buffer);
				object->set_is_rename(tmp_int == 1);
			} else if (!strcmp(read_line, "Ctmr")) {
				*error = 65;
				object->set_craft_timer(atoi(buffer));
			} else if (!strcmp(read_line, "Ozne")) {
				*error = 66;
				object->set_vnum_zone_from(atoi(buffer));
			} else {
				snprintf(buf, kMaxStringLength, "WARNING: \"%s\" is not valid key for character items! [value=\"%s\"]",
						 read_line, buffer);
				mudlog(buf, NRM, kLvlGreatGod, ERRLOG, true);
			}
		}
	}
	*error = 0;

	// Проверить вес фляг и т.п.
	if (object->get_type() == EObjType::kLiquidContainer
		|| object->get_type() == EObjType::kFountain) {
		if (object->get_weight() < GET_OBJ_VAL(object, 1)) {
			object->set_weight(GET_OBJ_VAL(object, 1) + 5);
		}
	}
	// проставляем имя жидкости
	if (object->get_type() == EObjType::kLiquidContainer) {
		name_from_drinkcon(object.get());
		if (GET_OBJ_VAL(object, 1) && GET_OBJ_VAL(object, 2)) {
			name_to_drinkcon(object.get(), GET_OBJ_VAL(object, 2));
		}
	}
	// Проверка на ингры
	if (object->get_type() == EObjType::kMagicComponent) {
		int err = im_assign_power(object.get());
		if (err) {
			*error = 100 + err;
		}
	}
	if (object->has_flag(EObjFlag::kNamed))//Именной предмет
	{
		object->cleanup_script();
	}
	ConvertDrinkconSkillField(object.get(), false);
	ConvertDrinkPoisonField(object.get(), false);   // issue.potion-hotfix: migrate player-held drink val[3]
	ConvertPotionToEValueKey(object.get(), false);
	ConvertSpellItemToEValueKey(object.get(), false);   // issue.magic-items: migrate held scroll/wand/staff
	ConvertDrinkconLiquidCore(object.get(), false);   // issue.magic-items-hotfix: reconcile held drink liquid core
	object->remove_incorrect_values_keys(object->get_type());
	object->set_extra_flag(EObjFlag::kTicktimer);
	// ВРЕМЕННЫЙ КОСТЫЛЬ (2026-06-19, issue #3459/#3490): краш-баг крафта (#3486) вешал
	// kNodrop ("!бросить") на предметы, у прототипа которых его нет. Снимаем флаг при
	// загрузке, если в прототипе-оригинале kNodrop нет, чтобы почистить уже испорченный
	// у игроков стаф. (Заклинание "проклясть" на таких предметах тоже снимется -- это
	// осознанный компромисс ради очистки массовой порчи.)
	// УДАЛИТЬ после 2026-07-19: к тому времени база игроков прочитается и очистится,
	// отдельно конвертировать пфайлы не нужно.
	if (rnum >= 0 && object->has_flag(EObjFlag::kNodrop)
		&& !obj_proto[rnum]->has_flag(EObjFlag::kNodrop)) {
		// rnum -- прототип-оригинал по vnum (в obj-файле всегда оригиналы), посчитан выше
		object->unset_extraflag(EObjFlag::kNodrop);
	}
	world_objects.decay_manager().insert(object.get());
	return (object);
}

// Есть ли в прототипе точно такое же экстра-описание. Если да -- в ренте его не
// пишем: при загрузке оно приходит из копии прототипа. Ключ сравниваем
// регистронезависимо (как матчинг ключей в игре), а текст -- точно, побайтно
// (== короче str_cmp: сначала сверяет длину, при равной делает memcmp, и не
// даункейсит по 2-3 сотни символов). Иначе описание, отличающееся хотя бы
// регистром, ошибочно считалось бы совпавшим и терялось при сохранении.
inline bool proto_has_descr(const ExtraDescription &odesc, const std::vector<ExtraDescription> &pdesc) {
	for (const auto &desc : pdesc) {
		if (!str_cmp(odesc.keyword.c_str(), desc.keyword.c_str())
			&& odesc.description == desc.description) {
			return true;
		}
	}

	return false;
}

void WriteMagicComponentItem(std::stringstream &out, ObjData *object, int location) {
	out << "#" << object->get_vnum() << "\n";
	// Положение в экипировке/контейнере: без него ингредиент из сумки
	// при загрузке оказывается в инвентаре (issue #3221).
	if (location) {
		out << "Lctn: " << location << "~\n";
	}
	out << "Ouid: " << object->get_unique_id() << "~\n";
	out << "Tmer: " << object->CObjectPrototype::get_timer() << "~\n";
	out << "Val1: " << GET_OBJ_VAL(object, 1) << "~\n";
	out << "Val2: " << GET_OBJ_VAL(object, 2) << "~\n";
	out << "Val3: " << GET_OBJ_VAL(object, 3) << "~\n";
	// Имя магкомпонента заполнено моба-источником (im_assign_power ставит is_rename), а короткий
	// формат его раньше не сохранял -- после ребута флаг терялся, и правка прототипа в olc
	// затирала имя шаблоном "@p1". Пишем флаг, чтобы olc_update_object восстановил имя.
	if (object->get_is_rename()) {
		out << "Rnme: 1~\n";
	}
	if (object->get_custom_label()) {
		out << "Clbl: " << object->get_custom_label()->text_label << "~\n";
		out << "ClID: " << object->get_custom_label()->author << "~\n";
		if (!object->get_custom_label()->clan_abbrev.empty()) {
			out << "ClCl: " << object->get_custom_label()->clan_abbrev << "~\n";
		}
	}
}

// Данная процедура помещает предмет в буфер
// [ ИСПОЛЬЗУЕТСЯ В НОВОМ ФОРМАТЕ ВЕЩЕЙ ПЕРСОНАЖА ОТ 10.12.04 ]
void write_one_object(std::stringstream &out, ObjData *object, int location) {
	if (object->get_type() == EObjType::kMagicComponent) {
		WriteMagicComponentItem(out, object, location);
		return;
	}
	ObjRnum orn = object->get_rnum();
	CObjectPrototype::shared_ptr proto_sp;

	if (object->get_parent_rnum() > -1) {
		proto_sp = obj_proto[object->get_parent_rnum()];
		out << "#" << object->get_parent_vnum();
	} else {
		proto_sp = obj_proto[orn];
		out << "#" << GET_OBJ_VNUM(object);
	}
	// Кешируем сырой указатель: shared_ptr::operator-> в горячем цикле
	// добавляет заметный overhead (atomic-ссылки, обёртки).
	const CObjectPrototype* const p = proto_sp.get();
	out << "\n";
	// Положение в экипировке (сохраняем только если > 0)
	if (location) {
		out << "Lctn: " << location << "~\n";
	}
	if (GET_OBJ_VNUM(object) >= 0) {
		out << "Ouid: " << object->get_unique_id() << "~\n";
		if (object->get_aliases() != p->get_aliases()) {
			out << "Alia: " << object->get_aliases() << "~\n";
		}
		// Падежи
		for (int i = grammar::ECase::kFirstCase; i <= grammar::ECase::kLastCase; ++i) {
			const auto name_case = static_cast<grammar::ECase>(i);
			const auto& obj_pname = object->get_PName(name_case);
			const auto& proto_pname = p->get_PName(name_case);
			if (obj_pname != proto_pname) {
				out << "Pad" << i << ": " << obj_pname << "~\n";
			}
		}
		if (!p->get_description().empty()
			&& object->get_description() != p->get_description()) {
			out << "Desc: " << object->get_description() << "~\n";
		}
		if (!object->get_action_description().empty()
			&& object->get_action_description() != p->get_action_description()) {
			out << "ADsc: " << object->get_action_description() << "~\n";
		}
		if (object->has_skills()) {
			// get_skills() без аргументов возвращает const ref на m_skills;
			// избегаем копии всей std::map<ESkill,int>.
			const auto& obj_skills = object->get_skills();
			const auto& proto_skills = p->get_skills();
			if (obj_skills != proto_skills) {
				for (const auto &sk : obj_skills) {
					out << "Skil: " << to_underlying(sk.first) << " " << sk.second << "~\n";
				}
			}
		}
		if (object->get_maximum_durability() != p->get_maximum_durability()) {
			out << "Maxx: " << object->get_maximum_durability() << "~\n";
		}
		if (object->get_current_durability() != p->get_current_durability()) {
			out << "Curr: " << object->get_current_durability() << "~\n";
		}
		if (object->get_material() != p->get_material()) {
			out << "Mter: " << object->get_material() << "~\n";
		}
		if (GET_OBJ_SEX(object) != GET_OBJ_SEX(p)) {
			out << "Sexx: " << static_cast<int>(GET_OBJ_SEX(object)) << "~\n";
		}
		// Таймер. Сохраняем актуальный остаток: ObjData::get_timer()
		// учитывает deadline в decay_manager, поэтому для свежезагруженного
		// объекта с m_timer == proto_timer уже после пары игровых часов мы
		// увидим правильное оставшееся время. Без этого Tmer не писался в
		// файл игрока (m_timer сам по себе не тикает), и прогресс распада
		// терялся на save/reload (issue #3189).
		// Сохранение идёт из главного цикла (heartbeat, shutdown), обращение
		// к decay_manager здесь безопасно.
		const int obj_timer = object->get_timer();
		const int proto_timer = p->get_timer();
		if (obj_timer != proto_timer) {
			out << "Tmer: " << obj_timer << "~\n";
		}
		// issue #3581: obj->spell/level -- мёртвая пара, в рент больше не пишем.
		if (object->get_is_rename()) {
			out << "Rnme: 1~\n";
		}
		if (object->get_craft_timer() > 0) {
			out << "Ctmr: " << object->get_craft_timer() << "~\n";
		}
		if (object->get_vnum_zone_from()) {
			out << "Ozne: " << object->get_vnum_zone_from() << "~\n";
		}
		const auto& obj_aff = object->get_affect_flags();
		const auto& proto_aff = p->get_affect_flags();
		if (obj_aff != proto_aff) {
			out << "Affs: " << obj_aff.to_numeric_string() << "~\n";
		}
		const auto& obj_anti = object->get_anti_flags();
		const auto& proto_anti = p->get_anti_flags();
		if (obj_anti != proto_anti) {
			out << "Anti: " << obj_anti.to_numeric_string() << "~\n";
		}
		const auto& obj_nofl = object->get_no_flags();
		const auto& proto_nofl = p->get_no_flags();
		if (obj_nofl != proto_nofl) {
			out << "Nofl: " << obj_nofl.to_numeric_string() << "~\n";
		}
		// Экстра флаги. Временные флаги (kBloody, рантайм-выставленный kNosell)
		// в файл не пишем. Снимаем их на копии FlagData без мутации самого
		// объекта -- иначе save-путь был бы не thread-safe и менял видимое
		// состояние предмета для игроков на момент сериализации.
		FlagData extra_to_save = object->get_extra_flags();
		extra_to_save.unset(EObjFlag::kBloody);
		if (object->has_flag(EObjFlag::kNosell)
			&& !p->has_flag(EObjFlag::kNosell)) {
			extra_to_save.unset(EObjFlag::kNosell);
		}
		if (extra_to_save != p->get_extra_flags()) {
			out << "Extr: " << extra_to_save.to_numeric_string() << "~\n";
		}
		if (object->get_wear_flags() != p->get_wear_flags()) {
			out << "Wear: " << object->get_wear_flags() << "~\n";
		}
		if (object->get_type() != p->get_type()) {
			out << "Type: " << object->get_type() << "~\n";
		}
		for (int i = 0; i < 4; ++i) {
			if (GET_OBJ_VAL(object, i) != GET_OBJ_VAL(p, i)) {
				out << "Val" << i << ": " << GET_OBJ_VAL(object, i) << "~\n";
			}
		}
		if (object->get_weight() != p->get_weight()) {
			out << "Weig: " << object->get_weight() << "~\n";
		}
		if (object->get_cost() != p->get_cost()) {
			out << "Cost: " << object->get_cost() << "~\n";
		}
		if (object->get_rent_off() != p->get_rent_off()) {
			out << "Rent: " << object->get_rent_off() << "~\n";
		}
		if (object->get_rent_on() != p->get_rent_on()) {
			out << "RntQ: " << object->get_rent_on() << "~\n";
		}
		if (object->get_owner() != ObjData::DEFAULT_OWNER) {
			out << "Ownr: " << object->get_owner() << "~\n";
		}
		if (object->get_crafter_uid() != ObjData::DEFAULT_MAKER) {
			out << "Mker: " << object->get_crafter_uid() << "~\n";
		}
		for (int j = 0; j < kMaxObjAffect; ++j) {
			const auto &oaff = object->get_affected(j);
			const auto &paff = p->get_affected(j);
			if (oaff.location != paff.location
				|| oaff.modifier != paff.modifier) {
				out << "Afc" << j << ": " << oaff.location << " " << oaff.modifier << "~\n";
			}
		}
		// Диф экстра-описаний против прототипа: описания, совпадающие с
		// прототипом (ключ + текст), в ренте не пишем -- при загрузке они
		// приходят из копии прототипа. Пишем только новые/изменённые; при
		// загрузке мерж идёт по ключу (см. read_one_object_new).
		for (const auto &descr : object->get_ex_description()) {
			if (proto_has_descr(descr, p->get_ex_description())) {
				continue;
			}
			out << "Edes: " << descr.keyword << "~\n"
				<< descr.description << "~\n";
		}
		if (object->get_auto_mort_req() > 0) {
			out << "Mort: " << object->get_auto_mort_req() << "~\n";
		}
		if (!object->get_dgscript_field().empty()) {
			out << "DGsc: " << object->get_dgscript_field() << "~\n";
		}
		if (object->get_all_values() != p->get_all_values()) {
			out << object->serialize_values();
		}
	}
	if (object->has_timed_spell()) {
		out << object->timed_spell().print();
	}
	if (!object->get_enchants().empty()) {
		out << object->serialize_enchants();
	}
	if (object->get_custom_label()) {
		out << "Clbl: " << object->get_custom_label()->text_label << "~\n";
		out << "ClID: " << object->get_custom_label()->author << "~\n";
		if (!object->get_custom_label()->clan_abbrev.empty()) {
			out << "ClCl: " << object->get_custom_label()->clan_abbrev << "~\n";
		}
	}
}

int auto_equip(CharData *ch, ObjData *obj, int location) {
	// Lots of checks...
	if (location > 0)    // Was wearing it.
	{
		const int j = location - 1;
		switch (j) {
			case EEquipPos::kLight: break;

			case EEquipPos::kFingerR:
			case EEquipPos::kFingerL:
				if (!CAN_WEAR(obj, EWearFlag::kFinger))    // not fitting
				{
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kNeck:
			case EEquipPos::kChest:
				if (!CAN_WEAR(obj, EWearFlag::kNeck)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kBody:
				if (!CAN_WEAR(obj, EWearFlag::kBody)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kHead:
				if (!CAN_WEAR(obj, EWearFlag::kHead)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kLegs:
				if (!CAN_WEAR(obj, EWearFlag::kLegs)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kFeet:
				if (!CAN_WEAR(obj, EWearFlag::kFeet)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kHands:
				if (!CAN_WEAR(obj, EWearFlag::kHands)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kArms:
				if (!CAN_WEAR(obj, EWearFlag::kArms)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kShield:
				if (!CAN_WEAR(obj, EWearFlag::kShield)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kShoulders:
				if (!CAN_WEAR(obj, EWearFlag::kShoulders)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kWaist:
				if (!CAN_WEAR(obj, EWearFlag::kWaist)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kQuiver:
				if (!CAN_WEAR(obj, EWearFlag::kQuiver)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kWristR:
			case EEquipPos::kWristL:
				if (!CAN_WEAR(obj, EWearFlag::kWrist)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kWield:
				if (!CAN_WEAR(obj, EWearFlag::kWield)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kHold:
				if (!CAN_WEAR(obj, EWearFlag::kHold)) {
					location = LOC_INVENTORY;
				}
				break;

			case EEquipPos::kBoths:
				if (!CAN_WEAR(obj, EWearFlag::kBoth)) {
					location = LOC_INVENTORY;
				}
				break;
			default: location = LOC_INVENTORY;
				break;
		}

		if (location > 0)    // Wearable.
		{
			if (!GET_EQ(ch, j)) {
				// Check the characters's alignment to prevent them from being
				// zapped through the auto-equipping.
				if (HaveIncompatibleAlign(ch, obj) || invalid_anti_class(ch, obj) || invalid_no_class(ch, obj)
					|| NamedStuff::check_named(ch, obj, 0)) {
					location = LOC_INVENTORY;
				} else {
					EquipObj(ch, obj, j, CharEquipFlag::no_cast | CharEquipFlag::skip_total);
//					log("Equipped with %s %d", (obj)->short_description, j);
				}
			} else    // Oops, saved a player with double equipment?
			{
				char aeq[128];
				sprintf(aeq, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
				mudlog(aeq, BRF, kLvlImmortal, SYSLOG, true);
				location = LOC_INVENTORY;
			}
		}
	}
	if (location <= 0)    // Inventory
	{
		PlaceObjToInventory(obj, ch);
	}
	return (location);
}

int Crash_delete_files(const std::size_t index) {
	char filename[kMaxStringLength + 1];
	FILE *fl;
	int retcode = false;

	if (static_cast<int>(index) < 0) {
		return retcode;
	}

	auto name = player_table[index].name();

	//удаляем файл описания объектов
	if (!get_filename(name.c_str(), filename, kTextCrashFile)) {
		log("SYSERR: Error deleting objects file for %s - unable to resolve file name.", name.c_str());
		retcode = false;
	} else {
		if (!(fl = fopen(filename, "rb"))) {
			if (errno != ENOENT)    // if it fails but NOT because of no file
				log("SYSERR: Error deleting objects file %s (1): %s", filename, strerror(errno));
			retcode = false;
		} else {
			fclose(fl);
			// if it fails, NOT because of no file
			if (remove(filename) < 0 && errno != ENOENT) {
				log("SYSERR: Error deleting objects file %s (2): %s", filename, strerror(errno));
				retcode = false;
			}
			FileCRC::reset(player_table[index].uid(), FileCRC::kTextObjs);
		}
	}

	//удаляем файл таймеров
	if (!get_filename(name.c_str(), filename, kTimeCrashFile)) {
		log("SYSERR: Error deleting timer file for %s - unable to resolve file name.", name.c_str());
		retcode = false;
	} else {
		if (!(fl = fopen(filename, "rb"))) {
			if (errno != ENOENT)    // if it fails but NOT because of no file
				log("SYSERR: deleting timer file %s (1): %s", filename, strerror(errno));
			retcode = false;
		} else {
			fclose(fl);
			// if it fails, NOT because of no file
			if (remove(filename) < 0 && errno != ENOENT) {
				log("SYSERR: deleting timer file %s (2): %s", filename, strerror(errno));
				retcode = false;
			}
			FileCRC::reset(player_table[index].uid(), FileCRC::kTimeObjs);
		}
	}

	return (retcode);
}

int Crash_delete_crashfile(CharData *ch) {
	int index;

	index = GET_INDEX(ch);
	if (index < 0)
		return false;
	if (!SAVEINFO(index))
		Crash_delete_files(index);
	return true;
}

// ********* Timer utils: create, read, write, list, timer_objects *********

void ClearCrashSavedObjects(std::size_t index) {
	int i = 0, rnum;
	Crash_delete_files(index);
	if (SAVEINFO(index)) {
		for (; i < SAVEINFO(index)->rent.n_items; i++) {
			if (SAVEINFO(index)->time[i].timer >= 0 &&
				(rnum = GetObjRnum(SAVEINFO(index)->time[i].vnum)) >= 0) {
				obj_proto.dec_stored(rnum);
			}
		}
		ClearSaveinfo(index);
	}
}

void Crash_create_timer(const std::size_t index, int/* num*/) {
	RecreateSaveinfo(index);
}

int ReadCrashTimerFile(std::size_t index, int temp) {
	FILE *fl;
	char fname[kMaxInputLength];
	int size = 0, count = 0, rnum, num = 0;
	struct SaveRentInfo rent;
	struct SaveTimeInfo info;

	auto name = player_table[index].name();
	if (!get_filename(name.c_str(), fname, kTimeCrashFile)) {
		log("[ReadTimer] Error reading %s timer file - unable to resolve file name.", name.c_str());
		return false;
	}
	if (!(fl = fopen(fname, "rb"))) {
		if (errno != ENOENT) {
			log("SYSERR: fatal error opening timer file %s", fname);
			return false;
		} else
			return true;
	}

	fseek(fl, 0L, SEEK_END);
	const long fsize = ftell(fl);
	rewind(fl);
	if ((size = static_cast<int>(fsize) - sizeof(struct SaveRentInfo)) < 0
		|| size % sizeof(struct SaveTimeInfo)) {
		log("WARNING:  Timer file %s is corrupt!", fname);
		fclose(fl);
		return false;
	}

	// Читаем файл целиком в буфер: из него же считаем CRC (без второго
	// чтения только что прочитанного файла) и парсим записи.
	std::string content(static_cast<std::size_t>(fsize), '\0');
	if (fsize > 0 && fread(&content[0], static_cast<std::size_t>(fsize), 1, fl) != 1) {
		log("SYSERR: I/O Error reading %s timer file.", name.c_str());
		fclose(fl);
		return false;
	}
	fclose(fl);

	// Сверка CRC из буфера вместо повторного чтения файла.
	FileCRC::verify_from_content(player_table[index].uid(), FileCRC::kTimeObjs,
		content.data(), content.size());

	std::memcpy(&rent, content.data(), sizeof(struct SaveRentInfo));
	sprintf(buf, "[ReadTimer] Reading timer file %s for %s :", fname, name.c_str());
	switch (rent.rentcode) {
		case RENT_RENTED: strncat(buf, " Rent ", sizeof(buf) - strlen(buf) - 1);
			break;
		case RENT_CRASH:
			//           if (rent.time<1001651000L) //креш-сейв до Sep 28 00:26:20 2001
			rent.net_cost_per_diem = 0;    //бесплатно!
			strncat(buf, " Crash ", sizeof(buf) - strlen(buf) - 1);
			break;
		case RENT_CRYO: strncat(buf, " Cryo ", sizeof(buf) - strlen(buf) - 1);
			break;
		case RENT_TIMEDOUT: strncat(buf, " TimedOut ", sizeof(buf) - strlen(buf) - 1);
			break;
		case RENT_FORCED: strncat(buf, " Forced ", sizeof(buf) - strlen(buf) - 1);
			break;
		default: log("[ReadTimer] Error reading %s timer file - undefined rent code.", name.c_str());
			return false;
			//strcat(buf, " Undef ");
			//rent.rentcode = RENT_CRASH;
			break;
	}
	strncat(buf, "rent code.", sizeof(buf) - strlen(buf) - 1);
	log("%s", buf);
	Crash_create_timer(index, rent.n_items);
	player_table[index].timer->rent = rent;

	const char *recptr = content.data() + sizeof(struct SaveRentInfo);
	const std::size_t records = size / sizeof(struct SaveTimeInfo);
	for (; static_cast<std::size_t>(count) < records && count < rent.n_items; count++) {
		std::memcpy(&info, recptr + count * sizeof(struct SaveTimeInfo), sizeof(struct SaveTimeInfo));
		if (info.vnum && info.timer >= -1) {
			player_table[index].timer->time.push_back(info);
			++num;
		} else {
			log("[ReadTimer] Warning: incorrect vnum (%d) or timer (%d) while reading %s timer file.",
				info.vnum, info.timer, name.c_str());
		}
		if (info.timer >= 0 && (rnum = GetObjRnum(info.vnum)) >= 0 && !temp) {
			obj_proto.inc_stored(rnum);
		}
	}

	if (rent.n_items != num) {
		log("[ReadTimer] Error reading %s timer file - file is corrupt.", fname);
		ClearSaveinfo(index);
		return false;
	} else
		return true;
}

void Crash_reload_timer(int index) {
	int i = 0, rnum;
	if (SAVEINFO(index)) {
		for (; i < SAVEINFO(index)->rent.n_items; i++) {
			if (SAVEINFO(index)->time[i].timer >= 0 &&
				(rnum = GetObjRnum(SAVEINFO(index)->time[i].vnum)) >= 0) {
				obj_proto.dec_stored(rnum);
			}
		}
		ClearSaveinfo(index);
	}

	if (!ReadCrashTimerFile(index, false)) {
		sprintf(buf, "SYSERR: Unable to read timer file for %s.", player_table[index].name().c_str());
		mudlog(buf, BRF, MAX(kLvlImmortal, kLvlGod), SYSLOG, true);
	}
}

int Crash_write_timer(const std::size_t index) {
	char fname[kMaxStringLength];

	auto name = player_table[index].name();
	if (!SAVEINFO(index)) {
		log("SYSERR: Error writing %s timer file - no data.", name.c_str());
		return false;
	}
	if (!get_filename(name.c_str(), fname, kTimeCrashFile)) {
		log("SYSERR: Error writing %s timer file - unable to resolve file name.", name.c_str());
		return false;
	}

	// Сериализуем таймеры в буфер, чтобы посчитать CRC из него же,
	// не перечитывая с диска только что записанный файл. Порядок и состав
	// байт идентичны прежней пораздельной записи (rent + n_items записей).
	const SaveInfo *si = SAVEINFO(index);
	std::string content;
	// reserve на пустой строке -- одна аллокация полного размера без копирований,
	// дальше append пишет на месте (буфер мелкий, но реаллокации тут не нужны).
	content.reserve(sizeof(SaveRentInfo)
		+ static_cast<std::size_t>(si->rent.n_items) * sizeof(SaveTimeInfo));
	content.append(reinterpret_cast<const char *>(&si->rent), sizeof(SaveRentInfo));
	for (int i = 0; i < si->rent.n_items; ++i) {
		content.append(reinterpret_cast<const char *>(&si->time[i]), sizeof(SaveTimeInfo));
	}

	FILE *fl = fopen(fname, "wb");
	if (!fl) {
		log("[WriteTimer] Error writing %s timer file - unable to open file %s.", name.c_str(), fname);
		return false;
	}
	fwrite(content.data(), 1, content.size(), fl);
	fclose(fl);
#ifndef _WIN32
	if (chmod(fname, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0) {
		std::stringstream ss;
		ss << "Error chmod file: " << fname << " (" << __FILE__ << " "<< __func__ << "  "<< __LINE__ << ")";
		mudlog(ss.str(), BRF, kLvlGod, SYSLOG, true);
	}
#endif
	// CRC из буфера вместо повторного чтения файла (см. FileCRC::update_from_content).
	FileCRC::update_from_content(player_table[index].uid(), FileCRC::kTimeObjs,
		content.data(), content.size());
	return true;
}

void Crash_timer_obj(const std::size_t index, long time) {
	int n_items = 0, idelete = 0, ideleted = 0, rnum, timer, i;
	int rentcode, timer_dec;

	auto name = player_table[index].name();
	if (!player_table[index].timer) {
		log("[TO] %s has no save data.", name.c_str());
		return;
	}
	rentcode = SAVEINFO(index)->rent.rentcode;
	timer_dec = time - SAVEINFO(index)->rent.time;

	//удаляем просроченные файлы ренты
	if (rentcode == RENT_RENTED && timer_dec > rent_file_timeout * kSecsPerRealDay) {
		ClearCrashSavedObjects(index);
		log("[TO] Deleting %s's rent info - time outed.", name.c_str());
		return;
	} else if (rentcode != RENT_CRYO && timer_dec > crash_file_timeout * kSecsPerRealDay) {
		ClearCrashSavedObjects(index);
		buf[0] = '\0';
		switch (rentcode) {
			case RENT_CRASH: log("[TO] Deleting crash rent info for %s  - time outed.", name.c_str());
				break;
			case RENT_FORCED: log("[TO] Deleting forced rent info for %s  - time outed.", name.c_str());
				break;
			case RENT_TIMEDOUT: log("[TO] Deleting autorent info for %s  - time outed.", name.c_str());
				break;
			default: log("[TO] Deleting UNKNOWN rent info for %s  - time outed.", name.c_str());
				break;
		}
		return;
	}

	timer_dec = (timer_dec / kSecsPerMudHour) + (timer_dec % kSecsPerMudHour ? 1 : 0);

	//уменьшаем таймеры
	n_items = player_table[index].timer->rent.n_items;
//  log("[TO] Checking items for %s (%d items, rented time %dmin):",
//      name, n_items, timer_dec);
//	sprintf (buf,"[TO] Checking items for %s (%d items) :", name, n_items);
//	mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
	for (i = 0; i < n_items; i++) {
		if (player_table[index].timer->time[i].vnum < 0) //для шмоток без прототипа идем мимо
			continue;
		if (player_table[index].timer->time[i].timer >= 0) {
			rnum = GetObjRnum(player_table[index].timer->time[i].vnum);
			if ((!stable_objs::IsTimerUnlimited(obj_proto[rnum].get())) && (!obj_proto[rnum]->has_flag(EObjFlag::kNoRentTimer))) {
				timer = player_table[index].timer->time[i].timer;
				if (timer < timer_dec) {
					player_table[index].timer->time[i].timer = -1;
					idelete++;
					if (rnum >= 0) {
						obj_proto.dec_stored(rnum);
						log("[TO] Player %s : item %s deleted - time outted", name.c_str(),
							obj_proto[rnum]->get_PName(grammar::ECase::kNom).c_str());
					}
				}
			}
		} else {
			ideleted++;
		}
	}

//  log("Objects (%d), Deleted (%d)+(%d).", n_items, ideleted, idelete);

	//если появились новые просроченные объекты, обновляем файл таймеров
	if (idelete) {
		if (!Crash_write_timer(index)) {
			sprintf(buf, "SYSERR: [TO] Error writing timer file for %s.", name.c_str());
			mudlog(buf, CMP, MAX(kLvlImmortal, kLvlGod), SYSLOG, true);
		}
	}
}

void Crash_list_objects(CharData *ch, int index) {
	ObjRnum rnum;
	struct SaveTimeInfo data;
	long timer_dec;
	float num_of_days;

	if (!SAVEINFO(index))
		return;

	timer_dec = time(0) - SAVEINFO(index)->rent.time;
	num_of_days = (float) timer_dec / kSecsPerRealDay;
	timer_dec = (timer_dec / kSecsPerMudHour) + (timer_dec % kSecsPerMudHour ? 1 : 0);

	snprintf(buf, sizeof(buf), "Код ренты - ");
	switch (SAVEINFO(index)->rent.rentcode) {
		case RENT_RENTED: strncat(buf, "Rented.\r\n", sizeof(buf) - strlen(buf) - 1);
			break;
		case RENT_CRASH: strncat(buf, "Crash.\r\n", sizeof(buf) - strlen(buf) - 1);
			break;
		case RENT_CRYO: strncat(buf, "Cryo.\r\n", sizeof(buf) - strlen(buf) - 1);
			break;
		case RENT_TIMEDOUT: strncat(buf, "TimedOut.\r\n", sizeof(buf) - strlen(buf) - 1);
			break;
		case RENT_FORCED: strncat(buf, "Forced.\r\n", sizeof(buf) - strlen(buf) - 1);
			break;
		default: strncat(buf, "UNDEF!\r\n", sizeof(buf) - strlen(buf) - 1);
			break;
	}
	std::stringstream ss;
	for (int i = 0; i < SAVEINFO(index)->rent.n_items; i++) {
		data = SAVEINFO(index)->time[i];
		rnum = GetObjRnum(data.vnum);
		if (data.vnum > 799 || data.vnum < 700) {
			int tmr = data.timer;
			auto obj = obj_proto[rnum];
			if (!(stable_objs::IsTimerUnlimited(obj.get()) || obj->has_flag(EObjFlag::kNoRentTimer))) {
				tmr = MAX(-1, data.timer - timer_dec);
			}
			ss << fmt::format(" [{:>7}] ({:>5}au) <{:>6}> {:<20}\r\n",
					data.vnum, obj->get_rent_off(), tmr, obj->get_short_description().c_str());
		}
	}
	SendMsgToChar(ss.str().c_str(), ch);
	sprintf(buf, "Время в ренте: %ld тиков.\r\n", timer_dec);
	SendMsgToChar(buf, ch);
	sprintf(buf, "Предметов: %d. Стоимость: (%d в день) * (%1.2f дней) = %d. ИНГРИДИЕНТЫ НЕ ВЫВОДЯТСЯ.\r\n",
			SAVEINFO(index)->rent.n_items,
			SAVEINFO(index)->rent.net_cost_per_diem, num_of_days,
			(int) (num_of_days * SAVEINFO(index)->rent.net_cost_per_diem));
	SendMsgToChar(buf, ch);
}

void Crash_listrent(CharData *ch, char *name) {
	int index = GetPlayerTablePosByName(name);
	if (index < 0) {
		SendMsgToChar("Нет такого игрока.\r\n", ch);
		return;
	}

	if (!SAVEINFO(index)) {
		if (!ReadCrashTimerFile(index, true)) {
			sprintf(buf, "Ubable to read %s timer file.\r\n", name);
			SendMsgToChar(buf, ch);
		} else if (!SAVEINFO(index)) {
			sprintf(buf, "%s не имеет файла ренты.\r\n", utils::CAP(name));
			SendMsgToChar(buf, ch);
		} else {
			sprintf(buf, "%s находится в игре. Содержимое файла ренты:\r\n", utils::CAP(name));
			SendMsgToChar(buf, ch);
			Crash_list_objects(ch, index);
			ClearSaveinfo(index);
		}
	} else {
		sprintf(buf, "%s находится в ренте. Содержимое файла ренты:\r\n", utils::CAP(name));
		SendMsgToChar(buf, ch);
		Crash_list_objects(ch, index);
	}
}

struct container_list_type {
	ObjData *tank;
	struct container_list_type *next;
	int location;
};

// разобраться с возвращаемым значением
// *******************  load_char_objects ********************
int Crash_load(CharData *ch) {
	FILE *fl;
	char fname[kMaxStringLength], *data, *readdata;
	int cost, i = 0, reccount, fsize, error, index;
	float num_of_days;
	ObjData *obj2, *obj_list = nullptr;
	int location, rnum;
	struct container_list_type *tank_list = nullptr, *tank, *tank_to;

	if ((index = GET_INDEX(ch)) < 0)
		return (1);

	Crash_reload_timer(index);

	if (!SAVEINFO(index)) {
		sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		return (1);
	}

	switch (RENTCODE(index)) {
		case RENT_RENTED: sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
			break;
		case RENT_CRASH: sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
			break;
		case RENT_CRYO: sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
			break;
		case RENT_FORCED: sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
			break;
		case RENT_TIMEDOUT: sprintf(buf, "%s retrieving auto-saved items and entering game.", GET_NAME(ch));
			break;
		default: sprintf(buf, "SYSERR: %s entering game with undefined rent code %d.", GET_NAME(ch), RENTCODE(index));
			mudlog(buf, BRF, MAX(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
			SendMsgToChar("\r\n** Неизвестный код ренты **\r\n"
						  "Проблемы с восстановлением ваших вещей из файла.\r\n"
						  "Обращайтесь за помощью к Богам.\r\n", ch);
			ClearCrashSavedObjects(index);
			return (1);
			break;
	}
	mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);

	//Деньги за постой
	num_of_days = (float) (time(0) - SAVEINFO(index)->rent.time) / kSecsPerRealDay;
	sprintf(buf, "%s was %1.2f days in rent.", GET_NAME(ch), num_of_days);
	mudlog(buf, LGH, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
	cost = (int) (SAVEINFO(index)->rent.net_cost_per_diem * num_of_days);
	cost = MAX(0, cost);
	// added by WorM (Видолюб) 2010.06.04 сумма потраченная на найм(возвращается при креше)
	if (RENTCODE(index) == RENT_CRASH) {
		if (!privilege::IsImmortal(ch) && CanUseFeat(ch, EFeat::kEmployer) && ch->player_specials->saved.HiredCost != 0) {
			if (ch->player_specials->saved.HiredCost < 0)
				currencies::AddBank(*ch, currencies::kGold, abs(ch->player_specials->saved.HiredCost), false, false);
			else
				currencies::AddHand(*ch, currencies::kGold, ch->player_specials->saved.HiredCost, false, false);
		}
		ch->player_specials->saved.HiredCost = 0;
	}
	// end by WorM
	// Бесплатная рента, если выйти в течение 2 часов после ребута или креша
	if (((RENTCODE(index) == RENT_CRASH || RENTCODE(index) == RENT_FORCED)
		&& SAVEINFO(index)->rent.time + free_crashrent_period * kSecsPerRealHour > time(0)) || free_rent) {
		sprintf(buf, "%s** На сей раз постой был бесплатным **%s\r\n", kColorWht, kColorNrm);
		SendMsgToChar(buf, ch);
		sprintf(buf, "%s entering game, free crashrent.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
	} else if (cost > currencies::GetHand(*ch, currencies::kGold) + currencies::GetBank(*ch, currencies::kGold)) {
		sprintf(buf, "%sВы находились на постое %1.2f дней.\n\r"
					 "%s"
					 "Вам предъявили счет на %d %s за постой (%d %s в день).\r\n"
					 "Но все, что у вас было - %ld %s... Увы. Все ваши вещи переданы мобам.%s\n\r",
				kColorWht,
				num_of_days,
				RENTCODE(index) ==
					RENT_TIMEDOUT ?
				"Вас пришлось тащить до кровати, за это постой был дороже.\r\n"
								  : "", cost, MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str(),
				SAVEINFO(index)->rent.net_cost_per_diem,
				MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(SAVEINFO(index)->rent.net_cost_per_diem, grammar::ECase::kNom).c_str(), currencies::GetHand(*ch, currencies::kGold) + currencies::GetBank(*ch, currencies::kGold),
				MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(currencies::GetHand(*ch, currencies::kGold) + currencies::GetBank(*ch, currencies::kGold), grammar::ECase::kNom).c_str(), kColorNrm);
		SendMsgToChar(buf, ch);
		sprintf(buf, "%s: rented equipment lost (no $).", GET_NAME(ch));
		mudlog(buf, LGH, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		currencies::SetBank(*ch, currencies::kGold, 0);
		currencies::SetHand(*ch, currencies::kGold, 0);
		ClearCrashSavedObjects(index);
		return (2);
	} else {
		if (cost) {
			sprintf(buf, "%sВы находились на постое %1.2f дней.\n\r"
						 "%s"
						 "С вас содрали %d %s за постой (%d %s в день).%s\r\n",
					kColorWht,
					num_of_days,
					RENTCODE(index) ==
						RENT_TIMEDOUT ?
					"Вас пришлось тащить до кровати, за это постой был дороже.\r\n"
									  : "", cost, MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str(),
					SAVEINFO(index)->rent.net_cost_per_diem,
					MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(SAVEINFO(index)->rent.net_cost_per_diem, grammar::ECase::kNom).c_str(), kColorNrm);
			SendMsgToChar(buf, ch);
		}
		currencies::RemoveTotal(*ch, currencies::kGold, cost);
	}

	//Чтение описаний объектов в буфер
	if (!get_filename(GET_NAME(ch), fname, kTextCrashFile) || !(fl = fopen(fname, "r+b"))) {
		SendMsgToChar("\r\n** Нет файла описания вещей **\r\n"
					  "Проблемы с восстановлением ваших вещей из файла.\r\n"
					  "Обращайтесь за помощью к Богам.\r\n", ch);
		ClearCrashSavedObjects(index);
		return (1);
	}
	fseek(fl, 0L, SEEK_END);
	fsize = ftell(fl);
	if (!fsize) {
		fclose(fl);
		SendMsgToChar("\r\n** Файл описания вещей пустой **\r\n"
					  "Проблемы с восстановлением ваших вещей из файла.\r\n"
					  "Обращайтесь за помощью к Богам.\r\n", ch);
		ClearCrashSavedObjects(index);
		return (1);
	}

	CREATE(readdata, fsize + 1);
	fseek(fl, 0L, SEEK_SET);
	if (!fread(readdata, fsize, 1, fl) || ferror(fl) || !readdata) {
		fclose(fl);
		// Чтение не удалось -- валидного буфера для сверки CRC нет, сверку
		// (которая раньше перечитывала файл) тут опускаем и просто выходим.
		SendMsgToChar("\r\n** Ошибка чтения файла описания вещей **\r\n"
					  "Проблемы с восстановлением ваших вещей из файла.\r\n"
					  "Обращайтесь за помощью к Богам.\r\n", ch);
		log("Memory error or cann't read %s(%d)...", fname, fsize);
		free(readdata);
		ClearCrashSavedObjects(index);
		return (1);
	};
	fclose(fl);
	// Сверка CRC из уже прочитанного буфера, без повторного чтения файла.
	FileCRC::verify_from_content(ch->get_uid(), FileCRC::kTextObjs, readdata, fsize);

	data = readdata;
	*(data + fsize) = '\0';

	//Создание объектов
	long timer_dec = time(0) - SAVEINFO(index)->rent.time;
	timer_dec = (timer_dec / kSecsPerMudHour) + (timer_dec % kSecsPerMudHour ? 1 : 0);

	for (fsize = 0, reccount = SAVEINFO(index)->rent.n_items;
		 reccount > 0 && *data && *data != END_CHAR; reccount--, fsize++) {
		i++;
		ObjData::shared_ptr obj;

		obj = read_one_object_new(&data, &error);
		if (!obj) {
			//SendMsgToChar("Ошибка при чтении - чтение предметов прервано.\r\n", ch);
			SendMsgToChar("Ошибка при чтении файла объектов.\r\n", ch);
			sprintf(buf, "SYSERR: Objects reading fail for %s error %d, stop reading.", GET_NAME(ch), error);
			mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
			continue;    //Ann
		}
		if (error) {
			snprintf(buf,
					 kMaxStringLength,
					 "WARNING: Error #%d reading item vnum #%d num #%d from %s.",
					 error,
					 obj->get_vnum(),
					 i,
					 fname);
			mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
		}
/*
		if (SAVEINFO(index)->time[fsize].vnum >= dungeons::kZoneStartDungeons * 100) {
			SendMsgToChar(ch, "Предмет из данжа: %s, заменяем на оригинал.\r\n", obj->get_PName(ECase::kNom).c_str());
			SAVEINFO(index)->time[fsize].vnum = obj->get_vnum();
		}
*/
		if (obj->get_vnum() != SAVEINFO(index)->time[fsize].vnum) {
			SendMsgToChar("Нет соответствия заголовков - чтение предметов прервано.\r\n", ch);
			sprintf(buf, "SYSERR: Objects reading fail for %s (2), stop reading.", GET_NAME(ch));
			mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
			ExtractObjFromWorld(obj.get());
			break;
		}

		//Check timers
		if (SAVEINFO(index)->time[fsize].timer > 0
			&& (rnum = GetObjRnum(SAVEINFO(index)->time[fsize].vnum)) >= 0) {
			obj_proto.dec_stored(rnum);
		}
		// вычтем таймер оффлайна
		if (!(stable_objs::IsTimerUnlimited(obj.get()) || obj->has_flag(EObjFlag::kNoRentTimer))) {
			const SaveInfo *si = SAVEINFO(index);
			const int saved_timer = si->time[fsize].timer;
			// Защита от мусорных значений в бинарном таймер-файле
			// (UNLIMITED_TIMER и близкие следы старых багов decay_manager
			// в otransform/oedit - см. #3213, #3225). Текстовый Tmer уже
			// клампится в read_one_object_new по тому же порогу 99999.
			// Если значение бракованное, не переписываем m_timer, иначе
			// dec_timer ниже разгонит UNLIMITED-N*timer_dec в дрейфующий
			// таймер, который медленно сходит к нулю.
			if (saved_timer <= 99999) {
				obj->set_timer(saved_timer);
				obj->dec_timer(timer_dec);
			}
		}

		std::string cap = obj->get_PName(grammar::ECase::kNom);
		cap[0] = UPPER(cap[0]);

		// Предмет разваливается от старости
		if (obj->get_timer() <= 0) {
			snprintf(buf, kMaxStringLength, "%s%s%s рассыпал%s от длительного использования.\r\n",
					 kColorWht,
					 cap.c_str(),
					 char_get_custom_label(obj.get(), ch).c_str(),
					 grammar::ObjSexEnding((obj)->get_sex(), 2));
			SendMsgToChar(buf, ch);
			ExtractObjFromWorld(obj.get());

			continue;
		}

		//очищаем ZoneDecay объедки
		if (obj->has_flag(EObjFlag::kZonedecay)) {
			sprintf(buf, "%s рассыпал%s в прах.\r\n", cap.c_str(), grammar::ObjSexEnding((obj)->get_sex(), 2));
			SendMsgToChar(buf, ch);
			ExtractObjFromWorld(obj.get());
			continue;
		}
		if (obj->has_flag(EObjFlag::kRepopDecay)) {
			sprintf(buf, "%s рассыпал%s в прах.\r\n", cap.c_str(), grammar::ObjSexEnding((obj)->get_sex(), 2));
			SendMsgToChar(buf, ch);
			ExtractObjFromWorld(obj.get());
			continue;
		}

		// Check valid class
		if (invalid_anti_class(ch, obj.get())
			|| invalid_unique(ch, obj.get())
			|| NamedStuff::check_named(ch, obj.get(), 0)) {
			sprintf(buf, "%s рассыпал%s, как запрещенн%s для вас.\r\n",
					cap.c_str(), grammar::ObjSexEnding((obj)->get_sex(), 2), grammar::ObjSexEnding((obj)->get_sex(), 3));
			SendMsgToChar(buf, ch);
			ExtractObjFromWorld(obj.get());
			continue;
		}

		//очищаем от крови
		if (obj->has_flag(EObjFlag::kBloody)) {
			obj->unset_extraflag(EObjFlag::kBloody);
		}

		obj->set_next_content(obj_list);
		obj_list = obj.get();
	}

	free(readdata);

	for (auto obj = obj_list; obj; obj = obj2) {
		obj2 = obj->get_next_content();
		obj->set_next_content(nullptr);
		if (obj->get_worn_on() >= 0)    // Equipped or in inventory
		{
			if (obj2
				&& obj2->get_worn_on() < 0
				&& obj->get_type() == EObjType::kContainer)    // This is container and it is not free
			{
				CREATE(tank, 1);
				tank->next = tank_list;
				tank->tank = obj;
				tank->location = 0;
				tank_list = tank;
			} else {
				while (tank_list)    // Clear tanks list
				{
					tank = tank_list;
					tank_list = tank->next;
					free(tank);
				}
			}
			location = obj->get_worn_on();
			obj->set_worn_on(0);

			auto_equip(ch, obj, location);
//			log("%s load_char_obj %d %ld %u", GET_NAME(ch), GET_OBJ_VNUM(obj), obj->get_unique_id(), obj->get_timer());
		} else {
			if (obj2
				&& obj2->get_worn_on() < obj->get_worn_on()
				&& obj->get_type() == EObjType::kContainer)    // This is container and it is not free
			{
				tank_to = tank_list;
				CREATE(tank, 1);
				tank->next = tank_list;
				tank->tank = obj;
				tank->location = obj->get_worn_on();
				tank_list = tank;
			} else {
				while ((tank_to = tank_list)) {
					// Clear all tank than less or equal this object location
					if (tank_list->location > obj->get_worn_on()) {
						break;
					} else {
						tank = tank_list;
						tank_list = tank->next;
						free(tank);
					}
				}
			}
			obj->set_worn_on(0);
			if (tank_to) {
				PlaceObjIntoObj(obj, tank_to->tank);
			} else {
				PlaceObjToInventory(obj, ch);
			}
//			log("%s load_char_obj %d %ld %u", GET_NAME(ch), GET_OBJ_VNUM(obj), obj->get_unique_id(), obj->get_timer());
		}
	}

	while (tank_list)    //Clear tanks list
	{
		tank = tank_list;
		tank_list = tank->next;
		free(tank);
	}

	affect_total(ch);
	ClearSaveinfo(index);
	//???
	//Crash_crashsave();
	return 0;
}

// ********** Some util functions for objects save... **********

void Crash_restore_weight(ObjData *obj) {
	for (; obj; obj = obj->get_next_content()) {
		Crash_restore_weight(obj->get_contains());
		if (obj->get_in_obj()) {
			obj->get_in_obj()->add_weight(obj->get_weight());
		}
	}
}

void Crash_extract_objs(ObjData *obj) {
	ObjData *next;
	for (; obj; obj = next) {
		next = obj->get_next_content();
		Crash_extract_objs(obj->get_contains());
		if (obj->get_rnum() >= 0 && obj->get_timer() >= 0) {
			obj_proto.inc_stored(obj->get_rnum());
		}
		ExtractObjFromWorld(obj);
	}
}

int Crash_is_unrentable(CharData *ch, ObjData *obj) {
	if (!obj) {
		return false;
	}
	if (obj->is_unrentable() || SetSystem::is_norent_set(ch, obj)) {
		return true;
	}
	return false;
}

void Crash_extract_norents(CharData *ch, ObjData *obj) {
	ObjData *next;
	for (; obj; obj = next) {
		next = obj->get_next_content();
		Crash_extract_norents(ch, obj->get_contains());
		if (Crash_is_unrentable(ch, obj)) {
			ExtractObjFromWorld(obj);
		}
	}
}

void Crash_extract_norent_eq(CharData *ch) {
	for (int j = 0; j < EEquipPos::kNumEquipPos; j++) {
		if (GET_EQ(ch, j) == nullptr) {
			continue;
		}

		if (Crash_is_unrentable(ch, GET_EQ(ch, j))) {
			PlaceObjToInventory(UnequipChar(ch, j, CharEquipFlags()), ch);
		} else {
			Crash_extract_norents(ch, GET_EQ(ch, j));
		}
	}
}

void Crash_extract_norent_charmee(CharData *ch) {
	if (!ch->followers.empty()) {
		for (auto *k : ch->followers) {
			if (!IsCharmice(k)
				|| !k->has_master()) {
				continue;
			}
			for (int j = 0; j < EEquipPos::kNumEquipPos; ++j) {
				if (!GET_EQ(k, j)) {
					continue;
				}

				if (Crash_is_unrentable(k, GET_EQ(k, j))) {
					PlaceObjToInventory(UnequipChar(k, j, CharEquipFlags()), k);
				} else {
					Crash_extract_norents(k, GET_EQ(k, j));
				}
			}
			Crash_extract_norents(k, k->carrying);
		}
	}
}

int Crash_calculate_rent(ObjData *obj) {
	int cost = 0;
	for (; obj; obj = obj->get_next_content()) {
		cost += Crash_calculate_rent(obj->get_contains());
		cost += MAX(0, obj->get_rent_off());
	}
	return (cost);
}

int Crash_calculate_rent_eq(ObjData *obj) {
	int cost = 0;
	for (; obj; obj = obj->get_next_content()) {
		cost += Crash_calculate_rent(obj->get_contains());
		cost += MAX(0, obj->get_rent_on());
	}
	return (cost);
}

int Crash_calculate_charmee_rent(CharData *ch) {
	int cost = 0;
	if (!ch->followers.empty()) {
		for (auto *k : ch->followers) {
			if (!IsCharmice(k)
				|| !k->has_master()) {
				continue;
			}

			cost = Crash_calculate_rent(k->carrying);
			for (int j = 0; j < EEquipPos::kNumEquipPos; ++j) {
				cost += Crash_calculate_rent_eq(GET_EQ(k, j));
			}
		}
	}
	return cost;
}

int Crash_calcitems(ObjData *obj) {
	int i = 0;
	for (; obj; obj = obj->get_next_content(), i++) {
		i += Crash_calcitems(obj->get_contains());
	}
	return (i);
}

int Crash_calc_charmee_items(CharData *ch) {
	int num = 0;
	if (!ch->followers.empty()) {
		for (auto *k : ch->followers) {
			if (!IsCharmice(k)
				|| !k->has_master())
				continue;
			for (int j = 0; j < EEquipPos::kNumEquipPos; j++)
				num += Crash_calcitems(GET_EQ(k, j));
			num += Crash_calcitems(k->carrying);
		}
	}
	return num;
}

void Crash_save(std::stringstream &write_buffer, int iplayer, ObjData *obj, int location, int savetype) {
	for (; obj; obj = obj->get_next_content()) {
		if (obj->get_in_obj()) {
			obj->get_in_obj()->sub_weight(obj->get_weight());
		}
		Crash_save(write_buffer, iplayer, obj->get_contains(), MIN(0, location) - 1, savetype);
		if (iplayer >= 0) {
			write_one_object(write_buffer, obj, location);
			SaveTimeInfo tmp_node;
			if (obj_proto[obj->get_rnum()]->get_parent_rnum() > -1) {
				tmp_node.vnum = obj_proto[obj->get_rnum()]->get_parent_vnum();
			} else {
				tmp_node.vnum = obj->get_vnum();
			}
			tmp_node.timer = obj->get_timer();
			SAVEINFO(iplayer)->time.push_back(tmp_node);

			if (savetype != RENT_CRASH) {
//				log("%s save_char_obj %d %ld %u", player_table[iplayer].name().c_str(),
//					GET_OBJ_VNUM(obj), obj->get_unique_id(), obj->get_timer());
			}
		}
	}
}

void crash_save_and_restore_weight(std::stringstream &write_buffer,
								   int iplayer,
								   ObjData *obj,
								   int location,
								   int savetype) {
	Crash_save(write_buffer, iplayer, obj, location, savetype);
	Crash_restore_weight(obj);
}

// ********************* save_char_objects ********************************
int save_char_objects(CharData *ch, int savetype, int rentcost) {
	char fname[kMaxStringLength];
	struct SaveRentInfo rent;
	int j, num = 0, iplayer = -1, cost;

	if (ch->IsNpc())
		return false;

	if ((iplayer = GET_INDEX(ch)) < 0) {
		sprintf(buf, "[SYSERR] Store file '%s' - INVALID Id %d", GET_NAME(ch), iplayer);
		mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
		return false;
	}

	// Таймер всей функции -- чтобы поймать спайки ниже порога total_io (#3440):
	// если время не в трёх замеренных фазах, оно всплывёт в "other".
	utils::CExecutionTimer fn_timer;

	// удаление !рент предметов
	if (savetype != RENT_CRASH) {
		Crash_extract_norent_eq(ch);
		Crash_extract_norents(ch, ch->carrying);
	}
	// при ребуте у чармиса тоже чистим
	if (savetype == RENT_FORCED) {
		Crash_extract_norent_charmee(ch);
	}

	// подсчет количества предметов
	for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
		num += Crash_calcitems(GET_EQ(ch, j));
	}
	num += Crash_calcitems(ch->carrying);

	int charmee_items = 0;
	if (savetype == RENT_CRASH || savetype == RENT_FORCED) {
		charmee_items = Crash_calc_charmee_items(ch);
		num += charmee_items;
	}

	log("Save obj: %s -> %d (%d)", ch->get_name().c_str(), num, charmee_items);
	ObjSaveSync::check(ch->get_uid(), ObjSaveSync::CHAR_SAVE);

	if (!num) {
		g_obj_crash_save_hash.erase(ch->get_uid());
		Crash_delete_files(iplayer);
		return false;
	}

	// цена ренты
	cost = Crash_calculate_rent(ch->carrying);
	for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
		cost += Crash_calculate_rent_eq(GET_EQ(ch, j));
	}
	if (savetype == RENT_CRASH || savetype == RENT_FORCED) {
		cost += Crash_calculate_charmee_rent(ch);
	}

	// чаевые
	if (min_rent_cost(ch) > 0) {
		cost += MAX(0, min_rent_cost(ch));
	} else {
		cost /= 2;
	}

	if (savetype == RENT_TIMEDOUT) {
		cost *= 2;
	}

	//CRYO-rent надо дорабатывать либо выкидывать нафиг
	if (savetype == RENT_CRYO) {
		rent.net_cost_per_diem = 0;
		currencies::RemoveHand(*ch, currencies::kGold, cost);
	}

	if (savetype == RENT_RENTED) {
		rent.net_cost_per_diem = rentcost;
	} else {
		rent.net_cost_per_diem = cost;
	}

	rent.rentcode = savetype;
	rent.time = time(0);
	rent.n_items = num;
	rent.gold = currencies::GetHand(*ch, currencies::kGold);
	rent.account = currencies::GetBank(*ch, currencies::kGold);

	Crash_create_timer(iplayer, num);
	SAVEINFO(iplayer)->rent = rent;

	// Тайминг сериализации предметов в буфер -- отдельно от дисковой записи,
	// чтобы в логе было видно, что именно долгое (issue #3396).
	utils::CExecutionTimer serialize_timer;
	std::stringstream write_buffer;
	write_buffer << "@ Items file\n";

	for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
		if (GET_EQ(ch, j)) {
			crash_save_and_restore_weight(write_buffer, iplayer, GET_EQ(ch, j), j + 1, savetype);
		}
	}

	crash_save_and_restore_weight(write_buffer, iplayer, ch->carrying, 0, savetype);

	if (!ch->followers.empty()
		&& (savetype == RENT_CRASH
			|| savetype == RENT_FORCED)) {
		for (auto *k : ch->followers) {
			if (!IsCharmice(k)
				|| !k->has_master()) {
				continue;
			}

			for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
				if (GET_EQ(k, j)) {
					crash_save_and_restore_weight(write_buffer, iplayer, GET_EQ(k, j), 0, savetype);
				}
			}

			crash_save_and_restore_weight(write_buffer, iplayer, k->carrying, 0, savetype);
		}
	}

	const double serialize_sec = serialize_timer.delta().count();

	// в принципе экстрактить здесь чармисовый шмот в случае ребута - смысла ноль
	if (savetype != RENT_CRASH) {
		for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
			if (GET_EQ(ch, j)) {
				Crash_extract_objs(GET_EQ(ch, j));
			}
		}
		Crash_extract_objs(ch->carrying);
	}

	double crc_sec = 0.0;
	utils::CExecutionTimer obj_io_timer;
	if (get_filename(GET_NAME(ch), fname, kTextCrashFile)) {
		write_buffer << "\n$\n$\n";
		// Пишем в бинарном режиме и из этого же буфера считаем CRC -- без
		// перечитывания только что записанного файла. Бинарный режим
		// гарантирует, что байты файла == байты буфера на всех платформах
		// (в текстовом режиме Windows транслировал бы \n -> \r\n и CRC из
		// буфера разошёлся бы с CRC файла).
		const std::string obj_content = write_buffer.str();
		// #3440: для периодического crash-сейва (RENT_CRASH) пропускаем запись
		// файла объектов, если набор предметов не изменился с прошлого сейва.
		// Файл объектов -- чистый payload предметов (рент-таймеры пишутся
		// отдельно в .time), поэтому одинаковый payload => идентичный файл =>
		// дисковая запись и пересчёт CRC не нужны. Это основной источник спайков
		// "Crash frac save" (синхронный write неизменившихся инвентарей).
		const std::size_t content_hash = std::hash<std::string>{}(obj_content);
		const auto cached = g_obj_crash_save_hash.find(ch->get_uid());
		const bool unchanged = (cached != g_obj_crash_save_hash.end() && cached->second == content_hash);
		if (savetype == RENT_CRASH && unchanged) {
			// предметы не менялись -- файл уже актуален, диск не трогаем
		} else {
			std::ofstream file(fname, std::ios::binary);
			if (!file.is_open()) {
				snprintf(buf, kMaxStringLength, "[SYSERR] Store objects file '%s'- MAY BE LOCKED.", fname);
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
				Crash_delete_files(iplayer);
				return false;
			}
			file.write(obj_content.data(), static_cast<std::streamsize>(obj_content.size()));
			file.close();
#ifndef _WIN32
			if (chmod(fname, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0) {
				std::stringstream ss;
				ss << "Error chmod file: " << fname << " (" << __FILE__ << " "<< __func__ << "  "<< __LINE__ << ")";
				mudlog(ss.str(), BRF, kLvlGod, SYSLOG, true);
			}
#endif
			utils::CExecutionTimer crc_timer;
			FileCRC::update_from_content(ch->get_uid(), FileCRC::kTextObjs,
				obj_content.data(), obj_content.size());
			crc_sec = crc_timer.delta().count();
			g_obj_crash_save_hash[ch->get_uid()] = content_hash;
		}
	} else {
		Crash_delete_files(iplayer);
		return false;
	}
	const double obj_io_sec = obj_io_timer.delta().count();
	// Чистая запись на диск (open+write+close+chmod) = весь io-блок минус CRC.
	const double write_sec = obj_io_sec - crc_sec;

	utils::CExecutionTimer timer_io_timer;
	if (!Crash_write_timer(iplayer)) {
		Crash_delete_files(iplayer);
		return false;
	}
	const double timer_io_sec = timer_io_timer.delta().count();

	// Профилирование синхронной записи (см. #3368, #3396). Разбито по фазам:
	// serialize -- сборка буфера предметов, write -- запись файла на диск
	// (open+write+close+chmod), crc -- подсчёт CRC, time_io -- запись .time.
	// Дисковый write почти не зависит от числа предметов и обычно доминирует,
	// поэтому "меньше предметов, но дольше" -- это про занятость диска, не про items.
	const double total_io_sec = serialize_sec + obj_io_sec + timer_io_sec;
	const double fn_sec = fn_timer.delta().count();
	// "other" -- время вне трёх замеренных фаз (calcitems, расчёт ренты,
	// экстракты, Crash_create_timer и пр.). Если спайк тут -- видно сразу.
	const double other_sec = fn_sec - total_io_sec;
	if (fn_sec > 0.02) {
		log("save_char_objects: %s items=%d serialize=%.4f write=%.4f crc=%.4f time_io=%.4f other=%.4f total=%.4f",
			GET_NAME(ch), num, serialize_sec, write_sec, crc_sec, timer_io_sec, other_sec, fn_sec);
	}

	if (savetype == RENT_CRASH) {
		ClearSaveinfo(iplayer);
	}

	return true;
}

//some dummy functions

void Crash_crashsave(CharData *ch) {
	save_char_objects(ch, RENT_CRASH, 0);
}

void Crash_ldsave(CharData *ch) {
	Crash_crashsave(ch);
}

void Crash_idlesave(CharData *ch) {
	save_char_objects(ch, RENT_TIMEDOUT, 0);
}

void Crash_rentsave(CharData *ch, int cost) {
	save_char_objects(ch, RENT_RENTED, cost);
}

int Crash_cryosave(CharData *ch, int cost) {
	return save_char_objects(ch, RENT_TIMEDOUT, cost);
}

// **************************************************************************
// * Routines used for the receptionist                                     *
// **************************************************************************

void Crash_rent_deadline(CharData *ch, CharData *recep, long cost) {
	long rent_deadline;

	if (!cost) {
		SendMsgToChar(specials::RentMsg(specials::ERentMsg::kCanLiveForever) + "\r\n", ch);
		return;
	}

	act(specials::RentMsg(specials::ERentMsg::kDeadlineIntro) + "\r\n", false, recep, 0, ch, kToVict);

	long depot_cost = static_cast<long>(Depot::get_total_cost_per_day(ch));
	if (depot_cost) {
		SendMsgToChar(fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kDepotCost)), fmt::arg("amount", depot_cost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(depot_cost, grammar::ECase::kNom).c_str())) + "\r\n", ch);
		cost += depot_cost;
	}

	SendMsgToChar(fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kRentCost)), fmt::arg("amount", cost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str())) + "\r\n", ch);
	rent_deadline = ((currencies::GetHand(*ch, currencies::kGold) + currencies::GetBank(*ch, currencies::kGold)) / cost);
	SendMsgToChar(fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kMoneyLasts)), fmt::arg("amount", rent_deadline), fmt::arg("day", grammar::GetDeclensionInNumber(rent_deadline, grammar::EWhat::kDay))) + "\r\n", ch);
}

int Crash_report_unrentables(CharData *ch, CharData *recep, ObjData *obj) {
	char buf[128];
	int has_norents = 0;

	if (obj) {
		if (Crash_is_unrentable(ch, obj)) {
			has_norents = 1;
			if (SetSystem::is_norent_set(ch, obj)) {
				snprintf(buf, sizeof(buf), "%s", fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kUnrentSet)), fmt::arg("item", OBJN(obj, ch, grammar::ECase::kAcc))).c_str());
			} else {
				snprintf(buf, sizeof(buf), "%s", fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kUnrent)), fmt::arg("item", OBJN(obj, ch, grammar::ECase::kAcc))).c_str());
			}
			act(buf, false, recep, 0, ch, kToVict);
		}
		has_norents += Crash_report_unrentables(ch, recep, obj->get_contains());
		has_norents += Crash_report_unrentables(ch, recep, obj->get_next_content());
	}
	return (has_norents);
}

// added by WorM (Видолюб)
//процедура для вывода стоимости ренты одной и той же шмотки(count кол-ва)
void Crash_report_rent_item(CharData *ch,
							CharData *recep,
							ObjData *obj,
							int count,
							int factor,
							int equip,
							int recursive) {
	static char buf[256];
	char bf[80], bf2[14];

	if (obj) {
		if (CAN_WEAR_ANY(obj)) {
			if (equip) {
				sprintf(bf, " (%d если снять)", obj->get_rent_off() * factor * count);
			} else {
				sprintf(bf, " (%d если надеть)", obj->get_rent_on() * factor * count);
			}
		} else {
			*bf = '\0';
		}

		if (count > 1) {
			sprintf(bf2, " [%d]", count);
		}

		sprintf(buf, "%s - %d %s%s за %s%s %s",
				recursive ? "" : kColorWht,
				(equip ? obj->get_rent_on() * count : obj->get_rent_off()) *
					factor * count,
				MUD::Currency(currencies::kGoldVnum).GetNameWithAmount((equip ? obj->get_rent_on() * count : obj->get_rent_off()) * factor * count, grammar::ECase::kNom).c_str(),
				bf, OBJN(obj, ch, grammar::ECase::kAcc), count > 1 ? bf2 : "", recursive ? "" : kColorNrm);
		act(buf, false, recep, 0, ch, kToVict);
	}
}
// end by WorM

void Crash_report_rent(CharData *ch, CharData *recep, ObjData *obj, int *cost,
					   long *n_items, int rentshow, int factor, int equip, int recursive) {
	ObjData *i, *push = nullptr;
	int push_count = 0;

	if (obj) {
		if (!Crash_is_unrentable(ch, obj)) {
			/*(*n_items)++;
			*cost += MAX(0, ((equip ? obj->get_rent_on() : obj->get_rent_off()) * factor));
			if (rentshow)
			{
				if (*n_items == 1)
				{
					if (!recursive)
						act("$n сказал$g Вам : \"Одет$W спать будешь? Хм.. Ну смотри, тогда дешевле возьму\"", false, recep, 0, ch, TO_VICT);
					else
						act("$n сказал$g Вам : \"Это я в чулане запру:\"", false, recep, 0, ch, TO_VICT);
				}
				if (CAN_WEAR_ANY(obj))
				{
					if (equip)
						sprintf(bf, " (%d если снять)", obj->get_rent_off() * factor);
					else
						sprintf(bf, " (%d если надеть)", obj->get_rent_on() * factor);
				}
				else
					*bf = '\0';
				sprintf(buf, "%s - %d %s%s за %s %s",
						recursive ? "" : KWHT,
						(equip ? obj->get_rent_on() : obj->get_rent_off()) *
						factor,
						desc_count((equip ? obj->get_rent_on() :
									obj->get_rent_off()) * factor,
								   EWhat::MONEYa), bf, OBJN(obj, ch, ECase::kAcc),
						recursive ? "" : KNRM;
				act(buf, false, recep, 0, ch, TO_VICT);
			}*/
			// added by WorM (Видолюб)
			//группирует вывод при ренте на подобии:
			// - 2700 кун (900 если надеть) за красный пузырек [9]
			for (i = obj; i; i = i->get_next_content()) {
				(*n_items)++;
				*cost += MAX(0, ((equip ? i->get_rent_on() : i->get_rent_off()) * factor));
				if (rentshow) {
					if (*n_items == 1) {
						if (!recursive) {
							act(specials::RentMsg(specials::ERentMsg::kRentIntroEquip), false, recep, 0, ch, kToVict);
						} else {
							act(specials::RentMsg(specials::ERentMsg::kRentIntroStore), false, recep, 0, ch, kToVict);
						}
					}

					if (!push) {
						push = i;
						push_count = 1;
					} else if (!IsObjsStackable(i, push)) {
						Crash_report_rent_item(ch, recep, push, push_count, factor, equip, recursive);
						if (recursive) {
							Crash_report_rent(ch,
											  recep,
											  push->get_contains(),
											  cost,
											  n_items,
											  rentshow,
											  factor,
											  false,
											  true);
						}
						push = i;
						push_count = 1;
					} else {
						push_count++;
					}
				}
			}
			if (push) {
				Crash_report_rent_item(ch, recep, push, push_count, factor, equip, recursive);
				if (recursive) {
					Crash_report_rent(ch, recep, push->get_contains(), cost, n_items, rentshow, factor, false, true);
				}
			}
			// end by WorM
		}
	}
}

int Crash_offer_rent(CharData *ch, CharData *receptionist, int rentshow, int factor, int *totalcost) {
	char buf[kMaxExtendLength];
	int i;
	long numitems = 0, norent;
// added by Dikiy (Лель)
	long numitems_weared = 0;
// end by Dikiy

	*totalcost = 0;
	norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
	for (i = 0; i < EEquipPos::kNumEquipPos; i++)
		norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));
	norent += Depot::report_unrentables(ch, receptionist);

	if (norent)
		return (false);

	*totalcost = min_rent_cost(ch) * factor;

	for (i = 0; i < EEquipPos::kNumEquipPos; i++)
		Crash_report_rent(ch, receptionist, GET_EQ(ch, i), totalcost, &numitems, rentshow, factor, true, false);

	numitems_weared = numitems;
	numitems = 0;

	Crash_report_rent(ch, receptionist, ch->carrying, totalcost, &numitems, rentshow, factor, false, true);

	for (i = 0; i < EEquipPos::kNumEquipPos; i++)
		if (GET_EQ(ch, i)) {
			Crash_report_rent(ch,
							  receptionist,
							  (GET_EQ(ch, i))->get_contains(),
							  totalcost,
							  &numitems,
							  rentshow,
							  factor,
							  false,
							  true);
		}

	numitems += numitems_weared;

	if (!numitems) {
		act(specials::RentMsg(specials::ERentMsg::kNothingToRent), false, receptionist, 0, ch, kToVict);
		return (false);
	}

	if (numitems > kMaxSavedItems) {
		snprintf(buf, kMaxExtendLength, "%s", (fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kTooManyItems1)), fmt::arg("max", kMaxSavedItems)) + "\r\n" + fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kTooManyItems2)), fmt::arg("count", numitems))).c_str());
		act(buf, false, receptionist, 0, ch, kToVict);
		return (false);
	}

	int divide = 1;
	if (min_rent_cost(ch) <= 0 && *totalcost <= 1000) {
		divide = 2;
	}

	if (rentshow) {
		if (min_rent_cost(ch) > 0) {
			snprintf(buf, kMaxExtendLength, "%s", fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kTipForBeer)), fmt::arg("amount", min_rent_cost(ch) * factor), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(min_rent_cost(ch) * factor, grammar::ECase::kNom).c_str())).c_str());
			act(buf, false, receptionist, 0, ch, kToVict);
		}

		snprintf(buf, kMaxExtendLength, "%s", fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kTotalCost)), fmt::arg("amount", *totalcost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(*totalcost, grammar::ECase::kNom).c_str()), fmt::arg("perday", (factor == RENT_FACTOR ? "в день " : ""))).c_str());
		act(buf, false, receptionist, 0, ch, kToVict);

		if (MAX(0, *totalcost / divide) > currencies::GetHand(*ch, currencies::kGold) + currencies::GetBank(*ch, currencies::kGold)) {
			act(specials::RentMsg(specials::ERentMsg::kNoMoneyEver), false, receptionist, 0, ch, kToVict);
			return (false);
		}

		*totalcost = MAX(0, *totalcost / divide);
		if (divide == 2) {
			act(specials::RentMsg(specials::ERentMsg::kHalfPrice), false, receptionist, 0, ch, kToVict);
		}

		if (factor == RENT_FACTOR) {
			Crash_rent_deadline(ch, receptionist, *totalcost);
		}
	} else {
		*totalcost = MAX(0, *totalcost / divide);
	}
	return (true);
}

enum class ERentAction { kRent, kOffer, kSettle };

int gen_receptionist(CharData *ch, CharData *recep, ERentAction action, int mode) {
	RoomRnum save_room;
	int cost, rentshow = true;

	save_room = ch->in_room;

	if (!AWAKE(recep)) {
		snprintf(buf, kMaxStringLength, "%s", (fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kRecepAsleep)), fmt::arg("recep", grammar::PersonalPronoun((recep)->get_sex()))) + "\r\n").c_str());
		SendMsgToChar(buf, ch);
		return (true);
	}
	if (!sight::CanSee(recep, ch)) {
		act(specials::RentMsg(specials::ERentMsg::kCantSee), false, recep, 0, 0, kToRoom);
		return (true);
	}
	if (Clan::InEnemyZone(ch)) {
		act(specials::RentMsg(specials::ERentMsg::kEnemyZone), false, recep, 0, 0, kToRoom);
		return (true);
	}
	if (NORENTABLE(ch)) {
		SendMsgToChar(specials::RentMsg(specials::ERentMsg::kNoRentableWar) + "\r\n", ch);
		return (true);
	}
	if (ch->GetEnemy()) {
		return (false);
	}
	if (free_rent) {
		act(specials::RentMsg(specials::ERentMsg::kFreeRent), false, recep, 0, ch, kToVict);
		rentshow = false;
	}
	if (action == ERentAction::kRent) {

		if (!Crash_offer_rent(ch, recep, rentshow, mode, &cost))
			return (true);

		if (rentshow) {
			if (mode == RENT_FACTOR)
				snprintf(buf, kMaxStringLength, "%s", fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kDailyCost)), fmt::arg("amount", cost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str())).c_str());
			else if (mode == CRYO_FACTOR)
				snprintf(buf, kMaxStringLength, "%s", fmt::format(fmt::runtime(specials::RentMsg(specials::ERentMsg::kDailyCostCryo)), fmt::arg("amount", cost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(cost, grammar::ECase::kNom).c_str())).c_str());
			act(buf, false, recep, 0, ch, kToVict);

			if (cost > currencies::GetHand(*ch, currencies::kGold) + currencies::GetBank(*ch, currencies::kGold)) {
				act(specials::RentMsg(specials::ERentMsg::kCantAfford), false, recep, 0, ch, kToVict);
				return (true);
			}
			if (cost && (mode == RENT_FACTOR))
				Crash_rent_deadline(ch, recep, cost);
		}
		if (free_rent) {
			cost = 0;
		}
		if (mode == RENT_FACTOR) {
			act(specials::RentMsg(specials::ERentMsg::kLockedAway), false, recep, 0, ch, kToVict);
			Crash_rentsave(ch, cost);
			sprintf(buf, "%s has rented (%d/day, %ld tot.)",
					GET_NAME(ch), cost, currencies::GetHand(*ch, currencies::kGold) + currencies::GetBank(*ch, currencies::kGold));
		} else    // cryo
		{
			act(specials::RentMsg(specials::ERentMsg::kLockedAway) + "\r\n" + specials::RentMsg(specials::ERentMsg::kCryoGhost) + "\r\n" + specials::RentMsg(specials::ERentMsg::kCryoLostTouch), false, recep, 0, ch, kToVict);
			Crash_cryosave(ch, cost);
			sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
			ch->SetFlag(EPlrFlag::kCryo);
		}

		mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);

		if ((save_room == r_helled_start_room)
			|| (save_room == r_named_start_room)
			|| (save_room == r_unreg_start_room))
			act(specials::RentMsg(specials::ERentMsg::kKickedToBench), false, recep, 0, ch, kToRoom);
		else {
			act(specials::RentMsg(specials::ERentMsg::kHelpedToSleep), false, recep, 0, ch, kToNotVict);
			GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
		}
		Clan::clan_invoice(ch, false);
		ch->save_char();
		ExtractCharFromWorld(ch, false);
	} else if (action == ERentAction::kOffer) {
		Crash_offer_rent(ch, recep, rentshow, mode, &cost);
		act(specials::RentMsg(specials::ERentMsg::kOfferStay), false, ch, 0, recep, kToRoom);
	} else {
		if ((save_room == r_helled_start_room)
			|| (save_room == r_named_start_room)
			|| (save_room == r_unreg_start_room))
			act(specials::RentMsg(specials::ERentMsg::kSettleForced), false, ch, 0, recep, kToChar);
		else {
			act(specials::RentMsg(specials::ERentMsg::kSettleOffer), false, recep, 0, ch, kToNotVict);
			act(specials::RentMsg(specials::ERentMsg::kSettleWelcome), false, ch, 0, recep, kToChar);
			sprintf(buf,
					"%s has changed loadroom from %d to %d.",
					GET_NAME(ch),
					GET_LOADROOM(ch),
					GET_ROOM_VNUM(save_room));
			GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
			mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
			SetBattleLag(ch, 1);
			ch->save_char();
		}
	}
	return (true);
}

namespace {
// issue.specials: rent-keeper subcommands. постой = rent+exit, предложение = show cost, поселиться =
// set this inn as home. (Plain quit/конец is do_quit; quitting next to a renter sets loadroom there.)
int RentRent(CharData *ch, void *me, char * /*rest*/) {
	return gen_receptionist(ch, reinterpret_cast<CharData *>(me), ERentAction::kRent, RENT_FACTOR);
}
int RentOffer(CharData *ch, void *me, char * /*rest*/) {
	return gen_receptionist(ch, reinterpret_cast<CharData *>(me), ERentAction::kOffer, RENT_FACTOR);
}
int RentSettle(CharData *ch, void *me, char * /*rest*/) {
	return gen_receptionist(ch, reinterpret_cast<CharData *>(me), ERentAction::kSettle, RENT_FACTOR);
}

const SubCmdResolver kRentCmds([] { return specials::RentMsg(specials::ERentMsg::kGreeting); }, {
	{{"постой", "rent"}, static_cast<int>(ERentAction::kRent), RentRent},
	{{"предложение", "offer"}, static_cast<int>(ERentAction::kOffer), RentOffer},
	{{"поселиться", "settle"}, static_cast<int>(ERentAction::kSettle), RentSettle},
});
} // namespace

int RentReceptionist(CharData *ch, void *me, int /*cmd*/, char *argument) {
	if (ch->IsNpc() || !ch->desc) {
		return 0;
	}
	return kRentCmds.Dispatch(ch, me, argument);
}

void Crash_frac_save_all(int frac_part) {
	DescriptorData *d;
	// OpenTelemetry: Track fractional save
	auto save_span = tracing::TraceManager::Instance().StartSpan("Player Save (Fractional)");
	save_span->SetAttribute("save_type", "frac");
	save_span->SetAttribute("frac_part", static_cast<int64_t>(frac_part));
	
	int saved_count = 0;

	for (d = descriptor_list; d; d = d->next) {
		if ((d->state == EConState::kPlaying) && !d->character->IsNpc() && GET_ACTIVITY(d->character) == frac_part) {

			utils::CExecutionTimer timer;
			Crash_crashsave(d->character.get());
			if (timer.delta().count() > 0.002)
				log("Crash_frac_save_all: Crash_crashsave, timer %f, save player: %s", timer.delta().count(), d->character->get_name().c_str());

			utils::CExecutionTimer timer1;
			d->character->save_char();
			if (timer1.delta().count() > 0.002)
				log("Crash_frac_save_all: save_char, timer %f, save player: %s", timer1.delta().count(), d->character->get_name().c_str());
			d->character->UnsetFlag(EPlrFlag::kCrashSave);
			saved_count++;
			
			// OpenTelemetry: Record save metrics
			std::map<std::string, std::string> attrs;
			attrs["save_type"] = "frac";
			attrs["character"] = d->character->get_name();
			
			observability::OtelMetrics::RecordHistogram("player.save.duration", timer.delta().count(), attrs);
			observability::OtelMetrics::RecordCounter("player.save.total", 1, attrs);
		}
	}
}

void Crash_save_all(void) {
	DescriptorData *d;
	auto save_span = tracing::TraceManager::Instance().StartSpan("Player Save (Full)");
	save_span->SetAttribute("save_type", "full");

	for (d = descriptor_list; d; d = d->next) {
		if ((d->state == EConState::kPlaying) && d->character->IsFlagged(EPlrFlag::kCrashSave)) {
			Crash_crashsave(d->character.get());
			d->character->save_char();
			d->character->UnsetFlag(EPlrFlag::kCrashSave);
		}
	}
}

// * Сейв при плановом ребуте/остановке с таймером != 0.
void Crash_save_all_rent(void) {
	character_list.foreach_on_copy([&](const auto &ch) {
		if (!ch->IsNpc()) {
			save_char_objects(ch.get(), RENT_FORCED, 0);
			log("Saving char: %s", GET_NAME(ch));
			ch->UnsetFlag(EPlrFlag::kCrashSave);
			//AFF_FLAGS(ch.get()).unset(EAffectFlag::AFF_GROUP);
			group::RemoveGroupFlags(ch.get());
			AFF_FLAGS(ch.get()).unset(EAffect::kHorse);
			ExtractCharFromWorld(ch.get(), false);
		}
	});
}

void Crash_frac_rent_time(int frac_part) {
	utils::CExecutionTimer timer;
	for (std::size_t c = 0; c < player_table.size(); c++) {
		if (player_table[c].activity == frac_part
			&& player_table[c].uid() != -1
			&& SAVEINFO(c)) {
			Crash_timer_obj(c, time(0));
		}
	}
	if (timer.delta().count() > 0.01)
		log("Crash_frac_rent_time: timer %f", timer.delta().count());
}

void Crash_rent_time(int/* dectime*/) {
	for (std::size_t c = 0; c < player_table.size(); c++) {
		if (player_table[c].uid() != -1) {
			Crash_timer_obj(c, time(0));
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
