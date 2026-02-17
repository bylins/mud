 /**************************************************************************
* OasisOLC - medit.cpp					Part of Bylins    *
* Copyright 1996 Harvey Gilpin.					  *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
***************************************************************************/

#include "engine/db/world_characters.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "gameplay/magic/spells.h"
#include "engine/db/db.h"
#include "engine/db/world_data_source_manager.h"
#include "olc.h"
#include "engine/core/handler.h"
#include "engine/scripting/dg_olc.h"
#include "gameplay/core/constants.h"
#include "gameplay/crafting/im.h"
#include "engine/entities/char_data.h"
#include "gameplay/skills/skills.h"
#include "gameplay/skills/track.h"
#include "engine/entities/room_data.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/mechanics/dead_load.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/sets_drop.h"
#include "gameplay/fight/fight.h"
#include "engine/entities/zone.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/id_converter.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/magic/spells_info.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/mem_queue.h"
#include "gameplay/ai/spec_procs.h"
#include <sys/stat.h>

//#include <third_party_libs/fmt/include/fmt/format.h>
#include <stack>
#include <sstream>

// * Set this to 1 for debugging logs in medit_save_internally.
#if 0
#define DEBUG
#endif

//-------------------------------------------------------------------

// * External variable declarations.

extern IndexData *mob_index;
extern CharData *mob_proto;
extern MobRnum top_of_mobt;
extern DescriptorData *descriptor_list;
#if defined(OASIS_MPROG)
extern const char *mobprog_types[];
#endif

int planebit(const char *str, int *plane, int *bit);

void clear_mob_charm(CharData *mob);

//-------------------------------------------------------------------

// * Handy internal macros.
#define GET_ALIAS(mob) ((mob)->GetCharAliases().c_str())
#define GET_LDESC(mob) ((mob)->player_data.long_descr)
#define GET_DDESC(mob) ((mob)->player_data.description)
#define GET_ATTACK(mob) ((mob)->mob_specials.attack_type)
#if defined(OASIS_MPROG)
#define GET_MPROG(mob)		(mob_index[(mob)->nr].mobprogs)
#define GET_MPROG_TYPE(mob)	(mob_index[(mob)->nr].progtypes)
#endif

//-------------------------------------------------------------------

// * Function prototypes.
void medit_setup(DescriptorData *d, int rmob_num);

void medit_mobile_init(CharData *mob);
void CopyMobilePrototypeForMedit(CharData *dst, CharData *src, bool partial_copy);
void medit_mobile_free(CharData *mob);

void medit_save_internally(DescriptorData *d);
void medit_save_to_disk(int zone_num);

void medit_parse(DescriptorData *d, char *arg);
void medit_disp_menu(DescriptorData *d);
void medit_disp_positions(DescriptorData *d);
void medit_disp_mob_flags(DescriptorData *d);
void medit_disp_aff_flags(DescriptorData *d);
void medit_disp_attack_types(DescriptorData *d);
void medit_disp_resistances(DescriptorData *d);
void medit_disp_race(DescriptorData *d);

#if defined(OASIS_MPROG)
void medit_disp_mprog(DescriptorData * d);
void medit_change_mprog(DescriptorData * d);
const char *medit_get_mprog_type(struct mob_prog_data *mprog);
#endif

//   Инициализация моба по-умолчанию
void medit_mobile_init(CharData *mob) {
	mob->set_hit(1);
	mob->mem_queue.total = 1;
	mob->set_max_move(100);
	mob->mem_queue.stored = 100;
	GET_NDD(mob) = GET_SDD(mob) = 1;
	GET_GOLD_NoDs(mob) = GET_GOLD_SiDs(mob) = 0;
	GET_WEIGHT(mob) = 200;
	GET_HEIGHT(mob) = 198;
	GET_SIZE(mob) = 30;
	mob->set_class(ECharClass::kNpcBase);
	GET_RACE(mob) = ENpcRace::kBasic;
	GET_MR(mob) = GET_AR(mob) = GET_PR(mob) = 0;

	mob->set_str(11);
	mob->set_dex(11);
	mob->set_int(11);
	mob->set_wis(11);
	mob->set_cha(11);
	mob->set_con(11);

	mob->SetNpcAttribute(true);
	mob->player_specials = player_special_data::s_for_mobiles;

	for (auto i = EResist::kFirstResist; i <= EResist::kLastResist; ++i) {
		GET_RESIST(mob, i) = 0;
	}
}

void CopyMobilePrototypeForMedit(CharData *dst, CharData *src, bool partial_copy)
/*++
   Функция делает создает копию ПРОТОТИПА моба.
   После вызова этой функции создается полностью независимая копия моба src.
   Все поля имеют те же значения, но занимают свои области памяти.
	  dst - "чистый" указатель на структуру char_data.
	  src - исходный моб
   Примечание: Неочищенный указатель dst приведет к утечке памяти.
			   Используйте medit_mobile_free() для очистки содержимого моба

   Т.к. работа идет с прототипом, необходимо отслеживать только следующие поля
   (остальные будут в безопасном состоянии)

   player_data.name
   player_data.short_descr
   player_data.long_descr
   player_data.description
   player_data.PNames[6]
   mob_specials.Questor - странно, т.к. похоже всегда NULL
   скрипт=NULL и прототипы
   помощники
   partial_copy не тронем падежи и некоторые поля
--*/
{
	// сохраняем старые значения
	CharData tmp(*dst);

	// Копирую все поверх
	*dst = *src;
	if (partial_copy) {
		dst->SetCharAliases(tmp.GetCharAliases());
		dst->set_npc_name(tmp.get_npc_name());
		dst->player_data.long_descr = tmp.player_data.long_descr;
		dst->player_data.description = tmp.player_data.description;
		dst->player_data.PNames[ECase::kNom] = tmp.player_data.PNames[ECase::kNom];
		dst->player_data.PNames[ECase::kGen] = tmp.player_data.PNames[ECase::kGen];
		dst->player_data.PNames[ECase::kDat] = tmp.player_data.PNames[ECase::kDat];
		dst->player_data.PNames[ECase::kAcc] = tmp.player_data.PNames[ECase::kAcc];
		dst->player_data.PNames[ECase::kIns] = tmp.player_data.PNames[ECase::kIns];
		dst->player_data.PNames[ECase::kPre] = tmp.player_data.PNames[ECase::kPre];
		if (tmp.mob_specials.dest_count > 0) { // ели был маршрут оставим
			dst->mob_specials.dest_count = tmp.mob_specials.dest_count;
			for (auto plane = 0; plane < tmp.mob_specials.dest_count; plane++) {
				dst->mob_specials.dest[plane] = tmp.mob_specials.dest[plane];
			}
		}
	}
	// Теперь дублирую память

	dst->mob_specials.Questor = (src->mob_specials.Questor && *src->mob_specials.Questor ? str_dup(src->mob_specials.Questor) : nullptr);
	if (partial_copy && !tmp.summon_helpers.empty()) { //если неполное копирование но хелперов нет, копирнем
		dst->summon_helpers.clear();
		std::copy(tmp.summon_helpers.begin(), tmp.summon_helpers.end(), std::back_inserter(dst->summon_helpers));
	}
	else {
		dst->summon_helpers.clear();
		std::copy(src->summon_helpers.begin(), src->summon_helpers.end(), std::back_inserter(dst->summon_helpers));
	}
	// Копирую скрипт и прототипы
	SCRIPT(dst)->cleanup();
	auto proto_script_old = new ObjData::triggers_list_t(*src->proto_script);
	dst->proto_script.reset(proto_script_old);
	//*dst->proto_script = *src->proto_script;
	if (partial_copy && !tmp.dl_list.empty())
		dst->dl_list = tmp.dl_list;
	else
		dst->dl_list = src->dl_list;
}

void medit_mobile_free(CharData *mob)
/*++
	Функция полностью освобождает память, занимаемую данными моба.
	ВНИМАНИЕ. Память самой структуры char_data не освобождается.
			 Необходимо дополнительно использовать free()

	add: вобщем если втыкаете здесь free, то надо обнулять указатель,
	иначе потом при выходе из олц в деструкторе будет повторная попытка
	очистить эту память и соответственно все плохо
	TODO: вообще канеш переделывать все это надо теперь
--*/
{
	int i;

	if (!mob) {
		return;
	}

	i = GET_MOB_RNUM(mob);    // задается в функции medit_setup

	if (i == -1 || mob == &mob_proto[i]) {
		// Нет прототипа или сам прототип, удалять все подряд

		if (mob->mob_specials.Questor) {
			free(mob->mob_specials.Questor);
			mob->mob_specials.Questor = nullptr;
		}
	} else {
		// Есть прототип, удалять несовпадающее

		if (mob->mob_specials.Questor && mob->mob_specials.Questor != mob_proto[i].mob_specials.Questor) {
			free(mob->mob_specials.Questor);
			mob->mob_specials.Questor = nullptr;
		}
	}
	mob->summon_helpers.clear();
}

// ***********************************************************************

void medit_setup(DescriptorData *d, int real_num)
/*++
   Подготовка данных для редактирования моба.
	  d        - OLC дескриптор
	  real_num - RNUM исходного моба, новый -1
--*/
{
	CharData *mob = new CharData;

	medit_mobile_init(mob);

	if (real_num == -1) {
		mob->set_rnum(kNobody);
		mob->SetCharAliases("неоконченный моб");
		mob->set_npc_name("неоконченный моб");
		mob->player_data.long_descr = "Неоконченный моб стоит тут.\r\n";
		mob->player_data.description = "Выглядит достаточно незавершенно.\r\n";

		mob->player_data.PNames[ECase::kNom] = "неоконченный моб";
		mob->player_data.PNames[ECase::kGen] = "неоконченного моба";
		mob->player_data.PNames[ECase::kDat] = "неоконченному мобу";
		mob->player_data.PNames[ECase::kAcc] = "неоконченного моба";
		mob->player_data.PNames[ECase::kIns] = "неоконченным мобом";
		mob->player_data.PNames[ECase::kPre] = "неоконченном мобе";
		mob->mob_specials.Questor = nullptr;
		mob->summon_helpers.clear();
#if defined(OASIS_MPROG)
		OLC_MPROGL(d) = nullptr;
		OLC_MPROG(d) = nullptr;
#endif
	} else {
#if defined(OASIS_MPROG)
		MPROG_DATA *temp;
		MPROG_DATA *head;
#endif

		CopyMobilePrototypeForMedit(mob, &mob_proto[real_num], false);

#if defined(OASIS_MPROG)
		/*
		 * I think there needs to be a brace from the if statement to the #endif
		 * according to the way the original patch was indented.  If this crashes,
		 * try it with the braces and report to greerga@van.ml.org on if that works.
		 */
		if (GET_MPROG(mob))
			CREATE(OLC_MPROGL(d), 1);
		head = OLC_MPROGL(d);
		for (temp = GET_MPROG(mob); temp; temp = temp->next)
		{
			OLC_MPROGL(d)->type = temp->type;
			OLC_MPROGL(d)->arglist = str_dup(temp->arglist);
			OLC_MPROGL(d)->comlist = str_dup(temp->comlist);
			if (temp->next)
			{
				CREATE(OLC_MPROGL(d)->next, 1);
				OLC_MPROGL(d) = OLC_MPROGL(d)->next;
			}
		}
		OLC_MPROGL(d) = head;
		OLC_MPROG(d) = OLC_MPROGL(d);
#endif

	}

	OLC_MOB(d) = mob;
	OLC_ITEM_TYPE(d) = MOB_TRIGGER;
	medit_disp_menu(d);
	OLC_VAL(d) = 0;        // Has changed flag. (It hasn't so far, we just made it.)
}

