#include "boot_data_files.h"

#include "engine/db/obj_prototypes.h"
#include "engine/scripting/dg_olc.h"
#include "gameplay/mechanics/dead_load.h"
#include "gameplay/communication/boards/boards.h"
#include "gameplay/communication/social.h"
#include "engine/db/description.h"
#include "engine/db/help.h"
#include "engine/scripting/dg_db_scripts.h"
#include "engine/db/global_objects.h"

#include "third_party_libs/fmt/include/fmt/format.h"

#include <regex>

extern int scheck;                        // TODO: get rid of this line
CharData *mob_proto;                    // TODO: get rid of this global variable

extern void ExtractTrigger(Trigger *trig);

class DataFile : public BaseDataFile {
 protected:
	DataFile(const std::string &file_name) : m_file(nullptr), m_file_name(file_name) {}
	bool open() override;
	void close() override;
	[[nodiscard]] const std::string &file_name() const { return m_file_name; }
	[[nodiscard]] const auto &file() const { return m_file; }
	void get_one_line(char *buf);

 private:
	FILE *m_file;
	std::string m_file_name;
};

bool DataFile::open() {
	const std::string file_name = full_file_name();
	m_file = fopen(file_name.c_str(), "r");
	if (nullptr == m_file) {
		log("SYSERR: %s: %s", file_name.c_str(), strerror(errno));
		exit(1);
	}
	return true;
}

void DataFile::close() {
	fclose(m_file);
}

void DataFile::get_one_line(char *buf) {
	if (fgets(buf, READ_SIZE, file()) == nullptr) {
		mudlog("SYSERR: error reading help file: not terminated with $?", DEF, kLvlImmortal, SYSLOG, true);
		buf[0] = '$';
		buf[1] = 0;
		return;
	}
	buf[strlen(buf) - 1] = '\0';    // take off the trailing \n
}

class DiscreteFile : public DataFile {
 public:
	DiscreteFile(const std::string &file_name) : DataFile(file_name) {}

	virtual bool load() override { return discrete_load(mode()); }
	virtual std::string full_file_name() const override { return prefixes(mode()) + file_name(); }

 protected:
	bool discrete_load(const EBootType mode);
	virtual void read_next_line(const int nr);

	// Allocates and reads a '~'-terminated string from the file.
	// '~' character might be escaped: double '~' is replaced with a single '~'.
	std::string fread_string();

	char fread_letter(FILE *fp);

	/// for mobs and rooms
	void dg_read_trigger(void *proto, int type, int proto_vnum);

	char m_line[256];

 private:
	virtual EBootType mode() const = 0;
	virtual void read_entry(const int nr) = 0;
};

bool DiscreteFile::discrete_load(const EBootType mode) {
	int nr = -1, last;

	for (;;) {
		read_next_line(nr);

		if (*m_line == '$') {
			return true;
		}
		// This file create ADAMANT MUD ETITOR ?
		if (strcmp(m_line, "#ADAMANT") == 0) {
			continue;
		}

		if (*m_line == '#') {
			last = nr;
			if (sscanf(m_line, "#%d", &nr) != 1) {
				log("SYSERR: Format error after %s #%d", boot_mode_name(mode), last);
				exit(1);
			}

			if (nr == 1) {
				log("SYSERR: Entity with vnum 1, filename=%s", file_name().c_str());
				exit(1);
			}

			if (nr >= kMaxProtoNumber) {
				return true;    // TODO: we need to return false here, but I don't know how to react on this for now.
			}

			read_entry(nr);
		} else {
			log("SYSERR: Format error in %s file %s near %s #%d",
				boot_mode_name(mode),
				file_name().c_str(),
				boot_mode_name(mode),
				nr);
			log("SYSERR: ... offending line: '%s'", m_line);
			exit(1);
		}
	}

	return true;
}

void DiscreteFile::read_next_line(const int nr) {
	if (!get_line(file(), m_line)) {
		if (nr == -1) {
			log("SYSERR: %s file %s is empty!", boot_mode_name(mode()), file_name().c_str());
		} else {
			log("SYSERR: Format error in %s after %s #%d\n"
				"...expecting a new %s, but file ended!\n"
				"(maybe the file is not terminated with '$'?)", file_name().c_str(),
				boot_mode_name(mode()), nr, boot_mode_name(mode()));
		}
		exit(1);
	}
}

std::string DiscreteFile::fread_string(void) {
	char bufferIn[kMaxRawInputLength];
	char bufferOut[kMaxStringLength];
	char *to = bufferOut;
	const char *end = bufferOut + sizeof(bufferOut) - 1;
	bool isEscaped = false;
	bool done = false;

	while (!done
		&& fgets(bufferIn, sizeof(bufferIn), file())
		&& to < end) {
		const char *from = bufferIn;
		char c;

		while ((c = *from++) && (to < end)) {
			if (c == '~') {
				if (isEscaped)
					*to++ = c; // escape sequence ~~ replaced with single ~
				isEscaped = !isEscaped;
			} else {
				if (isEscaped)
					done = true; // '~' followed by something else - terminate
				else {
					if ((c == '\n') && (to == bufferOut || *(to - 1) != '\r')) // NL without preceding CR
					{
						if ((to + 1) < end) {
							*to++ = '\r';
							*to++ = c;
						} else
							done = true; // not enough space for \r\n. Don't put \r or \n alone, terminate
					} else
						*to++ = c;
				}
			}
		}
	}
	*to = '\0';

	return std::string(bufferOut);
}

char DiscreteFile::fread_letter(FILE *fp) {
	char c;
	do {
		c = getc(fp);
	} while (isspace(c));

	return c;
}

void DiscreteFile::dg_read_trigger(void *proto, int type, int proto_vnum) {
	char line[kMaxTrglineLength];
	char junk[8];
	int vnum, rnum, count;
	CharData *mob;
	RoomData *room;

	get_line(file(), line);
	count = sscanf(line, "%s %d", junk, &vnum);

	if (count != 2)    // should do a better job of making this message
	{
		log("SYSERR: Error assigning trigger!");
		return;
	}

	rnum = GetTriggerRnum(vnum);
	if (rnum < 0) {
		sprintf(line, "SYSERR: Trigger vnum #%d asked for but non-existant!", vnum);
		log("%s", line);
		return;
	}

	switch (type) {
		case MOB_TRIGGER: mob = (CharData *) proto;
			mob->proto_script->push_back(vnum);
			add_trig_to_owner(-1, vnum, proto_vnum);
			break;
		case WLD_TRIGGER: room = (RoomData *) proto;
			room->proto_script->push_back(vnum);
			if (rnum >= 0) {
				const auto trigger_instance = read_trigger(rnum);
				if (add_trigger(SCRIPT(room).get(), trigger_instance, -1)) {
					add_trig_to_owner(-1, vnum, room->vnum);
				} else {
					ExtractTrigger(trigger_instance);
				}
			} else {
				sprintf(line, "SYSERR: non-existant trigger #%d assigned to room #%d", vnum, room->vnum);
				log("%s", line);
			}
			break;
		default: sprintf(line, "SYSERR: Trigger vnum #%d assigned to non-mob/obj/room", vnum);
			log("%s", line);
			break;
	}
}

class DataFileFactoryImpl : public DataFileFactory {
 public:
	using regex_ptr_t = std::shared_ptr<std::regex>;

	DataFileFactoryImpl() : m_load_obj_exp(std::make_shared<std::regex>(
		"^\\s*(?:%load%|load|oload|mload|wload)\\s+obj\\s+(\\d+)")) {}

	virtual BaseDataFile::shared_ptr get_file(const EBootType mode, const std::string &file_name) override;

 private:
	const regex_ptr_t m_load_obj_exp;
};

class TriggersFile : public DiscreteFile {
 public:
	TriggersFile(const std::string &file_name, const DataFileFactoryImpl::regex_ptr_t &expression) : DiscreteFile(
		file_name), m_load_obj_exp(expression) {}

