/************************************************************************
 * OasisOLC - oedit.c						v1.5	*
 * Copyright 1996 Harvey Gilpin.					*
 * Original author: Levork						*
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
 ************************************************************************/

#include "engine/db/world_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"
#include "engine/core/comm.h"
#include "gameplay/magic/spells.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/id_converter.h"
#include "engine/db/db.h"
#include "olc.h"
#include "engine/scripting/dg_olc.h"
#include "gameplay/crafting/im.h"
#include "gameplay/mechanics/depot.h"
#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"
#include "gameplay/skills/skills.h"
#include "gameplay/communication/parcel.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/economics/shop_ext.h"
#include "gameplay/core/constants.h"
#include "gameplay/mechanics/sets_drop.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/zone.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/magic/spells_info.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/dungeons.h"
#include <sys/stat.h>

#include <array>
#include <vector>
#include <stack>

// * External variable declarations.
/*extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *drinks[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *anti_bits[];
extern const char *no_bits[];
extern const char *weapon_affects[];
extern const char *material_name[];
extern const char *ingradient_bits[];
extern const char *magic_container_bits[];*/
extern DescriptorData *descriptor_list;
extern int top_imrecipes;
extern void ExtractObjFromWorld(ObjData *obj);

void oedit_setup(DescriptorData *d, int real_num);
void oedit_object_copy(ObjData *dst, CObjectPrototype *src);
void oedit_save_internally(DescriptorData *d);
void oedit_save_to_disk(int zone);
void oedit_parse(DescriptorData *d, char *arg);
void oedit_disp_spells_menu(DescriptorData *d);
void oedit_liquid_type(DescriptorData *d);
void oedit_disp_container_flags_menu(DescriptorData *d);
void oedit_disp_extradesc_menu(DescriptorData *d);
void oedit_disp_weapon_menu(DescriptorData *d);
void oedit_disp_val1_menu(DescriptorData *d);
void oedit_disp_val2_menu(DescriptorData *d);
void oedit_disp_val3_menu(DescriptorData *d);
void oedit_disp_val4_menu(DescriptorData *d);
void oedit_disp_type_menu(DescriptorData *d);
void oedit_disp_extra_menu(DescriptorData *d);
void oedit_disp_wear_menu(DescriptorData *d);
void oedit_disp_menu(DescriptorData *d);
void oedit_disp_skills_menu(DescriptorData *d);
void oedit_disp_receipts_menu(DescriptorData *d);
void oedit_disp_feats_menu(DescriptorData *d);
void oedit_disp_skills_mod_menu(DescriptorData *d);

// ------------------------------------------------------------------------
//  Utility and exported functions
// ------------------------------------------------------------------------
//***********************************************************************

void oedit_setup(DescriptorData *d, int real_num)
/*++
   Подготовка данных для редактирования объекта.
      d        - OLC дескриптор
      real_num - RNUM исходного объекта, новый -1
--*/
{
	ObjData *obj;
	const auto vnum = OLC_NUM(d);

	NEWCREATE(obj, vnum);

	if (real_num == -1) {
		obj->set_aliases("новый предмет");
		obj->set_description("что-то новое лежит здесь");
		obj->set_short_description("новый предмет");
		obj->set_PName(ECase::kNom, "это что");
		obj->set_PName(ECase::kGen, "нету чего");
		obj->set_PName(ECase::kDat, "привязать к чему");
		obj->set_PName(ECase::kAcc, "взять что");
		obj->set_PName(ECase::kIns, "вооружиться чем");
		obj->set_PName(ECase::kPre, "говорить о чем");
		obj->set_wear_flags(to_underlying(EWearFlag::kTake));
	} else {
		obj->clone_olc_object_from_prototype(vnum);
		obj->set_rnum(real_num);
		if (obj->get_type() == EObjType::kLiquidContainer) {
//			if (obj->get_val(1) > 0) {
				name_from_drinkcon(obj);
//			}
		}
	}

	OLC_OBJ(d) = obj;
	OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
	oedit_disp_menu(d);
	OLC_VAL(d) = 0;
}

// * Обновление данных у конкретной шмотки
void olc_update_object(int robj_num, ObjData *obj, ObjData *olc_obj) {
	// Итак, нашел объект
	// Внимание! Таймер объекта, его состояние и т.д. обновятся!

	// Удаляю его строки и т.д.
	// прототип скрипта не удалится, т.к. его у экземпляра нету
	// скрипт не удалится, т.к. его не удаляю

	bool fullUpdate = true; //флажок если дальше делать выборочные шаги
	/*if (obj->get_crafter_uid()) //Если шмотка крафченная
		fullUpdate = false;*/

	//если объект изменен кодом
	if (obj->has_flag(EObjFlag::kTransformed))
		fullUpdate = false;

	if (!fullUpdate) {
		//тут можно вставить изменение объекта ограниченное
		//в obj лежит объект, в olc_proto лежит прототип
		return;
	}


	// Сохраняю текущую игровую информацию
	ObjData tmp(*obj);

	// Копируем все поля из отредактированного нами объекта OLC_OBJ
	*obj = *olc_obj;

	//Восстанавливаем падежи если объект поренеймлен
	if (tmp.get_is_rename()) {
		obj->copy_name_from(&tmp);
		obj->set_is_rename(true);
	}
	// меняем на значения из шмоток в текущем мире
	obj->clear_proto_script();
	obj->set_unique_id(tmp.get_unique_id());
	obj->set_id(tmp.get_id()); // аук работает не по рнум а по id объекта, поэтому вернем и его
	obj->set_in_room(tmp.get_in_room());
	obj->set_rnum(robj_num);
	obj->set_owner(tmp.get_owner());
	obj->set_crafter_uid(tmp.get_crafter_uid());
	obj->set_carried_by(tmp.get_carried_by());
	obj->set_worn_by(tmp.get_worn_by());
	obj->set_worn_on(tmp.get_worn_on());
	obj->set_in_obj(tmp.get_in_obj());
	obj->set_contains(tmp.get_contains());
	obj->set_next_content(tmp.get_next_content());
	obj->set_next(tmp.get_next());
	obj->set_script(tmp.get_script());
	// для name_list
	obj->set_serial_num(tmp.get_serial_num());
	obj->set_current_durability((&tmp)->get_current_durability());
//	если таймер шмота в текущем мире меньше чем установленный, восстанавливаем его.
	if (tmp.get_timer() < olc_obj->get_timer()) {
		obj->set_timer(tmp.get_timer());
	}
	// емкостям сохраняем жидкость и кол-во глотков, во избежание жалоб
	if (tmp.get_type() == EObjType::kLiquidContainer
		&& obj->get_type() == EObjType::kLiquidContainer) {
		obj->set_val(1, GET_OBJ_VAL(&tmp, 1)); //кол-во глотков
		if (is_potion(&tmp)) {
			obj->set_val(2, GET_OBJ_VAL(&tmp, 2)); //описание жидкости
		}
		// сохранение в случае перелитых заклов
		// пока там ничего кроме заклов и нет - копируем весь values
		if (tmp.GetPotionValueKey(ObjVal::EValueKey::POTION_PROTO_VNUM) > 0) {
			obj->SetPotionValues(tmp.get_all_values());
		}
	}
	if (tmp.has_flag(EObjFlag::kTicktimer))//если у старого объекта запущен таймер
	{
		obj->set_extra_flag(EObjFlag::kTicktimer);//ставим флаг таймер запущен
	}
	if (tmp.has_flag(EObjFlag::kNamed))//если у старого объекта стоит флаг именной предмет
	{
		obj->set_extra_flag(EObjFlag::kNamed);//ставим флаг именной предмет
	}
	//восстанавливаем метки, если они были
	if (tmp.get_custom_label()) {
		obj->set_custom_label(new custom_label());
		obj->get_custom_label()->text_label = str_dup(tmp.get_custom_label()->text_label);
		obj->get_custom_label()->author = tmp.get_custom_label()->author;
		if (tmp.get_custom_label()->clan_abbrev != nullptr) {
			obj->get_custom_label()->clan_abbrev = str_dup(tmp.get_custom_label()->clan_abbrev);
		}
		obj->get_custom_label()->author_mail = str_dup(tmp.get_custom_label()->author_mail);
	}
	// восстановим силу ингров
	if (tmp.get_type() == EObjType::kMagicIngredient) {
		obj->set_val(0, tmp.get_val(0));
		obj->set_val(1, tmp.get_val(1));
		obj->set_val(2, tmp.get_val(2));
	}
}

// * Обновление полей объектов при изменении их прототипа через олц.
void olc_update_objects(int robj_num, ObjData *olc_obj) {
	world_objects.foreach_with_rnum(robj_num, [&](const ObjData::shared_ptr &object) {
		olc_update_object(robj_num, object.get(), olc_obj);
	});
	Depot::olc_update_from_proto(robj_num, olc_obj);
	Parcel::olc_update_from_proto(robj_num, olc_obj);
}

//------------------------------------------------------------------------

#define ZCMD zone_table[zone].cmd[cmd_no]