//-------------------------------------------------------------------

#define ZCMD zone_table[zone].cmd[cmd_no]

/*
* Save new/edited mob to memory.
*
* Здесь сейчас нельзя просто копировать через указатель поля из моба, т.к. при выходе
* они будут чиститься через деструктор, поэтому пока только через CopyMobilePrototypeForMedit
* add: прямое копирование без переаллокаций при добавлении нового моба работает
* только потому, что в деструкторе сейчас не очищаются аллокации прототипов.
* TODO: ес-сно это муть все
*/
void medit_save_internally(DescriptorData *d) {
	int rmob_num, found = 0, new_mob_num = 0, cmd_no, j;
	CharData *new_proto;
	IndexData *new_index;
	DescriptorData *dsc;

	//  rmob_num = GetMobRnum(OLC_NUM(d));
	rmob_num = GET_MOB_RNUM(OLC_MOB(d));
	//	set_test_data(OLC_MOB(d));

	if (rmob_num >= 0) {
		/*
		Собственно полная суть бага:
		Лоадим моба - его падежи указывают на адрес X в структуре mob_proto. Убиваю его.
		Дальше, я меняю моба, в прототипе падежи будут указывать уже на другой адрес.
		Дальше на тике происходит окончательный пурж моба, которого я убил - но так там падежи указывают на старый адрес - то происходит беда
		поэтому сразу чистим
		*/
		character_list.purge();
		// Такой моб есть. Обновляю прототип.
		log("[MEdit] Save mob to mem %d", rmob_num);
		// Удаление старого прототипа
		medit_mobile_free(&mob_proto[rmob_num]);
		// Обновляю прототип
		CopyMobilePrototypeForMedit(&mob_proto[rmob_num], OLC_MOB(d), false);
		// Теперь просто удалить OLC_MOB(d) и все будет хорошо
		medit_mobile_free(OLC_MOB(d));
		// Удаление "оболочки" произойдет в olc_cleanup

		if (mob_index[rmob_num].func == receptionist) {
			clear_mob_charm(&mob_proto[rmob_num]);
		}

		// В живых мобах необходимо обновить строки, иначе будут крэши
		for (const auto &live_mob : character_list) {
			if (IS_MOB(live_mob) && GET_MOB_RNUM(live_mob) == rmob_num) {
				live_mob->SetCharAliases((mob_proto + rmob_num)->GetCharAliases().c_str());
				live_mob->set_npc_name((mob_proto + rmob_num)->get_npc_name().c_str());
				// Только строки. Остальное после ресета/ребута
				// Возможна небольшая утечка памяти, но иначе очень большая запара
				GET_LDESC(live_mob) = GET_LDESC(mob_proto + rmob_num);
				GET_DDESC(live_mob) = GET_DDESC(mob_proto + rmob_num);
				for (j = ECase::kFirstCase; j <= ECase::kLastCase; j++) {
					live_mob->player_data.PNames[j] = mob_proto[rmob_num].player_data.PNames[j];
				}
				live_mob->summon_helpers.clear();
				std::copy((mob_proto + rmob_num)->summon_helpers.begin(), (mob_proto + rmob_num)->summon_helpers.end(), std::back_inserter(live_mob->summon_helpers));
				live_mob->mob_specials.Questor = (mob_proto + rmob_num)->mob_specials.Questor;
				// Скрипты и прототипы остаются от старого моба
			}
		}
	} else {
		// Совершенно новый моб
#if defined(DEBUG)
		fprintf(stderr, "top_of_mobt: %d, new top_of_mobt: %d\n", top_of_mobt, top_of_mobt + 1);
#endif

		new_proto = new CharData[top_of_mobt + 2];
		CREATE(new_index, top_of_mobt + 2);

		for (rmob_num = 0; rmob_num <= top_of_mobt; rmob_num++) {
			if (!found)    // Is this the place?
			{
				if (mob_index[rmob_num].vnum > OLC_NUM(d))    // Yep, stick it here.
				{
					found = true;
#if defined(DEBUG)
					fprintf(stderr, "Inserted: rmob_num: %d\n", rmob_num);
#endif
					new_index[rmob_num].vnum = OLC_NUM(d);
					new_index[rmob_num].total_online = 0;
					new_index[rmob_num].func = nullptr;
					new_mob_num = rmob_num;
					OLC_MOB(d)->set_rnum(rmob_num);
					CopyMobilePrototypeForMedit(&new_proto[rmob_num], OLC_MOB(d), false);
					//					new_proto[rmob_num] = *(OLC_MOB(d));
					new_index[rmob_num].zone = get_zone_rnum_by_mob_vnum(OLC_NUM(d));
					new_index[rmob_num].set_idx = -1;
					--rmob_num;
					continue;
				} else    // Nope, copy over as normal.
				{
					new_index[rmob_num] = mob_index[rmob_num];
					new_proto[rmob_num] = mob_proto[rmob_num];
				}
			} else    // We've already found it, copy the rest over.
			{
				new_index[rmob_num + 1] = mob_index[rmob_num];
				new_proto[rmob_num + 1] = mob_proto[rmob_num];
				new_proto[rmob_num + 1].set_rnum(rmob_num + 1);
			}
		}
		if (!found)    // Still not found, must add it to the top of the table.
		{
			new_index[rmob_num].vnum = OLC_NUM(d);
			new_index[rmob_num].total_online = 0;
			new_index[rmob_num].func = nullptr;
			new_mob_num = rmob_num;
			OLC_MOB(d)->set_rnum(rmob_num);

			CopyMobilePrototypeForMedit(&new_proto[rmob_num], OLC_MOB(d), false);

			new_index[rmob_num].zone = get_zone_rnum_by_mob_vnum(OLC_NUM(d));
			new_index[rmob_num].set_idx = -1;
		}

		// Replace tables.
		delete[] mob_proto;
		free(mob_index);

		mob_index = new_index;
		mob_proto = new_proto;
		top_of_mobt++;
		zone_table[OLC_ZNUM(d)].RnumMobsLocation.second++;

#if defined(DEBUG)
		fprintf(stderr, "Free ok.\n");
#endif

		// Оновная проблема - перенумерация всех существующих RNUM

		// 1. Увеличение RNUM существующих в мире мобов
		// Update live mobile rnums. //
		// new_mob_num - индекс, куда вставлен новый моб //
		// Для всех существующих мобов с RNUM>=new_mob_num нужно увеличить RNUM //
		for (const auto &live_mob : character_list) {
			if (GET_MOB_RNUM(live_mob) >= new_mob_num) {
				live_mob->set_rnum(1 + live_mob->get_rnum());
			}
		}

		// 2. Изменение параметров команд zon-файлов
		// Update zone table. //
		for (auto zone = 0u; zone < zone_table.size(); zone++) {
			if (!zone_table[zone].cmd)
				continue;
			for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
				if (ZCMD.command == 'M' || ZCMD.command == 'Q') {
					if (ZCMD.arg1 >= new_mob_num)
						ZCMD.arg1++;
				} else if (ZCMD.command == 'F') {
					if (ZCMD.arg2 >= new_mob_num)
						ZCMD.arg2++;
					if (ZCMD.arg3 >= new_mob_num)
						ZCMD.arg3++;
				}
		}
		// 4. Другие редактируемые мобы
		// * Update keepers in shops being edited and other mobs being edited.
		for (dsc = descriptor_list; dsc; dsc = dsc->next) {
			if (dsc->state == EConState::kMedit) {
				if (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_mob_num) {
					OLC_MOB(dsc)->set_rnum(1 + OLC_MOB(dsc)->get_rnum());
				}
			}
		}

		// 5. Информация о выслеживании
		for (j = kFirstRoom; j <= top_of_world; j++) {
			struct TrackData *track;

			for (track = world[j]->track; track; track = track->next) {
				if (IS_SET(track->track_info, TRACK_NPC) && track->who >= new_mob_num) {
					track->who++;
				}
			}
		}

		SetsDrop::renumber_obj_rnum(new_mob_num);
	}            // совершенно новый моб

#if defined(OASIS_MPROG)
	GET_MPROG(OLC_MOB(d)) = OLC_MPROGL(d);
	GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
	while (OLC_MPROGL(d))
	{
		GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
		OLC_MPROGL(d) = OLC_MPROGL(d)->next;
	}
#endif

//	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].vnum, OLC_SAVE_MOB);
	// Save only this specific mob (vnum = OLC_NUM(d))
	auto* data_source = world_loader::WorldDataSourceManager::Instance().GetDataSource();
	data_source->SaveMobs(OLC_ZNUM(d), OLC_NUM(d));
}

//-------------------------------------------------------------------

/*
 * Save ALL mobiles for a zone to their .mob file, mobs are all
 * saved in Extended format, regardless of whether they have any
 * extended fields.  Thanks to Sammy for ideas on this bit of code.
 */
