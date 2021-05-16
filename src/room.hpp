// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef ROOM_HPP_INCLUDED
#define ROOM_HPP_INCLUDED

#include "dg_script/dg_scripts.h"
#include "obj.hpp"
#include "constants.h"
#include "structs.h"
#include "sysdep.h"
#include "affects/affect_data.h"

class EXIT_DATA
{
public:
	EXIT_DATA();
	~EXIT_DATA();

	void set_keyword(std::string const& value);
	void set_vkeyword(std::string const& value);
	// установить оба падежа - именительный и винительный, разделённые "|"
	void set_keywords(std::string const& value);
	room_rnum to_room() const;
	void to_room(const room_rnum _);

	std::string general_description;	// When look DIR.         //

	char *keyword;		// for open/close       //
	char *vkeyword;		// алиас двери в винительном падеже для открывания/закрывания

	byte exit_info;	// Exit info             //
	ubyte lock_complexity; //Polud сложность замка
	obj_vnum key;		// Key's number (-1 for no key) //

private:
	room_rnum m_to_room;	// Where direction leads (NOWHERE) //
};

struct track_data
{
	int track_info;		// bitvector //
	int who;			// real_number for NPC, IDNUM for PC //
	std::array<int, 6> time_income;	// time bitvector //
	std::array<int, 6> time_outgone;
	struct track_data *next;
};

struct weather_control
{
	int rainlevel;
	int snowlevel;
	int icelevel;
	int sky;
	int weather_type;
	int duration;
};

// Структура хранит разннобразные характеристики комнат //
struct room_property_data
{
	int poison; //Пока только степень зараженности для SPELL_POISONED_FOG//
};

struct ROOM_DATA
{
	using room_affects_list_t = std::list<AFFECT_DATA<ERoomApplyLocation>::shared_ptr>;
	using exit_data_ptr = std::shared_ptr<EXIT_DATA>;
	using people_t = std::list<CHAR_DATA*>;

	ROOM_DATA();
	~ROOM_DATA();

	room_vnum number;	// Rooms number  (vnum)                //
	zone_rnum zone;		// Room zone (for resetting)          //
	int sector_type;		// sector type (move/hide)            //
	int sector_state;		//*** External, change by weather     //

	char *name;		// Rooms name 'You are ...'           //
	size_t description_num;    // номер описания в глобальном списке
	char *temp_description; // для олц, пока редактора не будет нормального
	EXTRA_DESCR_DATA::shared_ptr ex_description;	// for examine/look       //
	std::array<exit_data_ptr, NUM_OF_DIRS> dir_option;	// Directions //

	byte light;		// Number of lightsources in room //
	byte glight;		// Number of lightness person     //
	byte gdark;		// Number of darkness  person     //
	struct weather_control weather;		// Weather state for room //
	int (*func)(CHAR_DATA*, void*, int, char*);

	OBJ_DATA::triggers_list_ptr proto_script;	// list of default triggers  //
	SCRIPT_DATA::shared_ptr script;	// script info for the object //
	struct track_data *track;

	OBJ_DATA *contents;	// List of items in room              //
	people_t people;	// List of NPC / PC in room           //

	room_affects_list_t affected;	// affected by what spells       //
	FLAG_DATA affected_by;	// флаги которые в отличии от room_flags появляются от аффектов
							//и не могут быть записаны на диск

	// Всякие характеристики комнаты
	ubyte fires;		// Time when fires - костерок    //
	ubyte ices;		// Time when ices restore //

	int portal_room;
	ubyte portal_time; 	// Время жисти пентаграммы//
	long pkPenterUnique; //Постановщик пенты по мести

	int holes;		// Дырки для камне - копателей //

	// Параметры которые грузяться из файла (по крайней мере так планируется)
	struct room_property_data	base_property;
	// Добавки к параметрам  которые модифицируются аффектами ...
	struct room_property_data	add_property;

	int poison;		// Степень заражения территории в SPELL_POISONED_FOG //

	bool get_flag(const uint32_t flag) const { return m_room_flags.get(flag); }
	void set_flag(const uint32_t flag) { m_room_flags.set(flag); }
	void unset_flag(const uint32_t flag) { m_room_flags.unset(flag); }
	bool toggle_flag(const size_t plane, const uint32_t flag) { return m_room_flags.toggle_flag(plane, flag); }
	void clear_flags() { m_room_flags.clear(); }

	void flags_from_string(const char *flag) { m_room_flags.from_string(flag); };
	bool flags_sprint(char *result, const char *div, const int print_flag = 0) const { return m_room_flags.sprintbits(room_bits, result, div, print_flag); }
	void flags_tascii(int num_planes, char* ascii) { m_room_flags.tascii(num_planes, ascii); }

	void gm_flag(char *subfield, const char * const * const list, char *res) { m_room_flags.gm_flag(subfield, list, res); }

	CHAR_DATA* first_character() const;

	void cleanup_script();
	void set_name(std::string const& name);

private:
	FLAG_DATA m_room_flags;	// DEATH,DARK ... etc //
};

#endif // ROOM_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