	virtual EBootType mode() const override { return DB_BOOT_TRG; }

	static shared_ptr create(const std::string &file_name,
							 const DataFileFactoryImpl::regex_ptr_t &expression) {
		return shared_ptr(new TriggersFile(file_name,
										   expression));
	}

 private:
	virtual void read_entry(const int nr) override;
	void parse_trigger(int nr);

	const DataFileFactoryImpl::regex_ptr_t m_load_obj_exp;
};

void TriggersFile::read_entry(const int nr) {
	parse_trigger(nr);
}

void TriggersFile::parse_trigger(int vnum) {
	int t, add_flag, k;
	char line[256], flags[256];

	ZoneRnum zrn = GetZoneRnum(vnum / 100);

	if (zone_table[zrn].RnumTrigsLocation.first == -1) {
		zone_table[zrn].RnumTrigsLocation.first = top_of_trigt;
	}
	zone_table[zrn].RnumTrigsLocation.second = top_of_trigt;

	sprintf(buf2, "trig vnum %d", vnum);
	std::string name(fread_string());
	get_line(file(), line);

	int attach_type = 0;
	k = sscanf(line, "%d %s %d %d", &attach_type, flags, &t, &add_flag);

	if (0 > attach_type
		|| 2 < attach_type) {
		log("ERROR: Script with VNUM %d has attach_type %d. Read from line '%s'.", vnum, attach_type, line);
		attach_type = 0;
	}

	int trigger_type = 0;
	asciiflag_conv(flags, &trigger_type);
	const auto rnum = top_of_trigt;
	Trigger *trig = new Trigger(rnum, std::move(name), static_cast<byte>(attach_type), trigger_type);

	trig->narg = t;
	if (k == 4)
		trig->add_flag = add_flag;
	else 
		trig->add_flag = false;
	trig->arglist = fread_string();
	std::string cmds(fread_string());
	std::size_t pos = 0;
	int indlev = 0, num = 1;
	auto ptr = trig->cmdlist.get();

	do {
		std::size_t pos_end = cmds.find_first_of("\r\n", pos);
		std::string line = cmds.substr(pos, (pos_end == std::string::npos) ? pos_end : pos_end - pos);
		// exclude empty lines, but always include the last one to make sure the list is not empty
		if (!line.empty()) {
			utils::TrimRight(line);
			ptr->reset(new cmdlist_element());
			indent_trigger(line, &indlev);
			(*ptr)->cmd = line;
			(*ptr)->line_num = num++;
			ptr = &(*ptr)->next;

			std::smatch match;
			if (std::regex_search(line, match, *m_load_obj_exp)) {
				ObjVnum obj_num = std::stoi(match.str(1));
				const auto tlist_it = obj2triggers.find(obj_num);
				if (tlist_it != obj2triggers.end()) {
					//const auto trig_f = std::find(tlist_it->second.begin(), tlist_it->second.end(), top_of_trigt);
					const auto trig_f = std::find(tlist_it->second.begin(), tlist_it->second.end(), vnum);
					if (trig_f == tlist_it->second.end()) {
						//tlist_it->second.push_back(top_of_trigt);
						tlist_it->second.push_back(vnum);
					}
				} else {
					std::list<TrgVnum> tlist = {vnum};
					obj2triggers.emplace(obj_num, tlist);
				}
			}
		}
		if (pos_end != std::string::npos)
			pos_end = pos_end + 1;
		pos = pos_end;
	} while (pos != std::string::npos);

	if (indlev > 0) {
		char tmp[kMaxInputLength];
		snprintf(tmp, sizeof(tmp), "Positive indent-level on trigger #%d end.", vnum);
		log("%s", tmp);
		Boards::dg_script_text += tmp + std::string("\r\n");
	}

	AddTrigIndexEntry(vnum, trig);
}

class WorldFile : public DiscreteFile {
 public:
	WorldFile(const std::string &file_name) : DiscreteFile(file_name) {}

	virtual bool load() override;
	virtual EBootType mode() const override { return DB_BOOT_WLD; }

	static shared_ptr create(const std::string &file_name) { return shared_ptr(new WorldFile(file_name)); }

 private:
	virtual void read_entry(const int nr) override;

	/// loads rooms
	void parse_room(int virtual_nr);

	/// reads direction data
	void setup_dir(int room, unsigned dir);
};

void WorldFile::read_entry(const int nr) {
	parse_room(nr);
}

void WorldFile::parse_room(int virtual_nr) {
	int room_realnum = ++top_of_world;
	static ZoneRnum zone = 0;

	int t[10], i;
	char line[256], flags[128];
	char letter;

	if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
		log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
		exit(1);
	}
//	if (zone == 0 && zone_table[zone].RnumRoomsLocation.first == -1) {
//		zone_table[zone].RnumRoomsLocation.first = 1;
//	}
	while (virtual_nr > zone_table[zone].top) {
		if (++zone >= static_cast<ZoneRnum>(zone_table.size())) {
			log("SYSERR: Room %d is outside of any zone.", virtual_nr);
			exit(1);
		}
	}
	// Создаем новую комнату
	world.push_back(new RoomData);
	world[room_realnum]->zone_rn = zone;
	world[room_realnum]->vnum = virtual_nr;
	std::string tmpstr = fread_string();
	tmpstr[0] = UPPER(tmpstr[0]);
	world[room_realnum]->set_name(tmpstr);
//	if (zone_table[zone].RnumRoomsLocation.first == -1) {
//		zone_table[zone].RnumRoomsLocation.first = room_realnum;
//	}
//	zone_table[zone].RnumRoomsLocation.second = room_realnum;
	
	std::string desc = fread_string();
	utils::TrimRightIf(desc, " _");
	desc.shrink_to_fit();
	world[room_realnum]->description_num = RoomDescription::add_desc(desc);

	if (!get_line(file(), line)) {
		log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!", virtual_nr);
		exit(1);
	}

	if (sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
		log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
		exit(1);
	}
	// t[0] is the zone number; ignored with the zone-file system
	world[room_realnum]->flags_from_string(flags);
	world[room_realnum]->sector_type = t[2];

	CheckRoomForIncompatibleFlags(room_realnum);
	world[room_realnum]->func = nullptr;
	world[room_realnum]->contents = nullptr;
	world[room_realnum]->track = nullptr;
	world[room_realnum]->light = 0;    // Zero light sources
	world[room_realnum]->fires = 0;
	world[room_realnum]->gdark = 0;
	world[room_realnum]->glight = 0;
	world[room_realnum]->proto_script.reset(new ObjData::triggers_list_t());

	for (i = 0; i < EDirection::kMaxDirNum; i++) {
		world[room_realnum]->dir_option[i] = nullptr;
	}

	world[room_realnum]->ex_description = nullptr;

	sprintf(buf, "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

	for (;;) {
		if (!get_line(file(), line)) {
			log("%s", buf);
			exit(1);
		}
		switch (*line) {
			case 'D': setup_dir(room_realnum, atoi(line + 1));
				break;

			case 'E': {
				const ExtraDescription::shared_ptr new_descr(new ExtraDescription);
				new_descr->set_keyword(fread_string());
				new_descr->set_description(fread_string());
				if (new_descr->keyword && new_descr->description) {
					new_descr->next = world[room_realnum]->ex_description;
					world[room_realnum]->ex_description = new_descr;
				} else {
					sprintf(buf, "SYSERR: Format error in room #%d (Corrupt extradesc)", virtual_nr);
					log("%s", buf);
				}
			}
				break;

			case 'S':    // end of room
				// DG triggers -- script is defined after the end of the room 'T'
				do {
					letter = fread_letter(file());
					ungetc(letter, file());
					switch (letter) {
						case 'I':
							get_line(file(),
									 line); //оставлено для совместимости, удалить после пересохранения комнат
							break;

						case 'T': dg_read_trigger(world[room_realnum], WLD_TRIGGER, virtual_nr);
							break;
						default: letter = 0;
							break;
					}
				} while (letter != 0);
				return;

			default: log("%s", buf);
				exit(1);
		}
	}
}