void medit_save_to_disk(ZoneRnum zone_num) {
	int j, c, rmob_num, sum;
	FILE *mob_file;
	char fname[64];
	CharData *mob;

	ZoneVnum zone = zone_table[zone_num].vnum;
	MobVnum top = zone_table[zone_num].top;
	if (zone >= dungeons::kZoneStartDungeons) {
			sprintf(buf, "Отказ сохранения зоны %d на диск.", zone);
			mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
			return;
	}
	sprintf(fname, "%s/%d.new", MOB_PREFIX, zone);
	if (!(mob_file = fopen(fname, "w"))) {
		mudlog("SYSERR: OLC: Cannot open mob file!", BRF, kLvlBuilder, SYSLOG, true);
		return;
	}

	// * Seach the database for mobs in this zone and save them.
	for (MobVnum i = zone * 100; i <= top; i++) {
		if ((rmob_num = GetMobRnum(i)) == -1)
			continue;
		if (fprintf(mob_file, "#%d\n", mob_index[rmob_num].vnum) < 0) {
			mudlog("SYSERR: OLC: Cannot write mob file!\r\n", BRF, kLvlBuilder, SYSLOG, true);
			fclose(mob_file);
			return;
		}
		mob = (mob_proto + rmob_num);
			// * Clean up strings.
		if (mob->player_data.long_descr.empty())
			mob->player_data.long_descr = "неопределен";
		strcpy(buf1, mob->player_data.long_descr.c_str());
		strip_string(buf1);
		if (mob->player_data.description.empty())
			mob->player_data.description = "неопределен";
		strcpy(buf2, mob->player_data.description.c_str());
		strip_string(buf2);
		fprintf(mob_file, "%s~\n" "%s~\n" "%s~\n" "%s~\n" "%s~\n" "%s~\n" "%s~\n" "%s~\n" "%s~\n",
				not_null(GET_ALIAS(mob), "неопределен"),
				not_null(GET_PAD(mob, 0), "кто"),
				not_null(GET_PAD(mob, 1), "кого"),
				not_null(GET_PAD(mob, 2), "кому"),
				not_null(GET_PAD(mob, 3), "кого"),
				not_null(GET_PAD(mob, 4), "кем"),
				not_null(GET_PAD(mob, 5), "о ком"), buf1, buf2);
		if (mob->mob_specials.Questor)
			strcpy(buf1, mob->mob_specials.Questor);
		else
			strcpy(buf1, "");
		strip_string(buf1);
		*buf2 = 0;
		mob->PrintFlagsToAscii(buf2);
		AFF_FLAGS(mob).tascii(FlagData::kPlanesNumber, buf2);
		fprintf(mob_file, "%s%d E\n" "%d %d %d %dd%d+%d %dd%d+%d\n" "%dd%d+%ld %ld\n" "%d %d %d\n",
				buf2, GET_ALIGNMENT(mob),
				GetRealLevel(mob), 20 - GET_HR(mob), GET_AC(mob) / 10, mob->mem_queue.total,
				mob->mem_queue.stored, mob->get_hit(), GET_NDD(mob), GET_SDD(mob), GET_DR(mob), GET_GOLD_NoDs(mob),
				GET_GOLD_SiDs(mob), mob->get_gold(), mob->get_exp(), static_cast<int>(mob->GetPosition()),
				static_cast<int>(GET_DEFAULT_POS(mob)), static_cast<int>(mob->get_sex()));
		// * Deal with Extra stats in case they are there.
		sum = 0;
		for (ESaving save = ESaving::kFirst; save <= ESaving::kLast; ++save) {
			sum += GetSave(mob, save);
		}
		if (sum != 0)
			fprintf(mob_file, "Saves: %d %d %d %d\n",
					GetSave(mob, ESaving::kWill), GetSave(mob, ESaving::kCritical),
					GetSave(mob, ESaving::kStability), GetSave(mob, ESaving::kReflex));
		sum = 0;
		fprintf(mob_file, "Resistances: %d %d %d %d %d %d %d %d\n",
				GET_RESIST(mob, 0), GET_RESIST(mob, 1), GET_RESIST(mob, 2), GET_RESIST(mob, 3),
				GET_RESIST(mob, 4), GET_RESIST(mob, 5), GET_RESIST(mob, 6), GET_RESIST(mob, 7));
		if (mob->get_hitreg() != 0)
			fprintf(mob_file, "HPreg: %d\n", mob->get_hitreg());
		if (GET_ARMOUR(mob) != 0)
			fprintf(mob_file, "Armour: %d\n", GET_ARMOUR(mob));
		if (GET_MANAREG(mob) != 0)
			fprintf(mob_file, "PlusMem: %d\n", GET_MANAREG(mob));
		if (GET_CAST_SUCCESS(mob) != 0)
			fprintf(mob_file, "CastSuccess: %d\n", GET_CAST_SUCCESS(mob));
		if (GET_MORALE(mob) != 0)
			fprintf(mob_file, "Success: %d\n", GET_MORALE(mob));
		if (GET_INITIATIVE(mob) != 0)
			fprintf(mob_file, "Initiative: %d\n", GET_INITIATIVE(mob));
		if (GET_ABSORBE(mob) != 0)
			fprintf(mob_file, "Absorbe: %d\n", GET_ABSORBE(mob));
		if (GET_AR(mob) != 0)
			fprintf(mob_file, "AResist: %d\n", GET_AR(mob));
		if (GET_MR(mob) != 0)
			fprintf(mob_file, "MResist: %d\n", GET_MR(mob));
		// added by WorM (Видолюб) поглощение физ.урона в %
		if (GET_PR(mob) != 0)
			fprintf(mob_file, "PResist: %d\n", GET_PR(mob));
		// end by WorM
		if (GET_ATTACK(mob) != 0)
			fprintf(mob_file, "BareHandAttack: %d\n", GET_ATTACK(mob));
		for (c = 0; c < mob->mob_specials.dest_count; c++)
			fprintf(mob_file, "Destination: %d\n", mob->mob_specials.dest[c]);
		if (mob->get_str() != 11)
			fprintf(mob_file, "Str: %d\n", mob->get_str());
		if (mob->get_dex() != 11)
			fprintf(mob_file, "Dex: %d\n", mob->get_dex());
		if (mob->get_int() != 11)
			fprintf(mob_file, "Int: %d\n", mob->get_int());
		if (mob->get_wis() != 11)
			fprintf(mob_file, "Wis: %d\n", mob->get_wis());
		if (mob->get_con() != 11)
			fprintf(mob_file, "Con: %d\n", mob->get_con());
		if (mob->get_cha() != 11)
			fprintf(mob_file, "Cha: %d\n", mob->get_cha());
		if (GET_SIZE(mob))
			fprintf(mob_file, "Size: %d\n", GET_SIZE(mob));
		if (mob->mob_specials.like_work)
			fprintf(mob_file, "LikeWork: %d\n", mob->mob_specials.like_work);
		if (mob->mob_specials.MaxFactor)
			fprintf(mob_file, "MaxFactor: %d\n", mob->mob_specials.MaxFactor);
		if (mob->mob_specials.extra_attack)
			fprintf(mob_file, "ExtraAttack: %d\n", mob->mob_specials.extra_attack);
		if (mob->get_remort())
			fprintf(mob_file, "MobRemort: %d\n", mob->get_remort());
		if (GET_RACE(mob))
			fprintf(mob_file, "Race: %d\n", GET_RACE(mob));
		if (GET_HEIGHT(mob))
			fprintf(mob_file, "Height: %d\n", GET_HEIGHT(mob));
		if (GET_WEIGHT(mob))
			fprintf(mob_file, "Weight: %d\n", GET_WEIGHT(mob));
		strcpy(buf1, "Special_Bitvector: ");
		NPC_FLAGS(mob).tascii(FlagData::kPlanesNumber, buf1);
		fprintf(mob_file, "%s\n", buf1);
		for (const auto &feat : MUD::Feats()) {
			if (mob->HaveFeat(feat.GetId())) {
				fprintf(mob_file, "Feat: %d\n", to_underlying(feat.GetId()));
			}
		}
		for (const auto &skill : MUD::Skills()) {
			if (mob->GetSkill(skill.GetId()) && skill.IsValid()) {
				fprintf(mob_file, "Skill: %d %d\n", to_underlying(skill.GetId()), mob->GetSkill(skill.GetId()));
			}
		}
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			for (j = 1; j <= GET_SPELL_MEM(mob, spell_id); j++) {
				fprintf(mob_file, "Spell: %d\n", to_underlying(spell_id));
			}
		}
		for (auto helper : mob->summon_helpers) {
			fprintf(mob_file, "Helper: %d\n", helper);
		}
		if (mob->get_role_bits().any()) {
			std::string tmp = to_string(mob->get_role_bits());
			fprintf(mob_file, "Role: %s\n", tmp.c_str());
		}
		// * XXX: Add E-mob handlers here.
		fprintf(mob_file, "E\n");
		script_save_to_disk(mob_file, mob, MOB_TRIGGER);
		// Сохраняем список в файл
		for (auto p : mob->dl_list) {
			fprintf(mob_file, "L %d %d %d %d\n", p.obj_vnum, p.load_prob, p.load_type, p.spec_param);
		}
	}
	fprintf(mob_file, "$\n");
	fclose(mob_file);
	sprintf(buf2, "%s/%d.mob", MOB_PREFIX, zone);
	// * We're fubar'd if we crash between the two lines below.
	remove(buf2);
	rename(fname, buf2);
#ifndef _WIN32
	if (chmod(buf2, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0) {
		std::stringstream ss;
		ss << "Error chmod file: " << buf2 << " (" << __FILE__ << " "<< __func__ << "  "<< __LINE__ << ")";
		mudlog(ss.str(), BRF, kLvlGod, SYSLOG, true);
	}
#endif
	olc_remove_from_save_list(zone_table[zone_num].vnum, OLC_SAVE_MOB);
}

// **************************************************************************
// * Menu functions                                                         *
// **************************************************************************

// * Display positions. (sitting, standing, etc)
void medit_disp_positions(DescriptorData *d) {
	int i;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (i = 0; *position_types[i] != '\n'; i++) {
		sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("Выберите положение : ", d->character.get());
}

void medit_disp_add_parameters(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	sprintf(buf,
			"%s1%s ) Регенерация : %s%d%s\r\n"
			"%s2%s ) Броня : %s%d%s\r\n"
			"%s3%s ) Запоминание : %s%d%s\r\n"
			"%s4%s ) Успех колдовства : %s%d%s\r\n"
			"%s5%s ) Удача : %s%d%s\r\n"
			"%s6%s ) Инициатива : %s%d%s\r\n"
			"%s7%s ) Поглощение : %s%d%s\r\n"
			"%s8%s ) Иммунитет к магическим аффектам : %s%d%s\r\n"
			"%s9%s ) Иммунитет к магическим повреждениям : %s%d%s\r\n"
			"%s10%s) Иммунитет к физическим повреждениям : %s%d%s\r\n",
			grn, nrm, cyn, (OLC_MOB(d)->get_hitreg()), nrm,
			grn, nrm, cyn, GET_ARMOUR((OLC_MOB(d))), nrm,
			grn, nrm, cyn, GET_MANAREG((OLC_MOB(d))), nrm,
			grn, nrm, cyn, GET_CAST_SUCCESS((OLC_MOB(d))), nrm,
			grn, nrm, cyn, GET_MORALE((OLC_MOB(d))), nrm,
			grn, nrm, cyn, GET_INITIATIVE((OLC_MOB(d))), nrm,
			grn, nrm, cyn, GET_ABSORBE((OLC_MOB(d))), nrm,
			grn, nrm, cyn, GET_AR((OLC_MOB(d))), nrm,
			grn, nrm, cyn, GET_MR((OLC_MOB(d))), nrm,
			grn, nrm, cyn, GET_PR((OLC_MOB(d))), nrm);
	SendMsgToChar(buf, d->character.get());
	SendMsgToChar("Введите номер и величину параметра (0 - конец) : ", d->character.get());
}

void medit_disp_resistances(DescriptorData *d) {
	int i;

#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (i = 0; *resistance_types[i] != '\n'; i++) {
		sprintf(buf, "%s%2d%s) %s : %s%d%s\r\n",
				grn, i + 1, nrm, resistance_types[i], cyn, GET_RESIST(OLC_MOB(d), i), nrm);
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("Введите номер и величину сопротивления (-100..100%) (0 - конец) : ", d->character.get());
}

// *  Display saves
void medit_disp_saves(DescriptorData *d) {
	int num = 1;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (auto i: apply_negative) {
		if (i.savetype != ESaving::kNone) {
			sprintf(buf, "%s%2d%s) %s : %s%d%s\r\n",
					grn, num++, nrm, i.name.c_str(), cyn, GetSave(OLC_MOB(d), i.savetype), nrm);
			SendMsgToChar(buf, d->character.get());
		}
	}
	SendMsgToChar("Введите номер и величину спас-броска, отрицательное улучшает (0 - конец) : ", d->character.get());
}

#if defined(OASIS_MPROG)
// * Get the type of MobProg.
const char *medit_get_mprog_type(struct mob_prog_data *mprog)
{
	switch (mprog->type)
	{
	case IN_FILE_PROG:
		return ">in_file_prog";
	case ACT_PROG:
		return ">act_prog";
	case SPEECH_PROG:
		return ">speech_prog";
	case RAND_PROG:
		return ">rand_prog";
	case FIGHT_PROG:
		return ">fight_prog";
	case HITPRCNT_PROG:
		return ">hitprcnt_prog";
	case DEATH_PROG:
		return ">death_prog";
	case ENTRY_PROG:
		return ">entry_prog";
	case GREET_PROG:
		return ">greet_prog";
	case ALL_GREET_PROG:
		return ">all_greet_prog";
	case GIVE_PROG:
		return ">give_prog";
	case BRIBE_PROG:
		return ">bribe_prog";
	}
	return ">ERROR_PROG";
}

// * Display the MobProgs.
void medit_disp_mprog(DescriptorData * d)
{
	struct mob_prog_data *mprog = OLC_MPROGL(d);

	OLC_MTOTAL(d) = 1;

#if defined(CLEAR_SCREEN)
	SendMsgToChar("^[[H^[[J", d->character);
#endif
	while (mprog)
	{
		sprintf(buf, "%d) %s %s\r\n", OLC_MTOTAL(d),
			medit_get_mprog_type(mprog), (mprog->arglist ? mprog->arglist : "NONE"));
		SendMsgToChar(buf, d->character);
		OLC_MTOTAL(d)++;
		mprog = mprog->next;
	}
	sprintf(buf,
		"%d) Создать новую Mob Prog\r\n"
		"%d) Очистить Mob Prog\r\n"
		"Введите номер для редактирования [0 - выход]:  ", OLC_MTOTAL(d), OLC_MTOTAL(d) + 1);
	SendMsgToChar(buf, d->character);
	OLC_MODE(d) = MEDIT_MPROG;
}

//-------------------------------------------------------------------

// * Change the MobProgs.
void medit_change_mprog(DescriptorData * d)
{
#if defined(CLEAR_SCREEN)
	SendMsgToChar("^[[H^[[J", d->character);
#endif
	sprintf(buf,
		"1) Type: %s\r\n"
		"2) Args: %s\r\n"
		"3) Commands:\r\n%s\r\n\r\n"
		"Введите номер для редактирования [0 - выход]: ",
		medit_get_mprog_type(OLC_MPROG(d)),
		(OLC_MPROG(d)->arglist ? OLC_MPROG(d)->arglist : "NONE"),
		(OLC_MPROG(d)->comlist ? OLC_MPROG(d)->comlist : "NONE"));

	SendMsgToChar(buf, d->character);
	OLC_MODE(d) = MEDIT_CHANGE_MPROG;
}

//-------------------------------------------------------------------

// * Change the MobProg type.
void medit_disp_mprog_types(DescriptorData * d)
{
	int i;

#if defined(CLEAR_SCREEN)
	SendMsgToChar("^[[H^[[J", d->character);
#endif

	for (i = 0; i < NUM_PROGS - 1; i++)
	{
		sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, mobprog_types[i]);
		SendMsgToChar(buf, d->character);
	}
	SendMsgToChar("Введите тип mob prog : ", d->character);
	OLC_MODE(d) = MEDIT_MPROG_TYPE;
}
#endif

