#include "boot.data.files.hpp"

#include "object.prototypes.hpp"
#include "logger.hpp"
#include "dg_scripts.h"
#include "dg_olc.h"
#include "boards.h"
#include "constants.h"
#include "room.hpp"
#include "description.h"
#include "im.h"
#include "char.hpp"
#include "help.hpp"
#include "dg_db_scripts.hpp"
#include "utils.h"

#include <boost/algorithm/string/trim.hpp>

#include <iostream>

extern int scheck;						// TODO: get rid of this line
extern const char *unused_spellname;	// TODO: get rid of this line
CHAR_DATA *mob_proto;					// TODO: get rid of this global variable

class DataFile : public BaseDataFile
{
protected:
	DataFile(const std::string& file_name) : m_file(nullptr), m_file_name(file_name) {}
	virtual bool open() override;
	virtual void close() override;
	const std::string& file_name() const { return m_file_name; }
	const auto& file() const { return m_file; }
	void get_one_line(char *buf);

private:
	FILE* m_file;
	std::string m_file_name;
};

bool DataFile::open()
{
	const std::string file_name = full_file_name();
	m_file = fopen(file_name.c_str(), "r");
	if (nullptr == m_file)
	{
		log("SYSERR: %s: %s", file_name.c_str(), strerror(errno));
		exit(1);
	}
	return true;
}

void DataFile::close()
{
	fclose(m_file);
}

void DataFile::get_one_line(char *buf)
{
	if (fgets(buf, READ_SIZE, file()) == NULL)
	{
		mudlog("SYSERR: error reading help file: not terminated with $?", DEF, LVL_IMMORT, SYSLOG, TRUE);
		buf[0] = '$';
		buf[1] = 0;
		return;
	}
	buf[strlen(buf) - 1] = '\0';	// take off the trailing \n
}

class DiscreteFile : public DataFile
{
public:
	DiscreteFile(const std::string& file_name) : DataFile(file_name) {}

	virtual bool load() override { return discrete_load(mode()); }
	virtual std::string full_file_name() const override { return prefixes(mode()) + file_name(); }

protected:
	bool discrete_load(const EBootType mode);
	virtual void read_next_line(const int nr);

	/**
	* Reads and allocates space for a '~'-terminated string from a given file
	* '~' character might be escaped: double '~' will be substituted to a single '~'.
	*/
	char* fread_string();

	char fread_letter(FILE * fp);

	/// for mobs and rooms
	void dg_read_trigger(void *proto, int type);

	char m_line[256];

private:
	virtual EBootType mode() const = 0;
	virtual void read_entry(const int nr) = 0;

	/// modes positions correspond to DB_BOOT_xxx in db.h
	static const char *s_modes[];
};

const char *DiscreteFile::s_modes[] = { "world", "mob", "obj", "ZON", "SHP", "HLP", "trg" };

bool DiscreteFile::discrete_load(const EBootType mode)
{
	int nr = -1, last;

	for (;;)
	{
		read_next_line(nr);

		if (*m_line == '$')
		{
			return true;
		}
		// This file create ADAMANT MUD ETITOR ?
		if (strcmp(m_line, "#ADAMANT") == 0)
		{
			continue;
		}

		if (*m_line == '#')
		{
			last = nr;
			if (sscanf(m_line, "#%d", &nr) != 1)
			{
				log("SYSERR: Format error after %s #%d", s_modes[mode], last);
				exit(1);
			}

			if (nr == 1)
			{
				log("SYSERR: Entity with vnum 1, filename=%s", file_name().c_str());
				exit(1);
			}

			if (nr >= MAX_PROTO_NUMBER)
			{
				return true;	// TODO: we need to return false here, but I don't know how to react on this for now.
			}

			read_entry(nr);
		}
		else
		{
			log("SYSERR: Format error in %s file %s near %s #%d", s_modes[mode], file_name().c_str(), s_modes[mode], nr);
			log("SYSERR: ... offending line: '%s'", m_line);
			exit(1);
		}
	}

	return true;
}

void DiscreteFile::read_next_line(const int nr)
{
	if (!get_line(file(), m_line))
	{
		if (nr == -1)
		{
			log("SYSERR: %s file %s is empty!", s_modes[mode()], file_name().c_str());
		}
		else
		{
			log("SYSERR: Format error in %s after %s #%d\n"
				"...expecting a new %s, but file ended!\n"
				"(maybe the file is not terminated with '$'?)", file_name().c_str(),
				s_modes[mode()], nr, s_modes[mode()]);
		}
		exit(1);
	}
}

char* DiscreteFile::fread_string()
{
	char buffer[1 + MAX_STRING_LENGTH];
	char* to = buffer;

	bool done = false;
	int remained = MAX_STRING_LENGTH;
	const char* end = buffer + MAX_STRING_LENGTH;
	while (!done
		&& fgets(to, remained, file()))
	{
		const char* chunk_beginning = to;
		const char* from = to;
		while (end != from)
		{
			if ('~' == from[0])
			{
				if (1 + from != end
					&& '~' == from[1])
				{
					++from;	//	skip escaped '~'
				}
				else
				{
					done = true;
					break;
				}
			}

			const char c = *(from++);
			if ('\0' == c)
			{
				break;
			}
			*(to++) = c;
		}

		remained -= to - chunk_beginning;
	}
	*to = '\0';

	return strdup(buffer);
}

char DiscreteFile::fread_letter(FILE * fp)
{
	char c;
	do
	{
		c = getc(fp);
	} while (isspace(c));

	return c;
}

void DiscreteFile::dg_read_trigger(void *proto, int type)
{
	char line[256];
	char junk[8];
	int vnum, rnum, count;
	CHAR_DATA *mob;
	ROOM_DATA *room;

	get_line(file(), line);
	count = sscanf(line, "%s %d", junk, &vnum);

	if (count != 2)  	// should do a better job of making this message
	{
		log("SYSERR: Error assigning trigger!");
		return;
	}

	rnum = real_trigger(vnum);
	if (rnum < 0)
	{
		sprintf(line, "SYSERR: Trigger vnum #%d asked for but non-existant!", vnum);
		log("%s", line);
		return;
	}

	switch (type)
	{
	case MOB_TRIGGER:
		mob = (CHAR_DATA *)proto;
		mob->proto_script->push_back(vnum);
		if (owner_trig.find(vnum) == owner_trig.end())
		{
			owner_to_triggers_map_t tmp_map;
			owner_trig.emplace(vnum, tmp_map);
		}
		add_trig_to_owner(-1, vnum, GET_MOB_VNUM(mob));
		break;

	case WLD_TRIGGER:
		room = (ROOM_DATA *)proto;
		room->proto_script->push_back(vnum);

		if (rnum >= 0)
		{
			const auto trigger_instance = read_trigger(rnum);
			add_trigger(SCRIPT(room).get(), trigger_instance, -1);

			// для начала определяем, есть ли такой внум у нас в контейнере
			if (owner_trig.find(vnum) == owner_trig.end())
			{
				owner_to_triggers_map_t tmp_map;
				owner_trig.emplace(vnum, tmp_map);
			}
			add_trig_to_owner(-1, vnum, room->number);
		}
		else
		{
			sprintf(line, "SYSERR: non-existant trigger #%d assigned to room #%d", vnum, room->number);
			log("%s", line);
		}
		break;

	default:
		sprintf(line, "SYSERR: Trigger vnum #%d assigned to non-mob/obj/room", vnum);
		log("%s", line);
	}
}