void WorldFile::setup_dir(int room, unsigned dir) {
	if (dir >= world[room]->dir_option_proto.size()) {
		log("SYSERROR : dir=%d (%s:%d)", dir, __FILE__, __LINE__);
		return;
	}

	int t[5];
	char line[256];

	sprintf(buf2, "room #%d, direction D%u", GET_ROOM_VNUM(room), dir);

	world[room]->dir_option_proto[dir] = std::make_shared<ExitData>();
	world[room]->dir_option_proto[dir]->general_description = fread_string();

	// парс строки алиаса двери на имя; вининельный падеж, если он есть
	world[room]->dir_option_proto[dir]->set_keywords(fread_string());

	if (!get_line(file(), line)) {
		log("SYSERR: Format error, %s", line);
		exit(1);
	}
	int result = sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3);
	if (result == 3) {
		if (t[0] & 1)
			world[room]->dir_option_proto[dir]->exit_info = EExitFlag::kHasDoor;
		else if (t[0] & 2)
			world[room]->dir_option_proto[dir]->exit_info = EExitFlag::kHasDoor | EExitFlag::kPickroof;
		else
			world[room]->dir_option_proto[dir]->exit_info = 0;
		if (t[0] & 4)
			world[room]->dir_option_proto[dir]->exit_info |= EExitFlag::kHidden;

		world[room]->dir_option_proto[dir]->lock_complexity = 0;
	} else if (result == 4) {
		world[room]->dir_option_proto[dir]->exit_info = t[0];
		world[room]->dir_option_proto[dir]->lock_complexity = t[3];
	} else {
		log("SYSERR: Format error, %s", buf2);
		exit(1);
	}

	world[room]->dir_option_proto[dir]->key = t[1];
	world[room]->dir_option_proto[dir]->to_room(t[2]);
}

bool WorldFile::load() {
	const auto result = DiscreteFile::load();
	return result;
}

class ObjectFile : public DiscreteFile {
 public:
	ObjectFile(const std::string &file_name) : DiscreteFile(file_name) {}

	virtual EBootType mode() const override { return DB_BOOT_OBJ; }

	static shared_ptr create(const std::string &file_name) { return shared_ptr(new ObjectFile(file_name)); }

 private:
	virtual void read_next_line(const int nr) override;
	virtual void read_entry(const int nr) override;

	void parse_object(const int nr);

	/**
	* Extend later to include more checks.
	*
	* TODO: Add checks for unknown bitvectors.
	*/
	bool check_object(ObjData *obj);

	bool check_object_level(ObjData *obj, int val);
	bool check_object_spell_number(ObjData *obj, unsigned val);

	char m_buffer[kMaxStringLength];
};

void ObjectFile::read_next_line(const int nr) {
	if (nr < 0) {
		DiscreteFile::read_next_line(nr);
	}
}

void ObjectFile::read_entry(const int nr) {
	parse_object(nr);
}