//-------------------------------------------------------------------

// * Display the gender of the mobile.
void medit_disp_sex(DescriptorData *d) {
	int i;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (i = 0; i <= NUM_GENDERS; i++) {
		sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("Выберите пол : ", d->character.get());
}

void medit_disp_role(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif

	std::string out;
	char tmp[kMaxInputLength];
	auto &bits = OLC_MOB(d)->get_role_bits();

	for (unsigned i = 0; i < bits.size(); ++i) {
		snprintf(tmp, sizeof(tmp), "%s%2u%s) %s\r\n",
				 grn, i + 1, nrm,
				 i < npc_role_types.size() ? npc_role_types.at(i) : "UNDEF");
		out += tmp;
	}

	snprintf(tmp, sizeof(tmp), "Текущие флаги : %s", cyn);
	out += tmp;

	print_bitset(bits, npc_role_types, ",", out, true);

	snprintf(tmp, sizeof(tmp), "%s\r\nВыберите роли моба (0 - выход) : ", nrm);
	out += tmp;

	SendMsgToChar(out, d->character.get());
}

void medit_disp_features(DescriptorData *d) {
	int columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif

	for (const auto &feat : MUD::Feats()) {
		if (feat.IsInvalid()) {
			continue;
		}

		if (OLC_MOB(d)->HaveFeat(feat.GetId())) {
			sprintf(buf1, " %s[%s*%s]%s ", cyn, grn, cyn, nrm);
		} else {
			strcpy(buf1, "     ");
		}

		snprintf(buf, kMaxStringLength, "%s%3d%s) %25s%s%s", grn, to_underlying(feat.GetId()), nrm,
				 feat.GetCName(), buf1, !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}

	SendMsgToChar("\r\nУкажите номер способности. (0 - конец) : ", d->character.get());
}

void medit_disp_race(DescriptorData *d) {
	int i;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (i = 0; i < ENpcRace::kLastNpcRace - ENpcRace::kBasic + 1; i++) {
		sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, npc_race_types[i]);
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("Выберите расу моба : ", d->character.get());
}

// * Display attack types menu.
void medit_disp_attack_types(DescriptorData *d) {
	int i;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (i = 0; i < NUM_ATTACK_TYPES; i++) {
		sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, attack_hit_text[i].singular);
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("Выберите тип удара : ", d->character.get());
}

//-------------------------------------------------------------------
void medit_disp_helpers(DescriptorData *d) {
	int columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	SendMsgToChar("Установлены мобы-помощники :\r\n", d->character.get());
	for (auto helper : OLC_MOB(d)->summon_helpers) {
		sprintf(buf, "%s%6d%s %s", grn, helper, nrm, !(++columns % 6) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	if (!columns) {
		sprintf(buf, "%sНЕТ%s", cyn, nrm);
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("\r\nУкажите vnum моба-помощника (0 - конец) : ", d->character.get());
}

void medit_disp_skills(DescriptorData *d) {
	int columns = 0;
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (const auto &skill : MUD::Skills()) {
		if (skill.IsInvalid()) {
			continue;
		}

		if (OLC_MOB(d)->GetSkill(skill.GetId())) {
			sprintf(buf1, "%s[%3d]%s", cyn, OLC_MOB(d)->GetSkill(skill.GetId()), nrm);
		} else {
			strcpy(buf1, "     ");
		}

		snprintf(buf, kMaxStringLength, "%s%3d%s) %25s%s%s", grn, to_underlying(skill.GetId()), nrm,
				 skill.GetName(), buf1, !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("\r\nУкажите номер и уровень владения умением (0 - конец) : ", d->character.get());
}

void medit_disp_spells(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	int columns = 0;
	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		if (MUD::Spell(spell_id).IsUnavailable()) {
			continue;
		}
		if (GET_SPELL_MEM(OLC_MOB(d), spell_id)) {
			sprintf(buf1, "%s[%3d]%s", cyn, GET_SPELL_MEM(OLC_MOB(d), spell_id), nrm);
		} else {
			strcpy(buf1, "     ");
		}
		snprintf(buf, kMaxStringLength, "%s%3d%s) %25s%s%s", grn, to_underlying(spell_id), nrm,
				 MUD::Spell(spell_id).GetCName(), buf1, !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("\r\nУкажите номер и количество заклинаний (0 - конец) : ", d->character.get());
}

// * Display mob-flags menu.
void medit_disp_mob_flags(DescriptorData *d) {
	disp_planes_values(d, action_bits, 2);
	OLC_MOB(d)->char_specials.saved.act.sprintbits(action_bits, buf1, ",", 5);
	snprintf(buf, kMaxStringLength, "\r\nТекущие флаги : %s%s%s\r\nВыберите флаг (0 - выход) : ", cyn, buf1, nrm);
	SendMsgToChar(buf, d->character.get());
}

void medit_disp_npc_flags(DescriptorData *d) {
	disp_planes_values(d, function_bits, 2);
	OLC_MOB(d)->mob_specials.npc_flags.sprintbits(function_bits, buf1, ",", 5);
	snprintf(buf, kMaxStringLength, "\r\nТекущие флаги : %s%s%s\r\nВыберите флаг (0 - выход) : ", cyn, buf1, nrm);
	SendMsgToChar(buf, d->character.get());
}

// * Display affection flags menu.
void medit_disp_aff_flags(DescriptorData *d) {
	disp_planes_values(d, affected_bits, 2);
	OLC_MOB(d)->char_specials.saved.affected_by.sprintbits(affected_bits, buf1, ",", 5);
	snprintf(buf, kMaxStringLength, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ", cyn, buf1, nrm);
	SendMsgToChar(buf, d->character.get());
}

// * Display main menu.
void medit_disp_menu(DescriptorData *d) {
	int i;
	CharData *mob;

	mob = OLC_MOB(d);
	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
			"-- МОБ:  [%s%d%s]\r\n"
			"%s1%s) Пол: %s%-7.7s%s\r\n"
			"%s2%s) Синонимы: %s&S%s&s\r\n"
			"%s3&n) Именительный (это кто)         : %s&e\r\n"
			"%s4&n) Родительный (нет кого)         : %s&e\r\n"
			"%s5&n) Дательный  (дать кому)         : %s&e\r\n"
			"%s6&n) Винительный (ударить кого)     : %s&e\r\n"
			"%s7&n) Творительный (сражаться с кем) : %s&e\r\n"
			"%s8&n) Предложный (ехать на ком)      : %s&e\r\n"
			"%s9&n) Короткое :-\r\n&R&q%s&e&Q"
			"%sA&n) Полное (при осмотреть):-\r\n%s&e"
			"%sB%s) Уровень     : [%s%4d%s],%sC%s) Наклонности : [%s%4d%s]\r\n"
			"%sD%s) Попадание   : [%s%4d%s],%sE%s) Повреждение : [%s%4d%s]\r\n"
			"%sF%s) NumDamDice  : [%s%4d%s],%sG%s) SizeDamDice : [%s%4d%s]\r\n"
			"%sH%s) NumHPDice   : [%s%4d%s],%sI%s) SizeHPDice  : [%s%4d%s],%sJ%s) Доп. Жизнь: [%s%5d%s]\r\n"
			"%sK%s) Класс защиты: [%s%4d%s],%sL%s) Опыт        : [%s%9ld%s],\r\n"
			"%sM%s) Куны        : [%s%4ld%s],%sN%s) NumGoldDice : [%s%4d%s],%sO%s) SizeGoldDice: [%s%4d%s]\r\n",
			cyn, OLC_NUM(d), nrm,
			grn, nrm, yel, genders[(int) mob->get_sex()], nrm,
			grn, nrm, yel, GET_ALIAS(mob),
			grn, GET_PAD(mob, 0),
			grn, GET_PAD(mob, 1),
			grn, GET_PAD(mob, 2),
			grn, GET_PAD(mob, 3),
			grn, GET_PAD(mob, 4),
			grn, GET_PAD(mob, 5),
			grn, GET_LDESC(mob).c_str(),
			grn, GET_DDESC(mob).c_str(),
			grn, nrm, cyn, mob->GetLevel(), nrm,
			grn, nrm, cyn, GET_ALIGNMENT(mob), nrm,
			grn, nrm, cyn, GET_HR(mob), nrm,
			grn, nrm, cyn, GET_DR(mob), nrm,
			grn, nrm, cyn, GET_NDD(mob), nrm,
			grn, nrm, cyn, GET_SDD(mob), nrm,
			grn, nrm, cyn, mob->mem_queue.total, nrm,
			grn, nrm, cyn, mob->mem_queue.stored, nrm,
			grn, nrm, cyn, mob->get_hit(), nrm,
			grn, nrm, cyn, GET_AC(mob), nrm,
			grn, nrm, cyn, mob->get_exp(), nrm,
			grn, nrm, cyn, mob->get_gold(), nrm,
			grn, nrm, cyn, GET_GOLD_NoDs(mob), nrm, grn, nrm, cyn, GET_GOLD_SiDs(mob), nrm);
	SendMsgToChar(buf, d->character.get());

	mob->char_specials.saved.act.sprintbits(action_bits, buf1, ",", 4);
	mob->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, ",", 4);
	snprintf(buf, kMaxStringLength,
			 "%sP%s) Положение     : %s%s\r\n"
			 "%sR%s) По умолчанию  : %s%s\r\n"
			 "%sT%s) Тип атаки     : %s%s\r\n"
			 "%sU%s) Флаги   (MOB) : %s%s\r\n"
			 "%sV%s) Аффекты (AFF) : %s%s\r\n",
			 grn, nrm, yel, position_types[(int) mob->GetPosition()],
			 grn, nrm, yel, position_types[(int) GET_DEFAULT_POS(mob)],
			 grn, nrm, yel, attack_hit_text[GET_ATTACK(mob)].singular, grn, nrm, cyn, buf1, grn, nrm, cyn, buf2);
	SendMsgToChar(buf, d->character.get());

	mob->mob_specials.npc_flags.sprintbits(function_bits, buf1, ",", 4);
	*buf2 = '\0';
	if (GET_DEST(mob) == kNowhere) {
		strcpy(buf2, "-1,");
	} else {
		for (i = 0; i < mob->mob_specials.dest_count; i++) {
			sprintf(buf2 + strlen(buf2), "%d,", mob->mob_specials.dest[i]);
		}
	}
	*(buf2 + strlen(buf2) - 1) = '\0';

	std::string roles_str;
	if (mob->get_role_bits().any()) {
		print_bitset(mob->get_role_bits(), npc_role_types, ",", roles_str);
	} else {
		roles_str += "нет";
	}

	snprintf(buf, kMaxStringLength, "%sW%s) Флаги   (NPC) : %s%s\r\n"
									 "%sY%s) Маршрут движения: %s%s\r\n"
									 "%sZ%s) Помогают   : %s%s\r\n"
									 "%sА%s) Умения     : \r\n"
									 "%sБ%s) Заклинания : \r\n"
									 "%sВ%s) Сила : [%s%4d%s],%sГ%s) Ловк : [%s%4d%s],%sД%s) Тело : [%s%4d%s]\r\n"
									 "%sЕ%s) Мудр : [%s%4d%s],%sЖ%s) Ум   : [%s%4d%s],%sЗ%s) Обая : [%s%4d%s]\r\n"
									 "%sИ%s) Рост : [%s%4d%s],%sК%s) Вес  : [%s%4d%s],%sЛ%s) Разм : [%s%4d%s]\r\n"
									 "%sМ%s) Дополнительные атаки: [%s%4d%s]\r\n"
									 "%sХ%s) Перевоплощений: [%s%4d%s]\r\n"
									 "%sН%s) Шансы использования умений: [%s%4d%s]\r\n"
									 "%sП%s) Загружаемые объекты: %s%s\r\n"
									 "%sР%s) Роли моба: %s%s\r\n"
									 "%sС%s) Сопротивления:\r\n"
									 "%sТ%s) Спас-броски:\r\n"
									 "%sУ%s) Дополнительные параметры:\r\n"
									 "%sФ%s) Способности:\r\n"
									 "%sЦ%s) Раса моба: %s%s\r\n"
									 "%sЧ%s) Клонирование:%s\r\n"
									 "%sS%s) Триггера: %s%s\r\n"
									 "%sЮ%s) Через сколько мобов замакс: [%s%4d%s]\r\n"
									 "%sQ%s) Выход:\r\n" "Ваш выбор: ",
			 grn, nrm, cyn, buf1,
			 grn, nrm, cyn, buf2,
			 grn, nrm, cyn, mob->summon_helpers.empty() ? "No" : "Yes",
			 grn, nrm,
			 grn, nrm,
			 grn, nrm, cyn, mob->get_str(), nrm,
			 grn, nrm, cyn, mob->get_dex(), nrm,
			 grn, nrm, cyn, mob->get_con(), nrm,
			 grn, nrm, cyn, mob->get_wis(), nrm,
			 grn, nrm, cyn, mob->get_int(), nrm,
			 grn, nrm, cyn, mob->get_cha(), nrm,
			 grn, nrm, cyn, GET_HEIGHT(mob), nrm,
			 grn, nrm, cyn, GET_WEIGHT(mob), nrm,
			 grn, nrm, cyn, GET_SIZE(mob), nrm,
			 grn, nrm, cyn, mob->mob_specials.extra_attack, nrm,
			 grn, nrm, cyn, mob->get_remort(), nrm,
			 grn, nrm, cyn, mob->mob_specials.like_work, nrm,
			 grn, nrm, cyn, mob->dl_list.empty() ? "Нет" : "Есть",
			 grn, nrm, cyn, roles_str.c_str(),
			 grn, nrm,
			 grn, nrm,
			 grn, nrm,
			 grn, nrm,
			 grn, nrm, cyn, npc_race_types[GET_RACE(mob) - ENpcRace::kBasic],
			 grn, nrm, cyn,
			 grn, nrm, cyn, !mob->proto_script->empty() ? "Set." : "Not Set.",
			 grn, nrm, cyn, mob->mob_specials.MaxFactor, nrm,
			 grn, nrm);
	SendMsgToChar(buf, d->character.get());

	OLC_MODE(d) = MEDIT_MAIN_MENU;
}

// Display on_death load object list
void disp_dl_list(DescriptorData *d) {
	// Список загружаемых посмертно объектов:
	// - VNUM - Prob - SpecParam -
	// (Объекты не определены)
	// 1) ...
	// 2) ...
	int i;
	CharData *mob;

	mob = OLC_MOB(d);
	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
			"\r\n-- Объекты загружаемые посмертно в моба [%s%d%s]\r\n"
			"-- Предмет (VNUM,Вероятность,Тип загрузки,Спец.параметр) -- \r\n", cyn, OLC_NUM(d), nrm);

	SendMsgToChar(buf, d->character.get());

	if (!mob->dl_list.empty()) {
		i = 0;
		auto p = mob->dl_list.begin();
		while (p != mob->dl_list.end()) {
			i++;

			auto tobj = GetObjectPrototype(p->obj_vnum);
			const char *objname = nullptr;
			if (p->obj_vnum && tobj) {
				objname = tobj->get_PName(ECase::kNom).c_str();
			} else {
				objname = "Нет";
			}

			sprintf(buf, "%d. %s (%d,%d,%d,%d)\r\n",
					i, objname, p->obj_vnum, p->load_prob, p->load_type, p->spec_param);

			SendMsgToChar(buf, d->character.get());
			p++;
		}
	} else {
		SendMsgToChar("Предметы не определены\r\n", d->character.get());
	}
	// Выводим
	// A) Добавить.
	// B) Удалить.
	// C) Изменить.
	// Q) Выход.
	sprintf(buf,
			"\r\n"
			"%sА%s) Добавить\r\n"
			"%sБ%s) Удалить\r\n" "%sQ%s) Выход\r\n" "Ваш выбор:", grn, nrm, grn, nrm, grn, nrm);

	SendMsgToChar(buf, d->character.get());
}

void medit_disp_clone_menu(DescriptorData *d) {
	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
			"%s1%s) Заменить триггеры\r\n"
			"%s2%s) Не заменять триггеры\r\n"
			"%s3%s) Не заменять падежи, описания, триггеры и если есть destination, помогают, загр.объекты\r\n"
			"%s4%s) Quit\r\n"
			"Ваш выбор : ",
			grn, nrm,
			grn, nrm,
			grn, nrm,
			grn, nrm);

	SendMsgToChar(buf, d->character.get());
}

// ************************************************************************
// *      The GARGANTAUN event handler                                    *
// ************************************************************************

void medit_parse(DescriptorData *d, char *arg) {
	int i, number = 0, plane, bit;

	if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
		if (!*arg || (!a_isdigit(arg[0]) && ((*arg == '-') && (!a_isdigit(arg[1]))))) {
			SendMsgToChar("Это числовое поле, повторите ввод : ", d->character.get());
			return;
		}
	}

	switch (OLC_MODE(d)) {
		case MEDIT_CONFIRM_SAVESTRING:
			// * Ensure mob has MOB_ISNPC set or things will go pair shaped.
			OLC_MOB(d)->SetFlag(EMobFlag::kNpc);
			switch (*arg) {
				case 'y':
				case 'Y':
				case 'д':
				case 'Д':
					// * Save the mob in memory and to disk.
//					SendMsgToChar("Saving mobile to memory a.\r\n", d->character.get());
					medit_save_internally(d);
					sprintf(buf, "OLC: %s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
					olc_log("%s edit mob %d", GET_NAME(d->character), OLC_NUM(d));
					mudlog(buf, NRM, std::max(kLvlBuilder, GET_INVIS_LEV(d->character)), SYSLOG, true);
					// * Do NOT free strings! Just the mob structure.
					cleanup_olc(d, CLEANUP_STRUCTS);
					SendMsgToChar("Моб сохранен.\r\n", d->character.get());
					break;

				case 'n':
				case 'N':
				case 'н':
				case 'Н': cleanup_olc(d, CLEANUP_ALL);
					break;

				default: SendMsgToChar("Неверный выбор!\r\n", d->character.get());
					SendMsgToChar("Вы желаете сохранить моба? : ", d->character.get());
					break;
			}
			return;

			//-------------------------------------------------------------------
		case MEDIT_MAIN_MENU: i = 0;
			olc_log("%s command %c", GET_NAME(d->character), *arg);
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d))    // Anything been changed?
					{
						SendMsgToChar("Вы желаете сохранить моба? (y/n): ", d->character.get());
						OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
					} else {
						cleanup_olc(d, CLEANUP_ALL);
					}
					return;

				case '1': OLC_MODE(d) = MEDIT_SEX;
					medit_disp_sex(d);
					return;

				case '2': SendMsgToChar("Введите синонимы : ", d->character.get());
					OLC_MODE(d) = MEDIT_ALIAS;
					i--;
					break;

				case '3':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nИменительный падеж [это КТО]: ",
								  GET_PAD(OLC_MOB(d), 0));
					OLC_MODE(d) = MEDIT_PAD0;
					i--;
					break;

				case '4':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nРодительный падеж [нет КОГО] : ",
								  GET_PAD(OLC_MOB(d), 1));
					OLC_MODE(d) = MEDIT_PAD1;
					i--;
					break;

				case '5':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nДательный падеж [дать КОМУ] : ",
								  GET_PAD(OLC_MOB(d), 2));
					OLC_MODE(d) = MEDIT_PAD2;
					i--;
					break;

				case '6':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nВинительный падеж [ударить КОГО] : ",
								  GET_PAD(OLC_MOB(d), 3));
					OLC_MODE(d) = MEDIT_PAD3;
					i--;
					break;

				case '7':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nТворительный падеж [следовать за КЕМ] : ",
								  GET_PAD(OLC_MOB(d), 4));
					OLC_MODE(d) = MEDIT_PAD4;
					i--;
					break;

				case '8':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nПредложный падеж [говорить о КОМ] : ",
								  GET_PAD(OLC_MOB(d), 5));
					OLC_MODE(d) = MEDIT_PAD5;
					i--;
					break;

				case '9':
					SendMsgToChar(d->character.get(),
								  "&S%s&s\r\nВведите длинное описание :-\r\n| ",
								  GET_LDESC(OLC_MOB(d)).c_str());
					OLC_MODE(d) = MEDIT_L_DESC;
					i--;
					break;

				case 'a':
				case 'A': OLC_MODE(d) = MEDIT_D_DESC;
					iosystem::write_to_output("Введите описание моба: (/s сохранить /h помощь)\r\n\r\n", d);
					d->backstr = nullptr;
					if (OLC_MOB(d)->player_data.description != "") {
						iosystem::write_to_output(OLC_MOB(d)->player_data.description.c_str(), d);
						d->backstr = str_dup(OLC_MOB(d)->player_data.description.c_str());
					}
					d->writer.reset(new utils::DelegatedStdStringWriter(OLC_MOB(d)->player_data.description));
					d->max_str = MAX_MOB_DESC;
					d->mail_to = 0;
					OLC_VAL(d) = 1;
					return;

				case 'b':
				case 'B': OLC_MODE(d) = MEDIT_LEVEL;
					SendMsgToChar("Введите новое значение : ", d->character.get());
					return;
					break;
				case 'c':
				case 'C': OLC_MODE(d) = MEDIT_ALIGNMENT;
					i++;
					break;

				case 'd':
				case 'D': OLC_MODE(d) = MEDIT_HITROLL;
					i++;
					break;

				case 'e':
				case 'E': OLC_MODE(d) = MEDIT_DAMROLL;
					i++;
					break;

				case 'f':
				case 'F': OLC_MODE(d) = MEDIT_NDD;
					i++;
					break;

				case 'g':
				case 'G': OLC_MODE(d) = MEDIT_SDD;
					i++;
					break;

				case 'h':
				case 'H': OLC_MODE(d) = MEDIT_NUM_HP_DICE;
					i++;
					break;

				case 'i':
				case 'I': OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
					i++;
					break;

				case 'j':
				case 'J': OLC_MODE(d) = MEDIT_ADD_HP;
					i++;
					break;

				case 'k':
				case 'K': OLC_MODE(d) = MEDIT_AC;
					i++;
					break;

				case 'l':
				case 'L': OLC_MODE(d) = MEDIT_EXP;
					i++;
					break;

				case 'm':
				case 'M': OLC_MODE(d) = MEDIT_GOLD;
					i++;
					break;

				case 'n':
				case 'N': OLC_MODE(d) = MEDIT_GOLD_DICE;
					i++;
					break;

				case 'o':
				case 'O': OLC_MODE(d) = MEDIT_GOLD_SIZE;
					i++;
					break;

				case 'p':
				case 'P': OLC_MODE(d) = MEDIT_POS;
					medit_disp_positions(d);
					return;

				case 'r':
				case 'R': OLC_MODE(d) = MEDIT_DEFAULT_POS;
					medit_disp_positions(d);
					return;

				case 't':
				case 'T': OLC_MODE(d) = MEDIT_ATTACK;
					medit_disp_attack_types(d);
					return;

				case 'u':
				case 'U': OLC_MODE(d) = MEDIT_MOB_FLAGS;
					medit_disp_mob_flags(d);
					return;

				case 'v':
				case 'V': OLC_MODE(d) = MEDIT_AFF_FLAGS;
					medit_disp_aff_flags(d);
					return;

				case 'w':
				case 'W': OLC_MODE(d) = MEDIT_NPC_FLAGS;
					medit_disp_npc_flags(d);
					return;

