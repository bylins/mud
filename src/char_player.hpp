// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_HPP_INCLUDED
#define CHAR_PLAYER_HPP_INCLUDED

#include "conf.h"
#include "structs.h"

class PlayerBase
{
public:
	virtual int get_pfilepos() = 0;
	virtual void set_pfilepos(int num) = 0;
};

class Player : public PlayerBase
{
public:
	Player();
	virtual ~Player() {};

	int get_pfilepos();
	void set_pfilepos(int num);

private:
	int pfilepos; // playerfile pos
};

typedef boost::shared_ptr<Player> PlayerPtr;

class PlayerProxy : public PlayerBase
{
public:
	virtual ~PlayerProxy() {};

	int get_pfilepos();
	void set_pfilepos(int num);

	void create_player()
	{
		player.reset(new Player);
	}

private:
	PlayerPtr player;
	static Player mob;
};

#endif // CHAR_PLAYER_HPP_INCLUDED
