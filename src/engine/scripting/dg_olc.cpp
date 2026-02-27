/**************************************************************************
*  File: dg_olc.h.cpp                                      Part of Bylins   *
*  Usage: trig edit from olc functions                                    *
*                                                                         *
*  Usage: this source file is used in extending Oasis style OLC for       *
*  dg-scripts onto a CircleMUD that already has dg-scripts (as released   *
*  by Mark Heilpern on 1/1/98) implemented.                               *
*                                                                         *
* Parts of this file by Chris Jacobson of _Aliens vs Predator: The MUD_   *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#include "dg_olc.h"
#include "engine/entities/obj_data.h"
#include "engine/olc/olc.h"
#include "dg_event.h"
#include "engine/entities/char_data.h"
#include "engine/entities/zone.h"
#include <sys/stat.h>
#include "engine/db/world_characters.h"
#include "engine/db/global_objects.h"
#include "engine/db/world_data_source_manager.h"
#include "dg_db_scripts.h"
#include "gameplay/mechanics/dungeons.h"

extern const char *trig_types[], *otrig_types[], *wtrig_types[];
extern DescriptorData *descriptor_list;
extern int top_of_trigt;
extern void ExtractTrigger(Trigger *trig);

// prototype externally defined functions
void free_varlist(struct TriggerVar *vd);

void trigedit_disp_menu(DescriptorData *d);
void trigedit_save(DescriptorData *d);
bool trigedit_save_to_disk(int zone_rnum, int notify_level);
void trigedit_create_index(int znum, const char *type);
void indent_trigger(std::string &cmd, int *level);

inline void fprint_script(FILE *fp, const ObjData::triggers_list_t &scripts) {
	for (const auto vnum : scripts) {
		fprintf(fp, "T %d\n", vnum);
	}
}

// called when a mob or object is being saved to disk, so its script can
// be saved
void script_save_to_disk(FILE *fp, const void *item, int type) {
	if (type == MOB_TRIGGER) {
		fprint_script(fp, *static_cast<const CharData *>(item)->proto_script);
	} else if (type == OBJ_TRIGGER) {
		fprint_script(fp, static_cast<const CObjectPrototype *>(item)->get_proto_script());
	} else if (type == WLD_TRIGGER) {
		fprint_script(fp, *static_cast<const RoomData *>(item)->proto_script);
	} else {
		log("SYSERR: Invalid type passed to script_save_mobobj_to_disk()");
		return;
	}
}

/**************************************************************************
 *  Редактирование ТРИГГЕРОВ
 *  trigedit
 **************************************************************************/

void trigedit_setup_new(DescriptorData *d) {
	Trigger *trig = new Trigger(-1, "new trigger", MTRIG_GREET);

	// cmdlist will be a large char string until the trigger is saved
	CREATE(OLC_STORAGE(d), MAX_CMD_LENGTH);
	strcpy(OLC_STORAGE(d), "say My trigger commandlist is not complete!\r\n");
	trig->narg = 100;

	OLC_TRIG(d) = trig;
	OLC_VAL(d) = 0;        // Has changed flag. (It hasn't so far, we just made it.)

	trigedit_disp_menu(d);
}

void trigedit_setup_existing(DescriptorData *d, int rtrg_num) {
	// Allocate a scratch trigger structure
	Trigger *trig = new Trigger(*trig_index[rtrg_num]->proto);

	// convert cmdlist to a char string
	auto c = *trig->cmdlist;
	CREATE(OLC_STORAGE(d), MAX_CMD_LENGTH);
	strcpy(OLC_STORAGE(d), "");

	while (c) {
		strcat(OLC_STORAGE(d), c->cmd.c_str());
		strcat(OLC_STORAGE(d), "\r\n");
		c = c->next;
	}
	// now trig->cmdlist is something to pass to the text editor
	// it will be converted back to a real cmdlist_element list later

	OLC_TRIG(d) = trig;
	OLC_VAL(d) = 0;        // Has changed flag. (It hasn't so far, we just made it.)

	trigedit_disp_menu(d);
}