#if defined(OASIS_MPROG)
					case 'x':
					case 'X':
						OLC_MODE(d) = MEDIT_MPROG;
						medit_disp_mprog(d);
						return;
#endif

				case 's':
				case 'S': dg_olc_script_copy(d);
					OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
					dg_script_menu(d);
					return;

				case 'y':
				case 'Y': OLC_MODE(d) = MEDIT_DESTINATION;
					SendMsgToChar("\r\nВведите новое значение (-1 очистить все): ", d->character.get());
					return;
					break;

				case 'z':
				case 'Z': OLC_MODE(d) = MEDIT_HELPERS;
					medit_disp_helpers(d);
					return;

				case 'а':
				case 'А': OLC_MODE(d) = MEDIT_SKILLS;
					medit_disp_skills(d);
					return;

				case 'б':
				case 'Б': OLC_MODE(d) = MEDIT_SPELLS;
					medit_disp_spells(d);
					return;

				case 'в':
				case 'В': OLC_MODE(d) = MEDIT_STR;
					i++;
					break;

				case 'г':
				case 'Г': OLC_MODE(d) = MEDIT_DEX;
					i++;
					break;

				case 'д':
				case 'Д': OLC_MODE(d) = MEDIT_CON;
					i++;
					break;

				case 'е':
				case 'Е': OLC_MODE(d) = MEDIT_WIS;
					i++;
					break;

				case 'ж':
				case 'Ж': OLC_MODE(d) = MEDIT_INT;
					i++;
					break;

				case 'з':
				case 'З': OLC_MODE(d) = MEDIT_CHA;
					i++;
					break;

				case 'и':
				case 'И': OLC_MODE(d) = MEDIT_HEIGHT;
					i++;
					break;

				case 'к':
				case 'К': OLC_MODE(d) = MEDIT_WEIGHT;
					i++;
					break;

				case 'л':
				case 'Л': OLC_MODE(d) = MEDIT_SIZE;
					i++;
					break;

				case 'м':
				case 'М': OLC_MODE(d) = MEDIT_EXTRA;
					i++;
					break;

				case 'х':
				case 'Х': OLC_MODE(d) = MEDIT_REMORT;
					i++;
					break;

				case 'Ю':
				case 'ю': OLC_MODE(d) = MEDIT_MAXFACTOR;
					i++;
					break;

				case 'н':
				case 'Н': SendMsgToChar(d->character.get(), "\r\nВведите новое значение от 0 до 100%% :");
					OLC_MODE(d) = MEDIT_LIKE;
					return;

				case 'п':
				case 'П': OLC_MODE(d) = MEDIT_DLIST_MENU;
					disp_dl_list(d);
					return;

				case 'р':
				case 'Р': OLC_MODE(d) = MEDIT_ROLE;
					medit_disp_role(d);
					return;

				case 'с':
				case 'С': OLC_MODE(d) = MEDIT_RESISTANCES;
					medit_disp_resistances(d);
					return;

				case 'т':
				case 'Т': OLC_MODE(d) = MEDIT_SAVES;
					medit_disp_saves(d);
					return;

				case 'у':
				case 'У': OLC_MODE(d) = MEDIT_ADD_PARAMETERS;
					medit_disp_add_parameters(d);
					return;

				case 'ф':
				case 'Ф': OLC_MODE(d) = MEDIT_FEATURES;
					medit_disp_features(d);
					return;

				case 'ц':
				case 'Ц': OLC_MODE(d) = MEDIT_RACE;
					medit_disp_race(d);
					return;

				case 'ч':
				case 'Ч': OLC_MODE(d) = MEDIT_CLONE;
					medit_disp_clone_menu(d);
					return;

				default: medit_disp_menu(d);
					return;

			}

			if (i != 0) {
				SendMsgToChar(i == 1 ? "\r\nВведите новое значение : " :
							  i == -1 ? "\r\nВведите новый текст :\r\n] " : "\r\nОпаньки...:\r\n", d->character.get());
				return;
			}
			break;

		case OLC_SCRIPT_EDIT:
			if (dg_script_edit_parse(d, arg))
				return;
			break;

		case MEDIT_RACE: GET_RACE(OLC_MOB(d)) = std::max(to_underlying(ENpcRace::kBasic), std::min(to_underlying(ENpcRace::kLastNpcRace), atoi(arg) + ENpcRace::kBasic));
			break;

		case MEDIT_ROLE: {
			int num = atoi(arg);
			if (num != 0) {
				OLC_MOB(d)->set_role(num - 1, !OLC_MOB(d)->get_role(num - 1));
				medit_disp_role(d);
				return;
			}
		}
			break;

		case MEDIT_FEATURES: {
			number = atoi(arg);
			if (number == 0) {
				break;
			}
			auto feat_id = static_cast<EFeat>(number);
			if (MUD::Feat(feat_id).IsInvalid()) {
				SendMsgToChar("Неверный номер.\r\n", d->character.get());
			} else if (OLC_MOB(d)->HaveFeat(feat_id)) {
				OLC_MOB(d)->UnsetFeat(feat_id);
			} else {
				OLC_MOB(d)->SetFeat(feat_id);
			}
			medit_disp_features(d);
			return;
		}

		case MEDIT_RESISTANCES: number = atoi(arg);
			if (number == 0) {
				break;
			}
			--number;
			if (number < EResist::kFirstResist || number > EResist::kLastResist) {
				SendMsgToChar("Неверный номер.\r\n", d->character.get());
			} else if (sscanf(arg, "%d %d", &plane, &bit) < 2) {
				SendMsgToChar("Не указан уровень сопротивления.\r\n", d->character.get());
			} else {
				GET_RESIST(OLC_MOB(d), number) =  std::clamp(bit, kMinResistance, kMaxNpcResist);
			}
			medit_disp_resistances(d);
			return;

		case MEDIT_ADD_PARAMETERS: number = atoi(arg);
			if (number == 0) {
				break;
			}

			if (sscanf(arg, "%d %d", &plane, &bit) < 2) {
				SendMsgToChar("Не указана величина параметра.\r\n", d->character.get());
			} else
				switch (number) {
					case MEDIT_HPREG: OLC_MOB(d)->set_hitreg(std::clamp(bit, -200, 200));
						break;

					case MEDIT_ARMOUR: GET_ARMOUR(OLC_MOB(d)) = std::clamp(bit, 0, 100);
						break;

					case MEDIT_MANAREG: GET_MANAREG(OLC_MOB(d)) = std::clamp(bit, -400, 400);
						break;

					case MEDIT_CASTSUCCESS: GET_CAST_SUCCESS(OLC_MOB(d)) = std::clamp(bit, -1000, 1000);
						break;

					case MEDIT_SUCCESS: GET_MORALE(OLC_MOB(d)) = std::clamp(bit, 0, 200);
						break;

					case MEDIT_INITIATIVE: GET_INITIATIVE(OLC_MOB(d)) = std::clamp(bit, -100, 100);
						break;

					case MEDIT_ABSORBE: GET_ABSORBE(OLC_MOB(d)) = std::clamp(bit, -200, 200);
						break;

					case MEDIT_AR: GET_AR(OLC_MOB(d)) = std::clamp(bit, 0, 100);
						break;

					case MEDIT_MR: GET_MR(OLC_MOB(d)) = std::clamp(bit, 0, 100);
						break;

					case MEDIT_PR: GET_PR(OLC_MOB(d)) = std::clamp(bit, 0, 100);
						break;

					default: SendMsgToChar("Неверный номер.\r\n", d->character.get());
				}
			medit_disp_add_parameters(d);
			return;

		case MEDIT_SAVES: {
			number = atoi(arg);
			if (number == 0) {
				break;
			};
			auto saving = static_cast<ESaving>(number - 1);
			if (saving < ESaving::kFirst || saving > ESaving::kLast) {
				SendMsgToChar("Неверный номер.\r\n", d->character.get());
			} else if (sscanf(arg, "%d %d", &plane, &bit) < 2) {
				SendMsgToChar("Не указана величина спас-броска.\r\n", d->character.get());
			} else {
				SetSave(OLC_MOB(d), saving, std::clamp(bit, kMinSaving, kMaxSaving));
			}
			medit_disp_saves(d);
			return;
		}
		case MEDIT_ALIAS: OLC_MOB(d)->SetCharAliases(not_null(arg, "неопределен"));
			break;

		case MEDIT_PAD0: OLC_MOB(d)->player_data.PNames[ECase::kNom] = std::string(not_null(arg, "кто-то"));
			OLC_MOB(d)->set_npc_name(not_null(arg, "кто-то"));
			break;

		case MEDIT_PAD1: OLC_MOB(d)->player_data.PNames[ECase::kGen] = std::string(not_null(arg, "кого-то"));
			break;

		case MEDIT_PAD2: OLC_MOB(d)->player_data.PNames[ECase::kDat] = std::string(not_null(arg, "кому-то"));
			break;

		case MEDIT_PAD3: OLC_MOB(d)->player_data.PNames[ECase::kAcc] = std::string(not_null(arg, "кого-то"));
			break;

		case MEDIT_PAD4: OLC_MOB(d)->player_data.PNames[ECase::kIns] = std::string(not_null(arg, "кем-то"));
			break;

		case MEDIT_PAD5: OLC_MOB(d)->player_data.PNames[ECase::kPre] = std::string(not_null(arg, "о ком-то"));
			break;
			//-------------------------------------------------------------------
		case MEDIT_L_DESC:
			if (arg && *arg) {
				strcpy(buf, arg);
				strcat(buf, "\r\n");
				OLC_MOB(d)->player_data.long_descr = std::string(buf);
			} else {
				OLC_MOB(d)->player_data.long_descr = std::string("неопределен\r\n");
			}
			break;

		case MEDIT_D_DESC:
			// * We should never get here.
			cleanup_olc(d, CLEANUP_ALL);
			mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!", BRF, kLvlBuilder, SYSLOG, true);
			SendMsgToChar("Опаньки...\r\n", d->character.get());
			break;

