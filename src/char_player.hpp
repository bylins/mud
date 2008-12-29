// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_HPP_INCLUDED
#define CHAR_PLAYER_HPP_INCLUDED

#include <string>
#include <boost/shared_ptr.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "quested.hpp"
#include "mobmax.hpp"

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

	void add_reserved_money(int money);
	void remove_reserved_money(int money);
	int get_reserved_money() const;

	void inc_reserved_count();
	void dec_reserved_count();
	int get_reserved_count() const;

	// это все как обычно временно... =)
	friend void save_char(CHAR_DATA *ch);

	// внумы выполненных квестов
	Quested quested;
	// замаксы по отдельным мобам
	MobMax mobmax;
	// общие поля мобов
	static boost::shared_ptr<Player> shared_mob;

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
	// зарезервированные на почте куны (посылки)
	int reserved_money_;
	// кол-во шмоток уже находящихся в состоянии отправки
	int reserved_count_;
};

#endif // CHAR_PLAYER_HPP_INCLUDED
