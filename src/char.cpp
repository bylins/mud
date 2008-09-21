// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <sstream>
#include "char.hpp"
#include "utils.h"
#include "db.h"
#include "mobmax.h"
#include "pk.h"
#include "im.h"
#include "handler.h"
#include "interpreter.h"
#include "boards.h"
#include "privilege.hpp"
#include "skills.h"
#include "constants.h"

Character::Character()
		: nr(NOBODY),
		in_room(0),
		wait(0),
		punctual_wait(0),
		last_comm(0),
		player_specials(0),
		affected(0),
		timed(0),
		timed_feat(0),
		carrying(0),
		desc(0),
		id(0),
		proto_script(0),
		script(0),
		memory(0),
		next_in_room(0),
		next(0),
		next_fighting(0),
		followers(0),
		master(0),
		CasterLevel(0),
		DamageLevel(0),
		pk_list(0),
		helpers(0),
		track_dirs(0),
		CheckAggressive(0),
		ExtractTimer(0),
		Initiative(0),
		BattleCounter(0),
		Protecting(0),
		Touching(0),
		Poisoner(0),
		ing_list(0),
		dl_list(0)
{
	memset(&player_data, 0, sizeof(char_player_data));
	memset(&add_abils, 0, sizeof(char_played_ability_data));
	memset(&real_abils, 0, sizeof(char_ability_data));
	memset(&points, 0, sizeof(char_point_data));
	memset(&char_specials, 0, sizeof(char_special_data));
	memset(&mob_specials, 0, sizeof(mob_special_data));

	for (int i = 0; i < NUM_WEARS; i++)
		equipment[i] = 0;

	memset(&MemQueue, 0, sizeof(spell_mem_queue));
	memset(&Questing, 0, sizeof(quest_data));
	memset(&MobKill, 0, sizeof(mob_kill_data));
	memset(&Temporary, 0, sizeof(FLAG_DATA));
	memset(&BattleAffects, 0, sizeof(FLAG_DATA));
	memset(&extra_attack, 0, sizeof(extra_attack_type));
	memset(&cast_attack, 0, sizeof(cast_attack_type));

	char_specials.position = POS_STANDING;
	mob_specials.default_pos = POS_STANDING;
}

Character::~Character()
{
	if (GET_NAME(this))
		log("[FREE CHAR] (%s)", GET_NAME(this));

	int i, j, id = -1;
	struct alias_data *a;
	struct helper_data_type *temp;

	if (!IS_NPC(this) && GET_NAME(this))
	{
		id = get_ptable_by_name(GET_NAME(this));
		if (id >= 0)
		{
			player_table[id].level = (GET_REMORT(this) ? 30 : GET_LEVEL(this));
			player_table[id].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
		}
	}

	if (!IS_NPC(this) || (IS_NPC(this) && GET_MOB_RNUM(this) == -1))
	{	/* if this is a player, or a non-prototyped non-player, free all */
		if (GET_NAME(this))
			free(GET_NAME(this));

		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(this, j))
				free(GET_PAD(this, j));

		if (this->player_data.title)
			free(this->player_data.title);

		if (this->player_data.short_descr)
			free(this->player_data.short_descr);

		if (this->player_data.long_descr)
			free(this->player_data.long_descr);

		if (this->player_data.description)
			free(this->player_data.description);

		if (IS_NPC(this) && this->mob_specials.Questor)
			free(this->mob_specials.Questor);

		if (this->Questing.quests)
			free(this->Questing.quests);

		free_mkill(this);

		pk_free_list(this);

		while (this->helpers)
			REMOVE_FROM_LIST(this->helpers, this->helpers, next_helper);
	}
	else if ((i = GET_MOB_RNUM(this)) >= 0)
	{	/* otherwise, free strings only if the string is not pointing at proto */
		if (this->player_data.name && this->player_data.name != mob_proto[i].player_data.name)
			free(this->player_data.name);

		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(this, j)
					&& (this->player_data.PNames[j] != mob_proto[i].player_data.PNames[j]))
				free(this->player_data.PNames[j]);

		if (this->player_data.title && this->player_data.title != mob_proto[i].player_data.title)
			free(this->player_data.title);

		if (this->player_data.short_descr && this->player_data.short_descr != mob_proto[i].player_data.short_descr)
			free(this->player_data.short_descr);

		if (this->player_data.long_descr && this->player_data.long_descr != mob_proto[i].player_data.long_descr)
			free(this->player_data.long_descr);

		if (this->player_data.description && this->player_data.description != mob_proto[i].player_data.description)
			free(this->player_data.description);

		if (this->mob_specials.Questor && this->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
			free(this->mob_specials.Questor);
	}

	supress_godsapply = TRUE;
	while (this->affected)
		affect_remove(this, this->affected);
	supress_godsapply = FALSE;

	while (this->timed)
		timed_from_char(this, this->timed);

	if (this->desc)
		this->desc->character = NULL;

	if (this->player_specials != NULL && this->player_specials != &dummy_mob)
	{
		while ((a = GET_ALIASES(this)) != NULL)
		{
			GET_ALIASES(this) = (GET_ALIASES(this))->next;
			free_alias(a);
		}
		if (this->player_specials->poofin)
			free(this->player_specials->poofin);
		if (this->player_specials->poofout)
			free(this->player_specials->poofout);
		/* рецепты */
		while (GET_RSKILL(this) != NULL)
		{
			im_rskill *r;
			r = GET_RSKILL(this)->link;
			free(GET_RSKILL(this));
			GET_RSKILL(this) = r;
		}
		/* порталы */
		while (GET_PORTALS(this) != NULL)
		{
			struct char_portal_type *prt_next;
			prt_next = GET_PORTALS(this)->next;
			free(GET_PORTALS(this));
			GET_PORTALS(this) = prt_next;
		}
// Cleanup punish reasons
		if (MUTE_REASON(this))
			free(MUTE_REASON(this));
		if (DUMB_REASON(this))
			free(DUMB_REASON(this));
		if (HELL_REASON(this))
			free(HELL_REASON(this));
		if (FREEZE_REASON(this))
			free(FREEZE_REASON(this));
		if (NAME_REASON(this))
			free(NAME_REASON(this));
// End reasons cleanup

		if (KARMA(this))
			free(KARMA(this));

		if (GET_LAST_ALL_TELL(this))
			free(GET_LAST_ALL_TELL(this));
		free(GET_LOGS(this));
// shapirus: подчистим за криворукуми кодерами memory leak,
// вызванный неосвобождением фильтра базара...
		if (EXCHANGE_FILTER(this))
			free(EXCHANGE_FILTER(this));
		EXCHANGE_FILTER(this) = NULL;	// на всякий случай
// ...а заодно и игнор лист *смущ :)
		while (IGNORE_LIST(this))
		{
			struct ignore_data *ign_next;
			ign_next = IGNORE_LIST(this)->next;
			free(IGNORE_LIST(this));
			IGNORE_LIST(this) = ign_next;
		}
		IGNORE_LIST(this) = NULL;

		if (GET_CLAN_STATUS(this))
			free(GET_CLAN_STATUS(this));

		// Чистим лист логонов
		while (LOGON_LIST(this))
		{
			struct logon_data *log_next;
			log_next = LOGON_LIST(this)->next;
			free(this->player_specials->logons->ip);
			delete LOGON_LIST(this);
			LOGON_LIST(this) = log_next;
		}
		LOGON_LIST(this) = NULL;

		if (GET_BOARD(this))
			delete GET_BOARD(this);
		GET_BOARD(this) = 0;

		free(this->player_specials);
		this->player_specials = NULL;	// чтобы словить ACCESS VIOLATION !!!
		if (IS_NPC(this))
			log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(this), GET_MOB_VNUM(this));
	}
}