void ObjectFile::parse_object(const int nr) {
	int t[10], j = 0;
	char f0[256], f1[256], f2[256];

	ObjData *tobj = new ObjData(nr);

	// *** Add some initialization fields
	tobj->set_maximum_durability(ObjData::DEFAULT_MAXIMUM_DURABILITY);
	tobj->set_current_durability(ObjData::DEFAULT_CURRENT_DURABILITY);
	tobj->set_sex(EGender::kMale);
	tobj->set_timer(ObjData::DEFAULT_TIMER);
	tobj->set_level(1);
	tobj->set_destroyer(ObjData::DEFAULT_DESTROYER);

	sprintf(m_buffer, "object #%d", nr);

	// *** string data ***
	std::string aliases(fread_string());
	if (aliases.empty()) {
		log("SYSERR: Empty obj name or format error at or near %s", m_buffer);
		exit(1);
	}
	tobj->set_aliases(aliases);
	tobj->set_short_description(utils::colorLOW(fread_string()));

	strcpy(buf, tobj->get_short_description().c_str());
	tobj->set_PName(ECase::kNom, utils::colorLOW(buf)); //именительный падеж равен короткому описанию

	for (j = ECase::kGen; j <= ECase::kLastCase; j++) {
		tobj->set_PName(static_cast<ECase>(j), utils::colorLOW(fread_string()));
	}

	tobj->set_description(utils::colorCAP(fread_string()));
	tobj->set_action_description(fread_string());

	if (!get_line(file(), m_line)) {
		log("SYSERR: Expecting *1th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	int parsed_entries = sscanf(m_line, " %s %d %d %d", f0, t + 1, t + 2, t + 3);
	if (parsed_entries != 4) {
		log("SYSERR: Format error in *1th* numeric line (expecting 4 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}

	int sparam = 0;
	asciiflag_conv(f0, &sparam);
	tobj->set_spec_param(sparam);

	tobj->set_maximum_durability(t[1]);
	tobj->set_current_durability(MIN(t[1], t[2]));
	tobj->set_material(static_cast<EObjMaterial>(t[3]));

	if (tobj->get_current_durability() > tobj->get_maximum_durability()) {
		log("SYSERR: Obj_cur > Obj_Max, vnum: %d", nr);
	}

	if (!get_line(file(), m_line)) {
		log("SYSERR: Expecting *2th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, " %d %d %d %d", t, t + 1, t + 2, t + 3);
	if (parsed_entries != 4) {
		log("SYSERR: Format error in *2th* numeric line (expecting 4 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}
	tobj->set_sex(static_cast<EGender>(t[0]));
	int timer = t[1] > 0 ? t[1] : ObjData::SEVEN_DAYS;
	if (timer > 99999) {
		timer = 99999;
	}
	tobj->set_timer(timer);
	tobj->set_spell(t[2]);
	tobj->set_level(t[3]);

	if (!get_line(file(), m_line)) {
		log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, " %s %s %s", f0, f1, f2);
	if (parsed_entries != 3) {
		log("SYSERR: Format error in *3th* numeric line (expecting 3 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}

	tobj->load_affect_flags(f0);
	// ** Affects
	tobj->load_anti_flags(f1);
	// ** Miss for ...
	tobj->load_no_flags(f2);
	// ** Deny for ...

	if (!get_line(file(), m_line)) {
		log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, " %d %s %s", t, f1, f2);
	if (parsed_entries != 3) {
		log("SYSERR: Format error in *3th* misc line (expecting 3 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}

	tobj->set_type(static_cast<EObjType>(t[0]));        // ** What's a object
	tobj->load_extra_flags(f1);
	// ** Its effects
	int wear_flags = 0;
	asciiflag_conv(f2, &wear_flags);
	tobj->set_wear_flags(wear_flags);
	// ** Wear on ...

	if (!get_line(file(), m_line)) {
		log("SYSERR: Expecting *5th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, "%s %d %d %d", f0, t + 1, t + 2, t + 3);
	if (parsed_entries != 4) {
		log("SYSERR: Format error in *5th* numeric line (expecting 4 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}

	int first_value = 0;
	asciiflag_conv(f0, &first_value);
	tobj->set_val(0, first_value);
	tobj->set_val(1, t[1]);
	tobj->set_val(2, t[2]);
	tobj->set_val(3, t[3]);

	if (!get_line(file(), m_line)) {
		log("SYSERR: Expecting *6th* numeric line of %s, but file ended!", m_buffer);
		exit(1);
	}

	parsed_entries = sscanf(m_line, "%d %d %d %d", t, t + 1, t + 2, t + 3);
	if (parsed_entries != 4) {
		log("SYSERR: Format error in *6th* numeric line (expecting 4 args, got %d), %s", parsed_entries, m_buffer);
		exit(1);
	}
	tobj->set_weight(t[0]);
	tobj->set_cost(t[1]);
	tobj->set_rent_off(t[2]);
	tobj->set_rent_on(t[3]);

	// check to make sure that weight of containers exceeds curr. quantity
	if (tobj->get_type() == EObjType::kLiquidContainer
		|| tobj->get_type() == EObjType::kFountain) {
		if (tobj->get_weight() < tobj->get_val(1)) {
			tobj->set_weight(tobj->get_val(1) + 5);
		}
	}
	tobj->unset_extraflag(EObjFlag::kTransformed); //от шаловливых ручек
	tobj->unset_extraflag(EObjFlag::kTicktimer);

	// *** extra descriptions and affect fields ***
	strcat(m_buffer, ", after numeric constants\n" "...expecting 'E', 'A', '$', or next object number");
	j = 0;

	for (;;) {
		if (!get_line(file(), m_line)) {
			log("SYSERR: Format error in %s", m_buffer);
			exit(1);
		}
		switch (*m_line) {
			case 'E': {
				const ExtraDescription::shared_ptr new_descr(new ExtraDescription());
				new_descr->set_keyword(fread_string());
				new_descr->set_description(fread_string());
				if (new_descr->keyword && new_descr->description) {
					new_descr->next = tobj->get_ex_description();
					tobj->set_ex_description(new_descr);
				} else {
					const auto written =
						snprintf(buf, sizeof(buf), "SYSERR: Format error in %s (Corrupt extradesc)", m_buffer);
					buf[written] = '\0';

					log("%s", buf);
				}
			}
				break;

			case 'A':
				if (j >= kMaxObjAffect) {
					log("SYSERR: Too many A fields (%d max), %s", kMaxObjAffect, m_buffer);
					exit(1);
				}

				if (!get_line(file(), m_line)) {
					log("SYSERR: Format error in 'A' field, %s\n"
						"...expecting 2 numeric constants but file ended!", m_buffer);
					exit(1);
				}

				parsed_entries = sscanf(m_line, " %d %d ", t, t + 1);
				if (parsed_entries != 2) {
					log("SYSERR: Format error in 'A' field, %s\n"
						"...expecting 2 numeric arguments, got %d\n"
						"...offending line: '%s'", m_buffer, parsed_entries, m_line);
					exit(1);
				}

				tobj->set_affected(j, static_cast<EApply>(t[0]), t[1]);
				++j;
				break;

			case 'T':    // DG triggers
				dg_obj_trigger(m_line, tobj);
				break;

			case 'M': tobj->set_max_in_world(atoi(m_line + 1));
				break;

			case 'R': tobj->set_minimum_remorts(atoi(m_line + 1));
				break;

			case 'S':
				if (!get_line(file(), m_line)) {
					log("SYSERR: Format error in 'S' field, %s\n"
						"...expecting 2 numeric constants but file ended!", m_buffer);
					exit(1);
				}

				parsed_entries = sscanf(m_line, " %d %d ", t, t + 1);
				if (parsed_entries != 2) {
					log("SYSERR: Format error in 'S' field, %s\n"
						"...expecting 2 numeric arguments, got %d\n"
						"...offending line: '%s'", m_buffer, parsed_entries, m_line);
					exit(1);
				}
				tobj->set_skill(static_cast<ESkill>(t[0]), t[1]);
				break;

			case 'V': tobj->init_values_from_zone(m_line + 1);
				break;

			case '$':
			case '#': check_object(tobj);        // Anton Gorev (2015/12/29): do we need the result of this check?
				if (tobj->has_flag(EObjFlag::kZonedecay)
					|| tobj->has_flag(EObjFlag::kRepopDecay))
					tobj->set_max_in_world(-1);
				obj_proto.add(tobj, nr);
				return;

			default: log("SYSERR: Format error in %s", m_buffer);
				exit(1);
		}
	}
}

bool ObjectFile::check_object(ObjData *obj) {
	bool error = false;

	if (obj->get_weight() < 0) {
		error = true;
		log("SYSERR: Object #%d (%s) has negative weight (%d).",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), obj->get_weight());
	}
/* спам от антуражных шмоток
	if (obj->get_rent_off() <= 0) {
		error = true;
		log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), obj->get_rent_off());
	}
*/
	sprintbit(obj->get_wear_flags(), wear_bits, buf);
	if (strstr(buf, "UNDEFINED")) {
		error = true;
		log("SYSERR: Object #%d (%s) has unknown wear flags.", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
	}

	obj->get_extra_flags().sprintbits(extra_bits, buf, ",", 4);
	if (strstr(buf, "UNDEFINED")) {
		error = true;
		log("SYSERR: Object #%d (%s) has unknown extra flags.",
			GET_OBJ_VNUM(obj),
			obj->get_short_description().c_str());
	}

	obj->get_affect_flags().sprintbits(affected_bits, buf, ",", 4);

	if (strstr(buf, "UNDEFINED")) {
		error = true;
		log("SYSERR: Object #%d (%s) has unknown affection flags.",
			GET_OBJ_VNUM(obj),
			obj->get_short_description().c_str());
	}

	switch (obj->get_type()) {
		case EObjType::kLiquidContainer:
		case EObjType::kFountain:
			if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0)) {
				error = true;
				log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
					GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
			}
			break;

		case EObjType::kScroll:
		case EObjType::kPotion: error = error || check_object_level(obj, 0);
			error = error || check_object_spell_number(obj, 1);
			error = error || check_object_spell_number(obj, 2);
			error = error || check_object_spell_number(obj, 3);
			break;

		case EObjType::kBook: error = error || check_object_spell_number(obj, 1);
			break;

		case EObjType::kWand:
		case EObjType::kStaff: error = error || check_object_level(obj, 0);
			error = error || check_object_spell_number(obj, 3);
			if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1)) {
				error = true;
				log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
					GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
			}
			break;

		default: break;
	}
	obj->remove_incorrect_values_keys(obj->get_type());
	return error;
}

bool ObjectFile::check_object_level(ObjData *obj, int val) {
	bool error = false;

	if (GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > kLvlImplementator) {
		error = true;
		log("SYSERR: Object #%d (%s) has out of range level #%d.",
			GET_OBJ_VNUM(obj),
			obj->get_short_description().c_str(),
			GET_OBJ_VAL(obj, val));
	}
	return error;
}

bool ObjectFile::check_object_spell_number(ObjData *obj, unsigned val) {
	if (val >= ObjData::VALS_COUNT) {
		log("SYSERROR : val=%d (%s:%d)", val, __FILE__, __LINE__);
		return true;
	}

	auto error{false};
	if (GET_OBJ_VAL(obj, val) == -1)    // i.e.: no spell
		return error;

	/*
	* Check for negative spells, spells beyond the top define, and any
	* spell which is actually a skill.
	*/
	auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, val));
	if (MUD::Spell(spell_id).IsInvalid()) {
		error = true;
	}

	if (error) {
		log("SYSERR: Object #%d (%s) has out of range spell #%d.",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, val));
	}

	// Spell names don't exist in syntax check mode.
	if (scheck) {
		return error;
	}

	// Now check for unnamed spells.
	if (error) {
		log("SYSERR: Object #%d (%s) uses '%s' spell #%d.", GET_OBJ_VNUM(obj),
			obj->get_short_description().c_str(), MUD::Spell(spell_id).GetCName(), GET_OBJ_VAL(obj, val));
	}

	return error;
}

class MobileFile : public DiscreteFile {
 public:
	MobileFile(const std::string &file_name) : DiscreteFile(file_name) {}

	virtual EBootType mode() const override { return DB_BOOT_MOB; }

	static shared_ptr create(const std::string &file_name) { return shared_ptr(new MobileFile(file_name)); }

 private:
	virtual void read_entry(const int nr) override;

	void parse_mobile(const int nr);
	void parse_simple_mob(int i, int nr);
	void parse_enhanced_mob(int i, int nr);
	void parse_espec(char *buf, int i, int nr);
	void interpret_espec(const char *keyword, const char *value, int i, int nr);
};

void MobileFile::read_entry(const int nr) {
	parse_mobile(nr);
}

void MobileFile::parse_mobile(const int nr) {
	static int i = 0;
	int j, t[10];
	char line[256], letter;
	char f1[128], f2[128];
	mob_index[i].vnum = nr;
	mob_index[i].func = nullptr;
	mob_index[i].set_idx = -1;
	if (zone_table[GetZoneRnum(nr / 100)].RnumMobsLocation.first == -1) {
		zone_table[GetZoneRnum(nr / 100)].RnumMobsLocation.first = i;
	}
	zone_table[GetZoneRnum(nr / 100)].RnumMobsLocation.second = i;
//snprintf(tmpstr, BUFFER_SIZE, "ZONE_MOB zone %d first %d last %d", zone_table[zone].vnum, zone_table[zone].RnumMobsLocation.first, zone_table[zone].RnumMobsLocation.second);
	mob_proto[i].player_specials = player_special_data::s_for_mobiles;
	// **** String data
	mob_proto[i].SetCharAliases(fread_string());
	mob_proto[i].set_npc_name(fread_string());

	// real name
	mob_proto[i].player_data.PNames[ECase::kNom] = mob_proto[i].get_npc_name();
	for (j = ECase::kGen; j <= ECase::kLastCase; j++) {
		mob_proto[i].player_data.PNames[j] = fread_string();
	}
	mob_proto[i].player_data.long_descr = utils::colorCAP(fread_string());
	mob_proto[i].player_data.description = fread_string();
	mob_proto[i].mob_specials.Questor = nullptr;
	mob_proto[i].SetTitleStr("");
	mob_proto[i].set_level(1);

	// *** Numeric data ***
	if (!get_line(file(), line)) {
		log("SYSERR: Format error after string section of mob #%d\n"
			"...expecting line of form '# # # {S | E}', but file ended!\n%s", nr, line);
		exit(1);
	}

	if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4) {
		log("SYSERR: Format error after string section of mob #%d\n"
			"...expecting line of form '# # # {S | E}'\n%s", nr, line);
		exit(1);
	}
	mob_proto[i].SetFlagsFromString(f1);
	mob_proto[i].SetNpcAttribute(true); 
	AFF_FLAGS(&mob_proto[i]).from_string(f2);
	GET_ALIGNMENT(mob_proto + i) = t[2];
	switch (UPPER(letter)) {
		case 'S':        // Simple monsters
			parse_simple_mob(i, nr);
			break;

		case 'E':        // Circle3 Enhanced monsters
			parse_enhanced_mob(i, nr);
			break;

			// add new mob types here..
		default: log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
			exit(1);
	}

	// DG triggers -- script info follows mob S/E section
	// DG triggers -- script is defined after the end of the room 'T'
	// Ингредиентная магия -- 'I'
	// Объекты загружаемые по-смертно -- 'L'

	do {
		letter = fread_letter(file());
		ungetc(letter, file());
		switch (letter) {
			case 'I': // Оставлено для совместимости со старым форматиом (с ингредиентами в файлах зон).
				get_line(file(), line);
				break;

			case 'L': get_line(file(), line);
				dead_load::ParseDeadLoadLine(mob_proto[i].dl_list, line + 1);
				break;

			case 'T': dg_read_trigger(&mob_proto[i], MOB_TRIGGER, nr);
				break;
/*		case 'R':
			get_line(file(), line);
			sscanf(line, "%s", f1);
			mob_proto[i].mob_specials.mob_remort = atoi(f1);
			break;
*/
			default: letter = 0;
				break;
		}
	} while (letter != 0);

	for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
		mob_proto[i].equipment[j] = nullptr;
	}

	mob_proto[i].set_rnum(i);
	mob_proto[i].desc = nullptr;
	if ((mob_proto + 1)->GetLevel() == 0)
		SetTestData(mob_proto + i);

	top_of_mobt = i++;
}

void MobileFile::parse_simple_mob(int i, int nr) {
	int t[10];
	char line[256];

	mob_proto[i].set_str(11);
	mob_proto[i].set_int(11);
	mob_proto[i].set_wis(11);
	mob_proto[i].set_dex(11);
	mob_proto[i].set_con(11);
	mob_proto[i].set_cha(11);

	mob_proto[i].player_specials->saved.NameGod = 1001; // у мобов имя всегда одобрено
	if (!get_line(file(), line)) {
		log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
		exit(1);
	}
	if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
			   t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
		log("SYSERR: Format error in mob #%d, 1th line\n" "...expecting line of form '# # # #d#+# #d#+#'", nr);
		exit(1);
	}

	mob_proto[i].set_level(t[0]);
	mob_proto[i].real_abils.hitroll = 20 - t[1];
	mob_proto[i].real_abils.armor = 10 * t[2];

	// max hit = 0 is a flag that H, M, V is xdy+z
	(mob_proto + i)->set_max_hit(0);
	(mob_proto + i)->mem_queue.total = t[3];
	(mob_proto + i)->mem_queue.stored = t[4];
	(mob_proto + i)->set_hit(t[5]);

	(mob_proto + i)->set_move(100);
	(mob_proto + i)->set_max_move(100);

	mob_proto[i].mob_specials.damnodice = t[6];
	mob_proto[i].mob_specials.damsizedice = t[7];
	mob_proto[i].real_abils.damroll = t[8];

	if (!get_line(file(), line)) {
		log("SYSERR: Format error in mob #%d, 2th line\n"
			"...expecting line of form '#d#+# #', but file ended!", nr);
		exit(1);
	}
	if (sscanf(line, " %dd%d+%d %d", t, t + 1, t + 2, t + 3) != 4) {
		log("SYSERR: Format error in mob #%d, 2th line\n" "...expecting line of form '#d#+# #'", nr);
		exit(1);
	}

	mob_proto[i].set_gold(t[2], false);
	GET_GOLD_NoDs(mob_proto + i) = t[0];
	GET_GOLD_SiDs(mob_proto + i) = t[1];
	mob_proto[i].set_exp(t[3]);

	if (!get_line(file(), line)) {
		log("SYSERR: Format error in 3th line of mob #%d\n"
			"...expecting line of form '# # #', but file ended!", nr);
		exit(1);
	}

	switch (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3)) {
		case 3: mob_proto[i].mob_specials.speed = -1;
			break;
		case 4: mob_proto[i].mob_specials.speed = t[3];
			break;
		default: log("SYSERR: Format error in 3th line of mob #%d\n" "...expecting line of form '# # # #'", nr);
			exit(1);
	}

	mob_proto[i].SetPosition(static_cast<EPosition>(t[0]));
	mob_proto[i].mob_specials.default_pos = static_cast<EPosition>(t[1]);
	mob_proto[i].set_sex(static_cast<EGender>(t[2]));

	mob_proto[i].player_data.Race = ENpcRace::kBasic;
	mob_proto[i].set_class(ECharClass::kNpcBase);
	mob_proto[i].player_data.weight = 200;
	mob_proto[i].player_data.height = 198;

	/*
	* these are now save applies; base save numbers for MOBs are now from
	* the warrior save table.
	*/
	for (auto save = ESaving::kFirst; save <= ESaving::kLast; ++save) {
		SetSave(mob_proto + i, save, 0);
	}
}

void MobileFile::parse_enhanced_mob(int i, int nr) {
	char line[256];
	parse_simple_mob(i, nr);

	while (get_line(file(), line)) {
		if (!strcmp(line, "E"))    // end of the enhanced section
		{
/*			if (!mob_proto[i].get_role_bits().any())
			{
				mob_proto[i].mob_specials.MaxFactor = mob_proto[i].get_level() / 9;
//				log("SET maxfactor %d level mobs %d vnum %d  name %s", mob_proto[i].mob_specials.MaxFactor, mob_proto[i].get_level(), nr, mob_proto[i].get_npc_name().c_str());
			}
*/
//			if (mob_proto[i].mob_specials.MaxFactor > 0  && mob_proto[i].get_role_bits().any())
//				log("BOSS maxfactor %d level mobs %d vnum %d  name %s", mob_proto[i].mob_specials.MaxFactor, mob_proto[i].get_level(), nr, mob_proto[i].get_npc_name().c_str());
			return;
		} else if (*line == '#')    // we've hit the next mob, maybe?
		{
			log("SYSERR: Unterminated E section in mob #%d", nr);
			exit(1);
		} else {
			parse_espec(line, i, nr);

		}
	}

	log("SYSERR: Unexpected end of file reached after mob #%d", nr);
	exit(1);
}

void MobileFile::parse_espec(char *buf, int i, int nr) {
	char *ptr;

	if ((ptr = strchr(buf, ':')) != nullptr) {
		*(ptr++) = '\0';
		while (a_isspace(*ptr)) {
			ptr++;
		}
	} else {
		ptr = nullptr;
	}
	interpret_espec(buf, ptr, i, nr);
}

/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */
void MobileFile::interpret_espec(const char *keyword, const char *value, int i, int nr) {
	int num_arg, matched = 0, t[4];

	num_arg = atoi(value);

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))

	CASE("Resistances") {
		std::vector<std::string> array_string;
		array_string = utils::Split(value);
		if (array_string.size() < 7 || array_string.size() > EResist::kLastResist + 1) {
			log("SYSERROR : Excepted format <# # # # # # # #> for RESISTANCES in MOB #%d", i);
			return;
		}
		for (unsigned kk = 0; kk < array_string.size(); ++kk) {
			GET_RESIST(mob_proto + i, kk) = std::clamp(atoi(array_string[kk].c_str()), kMinResistance, kMaxNpcResist);
		}


/*		заготовка парса резистов у моба при загрузке мада, чтоб в след раз не придумывать
		if (GET_RESIST(mob_proto + i, 4) > 49 && !mob_proto[i].get_role(kBoss)) // жизнь и не боссы
		{
			if (zone_table[world[&mob_proto[i]->in_room]->zone].group < 3) // в зонах 0-2 группы
				log("RESIST LIVE num: %d Vnum: %d Level: %d Name: %s", GET_RESIST(mob_proto + i, 4), mob_index[i].vnum, GetRealLevel(&mob_proto[i]), GET_PAD(&mob_proto[i], 0));
		}
*/
	}

	CASE("Saves") {
		if (sscanf(value, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
			log("SYSERROR : Excepted format <# # # #> for SAVES in MOB #%d", i);
			return;
		}
		for (auto save = ESaving::kFirst; save <= ESaving::kLast; ++save) {
			SetSave(mob_proto + i, save, std::clamp(t[to_underlying(save)], kMinSaving, kMaxSaving));
		}
	}
// Svent: и что тут за коллекция магик намберов бесконечная? Вынести в настройки.
	CASE("HPReg") {
		mob_proto[i].add_abils.hitreg = std::clamp(num_arg, -200, 200);
	}

	CASE("Armour") {
		mob_proto[i].add_abils.armour = std::clamp(num_arg, 0, 100);
	}

	CASE("PlusMem") {
		mob_proto[i].add_abils.manareg = std::clamp(num_arg, -200, 200);
	}

	CASE("CastSuccess") {
		mob_proto[i].add_abils.cast_success = std::clamp(num_arg, -200, 300);
	}

	CASE("Success") {
		mob_proto[i].add_abils.morale = std::clamp(num_arg, 0, 100);
	}

	CASE("Initiative") {
		mob_proto[i].add_abils.initiative_add = std::clamp(num_arg, -200, 200);
	}

	CASE("Absorbe") {
		mob_proto[i].add_abils.absorb = std::clamp(num_arg, -200, 200);
	}
	CASE("AResist") {
		mob_proto[i].add_abils.aresist = std::clamp(num_arg, 0, 100);
	}
	CASE("MResist") {
		mob_proto[i].add_abils.mresist = std::clamp(num_arg, 0, 100);
	}

	CASE("PResist") {
		mob_proto[i].add_abils.presist = std::clamp(num_arg, 0, 100);
	}

	CASE("BareHandAttack") {
		mob_proto[i].mob_specials.attack_type = std::clamp(num_arg, 0, 99);
	}

	CASE("Destination") {
		if (mob_proto[i].mob_specials.dest_count < kMaxDest) {
			mob_proto[i].mob_specials.dest[mob_proto[i].mob_specials.dest_count] = num_arg;
			mob_proto[i].mob_specials.dest_count++;
		}
	}

	CASE("Str") {
		mob_proto[i].set_str(num_arg);
	}

	CASE("StrAdd") {
		mob_proto[i].set_str_add(num_arg);
	}

	CASE("Int") {
		mob_proto[i].set_int(num_arg);
	}

	CASE("Wis") {
		mob_proto[i].set_wis(num_arg);
	}

	CASE("Dex") {
		mob_proto[i].set_dex(num_arg);
	}

	CASE("Con") {
		mob_proto[i].set_con(num_arg);
	}

	CASE("Cha") {
		mob_proto[i].set_cha(num_arg);
	}

	CASE("Size") {
		mob_proto[i].real_abils.size = std::clamp<byte>(num_arg, 0, 100);
	}

	CASE("LikeWork") {
		mob_proto[i].mob_specials.like_work = std::clamp<byte>(num_arg, 0, 100);
	}

	CASE("MaxFactor") {
		mob_proto[i].mob_specials.MaxFactor = std::clamp<byte>(num_arg, 0, 127);
	}

	CASE("ExtraAttack") {
		mob_proto[i].mob_specials.extra_attack = std::clamp<byte>(num_arg, 0, 127);
	}

	CASE("MobRemort") {
		mob_proto[i].set_remort(std::clamp<byte>(num_arg, 0, 100));
	}

	CASE("Height") {
		mob_proto[i].player_data.height = std::clamp(num_arg, 0, 200);
	}

	CASE("Weight") {
		mob_proto[i].player_data.weight = std::clamp(num_arg, 0, 200);
	}

	CASE("Race") {
		mob_proto[i].player_data.Race = std::clamp(static_cast<ENpcRace>(num_arg), ENpcRace::kBasic, ENpcRace::kLastNpcRace);
	}

	CASE("Special_Bitvector") {
		mob_proto[i].mob_specials.npc_flags.from_string((char *) value);
	}

	CASE("Feat") {
		if (sscanf(value, "%d", t) != 1) {
			log("SYSERROR : Excepted format <#> for FEAT in MOB #%d", nr);
			return;
		}
		auto feat_id = static_cast<EFeat>(t[0]);
		if (MUD::Feats().IsUnknown(feat_id)) {
			log("SYSERROR : Unknown feat No %d for MOB #%d", t[0], nr);
			return;
		}
		(mob_proto + i)->SetFeat(feat_id);
	}

	CASE("Skill") {
		if (sscanf(value, "%d %d", t, t + 1) != 2) {
			log("SYSERROR : Excepted format <# #> for SKILL in MOB #%d", nr);
			return;
		}
		auto skill_id = static_cast<ESkill>(t[0]);
		if (MUD::Skills().IsInvalid(skill_id)) {
			log("SYSERROR : Unknown skill No %d for MOB #%d", t[0], nr);
			return;
		}
		t[1] = std::clamp(t[1], 0, MUD::Skill(skill_id).cap);
		(mob_proto + i)->set_skill(skill_id, t[1]);
	}

	CASE("Spell") {
		if (sscanf(value, "%d", t + 0) != 1) {
			log("SYSERROR : Excepted format <#> for SPELL in MOB #%d", nr);
			return;
		}
		auto spell_id = static_cast<ESpell>(t[0]);
		if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
			log("SYSERROR : Unknown spell No %d for MOB #%d", t[0], nr);
			return;
		}
		GET_SPELL_MEM(mob_proto + i, spell_id) += 1;
		(mob_proto + i)->caster_level += (MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE) ? 1 : 0);
		mob_proto[i].mob_specials.have_spell = true;
	}

	CASE("Helper") {
		auto it = find(mob_proto[i].summon_helpers.begin(), mob_proto[i].summon_helpers.end(), num_arg);
		if (it != mob_proto[i].summon_helpers.end()) {
			log("SYSERROR : Dublicate helpers #%d for MOB #%d", num_arg, nr);
			return;
		}
		mob_proto[i].summon_helpers.push_back(num_arg);
	}

	CASE("Role") {
		if (value && *value) {
			std::string str(value);
			CharData::role_t tmp(str);
//			tmp.resize(mob_proto[i].get_role().size());
			mob_proto[i].set_role(tmp);
		}
	}

	if (!matched) {
		log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d", keyword, nr);
	}
#undef CASE
}

class ZoneFile : public DataFile {
 public:
	ZoneFile(const std::string &file_name) : DataFile(file_name) {}

	virtual bool load() override { return load_zone(); }
	virtual std::string full_file_name() const override { return prefixes(DB_BOOT_ZON) + file_name(); }

	static shared_ptr create(const std::string &file_name) { return shared_ptr(new ZoneFile(file_name)); }

 private:
	bool load_zone();
	bool load_regular_zone();
	bool load_generated_zone() const;

	static int s_zone_number;
};

int ZoneFile::s_zone_number = 0;

bool ZoneFile::load_zone() {
	auto &zone = zone_table[s_zone_number];

	constexpr std::size_t BUFFER_SIZE = 256;
	char buf[BUFFER_SIZE];

	zone.level = 1;
	zone.type = 0;

	zone.typeA_count = 0;
	zone.typeB_count = 0;
	zone.locked = false;
	zone.reset_idle = false;
	zone.used = false;
	zone.activity = 0;
	zone.group = false;
	zone.count_reset = 0;
	zone.traffic = 0;
	zone.RnumTrigsLocation.first = -1;
	zone.RnumTrigsLocation.second = -1;
	zone.RnumMobsLocation.first = -1;
	zone.RnumMobsLocation.second = -1;
	zone.RnumRoomsLocation.first = -1;
	zone.RnumRoomsLocation.second = -1;
	get_line(file(), buf);

	auto result = false;
	{
		char type[BUFFER_SIZE];
		const auto count = sscanf(buf, "#%d %s", &zone.vnum, type);
		if (count < 1) {
			log("SYSERR: Format error in %s, line 1", full_file_name().c_str());
			exit(1);
		}
		size_t digits = full_file_name().find_first_of("1234567890");
		if (digits <= full_file_name().size()) {
			if (zone.vnum != atoi(full_file_name().c_str() + digits)) {
				log("SYSERR: файл %s содержит неверный номер зоны %d", full_file_name().c_str(), zone.vnum);
				exit(1);
			}
		}
		if (2 == count) {
			if (0 == strcmp(type, "static")) {
				result = load_regular_zone();
			} else if (0 == strcmp(type, "generated")) {
				result = load_generated_zone();
			}
		} else {
			result = load_regular_zone();
		}
	}
	s_zone_number++;
	return result;
}

bool ZoneFile::load_regular_zone() {
	auto &zone = zone_table[s_zone_number];

	sprintf(buf2, "beginning of zone #%d", zone.vnum);

	rewind(file());
	char *ptr;
	auto num_of_cmds = 0;
	while (get_line(file(), buf)) {
		ptr = buf;
		skip_spaces(&ptr);

		if (*ptr == 'A') {
			zone.typeA_count++;
		}

		if (*ptr == 'B') {
			zone.typeB_count++;
		}

		num_of_cmds++;    // this should be correct within 3 or so
	}

	rewind(file());
	if (zone.typeA_count) {
		CREATE(zone.typeA_list, zone.typeA_count);
	}

	auto b_number = 0;
	if (zone.typeB_count) {
		CREATE(zone.typeB_list, zone.typeB_count);
		CREATE(zone.typeB_flag, zone.typeB_count);
		// сбрасываем все флаги
		for (b_number = zone.typeB_count; b_number > 0; b_number--) {
			zone.typeB_flag[b_number - 1] = false;
		}
	}

	if (num_of_cmds == 0) {
		log("SYSERR: %s is empty!", full_file_name().c_str());
		exit(1);
	} else {
		CREATE(zone.cmd, num_of_cmds);
	}

	auto line_num = get_line(file(), buf);    // skip already processed "#<zone number> [<zone type>]" line

	line_num += get_line(file(), buf);
	if ((ptr = strchr(buf, '~')) != nullptr)    // take off the '~' if it's there
	{
		*ptr = '\0';
	}
	zone.name = buf;

	log("Читаем zon файл: %s", full_file_name().c_str());
	while (*buf != 'S' && !feof(file())) {
		line_num += get_line(file(), buf);

		if (*buf == '#') {
			break;
		}

		if (*buf == '^') {
			std::string comment = buf;
			utils::TrimIf(comment, "^~");
			zone.comment = comment;
		}

		if (*buf == '&') {
			std::string location = buf;
			utils::TrimIf(location, "&~");
			zone.location = location;
		}

		if (*buf == '!') {
			std::string autor = buf;
			utils::TrimIf(autor, "!~");
			zone.author = autor;
		}

		if (*buf == '$') {
			std::string description = buf ;
			utils::TrimIf(description, "$~");
			zone.description = description;
		}
	}

	if (*buf != '#') {
		log("SYSERR: ERROR!!! not # in file %s", full_file_name().c_str());
		exit(1);
	}
	auto group = 0;
	const auto count = sscanf(buf, "#%d %d %d %d", &zone.level, &zone.type, &group, &zone.entrance);
	if (count < 2) {
		log("SYSERR: ошибка чтения z.level, z.type, z.group, z.entrance: %s", buf);
		exit(1);
	}
	zone.group = (group == 0) ? 1 : group; //группы в 0 рыл не бывает
	line_num += get_line(file(), buf);

	char t1[80];
	char t2[80];
	*t1 = 0;
	*t2 = 0;
	auto tmp_reset_idle = 0;
	if (sscanf(buf, " %d %d %d %d %s %s", &zone.top, &zone.lifespan, &zone.reset_mode, &tmp_reset_idle, t1, t2) < 4) {
		// если нет четырех констант, то, возможно, это старый формат -- попробуем прочитать три
		const auto count = sscanf(buf, " %d %d %d %s %s",
								  &zone.top,
								  &zone.lifespan,
								  &zone.reset_mode,
								  t1,
								  t2);
		if (count < 3) {
			log("SYSERR: Format error in 3-constant line of %s", full_file_name().c_str());
			exit(1);
		}
	}
	zone.reset_idle = 0 != tmp_reset_idle;
	zone.under_construction = !str_cmp(t1, "test");
	zone.locked = !str_cmp(t2, "locked");
	zone.top = zone.vnum * 100 + 99;
	auto a_number = 0;
	auto cmd_no = 0;

	for (;;) {
		const auto lines_read = get_line(file(), buf);

		if (lines_read == 0) {
			log("SYSERR: Format error in %s - premature end of file", full_file_name().c_str());
			exit(1);
		}

		line_num += lines_read;
		ptr = buf;
		skip_spaces(&ptr);

		if ((zone.cmd[cmd_no].command = *ptr) == '*') {
			continue;
		}
		ptr++;

		// Новые параметры формата файла:
		// A номер_зоны -- зона типа A из списка
		// B номер_зоны -- зона типа B из списка
		if (zone.cmd[cmd_no].command == 'A') {
			sscanf(ptr, " %d", &zone.typeA_list[a_number]);
			a_number++;
			continue;
		}

		if (zone.cmd[cmd_no].command == 'B') {
			sscanf(ptr, " %d", &zone.typeB_list[b_number]);
			b_number++;
			continue;
		}

		if (zone.cmd[cmd_no].command == 'S' || zone.cmd[cmd_no].command == '$') {
			zone.cmd[cmd_no].command = 'S';
			break;
		}

		auto if_flag = 0;
		auto error = 0;
		zone.cmd[cmd_no].arg4 = -1;
		if (strchr("MOEGPDTVQF", zone.cmd[cmd_no].command) == nullptr)    // a 3-arg command
		{
			const auto count = sscanf(ptr, " %d %d %d ",
									  &if_flag,
									  &zone.cmd[cmd_no].arg1,
									  &zone.cmd[cmd_no].arg2);
			if (count != 3) {
				error = 1;
			}
		} else if (zone.cmd[cmd_no].command == 'V')    // a string-arg command
		{
			const auto count = sscanf(ptr, " %d %d %d %d %s %s",
									  &if_flag,
									  &zone.cmd[cmd_no].arg1,
									  &zone.cmd[cmd_no].arg2,
									  &zone.cmd[cmd_no].arg3,
									  t1,
									  t2);
			if (count != 6) {
				error = 1;
			} else {
				zone.cmd[cmd_no].sarg1 = str_dup(t1);
				zone.cmd[cmd_no].sarg2 = str_dup(t2);
			}
		} else if (zone.cmd[cmd_no].command == 'Q')    // a number command
		{
			const auto count = sscanf(ptr, " %d %d",
									  &if_flag,
									  &zone.cmd[cmd_no].arg1);
			if (count != 2) {
				error = 1;
			} else {
				if_flag = 0;
			}
		} else {
			const auto count = sscanf(ptr, " %d %d %d %d %d", &if_flag,
									  &zone.cmd[cmd_no].arg1,
									  &zone.cmd[cmd_no].arg2,
									  &zone.cmd[cmd_no].arg3,
									  &zone.cmd[cmd_no].arg4);
			if (count < 4) {
				error = 1;
			}
		}
		zone.cmd[cmd_no].if_flag = if_flag;

		if (error) {
			log("SYSERR: Format error in %s, line %d: '%s'", full_file_name().c_str(), line_num, buf);
			exit(1);
		}
		zone.cmd[cmd_no].line = line_num;
		cmd_no++;
	}

	return true;
}

bool ZoneFile::load_generated_zone() const {
	auto &zone = zone_table[s_zone_number];

	sprintf(buf2, "beginning of generated zone #%d", zone.vnum);

	return true;
}

class HelpFile : public DataFile {
 public:
	HelpFile(const std::string &file_name) : DataFile(file_name) {}

	virtual bool load() override { return load_help(); }
	virtual std::string full_file_name() const override { return prefixes(DB_BOOT_HLP) + file_name(); }

	static shared_ptr create(const std::string &file_name) { return shared_ptr(new HelpFile(file_name)); }

 protected:
	bool load_help();
};

bool HelpFile::load_help() {
#if defined(CIRCLE_MACINTOSH)
	static char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];	// ?
#else
	char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
#endif
	char line[READ_SIZE + 1];
	const char *scan;

	// get the first keyword line
	get_one_line(key);
	while (*key != '$')    // read in the corresponding help entry
	{
		strcpy(entry, strcat(key, "\r\n"));
		get_one_line(line);
		while (*line != '#') {
			// если вдруг файл внезапно закончился и '#' так и не встретился
			// логаем ошибку и заканчиваем парсинг во избежание зацикливания
			if ((*line == '$') && (*(line + 1) == 0)) {
				std::stringstream str_log;
				str_log << "SYSERR: unexpected EOF in help file: \"" << file_name() << "\"";
				mudlog(str_log.str().c_str(), DEF, kLvlImmortal, SYSLOG, true);
				break;
			}
			strcat(entry, strcat(line, "\r\n"));
			get_one_line(line);
		}
		// Assign read level
		int min_level = 0;
		if ((*line == '#') && (*(line + 1) != 0)) {
			min_level = atoi((line + 1));
		}
		min_level = MAX(0, MIN(min_level, kLvlImplementator));
		// now, add the entry to the index with each keyword on the keyword line
		std::string entry_str(entry);
		scan = one_word(key, next_key);
		while (*next_key) {
			std::string key_str(next_key);
			HelpSystem::add_static(key_str, entry_str, min_level);
			scan = one_word(scan, next_key);
		}

		// get next keyword line (or $)
		get_one_line(key);
	}

	return true;
}

class SocialsFile : public DataFile {
 public:
	SocialsFile(const std::string &file_name) : DataFile(file_name) {}

	virtual bool load() override { return load_socials(); }
	virtual std::string full_file_name() const override { return prefixes(DB_BOOT_SOCIAL) + file_name(); }

	static shared_ptr create(const std::string &file_name) { return shared_ptr(new SocialsFile(file_name)); }

 private:
	bool load_socials();
	char *str_dup_bl(const char *source);
};

bool SocialsFile::load_socials() {
	char line[kMaxInputLength], next_key[kMaxInputLength];
	const char *scan;
	int key = -1, message = -1, c_min_pos, c_max_pos, v_min_pos, v_max_pos, what;

	// get the first keyword line
	get_one_line(line);
	while (*line != '$') {
		message++;
		scan = one_word(line, next_key);
		while (*next_key) {
			key++;
			// Не нужен лишний спам, только мешает искать ошибки
			//log("Social %d '%s' - message %d", key, next_key, message);
			soc_keys_list[key].keyword = str_dup(next_key);
			soc_keys_list[key].social_message = message;
			scan = one_word(scan, next_key);
		}

		what = 0;
		get_one_line(line);
		while (*line != '#') {
			scan = line;
			skip_spaces(&scan);
			if (scan && *scan && *scan != ';') {
				switch (what) {
					case 0:
						if (sscanf(scan, " %d %d %d %d \n", &c_min_pos, &c_max_pos, &v_min_pos, &v_max_pos) < 4) {
							log("SYSERR: format error in %d social file near social '%s' #d #d #d #d\n", message, line);
							exit(1);
						}
						soc_mess_list[message].ch_min_pos = static_cast<EPosition>(c_min_pos);
						soc_mess_list[message].ch_max_pos = static_cast<EPosition>(c_max_pos);
						soc_mess_list[message].vict_min_pos = static_cast<EPosition>(v_min_pos);
						soc_mess_list[message].vict_max_pos = static_cast<EPosition>(v_max_pos);
						break;
					case 1: soc_mess_list[message].char_no_arg = str_dup_bl(scan);
						break;
					case 2: soc_mess_list[message].others_no_arg = str_dup_bl(scan);
						break;
					case 3: soc_mess_list[message].char_found = str_dup_bl(scan);
						break;
					case 4: soc_mess_list[message].others_found = str_dup_bl(scan);
						break;
					case 5: soc_mess_list[message].vict_found = str_dup_bl(scan);
						break;
					case 6: soc_mess_list[message].not_found = str_dup_bl(scan);
						break;
				}
			}

			if (!scan || *scan != ';') {
				what++;
			}

			get_one_line(line);
		}
		// get next keyword line (or $)
		get_one_line(line);
	}

	return true;
}

char *SocialsFile::str_dup_bl(const char *source) {
	char line[kMaxInputLength];

	line[0] = 0;
	if (source[0]) {
		strcat(line, "&K");
		strcat(line, source);
		strcat(line, "&n");
	}

	return (str_dup(line));
}

DataFileFactory::shared_ptr DataFileFactory::create() {
	return std::make_shared<DataFileFactoryImpl>();
}

BaseDataFile::shared_ptr DataFileFactoryImpl::get_file(const EBootType mode, const std::string &file_name) {
	switch (mode) {
		case DB_BOOT_WLD:return WorldFile::create(file_name);

		case DB_BOOT_MOB:return MobileFile::create(file_name);

		case DB_BOOT_OBJ:return ObjectFile::create(file_name);

		case DB_BOOT_ZON:return ZoneFile::create(file_name);

		case DB_BOOT_HLP:return HelpFile::create(file_name);

		case DB_BOOT_TRG:return TriggersFile::create(file_name, m_load_obj_exp);

		case DB_BOOT_SOCIAL:return SocialsFile::create(file_name);

		default:return nullptr;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