class TriggersFile : public DiscreteFile
{
public:
	TriggersFile(const std::string& file_name) : DiscreteFile(file_name) {}

	virtual EBootType mode() const override { return DB_BOOT_TRG; }

	static shared_ptr create(const std::string& file_name) { return shared_ptr(new TriggersFile(file_name)); }

private:
	virtual void read_entry(const int nr) override;
	void parse_trigger(int nr);
};

void TriggersFile::read_entry(const int nr)
{
	parse_trigger(nr);
}

void TriggersFile::parse_trigger(int nr)
{
	int t[2], k, indlev;

	char line[256], *cmds, flags[256], *s;

	sprintf(buf2, "trig vnum %d", nr);
	const auto trigger_name = fread_string();
	get_line(file(), line);

	int attach_type = 0;
	k = sscanf(line, "%d %s %d", &attach_type, flags, t);

	if (0 > attach_type
			|| 2 < attach_type)
	{
		log("ERROR: Script with VNUM %d has attach_type %d. Read from line '%s'.",
				nr, attach_type, line);
		attach_type = 0;
	}

	int trigger_type = 0;
	asciiflag_conv(flags, &trigger_type);

	const auto rnum = top_of_trigt;
	TRIG_DATA *trig = new TRIG_DATA(rnum, trigger_name, static_cast<byte>(attach_type), trigger_type);

	trig->narg = (k == 3) ? t[0] : 0;
	trig->arglist = fread_string();
	s = cmds = fread_string();

	trig->cmdlist->reset(new cmdlist_element());
	const auto& cmdlist = *trig->cmdlist;
	const auto cmd_token = strtok(s, "\n\r");
	cmdlist->cmd = cmd_token ? cmd_token : "";

	indlev = 0;
	indent_trigger(cmdlist->cmd, &indlev);

	auto cle = cmdlist;

	while ((s = strtok(NULL, "\n\r")))
	{
		cle->next.reset(new cmdlist_element());
		cle = cle->next;
		cle->cmd = s;
		indent_trigger(cle->cmd, &indlev);
	}

	if (indlev > 0)
	{
		char tmp[MAX_INPUT_LENGTH];
		snprintf(tmp, sizeof(tmp), "Positive indent-level on trigger #%d end.", nr);
		log("%s", tmp);
		Boards::dg_script_text += tmp + std::string("\r\n");
	}

	free(cmds);

	add_trig_index_entry(nr, trig);
}

class WorldFile : public DiscreteFile
{
public:
	WorldFile(const std::string& file_name) : DiscreteFile(file_name) {}

	virtual bool load() override;
	virtual EBootType mode() const override { return DB_BOOT_WLD; }

	static shared_ptr create(const std::string& file_name) { return shared_ptr(new WorldFile(file_name)); }

private:
	virtual void read_entry(const int nr) override;

	/// loads rooms
	void parse_room(int virtual_nr, const int virt);

	/// reads direction data
	void setup_dir(int room, unsigned dir);
};

void WorldFile::read_entry(const int nr)
{
	parse_room(nr, FALSE);
}

void WorldFile::parse_room(int virtual_nr, const int virt)
{
	static int room_nr = FIRST_ROOM, zone = 0;
	int t[10], i;
	char line[256], flags[128];
	char letter;

	if (virt)
	{
		virtual_nr = zone_table[zone].top;
	}

	sprintf(buf2, "room #%d%s", virtual_nr, virt ? "(virt)" : "");

	if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1))
	{
		log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
		exit(1);
	}
	while (virtual_nr > zone_table[zone].top)
	{
		if (++zone > top_of_zone_table)
		{
			log("SYSERR: Room %d is outside of any zone.", virtual_nr);
			exit(1);
		}
	}
	// Создаем новую комнату
	world.push_back(new ROOM_DATA);

	world[room_nr]->zone = zone;
	world[room_nr]->number = virtual_nr;
	if (virt)
	{
		world[room_nr]->name = str_dup("Виртуальная комната");
		world[room_nr]->description_num = RoomDescription::add_desc("Похоже, здесь вам делать нечего.");
		world[room_nr]->clear_flags();
		world[room_nr]->sector_type = SECT_SECRET;
	}
	else
	{
		// не ставьте тут конструкцию ? : , т.к. gcc >=4.х вызывает в ней fread_string два раза
		world[room_nr]->name = fread_string();
		if (!world[room_nr]->name)
			world[room_nr]->name = str_dup("");

		// тож временная галиматья
		char * temp_buf = fread_string();
		if (!temp_buf)
		{
			temp_buf = str_dup("");
		}
		else
		{
			std::string buffer(temp_buf);
			boost::trim_right_if(buffer, boost::is_any_of(std::string(" _"))); //убираем пробелы в конце строки
			RECREATE(temp_buf, buffer.length() + 1);
			strcpy(temp_buf, buffer.c_str());
		}
		world[room_nr]->description_num = RoomDescription::add_desc(temp_buf);
		free(temp_buf);

		if (!get_line(file(), line))
		{
			log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!", virtual_nr);
			exit(1);
		}

		if (sscanf(line, " %d %s %d ", t, flags, t + 2) != 3)
		{
			log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
			exit(1);
		}
		// t[0] is the zone number; ignored with the zone-file system
		world[room_nr]->flags_from_string(flags);
		world[room_nr]->sector_type = t[2];
	}

	check_room_flags(room_nr);

	// Обнуляем флаги от аффектов и сами аффекты на комнате.
	world[room_nr]->affected_by.clear();
	// Обнуляем базовые параметры (пока нет их загрузки)
	memset(&world[room_nr]->base_property, 0, sizeof(room_property_data));

	// Обнуляем добавочные параметры комнаты
	memset(&world[room_nr]->add_property, 0, sizeof(room_property_data));


	world[room_nr]->func = NULL;
	world[room_nr]->contents = NULL;
	world[room_nr]->track = NULL;
	world[room_nr]->light = 0;	// Zero light sources
	world[room_nr]->fires = 0;
	world[room_nr]->gdark = 0;
	world[room_nr]->glight = 0;
	world[room_nr]->ing_list = NULL;	// ингредиентов нет
	world[room_nr]->proto_script.reset(new OBJ_DATA::triggers_list_t());

	for (i = 0; i < NUM_OF_DIRS; i++)
	{
		world[room_nr]->dir_option[i] = NULL;
	}

	world[room_nr]->ex_description = NULL;
	if (virt)
	{
		top_of_world = room_nr++;
		return;
	}

	sprintf(buf, "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

	for (;;)
	{
		if (!get_line(file(), line))
		{
			log("%s", buf);
			exit(1);
		}
		switch (*line)
		{
		case 'D':
			setup_dir(room_nr, atoi(line + 1));
			break;

		case 'E':
			{
				const EXTRA_DESCR_DATA::shared_ptr new_descr(new EXTRA_DESCR_DATA);
				new_descr->keyword = fread_string();
				new_descr->description = fread_string();
				if (new_descr->keyword && new_descr->description)
				{
					new_descr->next = world[room_nr]->ex_description;
					world[room_nr]->ex_description = new_descr;
				}
				else
				{
					sprintf(buf, "SYSERR: Format error in room #%d (Corrupt extradesc)", virtual_nr);
					log("%s", buf);
				}
			}
			break;

		case 'S':	// end of room
					// DG triggers -- script is defined after the end of the room 'T'
					// Ингредиентная магия -- 'I'
			do
			{
				letter = fread_letter(file());
				ungetc(letter, file());
				switch (letter)
				{
				case 'I':
					get_line(file(), line);
					im_parse(&(world[room_nr]->ing_list), line + 1);
					break;
				case 'T':
					dg_read_trigger(world[room_nr], WLD_TRIGGER);
					break;
				default:
					letter = 0;
					break;
				}
			} while (letter != 0);
			top_of_world = room_nr++;
			return;

		default:
			log("%s", buf);
			exit(1);
		}
	}
}

