// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <sstream>
#include "char.hpp"
#include "utils.h"
#include "db.h"
#include "pk.h"
#include "im.h"
#include "handler.h"
#include "interpreter.h"
#include "boards.h"
#include "privilege.hpp"
#include "skills.h"
#include "constants.h"
#include "char_player.hpp"
#include "spells.h"
#include "comm.h"

Character::Character()
		:
		protecting_(0),
		touching_(0),
		fighting_(0),
		nr(NOBODY),
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
		Poisoner(0),
		ing_list(0),
		dl_list(0)
{
	memset(&extra_attack_, 0, sizeof(extra_attack_type));
	memset(&cast_attack_, 0, sizeof(cast_attack_type));

	memset(&player_data, 0, sizeof(char_player_data));
	memset(&add_abils, 0, sizeof(char_played_ability_data));
	memset(&real_abils, 0, sizeof(char_ability_data));
	memset(&points, 0, sizeof(char_point_data));
	memset(&char_specials, 0, sizeof(char_special_data));
	memset(&mob_specials, 0, sizeof(mob_special_data));

	for (int i = 0; i < NUM_WEARS; i++)
		equipment[i] = 0;

	memset(&MemQueue, 0, sizeof(spell_mem_queue));
	memset(&Temporary, 0, sizeof(FLAG_DATA));
	memset(&BattleAffects, 0, sizeof(FLAG_DATA));

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

void Character::create_player()
{
	player.reset(new Player);
}

void Character::create_mob_guard()
{
	player = Player::shared_mob;
}

/**
* Капитально расширенная версия сислога для теста battle_list.
*/
void battle_log(const char *format, ...)
{
	const char *filename = "../log/battle.log";
	static FILE *file = 0;
	if (!file)
	{
		file = fopen(filename, "a");
		if (!file)
		{
			log("SYSERR: can't open %s!", filename);
			return;
		}
		opened_files.push_back(file);
	}
	else if (!format)
		format = "SYSERR: // battle_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
#ifdef LOG_AUTOFLUSH
	fflush(file);
#endif
}

#define START_DEBUG battle_log("%s: %s start", GET_NAME(this), __func__)
#define STOP_DEBUG battle_log("%s: %s stop", GET_NAME(this), __func__)

// battle_list >> //////////////////////////////////////////////////////////////

/**
* Добавляем vict в свой список.
*/
void Character::add_battle_list(CHAR_DATA *vict)
{
START_DEBUG;
	BattleListType::iterator it = std::find(battle_list_.begin(), battle_list_.end(), vict);
	if (it == battle_list_.end())
	{
		battle_log("%s: add to list %s", GET_NAME(this), GET_NAME(vict));
		battle_list_.push_back(vict);
	}
	else
		battle_log("%s: do nothing, %s is already in list", GET_NAME(this), GET_NAME(vict));
STOP_DEBUG;
}

/**
* Проверяем есть ли у нас еще что-то, связанное с vict и удаляем его из списка,
* если нас с ним уже ничего не связывает.
*/
void Character::check_battle_list(CHAR_DATA *vict)
{
START_DEBUG;
	if (vict->get_protecting() != this
		&& vict->get_touching() != this
		&& vict->get_fighting() != this
		&& vict->get_extra_victim() != this
		&& vict->get_cast_char() != this)
	{
		BattleListType::iterator it = std::find(battle_list_.begin(), battle_list_.end(), vict);
		if (it != battle_list_.end())
		{
			battle_log("%s: remove from list %s", GET_NAME(this), GET_NAME(vict));
			battle_list_.erase(it);
		}
		else
			battle_log("%s: do nothing, %s is not in list", GET_NAME(this), GET_NAME(vict));
	}
STOP_DEBUG;
}

/**
* Чистим поля, связанные с нами у чаров, находящихся в нашем списке, и сам список.
*/
void Character::clear_battle_list()
{
START_DEBUG;
	BattleListType::iterator next_it;
	for (BattleListType::iterator it = battle_list_.begin(); it != battle_list_.end(); it = next_it)
	{
		CHAR_DATA *vict = *it;
		next_it = ++it;
		battle_log("%s: checking %s", GET_NAME(this), GET_NAME(vict));
		// пользовать it дальше нельзя
		if (vict->get_protecting() == this)
		{
			vict->set_protecting(0);
			CLR_AF_BATTLE(vict, EAF_PROTECT);
		}
		if (vict->get_touching() == this)
		{
			vict->set_touching(0);
			CLR_AF_BATTLE(vict, EAF_PROTECT);
		}
		if (vict->get_fighting() == this && IN_ROOM(vict) != NOWHERE)
		{
			log("[Change fighting] Change victim");
			CHAR_DATA *j;
			for (j = world[IN_ROOM(this)]->people; j; j = j->next_in_room)
			{
				if (j->get_fighting() == vict)
				{
					act("Вы переключили внимание на $N3.", FALSE, vict, 0, j, TO_CHAR);
					act("$n переключил$u на Вас !", FALSE, vict, 0, j, TO_VICT);
					vict->set_fighting(j);
					break;
				}
			}
			if (!j)
				stop_fighting(vict, FALSE);
		}
		if (vict->get_extra_victim() == this)
		{
			vict->set_extra_attack(0, 0);
		}
		if (vict->get_cast_char() == this)
		{
			vict->set_cast(0, 0, 0, 0, 0);
		}
	}
	battle_list_.clear();
STOP_DEBUG;
}

// << battle_list //////////////////////////////////////////////////////////////

// эти поля завязаны на battle_list >> /////////////////////////////////////////

CHAR_DATA * Character::get_protecting() const
{
	return protecting_;
}

void Character::set_protecting(CHAR_DATA *vict)
{
START_DEBUG;
	battle_log("%s: protecting_ = %s, vict = ", GET_NAME(this), protecting_ ? GET_NAME(protecting_) : "null", vict ? GET_NAME(vict) : "null");
	if (protecting_ && protecting_ != vict)
	{
		// чистим старую цель
		CHAR_DATA *tmp_vict = protecting_;
		protecting_ = 0;
		tmp_vict->check_battle_list(this);
	}

	if (vict != protecting_)
		vict->add_battle_list(this);

	protecting_ = vict;
STOP_DEBUG;
}

CHAR_DATA * Character::get_touching() const
{
	return touching_;
}

void Character::set_touching(CHAR_DATA *vict)
{
START_DEBUG;
	battle_log("%s: touching_ = %s, vict = ", GET_NAME(this), touching_ ? GET_NAME(touching_) : "null", vict ? GET_NAME(vict) : "null");
	if (touching_ && touching_ != vict)
	{
		// чистим старую цель
		CHAR_DATA *tmp_vict = touching_;
		touching_ = 0;
		tmp_vict->check_battle_list(this);
	}

	if (vict != touching_)
		vict->add_battle_list(this);

	touching_ = vict;
STOP_DEBUG;
}

CHAR_DATA * Character::get_fighting() const
{
	return fighting_;
}

void Character::set_fighting(CHAR_DATA *vict)
{
START_DEBUG;
	battle_log("%s: fighting_ = %s, vict = ", GET_NAME(this), fighting_ ? GET_NAME(fighting_) : "null", vict ? GET_NAME(vict) : "null");
	if (fighting_ && fighting_ != vict)
	{
		// чистим старую цель
		CHAR_DATA *tmp_vict = fighting_;
		fighting_ = 0;
		tmp_vict->check_battle_list(this);
	}

	if (vict != fighting_)
		vict->add_battle_list(this);

	fighting_ = vict;
STOP_DEBUG;
}

int Character::get_extra_skill() const
{
	return extra_attack_.used_skill;
}

CHAR_DATA * Character::get_extra_victim() const
{
	return extra_attack_.victim;
}

void Character::set_extra_attack(int skill, CHAR_DATA *vict)
{
START_DEBUG;
	battle_log("%s: extra_attack_.victim = %s, vict = ", GET_NAME(this), extra_attack_.victim ? GET_NAME(extra_attack_.victim) : "null", vict ? GET_NAME(vict) : "null");
	if (extra_attack_.victim && extra_attack_.victim != vict)
	{
		// чистим старую цель
		CHAR_DATA *tmp_vict = extra_attack_.victim;
		extra_attack_.victim = 0;
		tmp_vict->check_battle_list(this);
	}

	if (vict != extra_attack_.victim)
		vict->add_battle_list(this);

	extra_attack_.used_skill = skill;
	extra_attack_.victim = vict;
STOP_DEBUG;
}

void Character::set_cast(int spellnum, int spell_subst, CHAR_DATA *tch, OBJ_DATA *tobj, ROOM_DATA *troom)
{
START_DEBUG;
	battle_log("%s: cast_attack_.tch = %s, tch = ", GET_NAME(this), cast_attack_.tch ? GET_NAME(cast_attack_.tch) : "null", tch ? GET_NAME(tch) : "null");
	if (cast_attack_.tch && cast_attack_.tch != tch)
	{
		// чистим старую цель
		CHAR_DATA *tmp_vict = cast_attack_.tch;
		cast_attack_.tch = 0;
		tmp_vict->check_battle_list(this);
	}

	if (tch != cast_attack_.tch)
		tch->add_battle_list(this);

	cast_attack_.spellnum = spellnum;
	cast_attack_.spell_subst = spell_subst;
	cast_attack_.tch = tch;
	cast_attack_.tobj = tobj;
	cast_attack_.troom = troom;
STOP_DEBUG;
}

int Character::get_cast_spell() const
{
	return cast_attack_.spellnum;
}

int Character::get_cast_subst() const
{
	return cast_attack_.spell_subst;
}

CHAR_DATA * Character::get_cast_char() const
{
	return cast_attack_.tch;
}

OBJ_DATA * Character::get_cast_obj() const
{
	return cast_attack_.tobj;
}

// << эти поля завязаны на battle_list /////////////////////////////////////////