void oedit_save_internally(DescriptorData *d) {
	int robj_num;

	robj_num = OLC_OBJ(d)->get_rnum();
	ObjSystem::init_ilvl(OLC_OBJ(d));
	// * Write object to internal tables.
	if (robj_num >= 0) {    /*
		 * We need to run through each and every object currently in the
		 * game to see which ones are pointing to this prototype.
		 * if object is pointing to this prototype, then we need to replace it
		 * with the new one.
		 */
		log("[OEdit] Save object to mem %d", GET_OBJ_VNUM(OLC_OBJ(d)));
		olc_update_objects(robj_num, OLC_OBJ(d));

		// Все существующие в мире объекты обновлены согласно новому прототипу
		// Строки в этих объектах как ссылки на данные прототипа

		// Копирую новый прототип в массив
		// Использовать функцию oedit_object_copy() нельзя,
		// т.к. будут изменены указатели на данные прототипа
		obj_proto.set_rnum(robj_num, OLC_OBJ(d));    // old prototype will be deleted automatically
		// OLC_OBJ(d) удалять не нужно, т.к. он перенесен в массив
		// прототипов
	} else {
		// It's a new object, we must build new tables to contain it.
		log("[OEdit] Save mem an disk new %d(%zd/%zd)", OLC_NUM(d), 1 + obj_proto.size(), sizeof(ObjData));
		const size_t index = obj_proto.add(OLC_OBJ(d), OLC_NUM(d));
		const ZoneRnum zrn = get_zone_rnum_by_obj_vnum(OLC_NUM(d));
		obj_proto.zone(index, zrn);
	}

//	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].vnum, OLC_SAVE_OBJ);
	oedit_save_to_disk(OLC_ZNUM(d));
}

//------------------------------------------------------------------------

void oedit_save_to_disk(ZoneRnum zone_num) {
	int counter, counter2, realcounter;
	FILE *fp;

	if (zone_table[zone_num].vnum >= dungeons::kZoneStartDungeons) {
			sprintf(buf, "Отказ сохранения зоны %d на диск.", zone_table[zone_num].vnum);
			mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
			return;
	}
	sprintf(buf, "%s/%d.new", OBJ_PREFIX, zone_table[zone_num].vnum);
	if (!(fp = fopen(buf, "w+"))) {
		mudlog("SYSERR: OLC: Cannot open objects file!", BRF, kLvlBuilder, SYSLOG, true);
		return;
	}
	// * Start running through all objects in this zone.
	for (counter = zone_table[zone_num].vnum * 100; counter <= zone_table[zone_num].top; counter++) {
		if ((realcounter = GetObjRnum(counter)) >= 0) {
			const auto &obj = obj_proto[realcounter];
			if (!obj->get_action_description().empty()) {
				strcpy(buf1, obj->get_action_description().c_str());
				strip_string(buf1);
			} else {
				*buf1 = '\0';
			}
			*buf2 = '\0';
			obj->get_affect_flags().tascii(FlagData::kPlanesNumber, buf2);
			obj->get_anti_flags().tascii(FlagData::kPlanesNumber, buf2);
			obj->get_no_flags().tascii(FlagData::kPlanesNumber, buf2);
			sprintf(buf2 + strlen(buf2), "\n%d ", obj->get_type());
			obj->get_extra_flags().tascii(FlagData::kPlanesNumber, buf2);
			const auto wear_flags = obj->get_wear_flags();
			tascii(&wear_flags, 1, buf2);
			strcat(buf2, "\n");

			fprintf(fp, "#%d\n"
						"%s~\n"
						"%s~\n"
						"%s~\n"
						"%s~\n"
						"%s~\n"
						"%s~\n"
						"%s~\n"
						"%s~\n"
						"%s~\n"
						"%d %d %d %d\n"
						"%d %d %d %d\n"
						"%s"
						"%d %d %d %d\n"
						"%d %d %d %d\n",
					obj->get_vnum(),
					!obj->get_aliases().empty() ? obj->get_aliases().c_str() : "undefined",
					!obj->get_PName(ECase::kNom).empty() ? obj->get_PName(ECase::kNom).c_str() : "что-то",
					!obj->get_PName(ECase::kGen).empty() ? obj->get_PName(ECase::kGen).c_str() : "чего-то",
					!obj->get_PName(ECase::kDat).empty() ? obj->get_PName(ECase::kDat).c_str() : "чему-то",
					!obj->get_PName(ECase::kAcc).empty() ? obj->get_PName(ECase::kAcc).c_str() : "что-то",
					!obj->get_PName(ECase::kIns).empty() ? obj->get_PName(ECase::kIns).c_str() : "чем-то",
					!obj->get_PName(ECase::kPre).empty() ? obj->get_PName(ECase::kPre).c_str() : "о чем-то",
					!obj->get_description().empty() ? obj->get_description().c_str() : "undefined",
					buf1,
					obj->get_spec_param(), obj->get_maximum_durability(), obj->get_current_durability(),
					obj->get_material(), to_underlying(GET_OBJ_SEX(obj)),
					obj->get_timer(), to_underlying(obj->get_spell()),
					obj->get_level(), buf2, GET_OBJ_VAL(obj, 0),
					GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
					GET_OBJ_VAL(obj, 3), obj->get_weight(),
					obj->get_cost(), obj->get_rent_off(), obj->get_rent_on());

			script_save_to_disk(fp, obj.get(), OBJ_TRIGGER);

			if (obj->get_max_in_world()) {
				fprintf(fp, "M %d\n", obj->get_max_in_world());
			}

			if (obj->get_minimum_remorts() != 0) {
				fprintf(fp, "R %d\n", obj->get_minimum_remorts());
			}

			// * Do we have extra descriptions?
			if (obj->get_ex_description())    // Yes, save them too.
			{
				for (auto ex_desc = obj->get_ex_description(); ex_desc; ex_desc = ex_desc->next) {
					// * Sanity check to prevent nasty protection faults.
					if (!ex_desc->keyword
						|| !ex_desc->description) {
						mudlog("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!",
							   BRF, kLvlBuilder, SYSLOG, true);
						continue;
					}
					strcpy(buf1, ex_desc->description);
					strip_string(buf1);
					fprintf(fp, "E\n" "%s~\n" "%s~\n", ex_desc->keyword, buf1);
				}
			}
			// * Do we have affects?
			for (counter2 = 0; counter2 < kMaxObjAffect; counter2++) {
				if (obj->get_affected(counter2).location
					&& obj->get_affected(counter2).modifier) {
					fprintf(fp, "A\n%d %d\n",
							obj->get_affected(counter2).location,
							obj->get_affected(counter2).modifier);
				}
			}

			if (obj->has_skills()) {
				CObjectPrototype::skills_t skills;
				obj->get_skills(skills);
				for (const auto &it : skills) {
					fprintf(fp, "S\n%d %d\n", to_underlying(it.first), it.second);
				}
			}

			// ObjVal
			const auto values = obj->get_all_values().print_to_zone();
			if (!values.empty()) {
				fprintf(fp, "%s", values.c_str());
			}
		}
	}

	// * Write the final line, close the file.
	fprintf(fp, "$\n$\n");
	fclose(fp);
	sprintf(buf2, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].vnum);
	// * We're fubar'd if we crash between the two lines below.
	remove(buf2);
	rename(buf, buf2);
	if (chmod(buf2, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0) {
		std::stringstream ss;
		ss << "Error chmod file: " << buf2 << " (" << __FILE__ << " "<< __func__ << "  "<< __LINE__ << ")";
		mudlog(ss.str(), BRF, kLvlGod, SYSLOG, true);
	}

	olc_remove_from_save_list(zone_table[zone_num].vnum, OLC_SAVE_OBJ);
}

// **************************************************************************
// * Menu functions                                                         *
// **************************************************************************

// * For container flags.
void oedit_disp_container_flags_menu(DescriptorData *d) {
	sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, buf1);
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	snprintf(buf, kMaxStringLength,
			 "%s1%s) Закрываем\r\n"
			 "%s2%s) Нельзя взломать\r\n"
			 "%s3%s) Закрыт\r\n"
			 "%s4%s) Заперт\r\n"
			 "Флаги контейнера: %s%s%s\r\n"
			 "Выберите флаг, 0 - выход : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, buf1, nrm);
	SendMsgToChar(buf, d->character.get());
}

