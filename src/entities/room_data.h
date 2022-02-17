// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef ROOM_HPP_INCLUDED
#define ROOM_HPP_INCLUDED

#include "affects/affect_data.h"
#include "constants.h"
#include "dg_script/dg_scripts.h"
#include "entities/entities_constants.h"
#include "entities/obj_data.h"
#include "magic/magic_rooms.h"
#include "sysdep.h"

class ExitData {
 public:
	ExitData();
	~ExitData();

	void set_keyword(std::string const &value);
	void set_vkeyword(std::string const &value);
	// установить оба падежа - именительный и винительный, разделённые "|"
	void set_keywords(std::string const &value);
	RoomRnum to_room() const;
	void to_room(const RoomRnum _);

	std::string general_description;    // When look DIR.         //

	char *keyword;        // for open/close       //
	char *vkeyword;        // алиас двери в винительном падеже для открывания/закрывания

	byte exit_info;    // Exit info             //
	ubyte lock_complexity; //Polud сложность замка
	ObjVnum key;        // Key's number (-1 for no key) //

 private:
	RoomRnum to_room_;    // Where direction leads (kNowhere) //
};

struct TrackData {
	int track_info;        // bitvector //
	int who;            // real_number for NPC, IDNUM for PC //
	std::array<int, 6> time_income;    // time bitvector //
	std::array<int, 6> time_outgone;
	struct TrackData *next;
};

struct WeatherControl {
	int rainlevel;
	int snowlevel;
	int icelevel;
	int sky;
	int weather_type;
	int duration;
};

// Структура хранит разннобразные характеристики комнат //
struct RoomState {
	int poison; //Пока только степень зараженности для SPELL_POISONED_FOG//
};

struct RoomData {
	using exit_data_ptr = std::shared_ptr<ExitData>;
	using people_t = std::list<CharData *>;

	RoomData();
	~RoomData();

	RoomVnum room_vn;    // Rooms number  (vnum)                //
	ZoneRnum zone_rn;        // Room zone (for resetting)          //
	int sector_type;        // sector type (move/hide)            //
	int sector_state;        //*** External, change by weather     //

	char *name;        // Rooms name 'You are ...'           //
	size_t description_num;    // номер описания в глобальном списке
	char *temp_description; // для олц, пока редактора не будет нормального
	ExtraDescription::shared_ptr ex_description;    // for examine/look       //
	std::array<exit_data_ptr, kDirMaxNumber> dir_option;    // Directions //

	byte light;        // Number of lightsources in room //
	byte glight;        // Number of lightness person     //
	byte gdark;        // Number of darkness  person     //
	struct WeatherControl weather;        // Weather state for room //
	int (*func)(CharData *, void *, int, char *);

	ObjData::triggers_list_ptr proto_script;    // list of default triggers  //
	Script::shared_ptr script;    // script info for the object //
	struct TrackData *track;

	ObjData *contents;    // List of items in room              //
	people_t people;    // List of NPC / PC in room           //

	room_spells::RoomAffects affected;    // affected by what spells       //
	FlagData affected_by;    // флаги которые в отличии от room_flags появляются от аффектов
	//и не могут быть записаны на диск

	// Всякие характеристики комнаты
	ubyte fires;        // Time when fires - костерок    //
	ubyte ices;        // Time when ices restore //

	int portal_room;
	ubyte portal_time;    // Время жисти пентаграммы//
	long pkPenterUnique; //Постановщик пенты по мести

	int holes;        // Дырки для камне - копателей //

	// Параметры которые грузяться из файла (по крайней мере так планируется)
	struct RoomState base_property;
	// Добавки к параметрам  которые модифицируются аффектами ...
	struct RoomState add_property;

	int poison;        // Степень заражения территории в SPELL_POISONED_FOG //

	bool get_flag(const uint32_t flag) const { return m_room_flags.get(flag); }
	void set_flag(const uint32_t flag) { m_room_flags.set(flag); }
	void unset_flag(const uint32_t flag) { m_room_flags.unset(flag); }
	bool toggle_flag(const size_t plane, const uint32_t flag) { return m_room_flags.toggle_flag(plane, flag); }
	void clear_flags() { m_room_flags.clear(); }

	void flags_from_string(const char *flag) { m_room_flags.from_string(flag); };
	bool flags_sprint(char *result, const char *div, const int print_flag = 0) const {
		return m_room_flags.sprintbits(room_bits,
									   result,
									   div,
									   print_flag);
	}
	void flags_tascii(int num_planes, char *ascii) { m_room_flags.tascii(num_planes, ascii); }

	void gm_flag(char *subfield, const char *const *const list, char *res) {
		m_room_flags.gm_flag(subfield,
							 list,
							 res);
	}

	CharData *first_character() const;

	void cleanup_script();
	void set_name(std::string const &name);

 private:
	FlagData m_room_flags;    // DEATH,DARK ... etc //
};

#endif // ROOM_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