void trigedit_disp_menu(DescriptorData *d) {
	Trigger *trig = OLC_TRIG(d);
	const char *attach_type;
	char trgtypes[256];

	if (trig->get_attach_type() == MOB_TRIGGER) {
		attach_type = "Mobiles";
		sprintbit(GET_TRIG_TYPE(trig), trig_types, trgtypes);
	} else if (trig->get_attach_type() == OBJ_TRIGGER) {
		attach_type = "Objects";
		sprintbit(GET_TRIG_TYPE(trig), otrig_types, trgtypes);
	} else if (trig->get_attach_type() == WLD_TRIGGER) {
		attach_type = "Rooms";
		sprintbit(GET_TRIG_TYPE(trig), wtrig_types, trgtypes);
	} else {
		attach_type = "undefined";
		trgtypes[0] = '\0';
	}
	std::stringstream out;
	out << "Редактирование триггера " << "[&y" << OLC_NUM(d) << "&n]\r\n\r\n"
			<< "&g1)&n Название         : " << "&y" <<GET_TRIG_NAME(trig) << "\r\n"
			<< "&g2)&n Тип: " << "&y" << attach_type << "\r\n"
			<< "&g3)&n События: " << "&y" << trgtypes << "\r\n"
			<< "&g4)&n Числовой Аргумент : " << "&y" << trig->narg << "\r\n"
			<< "&g5)&n Аргументы    : " << "&y" << trig->arglist.c_str() << "\r\n"
			<< "&g6)&n Команды:\r\n"
			<< "&c" << OLC_STORAGE(d);
	if (trig->get_attach_type() == MOB_TRIGGER) {
		out << "&g7)&n Обрабатывать команды моба в стане? : &y" << (trig->add_flag ? "ДА" : "НЕТ") << "&n\r\n";
	}
	out << "&gQ)&n Завершить редактирование\r\n" "Введите Выбранное :";
	SendMsgToChar(out.str(), d->character.get());
	OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
}

void trigedit_disp_types(DescriptorData *d) {
	int i, columns = 0;
	const char **types;

	switch (OLC_TRIG(d)->get_attach_type()) {
		case WLD_TRIGGER: types = wtrig_types;
			break;
		case OBJ_TRIGGER: types = otrig_types;
			break;
		case MOB_TRIGGER:
		default: types = trig_types;
			break;
	}

#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif

	for (i = 0; i < NUM_TRIG_TYPE_FLAGS; i++) {
		sprintf(buf, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, types[i], !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}

	sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), types, buf1, 2);
	snprintf(buf, kMaxStringLength, "\r\nCurrent types : %s%s%s\r\nEnter type (0 to quit) : ", cyn, buf1, nrm);
	SendMsgToChar(buf, d->character.get());
}

