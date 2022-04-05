/*   File: objsave.cpp                                     Part of Bylins  *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

// * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>

#include "obj_save.h"

#include "world_objects.h"
#include "entities/world_characters.h"
#include "obj_prototypes.h"
#include "handler.h"
#include "color.h"
#include "house.h"
#include "depot.h"
#include "liquid.h"
#include "utils/file_crc.h"
#include "game_mechanics/named_stuff.h"
#include "utils/utils_char_obj.inl"

#include <boost/algorithm/string.hpp>

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
extern RoomRnum r_unreg_start_room;

#define RENTCODE(number) (player_table[(number)].timer->rent.rentcode)
#define GET_INDEX(ch) (get_ptable_by_name(GET_NAME(ch)))

// Extern functions
void do_tell(CharData *ch, char *argument, int cmd, int subcmd);
int receptionist(CharData *ch, void *me, int cmd, char *argument);
int cryogenicist(CharData *ch, void *me, int cmd, char *argument);
int invalid_no_class(CharData *ch, const ObjData *obj);
int invalid_anti_class(CharData *ch, const ObjData *obj);
int invalid_unique(CharData *ch, const ObjData *obj);
int min_rent_cost(CharData *ch);
extern bool check_obj_in_system_zone(int vnum);
// local functions
void Crash_extract_norent_eq(CharData *ch);
int auto_equip(CharData *ch, ObjData *obj, int location);
int Crash_report_unrentables(CharData *ch, CharData *recep, ObjData *obj);
void Crash_report_rent(CharData *ch, CharData *recep, ObjData *obj,
					   int *cost, long *nitems, int rentshow, int factor, int equip, int recursive);
int gen_receptionist(CharData *ch, CharData *recep, int cmd, char *arg, int mode);
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

int get_buf_line(char **source, char *target) {
	char *otarget = target;
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
		*target = **source;
		if (!isspace(static_cast<unsigned char>(*target++)))
			empty = false;
		*target = '\0';
	}
	return (false);
}

int get_buf_lines(char **source, char *target) {
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
	ObjData::shared_ptr object;

	*error = 1;
	// Станем на начало предмета
	for (; **data != DIV_CHAR; (*data)++) {
		if (!**data || **data == END_CHAR) {
			return object;
		}
	}

	*error = 2;
	// Пропустим #
	(*data)++;

	// Считаем vnum предмета
	if (!get_buf_line(data, buffer)) {
		return object;
	}

	*error = 3;
	vnum = atoi(buffer);
	if (!vnum) {
		return object;
	}

	// Если без прототипа, создаем новый. Иначе читаем прототип и возвращаем NULL,
	// если прототип не существует.
	if (vnum < 0) {
		object = world_objects.create_blank();
	} else {
		object = world_objects.create_from_prototype_by_vnum(vnum);
		if (!object) {
			*error = 4;
			return nullptr;
		}
	}
	// Далее найденные параметры присваиваем к прототипу
	while (get_buf_lines(data, buffer)) {
		tag_argument(buffer, read_line);

		//if (read_line != NULL) read_line cannot be NULL because it is a local array
		{
			// Чтобы не портился прототип вещи некоторые параметры изменяются лишь у вещей с vnum < 0
			if (!strcmp(read_line, "Alia")) {
				*error = 6;
				object->set_aliases(buffer);
			} else if (!strcmp(read_line, "Pad0")) {
				*error = 7;
				object->set_PName(0, buffer);
				object->set_short_description(buffer);
			} else if (!strcmp(read_line, "Pad1")) {
				*error = 8;
				object->set_PName(1, buffer);
			} else if (!strcmp(read_line, "Pad2")) {
				*error = 9;
				object->set_PName(2, buffer);
			} else if (!strcmp(read_line, "Pad3")) {
				*error = 10;
				object->set_PName(3, buffer);
			} else if (!strcmp(read_line, "Pad4")) {
				*error = 11;
				object->set_PName(4, buffer);
			} else if (!strcmp(read_line, "Pad5")) {
				*error = 12;
				object->set_PName(5, buffer);
			} else if (!strcmp(read_line, "Desc")) {
				*error = 13;
				object->set_description(buffer);
			} else if (!strcmp(read_line, "ADsc")) {
				*error = 14;
				if (strcmp(buffer, "NULL")) {
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
				object->set_sex(static_cast<ESex>(atoi(buffer)));
			} else if (!strcmp(read_line, "Tmer")) {
				*error = 20;
				object->set_timer(atoi(buffer));
			} else if (!strcmp(read_line, "Spll")) {
				*error = 21;
				object->set_spell(atoi(buffer));
			} else if (!strcmp(read_line, "Levl")) {
				*error = 22;
				object->set_level(atoi(buffer));
			} else if (!strcmp(read_line, "Affs")) {
				*error = 23;
				object->set_affect_flags(clear_flags);
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
				object->set_weight(atoi(buffer));
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
			} else if (!strcmp(read_line, "Prnt")) {
				*error = 39;
				object->set_parent(atoi(buffer));
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
				ExtraDescription::shared_ptr new_descr(new ExtraDescription());
				new_descr->keyword = str_dup(buffer);
				if (!strcmp(new_descr->keyword, "None")) {
					object->set_ex_description(nullptr);
				} else {
					if (!get_buf_lines(data, buffer)) {
						*error = 47;
						return (object);
					}
					new_descr->description = str_dup(buffer);
					object->set_ex_description(new_descr);
				}
			} else if (!strcmp(read_line, "Ouid")) {
				*error = 48;
				object->set_uid(atoi(buffer));
			} else if (!strcmp(read_line, "TSpl")) {
				*error = 49;
				std::stringstream text(buffer);
				std::string tmp_buf;

				while (std::getline(text, tmp_buf)) {
					boost::trim(tmp_buf);
					if (tmp_buf.empty() || tmp_buf[0] == '~') {
						break;
					}
					if (sscanf(tmp_buf.c_str(), "%d %d", t, t + 1) != 2) {
						*error = 50;
						return object;
					}
					object->add_timed_spell(t[0], t[1]);
				}
			} else if (!strcmp(read_line, "Mort")) {
				*error = 51;
				object->set_minimum_remorts(atoi(buffer));
			} else if (!strcmp(read_line, "Ench")) {
				ObjectEnchant::enchant tmp_aff;
				std::stringstream text(buffer);
				std::string tmp_buf;

				while (std::getline(text, tmp_buf)) {
					boost::trim(tmp_buf);
					if (tmp_buf.empty() || tmp_buf[0] == '~') {
						break;
					}
					switch (tmp_buf[0]) {
						case 'I': tmp_aff.name_ = tmp_buf.substr(1);
							boost::trim(tmp_aff.name_);
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
				object->get_custom_label()->label_text = str_dup(buffer);
			} else if (!strcmp(read_line, "ClID")) // id чара
			{
				*error = 61;
				if (!object->get_custom_label()) {
					object->set_custom_label(new custom_label());
				}

				object->get_custom_label()->author = atoi(buffer);
				if (object->get_custom_label()->author > 0) {
					for (std::size_t i = 0; i < player_table.size(); i++) {
						if (player_table[i].id() == object->get_custom_label()->author) {
							object->get_custom_label()->author_mail = str_dup(player_table[i].mail);
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
				object->get_custom_label()->clan = str_dup(buffer);
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
	if (GET_OBJ_TYPE(object) == EObjType::kLiquidContainer
		|| GET_OBJ_TYPE(object) == EObjType::kFountain) {
		if (GET_OBJ_WEIGHT(object) < GET_OBJ_VAL(object, 1)) {
			object->set_weight(GET_OBJ_VAL(object, 1) + 5);
		}
	}
	// проставляем имя жидкости
	if (GET_OBJ_TYPE(object) == EObjType::kLiquidContainer) {
		name_from_drinkcon(object.get());
		if (GET_OBJ_VAL(object, 1) && GET_OBJ_VAL(object, 2)) {
			name_to_drinkcon(object.get(), GET_OBJ_VAL(object, 2));
		}
	}
	// Проверка на ингры
	if (GET_OBJ_TYPE(object) == EObjType::kMagicIngredient) {
		int err = im_assign_power(object.get());
		if (err) {
			*error = 100 + err;
		}
	}
	if (object->has_flag(EObjFlag::kNamed))//Именной предмет
	{
		object->cleanup_script();
	}
	convert_drinkcon_skill(object.get(), false);
	object->remove_incorrect_values_keys(GET_OBJ_TYPE(object));

	return (object);
}

// Данная процедура выбирает предмет из буфера
// ВНИМАНИЕ!!! эта функция используется только для чтения вещей персонажа,
// сохраненных в старом формате, для чтения нового формата применяется ф-ия read_one_object_new
ObjData::shared_ptr read_one_object(char **data, int *error) {
	char buffer[kMaxStringLength], f0[kMaxStringLength], f1[kMaxStringLength], f2[kMaxStringLength];
	int vnum, i, j, t[5];

	*error = 1;
	// Станем на начало предмета
	for (; **data != DIV_CHAR; (*data)++) {
		if (!**data || **data == END_CHAR) {
			return nullptr;
		}
	}
	*error = 2;
	// Пропустим #
	(*data)++;
	// Считаем vnum предмета
	if (!get_buf_line(data, buffer)) {
		return nullptr;
	}
	*error = 3;
	if (!(vnum = atoi(buffer))) {
		return nullptr;
	}

	ObjData::shared_ptr object;
	if (vnum < 0)        // Предмет не имеет прототипа
	{
		object = world_objects.create_blank();
		*error = 4;
		if (!get_buf_lines(data, buffer)) {
			return object;
		}
		// Алиасы
		object->set_aliases(buffer);
		// Падежи
		*error = 5;
		for (i = 0; i < CObjectPrototype::NUM_PADS; i++) {
			if (!get_buf_lines(data, buffer)) {
				return object;
			}

			object->set_PName(i, buffer);
			if (i == 0) {
				object->set_short_description(buffer);
			}
		}
		// Описание когда на земле
		*error = 6;
		if (!get_buf_lines(data, buffer)) {
			return object;
		}
		object->set_description(buffer);
		// Описание при действии
		*error = 7;
		if (!get_buf_lines(data, buffer)) {
			return object;
		}
		object->set_action_description(buffer);
	} else {
		object = world_objects.create_from_prototype_by_vnum(vnum);
		if (!object) {
			*error = 8;
			return nullptr;
		}
	}

	*error = 9;
	if (!get_buf_line(data, buffer)
		|| sscanf(buffer, " %s %d %d %d", f0, t + 1, t + 2, t + 3) != 4) {
		return object;
	}

	int skill = 0;
	asciiflag_conv(f0, &skill);
	object->set_skill(skill);

	object->set_maximum_durability(t[1]);
	object->set_current_durability(t[2]);
	object->set_material(static_cast<EObjMaterial>(t[3]));

	*error = 10;
	if (!get_buf_line(data, buffer)
		|| sscanf(buffer, " %d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
		return object;
	}
	object->set_sex(static_cast<ESex>(t[0]));
	object->set_timer(t[1]);
	object->set_spell(t[2]);
	object->set_level(t[3]);

	*error = 11;
	if (!get_buf_line(data, buffer)
		|| sscanf(buffer, " %s %s %s", f0, f1, f2) != 3) {
		return object;
	}
	object->set_affect_flags(clear_flags);
	object->set_anti_flags(clear_flags);
	object->set_no_flags(clear_flags);
	object->load_affect_flags(f0);
	object->load_anti_flags(f0);
	object->load_no_flags(f0);

	*error = 12;
	if (!get_buf_line(data, buffer)
		|| sscanf(buffer, " %d %s %s", t, f1, f2) != 3) {
		return object;
	}
	object->set_type(static_cast<EObjType>(t[0]));
	object->set_extra_flags(clear_flags);
	object->set_wear_flags(0);
	object->load_extra_flags(f1);

	auto wear = object->get_wear_flags();
	asciiflag_conv(f2, &wear);
	object->set_wear_flags(wear);

	*error = 13;
	if (!get_buf_line(data, buffer)
		|| sscanf(buffer, "%s %d %d %d", f0, t + 1, t + 2, t + 3) != 4) {
		return object;
	}
	object->set_val(0, 0);
	auto val0 = object->get_val(0);
	asciiflag_conv(f0, &val0);

	object->set_val(0, val0);
	object->set_val(1, t[1]);
	object->set_val(2, t[2]);
	object->set_val(3, t[3]);

	*error = 14;
	if (!get_buf_line(data, buffer)
		|| sscanf(buffer, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
		return object;
	}
	object->set_weight(t[0]);
	object->set_cost(t[1]);
	object->set_rent_off(t[2]);
	object->set_rent_on(t[3]);

	*error = 15;
	if (!get_buf_line(data, buffer)
		|| sscanf(buffer, "%d %d", t, t + 1) != 2) {
		return object;
	}
	object->set_worn_on(t[0]);
	object->set_owner(t[1]);

	// Проверить вес фляг и т.п.
	if (GET_OBJ_TYPE(object) == EObjType::kLiquidContainer
		|| GET_OBJ_TYPE(object) == EObjType::kFountain) {
		if (GET_OBJ_WEIGHT(object) < GET_OBJ_VAL(object, 1)) {
			object->set_weight(GET_OBJ_VAL(object, 1) + 5);
		}
	}

	object->set_ex_description(nullptr);    // Exclude doubling ex_description !!!
	j = 0;

	for (;;) {
		if (!get_buf_line(data, buffer)) {
			*error = 0;
			for (; j < kMaxObjAffect; j++) {
				object->set_affected(j, EApply::kNone, 0);
			}

			if (GET_OBJ_TYPE(object) == EObjType::kMagicIngredient) {
				int err = im_assign_power(object.get());
				if (err) {
					*error = 100 + err;
				}
			}

			return object;
		}

		switch (*buffer) {
			case 'E': {
				ExtraDescription::shared_ptr new_descr(new ExtraDescription());
				if (!get_buf_lines(data, buffer)) {
					*error = 16;
					return object;
				}
				new_descr->keyword = str_dup(buffer);
				if (!get_buf_lines(data, buffer)) {
					*error = 17;
					return object;
				}
				new_descr->description = str_dup(buffer);
				new_descr->next = object->get_ex_description();
				object->set_ex_description(new_descr);
			}
				break;

			case 'A':
				if (j >= kMaxObjAffect) {
					*error = 18;
					return object;
				}
				if (!get_buf_line(data, buffer)) {
					*error = 19;
					return object;
				}
				if (sscanf(buffer, " %d %d ", t, t + 1) == 2) {
					object->set_affected(j, static_cast<EApply>(t[0]), t[1]);
					j++;
				}
				break;

			case 'M':
				// Вставляем сюда уникальный номер создателя
				if (!get_buf_line(data, buffer)) {
					*error = 20;
					return object;
				}
				if (sscanf(buffer, " %d ", t) == 1) {
					object->set_crafter_uid(t[0]);
				}
				break;

			case 'P':
				if (!get_buf_line(data, buffer)) {
					*error = 21;
					return object;
				}
				if (sscanf(buffer, " %d ", t) == 1) {
					int rnum;
					object->set_parent(t[0]);
					rnum = real_mobile(GET_OBJ_PARENT(object));
					if (rnum > -1) {
						trans_obj_name(object.get(), &mob_proto[rnum]);
					}
				}
				break;

			default: break;
		}
	}
	*error = 22;

	return object;
}

// shapirus: функция проверки наличия доп. описания в прототипе
inline bool proto_has_descr(const ExtraDescription::shared_ptr &odesc, const ExtraDescription::shared_ptr &pdesc) {
	for (auto desc = pdesc; desc; desc = desc->next) {
		if (!str_cmp(odesc->keyword, desc->keyword)
			&& !str_cmp(odesc->description, desc->description)) {
			return true;
		}
	}

	return false;
}

// Данная процедура помещает предмет в буфер
// [ ИСПОЛЬЗУЕТСЯ В НОВОМ ФОРМАТЕ ВЕЩЕЙ ПЕРСОНАЖА ОТ 10.12.04 ]
void write_one_object(std::stringstream &out, ObjData *object, int location) {
	char buf[kMaxStringLength];
	char buf2[kMaxStringLength];
	int i, j;

	// vnum
	out << "#" << GET_OBJ_VNUM(object) << "\n";

	// Положение в экипировке (сохраняем только если > 0)
	if (location) {
		out << "Lctn: " << location << "~\n";
	}

	// Если у шмотки есть прототип то будем сохранять по обрезанной схеме, иначе
	// придется сохранять все статсы шмотки.
	auto proto = get_object_prototype(GET_OBJ_VNUM(object));

/*	auto obj_ptr = world_objects.get_by_raw_ptr(object);
	if (!obj_ptr)
	{
		log("Object was purged.");
		//return;
	}
*/
//	log("Write one object: %s", object->get_PName(0).c_str());

	if (GET_OBJ_VNUM(object) >= 0 && proto) {
		// Сохраняем UID
		out << "Ouid: " << GET_OBJ_UID(object) << "~\n";
		// Алиасы
		if (str_cmp(GET_OBJ_ALIAS(object), GET_OBJ_ALIAS(proto))) {
			out << "Alia: " << GET_OBJ_ALIAS(object) << "~\n";
		}
		// Падежи
		for (i = 0; i < CObjectPrototype::NUM_PADS; i++) {
			if (str_cmp(GET_OBJ_PNAME(object, i), GET_OBJ_PNAME(proto, i))) {
				out << "Pad" << i << ": " << GET_OBJ_PNAME(object, i) << "~\n";
			}
		}
		// Описание когда на земле
		if (!GET_OBJ_DESC(proto).empty()
			&& str_cmp(GET_OBJ_DESC(object), GET_OBJ_DESC(proto))) {
			out << "Desc: " << GET_OBJ_DESC(object) << "~\n";
		}

		// Описание при действии
		if (!GET_OBJ_ACT(object).empty()
			&& !GET_OBJ_ACT(proto).empty()) {
			if (str_cmp(GET_OBJ_ACT(object), GET_OBJ_ACT(proto))) {
				out << "ADsc: " << GET_OBJ_ACT(object) << "~\n";
			}
		}

		if (object->has_skills()) {
			CObjectPrototype::skills_t tmp_skills_object_;
			CObjectPrototype::skills_t tmp_skills_proto_;
			object->get_skills(tmp_skills_object_);
			proto->get_skills(tmp_skills_proto_);
			// Тренируемый скилл
			if (tmp_skills_object_ != tmp_skills_proto_) {
				for (const auto &it : tmp_skills_object_) {
					out << "Skil: " << to_underlying(it.first) << " " << it.second << "~\n";
				}

			}
		}
		// Макс. прочность
		if (GET_OBJ_MAX(object) != GET_OBJ_MAX(proto)) {
			out << "Maxx: " << GET_OBJ_MAX(object) << "~\n";
		}
		// Текущая прочность
		if (GET_OBJ_CUR(object) != GET_OBJ_CUR(proto)) {
			out << "Curr: " << GET_OBJ_CUR(object) << "~\n";
		}
		// Материал
		if (GET_OBJ_MATER(object) != GET_OBJ_MATER(proto)) {
			out << "Mter: " << GET_OBJ_MATER(object) << "~\n";
		}
		// Пол
		if (GET_OBJ_SEX(object) != GET_OBJ_SEX(proto)) {
			out << "Sexx: " << static_cast<int>(GET_OBJ_SEX(object)) << "~\n";
		}
		// Таймер
		if (object->get_timer() != proto->get_timer()) {
			out << "Tmer: " << object->get_timer() << "~\n";
			if (!check_obj_in_system_zone(GET_OBJ_VNUM(object))) //если шмот в служебной зоне, то таймер не трогаем
			{
				// на тот случай, если есть объекты с таймером, больше таймера прототипа
				int temp_timer = obj_proto[GET_OBJ_RNUM(object)]->get_timer();
				if (object->get_timer() > temp_timer)
					object->set_timer(temp_timer);
			}
		}
		// Сложность замкА
		if (GET_OBJ_SPELL(object) != GET_OBJ_SPELL(proto)) {
			out << "Spll: " << GET_OBJ_SPELL(object) << "~\n";
		}
		// Уровень заклинания
		if (GET_OBJ_LEVEL(object) != GET_OBJ_LEVEL(proto)) {
			out << "Levl: " << GET_OBJ_LEVEL(object) << "~\n";
		}
		// была ли шмотка ренейм
		if (GET_OBJ_RENAME(object) != false) {
			out << "Rnme: 1~\n";
		}
		// скрафчено с таймером
		if ((GET_OBJ_CRAFTIMER(object) > 0)) {
			out << "Ctmr: " << GET_OBJ_CRAFTIMER(object) << "~\n";
		}
		// в какой зоне было загружено в мир
		if (GET_OBJ_VNUM_ZONE_FROM(object)) {
			out << "Ozne: " << GET_OBJ_VNUM_ZONE_FROM(object) << "~\n";
		}
		// Наводимые аффекты
		*buf = '\0';
		*buf2 = '\0';
		GET_OBJ_AFFECTS(object).tascii(4, buf);
		GET_OBJ_AFFECTS(proto).tascii(4, buf2);
		if (str_cmp(buf, buf2)) {
			out << "Affs: " << buf << "~\n";
		}
		// Анти флаги
		*buf = '\0';
		*buf2 = '\0';
		GET_OBJ_ANTI(object).tascii(4, buf);
		GET_OBJ_ANTI(proto).tascii(4, buf2);
		if (str_cmp(buf, buf2)) {
			out << "Anti: " << buf << "~\n";
		}
		// Запрещающие флаги
		*buf = '\0';
		*buf2 = '\0';
		GET_OBJ_NO(object).tascii(4, buf);
		GET_OBJ_NO(proto).tascii(4, buf2);
		if (str_cmp(buf, buf2)) {
			out << "Nofl: " << buf << "~\n";
		}
		// Экстра флаги
		*buf = '\0';
		*buf2 = '\0';
		//Временно убираем флаг !окровавлен! с вещи, чтобы он не сохранялся
		bool blooded = object->has_flag(EObjFlag::kBloody);
		if (blooded) {
			object->unset_extraflag(EObjFlag::kBloody);
		}
		GET_OBJ_EXTRA(object).tascii(4, buf);
		GET_OBJ_EXTRA(proto).tascii(4, buf2);
		if (blooded) //Возвращаем флаг назад
		{
			object->set_extra_flag(EObjFlag::kBloody);
		}
		if (str_cmp(buf, buf2)) {
			out << "Extr: " << buf << "~\n";
		}
		// Флаги слотов экипировки
		*buf = '\0';
		*buf2 = '\0';

		auto wear = object->get_wear_flags();
		tascii(&wear, 1, buf);

		wear = proto->get_wear_flags();
		tascii(&wear, 1, buf2);

		if (str_cmp(buf, buf2)) {
			out << "Wear: " << buf << "~\n";
		}
		// Тип предмета
		if (GET_OBJ_TYPE(object) != GET_OBJ_TYPE(proto)) {
			out << "Type: " << GET_OBJ_TYPE(object) << "~\n";
		}
		// Значение 0, Значение 1, Значение 2, Значение 3.
		for (i = 0; i < 4; i++) {
			if (GET_OBJ_VAL(object, i) != GET_OBJ_VAL(proto, i)) {
				out << "Val" << i << ": " << GET_OBJ_VAL(object, i) << "~\n";
			}
		}
		// Вес
		if (GET_OBJ_WEIGHT(object) != GET_OBJ_WEIGHT(proto)) {
			out << "Weig: " << GET_OBJ_WEIGHT(object) << "~\n";
		}
		// Цена
		if (GET_OBJ_COST(object) != GET_OBJ_COST(proto)) {
			out << "Cost: " << GET_OBJ_COST(object) << "~\n";
		}
		// Рента (снято)
		if (GET_OBJ_RENT(object) != GET_OBJ_RENT(proto)) {
			out << "Rent: " << GET_OBJ_RENT(object) << "~\n";
		}
		// Рента (одето)
		if (GET_OBJ_RENTEQ(object) != GET_OBJ_RENTEQ(proto)) {
			out << "RntQ: " << GET_OBJ_RENTEQ(object) << "~\n";
		}
		// Владелец
		if (GET_OBJ_OWNER(object) != ObjData::DEFAULT_OWNER) {
			out << "Ownr: " << GET_OBJ_OWNER(object) << "~\n";
		}
		// Создатель
		if (GET_OBJ_MAKER(object) != ObjData::DEFAULT_MAKER) {
			out << "Mker: " << GET_OBJ_MAKER(object) << "~\n";
		}
		// Родитель
		if (GET_OBJ_PARENT(object) != ObjData::DEFAULT_PARENT) {
			out << "Prnt: " << GET_OBJ_PARENT(object) << "~\n";
		}

		// Аффекты
		for (j = 0; j < kMaxObjAffect; j++) {
			const auto &oaff = object->get_affected(j);
			const auto &paff = proto->get_affected(j);
			if (oaff.location != paff.location
				|| oaff.modifier != paff.modifier) {
				out << "Afc" << j << ": " << oaff.location << " " << oaff.modifier << "~\n";
			}
		}

		// Доп описания
		// shapirus: исправлена ошибка с несохранением, например, меток
		// на крафтеных луках
		for (auto descr = object->get_ex_description(); descr; descr = descr->next) {
			if (proto_has_descr(descr, proto->get_ex_description())) {
				continue;
			}
			out << "Edes: " << (descr->keyword ? descr->keyword : "") << "~\n"
				<< (descr->description ? descr->description : "") << "~\n";
		}

		// Если у прототипа есть описание, а у сохраняемой вещи - нет, сохраняем None
		if (!object->get_ex_description()
			&& proto->get_ex_description()) {
			out << "Edes: None~\n";
		}

		// требования по мортам
		if (object->get_auto_mort_req() > 0)
//			&& object->get_manual_mort_req() != proto->get_manual_mort_req())
		{
			out << "Mort: " << object->get_auto_mort_req() << "~\n";
		}

		// ObjectValue предмета, если есть что сохранять
		if (object->get_all_values() != proto->get_all_values()) {
			out << object->serialize_values();
		}
	} else        // Если у шмотки нет прототипа - придется сохранять ее целиком.
				//6.12.2021 такого уже не может быть можно удалить нахрен
	{
		// Алиасы
		if (!GET_OBJ_ALIAS(object).empty()) {
			out << "Alia: " << GET_OBJ_ALIAS(object) << "~\n";
		}

		// Падежи
		for (i = 0; i < CObjectPrototype::NUM_PADS; i++) {
			if (!GET_OBJ_PNAME(object, i).empty()) {
				out << "Pad" << i << ": " << GET_OBJ_PNAME(object, i) << "~\n";
			}
		}

		// Описание когда на земле
		if (!GET_OBJ_DESC(object).empty()) {
			out << "Desc: " << GET_OBJ_DESC(object) << "~\n";
		}

		// Описание при действии
		if (!GET_OBJ_ACT(object).empty()) {
			out << "ADsc: " << GET_OBJ_ACT(object) << "~\n";
		}

		// Тренируемый скилл
		if (object->has_skills()) {
			CObjectPrototype::skills_t tmp_skills_object_;
			object->get_skills(tmp_skills_object_);
			for (const auto &it : tmp_skills_object_) {
				out << "Skil: " << to_underlying(it.first) << " " << it.second << "~\n";
			}
		}

		// Макс. прочность
		if (GET_OBJ_MAX(object)) {
			out << "Maxx: " << GET_OBJ_MAX(object) << "~\n";
		}

		// Текущая прочность
		if (GET_OBJ_CUR(object)) {
			out << "Curr: " << GET_OBJ_CUR(object) << "~\n";
		}
		// Материал
		if (GET_OBJ_MATER(object)) {
			out << "Mter: " << GET_OBJ_MATER(object) << "~\n";
		}
		// Пол
		if (ESex::kNeutral != GET_OBJ_SEX(object)) {
			out << "Sexx: " << static_cast<int>(GET_OBJ_SEX(object)) << "~\n";
		}
		// Таймер
		if (object->get_timer()) {
			out << "Tmer: " << object->get_timer() << "~\n";
		}
		// Сложность замкА
		if (GET_OBJ_SPELL(object)) {
			out << "Spll: " << GET_OBJ_SPELL(object) << "~\n";
		}
		// Уровень заклинания
		if (GET_OBJ_LEVEL(object)) {
			out << "Levl: " << GET_OBJ_LEVEL(object) << "~\n";
		}
		// Наводимые аффекты
		*buf = '\0';
		GET_OBJ_AFFECTS(object).tascii(4, buf);
		out << "Affs: " << buf << "~\n";
		// Анти флаги
		*buf = '\0';
		GET_OBJ_ANTI(object).tascii(4, buf);
		out << "Anti: " << buf << "~\n";
		// Запрещающие флаги
		*buf = '\0';
		GET_OBJ_NO(object).tascii(4, buf);
		out << "Nofl: " << buf << "~\n";
		// Экстра флаги
		*buf = '\0';
		GET_OBJ_EXTRA(object).tascii(4, buf);
		out << "Extr: " << buf << "~\n";

		// Флаги слотов экипировки
		*buf = '\0';
		auto wear = GET_OBJ_WEAR(object);
		tascii(&wear, 1, buf);
		out << "Wear: " << buf << "~\n";

		// Тип предмета
		out << "Type: " << GET_OBJ_TYPE(object) << "~\n";
		// Значение 0, Значение 1, Значение 2, Значение 3.
		for (i = 0; i < 4; i++) {
			if (GET_OBJ_VAL(object, i)) {
				out << "Val" << i << ": " << GET_OBJ_VAL(object, i) << "~\n";
			}
		}
		// Вес
		if (GET_OBJ_WEIGHT(object)) {
			out << "Weig: " << GET_OBJ_WEIGHT(object) << "~\n";
		}
		// Цена
		if (GET_OBJ_COST(object)) {
			out << "Cost: " << GET_OBJ_COST(object) << "~\n";
		}
		// Рента (снято)
		if (GET_OBJ_RENT(object)) {
			out << "Rent: " << GET_OBJ_RENT(object) << "~\n";
		}
		// Рента (одето)
		if (GET_OBJ_RENTEQ(object)) {
			out << "RntQ: " << GET_OBJ_RENTEQ(object) << "~\n";
		}
		// Владелец
		if (GET_OBJ_OWNER(object)) {
			out << "Ownr: " << GET_OBJ_OWNER(object) << "~\n";
		}
		// Создатель
		if (GET_OBJ_MAKER(object)) {
			out << "Mker: " << GET_OBJ_MAKER(object) << "~\n";
		}
		// Родитель
		if (GET_OBJ_PARENT(object)) {
			out << "Prnt: " << GET_OBJ_PARENT(object) << "~\n";
		}
		// была ли шмотка ренейм
		if (GET_OBJ_RENAME(object))
			out << "Rnme: 1~\n";
		else
			out << "Rnme: 0~\n";
		// есть ли на шмотке таймер при крафте
		if (GET_OBJ_CRAFTIMER(object) > 0) {
			out << "Ctmr: " << GET_OBJ_CRAFTIMER(object) << "~\n";
		}
		// в какой зоне было загружено в мир
		if (GET_OBJ_VNUM_ZONE_FROM(object)) {
			out << "Ozne: " << GET_OBJ_VNUM_ZONE_FROM(object) << "~\n";
		}
		// Аффекты
		for (j = 0; j < kMaxObjAffect; j++) {
			if (object->get_affected(j).location
				&& object->get_affected(j).modifier) {
				out << "Afc" << j << ": " << object->get_affected(j).location
					<< " " << object->get_affected(j).modifier << "~\n";
			}
		}

		// Доп описания
		for (auto descr = object->get_ex_description(); descr; descr = descr->next) {
			out << "Edes: " << (descr->keyword ? descr->keyword : "") << "~\n"
				<< (descr->description ? descr->description : "") << "~\n";
		}

		// требования по мортам
		if (object->get_auto_mort_req() > 0) {
			out << "Mort: " << object->get_auto_mort_req() << "~\n";
		}

		// ObjectValue предмета, если есть что сохранять
		out << object->serialize_values();
	}

	// обкаст (если он есть) сохраняется в любом случае независимо от прототипа
	if (object->has_timed_spell()) {
		out << object->timed_spell().print();
	}

	// накладываемые энчанты
	if (!object->get_enchants().empty()) {
		out << object->serialize_enchants();
	}

	// кастомная метка
	if (object->get_custom_label()) {
		out << "Clbl: " << object->get_custom_label()->label_text << "~\n";
		out << "ClID: " << object->get_custom_label()->author << "~\n";
		if (object->get_custom_label()->clan) {
			out << "ClCl: " << object->get_custom_label()->clan << "~\n";
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
	char filename[kMaxStringLength + 1], name[kMaxNameLength + 1];
	FILE *fl;
	int retcode = false;

	if (static_cast<int>(index) < 0) {
		return retcode;
	}

	strcpy(name, player_table[index].name());

	//удаляем файл описания объектов
	if (!get_filename(name, filename, kTextCrashFile)) {
		log("SYSERR: Error deleting objects file for %s - unable to resolve file name.", name);
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
			FileCRC::check_crc(filename, FileCRC::UPDATE_TEXTOBJS, player_table[index].unique);
		}
	}

	//удаляем файл таймеров
	if (!get_filename(name, filename, kTimeCrashFile)) {
		log("SYSERR: Error deleting timer file for %s - unable to resolve file name.", name);
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
			FileCRC::check_crc(filename, FileCRC::UPDATE_TIMEOBJS, player_table[index].unique);
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

void Crash_clear_objects(const std::size_t index) {
	int i = 0, rnum;
	Crash_delete_files(index);
	if (SAVEINFO(index)) {
		for (; i < SAVEINFO(index)->rent.nitems; i++) {
			if (SAVEINFO(index)->time[i].timer >= 0 &&
				(rnum = real_object(SAVEINFO(index)->time[i].vnum)) >= 0) {
				obj_proto.dec_stored(rnum);
			}
		}
		clear_saveinfo(index);
	}
}

void Crash_create_timer(const std::size_t index, int/* num*/) {
	recreate_saveinfo(index);
}

int Crash_read_timer(const std::size_t index, int temp) {
	FILE *fl;
	char fname[kMaxInputLength];
	char name[kMaxNameLength + 1];
	int size = 0, count = 0, rnum, num = 0;
	struct SaveRentInfo rent;
	struct SaveTimeInfo info;

	strcpy(name, player_table[index].name());
	if (!get_filename(name, fname, kTimeCrashFile)) {
		log("[ReadTimer] Error reading %s timer file - unable to resolve file name.", name);
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
	size = ftell(fl);
	rewind(fl);
	if ((size = size - sizeof(struct SaveRentInfo)) < 0 || size % sizeof(struct SaveTimeInfo)) {
		log("WARNING:  Timer file %s is corrupt!", fname);
		return false;
	}

	sprintf(buf, "[ReadTimer] Reading timer file %s for %s :", fname, name);
	size_t dummy = fread(&rent, sizeof(struct SaveRentInfo), 1, fl);
	switch (rent.rentcode) {
		case RENT_RENTED: strcat(buf, " Rent ");
			break;
		case RENT_CRASH:
			//           if (rent.time<1001651000L) //креш-сейв до Sep 28 00:26:20 2001
			rent.net_cost_per_diem = 0;    //бесплатно!
			strcat(buf, " Crash ");
			break;
		case RENT_CRYO: strcat(buf, " Cryo ");
			break;
		case RENT_TIMEDOUT: strcat(buf, " TimedOut ");
			break;
		case RENT_FORCED: strcat(buf, " Forced ");
			break;
		default: log("[ReadTimer] Error reading %s timer file - undefined rent code.", name);
			return false;
			//strcat(buf, " Undef ");
			//rent.rentcode = RENT_CRASH;
			break;
	}
	strcat(buf, "rent code.");
	log("%s", buf);
	Crash_create_timer(index, rent.nitems);
	player_table[index].timer->rent = rent;
	for (; count < rent.nitems && !feof(fl); count++) {
		dummy = fread(&info, sizeof(struct SaveTimeInfo), 1, fl);
		if (ferror(fl)) {
			log("SYSERR: I/O Error reading %s timer file.", name);
			fclose(fl);
			FileCRC::check_crc(fname, FileCRC::TIMEOBJS, player_table[index].unique);
			clear_saveinfo(index);
			return false;
		}
		if (info.vnum && info.timer >= -1) {
			player_table[index].timer->time.push_back(info);
			++num;
		} else {
			log("[ReadTimer] Warning: incorrect vnum (%d) or timer (%d) while reading %s timer file.",
				info.vnum, info.timer, name);
		}
		if (info.timer >= 0 && (rnum = real_object(info.vnum)) >= 0 && !temp) {
			obj_proto.inc_stored(rnum);
		}
	}
	UNUSED_ARG(dummy);

	fclose(fl);
	FileCRC::check_crc(fname, FileCRC::TIMEOBJS, player_table[index].unique);

	if (rent.nitems != num) {
		log("[ReadTimer] Error reading %s timer file - file is corrupt.", fname);
		clear_saveinfo(index);
		return false;
	} else
		return true;
}

void Crash_reload_timer(int index) {
	int i = 0, rnum;
	if (SAVEINFO(index)) {
		for (; i < SAVEINFO(index)->rent.nitems; i++) {
			if (SAVEINFO(index)->time[i].timer >= 0 &&
				(rnum = real_object(SAVEINFO(index)->time[i].vnum)) >= 0) {
				obj_proto.dec_stored(rnum);
			}
		}
		clear_saveinfo(index);
	}

	if (!Crash_read_timer(index, false)) {
		sprintf(buf, "SYSERR: Unable to read timer file for %s.", player_table[index].name());
		mudlog(buf, BRF, MAX(kLvlImmortal, kLvlGod), SYSLOG, true);
	}
}

int Crash_write_timer(const std::size_t index) {
	FILE *fl;
	char fname[kMaxStringLength];
	char name[kMaxNameLength + 1];

	strcpy(name, player_table[index].name());
	if (!SAVEINFO(index)) {
		log("SYSERR: Error writing %s timer file - no data.", name);
		return false;
	}
	if (!get_filename(name, fname, kTimeCrashFile)) {
		log("SYSERR: Error writing %s timer file - unable to resolve file name.", name);
		return false;
	}
	if (!(fl = fopen(fname, "wb"))) {
		log("[WriteTimer] Error writing %s timer file - unable to open file %s.", name, fname);
		return false;
	}
	fwrite(&(SAVEINFO(index)->rent), sizeof(SaveRentInfo), 1, fl);
	for (int i = 0; i < SAVEINFO(index)->rent.nitems; ++i) {
		fwrite(&(SAVEINFO(index)->time[i]), sizeof(SaveTimeInfo), 1, fl);
	}
	fclose(fl);
	FileCRC::check_crc(fname, FileCRC::UPDATE_TIMEOBJS, player_table[index].unique);
	return true;
}

void Crash_timer_obj(const std::size_t index, long time) {
	char name[kMaxNameLength + 1];
	int nitems = 0, idelete = 0, ideleted = 0, rnum, timer, i;
	int rentcode, timer_dec;

#ifndef USE_AUTOEQ
	return;
#endif

	strcpy(name, player_table[index].name());

	if (!player_table[index].timer) {
		log("[TO] %s has no save data.", name);
		return;
	}
	rentcode = SAVEINFO(index)->rent.rentcode;
	timer_dec = time - SAVEINFO(index)->rent.time;

	//удаляем просроченные файлы ренты
	if (rentcode == RENT_RENTED && timer_dec > rent_file_timeout * kSecsPerRealDay) {
		Crash_clear_objects(index);
		log("[TO] Deleting %s's rent info - time outed.", name);
		return;
	} else if (rentcode != RENT_CRYO && timer_dec > crash_file_timeout * kSecsPerRealDay) {
		Crash_clear_objects(index);
		strcpy(buf, "");
		switch (rentcode) {
			case RENT_CRASH: log("[TO] Deleting crash rent info for %s  - time outed.", name);
				break;
			case RENT_FORCED: log("[TO] Deleting forced rent info for %s  - time outed.", name);
				break;
			case RENT_TIMEDOUT: log("[TO] Deleting autorent info for %s  - time outed.", name);
				break;
			default: log("[TO] Deleting UNKNOWN rent info for %s  - time outed.", name);
				break;
		}
		return;
	}

	timer_dec = (timer_dec / kSecsPerMudHour) + (timer_dec % kSecsPerMudHour ? 1 : 0);

	//уменьшаем таймеры
	nitems = player_table[index].timer->rent.nitems;
//  log("[TO] Checking items for %s (%d items, rented time %dmin):",
//      name, nitems, timer_dec);
	//sprintf (buf,"[TO] Checking items for %s (%d items) :", name, nitems);
	//mudlog(buf, BRF, kLevelImmortal, SYSLOG, true);
	for (i = 0; i < nitems; i++) {
		if (player_table[index].timer->time[i].vnum < 0) //для шмоток без прототипа идем мимо
			continue;
		if (player_table[index].timer->time[i].timer >= 0) {
			rnum = real_object(player_table[index].timer->time[i].vnum);
			if (!check_unlimited_timer(obj_proto[rnum].get())) {
				timer = player_table[index].timer->time[i].timer;
				if (timer < timer_dec) {
					player_table[index].timer->time[i].timer = -1;
					idelete++;
					if (rnum >= 0) {
						obj_proto.dec_stored(rnum);
						log("[TO] Player %s : item %s deleted - time outted", name, obj_proto[rnum]->get_PName(0).c_str());
					}
				}
			}
		} else {
			ideleted++;
		}
	}

//  log("Objects (%d), Deleted (%d)+(%d).", nitems, ideleted, idelete);

	//если появились новые просроченные объекты, обновляем файл таймеров
	if (idelete) {
		if (!Crash_write_timer(index)) {
			sprintf(buf, "SYSERR: [TO] Error writing timer file for %s.", name);
			mudlog(buf, CMP, MAX(kLvlImmortal, kLvlGod), SYSLOG, true);
		}
	}
}

void Crash_list_objects(CharData *ch, int index) {
	int i = 0, rnum;
	struct SaveTimeInfo data;
	long timer_dec;
	float num_of_days;

	if (!SAVEINFO(index))
		return;

	timer_dec = time(0) - SAVEINFO(index)->rent.time;
	num_of_days = (float) timer_dec / kSecsPerRealDay;
	timer_dec = (timer_dec / kSecsPerMudHour) + (timer_dec % kSecsPerMudHour ? 1 : 0);

	strcpy(buf, "Код ренты - ");
	switch (SAVEINFO(index)->rent.rentcode) {
		case RENT_RENTED: strcat(buf, "Rented.\r\n");
			break;
		case RENT_CRASH: strcat(buf, "Crash.\r\n");
			break;
		case RENT_CRYO: strcat(buf, "Cryo.\r\n");
			break;
		case RENT_TIMEDOUT: strcat(buf, "TimedOut.\r\n");
			break;
		case RENT_FORCED: strcat(buf, "Forced.\r\n");
			break;
		default: strcat(buf, "UNDEF!\r\n");
			break;
	}

	for (; i < SAVEINFO(index)->rent.nitems; i++) {
		data = SAVEINFO(index)->time[i];
		if (((rnum = real_object(data.vnum)) > -1) && ((data.vnum > 799) || (data.vnum < 700))) {
			sprintf(buf + strlen(buf), " [%5d] (%5dau) <%6d> %-20s\r\n",
					data.vnum, GET_OBJ_RENT(obj_proto[rnum]),
					MAX(-1, data.timer - timer_dec), obj_proto[rnum]->get_short_description().c_str());
		} else if ((data.vnum > 799) || (data.vnum < 700)) {
			sprintf(buf + strlen(buf), " [%5d] (?????au) <%2d> %-20s\r\n",
					data.vnum, MAX(-1, data.timer - timer_dec), "БЕЗ ПРОТОТИПА");
		}

		if (strlen(buf) > kMaxStringLength - 80) {
			strcat(buf, "** Excessive rent listing. **\r\n");
			break;
		}
	}

	send_to_char(buf, ch);
	sprintf(buf, "Время в ренте: %ld тиков.\r\n", timer_dec);
	send_to_char(buf, ch);
	sprintf(buf, "Предметов: %d. Стоимость: (%d в день) * (%1.2f дней) = %d. ИНГРИДИЕНТЫ НЕ ВЫВОДЯТСЯ.\r\n",
			SAVEINFO(index)->rent.nitems,
			SAVEINFO(index)->rent.net_cost_per_diem, num_of_days,
			(int) (num_of_days * SAVEINFO(index)->rent.net_cost_per_diem));
	send_to_char(buf, ch);
}

void Crash_listrent(CharData *ch, char *name) {
	int index;

	index = get_ptable_by_name(name);

	if (index < 0) {
		send_to_char("Нет такого игрока.\r\n", ch);
		return;
	}

	if (!SAVEINFO(index)) {
		if (!Crash_read_timer(index, true)) {
			sprintf(buf, "Ubable to read %s timer file.\r\n", name);
			send_to_char(buf, ch);
		} else if (!SAVEINFO(index)) {
			sprintf(buf, "%s не имеет файла ренты.\r\n", CAP(name));
			send_to_char(buf, ch);
		} else {
			sprintf(buf, "%s находится в игре. Содержимое файла ренты:\r\n", CAP(name));
			send_to_char(buf, ch);
			Crash_list_objects(ch, index);
			clear_saveinfo(index);
		}
	} else {
		sprintf(buf, "%s находится в ренте. Содержимое файла ренты:\r\n", CAP(name));
		send_to_char(buf, ch);
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
	bool need_convert_character_objects = 0;    // add by Pereplut

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
			send_to_char("\r\n** Неизвестный код ренты **\r\n"
						 "Проблемы с восстановлением ваших вещей из файла.\r\n"
						 "Обращайтесь за помощью к Богам.\r\n", ch);
			Crash_clear_objects(index);
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
		if (!IS_IMMORTAL(ch) && IsAbleToUseFeat(ch, EFeat::kEmployer) && ch->player_specials->saved.HiredCost != 0) {
			if (ch->player_specials->saved.HiredCost < 0)
				ch->add_bank(abs(ch->player_specials->saved.HiredCost), false);
			else
				ch->add_gold(ch->player_specials->saved.HiredCost, false);
		}
		ch->player_specials->saved.HiredCost = 0;
	}
	// end by WorM
	// Бесплатная рента, если выйти в течение 2 часов после ребута или креша
	if (((RENTCODE(index) == RENT_CRASH || RENTCODE(index) == RENT_FORCED)
		&& SAVEINFO(index)->rent.time + free_crashrent_period * kSecsPerRealHour > time(0)) || free_rent) {
		sprintf(buf, "%s** На сей раз постой был бесплатным **%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		sprintf(buf, "%s entering game, free crashrent.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
	} else if (cost > ch->get_gold() + ch->get_bank()) {
		sprintf(buf, "%sВы находились на постое %1.2f дней.\n\r"
					 "%s"
					 "Вам предъявили счет на %d %s за постой (%d %s в день).\r\n"
					 "Но все, что у вас было - %ld %s... Увы. Все ваши вещи переданы мобам.%s\n\r",
				CCWHT(ch, C_NRM),
				num_of_days,
				RENTCODE(index) ==
					RENT_TIMEDOUT ?
				"Вас пришлось тащить до кровати, за это постой был дороже.\r\n"
								  : "", cost, GetDeclensionInNumber(cost, EWhat::kMoneyU),
				SAVEINFO(index)->rent.net_cost_per_diem,
				GetDeclensionInNumber(SAVEINFO(index)->rent.net_cost_per_diem,
									  EWhat::kMoneyA), ch->get_gold() + ch->get_bank(),
				GetDeclensionInNumber(ch->get_gold() + ch->get_bank(), EWhat::kMoneyA), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		sprintf(buf, "%s: rented equipment lost (no $).", GET_NAME(ch));
		mudlog(buf, LGH, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		ch->set_bank(0);
		ch->set_gold(0);
		Crash_clear_objects(index);
		return (2);
	} else {
		if (cost) {
			sprintf(buf, "%sВы находились на постое %1.2f дней.\n\r"
						 "%s"
						 "С вас содрали %d %s за постой (%d %s в день).%s\r\n",
					CCWHT(ch, C_NRM),
					num_of_days,
					RENTCODE(index) ==
						RENT_TIMEDOUT ?
					"Вас пришлось тащить до кровати, за это постой был дороже.\r\n"
									  : "", cost, GetDeclensionInNumber(cost, EWhat::kMoneyU),
					SAVEINFO(index)->rent.net_cost_per_diem,
					GetDeclensionInNumber(SAVEINFO(index)->rent.net_cost_per_diem, EWhat::kMoneyA), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		ch->remove_both_gold(cost);
	}

	//Чтение описаний объектов в буфер
	if (!get_filename(GET_NAME(ch), fname, kTextCrashFile) || !(fl = fopen(fname, "r+b"))) {
		send_to_char("\r\n** Нет файла описания вещей **\r\n"
					 "Проблемы с восстановлением ваших вещей из файла.\r\n"
					 "Обращайтесь за помощью к Богам.\r\n", ch);
		Crash_clear_objects(index);
		return (1);
	}
	fseek(fl, 0L, SEEK_END);
	fsize = ftell(fl);
	if (!fsize) {
		fclose(fl);
		send_to_char("\r\n** Файл описания вещей пустой **\r\n"
					 "Проблемы с восстановлением ваших вещей из файла.\r\n"
					 "Обращайтесь за помощью к Богам.\r\n", ch);
		Crash_clear_objects(index);
		return (1);
	}

	CREATE(readdata, fsize + 1);
	fseek(fl, 0L, SEEK_SET);
	if (!fread(readdata, fsize, 1, fl) || ferror(fl) || !readdata) {
		fclose(fl);
		FileCRC::check_crc(fname, FileCRC::TEXTOBJS, GET_UNIQUE(ch));
		send_to_char("\r\n** Ошибка чтения файла описания вещей **\r\n"
					 "Проблемы с восстановлением ваших вещей из файла.\r\n"
					 "Обращайтесь за помощью к Богам.\r\n", ch);
		log("Memory error or cann't read %s(%d)...", fname, fsize);
		free(readdata);
		Crash_clear_objects(index);
		return (1);
	};
	fclose(fl);
	FileCRC::check_crc(fname, FileCRC::TEXTOBJS, GET_UNIQUE(ch));

	data = readdata;
	*(data + fsize) = '\0';

	// Проверка в каком формате записана информация о персонаже.
	if (!strn_cmp(readdata, "@", 1))
		need_convert_character_objects = 1;

	//Создание объектов
	long timer_dec = time(0) - SAVEINFO(index)->rent.time;
	timer_dec = (timer_dec / kSecsPerMudHour) + (timer_dec % kSecsPerMudHour ? 1 : 0);

	for (fsize = 0, reccount = SAVEINFO(index)->rent.nitems;
		 reccount > 0 && *data && *data != END_CHAR; reccount--, fsize++) {
		i++;
		ObjData::shared_ptr obj;
		if (need_convert_character_objects) {
			// Формат новый => используем новую функцию
			obj = read_one_object_new(&data, &error);
			if (!obj) {
				//send_to_char ("Ошибка при чтении - чтение предметов прервано.\r\n", ch);
				send_to_char("Ошибка при чтении файла объектов.\r\n", ch);
				sprintf(buf, "SYSERR: Objects reading fail for %s error %d, stop reading.", GET_NAME(ch), error);
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);

				continue;    //Ann
			}
		} else {
			// Формат старый => используем старую функцию
			obj = read_one_object(&data, &error);
			if (!obj) {
				//send_to_char ("Ошибка при чтении - чтение предметов прервано.\r\n", ch);
				send_to_char("Ошибка при чтении файла объектов.\r\n", ch);
				sprintf(buf, "SYSERR: Objects reading fail for %s error %d, stop reading.",
						GET_NAME(ch), error);
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
				//break;
				continue;    //Ann
			}
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

		if (obj->get_vnum() != SAVEINFO(index)->time[fsize].vnum) {
			send_to_char("Нет соответствия заголовков - чтение предметов прервано.\r\n", ch);
			sprintf(buf, "SYSERR: Objects reading fail for %s (2), stop reading.", GET_NAME(ch));
			mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
			ExtractObjFromWorld(obj.get());
			break;
		}

		//Check timers
		if (SAVEINFO(index)->time[fsize].timer > 0
			&& (rnum = real_object(SAVEINFO(index)->time[fsize].vnum)) >= 0) {
			obj_proto.dec_stored(rnum);
		}
		// в два действия, чтобы заодно снять и таймер обкаста
		if (!check_unlimited_timer(obj.get())) {
			const SaveInfo *si = SAVEINFO(index);
			obj->set_timer(si->time[fsize].timer);
			obj->dec_timer(timer_dec);
		}

		std::string cap = obj->get_PName(0);
		cap[0] = UPPER(cap[0]);

		// Предмет разваливается от старости
		if (obj->get_timer() <= 0) {
			snprintf(buf, kMaxStringLength, "%s%s%s рассыпал%s от длительного использования.\r\n",
					 CCWHT(ch, C_NRM),
					 cap.c_str(),
					 char_get_custom_label(obj.get(), ch).c_str(),
					 GET_OBJ_SUF_2(obj));
			send_to_char(buf, ch);
			ExtractObjFromWorld(obj.get());

			continue;
		}

		//очищаем ZoneDecay объедки
		if (obj->has_flag(EObjFlag::kZonedacay)) {
			sprintf(buf, "%s рассыпал%s в прах.\r\n", cap.c_str(), GET_OBJ_SUF_2(obj));
			send_to_char(buf, ch);
			ExtractObjFromWorld(obj.get());
			continue;
		}
		//очищаем RepopDecay объедки
		if (obj->has_flag(EObjFlag::kRepopDecay)) {
			sprintf(buf, "%s рассыпал%s в прах.\r\n", cap.c_str(), GET_OBJ_SUF_2(obj));
			send_to_char(buf, ch);
			ExtractObjFromWorld(obj.get());
			continue;
		}

		// Check valid class
		if (invalid_anti_class(ch, obj.get())
			|| invalid_unique(ch, obj.get())
			|| NamedStuff::check_named(ch, obj.get(), 0)) {
			sprintf(buf, "%s рассыпал%s, как запрещенн%s для вас.\r\n",
					cap.c_str(), GET_OBJ_SUF_2(obj), GET_OBJ_SUF_3(obj));
			send_to_char(buf, ch);
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
				&& GET_OBJ_TYPE(obj) == EObjType::kContainer)    // This is container and it is not free
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
			log("%s load_char_obj %d %d %u", GET_NAME(ch), GET_OBJ_VNUM(obj), obj->get_uid(), obj->get_timer());
		} else {
			if (obj2
				&& obj2->get_worn_on() < obj->get_worn_on()
				&& GET_OBJ_TYPE(obj) == EObjType::kContainer)    // This is container and it is not free
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
			log("%s load_char_obj %d %d %u", GET_NAME(ch), GET_OBJ_VNUM(obj), obj->get_uid(), obj->get_timer());
		}
	}

	while (tank_list)    //Clear tanks list
	{
		tank = tank_list;
		tank_list = tank->next;
		free(tank);
	}

	affect_total(ch);
	clear_saveinfo(index);
	//???
	//Crash_crashsave();
	return 0;
}

// ********** Some util functions for objects save... **********

void Crash_restore_weight(ObjData *obj) {
	for (; obj; obj = obj->get_next_content()) {
		Crash_restore_weight(obj->get_contains());
		if (obj->get_in_obj()) {
			obj->get_in_obj()->add_weight(GET_OBJ_WEIGHT(obj));
		}
	}
}

void Crash_extract_objs(ObjData *obj) {
	ObjData *next;
	for (; obj; obj = next) {
		next = obj->get_next_content();
		Crash_extract_objs(obj->get_contains());
		if (GET_OBJ_RNUM(obj) >= 0 && obj->get_timer() >= 0) {
			obj_proto.inc_stored(GET_OBJ_RNUM(obj));
		}
		ExtractObjFromWorld(obj);
	}
}

int Crash_is_unrentable(CharData *ch, ObjData *obj) {
	if (!obj) {
		return false;
	}

	if (obj->has_flag(EObjFlag::kNorent)
		|| GET_OBJ_RENT(obj) < 0
		|| obj->has_flag(EObjFlag::kRepopDecay)
		|| obj->has_flag(EObjFlag::kZonedacay)
		|| (GET_OBJ_RNUM(obj) <= kNothing
			&& GET_OBJ_TYPE(obj) != EObjType::kMoney)
		|| GET_OBJ_TYPE(obj) == EObjType::kKey
		|| SetSystem::is_norent_set(ch, obj)) {
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
	if (ch->followers) {
		for (struct Follower *k = ch->followers; k; k = k->next) {
			if (!IS_CHARMICE(k->ch)
				|| !k->ch->has_master()) {
				continue;
			}
			for (int j = 0; j < EEquipPos::kNumEquipPos; ++j) {
				if (!GET_EQ(k->ch, j)) {
					continue;
				}

				if (Crash_is_unrentable(k->ch, GET_EQ(k->ch, j))) {
					PlaceObjToInventory(UnequipChar(k->ch, j, CharEquipFlags()), k->ch);
				} else {
					Crash_extract_norents(k->ch, GET_EQ(k->ch, j));
				}
			}
			Crash_extract_norents(k->ch, k->ch->carrying);
		}
	}
}

int Crash_calculate_rent(ObjData *obj) {
	int cost = 0;
	for (; obj; obj = obj->get_next_content()) {
		cost += Crash_calculate_rent(obj->get_contains());
		cost += MAX(0, GET_OBJ_RENT(obj));
	}
	return (cost);
}

int Crash_calculate_rent_eq(ObjData *obj) {
	int cost = 0;
	for (; obj; obj = obj->get_next_content()) {
		cost += Crash_calculate_rent(obj->get_contains());
		cost += MAX(0, GET_OBJ_RENTEQ(obj));
	}
	return (cost);
}

int Crash_calculate_charmee_rent(CharData *ch) {
	int cost = 0;
	if (ch->followers) {
		for (struct Follower *k = ch->followers; k; k = k->next) {
			if (!IS_CHARMICE(k->ch)
				|| !k->ch->has_master()) {
				continue;
			}

			cost = Crash_calculate_rent(k->ch->carrying);
			for (int j = 0; j < EEquipPos::kNumEquipPos; ++j) {
				cost += Crash_calculate_rent_eq(GET_EQ(k->ch, j));
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
	if (ch->followers) {
		for (struct Follower *k = ch->followers; k; k = k->next) {
			if (!IS_CHARMICE(k->ch)
				|| !k->ch->has_master())
				continue;
			for (int j = 0; j < EEquipPos::kNumEquipPos; j++)
				num += Crash_calcitems(GET_EQ(k->ch, j));
			num += Crash_calcitems(k->ch->carrying);
		}
	}
	return num;
}

void Crash_save(std::stringstream &write_buffer, int iplayer, ObjData *obj, int location, int savetype) {
	for (; obj; obj = obj->get_next_content()) {
		if (obj->get_in_obj()) {
			obj->get_in_obj()->sub_weight(GET_OBJ_WEIGHT(obj));
		}
		Crash_save(write_buffer, iplayer, obj->get_contains(), MIN(0, location) - 1, savetype);
		if (iplayer >= 0) {
			write_one_object(write_buffer, obj, location);
			SaveTimeInfo tmp_node;
			tmp_node.vnum = GET_OBJ_VNUM(obj);
			tmp_node.timer = obj->get_timer();
			SAVEINFO(iplayer)->time.push_back(tmp_node);

			if (savetype != RENT_CRASH) {
				log("%s save_char_obj %d %d %u", player_table[iplayer].name(),
					GET_OBJ_VNUM(obj), obj->get_uid(), obj->get_timer());
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

	if (ch->is_npc())
		return false;

	if ((iplayer = GET_INDEX(ch)) < 0) {
		sprintf(buf, "[SYSERR] Store file '%s' - INVALID Id %d", GET_NAME(ch), iplayer);
		mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
		return false;
	}

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
		ch->remove_gold(cost);
	}

	if (savetype == RENT_RENTED) {
		rent.net_cost_per_diem = rentcost;
	} else {
		rent.net_cost_per_diem = cost;
	}

	rent.rentcode = savetype;
	rent.time = time(0);
	rent.nitems = num;
	rent.gold = ch->get_gold();
	rent.account = ch->get_bank();

	Crash_create_timer(iplayer, num);
	SAVEINFO(iplayer)->rent = rent;

	std::stringstream write_buffer;
	write_buffer << "@ Items file\n";

	for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
		if (GET_EQ(ch, j)) {
			crash_save_and_restore_weight(write_buffer, iplayer, GET_EQ(ch, j), j + 1, savetype);
		}
	}

	crash_save_and_restore_weight(write_buffer, iplayer, ch->carrying, 0, savetype);

	if (ch->followers
		&& (savetype == RENT_CRASH
			|| savetype == RENT_FORCED)) {
		for (struct Follower *k = ch->followers; k; k = k->next) {
			if (!IS_CHARMICE(k->ch)
				|| !k->ch->has_master()) {
				continue;
			}

			for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
				if (GET_EQ(k->ch, j)) {
					crash_save_and_restore_weight(write_buffer, iplayer, GET_EQ(k->ch, j), 0, savetype);
				}
			}

			crash_save_and_restore_weight(write_buffer, iplayer, k->ch->carrying, 0, savetype);
		}
	}

	// в принципе экстрактить здесь чармисовый шмот в случае ребута - смысла ноль
	if (savetype != RENT_CRASH) {
		for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
			if (GET_EQ(ch, j)) {
				Crash_extract_objs(GET_EQ(ch, j));
			}
		}
		Crash_extract_objs(ch->carrying);
	}

	if (get_filename(GET_NAME(ch), fname, kTextCrashFile)) {
		std::ofstream file(fname);
		if (!file.is_open()) {
			snprintf(buf, kMaxStringLength, "[SYSERR] Store objects file '%s'- MAY BE LOCKED.", fname);
			mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
			Crash_delete_files(iplayer);
			return false;
		}
		write_buffer << "\n$\n$\n";
		file << write_buffer.rdbuf();
		file.close();
		FileCRC::check_crc(fname, FileCRC::UPDATE_TEXTOBJS, GET_UNIQUE(ch));
	} else {
		Crash_delete_files(iplayer);
		return false;
	}

	if (!Crash_write_timer(iplayer)) {
		Crash_delete_files(iplayer);
		return false;
	}

	if (savetype == RENT_CRASH) {
		clear_saveinfo(iplayer);
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
		send_to_char("Ты сможешь жить у меня до второго пришествия.\r\n", ch);
		return;
	}

	act("$n сказал$g вам :\r\n", false, recep, 0, ch, kToVict);

	long depot_cost = static_cast<long>(Depot::get_total_cost_per_day(ch));
	if (depot_cost) {
		send_to_char(ch, "\"За вещи в хранилище придется доплатить %ld %s.\"\r\n",
					 depot_cost, GetDeclensionInNumber(depot_cost, EWhat::kMoneyU));
		cost += depot_cost;
	}

	send_to_char(ch, "\"Постой обойдется тебе в %ld %s.\"\r\n", cost, GetDeclensionInNumber(cost, EWhat::kMoneyU));
	rent_deadline = ((ch->get_gold() + ch->get_bank()) / cost);
	send_to_char(ch, "\"Твоих денег хватит на %ld %s.\"\r\n", rent_deadline,
				 GetDeclensionInNumber(rent_deadline, EWhat::kDay));
}

int Crash_report_unrentables(CharData *ch, CharData *recep, ObjData *obj) {
	char buf[128];
	int has_norents = 0;

	if (obj) {
		if (Crash_is_unrentable(ch, obj)) {
			has_norents = 1;
			if (SetSystem::is_norent_set(ch, obj)) {
				snprintf(buf, sizeof(buf),
						 "$n сказал$g вам : \"Я не приму на постой %s - требуется две и более вещи из набора.\"",
						 OBJN(obj, ch, 3));
			} else {
				snprintf(buf, sizeof(buf),
						 "$n сказал$g вам : \"Я не приму на постой %s.\"", OBJN(obj, ch, 3));
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
	char bf[80], bf2[12];

	if (obj) {
		if (CAN_WEAR_ANY(obj)) {
			if (equip) {
				sprintf(bf, " (%d если снять)", GET_OBJ_RENT(obj) * factor * count);
			} else {
				sprintf(bf, " (%d если надеть)", GET_OBJ_RENTEQ(obj) * factor * count);
			}
		} else {
			*bf = '\0';
		}

		if (count > 1) {
			sprintf(bf2, " [%d]", count);
		}

		sprintf(buf, "%s - %d %s%s за %s%s %s",
				recursive ? "" : CCWHT(ch, C_SPR),
				(equip ? GET_OBJ_RENTEQ(obj) * count : GET_OBJ_RENT(obj)) *
					factor * count,
				GetDeclensionInNumber((equip ? GET_OBJ_RENTEQ(obj) * count : GET_OBJ_RENT(obj)) * factor * count,
									  EWhat::kMoneyA),
				bf, OBJN(obj, ch, 3),
				count > 1 ? bf2 : "",
				recursive ? "" : CCNRM(ch, C_SPR));
		act(buf, false, recep, 0, ch, kToVict);
	}
}
// end by WorM

void Crash_report_rent(CharData *ch, CharData *recep, ObjData *obj, int *cost,
					   long *nitems, int rentshow, int factor, int equip, int recursive) {
	ObjData *i, *push = nullptr;
	int push_count = 0;

	if (obj) {
		if (!Crash_is_unrentable(ch, obj)) {
			/*(*nitems)++;
			*cost += MAX(0, ((equip ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj)) * factor));
			if (rentshow)
			{
				if (*nitems == 1)
				{
					if (!recursive)
						act("$n сказал$g Вам : \"Одет$W спать будешь? Хм.. Ну смотри, тогда дешевле возьму\"", false, recep, 0, ch, TO_VICT);
					else
						act("$n сказал$g Вам : \"Это я в чулане запру:\"", false, recep, 0, ch, TO_VICT);
				}
				if (CAN_WEAR_ANY(obj))
				{
					if (equip)
						sprintf(bf, " (%d если снять)", GET_OBJ_RENT(obj) * factor);
					else
						sprintf(bf, " (%d если надеть)", GET_OBJ_RENTEQ(obj) * factor);
				}
				else
					*bf = '\0';
				sprintf(buf, "%s - %d %s%s за %s %s",
						recursive ? "" : CCWHT(ch, C_SPR),
						(equip ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj)) *
						factor,
						desc_count((equip ? GET_OBJ_RENTEQ(obj) :
									GET_OBJ_RENT(obj)) * factor,
								   EWhat::MONEYa), bf, OBJN(obj, ch, 3),
						recursive ? "" : CCNRM(ch, C_SPR));
				act(buf, false, recep, 0, ch, TO_VICT);
			}*/
			// added by WorM (Видолюб)
			//группирует вывод при ренте на подобии:
			// - 2700 кун (900 если надеть) за красный пузырек [9]
			for (i = obj; i; i = i->get_next_content()) {
				(*nitems)++;
				*cost += MAX(0, ((equip ? GET_OBJ_RENTEQ(i) : GET_OBJ_RENT(i)) * factor));
				if (rentshow) {
					if (*nitems == 1) {
						if (!recursive) {
							act("$n сказал$g вам : \"Одет$W спать будешь? Хм.. Ну смотри, тогда дешевле возьму\"",
								false,
								recep,
								0,
								ch,
								kToVict);
						} else {
							act("$n сказал$g вам : \"Это я в чулане запру:\"", false, recep, 0, ch, kToVict);
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
											  nitems,
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
					Crash_report_rent(ch, recep, push->get_contains(), cost, nitems, rentshow, factor, false, true);
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
		act("$n сказал$g вам : \"Но у тебя ведь ничего нет! Просто набери \"конец\"!\"",
			false,
			receptionist,
			0,
			ch,
			kToVict);
		return (false);
	}

	if (numitems > MAX_SAVED_ITEMS) {
		sprintf(buf,
				"$n сказал$g вам : \"Извините, но я не могу хранить больше %d предметов.\"\r\n"
				"$n сказал$g вам : \"В данный момент их у вас %ld.\"", MAX_SAVED_ITEMS, numitems);
		act(buf, false, receptionist, 0, ch, kToVict);
		return (false);
	}

	int divide = 1;
	if (min_rent_cost(ch) <= 0 && *totalcost <= 1000) {
		divide = 2;
	}

	if (rentshow) {
		if (min_rent_cost(ch) > 0) {
			sprintf(buf,
					"$n сказал$g вам : \"И еще %d %s мне на сбитень с медовухой :)\"",
					min_rent_cost(ch) * factor, GetDeclensionInNumber(min_rent_cost(ch) * factor, EWhat::kMoneyU));
			act(buf, false, receptionist, 0, ch, kToVict);
		}

		sprintf(buf, "$n сказал$g вам : \"В сумме это составит %d %s %s.\"",
				*totalcost, GetDeclensionInNumber(*totalcost, EWhat::kMoneyU),
				(factor == RENT_FACTOR ? "в день " : ""));
		act(buf, false, receptionist, 0, ch, kToVict);

		if (MAX(0, *totalcost / divide) > ch->get_gold() + ch->get_bank()) {
			act("\"...которых у тебя отродясь не было.\"", false, receptionist, 0, ch, kToVict);
			return (false);
		}

		*totalcost = MAX(0, *totalcost / divide);
		if (divide == 2) {
			act("$n сказал$g вам : \"Так уж и быть, я скощу тебе половину.\"",
				false, receptionist, 0, ch, kToVict);
		}

		if (factor == RENT_FACTOR) {
			Crash_rent_deadline(ch, receptionist, *totalcost);
		}
	} else {
		*totalcost = MAX(0, *totalcost / divide);
	}
	return (true);
}

int gen_receptionist(CharData *ch, CharData *recep, int cmd, char * /*arg*/, int mode) {
	RoomRnum save_room;
	int cost, rentshow = true;

	if (!ch->desc || ch->is_npc())
		return (false);

	if (!cmd && !number(0, 5))
		return (false);

	if (!CMD_IS("offer") && !CMD_IS("предложение")
		&& !CMD_IS("rent") && !CMD_IS("постой")
		&& !CMD_IS("quit") && !CMD_IS("конец")
		&& !CMD_IS("settle") && !CMD_IS("поселиться"))
		return (false);

	save_room = ch->in_room;

	if (CMD_IS("конец") || CMD_IS("quit")) {
		if (save_room != r_helled_start_room &&
			save_room != r_named_start_room && save_room != r_unreg_start_room)
			GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
		return (false);
	}

	if (!AWAKE(recep)) {
		sprintf(buf, "%s не в состоянии говорить с вами...\r\n", HSSH(recep));
		send_to_char(buf, ch);
		return (true);
	}
	if (!CAN_SEE(recep, ch)) {
		act("$n сказал$g : \"Не люблю говорить с теми, кого я не вижу!\"", false, recep, 0, 0, kToRoom);
		return (true);
	}
	if (Clan::InEnemyZone(ch)) {
		act("$n сказал$g : \"Чужакам здесь не место!\"", false, recep, 0, 0, kToRoom);
		return (true);
	}
	if (NORENTABLE(ch)) {
		send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
		return (true);
	}
	if (ch->get_fighting()) {
		return (false);
	}
	if (free_rent) {
		act("$n сказал$g вам : \"Сегодня спим нахаляву!\"", false, recep, 0, ch, kToVict);
		rentshow = false;
	}
	if (CMD_IS("rent") || CMD_IS("постой")) {

		if (!Crash_offer_rent(ch, recep, rentshow, mode, &cost))
			return (true);

		if (rentshow) {
			if (mode == RENT_FACTOR)
				sprintf(buf,
						"$n сказал$g вам : \"Дневной постой обойдется тебе в! %d %s.\"",
						cost, GetDeclensionInNumber(cost, EWhat::kMoneyU));
			else if (mode == CRYO_FACTOR)
				sprintf(buf,
						"$n сказал$g вам : \"Дневной постой обойдется тебе в %d %s (за пользование холодильником :)\"",
						cost, GetDeclensionInNumber(cost, EWhat::kMoneyU));
			act(buf, false, recep, 0, ch, kToVict);

			if (cost > ch->get_gold() + ch->get_bank()) {
				act("$n сказал$g вам : '..но такой голытьбе, как ты, это не по карману.'",
					false, recep, 0, ch, kToVict);
				return (true);
			}
			if (cost && (mode == RENT_FACTOR))
				Crash_rent_deadline(ch, recep, cost);
		}
		if (free_rent) {
			cost = 0;
		}
		if (mode == RENT_FACTOR) {
			act("$n запер$q ваши вещи в сундук и повел$g в тесную каморку.", false, recep, 0, ch, kToVict);
			Crash_rentsave(ch, cost);
			sprintf(buf, "%s has rented (%d/day, %ld tot.)",
					GET_NAME(ch), cost, ch->get_gold() + ch->get_bank());
		} else    // cryo
		{
			act("$n запер$q ваши вещи в сундук и повел$g в тесную каморку.\r\n"
				"Белый призрак появился в комнате, обдав вас холодом...\r\n"
				"Вы потеряли связь с окружающими вас...", false, recep, 0, ch, kToVict);
			Crash_cryosave(ch, cost);
			sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
			PLR_FLAGS(ch).set(EPlrFlag::kCryo);
		}

		mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);

		if ((save_room == r_helled_start_room)
			|| (save_room == r_named_start_room)
			|| (save_room == r_unreg_start_room))
			act("$n проводил$g $N3 мощным пинком на свободную лавку.", false, recep, 0, ch, kToRoom);
		else {
			act("$n помог$q $N2 отойти ко сну.", false, recep, 0, ch, kToNotVict);
			GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
		}
		Clan::clan_invoice(ch, false);
		ch->save_char();
		ExtractCharFromWorld(ch, false);
	} else if (CMD_IS("offer") || CMD_IS("предложение")) {
		Crash_offer_rent(ch, recep, rentshow, mode, &cost);
		act("$N предложил$G $n2 остановиться у н$S.", false, ch, 0, recep, kToRoom);
	} else {
		if ((save_room == r_helled_start_room)
			|| (save_room == r_named_start_room)
			|| (save_room == r_unreg_start_room))
			act("$N сказал$G : \"Куда же ты денешься от меня?\"", false, ch, 0, recep, kToChar);
		else {
			act("$n предложил$g $N2 поселиться у н$s.", false, recep, 0, ch, kToNotVict);
			act("$N сказал$G вам : \"Примем в любое время и почти в любом состоянии!\"",
				false,
				ch,
				0,
				recep,
				kToChar);
			sprintf(buf,
					"%s has changed loadroom from %d to %d.",
					GET_NAME(ch),
					GET_LOADROOM(ch),
					GET_ROOM_VNUM(save_room));
			GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
			mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
			WAIT_STATE(ch, 1 * kPulseViolence);
			ch->save_char();
		}
	}
	return (true);
}

int receptionist(CharData *ch, void *me, int cmd, char *argument) {
	return (gen_receptionist(ch, (CharData *) me, cmd, argument, RENT_FACTOR));
}

int cryogenicist(CharData *ch, void *me, int cmd, char *argument) {
	return (gen_receptionist(ch, (CharData *) me, cmd, argument, CRYO_FACTOR));
}

void Crash_frac_save_all(int frac_part) {
	DescriptorData *d;

	for (d = descriptor_list; d; d = d->next) {
		if ((STATE(d) == CON_PLAYING) && !d->character->is_npc() && GET_ACTIVITY(d->character) == frac_part) {
			Crash_crashsave(d->character.get());
			d->character->save_char();
			PLR_FLAGS(d->character).unset(EPlrFlag::kCrashSave);
		}
	}
}

void Crash_save_all(void) {
	DescriptorData *d;
	for (d = descriptor_list; d; d = d->next) {
		if ((STATE(d) == CON_PLAYING) && PLR_FLAGGED(d->character, EPlrFlag::kCrashSave)) {
			Crash_crashsave(d->character.get());
			d->character->save_char();
			PLR_FLAGS(d->character).unset(EPlrFlag::kCrashSave);
		}
	}
}

// * Сейв при плановом ребуте/остановке с таймером != 0.
void Crash_save_all_rent(void) {
	// shapirus: проходим не по списку дескрипторов,
	// а по списку чаров, чтобы сохранить заодно и тех,
	// кто перед ребутом ушел в ЛД с целью сохранить
	// свои грязные денежки.

	character_list.foreach_on_copy([&](const auto &ch) {
		if (!ch->is_npc()) {
			save_char_objects(ch.get(), RENT_FORCED, 0);
			log("Saving char: %s", GET_NAME(ch));
			PLR_FLAGS(ch).unset(EPlrFlag::kCrashSave);
			//AFF_FLAGS(ch.get()).unset(EAffectFlag::AFF_GROUP);
			(ch.get())->removeGroupFlags();
			AFF_FLAGS(ch.get()).unset(EAffect::kHorse);
			ExtractCharFromWorld(ch.get(), false);
		}
	});
}

void Crash_frac_rent_time(int frac_part) {
	for (std::size_t c = 0; c < player_table.size(); c++) {
		if (player_table[c].activity == frac_part
			&& player_table[c].unique != -1
			&& SAVEINFO(c)) {
			Crash_timer_obj(c, time(0));
		}
	}
}

void Crash_rent_time(int/* dectime*/) {
	for (std::size_t c = 0; c < player_table.size(); c++) {
		if (player_table[c].unique != -1) {
			Crash_timer_obj(c, time(0));
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
