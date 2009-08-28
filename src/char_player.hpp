// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_HPP_INCLUDED
#define CHAR_PLAYER_HPP_INCLUDED

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "quested.hpp"
#include "mobmax.hpp"
#include "remember.hpp"

// кол-во сохраняемых стартовых статов в файле
const int START_STATS_TOTAL = 6;

class Player
{
public:
	Player();
	int get_pfilepos() const;
	void set_pfilepos(int pfilepos);

	room_rnum get_was_in_room() const;
	void set_was_in_room(room_rnum was_in_room);

	std::string const & get_passwd() const;
	void set_passwd(std::string const & passwd);

	room_rnum get_from_room() const;
	void set_from_room(room_rnum was_in_room);

	void remort();
	void reset();

	int get_start_stat(int num);
	void set_start_stat(int stat_num, int number);

	void set_last_tell(const char *text);
	std::string & get_last_tell();

	// это все как обычно временно... =)
	friend void save_char(CHAR_DATA *ch);

	// внумы выполненных квестов
	Quested quested;
	// замаксы по отдельным мобам
	MobMax mobmax;

	// общие поля мобов
	static boost::shared_ptr<Player> shared_mob;
	// обертки для контроля за мобами
	void set_answer_id(int id);
	int get_answer_id() const;
	void add_remember(std::string text, int flag);
	std::string get_remember(int flag) const;
	bool set_remember_num(unsigned int num);
	unsigned int get_remember_num() const;

private:
	// порядковый номер в файле плеер-листа (не особо нужен, но бывает удобно видеть по кто)
	// TODO: вообще его можно пользовать вместо постоянного поиска по имени при сейвах чара и т.п. вещах, пользующих
	// get_ptable_by_name или find_name (дублирование кода кстати) и всякие поиски по ид/уид, если уже имеем чар-дату
	int pfilepos_;
	// комната, в которой был чар до того, как его поместили в странную (linkdrop)
	room_rnum was_in_room_;
	// хэш пароля
	std::string passwd_;
	// комната, где был чар до вызова char_from_room (was_in_room_ под это использовать не оч хорошо)
	// в данный момент поле нужно для проверки чара на бд при входе на арену любым способом, но может и еще потом пригодиться
	room_rnum from_room_;
	// стартовые статы
	boost::array<int, START_STATS_TOTAL> start_stats_;
	// вспомнить
	CharRemember remember_;
	// последняя введенная строка (от спама)
	std::string last_tell_;
};

#endif // CHAR_PLAYER_HPP_INCLUDED