void trigedit_parse(DescriptorData *d, char *arg) {
	int i = 0;

	switch (OLC_MODE(d)) {
		case TRIGEDIT_MAIN_MENU:
			switch (tolower(*arg)) {
				case 'q':
					if (OLC_VAL(d))    // Anything been changed?
					{
						if (!GET_TRIG_TYPE(OLC_TRIG(d))) {
							SendMsgToChar("Invalid Trigger Type! Answer a to abort quit!\r\n", d->character.get());
						}
						SendMsgToChar("Желаете ли вы сохранить изменения в триггере? (y/n): ", d->character.get());
						OLC_MODE(d) = TRIGEDIT_CONFIRM_SAVESTRING;
					} else {
						cleanup_olc(d, CLEANUP_ALL);
					}
					return;

				case '1': OLC_MODE(d) = TRIGEDIT_NAME;
		 			SendMsgToChar("Название: ", d->character.get());
					break;

				case '2': OLC_MODE(d) = TRIGEDIT_INTENDED;
					SendMsgToChar("0: Мобы, 1: Объекты, 2: Комнаты: ", d->character.get());
					break;

				case '3': OLC_MODE(d) = TRIGEDIT_TYPES;
					trigedit_disp_types(d);
					break;
				case '4': OLC_MODE(d) = TRIGEDIT_NARG;
					SendMsgToChar("Числовой аргумент: ", d->character.get());
					break;

				case '5': OLC_MODE(d) = TRIGEDIT_ARGUMENT;
					SendMsgToChar("Аргументы: ", d->character.get());
					break;

				case '6': OLC_MODE(d) = TRIGEDIT_COMMANDS;
					SendMsgToChar("Введите команды триггера: (/s saves /h for help)\r\n\r\n", d->character.get());
					d->backstr = nullptr;
					if (OLC_STORAGE(d)) {
						SendMsgToChar(d->character.get(), "&S%s&s", OLC_STORAGE(d));
						d->backstr = str_dup(OLC_STORAGE(d));
					}
					d->writer.reset(new utils::DelegatedStringWriter(OLC_STORAGE(d)));
					d->max_str = MAX_CMD_LENGTH;
					d->mail_to = 0;
					OLC_VAL(d) = 1;
					break;
				case '7': 
					if (OLC_TRIG(d)->get_attach_type() == MOB_TRIGGER) {
						OLC_MODE(d) = TRIGEDIT_ADDFLAG;
						SendMsgToChar("Флаг (1-ДА): ", d->character.get());
					} else trigedit_disp_menu(d);
					break;

				default: trigedit_disp_menu(d);
					return;
			}
			return;

		case TRIGEDIT_CONFIRM_SAVESTRING:
			switch (tolower(*arg)) {
				case 'y': trigedit_save(d);
					sprintf(buf, "OLC: %s edits trigger %d", GET_NAME(d->character), OLC_NUM(d));
					olc_log("%s end trig %d", GET_NAME(d->character), OLC_NUM(d));
					mudlog(buf, NRM, MAX(kLvlBuilder, GET_INVIS_LEV(d->character)), SYSLOG, true);
					// fall through

				case 'n': cleanup_olc(d, CLEANUP_ALL);
					return;

				case 'a':    // abort quitting
					break;

				default: SendMsgToChar("Неверный выбор!\r\n", d->character.get());
					SendMsgToChar("Желаете ли вы сохранить изменения в триггере? : ", d->character.get());
					return;
			}
			break;

		case TRIGEDIT_NAME: OLC_TRIG(d)->set_name((arg && *arg) ? arg : "undefined");
			OLC_VAL(d)++;
			break;

		case TRIGEDIT_INTENDED:
			if ((atoi(arg) >= MOB_TRIGGER) && (atoi(arg) <= WLD_TRIGGER)) {
				OLC_TRIG(d)->set_attach_type(atoi(arg));
			}
			OLC_VAL(d)++;
			break;

		case TRIGEDIT_NARG: OLC_TRIG(d)->narg = atoi(arg);
			OLC_VAL(d)++;
			break;

		case TRIGEDIT_ARGUMENT: OLC_TRIG(d)->arglist = (arg && *arg) ? arg : "";
			OLC_VAL(d)++;
			break;

		case TRIGEDIT_ADDFLAG: OLC_TRIG(d)->add_flag = atoi(arg) == 1 ? 1 : 0;
			OLC_VAL(d)++;
			break;

		case TRIGEDIT_TYPES:
			if ((i = atoi(arg)) == 0) {
				break;
			} else if (!((i < 0) || (i > NUM_TRIG_TYPE_FLAGS))) {
				auto trigger_type = OLC_TRIG(d)->get_trigger_type();
				TOGGLE_BIT(trigger_type, 1 << (i - 1));
				OLC_TRIG(d)->set_trigger_type(trigger_type);
			}
			OLC_VAL(d)++;
			trigedit_disp_types(d);
			return;

		case TRIGEDIT_COMMANDS: break;
	}

	OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
	trigedit_disp_menu(d);
}

// * print out the letter codes pertaining to the bits set in 'data'
void sprintbyts(int data, char *dest) {
	int i;
	char *p = dest;

	for (i = 0; i < 32; i++) {
		if (data & (1 << i)) {
			*p = ((i <= 25) ? ('a' + i) : ('A' + i));
			p++;
		}
	}
	*p = '\0';
}
void TriggerDistribution(DescriptorData *d) {
	switch (OLC_TRIG(d)->get_attach_type()) {
		case WLD_TRIGGER: // в комнатах ссылка на прототип
			for (RoomRnum nr = kFirstRoom; nr <= top_of_world; nr++) {
				if (!SCRIPT(world[nr])->has_triggers())
					continue;
				auto sc = SCRIPT(world[nr]); 
				for (auto t : sc->script_trig_list) {
					if (GET_TRIG_VNUM(t) == OLC_NUM(d)) {
						SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);
					}
				}
			}
		break;
		case MOB_TRIGGER:
			for (auto &ch : character_list) {
				if (!SCRIPT(ch)->has_triggers())
					continue;
				auto sc = SCRIPT(ch);
				for (auto t : sc->script_trig_list) {
					if (GET_TRIG_VNUM(t) == OLC_NUM(d)) {
						sc->remove_trigger(OLC_NUM(d));
						auto trig = read_trigger(GetTriggerRnum(OLC_NUM(d)));
						if (!add_trigger(ch->script.get(), trig, -1)) {
							ExtractTrigger(trig);
						}
					}
				}
			}
		break;
		case OBJ_TRIGGER:
			world_objects.foreach_on_copy([&d](const ObjData::shared_ptr &obj_ptr) {
				if (!obj_ptr->get_script()->has_triggers())
					return;
				auto sc = obj_ptr->get_script().get();
				for (auto t : sc->script_trig_list) {
					if (GET_TRIG_VNUM(t) == OLC_NUM(d)) {
						sc->remove_trigger(OLC_NUM(d));
						auto trig = read_trigger(GetTriggerRnum(OLC_NUM(d)));
						if (!add_trigger(obj_ptr->get_script().get(), trig, -1)) {
							ExtractTrigger(trig);
						}
					}
				}
			});
		break;
		default:
		break;
	}
}