#if defined(OASIS_MPROG)
			case MEDIT_MPROG_COMLIST:
				// * We should never get here, but if we do, bail out.
				cleanup_olc(d, CLEANUP_ALL);
				mudlog("SYSERR: OLC: medit_parse(): Reached MPROG_COMLIST case!", BRF, kLevelBuilder, SYSLOG, true);
				break;
#endif

		case MEDIT_MOB_FLAGS: number = planebit(arg, &plane, &bit);
			if (number < 0) {
				medit_disp_mob_flags(d);
				return;
			} else if (number == 0) {
				break;
			} else {
				OLC_MOB(d)->char_specials.saved.act.toggle_flag(plane, 1 << bit);
				medit_disp_mob_flags(d);
				if (OLC_MOB(d)->IsFlagged(EMobFlag::kIgnoresFormation)) {
					OLC_MOB(d)->set_role(static_cast<unsigned>(EMobClass::kRogue), true);
				}
				return;
			}

		case MEDIT_NPC_FLAGS: number = planebit(arg, &plane, &bit);
			if (number < 0) {
				medit_disp_npc_flags(d);
				return;
			} else if (number == 0) {
				break;
			} else {
				OLC_MOB(d)->mob_specials.npc_flags.toggle_flag(plane, 1 << bit);
				medit_disp_npc_flags(d);
				return;
			}

		case MEDIT_AFF_FLAGS: number = planebit(arg, &plane, &bit);
			if (number < 0) {
				medit_disp_aff_flags(d);
				return;
			} else if (number == 0) {
				break;
			} else {
				OLC_MOB(d)->char_specials.saved.affected_by.toggle_flag(plane, 1 << bit);
				medit_disp_aff_flags(d);
				return;
			}

