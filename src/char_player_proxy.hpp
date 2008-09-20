// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_PROXY_HPP_INCLUDED
#define CHAR_PLAYER_PROXY_HPP_INCLUDED

#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "char_player.hpp"

typedef boost::shared_ptr<Player> PlayerPtr;

/**
* Обертки методов плеера для проверки на моба.
*/
class PlayerProxy : public PlayerI
{
public:
	int get_pfilepos() const;
	void set_pfilepos(int pfilepos);

	room_rnum get_was_in_room() const;
	void set_was_in_room(room_rnum was_in_room);

	std::string const & get_passwd() const;
	void set_passwd(std::string const & passwd);

	void create_player()
	{
		player.reset(new Player);
	}

private:
	PlayerPtr player;
	static Player mob;
};

#endif // CHAR_PLAYER_PROXY_HPP_INCLUDED