// save the zone's triggers to internal memory and to disk
void trigedit_save(DescriptorData *d) {
	int trig_rnum, i;
	int found = 0;
	char *s;
	Trigger *proto;
	Trigger *trig = OLC_TRIG(d);
	IndexData **new_index;
	int zone;
	DescriptorData *dsc;

	// Recompile the command list from the new script
	s = OLC_STORAGE(d);
	olc_log("%s start trig %d:\n%s", GET_NAME(d->character), OLC_NUM(d), OLC_STORAGE(d));

	trig->cmdlist->reset(new cmdlist_element());
	const auto &cmdlist = *trig->cmdlist;
	const auto cmd_token = strtok(s, "\n\r");
	if (cmd_token) { //тут штатная ошибка циркуля, если strok не нашел подстроку то он возвращает не nullptr а строку полностью т.е. надо str_cmp(cmd_token, s)
		cmdlist->cmd = cmd_token;
		cmdlist->line_num = 1;
	} else {
		cmdlist->cmd = "";
		cmdlist->line_num = 0;
	}
	auto cmd = cmdlist;
	int line_num = 2;
	while ((s = strtok(nullptr, "\n\r"))) {
		cmd->next.reset(new cmdlist_element());
		cmd = cmd->next;
		cmd->cmd = s;
		cmd->line_num = line_num++;
	}
	cmd->next.reset();
//	log("Триггер зона1 %d внум %d ласт %d", OLC_ZNUM(d), zone_table[OLC_ZNUM(d)].vnum, trig_index[zone_table[OLC_ZNUM(d)].RnumTrigsLocation.second]->vnum);

	if ((trig_rnum = GetTriggerRnum(OLC_NUM(d))) != -1) {
		// Этот триггер уже есть.

		// Очистка прототипа
		proto = trig_index[trig_rnum]->proto;
		proto->cmdlist.reset();

		// make the prototype look like what we have
		*proto = *trig;

		// go through the mud and replace existing triggers
		if (trigger_list.has_triggers_with_rnum(trig_rnum)) {
			const auto triggers = trigger_list.get_triggers_with_rnum(trig_rnum);
			for (const auto trigger : triggers) {
				// because rnum of the trigger may change (not likely but technically is possible)
				// we have to rebind trigger after changing
				trigger_list.remove(trigger);

				trigger->arglist.clear();
				trigger->set_name("");
				if (GET_TRIG_WAIT(trigger).time_remaining > 0) {
					free(GET_TRIG_WAIT(trigger).info);    // Причина уже обсуждалась
					remove_event(GET_TRIG_WAIT(trigger));
				}

				trigger->clear_var_list();
				*trigger = *proto;

				trigger_list.add(trigger);
			}
		}
	} else {
		// this is a new trigger
		CREATE(new_index, top_of_trigt + 2);

		for (i = 0; i < top_of_trigt; i++) {
			if (!found) {
				if (trig_index[i]->vnum > OLC_NUM(d)) {
					found = true;
					trig_rnum = i;

					CREATE(new_index[trig_rnum], 1);
					OLC_TRIG(d)->set_rnum(trig_rnum);
					new_index[trig_rnum]->vnum = OLC_NUM(d);
					new_index[trig_rnum]->total_online = 0;
					new_index[trig_rnum]->func = nullptr;
					new_index[trig_rnum]->proto = new Trigger(*trig);
					--i;
					continue;    // повторить копирование еще раз, но уже по-другому
				} else {
					new_index[i] = trig_index[i];
				}
			} else {
				// докопировать
				new_index[i + 1] = trig_index[i];
				proto = trig_index[i]->proto;
				proto->set_rnum(i + 1);
			}
		}
		if (!found) {
			trig_rnum = i;
			CREATE(new_index[trig_rnum], 1);
			OLC_TRIG(d)->set_rnum(trig_rnum);
			new_index[trig_rnum]->vnum = OLC_NUM(d);
			new_index[trig_rnum]->total_online = 0;
			new_index[trig_rnum]->func = nullptr;
			new_index[trig_rnum]->proto = new Trigger(*trig);
		}
		free(trig_index);
		trig_index = new_index;
		top_of_trigt++;
		zone_table[OLC_ZNUM(d)].RnumTrigsLocation.second++;
		for (ZoneRnum zrn = OLC_ZNUM(d) + 1; zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
			zone_table[zrn].RnumTrigsLocation.first++;
			zone_table[zrn].RnumTrigsLocation.second++;
		}

		// HERE IT HAS TO GO THROUGH AND FIX ALL SCRIPTS/TRIGS OF HIGHER RNUM
		trigger_list.shift_rnums_from(trig_rnum);

		// * Update other trigs being edited.
		for (dsc = descriptor_list; dsc; dsc = dsc->next) {
			if (dsc->state == EConState::kTrigedit) {
				if (GET_TRIG_RNUM(OLC_TRIG(dsc)) >= trig_rnum) {
					OLC_TRIG(dsc)->set_rnum(1 + OLC_TRIG(dsc)->get_rnum());
				}
			}
		}
	}
	
	// Save trigger to disk using data source abstraction (YAML/SQLite/Legacy)
	TriggerDistribution(d);
	zone = zone_table[OLC_ZNUM(d)].vnum;
	int notify_level = MAX(kLvlBuilder, GET_INVIS_LEV(d->character));

	auto* data_source = world_loader::WorldDataSourceManager::Instance().GetDataSource();
	if (!data_source->SaveTriggers(OLC_ZNUM(d), OLC_NUM(d), notify_level)) {
		SendMsgToChar("ОШИБКА: Не удалось сохранить триггер!\r\n", d->character.get());
		return;
	}

	SendMsgToChar("Триггер сохранен.\r\n", d->character.get());
	trigedit_create_index(zone, "trg");
}