// * For extra descriptions.
void oedit_disp_extradesc_menu(DescriptorData *d) {
	auto extra_desc = OLC_DESC(d);
	strcpy(buf1, !extra_desc->next ? "<Not set>\r\n" : "Set.");
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	snprintf(buf, kMaxStringLength,
			 "Меню экстрадескрипторов\r\n"
			 "%s1%s) Ключ: %s%s\r\n"
			 "%s2%s) Описание:\r\n%s%s\r\n"
			 "%s3%s) Следующий дескриптор: %s\r\n"
			 "%s0%s) Выход\r\n"
			 "Ваш выбор : ",
			 grn, nrm, yel, not_null(extra_desc->keyword, "<NONE>"),
			 grn, nrm, yel, not_null(extra_desc->description, "<NONE>"),
			 grn, nrm, buf1, grn, nrm);
	SendMsgToChar(buf, d->character.get());
	OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

// * Ask for *which* apply to edit.
void oedit_disp_prompt_apply_menu(DescriptorData *d) {
	int counter;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < kMaxObjAffect; counter++) {
		if (OLC_OBJ(d)->get_affected(counter).modifier) {
			sprinttype(OLC_OBJ(d)->get_affected(counter).location, apply_types, buf2);
			snprintf(buf, kMaxStringLength, " %s%d%s) %+d to %s", grn, counter + 1, nrm,
					 OLC_OBJ(d)->get_affected(counter).modifier, buf2);
			if (IsNegativeApply(OLC_OBJ(d)->get_affected(counter).location)) {
				strcat(buf, "   &g(в + ухудшает)&n");
			} 
			SendMsgToChar(buf, d->character.get());
			SendMsgToChar("\r\n", d->character.get());
		} else {
			sprintf(buf, " %s%d%s) Ничего.\r\n", grn, counter + 1, nrm);
			SendMsgToChar(buf, d->character.get());
		}
	}
	SendMsgToChar("\r\nВыберите изменяемый аффект (0 - выход) : ", d->character.get());
	OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

// * Ask for liquid type.
void oedit_liquid_type(DescriptorData *d) {
	int counter, columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_LIQ_TYPES; counter++) {
		sprintf(buf, " %s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				drinks[counter], !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите тип жидкости : ", nrm);
	SendMsgToChar(buf, d->character.get());
	OLC_MODE(d) = OEDIT_VALUE_3;
}

void show_apply_olc(DescriptorData *d) {
	int counter, columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < EApply::kNumberApplies; counter++) {
		sprintf(buf, "%s%2d%s) %-40.40s %s", grn, counter, nrm,
				apply_types[counter], !(++columns % 3) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("\r\nЧто добавляем (0 - выход) : ", d->character.get());
}

// * The actual apply to set.
void oedit_disp_apply_menu(DescriptorData *d) {
	show_apply_olc(d);
	OLC_MODE(d) = OEDIT_APPLY;
}

// * Weapon type.
void oedit_disp_weapon_menu(DescriptorData *d) {
	int counter, columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ATTACK_TYPES; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				attack_hit_text[counter].singular, !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("\r\nВыберите тип удара (0 - выход): ", d->character.get());
}

// * Spell type.
void oedit_disp_spells_menu(DescriptorData *d) {
	int columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		if (MUD::Spell(spell_id).IsInvalid()) {
			continue;
		}
		sprintf(buf, "%s%2d%s) %s%-30.30s %s", grn, to_underlying(spell_id), nrm, yel,
				MUD::Spell(spell_id).GetCName(), !(++columns % 4) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите магию (0 - выход) : ", nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_skills2_menu(DescriptorData *d) {
	int columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (auto skill_id = ESkill::kFirst; skill_id <= ESkill::kLast; ++skill_id) {
		if (MUD::Skills().IsInvalid(skill_id)) {
			continue;
		}

		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, to_underlying(skill_id), nrm, yel,
				MUD::Skill(skill_id).GetName(), !(++columns % 3) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите умение (0 - выход) : ", nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_receipts_menu(DescriptorData *d) {
	int columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (int counter = 0; counter <= top_imrecipes; counter++) {
		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				imrecipes[counter].name, !(++columns % 3) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите рецепт : ", nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_feats_menu(DescriptorData *d) {
	int columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (const auto &feat : MUD::Feats()) {
		if (feat.IsInvalid()) {
			continue;
		}

		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, to_underlying(feat.GetId()), nrm, yel,
				feat.GetCName(), !(++columns % 3) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите способность (0 - выход) : ", nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_skills_mod_menu(DescriptorData *d) {
	int columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	int percent;
	for (auto skill_id = ESkill::kFirst; skill_id <= ESkill::kLast; ++skill_id) {
		if (MUD::Skills().IsInvalid(skill_id)) {
			continue;
		}

		percent = OLC_OBJ(d)->get_skill(skill_id);
		if (percent != 0) {
			sprintf(buf1, "%s[%3d]%s", cyn, percent, nrm);
		} else {
			strcpy(buf1, "     ");
		}
		snprintf(buf, kMaxStringLength, "%s%3d%s) %25s%s%s", grn, to_underlying(skill_id), nrm,
				 MUD::Skill(skill_id).GetName(), buf1, !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("\r\nУкажите номер и уровень владения умением (0 - конец) : ", d->character.get());
}

// * Object value #1
void oedit_disp_val1_menu(DescriptorData *d) {
	OLC_MODE(d) = OEDIT_VALUE_1;
	switch (OLC_OBJ(d)->get_type()) {
		case EObjType::kLightSource:
			// * values 0 and 1 are unused.. jump to 2
			oedit_disp_val3_menu(d);
			break;

		case EObjType::kScroll:
		case EObjType::kWand:
		case EObjType::kStaff:
		case EObjType::kPotion: SendMsgToChar("Уровень заклинания : ", d->character.get());
			break;

		case EObjType::kWeapon:
			// * This doesn't seem to be used if I remember right.
			SendMsgToChar("Модификатор попадания : ", d->character.get());
			break;

		case EObjType::kArmor:
		case EObjType::kLightArmor:
		case EObjType::kMediumArmor:
		case EObjType::kHeavyArmor: SendMsgToChar("Изменяет АС на : ", d->character.get());
			break;

		case EObjType::kContainer: SendMsgToChar("Максимально вместимый вес : ", d->character.get());
			break;

		case EObjType::kLiquidContainer:
		case EObjType::kFountain: SendMsgToChar("Количество глотков : ", d->character.get());
			break;

		case EObjType::kFood: SendMsgToChar("На сколько часов насыщает : ", d->character.get());
			break;

		case EObjType::kMoney: SendMsgToChar("Сумма : ", d->character.get());
			break;

		case EObjType::kNote:
			// * This is supposed to be language, but it's unused.
			break;

		case EObjType::kBook:
			sprintf(buf,
					"%s0%s) %sКнига заклинаний\r\n"
					"%s1%s) %sКнига умений\r\n"
					"%s2%s) %sУлучшение умения\r\n"
					"%s3%s) %sКнига рецептов\r\n"
					"%s4%s) %sКнига способностей\r\n"
					"%sВыберите тип книги : ",
					grn,
					nrm,
					yel,
					grn,
					nrm,
					yel,
					grn,
					nrm,
					yel,
					grn,
					nrm,
					yel,
					grn,
					nrm,
					yel,
					nrm);
			SendMsgToChar(buf, d->character.get());
			break;

		case EObjType::kIngredient:
			SendMsgToChar("Первый байт - лаг после применения в сек, 6 бит - уровень : ",
						 d->character.get());
			break;

		case EObjType::kMagicIngredient: oedit_disp_val4_menu(d);
			break;

		case EObjType::kCraftMaterial: SendMsgToChar("Уровень игрока для использования + морт * 2: ", d->character.get());
			break;

		case EObjType::kBandage: SendMsgToChar("Хитов в секунду: ", d->character.get());
			break;

		case EObjType::kEnchant: SendMsgToChar("Изменяет вес: ", d->character.get());
			break;
		case EObjType::kMagicContaner:
		case EObjType::kMagicArrow: oedit_disp_spells_menu(d);
			break;

		default: oedit_disp_menu(d);
	}
}

// * Object value #2
void oedit_disp_val2_menu(DescriptorData *d) {
	OLC_MODE(d) = OEDIT_VALUE_2;
	switch (OLC_OBJ(d)->get_type()) {
		case EObjType::kScroll:
		case EObjType::kPotion: oedit_disp_spells_menu(d);
			break;

		case EObjType::kWand:
		case EObjType::kStaff: SendMsgToChar("Количество зарядов : ", d->character.get());
			break;

		case EObjType::kWeapon: SendMsgToChar("Количество бросков кубика : ", d->character.get());
			break;

		case EObjType::kArmor:
		case EObjType::kLightArmor:
		case EObjType::kMediumArmor:
		case EObjType::kHeavyArmor: SendMsgToChar("Изменяет броню на : ", d->character.get());
			break;

		case EObjType::kFood:
			// * Values 2 and 3 are unused, jump to 4...Odd.
			oedit_disp_val4_menu(d);
			break;

		case EObjType::kMoney:
			sprintf(buf,
					"%s0%s) %sКуны\r\n"
					"%s1%s) %sСлава\r\n"
					"%s2%s) %sГривны\r\n"
					"%s3%s) %sСнежинки\r\n"
					"%sВыберите тип валюты : ",
					grn, nrm, yel,
					grn, nrm, yel,
					grn, nrm, yel,
					grn, nrm, yel,
					nrm);
			SendMsgToChar(buf, d->character.get());
			break;

		case EObjType::kContainer:
			// * These are flags, needs a bit of special handling.
			oedit_disp_container_flags_menu(d);
			break;

		case EObjType::kLiquidContainer:
		case EObjType::kFountain: SendMsgToChar("Начальное количество глотков : ", d->character.get());
			break;

		case EObjType::kBook:
			switch (GET_OBJ_VAL(OLC_OBJ(d), 0)) {
				case EBook::kSpell: oedit_disp_spells_menu(d);
					break;

				case EBook::kSkill:
				case EBook::kSkillUpgrade: oedit_disp_skills2_menu(d);
					break;

				case EBook::kReceipt: oedit_disp_receipts_menu(d);
					break;

				case EBook::kFeat: oedit_disp_feats_menu(d);
					break;

				default: oedit_disp_val4_menu(d);
			}
			break;

		case EObjType::kIngredient: SendMsgToChar("Виртуальный номер прототипа  : ", d->character.get());
			break;

		case EObjType::kCraftMaterial: SendMsgToChar("Введите VNUM прототипа: ", d->character.get());
			break;

		case EObjType::kMagicContaner: SendMsgToChar("Объем колчана: ", d->character.get());
			break;

		case EObjType::kMagicArrow: SendMsgToChar("Размер пучка: ", d->character.get());
			break;

		default: oedit_disp_menu(d);
	}
}

// * Object value #3
void oedit_disp_val3_menu(DescriptorData *d) {
	OLC_MODE(d) = OEDIT_VALUE_3;
	switch (OLC_OBJ(d)->get_type()) {
		case EObjType::kLightSource:
			SendMsgToChar("Длительность горения (0 = погасла, -1 - вечный свет) : ",
						 d->character.get());
			break;

		case EObjType::kScroll:
		case EObjType::kPotion: oedit_disp_spells_menu(d);
			break;

		case EObjType::kWand:
		case EObjType::kStaff: SendMsgToChar("Осталось зарядов : ", d->character.get());
			break;

		case EObjType::kWeapon: SendMsgToChar("Количество граней кубика : ", d->character.get());
			break;

		case EObjType::kContainer:
			SendMsgToChar("Vnum ключа для контейнера (-1 - нет ключа) : ",
						 d->character.get());
			break;

		case EObjType::kLiquidContainer:
		case EObjType::kFountain: oedit_liquid_type(d);
			break;

		case EObjType::kBook:
//		SendMsgToChar("Уровень изучения (+ к умению если тип = 2 ) : ", d->character);
			switch (GET_OBJ_VAL(OLC_OBJ(d), 0)) {
				case EBook::kSkill: SendMsgToChar("Введите уровень изучения : ", d->character.get());
					break;
				case EBook::kSkillUpgrade: SendMsgToChar("На сколько увеличится умение : ", d->character.get());
					break;
				default: oedit_disp_val4_menu(d);
			}
			break;

		case EObjType::kIngredient: SendMsgToChar("Сколько раз можно использовать : ", d->character.get());
			break;

		case EObjType::kCraftMaterial: SendMsgToChar("Введите силу ингридиента: ", d->character.get());
			break;

		case EObjType::kMagicContaner:
		case EObjType::kMagicArrow: SendMsgToChar("Количество стрел: ", d->character.get());
			break;

		default: oedit_disp_menu(d);
	}
}

// * Object value #4
void oedit_disp_val4_menu(DescriptorData *d) {
	OLC_MODE(d) = OEDIT_VALUE_4;
	switch (OLC_OBJ(d)->get_type()) {
		case EObjType::kScroll:
		case EObjType::kPotion:
		case EObjType::kWand:
		case EObjType::kStaff: oedit_disp_spells_menu(d);
			break;

		case EObjType::kWeapon: oedit_disp_weapon_menu(d);
			break;

		case EObjType::kLiquidContainer:
		case EObjType::kFountain:
		case EObjType::kFood:
			SendMsgToChar("Отравлено (0 - не отравлено, 1 - отравлено, >1 - таймер) : ",
						 d->character.get());
			break;

		case EObjType::kBook:
			switch (GET_OBJ_VAL(OLC_OBJ(d), 0)) {
				case EBook::kSkillUpgrade:
					SendMsgToChar("Максимальный % умения :\r\n"
								 "Если <= 0, то учитывается только макс. возможный навык игрока на данном реморте.\r\n"
								 "Если > 0, то учитывается только данное значение без учета макс. навыка игрока.\r\n",
								 d->character.get());
					break;

				default: OLC_VAL(d) = 1;
					oedit_disp_menu(d);
			}
			break;

		case EObjType::kMagicIngredient: SendMsgToChar("Класс ингредиента (0-РОСЛЬ,1-ЖИВЬ,2-ТВЕРДЬ): ", d->character.get());
			break;

		case EObjType::kCraftMaterial: SendMsgToChar("Введите условный уровень: ", d->character.get());
			break;

		case EObjType::kContainer: SendMsgToChar("Введите сложность замка (0-1000): ", d->character.get());
			break;

		default: oedit_disp_menu(d);
	}
}

// * Object type.
void oedit_disp_type_menu(DescriptorData *d) {
	int counter, columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ITEM_TYPES; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				item_types[counter], !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("\r\nВыберите тип предмета : ", d->character.get());
}

// * Object extra flags.
void oedit_disp_extra_menu(DescriptorData *d) {
	disp_planes_values(d, extra_bits, 2);
	OLC_OBJ(d)->get_extra_flags().sprintbits(extra_bits, buf1, ",", 5);
	snprintf(buf,
			 kMaxStringLength,
			 "\r\nЭкстрафлаги: %s%s%s\r\n" "Выберите экстрафлаг: (помеченное '*' пользоваться вдумчиво. 0 - выход) : ",
			 cyn,
			 buf1,
			 nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_anti_menu(DescriptorData *d) {
	disp_planes_values(d, anti_bits, 2);
	OLC_OBJ(d)->get_anti_flags().sprintbits(anti_bits, buf1, ",", 5);
	snprintf(buf,
			 kMaxStringLength,
			 "\r\nПредмет запрещен для : %s%s%s\r\n" "Выберите флаг запрета (0 - выход) : ",
			 cyn,
			 buf1,
			 nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_no_menu(DescriptorData *d) {
	disp_planes_values(d, no_bits, 2);
	OLC_OBJ(d)->get_no_flags().sprintbits(no_bits, buf1, ",", 5);
	snprintf(buf,
			 kMaxStringLength,
			 "\r\nПредмет неудобен для : %s%s%s\r\n" "Выберите флаг неудобств (0 - выход) : ",
			 cyn,
			 buf1,
			 nrm);
	SendMsgToChar(buf, d->character.get());
}

void show_weapon_affects_olc(DescriptorData *d, const FlagData &flags) {
	disp_planes_values(d, weapon_affects, 2);
	flags.sprintbits(weapon_affects, buf1, ",", 5);
	snprintf(buf, kMaxStringLength, "\r\nНакладываемые аффекты : %s%s%s\r\n"
									 "Выберите аффект (0 - выход) : ", cyn, buf1, nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_affects_menu(DescriptorData *d) {
	show_weapon_affects_olc(d, OLC_OBJ(d)->get_affect_flags());
}

// * Object wear flags.
void oedit_disp_wear_menu(DescriptorData *d) {
	int counter, columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ITEM_WEARS; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				wear_bits[counter], !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintbit(OLC_OBJ(d)->get_wear_flags(), wear_bits, buf1);
	snprintf(buf,
			 kMaxStringLength,
			 "\r\nМожет быть одет : %s%s%s\r\n" "Выберите позицию (0 - выход) : ",
			 cyn,
			 buf1,
			 nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_mater_menu(DescriptorData *d) {
	int counter, columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < 32 && *material_name[counter] != '\n'; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				material_name[counter], !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintf(buf, "\r\nСделан из : %s%s%s\r\n"
				 "Выберите материал (0 - выход) : ", cyn, material_name[OLC_OBJ(d)->get_material()], nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_ingradient_menu(DescriptorData *d) {
	int counter, columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < 32 && *ingradient_bits[counter] != '\n'; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				ingradient_bits[counter], !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintbit(OLC_OBJ(d)->get_spec_param(), ingradient_bits, buf1);
	snprintf(buf, kMaxStringLength, "\r\nТип ингредиента : %s%s%s\r\n" "Дополните тип (0 - выход) : ", cyn, buf1, nrm);
	SendMsgToChar(buf, d->character.get());
}

void oedit_disp_magic_container_menu(DescriptorData *d) {
	int counter, columns = 0;
	for (counter = 0; counter < 32 && *magic_container_bits[counter] != '\n'; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				magic_container_bits[counter], !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintbit(OLC_OBJ(d)->get_spec_param(), magic_container_bits, buf1);
	snprintf(buf, kMaxStringLength, "\r\nТип контейнера : %s%s%s\r\n" "Дополните тип (0 - выход) : ", cyn, buf1, nrm);
	SendMsgToChar(buf, d->character.get());
}

std::string print_spell_value(ObjData *obj, const ObjVal::EValueKey key1, const ObjVal::EValueKey key2) {
	if (obj->GetPotionValueKey(key1) < 0) {
		return "нет";
	}
	char buf_[kMaxInputLength];
	snprintf(buf_, sizeof(buf_), "%s:%d",
			 MUD::Spell(static_cast<ESpell>(obj->GetPotionValueKey(key1))).GetCName(), obj->GetPotionValueKey(key2));
	return buf_;
}

void drinkcon_values_menu(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif

	char buf_[1024];
	snprintf(buf_, sizeof(buf_),
			 "%s1%s) Заклинание1 : %s%s\r\n"
			 "%s2%s) Заклинание2 : %s%s\r\n"
			 "%s3%s) Заклинание3 : %s%s\r\n"
			 "%s",
			 grn, nrm, cyn,
			 print_spell_value(OLC_OBJ(d),
							   ObjVal::EValueKey::POTION_SPELL1_NUM,
							   ObjVal::EValueKey::POTION_SPELL1_LVL).c_str(),
			 grn, nrm, cyn,
			 print_spell_value(OLC_OBJ(d),
							   ObjVal::EValueKey::POTION_SPELL2_NUM,
							   ObjVal::EValueKey::POTION_SPELL2_LVL).c_str(),
			 grn, nrm, cyn,
			 print_spell_value(OLC_OBJ(d),
							   ObjVal::EValueKey::POTION_SPELL3_NUM,
							   ObjVal::EValueKey::POTION_SPELL3_LVL).c_str(),
			 nrm);

	SendMsgToChar(buf_, d->character.get());
	SendMsgToChar("Ваш выбор (0 - выход) :", d->character.get());
	return;
}

std::array<const char *, 9> wskill_bits =
	{{
		 "палицы и дубины(141)",
		 "секиры(142)",
		 "длинные лезвия(143)",
		 "короткие лезвия(144)",
		 "иное(145)",
		 "двуручники(146)",
		 "проникающее(147)",
		 "копья и рогатины(148)",
		 "луки(154)"
	 }};

void oedit_disp_skills_menu(DescriptorData *d) {
	if (OLC_OBJ(d)->get_type() == EObjType::kIngredient) {
		oedit_disp_ingradient_menu(d);
		return;
	}
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	int columns = 0;
	for (size_t counter = 0; counter < wskill_bits.size(); counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s",
				grn,
				static_cast<int>(counter + 1),
				nrm,
				wskill_bits[counter],
				!(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	sprintf(buf,
			"%sТренируемое умение : %s%d%s\r\n"
			"Выберите умение (0 - выход) : ",
			(columns % 2 == 1 ? "\r\n" : ""), cyn, OLC_OBJ(d)->get_spec_param(), nrm);
	SendMsgToChar(buf, d->character.get());
}

std::string print_values2_menu(ObjData *obj) {
	if (obj->get_type() == EObjType::kLiquidContainer
		|| obj->get_type() == EObjType::kFountain) {
		return "Спец. параметры";
	}

	char buf_[kMaxInputLength];    
	snprintf(buf_, sizeof(buf_), "Спец. параметры: %d", obj->get_spec_param());
	return buf_;
}

// * Display main menu.
void oedit_disp_menu(DescriptorData *d) {
	ObjData *obj;

	obj = OLC_OBJ(d);
	sprinttype(obj->get_type(), item_types, buf1);
	obj->get_extra_flags().sprintbits(extra_bits, buf2, ",", 4);

	snprintf(buf, kMaxStringLength,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
			 "-- Предмет : [%s%d%s]\r\n"
			 "%s1%s) Синонимы : %s&S%s&s\r\n"
			 "%s2&n) Именительный (это ЧТО)             : %s&e\r\n"
			 "%s3&n) Родительный  (нету ЧЕГО)           : %s&e\r\n"
			 "%s4&n) Дательный    (прикрепить к ЧЕМУ)   : %s&e\r\n"
			 "%s5&n) Винительный  (держать ЧТО)         : %s&e\r\n"
			 "%s6&n) Творительный (вооружиться ЧЕМ)     : %s&e\r\n"
			 "%s7&n) Предложный   (писать на ЧЕМ)       : %s&e\r\n"
			 "%s8&n) Краткое описание  :-\r\n&Y&q%s&e&Q\r\n"
			 "%s9&n) Опис.при действии :-\r\n%s%s\r\n"
			 "%sA%s) Тип предмета      :-\r\n%s%s\r\n"
			 "%sB%s) Экстрафлаги       :-\r\n%s%s\r\n",
			 cyn, OLC_NUM(d), nrm,
			 grn, nrm, yel, not_empty(obj->get_aliases()),
			 grn, not_empty(obj->get_PName(ECase::kNom)),
			 grn, not_empty(obj->get_PName(ECase::kGen)),
			 grn, not_empty(obj->get_PName(ECase::kDat)),
			 grn, not_empty(obj->get_PName(ECase::kAcc)),
			 grn, not_empty(obj->get_PName(ECase::kIns)),
			 grn, not_empty(obj->get_PName(ECase::kPre)),
			 grn, not_empty(obj->get_description()),
			 grn, yel, not_empty(obj->get_action_description(), "<not set>\r\n"),
			 grn, nrm, cyn, buf1, grn, nrm, cyn, buf2);
	// * Send first half.
	SendMsgToChar(buf, d->character.get());

	sprintbit(obj->get_wear_flags(), wear_bits, buf1);
	obj->get_no_flags().sprintbits(no_bits, buf2, ",");
	snprintf(buf, kMaxStringLength,
			 "%sC%s) Одевается  : %s%s\r\n"
			 "%sD%s) Неудобен    : %s%s\r\n", grn, nrm, cyn, buf1, grn, nrm, cyn, buf2);
	SendMsgToChar(buf, d->character.get());

	obj->get_anti_flags().sprintbits(anti_bits, buf1, ",", 4);
	obj->get_affect_flags().sprintbits(weapon_affects, buf2, ",", 4);
	const size_t gender = static_cast<size_t>(to_underlying(GET_OBJ_SEX(obj)));
	snprintf(buf, kMaxStringLength,
			 "%sE%s) Запрещен    : %s%s\r\n"
			 "%sF%s) Вес         : %s%8d   %sG%s) Цена        : %s%d\r\n"
			 "%sH%s) Рента(снято): %s%8d   %sI%s) Рента(одето): %s%d\r\n"
			 "%sJ%s) Макс.проч.  : %s%8d   %sK%s) Тек.проч    : %s%d\r\n"
			 "%sL%s) Материал    : %s%s\r\n"
			 "%sM%s) Таймер      : %s%8d   %sN%s) %s\r\n"
			 "%sO%s) Values      : %s%d %d %d %d\r\n"
			 "%sP%s) Аффекты     : %s%s\r\n"
			 "%sR%s) Меню наводимых аффектов\r\n"
			 "%sT%s) Меню экстраописаний\r\n"
			 "%sS%s) Скрипт      : %s%s\r\n"
			 "%sU%s) Пол         : %s%s\r\n"
			 "%sV%s) Макс.в мире : %s%d\r\n"
			 "%sW%s) Меню умений\r\n"
			 "%sX%s) Требует перевоплощений: %s%d\r\n"
			 "%sZ%s) Клонирование\r\n"
			 "%sQ%s) Quit\r\n"
			 "Ваш выбор : ",
			 grn, nrm, cyn, buf1,
			 grn, nrm, cyn, obj->get_weight(),
			 grn, nrm, cyn, obj->get_cost(),
			 grn, nrm, cyn, obj->get_rent_off(),
			 grn, nrm, cyn, obj->get_rent_on(),
			 grn, nrm, cyn, obj->get_maximum_durability(),
			 grn, nrm, cyn, obj->get_current_durability(),
			 grn, nrm, cyn, material_name[obj->get_material()],
			 grn, nrm, cyn, obj->get_timer(),
			 grn, nrm, print_values2_menu(obj).c_str(),
			 grn, nrm, cyn,
			 GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
			 GET_OBJ_VAL(obj, 3), grn, nrm, grn, buf2, grn, nrm, grn, nrm, grn,
			 nrm, cyn, !obj->get_proto_script().empty() ? "Присутствуют" : "Отсутствуют",
			 grn, nrm, cyn, genders[gender],
			 grn, nrm, cyn, obj->get_max_in_world(),
			 grn, nrm,
			 grn, nrm, cyn, obj->get_minimum_remorts(),
			 grn, nrm,
			 grn, nrm);
	SendMsgToChar(buf, d->character.get());

	OLC_MODE(d) = OEDIT_MAIN_MENU;
}

// ***************************************************************************
// * main loop (of sorts).. basically interpreter throws all input to here   *
// ***************************************************************************
int planebit(const char *str, int *plane, int *bit) {
	if (!str || !*str)
		return (-1);
	if (*str == '0')
		return (0);
	if (*str >= 'a' && *str <= 'z')
		*bit = (*(str) - 'a');
	else if (*str >= 'A' && *str <= 'D')
		*bit = (*(str) - 'A' + 26);
	else
		return (-1);

	if (*(str + 1) >= '0' && *(str + 1) <= '3')
		*plane = (*(str + 1) - '0');
	else
		return (-1);
	return (1);
}

void check_potion_proto(ObjData *obj) {
	if (obj->GetPotionValueKey(ObjVal::EValueKey::POTION_SPELL1_NUM) > 0
		|| obj->GetPotionValueKey(ObjVal::EValueKey::POTION_SPELL2_NUM) > 0
		|| obj->GetPotionValueKey(ObjVal::EValueKey::POTION_SPELL3_NUM) > 0) {
		obj->SetPotionValueKey(ObjVal::EValueKey::POTION_PROTO_VNUM, 0);
	} else {
		obj->SetPotionValueKey(ObjVal::EValueKey::POTION_PROTO_VNUM, -1);
	}
}

bool parse_val_spell_num(DescriptorData *d, const ObjVal::EValueKey key, int val) {
	auto spell_id = static_cast<ESpell>(val);
	if (spell_id < ESpell::kFirst || spell_id >= ESpell::kLast) {
		if (val != 0) {
			SendMsgToChar("Неверный выбор.\r\n", d->character.get());
		}
		OLC_OBJ(d)->SetPotionValueKey(key, -1);
		check_potion_proto(OLC_OBJ(d));
		OLC_MODE(d) = OEDIT_DRINKCON_VALUES;
		drinkcon_values_menu(d);
		return false;
	}
	OLC_OBJ(d)->SetPotionValueKey(key, val);
	SendMsgToChar(d->character.get(), "Выбранное заклинание: %s\r\n"
									  "Ведите уровень заклинания от 1 до 50 (0 - выход) :",
				  MUD::Spell(spell_id).GetCName());
	return true;
}

void parse_val_spell_lvl(DescriptorData *d, const ObjVal::EValueKey key, int val) {
	if (val <= 0 || val > 50) {
		if (val != 0) {
			SendMsgToChar("Некорректный уровень заклинания.\r\n", d->character.get());
		}

		switch (key) {
			case ObjVal::EValueKey::POTION_SPELL1_LVL: OLC_OBJ(d)->SetPotionValueKey(ObjVal::EValueKey::POTION_SPELL1_NUM, -1);
				break;

			case ObjVal::EValueKey::POTION_SPELL2_LVL: OLC_OBJ(d)->SetPotionValueKey(ObjVal::EValueKey::POTION_SPELL2_NUM, -1);
				break;

			case ObjVal::EValueKey::POTION_SPELL3_LVL: OLC_OBJ(d)->SetPotionValueKey(ObjVal::EValueKey::POTION_SPELL3_NUM, -1);
				break;

			default: break;
		}

		check_potion_proto(OLC_OBJ(d));
		OLC_MODE(d) = OEDIT_DRINKCON_VALUES;
		drinkcon_values_menu(d);

		return;
	}
	OLC_OBJ(d)->SetPotionValueKey(key, val);
	check_potion_proto(OLC_OBJ(d));
	OLC_MODE(d) = OEDIT_DRINKCON_VALUES;
	drinkcon_values_menu(d);
}

void oedit_disp_clone_menu(DescriptorData *d) {
	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
			"%s1%s) Заменить триггеры\r\n"
			"%s2%s) Не заменять триггеры\r\n"
			"%s3%s) Quit\r\n"
			"Ваш выбор : ",
			grn, nrm,
			grn, nrm,
			grn, nrm);

	SendMsgToChar(buf, d->character.get());
}

void oedit_parse(DescriptorData *d, char *arg) {
	int number{};
	int max_val, min_val, plane, bit;
	bool need_break = false;
	ESkill skill_id{};

	switch (OLC_MODE(d)) {
		case OEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
				case 'д':
				case 'Д': SendMsgToChar("Объект сохранен.\r\n", d->character.get());
					OLC_OBJ(d)->remove_incorrect_values_keys(OLC_OBJ(d)->get_type());
					oedit_save_internally(d);
					sprintf(buf, "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
					olc_log("%s edit obj %d", GET_NAME(d->character), OLC_NUM(d));
					mudlog(buf, NRM, std::max(kLvlBuilder, GET_INVIS_LEV(d->character)), SYSLOG, true);
					cleanup_olc(d, CLEANUP_STRUCTS);
					break;

				case 'n':
				case 'N':
				case 'н':
				case 'Н': cleanup_olc(d, CLEANUP_ALL);
					break;

				default: SendMsgToChar("Неверный выбор!\r\n", d->character.get());
					SendMsgToChar("Вы желаете сохранить этот предмет?\r\n", d->character.get());
					break;
			}
			return;

		case OEDIT_MAIN_MENU:
			// * Throw us out to whichever edit mode based on user input.
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d))    // Something has been modified.
					{
						SendMsgToChar("Вы желаете сохранить этот предмет? : ", d->character.get());
						OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
					} else {
						cleanup_olc(d, CLEANUP_ALL);
					}
					return;

				case '1': SendMsgToChar("Введите синонимы : ", d->character.get());
					OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
					break;

				case '2':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nИменительный падеж [это ЧТО] : ",
								  OLC_OBJ(d)->get_PName(ECase::kNom).c_str());
					OLC_MODE(d) = OEDIT_PAD0;
					break;

				case '3':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nРодительный падеж [нет ЧЕГО] : ",
								  OLC_OBJ(d)->get_PName(ECase::kGen).c_str());
					OLC_MODE(d) = OEDIT_PAD1;
					break;

				case '4':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nДательный падеж [прикрепить к ЧЕМУ] : ",
								  OLC_OBJ(d)->get_PName(ECase::kDat).c_str());
					OLC_MODE(d) = OEDIT_PAD2;
					break;

				case '5':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nВинительный падеж [держать ЧТО] : ",
								  OLC_OBJ(d)->get_PName(ECase::kAcc).c_str());
					OLC_MODE(d) = OEDIT_PAD3;
					break;

				case '6':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nТворительный падеж [вооружиться ЧЕМ] : ",
								  OLC_OBJ(d)->get_PName(ECase::kIns).c_str());
					OLC_MODE(d) = OEDIT_PAD4;
					break;
				case '7':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nПредложный падеж [писать на ЧЕМ] : ",
								  OLC_OBJ(d)->get_PName(ECase::kPre).c_str());
					OLC_MODE(d) = OEDIT_PAD5;
					break;

				case '8':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nВведите длинное описание :-\r\n| ",
								  OLC_OBJ(d)->get_description().c_str());
					OLC_MODE(d) = OEDIT_LONGDESC;
					break;

				case '9': OLC_MODE(d) = OEDIT_ACTDESC;
					iosystem::write_to_output("Введите описание при применении: (/s сохранить /h помощь)\r\n\r\n", d);
					d->backstr = nullptr;
					if (!OLC_OBJ(d)->get_action_description().empty()) {
					iosystem::write_to_output(OLC_OBJ(d)->get_action_description().c_str(), d);
						d->backstr = str_dup(OLC_OBJ(d)->get_action_description().c_str());
					}
					d->writer.reset(new CActionDescriptionWriter(*OLC_OBJ(d)));
					d->max_str = 4096;
					d->mail_to = 0;
					OLC_VAL(d) = 1;
					break;

				case 'a':
				case 'A': oedit_disp_type_menu(d);
					OLC_MODE(d) = OEDIT_TYPE;
					break;

				case 'b':
				case 'B': oedit_disp_extra_menu(d);
					OLC_MODE(d) = OEDIT_EXTRAS;
					break;

				case 'c':
				case 'C': oedit_disp_wear_menu(d);
					OLC_MODE(d) = OEDIT_WEAR;
					break;

				case 'd':
				case 'D': oedit_disp_no_menu(d);
					OLC_MODE(d) = OEDIT_NO;
					break;

				case 'e':
				case 'E': oedit_disp_anti_menu(d);
					OLC_MODE(d) = OEDIT_ANTI;
					break;

				case 'f':
				case 'F': SendMsgToChar("Вес предмета : ", d->character.get());
					OLC_MODE(d) = OEDIT_WEIGHT;
					break;

				case 'g':
				case 'G': SendMsgToChar("Цена предмета : ", d->character.get());
					OLC_MODE(d) = OEDIT_COST;
					break;

				case 'h':
				case 'H': SendMsgToChar("Рента предмета (в инвентаре) : ", d->character.get());
					OLC_MODE(d) = OEDIT_COSTPERDAY;
					break;

				case 'i':
				case 'I': SendMsgToChar("Рента предмета (в экипировке) : ", d->character.get());
					OLC_MODE(d) = OEDIT_COSTPERDAYEQ;
					break;

				case 'j':
				case 'J': SendMsgToChar("Максимальная прочность : ", d->character.get());
					OLC_MODE(d) = OEDIT_MAXVALUE;
					break;

				case 'k':
				case 'K': SendMsgToChar("Текущая прочность : ", d->character.get());
					OLC_MODE(d) = OEDIT_CURVALUE;
					break;

				case 'l':
				case 'L': oedit_disp_mater_menu(d);
					OLC_MODE(d) = OEDIT_MATER;
					break;

				case 'm':
				case 'M': SendMsgToChar("Таймер (в минутах РЛ) : ", d->character.get());
					OLC_MODE(d) = OEDIT_TIMER;
					break;

				case 'n':
				case 'N':
					if (OLC_OBJ(d)->get_type() == EObjType::kWeapon
						|| OLC_OBJ(d)->get_type() == EObjType::kIngredient) {
						oedit_disp_skills_menu(d);
						OLC_MODE(d) = OEDIT_SKILL;
					} else if (OLC_OBJ(d)->get_type() == EObjType::kLiquidContainer
						|| OLC_OBJ(d)->get_type() == EObjType::kFountain) {
						drinkcon_values_menu(d);
						OLC_MODE(d) = OEDIT_DRINKCON_VALUES;
					} else {
						oedit_disp_menu(d);
					}
					break;

				case 'o':
				case 'O':
					// * Clear any old values
					OLC_OBJ(d)->set_val(0, 0);
					OLC_OBJ(d)->set_val(1, 0);
					OLC_OBJ(d)->set_val(2, 0);
					OLC_OBJ(d)->set_val(3, 0);
					oedit_disp_val1_menu(d);
					break;

				case 'p':
				case 'P': oedit_disp_affects_menu(d);
					OLC_MODE(d) = OEDIT_AFFECTS;
					break;

				case 'r':
				case 'R': oedit_disp_prompt_apply_menu(d);
					break;

				case 't':
				case 'T':
					// * If extra descriptions don't exist.
					if (!OLC_OBJ(d)->get_ex_description()) {
						OLC_OBJ(d)->set_ex_description(new ExtraDescription());
					}
					OLC_DESC(d) = OLC_OBJ(d)->get_ex_description();
					oedit_disp_extradesc_menu(d);
					break;

				case 's':
				case 'S': dg_olc_script_copy(d);
					OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
					dg_script_menu(d);
					return;

				case 'u':
				case 'U': SendMsgToChar("Пол (0-3) : ", d->character.get());
					OLC_MODE(d) = OEDIT_SEXVALUE;
					break;

				case 'v':
				case 'V': SendMsgToChar("Максимальное число в мире : ", d->character.get());
					OLC_MODE(d) = OEDIT_MIWVALUE;
					break;

				case 'w':
				case 'W': oedit_disp_skills_mod_menu(d);
					OLC_MODE(d) = OEDIT_SKILLS;
					break;

				case 'x':
				case 'X':
					SendMsgToChar(
						"Требует перевоплощений (-1 выключено, 0 автопростановка, в - до какого морта, в + после какого): ",
						d->character.get());
					OLC_MODE(d) = OEDIT_MORT_REQ;
					break;

				case 'z':
				case 'Z': OLC_MODE(d) = OEDIT_CLONE;
					oedit_disp_clone_menu(d);
					break;

				default: oedit_disp_menu(d);
					break;
			}
			olc_log("%s command %c", GET_NAME(d->character), *arg);
			return;
			// * end of OEDIT_MAIN_MENU

		case OLC_SCRIPT_EDIT:
			if (dg_script_edit_parse(d, arg)) {
				return;
			}
			break;

		case OEDIT_EDIT_NAMELIST: OLC_OBJ(d)->set_aliases(not_null(arg, nullptr));
			break;

		case OEDIT_PAD0: OLC_OBJ(d)->set_short_description(not_null(arg, "что-то"));
			OLC_OBJ(d)->set_PName(ECase::kNom, not_null(arg, "что-то"));
			break;

		case OEDIT_PAD1: OLC_OBJ(d)->set_PName(ECase::kGen, not_null(arg, "-чего-то"));
			break;

		case OEDIT_PAD2: OLC_OBJ(d)->set_PName(ECase::kDat, not_null(arg, "-чему-то"));
			break;

		case OEDIT_PAD3: OLC_OBJ(d)->set_PName(ECase::kAcc, not_null(arg, "-что-то"));
			break;

		case OEDIT_PAD4: OLC_OBJ(d)->set_PName(ECase::kIns, not_null(arg, "-чем-то"));
			break;

		case OEDIT_PAD5: OLC_OBJ(d)->set_PName(ECase::kPre, not_null(arg, "-чем-то"));
			break;

		case OEDIT_LONGDESC: OLC_OBJ(d)->set_description(not_null(arg, "неопределено"));
			break;

		case OEDIT_TYPE: number = atoi(arg);
			if ((number < 1) || (number >= NUM_ITEM_TYPES)) {
				SendMsgToChar("Invalid choice, try again : ", d->character.get());
				return;
			} else {
				OLC_OBJ(d)->set_type(static_cast<EObjType>(number));
				sprintf(buf, "%s  меняет тип предмета для %d!!!", GET_NAME(d->character), OLC_NUM(d));
				mudlog(buf, BRF, kLvlGod, SYSLOG, true);
				if (number != EObjType::kWeapon && number != EObjType::kIngredient) {
					OLC_OBJ(d)->set_spec_param(0);
				}
			}
			break;

		case OEDIT_EXTRAS: number = planebit(arg, &plane, &bit);
			if (number < 0) {
				oedit_disp_extra_menu(d);
				return;
			} else if (number == 0) {
				break;
			} else {
				OLC_OBJ(d)->toggle_extra_flag(plane, 1 << bit);
				oedit_disp_extra_menu(d);
				return;
			}

		case OEDIT_WEAR: number = atoi(arg);
			if ((number < 0) || (number > NUM_ITEM_WEARS)) {
				SendMsgToChar("Неверный выбор!\r\n", d->character.get());
				oedit_disp_wear_menu(d);
				return;
			} else if (number == 0)    // Quit.
			{
				break;
			} else {
				OLC_OBJ(d)->toggle_wear_flag(1 << (number - 1));
				oedit_disp_wear_menu(d);
				return;
			}

		case OEDIT_NO: number = planebit(arg, &plane, &bit);
			if (number < 0) {
				oedit_disp_no_menu(d);
				return;
			} else if (number == 0) {
				break;
			} else {
				OLC_OBJ(d)->toggle_no_flag(plane, 1 << bit);
				oedit_disp_no_menu(d);
				return;
			}

		case OEDIT_ANTI: number = planebit(arg, &plane, &bit);
			if (number < 0) {
				oedit_disp_anti_menu(d);
				return;
			} else if (number == 0) {
				break;
			} else {
				OLC_OBJ(d)->toggle_anti_flag(plane, 1 << bit);
				oedit_disp_anti_menu(d);
				return;
			}

		case OEDIT_WEIGHT: OLC_OBJ(d)->set_weight(atoi(arg));
			break;

		case OEDIT_COST: OLC_OBJ(d)->set_cost(atoi(arg));
			break;

		case OEDIT_COSTPERDAY: OLC_OBJ(d)->set_rent_off(atoi(arg));
			break;

		case OEDIT_MAXVALUE: OLC_OBJ(d)->set_maximum_durability(atoi(arg));
			break;

		case OEDIT_CURVALUE: OLC_OBJ(d)->set_current_durability(MIN(OLC_OBJ(d)->get_maximum_durability(), atoi(arg)));
			break;

		case OEDIT_SEXVALUE:
			if ((number = atoi(arg)) >= 0
				&& number < static_cast<int>(EGender::kLast)) {
				OLC_OBJ(d)->set_sex(static_cast<EGender>(number));
			} else {
				SendMsgToChar("Пол (0-3) : ", d->character.get());
				return;
			}
			break;

		case OEDIT_MIWVALUE:
			if ((number = atoi(arg)) >= -1 && number <= 10000 && number != 0) {
				OLC_OBJ(d)->set_max_in_world(number);
			} else {
				SendMsgToChar("Максимальное число предметов в мире (1-100000 или -1) : ", d->character.get());
				return;
			}
			break;

		case OEDIT_MATER: number = atoi(arg);
			if (number < 0 || number > NUM_MATERIALS) {
				oedit_disp_mater_menu(d);
				return;
			} else if (number > 0) {
				OLC_OBJ(d)->set_material(static_cast<EObjMaterial>(number - 1));
			}
			break;

		case OEDIT_COSTPERDAYEQ: OLC_OBJ(d)->set_rent_on(atoi(arg));
			break;

		case OEDIT_TIMER: OLC_OBJ(d)->set_timer(atoi(arg));
			break;

		case OEDIT_SKILL: number = atoi(arg);
			if (number < 0) {
				oedit_disp_skills_menu(d);
				return;
			}
			if (number == 0) {
				break;
			}
			if (OLC_OBJ(d)->get_type() == EObjType::kIngredient) {
				OLC_OBJ(d)->toggle_skill(1 << (number - 1));
				oedit_disp_skills_menu(d);
				return;
			}
			if (OLC_OBJ(d)->get_type() == EObjType::kWeapon)
				switch (number) {
					case 1: number = 141;
						break;
					case 2: number = 142;
						break;
					case 3: number = 143;
						break;
					case 4: number = 144;
						break;
					case 5: number = 145;
						break;
					case 6: number = 146;
						break;
					case 7: number = 147;
						break;
					case 8: number = 148;
						break;
					case 9: number = 154;
						break;
					default: oedit_disp_skills_menu(d);
						return;
				}
			OLC_OBJ(d)->set_spec_param(number);
			oedit_disp_skills_menu(d);
			return;
			break;

		case OEDIT_VALUE_1:
			// * Lucky, I don't need to check any of these for out of range values.
			// * Hmm, I'm not so sure - Rv
			number = atoi(arg);

			if (OLC_OBJ(d)->get_type() == EObjType::kBook
				&& (number < 0
					|| number > 4)) {
				SendMsgToChar("Неправильный тип книги, повторите.\r\n", d->character.get());
				oedit_disp_val1_menu(d);
				return;
			}
			// * proceed to menu 2
			OLC_OBJ(d)->set_val(0, number);
			OLC_VAL(d) = 1;
			oedit_disp_val2_menu(d);
			return;

		case OEDIT_VALUE_2:
			// * Here, I do need to check for out of range values.
			number = atoi(arg);
			switch (OLC_OBJ(d)->get_type()) {
				case EObjType::kScroll:
				case EObjType::kPotion: {
					if (number == 0) {
						need_break = true;
						break;
					}
					auto spell_id = static_cast<ESpell>(number);
					if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
						oedit_disp_val2_menu(d);
					} else {
						OLC_OBJ(d)->set_val(1, number);
						oedit_disp_val3_menu(d);
					}
					return;
				}
				case EObjType::kContainer:
					// Needs some special handling since we are dealing with flag values
					// here.
					if (number < 0
						|| number > 4) {
						oedit_disp_container_flags_menu(d);
					} else if (number != 0) {
						OLC_OBJ(d)->toggle_val_bit(1, 1 << (number - 1));
						OLC_VAL(d) = 1;
						oedit_disp_val2_menu(d);
					} else {
						oedit_disp_val3_menu(d);
					}
					return;

				case EObjType::kBook:
					switch (GET_OBJ_VAL(OLC_OBJ(d), 0)) {
						case EBook::kSpell: {
							if (number == 0) {
								OLC_VAL(d) = 0;
								oedit_disp_menu(d);
								return;
							}
							auto spell_id = static_cast<ESpell>(number);
							if (MUD::Spell(spell_id).IsInvalid()) {
								SendMsgToChar("Неизвестное заклинание, повторите.\r\n", d->character.get());
								oedit_disp_val2_menu(d);
								return;
							}
							break;
						}

						case EBook::kSkill:
						case EBook::kSkillUpgrade:
							if (number == 0) {
								OLC_VAL(d) = 0;
								oedit_disp_menu(d);
								return;
							}
							skill_id = static_cast<ESkill>(number);
							if (skill_id > ESkill::kLast
								|| !MUD::Skill(skill_id).GetName()
								|| *MUD::Skill(skill_id).GetName() == '!') {
								SendMsgToChar("Неизвестное умение, повторите.\r\n", d->character.get());
								oedit_disp_val2_menu(d);
								return;
							}
							break;

						case EBook::kReceipt:
							if (number > top_imrecipes || number < 0 || !imrecipes[number].name) {
								SendMsgToChar("Неизвестный рецепт, повторите.\r\n", d->character.get());
								oedit_disp_val2_menu(d);
								return;
							}
							break;

						case EBook::kFeat:
							if (number == 0) {
								OLC_VAL(d) = 0;
								oedit_disp_menu(d);
								return;
							}
							auto feat_id = static_cast<EFeat>(number);
							if (MUD::Feat(feat_id).IsInvalid()) {
								SendMsgToChar("Неизвестная способность, повторите.\r\n", d->character.get());
								oedit_disp_val2_menu(d);
								return;
							}
							break;
					}
				default: break;
			}
			if (need_break)
				break;
			OLC_OBJ(d)->set_val(1, number);
			OLC_VAL(d) = 1;
			oedit_disp_val3_menu(d);
			return;

		case OEDIT_VALUE_3: number = atoi(arg);
						// * Quick'n'easy error checking.
			switch (OLC_OBJ(d)->get_type()) {
				case EObjType::kScroll:
				case EObjType::kPotion: 
					if (number == 0) {
						need_break = true;
						break;
					}
					min_val = -1;
					max_val = to_underlying(ESpell::kLast);
					break;

				case EObjType::kWeapon: min_val = 1;
					max_val = 50;
					break;

				case EObjType::kWand:
				case EObjType::kStaff: min_val = 0;
					max_val = 200;
					break;

				case EObjType::kLiquidContainer:
				case EObjType::kFountain: min_val = 0;
					max_val = NUM_LIQ_TYPES - 1;
					break;

				case EObjType::kCraftMaterial: min_val = 0;
					max_val = 1000;
					break;

				default: min_val = -999999;
					max_val = 999999;
			}
			if (need_break)
				break;
			OLC_OBJ(d)->set_val(2, MAX(min_val, MIN(number, max_val)));
			OLC_VAL(d) = 1;
			oedit_disp_val4_menu(d);
			return;

		case OEDIT_VALUE_4:
			number = atoi(arg);
			min_val = -999999;
			max_val = 999999;
			switch (OLC_OBJ(d)->get_type()) {
				case EObjType::kScroll:
				case EObjType::kPotion:
					if (number == 0) {
						break;
					}
					min_val = 1;
					max_val = to_underlying(ESpell::kLast);
					break;
				case EObjType::kWand:
				case EObjType::kStaff: 
					min_val = 1;
					max_val = to_underlying(ESpell::kLast);
					break;
				case EObjType::kWeapon: 
					min_val = 0;
					max_val = NUM_ATTACK_TYPES - 1;
					break;
				case EObjType::kMagicIngredient: 
					min_val = 0;
					max_val = 2;
					break;
				case EObjType::kCraftMaterial: 
					min_val = 0;
					max_val = 100;
					break;
				default:
					break;
			}
			OLC_OBJ(d)->set_val(3, MAX(min_val, MIN(number, max_val)));
			break;

		case OEDIT_AFFECTS: number = planebit(arg, &plane, &bit);
			if (number < 0) {
				oedit_disp_affects_menu(d);
				return;
			} else if (number == 0) {
				break;
			} else {
				OLC_OBJ(d)->toggle_affect_flag(plane, 1 << bit);
				oedit_disp_affects_menu(d);
				return;
			}

		case OEDIT_PROMPT_APPLY:
			if ((number = atoi(arg)) == 0)
				break;
			else if (number < 0 || number > kMaxObjAffect) {
				oedit_disp_prompt_apply_menu(d);
				return;
			}
			OLC_VAL(d) = number - 1;
			OLC_MODE(d) = OEDIT_APPLY;
			oedit_disp_apply_menu(d);
			return;

		case OEDIT_APPLY:
			if ((number = atoi(arg)) == 0) {
				OLC_OBJ(d)->set_affected(OLC_VAL(d), EApply::kNone, 0);
				oedit_disp_prompt_apply_menu(d);
			} else if (number < 0 || number >= EApply::kNumberApplies) {
				oedit_disp_apply_menu(d);
			} else {
				if (apply_types[number][0] == '*') {
					SendMsgToChar("&RЭтот параметр устанавливать запрещено.&n\r\n", d->character.get());
					oedit_disp_apply_menu(d);
					return;
				}
				OLC_OBJ(d)->set_affected_location(OLC_VAL(d), static_cast<EApply>(number));
				SendMsgToChar("Modifier : ", d->character.get());
				OLC_MODE(d) = OEDIT_APPLYMOD;
			}
			return;

		case OEDIT_APPLYMOD: OLC_OBJ(d)->set_affected_modifier(OLC_VAL(d), atoi(arg));
			oedit_disp_prompt_apply_menu(d);
			return;

		case OEDIT_EXTRADESC_KEY:
			if (OLC_DESC(d)->keyword)
				free(OLC_DESC(d)->keyword);
			OLC_DESC(d)->keyword = str_dup(not_null(arg, nullptr));
			oedit_disp_extradesc_menu(d);
			return;

		case OEDIT_EXTRADESC_MENU:
			switch ((number = atoi(arg))) {
				case 0:
					if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
						OLC_DESC(d).reset();
						OLC_OBJ(d)->set_ex_description(nullptr);
					}
					break;

				case 1: OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
					SendMsgToChar("Enter keywords, separated by spaces :-\r\n| ", d->character.get());
					return;

				case 2: OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
				iosystem::write_to_output("Enter the extra description: (/s saves /h for help)\r\n\r\n", d);
					d->backstr = nullptr;
					if (OLC_DESC(d)->description) {
					iosystem::write_to_output(OLC_DESC(d)->description, d);
						d->backstr = str_dup(OLC_DESC(d)->description);
					}
					d->writer.reset(new utils::DelegatedStringWriter(OLC_DESC(d)->description));
					d->max_str = 4096;
					d->mail_to = 0;
					OLC_VAL(d) = 1;
					return;

				case 3:
					// * Only go to the next description if this one is finished.
					if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) {
						if (OLC_DESC(d)->next) {
							OLC_DESC(d) = OLC_DESC(d)->next;
						} else    // Make new extra description and attach at end.
						{
							ExtraDescription::shared_ptr new_extra(new ExtraDescription());
							OLC_DESC(d)->next = new_extra;
							OLC_DESC(d) = OLC_DESC(d)->next;
						}
					}
					// * No break - drop into default case.

					// fall through
				default: oedit_disp_extradesc_menu(d);
					return;
			}
			break;

		case OEDIT_SKILLS:
			number = atoi(arg);
			if (number == 0) {
				break;
			}
			skill_id = static_cast<ESkill>(number);
			if (MUD::Skills().IsInvalid(skill_id)) {
				SendMsgToChar("Неизвестное умение.\r\n", d->character.get());
			} else if (OLC_OBJ(d)->get_skill(skill_id) != 0) {
				OLC_OBJ(d)->set_skill(skill_id, 0);
			} else if (sscanf(arg, "%d %d", &plane, &bit) < 2) {
				SendMsgToChar("Не указан уровень владения умением.\r\n", d->character.get());
			} else {
				OLC_OBJ(d)->set_skill(skill_id, std::clamp(bit, -200, 200));
			}
			oedit_disp_skills_mod_menu(d);
			return;

		case OEDIT_MORT_REQ: number = atoi(arg);
			OLC_OBJ(d)->set_minimum_remorts(number);
			break;

		case OEDIT_DRINKCON_VALUES:
			switch (number = atoi(arg)) {
				case 0: break;
				case 1: OLC_MODE(d) = OEDIT_POTION_SPELL1_NUM;
					oedit_disp_spells_menu(d);
					return;
				case 2: OLC_MODE(d) = OEDIT_POTION_SPELL2_NUM;
					oedit_disp_spells_menu(d);
					return;
				case 3: OLC_MODE(d) = OEDIT_POTION_SPELL3_NUM;
					oedit_disp_spells_menu(d);
					return;
				default: SendMsgToChar("Неверный выбор.\r\n", d->character.get());
					drinkcon_values_menu(d);
					return;
			}
			break;
		case OEDIT_POTION_SPELL1_NUM: number = atoi(arg);
			if (parse_val_spell_num(d, ObjVal::EValueKey::POTION_SPELL1_NUM, number)) {
				OLC_MODE(d) = OEDIT_POTION_SPELL1_LVL;
			}
			return;
		case OEDIT_POTION_SPELL2_NUM: number = atoi(arg);
			if (parse_val_spell_num(d, ObjVal::EValueKey::POTION_SPELL2_NUM, number)) {
				OLC_MODE(d) = OEDIT_POTION_SPELL2_LVL;
			}
			return;
		case OEDIT_POTION_SPELL3_NUM: number = atoi(arg);
			if (parse_val_spell_num(d, ObjVal::EValueKey::POTION_SPELL3_NUM, number)) {
				OLC_MODE(d) = OEDIT_POTION_SPELL3_LVL;
			}
			return;
		case OEDIT_POTION_SPELL1_LVL: number = atoi(arg);
			parse_val_spell_lvl(d, ObjVal::EValueKey::POTION_SPELL1_LVL, number);
			return;
		case OEDIT_POTION_SPELL2_LVL: number = atoi(arg);
			parse_val_spell_lvl(d, ObjVal::EValueKey::POTION_SPELL2_LVL, number);
			return;
		case OEDIT_POTION_SPELL3_LVL: number = atoi(arg);
			parse_val_spell_lvl(d, ObjVal::EValueKey::POTION_SPELL3_LVL, number);
			return;
		case OEDIT_CLONE:
			switch (*arg) {
				case '1': OLC_MODE(d) = OEDIT_CLONE_WITH_TRIGGERS;
					SendMsgToChar("Введите VNUM объекта для клонирования:", d->character.get());
					return;
				case '2': OLC_MODE(d) = OEDIT_CLONE_WITHOUT_TRIGGERS;
					SendMsgToChar("Введите VNUM объекта для клонирования:", d->character.get());
					return;
				case '3': break;    //to main menu
				default: oedit_disp_clone_menu(d);
					return;
			}
			break;
		case OEDIT_CLONE_WITH_TRIGGERS: {
			number = atoi(arg);
			const int rnum_object = GetObjRnum((OLC_OBJ(d)->get_vnum()));
			if (!OLC_OBJ(d)->clone_olc_object_from_prototype(number)) {
				SendMsgToChar("Нет объекта с таким внумом. Повторите ввод : ", d->character.get());
				return;
			}
			if (rnum_object >= 0)
				OLC_OBJ(d)->set_rnum(rnum_object);
			break;
		}
		case OEDIT_CLONE_WITHOUT_TRIGGERS: {
			number = atoi(arg);

			auto proto_script_old = OLC_OBJ(d)->get_proto_script();
			const int rnum_object = GetObjRnum((OLC_OBJ(d)->get_vnum()));

			if (!OLC_OBJ(d)->clone_olc_object_from_prototype(number)) {
				SendMsgToChar("Нет объекта с таким внумом. Повторите ввод: :", d->character.get());
				return;
			}
			if (rnum_object >= 0)
				OLC_OBJ(d)->set_rnum(rnum_object);
			OLC_OBJ(d)->set_proto_script(proto_script_old);
			break;
		}
		default: mudlog("SYSERR: OLC: Reached default case in oedit_parse()!", BRF, kLvlBuilder, SYSLOG, true);
			SendMsgToChar("Oops...\r\n", d->character.get());
			break;
	}

	// * If we get here, we have changed something.
	OLC_VAL(d) = 1;
	oedit_disp_menu(d);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