void WorldFile::setup_dir(int room, unsigned dir)
{
	if (dir >= world[room]->dir_option.size())
	{
		log("SYSERROR : dir=%d (%s:%d)", dir, __FILE__, __LINE__);
		return;
	}

	int t[5];
	char line[256];

	sprintf(buf2, "room #%d, direction D%u", GET_ROOM_VNUM(room), dir);

	world[room]->dir_option[dir].reset(new EXIT_DATA());
	const std::shared_ptr<char> general_description(fread_string(), free);
	world[room]->dir_option[dir]->general_description = general_description.get();

	// парс строки алиаса двери на имя;вининельный падеж, если он есть
	char *alias = fread_string();
	if (alias && *alias)
	{
		std::string buffer(alias);
		std::string::size_type i = buffer.find('|');
		if (i != std::string::npos)
		{
			world[room]->dir_option[dir]->keyword = str_dup(buffer.substr(0, i).c_str());
			world[room]->dir_option[dir]->vkeyword = str_dup(buffer.substr(++i).c_str());
		}
		else
		{
			world[room]->dir_option[dir]->keyword = str_dup(buffer.c_str());
			world[room]->dir_option[dir]->vkeyword = str_dup(buffer.c_str());
		}
	}
	free(alias);

	if (!get_line(file(), line))
	{
		log("SYSERR: Format error, %s", line);
		exit(1);
	}
	int result = sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3);
	if (result == 3)//Polud видимо "старый" формат (20.10.2010), прочитаем в старом
	{
		if (t[0] & 1)
			world[room]->dir_option[dir]->exit_info = EX_ISDOOR;
		else if (t[0] & 2)
			world[room]->dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
		else
			world[room]->dir_option[dir]->exit_info = 0;
		if (t[0] & 4)
			world[room]->dir_option[dir]->exit_info |= EX_HIDDEN;

		world[room]->dir_option[dir]->lock_complexity = 0;
	}
	else if (result == 4)
	{
		world[room]->dir_option[dir]->exit_info = t[0];
		world[room]->dir_option[dir]->lock_complexity = t[3];
	}
	else
	{
		log("SYSERR: Format error, %s", buf2);
		exit(1);
	}

	world[room]->dir_option[dir]->key = t[1];
	world[room]->dir_option[dir]->to_room = t[2];
}

bool WorldFile::load()
{
	const auto result = DiscreteFile::load();
	parse_room(0, TRUE);
	return result;
}

class ObjectFile : public DiscreteFile
{
public:
	ObjectFile(const std::string& file_name) : DiscreteFile(file_name) {}

	virtual EBootType mode() const override { return DB_BOOT_OBJ; }

	static shared_ptr create(const std::string& file_name) { return shared_ptr(new ObjectFile(file_name)); }

private:
	virtual void read_next_line(const int nr) override;
	virtual void read_entry(const int nr) override;

	void parse_object(const int nr);

	/**
	* Extend later to include more checks.
	*
	* TODO: Add checks for unknown bitvectors.
	*/
	bool check_object(OBJ_DATA* obj);

	bool check_object_level(OBJ_DATA * obj, int val);
	bool check_object_spell_number(OBJ_DATA * obj, unsigned val);

	char m_buffer[MAX_STRING_LENGTH];
};

void ObjectFile::read_next_line(const int nr)
{
	if (nr < 0)
	{
		DiscreteFile::read_next_line(nr);
	}
}

void ObjectFile::read_entry(const int nr)
{
	parse_object(nr);
}