/**
*
*/
int Character::get_skill(int skill_num)
{
	if (Privilege::check_skills(this))
	{
		CharSkillsType::iterator it = skills.find(skill_num);
		if (it != skills.end())
			return it->second;
	}
	return 0;
}

/**
* Нулевой скилл мы не сетим, а при обнулении уже имеющегося удалем эту запись.
*/
void Character::set_skill(int skill_num, int percent)
{
	if (skill_num < 0 || skill_num > MAX_SKILL_NUM)
	{
		log("SYSERROR: неизвесный номер скилла %d в set_skill.", skill_num);
		return;
	}

	CharSkillsType::iterator it = skills.find(skill_num);
	if (it != skills.end())
	{
		if (percent)
			it->second = percent;
		else
			skills.erase(it);
	}
	else if (percent)
		skills[skill_num] = percent;
}

/**
*
*/
void Character::clear_skills()
{
	skills.clear();
}

/**
*
*/
int Character::get_skills_count()
{
	return skills.size();
}

////////////////////////////////////////////////////////////////////////////////
void Character::create_player()
{
	player.reset(new Player);
}

static PlayerPtr shared_mob(new Player);
#define CHECK_MOB_GUARD(player) if (player == shared_mob) log("SYSERR: Mob using player %s.", __func__)

void Character::create_mob_guard()
{
	player = shared_mob;
}

Player::Player()
	: pfilepos_(-1),
	was_in_room_(NOWHERE)
{

}

int Character::get_pfilepos() const
{
	CHECK_MOB_GUARD(player);
	return player->pfilepos_;
}

void Character::set_pfilepos(int pfilepos)
{
	CHECK_MOB_GUARD(player);
	player->pfilepos_ = pfilepos;
}

room_rnum Character::get_was_in_room() const
{
	CHECK_MOB_GUARD(player);
	return player->was_in_room_;
}

void Character::set_was_in_room(room_rnum was_in_room)
{
	CHECK_MOB_GUARD(player);
	player->was_in_room_ = was_in_room;
}

std::string const & Character::get_passwd() const
{
	CHECK_MOB_GUARD(player);
	return player->passwd_;
}

void Character::set_passwd(std::string const & passwd)
{
	CHECK_MOB_GUARD(player);
	player->passwd_ = passwd;
}
////////////////////////////////////////////////////////////////////////////////