// Save all triggers for a zone to disk (without requiring DescriptorData)
bool trigedit_save_to_disk(int zone_rnum, int notify_level) {
	FILE *trig_file;
	int trig_rnum, i;
	Trigger *trig;
	int zone, top;
	char buf[kMaxStringLength];
	char bitBuf[kMaxInputLength];
	char fname[kMaxInputLength];
	char logbuf[kMaxInputLength];

	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size())) {
		log("SYSERR: trigedit_save_to_disk: Invalid zone rnum %d", zone_rnum);
		return false;
	}

	zone = zone_table[zone_rnum].vnum;
	top = zone_table[zone_rnum].top;

	if (zone >= dungeons::kZoneStartDungeons) {
		sprintf(buf, "Отказ сохранения зоны %d на диск.", zone);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return false;
	}

	sprintf(fname, "%s/%i.new", TRG_PREFIX, zone);
	if (!(trig_file = fopen(fname, "w"))) {
		snprintf(logbuf, kMaxInputLength, "SYSERR: OLC: Can't open trig file \"%s\"", fname);
		mudlog(logbuf, BRF, notify_level, SYSLOG, true);
		return false;
	}

	for (i = zone * 100; i <= top; i++) {
		if ((trig_rnum = GetTriggerRnum(i)) != -1) {
			trig = trig_index[trig_rnum]->proto;

			if (fprintf(trig_file, "#%d\n", i) < 0) {
				sprintf(logbuf, "SYSERR: OLC: Can't write trig file!");
				mudlog(logbuf, BRF, notify_level, SYSLOG, true);
				fclose(trig_file);
				return false;
			}
			sprintbyts(GET_TRIG_TYPE(trig), bitBuf);
			fprintf(trig_file, "%s~\n"
							   "%d %s %d %d\n"
							   "%s~\n",
					(GET_TRIG_NAME(trig)) ? (GET_TRIG_NAME(trig)) :
					"unknown trigger", trig->get_attach_type(), bitBuf,
					GET_TRIG_NARG(trig), trig->add_flag, trig->arglist.c_str());

			// Build the text for the script
			int lev = 0;
			strcpy(buf, "");
			for (auto cmd = *trig->cmdlist; cmd; cmd = cmd->next) {
				// Indenting
				indent_trigger(cmd->cmd, &lev);
				strcat(buf, cmd->cmd.c_str());
				strcat(buf, "\n");
			}

			if (!buf[0]) {
				strcpy(buf, "* Empty script~\n");
				fprintf(trig_file, "%s", buf);
			} else {
				char *p;
				// замена одиночного '~' на '~~'
				p = strtok(buf, "~");
				fprintf(trig_file, "%s", p);
				while ((p = strtok(nullptr, "~")) != nullptr) {
					fprintf(trig_file, "~~%s", p);
				}
				fprintf(trig_file, "~\n");
			}

			*buf = '\0';
		}
	}

	fprintf(trig_file, "$\n$\n");
	fclose(trig_file);

	sprintf(buf, "%s/%d.trg", TRG_PREFIX, zone);
	remove(buf);
	rename(fname, buf);