#if defined(OASIS_MPROG)
			case MEDIT_MPROG:
				if ((i = atoi(arg)) == 0)
					medit_disp_menu(d);
				else if (i == OLC_MTOTAL(d))
				{
					struct mob_prog_data *temp;
					CREATE(temp, 1);
					temp->next = OLC_MPROGL(d);
					temp->type = -1;
					temp->arglist = NULL;
					temp->comlist = NULL;
					OLC_MPROG(d) = temp;
					OLC_MPROGL(d) = temp;
					OLC_MODE(d) = MEDIT_CHANGE_MPROG;
					medit_change_mprog(d);
				}
				else if (i < OLC_MTOTAL(d))
				{
					struct mob_prog_data *temp;
					int x = 1;
					for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
						x++;
					OLC_MPROG(d) = temp;
					OLC_MODE(d) = MEDIT_CHANGE_MPROG;
					medit_change_mprog(d);
				}
				else if (i == OLC_MTOTAL(d) + 1)
				{
					SendMsgToChar("Какого моба Вы хотите очистить? ", d->character);
					OLC_MODE(d) = MEDIT_PURGE_MPROG;
				}
				else
					medit_disp_menu(d);
				return;

			case MEDIT_PURGE_MPROG:
				if ((i = atoi(arg)) > 0 && i < OLC_MTOTAL(d))
				{
					struct mob_prog_data *temp;
					int x = 1;

					for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
						x++;
					OLC_MPROG(d) = temp;
					REMOVE_FROM_LIST(OLC_MPROG(d), OLC_MPROGL(d), next);
					free(OLC_MPROG(d)->arglist);
					free(OLC_MPROG(d)->comlist);
					free(OLC_MPROG(d));
					OLC_MPROG(d) = NULL;
					OLC_VAL(d) = 1;
				}
				medit_disp_mprog(d);
				return;

			case MEDIT_CHANGE_MPROG:
			{
				if ((i = atoi(arg)) == 1)
					medit_disp_mprog_types(d);
				else if (i == 2)
				{
					SendMsgToChar("Введите новый список аргументов: ", d->character);
					OLC_MODE(d) = MEDIT_MPROG_ARGS;
				}
				else if (i == 3)
				{
					SendMsgToChar("Введите новую mob prog команду:\r\n", d->character);
					// * Pass control to modify.c for typing.
					OLC_MODE(d) = MEDIT_MPROG_COMLIST;
					d->backstr = NULL;
					if (OLC_MPROG(d)->comlist)
					{
						iosystem::write_to_output(OLC_MPROG(d)->comlist, d);
						d->backstr = str_dup(OLC_MPROG(d)->comlist);
					}
					d->str = &OLC_MPROG(d)->comlist;
					d->max_str = kMaxStringLength;
					d->mail_to = 0;
					OLC_VAL(d) = 1;
				}
				else
					medit_disp_mprog(d);
				return;
#endif

			// * Numerical responses.

#if defined(OASIS_MPROG)
			/*
			  David Klasinc suggests for MEDIT_MPROG_TYPE:
				switch (atoi(arg)) {
				  case 0: OLC_MPROG(d)->type = 0; break;
				  case 1: OLC_MPROG(d)->type = 1; break;
				  case 2: OLC_MPROG(d)->type = 2; break;
				  case 3: OLC_MPROG(d)->type = 4; break;
				  case 4: OLC_MPROG(d)->type = 8; break;
				  case 5: OLC_MPROG(d)->type = 16; break;
				  case 6: OLC_MPROG(d)->type = 32; break;
				  case 7: OLC_MPROG(d)->type = 64; break;
				  case 8: OLC_MPROG(d)->type = 128; break;
				  case 9: OLC_MPROG(d)->type = 256; break;
				  case 10: OLC_MPROG(d)->type = 512; break;
				  case 11: OLC_MPROG(d)->type = 1024; break;
				  default: OLC_MPROG(d)->type = -1; break;
				}
			*/

		case MEDIT_MPROG_TYPE:
			OLC_MPROG(d)->type = (1 << std::max(0, std::min(atoi(arg), NUM_PROGS - 1)));
			OLC_VAL(d) = 1;
			medit_change_mprog(d);
			return;

		case MEDIT_MPROG_ARGS:
			OLC_MPROG(d)->arglist = str_dup(arg);
			OLC_VAL(d) = 1;
			medit_change_mprog(d);
			return;
#endif

		case MEDIT_SEX: OLC_MOB(d)->set_sex(static_cast<EGender>(std::max(0, std::min(NUM_GENDERS, atoi(arg)))));
			break;

		case MEDIT_HITROLL: GET_HR(OLC_MOB(d)) = std::max(0, std::min(500, atoi(arg)));
			break;

		case MEDIT_DAMROLL: GET_DR(OLC_MOB(d)) = std::max(0, std::min(50000, atoi(arg)));
			break;

		case MEDIT_NDD: GET_NDD(OLC_MOB(d)) = std::max(0, std::min(127, atoi(arg)));
			break;

		case MEDIT_SDD: GET_SDD(OLC_MOB(d)) = std::max(0, std::min(127, atoi(arg)));
			break;

		case MEDIT_NUM_HP_DICE: OLC_MOB(d)->mem_queue.total = std::max(0, std::min(500, atoi(arg)));
			break;

		case MEDIT_SIZE_HP_DICE: OLC_MOB(d)->mem_queue.stored = std::max(0, std::min(100000, atoi(arg)));
			break;

		case MEDIT_ADD_HP: OLC_MOB(d)->set_hit(std::max(0, std::min(5000000, atoi(arg))));
			break;

		case MEDIT_AC: GET_AC(OLC_MOB(d)) = std::max(-300, std::min(100, atoi(arg)));
			break;

		case MEDIT_EXP: OLC_MOB(d)->set_exp(atoi(arg));
			break;

		case MEDIT_GOLD: OLC_MOB(d)->set_gold(atoi(arg));
			break;

		case MEDIT_GOLD_DICE: GET_GOLD_NoDs(OLC_MOB(d)) = std::max(0, atoi(arg));
			break;

		case MEDIT_GOLD_SIZE: GET_GOLD_SiDs(OLC_MOB(d)) = std::max(0, atoi(arg));
			break;

		case MEDIT_POS: {
			auto pos = std::clamp(static_cast<EPosition>(atoi(arg)), EPosition::kDead, --EPosition::kLast);
			OLC_MOB(d)->SetPosition(pos);
			break;
		}
		case MEDIT_DEFAULT_POS:
			GET_DEFAULT_POS(OLC_MOB(d)) =
				std::clamp(static_cast<EPosition>(atoi(arg)), EPosition::kDead, --EPosition::kLast);
			break;
		case MEDIT_ATTACK: GET_ATTACK(OLC_MOB(d)) = std::max(0, std::min(NUM_ATTACK_TYPES - 1, atoi(arg)));
			break;

		case MEDIT_LEVEL: OLC_MOB(d)->set_level(atoi(arg));
			break;

		case MEDIT_ALIGNMENT: GET_ALIGNMENT(OLC_MOB(d)) = std::max(-1000, std::min(1000, atoi(arg)));
			break;

		case MEDIT_DESTINATION: number = atoi(arg);
			if (number == -1) {
				OLC_MOB(d)->mob_specials.dest_count = 0;
				break;
			}
			if ((plane = GetRoomRnum(number)) == kNowhere) {
				SendMsgToChar("Нет такой комнаты.\r\n", d->character.get());
			} else {
				for (plane = 0; plane < OLC_MOB(d)->mob_specials.dest_count; plane++) {
					if (number == OLC_MOB(d)->mob_specials.dest[plane]) {
						OLC_MOB(d)->mob_specials.dest_count--;
						for (; plane < OLC_MOB(d)->mob_specials.dest_count; plane++)
							OLC_MOB(d)->mob_specials.dest[plane] =
								OLC_MOB(d)->mob_specials.dest[plane + 1];
						OLC_MOB(d)->mob_specials.dest[plane] = 0;
						plane++;
						break;
					}
				}
				if (plane == OLC_MOB(d)->mob_specials.dest_count && plane < kMaxDest) {
					OLC_MOB(d)->mob_specials.dest_count++;
					OLC_MOB(d)->mob_specials.dest[plane] = number;
				}
			}
			break;

		case MEDIT_HELPERS: number = atoi(arg);
			if (number == 0) {
				break;
			}
			if ((plane = GetMobRnum(number)) < 0) {
				SendMsgToChar("Нет такого моба.\r\n", d->character.get());
			} else {
				auto it = std::find(OLC_MOB(d)->summon_helpers.begin(), OLC_MOB(d)->summon_helpers.end(), number);
				if (it != OLC_MOB(d)->summon_helpers.end()) {
					OLC_MOB(d)->summon_helpers.erase(it);
				}
				else {
					OLC_MOB(d)->summon_helpers.push_back(number);
				}
			}
			medit_disp_helpers(d);
			return;

		case MEDIT_SKILLS: {
			number = atoi(arg);
			if (number == 0) {
				break;
			}
			auto skill_id = static_cast<ESkill>(number);
			if (MUD::Skills().IsInvalid(skill_id)) {
				SendMsgToChar("Неизвестное умение.\r\n", d->character.get());
			} else if (OLC_MOB(d)->GetSkill(skill_id)) {
				OLC_MOB(d)->set_skill(skill_id, 0);
			} else if (sscanf(arg, "%d %d", &plane, &bit) < 2) {
				SendMsgToChar("Не указан уровень владения умением.\r\n", d->character.get());
			} else {
				OLC_MOB(d)->set_skill(skill_id, std::clamp(bit, 0, MUD::Skill(skill_id).cap));
			}
			medit_disp_skills(d);
			return;
		}

		case MEDIT_SPELLS: {
			number = atoi(arg);
			if (number == 0) {
				OLC_MOB(d)->mob_specials.have_spell = false;
				for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
					if (MUD::Spell(spell_id).IsUnavailable()) {
						continue;
					}
					if (GET_SPELL_MEM(OLC_MOB(d), spell_id)) {
						OLC_MOB(d)->mob_specials.have_spell = true;
						break;
					}
				}
				break;
			}
			auto spell_id = static_cast<ESpell>(number);
			if (MUD::Spell(spell_id).IsInvalid()) {
				SendMsgToChar("Неизвестное заклинание.\r\n", d->character.get());
			} else if (sscanf(arg, "%d %d", &plane, &bit) < 2) {
				SendMsgToChar("Не указано количество заклинаний.\r\n", d->character.get());
			} else {
				GET_SPELL_MEM(OLC_MOB(d), spell_id) = std::min(200, std::max(0, bit));
			}
			medit_disp_spells(d);
			return;
		}

		case MEDIT_STR: OLC_MOB(d)->set_str(std::min(100, std::max(1, atoi(arg))));
			break;

		case MEDIT_DEX: OLC_MOB(d)->set_dex(std::min(100, std::max(1, atoi(arg))));
			break;

		case MEDIT_CON: OLC_MOB(d)->set_con(std::min(100, std::max(1, atoi(arg))));
			break;

		case MEDIT_WIS: OLC_MOB(d)->set_wis(std::min(100, std::max(1, atoi(arg))));
			break;

		case MEDIT_INT: OLC_MOB(d)->set_int(std::min(100, std::max(1, atoi(arg))));
			break;

		case MEDIT_CHA: OLC_MOB(d)->set_cha(std::min(100, std::max(1, atoi(arg))));
			break;

		case MEDIT_WEIGHT: GET_WEIGHT(OLC_MOB(d)) = std::min(200, std::max(1, atoi(arg)));
			break;

		case MEDIT_HEIGHT: GET_HEIGHT(OLC_MOB(d)) = std::min(200, std::max(50, atoi(arg)));
			break;

		case MEDIT_SIZE: GET_SIZE(OLC_MOB(d)) = std::min(100, std::max(1, atoi(arg)));
			break;

		case MEDIT_EXTRA: OLC_MOB(d)->mob_specials.extra_attack = std::min(5, std::max(0, atoi(arg)));
			break;

		case MEDIT_REMORT: OLC_MOB(d)->set_remort(std::min(100, std::max(0, atoi(arg))));
			break;

		case MEDIT_LIKE: OLC_MOB(d)->mob_specials.like_work = std::min(100, std::max(0, atoi(arg)));
			break;

		case MEDIT_DLIST_MENU:
			if (*arg) {
				// Обрабатываем комнады добавить удалить и.т.п
				switch (*arg) {
					case 'а':
					case 'А':
						// Добавляем запись.
						OLC_MODE(d) = MEDIT_DLIST_ADD;
						SendMsgToChar("\r\nVNUM - виртуальный номер прототипа\r\n"
									  "LoadProb - процент загрузки\r\n"
									  "LoadType - \r\n"
									  "  0 - загружать всегда. \r\n"
									  "  1 - загружать если предыдущий предмет списка был загружен. \r\n"
									  "  2 - загружать всегда, не менять результата предыдущей загрузки. \r\n"
									  "  3 - загружать если был загружен предыдущий, не менять результата.\r\n"
									  "SpecParam - спец.параметр:\r\n"
									  "  0 - загружать всегда. \r\n"
									  "  1 - загружать с убывающей вероятностью. \r\n"
									  "  2 - загружать при освежевании трупа NPC. \r\n"
									  "Введите через пробел \r\n(VNUM LoadProb LoadType SpecParam):",
									  d->character.get());

						return;

					case 'б':
					case 'Б':
						// Удаляем запись.
						OLC_MODE(d) = MEDIT_DLIST_DEL;
						SendMsgToChar("\r\nВведите номер удаляемой записи:", d->character.get());
						return;

					case 'q':
					case 'Q': OLC_MODE(d) = MEDIT_MAIN_MENU;
						medit_disp_menu(d);
						return;
				}

			}
			SendMsgToChar("\r\nНеверный выбор.\r\n", d->character.get());
			OLC_MODE(d) = MEDIT_DLIST_MENU;
			disp_dl_list(d);
			return;

		case MEDIT_DLIST_ADD:
			if (!dead_load::ParseDeadLoadLine(OLC_MOB(d)->dl_list, arg))
				SendMsgToChar("\r\nНеверный ввод.\r\n", d->character.get());
			else {
				SendMsgToChar("\r\nЗапись добавлена.\r\n", d->character.get());
				OLC_VAL(d) = 1;
			}
			OLC_MODE(d) = MEDIT_DLIST_MENU;
			disp_dl_list(d);
			return;

		case MEDIT_DLIST_DEL: number = atoi(arg);
			if (number != 0) {
				if (OLC_MOB(d)->dl_list.empty()) {
					SendMsgToChar("Список пуст!\r\n", d->character.get());
					OLC_MODE(d) = MEDIT_DLIST_MENU;
					disp_dl_list(d);
					return;
				}
				// Удаляем указаный элемент.
				
				i = 0;
				auto p = OLC_MOB(d)->dl_list.begin();
			
				while (p != OLC_MOB(d)->dl_list.end() && i < number - 1) {
					p++;
					i++;
				}
				if (i == number - 1) {
					OLC_MOB(d)->dl_list.erase(p);
					SendMsgToChar("\r\nЗапись удалена.\r\n", d->character.get());
					OLC_VAL(d) = 1;
				} else
					SendMsgToChar("\r\nЗапись не найдена.\r\n", d->character.get());
			}
			OLC_MODE(d) = MEDIT_DLIST_MENU;
			disp_dl_list(d);

			return;

		case MEDIT_CLONE:
			switch (*arg) {
				case '1': OLC_MODE(d) = MEDIT_CLONE_WITH_TRIGGERS;
					SendMsgToChar("Введите VNUM моба для клонирования:", d->character.get());
					return;
				case '2': OLC_MODE(d) = MEDIT_CLONE_WITHOUT_TRIGGERS;
					SendMsgToChar("Введите VNUM моба для клонирования:", d->character.get());
					return;
				case '3': OLC_MODE(d) = MEDIT_CLONE_PARTIAL;
					SendMsgToChar("Введите VNUM моба для клонирования:", d->character.get());
					return;
				case '4': break;    //to main menu
				default: medit_disp_clone_menu(d);
					return;
			}
			break;

		case MEDIT_CLONE_WITH_TRIGGERS: {
			auto rnum = GetMobRnum(atoi(arg));

			if (rnum < 0) {
				SendMsgToChar("Нет моба с таким внумом. Повторите ввод:", d->character.get());
				return;
			}

			auto rnum_old = GET_MOB_RNUM(OLC_MOB(d));
			CopyMobilePrototypeForMedit(OLC_MOB(d), &mob_proto[rnum], false);
			OLC_MOB(d)->set_rnum(rnum_old);

			break;
		}

		case MEDIT_CLONE_WITHOUT_TRIGGERS: {
			auto rnum = GetMobRnum(atoi(arg));

			if (rnum < 0) {
				SendMsgToChar("Нет моба с таким внумом. Повторите ввод:", d->character.get());
				return;
			}

			auto rnum_old = GET_MOB_RNUM(OLC_MOB(d));
			auto proto_script_old = OLC_MOB(d)->proto_script;
			CopyMobilePrototypeForMedit(OLC_MOB(d), &mob_proto[rnum], false);
			OLC_MOB(d)->set_rnum(rnum_old);
			OLC_MOB(d)->proto_script = proto_script_old;

			break;
		}

		case MEDIT_CLONE_PARTIAL: {
			auto rnum = GetMobRnum(atoi(arg));

			if (rnum < 0) {
				SendMsgToChar("Нет моба с таким внумом. Повторите ввод:", d->character.get());
				return;
			}

			auto rnum_old = GET_MOB_RNUM(OLC_MOB(d));
			auto proto_script_old = OLC_MOB(d)->proto_script;
			CopyMobilePrototypeForMedit(OLC_MOB(d), &mob_proto[rnum], true);
			OLC_MOB(d)->set_rnum(rnum_old);
			OLC_MOB(d)->proto_script = proto_script_old;

			break;
		}
		case MEDIT_MAXFACTOR: OLC_MOB(d)->mob_specials.MaxFactor = std::max(0, atoi(arg));
			break;

		default:
			// * We should never get here.
			cleanup_olc(d, CLEANUP_ALL);
			mudlog("SYSERR: OLC: medit_parse(): Reached default case!", BRF, kLvlBuilder, SYSLOG, true);
			SendMsgToChar("Oops...\r\n", d->character.get());
			break;
	}

	/*
	 * END OF CASE
	 * If we get here, we have probably changed something, and now want to
	 * return to main menu.  Use OLC_VAL as a 'has changed' flag
	 */

	OLC_VAL(d) = 1;
	medit_disp_menu(d);
}
// * End of medit_parse(), thank god.

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
