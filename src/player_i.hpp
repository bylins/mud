// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PLAYER_I_HPP_INCLUDED
#define PLAYER_I_HPP_INCLUDED

#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

extern room_rnum r_helled_start_room;

/**
* Интерфейс плеера с пустыми функциями, находящийся у моба.
* Соответственно здесь должны повторяться все открытые методы Player.
* TODO: логирование на mob using как раньше в плеере.
*/
class PlayerI
{
public:
	virtual int get_pfilepos() const { return -1; };
	virtual void set_pfilepos(int pfilepos) {};

	virtual room_rnum get_was_in_room() const { return NOWHERE; };
	virtual void set_was_in_room(room_rnum was_in_room) {};

	virtual std::string const & get_passwd() const { return empty_const_str; };
	virtual void set_passwd(std::string const & passwd) {};

	virtual room_rnum get_from_room() const { return r_helled_start_room; };
	virtual void set_from_room(room_rnum was_in_room) {};

	virtual void remort() {};
	virtual void reset() {};

	virtual int get_start_stat(int num) { return 0; };
	virtual void set_start_stat(int stat_num, int number) {};

	virtual void set_last_tell(const char *text) {};
	virtual std::string const & get_last_tell() { return empty_const_str; };

	virtual void set_answer_id(int id) {};
	virtual int get_answer_id() const { return NOBODY; };

	virtual void remember_add(std::string text, int flag) {};
	virtual std::string remember_get(int flag) const { return ""; };
	virtual bool remember_set_num(unsigned int num) { return false; };
	virtual unsigned int remember_get_num() const { return 0; };

	virtual void quested_add(CHAR_DATA *ch, int vnum, char *text) {};
	virtual bool quested_remove(int vnum) { return false; };
	virtual bool quested_get(int vnum) const { return false; };
	virtual std::string quested_get_text(int vnum) const { return ""; };
	virtual std::string quested_print() const { return ""; };
	virtual void quested_save(FILE *saved) const {};

	virtual int mobmax_get(int vnum) const { return 0; };
	virtual void mobmax_add(CHAR_DATA *ch, int vnum, int count, int level) {};
	virtual void mobmax_load(CHAR_DATA *ch, int vnum, int count, int level) {};
	virtual void mobmax_remove(int vnum) {};
	virtual void mobmax_save(FILE *saved) const {};

protected:
	PlayerI() {};
	virtual ~PlayerI() {};

private:
	static std::string empty_const_str;
};

#endif // PLAYER_I_HPP_INCLUDED