void ObjectFile::parse_object(const int nr)
{
	int t[10], j = 0;
	char *tmpptr;
	char f0[256], f1[256], f2[256];

	OBJ_DATA *tobj = new OBJ_DATA(nr);

	// *** Add some initialization fields
	tobj->set_maximum_durability(OBJ_DATA::DEFAULT_MAXIMUM_DURABILITY);
	tobj->set_current_durability(OBJ_DATA::DEFAULT_CURRENT_DURABILITY);
	tobj->set_sex(DEFAULT_SEX);
	tobj->set_timer(OBJ_DATA::DEFAULT_TIMER);
	tobj->set_level(1);
	tobj->set_destroyer(OBJ_DATA::DEFAULT_DESTROYER);

	sprintf(m_buffer, "object #%d", nr);

	// *** string data ***
	const char* aliases = fread_string();
	if (aliases == nullptr)
	{
		log("SYSERR: Null obj name or format error at or near %s", m_buffer);
		exit(1);
	}

	tobj->set_aliases(aliases);
	tmpptr = fread_string();
//	*tmpptr = LOWER(*tmpptr);
	tobj->set_short_description(colorLOW(tmpptr));

	strcpy(buf, tobj->get_short_description().c_str());
	tobj->set_PName(0, colorLOW(buf)); //именительный падеж равен короткому описанию

	for (j = 1; j < CObjectPrototype::NUM_PADS; j++)
	{
		char *str = fread_string();
		tobj->set_PName(j, colorLOW(str));
	}

	tmpptr = fread_string();
	if (tmpptr && *tmpptr)
	{
		colorCAP(tmpptr);
	}
	tobj->set_description(tmpptr ? tmpptr : "");

	auto action_description = fread_string();
	tobj->set_action_description(action_description ? action_description : "");

	if (!get_line(file(), m_line))
	{
		log("SYSERR: Expecting *1th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	int parsed_entries = sscanf(m_line, " %s %d %d %d", f0, t + 1, t + 2, t + 3);
	if (parsed_entries != 4)
	{
		log("SYSERR: Format error in *1th* numeric line (expecting 4 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}

	int skill = 0;
	asciiflag_conv(f0, &skill);
	tobj->set_skill(skill);

	tobj->set_maximum_durability(t[1]);
	tobj->set_current_durability(MIN(t[1], t[2]));
	tobj->set_material(static_cast<OBJ_DATA::EObjectMaterial>(t[3]));

	if (tobj->get_current_durability() > tobj->get_maximum_durability())
	{
		log("SYSERR: Obj_cur > Obj_Max, vnum: %d", nr);
	}

	if (!get_line(file(), m_line))
	{
		log("SYSERR: Expecting *2th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, " %d %d %d %d", t, t + 1, t + 2, t + 3);
	if (parsed_entries != 4)
	{
		log("SYSERR: Format error in *2th* numeric line (expecting 4 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}
	tobj->set_sex(static_cast<ESex>(t[0]));
	int timer = t[1] > 0 ? t[1] : OBJ_DATA::SEVEN_DAYS;
	// шмоток с бесконечным таймером проставленным через olc или текстовый редактор
	// не должно быть
	if (timer == OBJ_DATA::UNLIMITED_TIMER)
	{
		timer--;
		tobj->set_extra_flag(EExtraFlag::ITEM_TICKTIMER);
	}
	tobj->set_timer(timer);
	tobj->set_spell(t[2] < 1 || t[2] > SPELLS_COUNT ? SPELL_NO_SPELL : t[2]);
	tobj->set_level(t[3]);

	if (!get_line(file(), m_line))
	{
		log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, " %s %s %s", f0, f1, f2);
	if (parsed_entries != 3)
	{
		log("SYSERR: Format error in *3th* numeric line (expecting 3 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}

	tobj->load_affect_flags(f0);
	// ** Affects
	tobj->load_anti_flags(f1);
	// ** Miss for ...
	tobj->load_no_flags(f2);
	// ** Deny for ...

	if (!get_line(file(), m_line))
	{
		log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, " %d %s %s", t, f1, f2);
	if (parsed_entries != 3)
	{
		log("SYSERR: Format error in *3th* misc line (expecting 3 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}

	tobj->set_type(static_cast<OBJ_DATA::EObjectType>(t[0]));	    // ** What's a object
	tobj->load_extra_flags(f1);
	// ** Its effects
	int wear_flags = 0;
	asciiflag_conv(f2, &wear_flags);
	tobj->set_wear_flags(wear_flags);
	// ** Wear on ...

	if (!get_line(file(), m_line))
	{
		log("SYSERR: Expecting *5th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, "%s %d %d %d", f0, t + 1, t + 2, t + 3);
	if (parsed_entries != 4)
	{
		log("SYSERR: Format error in *5th* numeric line (expecting 4 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}

	int first_value = 0;
	asciiflag_conv(f0, &first_value);
	tobj->set_val(0, first_value);
	tobj->set_val(1, t[1]);
	tobj->set_val(2, t[2]);
	tobj->set_val(3, t[3]);

	if (!get_line(file(), m_line))
	{
		log("SYSERR: Expecting *6th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, "%d %d %d %d", t, t + 1, t + 2, t + 3);
	if (parsed_entries != 4)
	{
		log("SYSERR: Format error in *6th* numeric line (expecting 4 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}
	tobj->set_weight(t[0]);
	tobj->set_cost(t[1]);
	tobj->set_rent_off(t[2]);
	tobj->set_rent_on(t[3]);

	// check to make sure that weight of containers exceeds curr. quantity
	if (tobj->get_type() == OBJ_DATA::ITEM_DRINKCON
		|| tobj->get_type() == OBJ_DATA::ITEM_FOUNTAIN)
	{
		if (tobj->get_weight() < tobj->get_val(1))
		{
			tobj->set_weight(tobj->get_val(1) + 5);
		}
	}

	// *** extra descriptions and affect fields ***
	strcat(m_buffer, ", after numeric constants\n" "...expecting 'E', 'A', '$', or next object number");
	j = 0;

	for (;;)
	{
		if (!get_line(file(), m_line))
		{
			log("SYSERR: Format error in %s", m_buffer);
			exit(1);
		}
		switch (*m_line)
		{
		case 'E':
			{
				const EXTRA_DESCR_DATA::shared_ptr new_descr(new EXTRA_DESCR_DATA());
				new_descr->keyword = fread_string();
				new_descr->description = fread_string();
				if (new_descr->keyword && new_descr->description)
				{
					new_descr->next = tobj->get_ex_description();
					tobj->set_ex_description(new_descr);
				}
				else
				{
					sprintf(buf, "SYSERR: Format error in %s (Corrupt extradesc)", m_buffer);
					log("%s", buf);
				}
			}
			break;

		case 'A':
			if (j >= MAX_OBJ_AFFECT)
			{
				log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, m_buffer);
				exit(1);
			}

			if (!get_line(file(), m_line))
			{
				log("SYSERR: Format error in 'A' field, %s\n"
					"...expecting 2 numeric constants but file ended!", m_buffer);
				exit(1);
			}

			parsed_entries = sscanf(m_line, " %d %d ", t, t + 1);
			if (parsed_entries != 2)
			{
				log("SYSERR: Format error in 'A' field, %s\n"
					"...expecting 2 numeric arguments, got %d\n"
					"...offending line: '%s'", m_buffer, parsed_entries, m_line);
				exit(1);
			}

			tobj->set_affected(j, static_cast<EApplyLocation>(t[0]), t[1]);
			++j;
			break;

		case 'T':	// DG triggers
			dg_obj_trigger(m_line, tobj);
			break;

		case 'M':
			tobj->set_max_in_world(atoi(m_line + 1));
			break;

		case 'R':
			tobj->set_minimum_remorts(atoi(m_line + 1));
			break;

		case 'S':
			if (!get_line(file(), m_line))
			{
				log("SYSERR: Format error in 'S' field, %s\n"
					"...expecting 2 numeric constants but file ended!", m_buffer);
				exit(1);
			}

			parsed_entries = sscanf(m_line, " %d %d ", t, t + 1);
			if (parsed_entries != 2)
			{
				log("SYSERR: Format error in 'S' field, %s\n"
					"...expecting 2 numeric arguments, got %d\n"
					"...offending line: '%s'", m_buffer, parsed_entries, m_line);
				exit(1);
			}
			tobj->set_skill(t[0], t[1]);
			break;

		case 'V':
			tobj->init_values_from_zone(m_line + 1);
			break;

		case '$':
		case '#':
			check_object(tobj);		// Anton Gorev (2015/12/29): do we need the result of this check?
			obj_proto.add(tobj, nr);
			return;

		default:
			log("SYSERR: Format error in %s", m_buffer);
			exit(1);
		}
	}
}

bool ObjectFile::check_object(OBJ_DATA* obj)
{
	bool error = false;

	if (GET_OBJ_WEIGHT(obj) < 0)
	{
		error = true;
		log("SYSERR: Object #%d (%s) has negative weight (%d).",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_WEIGHT(obj));
	}

	if (GET_OBJ_RENT(obj) <= 0)
	{
		error = true;
		log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_RENT(obj));
	}

	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf);
	if (strstr(buf, "UNDEFINED"))
	{
		error = true;
		log("SYSERR: Object #%d (%s) has unknown wear flags.", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
	}

	GET_OBJ_EXTRA(obj).sprintbits(extra_bits, buf, ",",4);
	if (strstr(buf, "UNDEFINED"))
	{
		error = true;
		log("SYSERR: Object #%d (%s) has unknown extra flags.", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
	}

	obj->get_affect_flags().sprintbits(affected_bits, buf, ",",4);

	if (strstr(buf, "UNDEFINED"))
	{
		error = true;
		log("SYSERR: Object #%d (%s) has unknown affection flags.", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
	}

	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_DRINKCON:
	case OBJ_DATA::ITEM_FOUNTAIN:
		if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0))
		{
			error = true;
			log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
				GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
		}
		break;

	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
		error = error || check_object_level(obj, 0);
		error = error || check_object_spell_number(obj, 1);
		error = error || check_object_spell_number(obj, 2);
		error = error || check_object_spell_number(obj, 3);
		break;

	case OBJ_DATA::ITEM_BOOK:
		error = error || check_object_spell_number(obj, 1);
		break;

	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		error = error || check_object_level(obj, 0);
		error = error || check_object_spell_number(obj, 3);
		if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1))
		{
			error = true;
			log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
				GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
		}
		break;

	default:
		break;
	}

	obj->remove_incorrect_values_keys(GET_OBJ_TYPE(obj));
	return error;
}

bool ObjectFile::check_object_level(OBJ_DATA * obj, int val)
{
	bool error = false;

	if (GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL)
	{
		error = true;
		log("SYSERR: Object #%d (%s) has out of range level #%d.",
			GET_OBJ_VNUM(obj),
			obj->get_short_description().c_str(),
			GET_OBJ_VAL(obj, val));
	}
	return error;
}

bool ObjectFile::check_object_spell_number(OBJ_DATA * obj, unsigned val)
{
	if (val >= OBJ_DATA::VALS_COUNT)
	{
		log("SYSERROR : val=%d (%s:%d)", val, __FILE__, __LINE__);
		return true;
	}

	bool error = false;
	const char *spellname;

	if (GET_OBJ_VAL(obj, val) == -1)	// i.e.: no spell
		return error;

	/*
	* Check for negative spells, spells beyond the top define, and any
	* spell which is actually a skill.
	*/
	if (GET_OBJ_VAL(obj, val) < 0)
	{
		error = true;
	}
	if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
	{
		error = true;
	}
	if (error)
	{
		log("SYSERR: Object #%d (%s) has out of range spell #%d.",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, val));
	}

	if (scheck)		// Spell names don't exist in syntax check mode.
	{
		return error;
	}

	// Now check for unnamed spells.
	spellname = spell_name(GET_OBJ_VAL(obj, val));

	if (error
		&& (spellname == unused_spellname
			|| !str_cmp("UNDEFINED", spellname)))
	{
		log("SYSERR: Object #%d (%s) uses '%s' spell #%d.", GET_OBJ_VNUM(obj),
			obj->get_short_description().c_str(), spellname, GET_OBJ_VAL(obj, val));
	}

	return error;
}

class MobileFile : public DiscreteFile
{
public:
	MobileFile(const std::string& file_name) : DiscreteFile(file_name) {}

	virtual EBootType mode() const override { return DB_BOOT_MOB; }

	static shared_ptr create(const std::string& file_name) { return shared_ptr(new MobileFile(file_name)); }

private:
	virtual void read_entry(const int nr) override;

	void parse_mobile(const int nr);
	void parse_simple_mob(int i, int nr);
	void parse_enhanced_mob(int i, int nr);
	void parse_espec(char *buf, int i, int nr);
	void interpret_espec(const char *keyword, const char *value, int i, int nr);
};

void MobileFile::read_entry(const int nr)
{
	parse_mobile(nr);
}

void MobileFile::parse_mobile(const int nr)
{
	static int i = 0;
	int j, t[10];
	char line[256], letter;
	char f1[128], f2[128];
	char *str;
	mob_index[i].vnum = nr;
	mob_index[i].number = 0;
	mob_index[i].func = NULL;
	mob_index[i].set_idx = -1;

	sprintf(buf2, "mob vnum %d", nr);

	mob_proto[i].player_specials = player_special_data::s_for_mobiles;

	// **** String data
	char *tmp_str = fread_string();
	mob_proto[i].set_pc_name(tmp_str);
	if (tmp_str)
	{
		free(tmp_str);
	}
	tmp_str = fread_string();
	mob_proto[i].set_npc_name(tmp_str);
	if (tmp_str)
	{
		free(tmp_str);
	}

	// real name
	CREATE(GET_PAD(mob_proto + i, 0), mob_proto[i].get_npc_name().size() + 1);
	strcpy(GET_PAD(mob_proto + i, 0), mob_proto[i].get_npc_name().c_str());
	for (j = 1; j < CObjectPrototype::NUM_PADS; j++)
	{
		GET_PAD(mob_proto + i, j) = fread_string();
	}
	str = fread_string();
	mob_proto[i].player_data.long_descr = colorCAP(str);
	mob_proto[i].player_data.description = fread_string();
	mob_proto[i].mob_specials.Questor = NULL;
	mob_proto[i].player_data.title = NULL;
	mob_proto[i].set_level(1);

	// *** Numeric data ***
	if (!get_line(file(), line))
	{
		log("SYSERR: Format error after string section of mob #%d\n"
			"...expecting line of form '# # # {S | E}', but file ended!\n%s", nr, line);
		exit(1);
	}

	if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4)
	{
		log("SYSERR: Format error after string section of mob #%d\n"
			"...expecting line of form '# # # {S | E}'\n%s", nr, line);
		exit(1);
	}
	MOB_FLAGS(&mob_proto[i]).from_string(f1);
	MOB_FLAGS(&mob_proto[i]).set(MOB_ISNPC);
	AFF_FLAGS(&mob_proto[i]).from_string(f2);
	GET_ALIGNMENT(mob_proto + i) = t[2];
	switch (UPPER(letter))
	{
	case 'S':		// Simple monsters
		parse_simple_mob(i, nr);
		break;

	case 'E':		// Circle3 Enhanced monsters
		parse_enhanced_mob(i, nr);
		break;

		// add new mob types here..
	default:
		log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
		exit(1);
	}

	// DG triggers -- script info follows mob S/E section
	// DG triggers -- script is defined after the end of the room 'T'
	// Ингредиентная магия -- 'I'
	// Объекты загружаемые по-смертно -- 'D'

	do
	{
		letter = fread_letter(file());
		ungetc(letter, file());
		switch (letter)
		{
		case 'I':
			get_line(file(), line);
			im_parse(&mob_proto[i].ing_list, line + 1);
			break;

		case 'L':
			get_line(file(), line);
			dl_parse(&mob_proto[i].dl_list, line + 1);
			break;

		case 'T':
			dg_read_trigger(&mob_proto[i], MOB_TRIGGER);
			break;

		default:
			letter = 0;
			break;
		}
	} while (letter != 0);

	for (j = 0; j < NUM_WEARS; j++)
	{
		mob_proto[i].equipment[j] = NULL;
	}

	mob_proto[i].set_rnum(i);
	mob_proto[i].desc = NULL;

	set_test_data(mob_proto + i);

	top_of_mobt = i++;
}

void MobileFile::parse_simple_mob(int i, int nr)
{
	int j, t[10];
	char line[256];

	mob_proto[i].set_str(11);
	mob_proto[i].set_int(11);
	mob_proto[i].set_wis(11);
	mob_proto[i].set_dex(11);
	mob_proto[i].set_con(11);
	mob_proto[i].set_cha(11);

	if (!get_line(file(), line))
	{
		log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
		exit(1);
	}
	if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
		t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9)
	{
		log("SYSERR: Format error in mob #%d, 1th line\n" "...expecting line of form '# # # #d#+# #d#+#'", nr);
		exit(1);
	}

	mob_proto[i].set_level(t[0]);
	mob_proto[i].real_abils.hitroll = 20 - t[1];
	mob_proto[i].real_abils.armor = 10 * t[2];

	// max hit = 0 is a flag that H, M, V is xdy+z
	GET_MAX_HIT(mob_proto + i) = 0;
	GET_MEM_TOTAL(mob_proto + i) = t[3];
	GET_MEM_COMPLETED(mob_proto + i) = t[4];
	mob_proto[i].points.hit = t[5];

	mob_proto[i].points.move = 100;
	mob_proto[i].points.max_move = 100;

	mob_proto[i].mob_specials.damnodice = t[6];
	mob_proto[i].mob_specials.damsizedice = t[7];
	mob_proto[i].real_abils.damroll = t[8];

	if (!get_line(file(), line))
	{
		log("SYSERR: Format error in mob #%d, 2th line\n"
			"...expecting line of form '#d#+# #', but file ended!", nr);
		exit(1);
	}
	if (sscanf(line, " %dd%d+%d %d", t, t + 1, t + 2, t + 3) != 4)
	{
		log("SYSERR: Format error in mob #%d, 2th line\n" "...expecting line of form '#d#+# #'", nr);
		exit(1);
	}

	mob_proto[i].set_gold(t[2], false);
	GET_GOLD_NoDs(mob_proto + i) = t[0];
	GET_GOLD_SiDs(mob_proto + i) = t[1];
	mob_proto[i].set_exp(t[3]);

	if (!get_line(file(), line))
	{
		log("SYSERR: Format error in 3th line of mob #%d\n"
			"...expecting line of form '# # #', but file ended!", nr);
		exit(1);
	}

	switch (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3))
	{
	case 3:
		mob_proto[i].mob_specials.speed = -1;
		break;
	case 4:
		mob_proto[i].mob_specials.speed = t[3];
		break;
	default:
		log("SYSERR: Format error in 3th line of mob #%d\n" "...expecting line of form '# # # #'", nr);
		exit(1);
	}

	mob_proto[i].char_specials.position = t[0];
	mob_proto[i].mob_specials.default_pos = t[1];
	mob_proto[i].player_data.sex = static_cast<ESex>(t[2]);

	mob_proto[i].player_data.Race = NPC_RACE_BASIC;
	mob_proto[i].set_class(NPC_CLASS_BASE);
	mob_proto[i].player_data.weight = 200;
	mob_proto[i].player_data.height = 198;

	/*
	* these are now save applies; base save numbers for MOBs are now from
	* the warrior save table.
	*/
	for (j = 0; j < SAVING_COUNT; j++)
		GET_SAVE(mob_proto + i, j) = 0;
}

void MobileFile::parse_enhanced_mob(int i, int nr)
{
	char line[256];

	parse_simple_mob(i, nr);

	while (get_line(file(), line))
	{
		if (!strcmp(line, "E"))	// end of the enhanced section
			return;
		else if (*line == '#')  	// we've hit the next mob, maybe?
		{
			log("SYSERR: Unterminated E section in mob #%d", nr);
			exit(1);
		}
		else
		{
			parse_espec(line, i, nr);
		}
	}

	log("SYSERR: Unexpected end of file reached after mob #%d", nr);
	exit(1);
}

void MobileFile::parse_espec(char *buf, int i, int nr)
{
	char *ptr;

	if ((ptr = strchr(buf, ':')) != NULL)
	{
		*(ptr++) = '\0';
		while (a_isspace(*ptr))
		{
			ptr++;
		}
	}
	else
	{
		ptr = NULL;
	}
	interpret_espec(buf, ptr, i, nr);
}

void MobileFile::interpret_espec(const char *keyword, const char *value, int i, int nr)
{
	struct helper_data_type *helper;
	int k, num_arg, matched = 0, t[7];

	num_arg = atoi(value);

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

	//Added by Adept
	CASE("Resistances")
	{
		if (sscanf(value, "%d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6) != 7)
		{
			log("SYSERROR : Excepted format <# # # # # # #> for RESISTANCES in MOB #%d", i);
			return;
		}
		for (k = 0; k < MAX_NUMBER_RESISTANCE; k++)
			GET_RESIST(mob_proto + i, k) = MIN(300, MAX(-1000, t[k]));
//		заготовка парса резистов у моба при загрузке мада, чтоб в след раз не придумывать
//		if (GET_RESIST(mob_proto + i, 4) > 49 && !mob_proto[i].get_role(MOB_ROLE_BOSS)) // жизнь и не боссы
//		{
//			if (zone_table[world[IN_ROOM(&mob_proto[i])]->zone].group < 3) // в зонах 0-2 группы
//				log("RESIST LIVE num: %d Vnum: %d Level: %d Name: %s", GET_RESIST(mob_proto + i, 4), mob_index[i].vnum, GET_LEVEL(&mob_proto[i]), GET_PAD(&mob_proto[i], 0));
//		}
	}

	CASE("Saves")
	{
		if (sscanf(value, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4)
		{
			log("SYSERROR : Excepted format <# # # #> for SAVES in MOB #%d", i);
			return;
		}
		for (k = 0; k < SAVING_COUNT; k++)
			GET_SAVE(mob_proto + i, k) = MIN(200, MAX(-200, t[k]));
	}

	CASE("HPReg")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.hitreg = num_arg;
	}

	CASE("Armour")
	{
		RANGE(0, 100);
		mob_proto[i].add_abils.armour = num_arg;
	}

	CASE("PlusMem")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.manareg = num_arg;
	}

	CASE("CastSuccess")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.cast_success = num_arg;
	}

	CASE("Success")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.morale_add = num_arg;
	}

	CASE("Initiative")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.initiative_add = num_arg;
	}

	CASE("Absorbe")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.absorb = num_arg;
	}
	CASE("AResist")
	{
		RANGE(0, 100);
		mob_proto[i].add_abils.aresist = num_arg;
	}
	CASE("MResist")
	{
		RANGE(0, 100);
		mob_proto[i].add_abils.mresist = num_arg;
	}
	//End of changed
	// added by WorM (Видолюб) поглощение физ.урона в %
	CASE("PResist")
	{
		RANGE(0, 100);
		mob_proto[i].add_abils.presist = num_arg;
	}
	// end by WorM
	CASE("BareHandAttack")
	{
		RANGE(0, 99);
		mob_proto[i].mob_specials.attack_type = num_arg;
	}

	CASE("Destination")
	{
		if (mob_proto[i].mob_specials.dest_count < MAX_DEST)
		{
			mob_proto[i].mob_specials.dest[mob_proto[i].mob_specials.dest_count] = num_arg;
			mob_proto[i].mob_specials.dest_count++;
		}
	}

	CASE("Str")
	{
		mob_proto[i].set_str(num_arg);
	}

	CASE("StrAdd")
	{
		mob_proto[i].set_str_add(num_arg);
	}

	CASE("Int")
	{
		RANGE(3, 50);
		mob_proto[i].set_int(num_arg);
	}

	CASE("Wis")
	{
		RANGE(3, 50);
		mob_proto[i].set_wis(num_arg);
	}

	CASE("Dex")
	{
		mob_proto[i].set_dex(num_arg);
	}

	CASE("Con")
	{
		mob_proto[i].set_con(num_arg);
	}

	CASE("Cha")
	{
		RANGE(3, 50);
		mob_proto[i].set_cha(num_arg);
	}

	CASE("Size")
	{
		RANGE(0, 100);
		mob_proto[i].real_abils.size = num_arg;
	}
	// *** Extended for Adamant
	CASE("LikeWork")
	{
		RANGE(0, 200);
		mob_proto[i].mob_specials.LikeWork = num_arg;
	}

	CASE("MaxFactor")
	{
		RANGE(0, 255);
		mob_proto[i].mob_specials.MaxFactor = num_arg;
	}

	CASE("ExtraAttack")
	{
		RANGE(0, 255);
		mob_proto[i].mob_specials.ExtraAttack = num_arg;
	}

	CASE("Class")
	{
		RANGE(NPC_CLASS_BASE, NPC_CLASS_LAST);
		mob_proto[i].set_class(num_arg);
	}


	CASE("Height")
	{
		RANGE(0, 200);
		mob_proto[i].player_data.height = num_arg;
	}

	CASE("Weight")
	{
		RANGE(0, 200);
		mob_proto[i].player_data.weight = num_arg;
	}

	CASE("Race")
	{
		RANGE(NPC_RACE_BASIC, NPC_RACE_NEXT - 1);
		mob_proto[i].player_data.Race = num_arg;
	}

	CASE("Special_Bitvector")
	{
		mob_proto[i].mob_specials.npc_flags.from_string((char *)value);
		// *** Empty now
	}

	// Gorrah
	CASE("Feat")
	{
		if (sscanf(value, "%d", t) != 1)
		{
			log("SYSERROR : Excepted format <#> for FEAT in MOB #%d", i);
			return;
		}
		if (t[0] >= MAX_FEATS || t[0] <= 0)
		{
			log("SYSERROR : Unknown feat No %d for MOB #%d", t[0], i);
			return;
		}
		SET_FEAT(mob_proto + i, t[0]);
	}
	// End of changes

	CASE("Skill")
	{
		if (sscanf(value, "%d %d", t, t + 1) != 2)
		{
			log("SYSERROR : Excepted format <# #> for SKILL in MOB #%d", i);
			return;
		}
		if (t[0] > MAX_SKILL_NUM || t[0] < 1)
		{
			log("SYSERROR : Unknown skill No %d for MOB #%d", t[0], i);
			return;
		}
		t[1] = MIN(200, MAX(0, t[1]));
		(mob_proto + i)->set_skill(static_cast<ESkill>(t[0]), t[1]);
	}

	CASE("Spell")
	{
		if (sscanf(value, "%d", t + 0) != 1)
		{
			log("SYSERROR : Excepted format <#> for SPELL in MOB #%d", i);
			return;
		}
		if (t[0] > MAX_SPELLS || t[0] < 1)
		{
			log("SYSERROR : Unknown spell No %d for MOB #%d", t[0], i);
			return;
		}
		GET_SPELL_MEM(mob_proto + i, t[0]) += 1;
		GET_CASTER(mob_proto + i) += (IS_SET(spell_info[t[0]].routines, NPC_CALCULATE) ? 1 : 0);
	}

	CASE("Helper")
	{
		CREATE(helper, 1);
		helper->mob_vnum = num_arg;
		helper->next_helper = GET_HELPER(mob_proto + i);
		GET_HELPER(mob_proto + i) = helper;
	}

	CASE("Role")
	{
		if (value && *value)
		{
			std::string str(value);
			CHAR_DATA::role_t tmp(str);
			tmp.resize(mob_proto[i].get_role().size());
			mob_proto[i].set_role(tmp);
		}
	}

	if (!matched)
	{
		log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d", keyword, nr);
	}
#undef CASE
#undef RANGE
}

class ZoneFile : public DataFile
{
public:
	ZoneFile(const std::string& file_name) : DataFile(file_name) {}

	virtual bool load() override { return load_zones(); }
	virtual std::string full_file_name() const override { return prefixes(DB_BOOT_ZON) + file_name(); }

	static shared_ptr create(const std::string& file_name) { return shared_ptr(new ZoneFile(file_name)); }

protected:
	bool load_zones();
};

bool ZoneFile::load_zones()
{
#define ZCMD zone_table[zone].cmd[cmd_no]
#define Z zone_table[zone]

	static zone_rnum zone = 0;
	int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, a_number = 0, b_number = 0;
	char *ptr, buf[256];
	char t1[80], t2[80];
	//MZ.load
	Z.level = 1;
	Z.type = 0;
	//-MZ.load
	Z.typeA_count = 0;
	Z.typeB_count = 0;
	Z.locked = FALSE;
	Z.reset_idle = FALSE;
	Z.used = FALSE;
	Z.activity = 0;
	Z.comment = 0;
	Z.location = 0;
	Z.description = 0;
	Z.autor = 0;
	Z.group = false;
	Z.count_reset = 0;
	Z.traffic = 0;

	while (get_line(file(), buf))
	{
		ptr = buf;
		skip_spaces(&ptr);
		if (*ptr == 'A')
			Z.typeA_count++;
		if (*ptr == 'B')
			Z.typeB_count++;
		num_of_cmds++;	// this should be correct within 3 or so
	}
	rewind(file());
	if (Z.typeA_count)
	{
		CREATE(Z.typeA_list, Z.typeA_count);
	}
	if (Z.typeB_count)
	{
		CREATE(Z.typeB_list, Z.typeB_count);
		CREATE(Z.typeB_flag, Z.typeB_count);
		// сбрасываем все флаги
		for (b_number = Z.typeB_count; b_number > 0; b_number--)
			Z.typeB_flag[b_number - 1] = FALSE;
	}

	if (num_of_cmds == 0)
	{
		log("SYSERR: %s is empty!", full_file_name().c_str());
		exit(1);
	}
	else
	{
		CREATE(Z.cmd, num_of_cmds);
	}

	line_num += get_line(file(), buf);

	if (sscanf(buf, "#%d", &Z.number) != 1)
	{
		log("SYSERR: Format error in %s, line %d", full_file_name().c_str(), line_num);
		exit(1);
	}
	sprintf(buf2, "beginning of zone #%d", Z.number);

	line_num += get_line(file(), buf);
	if ((ptr = strchr(buf, '~')) != NULL)	// take off the '~' if it's there
		*ptr = '\0';
	Z.name = str_dup(buf);

	log("Читаем zon файл: %s", full_file_name().c_str());
	while (*buf !='S' && !feof(file()))
	{	
		line_num += get_line(file(), buf);
		if (*buf == '#')
			break;
		if (*buf == '^')
		{
			std::string comment(buf);
			boost::trim_if(comment, boost::is_any_of(std::string("^~")));
			Z.comment = str_dup(comment.c_str());
		}
		if (*buf == '&')
		{
			std::string location(buf);
			boost::trim_if(location, boost::is_any_of(std::string("&~")));
			Z.location = str_dup(location.c_str());
		}
		if (*buf == '!')
		{
			std::string autor(buf);
			boost::trim_if(autor, boost::is_any_of(std::string("!~")));
			Z.autor = str_dup(autor.c_str());
		}
		if (*buf == '$')
		{
			std::string description(buf);
			boost::trim_if(description, boost::is_any_of(std::string("$~")));
			Z.description = str_dup(description.c_str());
		}
	}
	if (*buf != '#')
	{
		log("SYSERR: ERROR!!! not # in file %s", full_file_name().c_str());
		exit(1);
	}
	
	int group = 0;
	if (sscanf(buf, "#%d %d %d", &Z.level, &Z.type, &group) < 3)
	{
		if (sscanf(buf, "#%d %d", &Z.level, &Z.type) < 2)
		{
			log("SYSERR: ошибка чтения z.level, z.type, z.group: %s", buf);
			exit(1);
		}
	}
	Z.group = (group == 0) ? 1 : group; //группы в 0 рыл не бывает
	line_num += get_line(file(), buf);

	*t1 = 0;
	*t2 = 0;
	int tmp_reset_idle = 0;
	if (sscanf(buf, " %d %d %d %d %s %s", &Z.top, &Z.lifespan, &Z.reset_mode, &tmp_reset_idle, t1, t2) < 4)
	{
		// если нет четырех констант, то, возможно, это старый формат -- попробуем прочитать три
		if (sscanf(buf, " %d %d %d %s %s", &Z.top, &Z.lifespan, &Z.reset_mode, t1, t2) < 3)
		{
			log("SYSERR: Format error in 3-constant line of %s", full_file_name().c_str());
			exit(1);
		}
	}
	Z.reset_idle = 0 != tmp_reset_idle;
	Z.under_construction = !str_cmp(t1, "test");
	Z.locked = !str_cmp(t2, "locked");
	cmd_no = 0;

	for (;;)
	{
		if ((tmp = get_line(file(), buf)) == 0)
		{
			log("SYSERR: Format error in %s - premature end of file", full_file_name().c_str());
			exit(1);
		}
		line_num += tmp;
		ptr = buf;
		skip_spaces(&ptr);

		if ((ZCMD.command = *ptr) == '*')
		{
			continue;
		}
		ptr++;
		// Новые параметры формата файла:
		// A номер_зоны -- зона типа A из списка
		// B номер_зоны -- зона типа B из списка
		if (ZCMD.command == 'A')
		{
			sscanf(ptr, " %d", &Z.typeA_list[a_number]);
			a_number++;
			continue;
		}
		if (ZCMD.command == 'B')
		{
			sscanf(ptr, " %d", &Z.typeB_list[b_number]);
			b_number++;
			continue;
		}

		if (ZCMD.command == 'S' || ZCMD.command == '$')
		{
			ZCMD.command = 'S';
			break;
		}
		error = 0;
		ZCMD.arg4 = -1;
		if (strchr("MOEGPDTVQF", ZCMD.command) == NULL)  	// a 3-arg command
		{
			if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
				error = 1;
		}
		else if (ZCMD.command == 'V')  	// a string-arg command
		{
			if (sscanf(ptr, " %d %d %d %d %s %s", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, t1, t2) != 6)
				error = 1;
			else
			{
				ZCMD.sarg1 = str_dup(t1);
				ZCMD.sarg2 = str_dup(t2);
			}
		}
		else if (ZCMD.command == 'Q')  	// a number command
		{
			if (sscanf(ptr, " %d %d", &tmp, &ZCMD.arg1) != 2)
				error = 1;
			else
				tmp = 0;
		}
		else
		{
			if (sscanf(ptr, " %d %d %d %d %d", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4) < 4)
				error = 1;
		}

		ZCMD.if_flag = tmp;

		if (error)
		{
			log("SYSERR: Format error in %s, line %d: '%s'", full_file_name().c_str(), line_num, buf);
			exit(1);
		}
		ZCMD.line = line_num;
		cmd_no++;
	}
	top_of_zone_table = zone++;
#undef Z
#undef ZCMD

	return true;
}

class HelpFile : public DataFile
{
public:
	HelpFile(const std::string& file_name) : DataFile(file_name) {}

	virtual bool load() override { return load_help(); }
	virtual std::string full_file_name() const override { return prefixes(DB_BOOT_HLP) + file_name(); }

	static shared_ptr create(const std::string& file_name) { return shared_ptr(new HelpFile(file_name)); }

protected:
	bool load_help();
};

bool HelpFile::load_help()
{
#if defined(CIRCLE_MACINTOSH)
	static char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];	// ?
#else
	char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
#endif
	char line[READ_SIZE + 1];
	const char* scan;

	// get the first keyword line
	get_one_line(key);
	while (*key != '$')  	// read in the corresponding help entry
	{
		strcpy(entry, strcat(key, "\r\n"));
		get_one_line(line);
		while (*line != '#')
		{
			strcat(entry, strcat(line, "\r\n"));
			get_one_line(line);
		}
		// Assign read level
		int min_level = 0;
		if ((*line == '#') && (*(line + 1) != 0))
		{
			min_level = atoi((line + 1));
		}
		min_level = MAX(0, MIN(min_level, LVL_IMPL));
		// now, add the entry to the index with each keyword on the keyword line
		std::string entry_str(entry);
		scan = one_word(key, next_key);
		while (*next_key)
		{
			std::string key_str(next_key);
			HelpSystem::add_static(key_str, entry_str, min_level);
			scan = one_word(scan, next_key);
		}

		// get next keyword line (or $)
		get_one_line(key);
	}

	return true;
}

class SocialsFile : public DataFile
{
public:
	SocialsFile(const std::string& file_name) : DataFile(file_name) {}

	virtual bool load() override { return load_socials(); }
	virtual std::string full_file_name() const override { return prefixes(DB_BOOT_SOCIAL) + file_name(); }

	static shared_ptr create(const std::string& file_name) { return shared_ptr(new SocialsFile(file_name)); }

private:
	bool load_socials();
	char *str_dup_bl(const char *source);
};

bool SocialsFile::load_socials()
{
	char line[MAX_INPUT_LENGTH], next_key[MAX_INPUT_LENGTH];
	const char* scan;
	int key = -1, message = -1, c_min_pos, c_max_pos, v_min_pos, v_max_pos, what;

	// get the first keyword line
	get_one_line(line);
	while (*line != '$')
	{
		message++;
		scan = one_word(line, next_key);
		while (*next_key)
		{
			key++;
			log("Social %d '%s' - message %d", key, next_key, message);
			soc_keys_list[key].keyword = str_dup(next_key);
			soc_keys_list[key].social_message = message;
			scan = one_word(scan, next_key);
		}

		what = 0;
		get_one_line(line);
		while (*line != '#')
		{
			scan = line;
			skip_spaces(&scan);
			if (scan && *scan && *scan != ';')
			{
				switch (what)
				{
				case 0:
					if (sscanf
					(scan, " %d %d %d %d \n", &c_min_pos, &c_max_pos,
						&v_min_pos, &v_max_pos) < 4)
					{
						log("SYSERR: format error in %d social file near social '%s' #d #d #d #d\n", message, line);
						exit(1);
					}
					soc_mess_list[message].ch_min_pos = c_min_pos;
					soc_mess_list[message].ch_max_pos = c_max_pos;
					soc_mess_list[message].vict_min_pos = v_min_pos;
					soc_mess_list[message].vict_max_pos = v_max_pos;
					break;
				case 1:
					soc_mess_list[message].char_no_arg = str_dup_bl(scan);
					break;
				case 2:
					soc_mess_list[message].others_no_arg = str_dup_bl(scan);
					break;
				case 3:
					soc_mess_list[message].char_found = str_dup_bl(scan);
					break;
				case 4:
					soc_mess_list[message].others_found = str_dup_bl(scan);
					break;
				case 5:
					soc_mess_list[message].vict_found = str_dup_bl(scan);
					break;
				case 6:
					soc_mess_list[message].not_found = str_dup_bl(scan);
					break;
				}
			}

			if (!scan || *scan != ';')
			{
				what++;
			}

			get_one_line(line);
		}
		// get next keyword line (or $)
		get_one_line(line);
	}

	return true;
}

char * SocialsFile::str_dup_bl(const char *source)
{
	char line[MAX_INPUT_LENGTH];

	line[0] = 0;
	if (source[0])
	{
		strcat(line, "&K");
		strcat(line, source);
		strcat(line, "&n");
	}

	return (str_dup(line));
}

BaseDataFile::shared_ptr DataFileFactory::get_file(const EBootType mode, const std::string& file_name)
{
	switch (mode)
	{
	case DB_BOOT_WLD:
		return WorldFile::create(file_name);

	case DB_BOOT_MOB:
		return MobileFile::create(file_name);

	case DB_BOOT_OBJ:
		return ObjectFile::create(file_name);

	case DB_BOOT_ZON:
		return ZoneFile::create(file_name);

	case DB_BOOT_HLP:
		return HelpFile::create(file_name);

	case DB_BOOT_TRG:
		return TriggersFile::create(file_name);

	case DB_BOOT_SOCIAL:
		return SocialsFile::create(file_name);

	default:
		return nullptr;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