#ifndef _WIN32
	if (chmod(buf, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0) {
		std::stringstream ss;
		ss << "Error chmod file: " << buf << " (" << __FILE__ << " "<< __func__ << "  "<< __LINE__ << ")";
		mudlog(ss.str(), BRF, kLvlGod, SYSLOG, true);
	}
#endif

	trigedit_create_index(zone, "trg");
	return true;
}

void trigedit_create_index(int znum, const char *type) {
	FILE *newfile, *oldfile;
	char new_name[32], old_name[32];
	int num, found = false;

	const char *prefix = TRG_PREFIX;

	sprintf(old_name, "%s/index", prefix);
	sprintf(new_name, "%s/newindex", prefix);

	if (!(oldfile = fopen(old_name, "r"))) {
		snprintf(buf1, kMaxStringLength, "SYSERR: TRIGEDIT: Failed to open %s", buf);
		mudlog(buf1, BRF, kLvlImplementator, SYSLOG, true);
		return;
	} else if (!(newfile = fopen(new_name, "w"))) {
		snprintf(buf1, kMaxStringLength, "SYSERR: TRIGEDIT: Failed to open %s", buf);
		mudlog(buf1, BRF, kLvlImplementator, SYSLOG, true);
		return;
	}

	// Index contents must be in order: search through the old file for the
	// right place, insert the new file, then copy the rest over.
	sprintf(buf1, "%d.%s", znum, type);
	while (get_line(oldfile, buf)) {
		if (*buf == '$') {
			fprintf(newfile, "%s\n$\n", (!found ? buf1 : "$"));
			break;
		} else if (!found) {
			sscanf(buf, "%d", &num);
			if (num == znum)
				found = true;
			else if (num > znum) {
				found = true;
				fprintf(newfile, "%s\n", buf1);
			}
		}
		fprintf(newfile, "%s\n", buf);
	}

	fclose(newfile);
	fclose(oldfile);

	// * Out with the old, in with the new.
	remove(old_name);
	rename(new_name, old_name);
}


//****************************************************************


/**************************************************************************
 *  Редактирование ПРОТОТИПОВ СКРИПТОВ
 *  trigedit
 **************************************************************************/


void dg_olc_script_free(DescriptorData *d)
//   Удаление прототипа в OLC_SCRIPT
{
	OLC_SCRIPT(d).clear();
}

void dg_olc_script_copy(DescriptorData *d)
//   Создание копии прототипа скрипта для текщего редактируемого моба/объекта/комнаты
{
	switch (OLC_ITEM_TYPE(d)) {
		case MOB_TRIGGER: OLC_SCRIPT(d) = *OLC_MOB(d)->proto_script;
			break;

		case OBJ_TRIGGER: OLC_SCRIPT(d) = OLC_OBJ(d)->get_proto_script();
			break;

		default: OLC_SCRIPT(d) = *OLC_ROOM(d)->proto_script;
	}
}

