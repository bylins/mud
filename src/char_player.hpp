// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_HPP_INCLUDED
#define CHAR_PLAYER_HPP_INCLUDED

#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

// Значит для добавления нового поля и его методов сейчас нужно прописать:
// чисто виртуальные методы в PlayerI, поле и методы в Player, методы в PlayerProxy

/**
* Интерфейс для плеер класса и его обертки.
*/
class PlayerI
{
public:
	virtual ~PlayerI() = 0;

private:
	virtual int get_pfilepos() const = 0;
	virtual void set_pfilepos(int pfilepos) = 0;

	virtual room_rnum get_was_in_room() const = 0;
	virtual void set_was_in_room(room_rnum was_in_room) = 0;

	virtual std::string const & get_passwd() const = 0;
	virtual void set_passwd(std::string const & passwd) = 0;
};

/**
* Здесь собственно поля и методы плеера.
*/
class Player : public PlayerI
{
public:
	Player();

	int get_pfilepos() const;
	void set_pfilepos(int pfilepos);

	room_rnum get_was_in_room() const;
	void set_was_in_room(room_rnum was_in_room);

	std::string const & get_passwd() const;
	void set_passwd(std::string const & passwd);

private:
	// порядковый номер в файле плеер-листа (не особо нужен, но бывает удобно видеть по кто)
	int pfilepos_;
	// комната, в которой был чар до того, как его поместили в странную (linkdrop)
	room_rnum was_in_room_;
	// хэш пароля
	std::string passwd_;
};

#endif // CHAR_PLAYER_HPP_INCLUDED