void dg_script_menu(DescriptorData *d) {
	int i = 0;

	// make sure our input parser gets used
	OLC_MODE(d) = OLC_SCRIPT_EDIT;
	OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;

#if defined(CLEAR_SCREEN)    // done wierd to compile with the vararg send()
#define FMT    "[H[J     Script Editor\r\n\r\n     Trigger List:\r\n"
#else
#define FMT    "     Script Editor\r\n\r\n     Trigger List:\r\n"
#endif

	SendMsgToChar(FMT, d->character.get());
#undef FMT

	for (const auto trigger_vnum : OLC_SCRIPT(d)) {
		sprintf(buf, "     %2d) [%s%d%s] %s%s%s", ++i, cyn,
				trigger_vnum, nrm, cyn, trig_index[GetTriggerRnum(trigger_vnum)]->proto->get_name().c_str(), nrm);
		SendMsgToChar(buf, d->character.get());
		if (trig_index[GetTriggerRnum(trigger_vnum)]->proto->get_attach_type() != OLC_ITEM_TYPE(d)) {
			sprintf(buf, "   %s** Mis-matched Trigger Type **%s\r\n", grn, nrm);
		} else {
			sprintf(buf, "\r\n");
		}
		SendMsgToChar(buf, d->character.get());
	}

	if (i == 0) {
		SendMsgToChar("     <none>\r\n", d->character.get());
	}

	sprintf(buf, "\r\n"
				 " %sN%s)  Новый триггер для этого скрипта\r\n"
				 " %sD%s)  Удалить триггер в этом скрипте\r\n"
				 " %sX%s)  Выйти из редактора скрипта\r\n"
				 " %sQ%s)  Выйти из редактора скрипта (без сохранения) \r\n\r\n"
				 "     Введите выбранное :", grn, nrm, grn, nrm, grn, nrm, grn, nrm);
	SendMsgToChar(buf, d->character.get());
}

int dg_script_edit_parse(DescriptorData *d, char *arg) {
	int count, pos, vnum;

	switch (OLC_SCRIPT_EDIT_MODE(d)) {
		case SCRIPT_MAIN_MENU:
			switch (tolower(*arg)) {
				case 'x':
					if (OLC_ITEM_TYPE(d) == MOB_TRIGGER) {
						OLC_SCRIPT(d).swap(*OLC_MOB(d)->proto_script);
					} else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER) {
						OLC_OBJ(d)->swap_proto_script(OLC_SCRIPT(d));
					} else {
						OLC_SCRIPT(d).swap(*OLC_ROOM(d)->proto_script);
					}
					// тут break не нужен

					// fall through
				case 'q': dg_olc_script_free(d);
					return 0;

				case 'n': SendMsgToChar("\r\nPlease enter position, vnum   (ex: 1, 200):", d->character.get());
					OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_NEW_TRIGGER;
					break;

				case 'd': SendMsgToChar("     Which entry should be deleted?  0 to abort :", d->character.get());
					OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_DEL_TRIGGER;
					break;

				default: dg_script_menu(d);
					break;
			}
			return 1;

		case SCRIPT_NEW_TRIGGER: vnum = -1;
			count = sscanf(arg, "%d, %d", &pos, &vnum);
			if (count == 1) {
				vnum = pos;
				pos = 999;
			}

			if (pos <= 0) {
				break;    // this aborts a new trigger entry
			}

			if (vnum == 0) {
				break;    // this aborts a new trigger entry
			}

			if (GetTriggerRnum(vnum) < 0) {
				SendMsgToChar("Неверный VNUM триггера!\r\n"
							 "Пожалуйста, выберите позицию, vnum   (ex: 1, 200):", d->character.get());
				return 1;
			}

			{
				// add the new info in position
				auto t = OLC_SCRIPT(d).begin();
				while (--pos && t != OLC_SCRIPT(d).end()) {
					++t;
				}
				owner_trig[*t][-1].insert(OLC_NUM(d));
				OLC_SCRIPT(d).insert(t, vnum);
				OLC_VAL(d)++;
			}
			break;

		case SCRIPT_DEL_TRIGGER: pos = atoi(arg);
			if (pos <= 0) {
				break;
			}

			{
				auto t = OLC_SCRIPT(d).begin();
				while (--pos && t != OLC_SCRIPT(d).end()) {
					++t;
				}
				if (t != OLC_SCRIPT(d).end()) {
					owner_trig[*t][-1].erase(OLC_NUM(d));
					if (owner_trig[*t][-1].empty()) {
						owner_trig[*t].erase(-1);
					}
					OLC_SCRIPT(d).erase(t);
				}
			}

			break;
	}

	dg_script_menu(d);
	return 1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
