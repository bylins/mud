// $RCSfile: char.cpp,v $     $Date: 2012/01/27 04:04:43 $     $Revision: 1.85 $
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <sstream>
#include <list>
#include <boost/format.hpp>
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
#include "room.hpp"
#include "player_races.hpp"
#include "celebrates.hpp"
#include "cache.hpp"
#include "fight.h"
#include "house.h"
#include "help.hpp"

std::string PlayerI::empty_const_str;
MapSystem::Options PlayerI::empty_map_options;

namespace
{

// список чаров/мобов после пуржа для последующего удаления оболочки
typedef std::vector<CHAR_DATA *> PurgedCharList;
PurgedCharList purged_char_list;

// * На перспективу - втыкать во все методы character.
void check_purged(const CHAR_DATA *ch, const char *fnc)
{
	if (ch->purged())
	{
		log("SYSERR: Using purged character (%s).", fnc);
	}
}

int normolize_skill(int percent)
{
	const static int KMinSkillPercent = 0;
	const static int KMaxSkillPercent = 200;

	if (percent < KMinSkillPercent)
		percent = KMinSkillPercent;
	else if (percent > KMaxSkillPercent)
		percent = KMaxSkillPercent;

	return percent;
}

// список для быстрого прогона по сражающимся при пурже моба
// при попадании в список чар остается в нем до своего экстракта
std::list<CHAR_DATA *> fighting_list;

} // namespace

////////////////////////////////////////////////////////////////////////////////

namespace CharacterSystem
{

// * Реальное удаление указателей.
void release_purged_list()
{
	for (PurgedCharList::iterator i = purged_char_list.begin();
		i != purged_char_list.end(); ++i)
	{
		delete *i;
	}
	purged_char_list.clear();
}

} // namespace CharacterSystem

////////////////////////////////////////////////////////////////////////////////

Character::Character()
	: role_(MOB_ROLE_TOTAL_NUM)
{
	this->zero_init();
	current_morph_ = NormalMorph::GetNormalMorph(this);
	caching::character_cache.add(this);
}

Character::~Character()
{
	if (!purged_)
	{
		this->purge(true);
	}
}

/**
* Обнуление всех полей Character (аналог конструктора),
* вынесено в отдельную функцию, чтобы дергать из purge().
*/
void Character::zero_init()
{
	protecting_ = 0;
	touching_ = 0;
	fighting_ = 0;
	in_fighting_list_ = 0;
	serial_num_ = 0;
	purged_ = 0;
	// на плеер-таблицу
	chclass_ = 0;
	level_ = 0;
	idnum_ = 0;
	uid_ = 0;
	exp_ = 0;
	remorts_ = 0;
	last_logon_ = 0;
	gold_ = 0;
	bank_gold_ = 0;
	str_ = 0;
	str_add_ = 0;
	dex_ = 0;
	dex_add_ = 0;
	con_ = 0;
	con_add_ = 0;
	wis_ = 0;
	wis_add_ = 0;
	int_ = 0;
	int_add_ = 0;
	cha_ = 0;
	cha_add_ = 0;
	role_.reset();
	attackers_.clear();
	restore_timer_ = 0;
	// char_data
	nr = NOBODY;
	in_room = 0;
	wait = 0;
	punctual_wait = 0;
	last_comm = 0;
	player_specials = 0;
	affected = 0;
	timed = 0;
	timed_feat = 0;
	carrying = 0;
	desc = 0;
	id = 0;
	proto_script = 0;
	script = 0;
	memory = 0;
	next_in_room = 0;
	next = 0;
	next_fighting = 0;
	followers = 0;
	master = 0;
	CasterLevel = 0;
	DamageLevel = 0;
	pk_list = 0;
	helpers = 0;
	track_dirs = 0;
	CheckAggressive = 0;
	ExtractTimer = 0;
	Initiative = 0;
	BattleCounter = 0;
	round_counter = 0;
	Poisoner = 0;
	ing_list = 0;
	dl_list = 0;

	memset(&extra_attack_, 0, sizeof(extra_attack_type));
	memset(&cast_attack_, 0, sizeof(cast_attack_type));
	memset(&player_data, 0, sizeof(char_player_data));
	memset(&add_abils, 0, sizeof(char_played_ability_data));
	memset(&real_abils, 0, sizeof(char_ability_data));
	memset(&points, 0, sizeof(char_point_data));
	memset(&char_specials, 0, sizeof(char_special_data));
	memset(&mob_specials, 0, sizeof(mob_special_data));

	for (int i = 0; i < NUM_WEARS; i++)
	{
		equipment[i] = 0;
	}

	memset(&MemQueue, 0, sizeof(spell_mem_queue));
	memset(&Temporary, 0, sizeof(FLAG_DATA));
	memset(&BattleAffects, 0, sizeof(FLAG_DATA));
	char_specials.position = POS_STANDING;
	mob_specials.default_pos = POS_STANDING;
}

/**
 * Освобождение выделенной в Character памяти, вынесено из деструктора,
 * т.к. есть необходимость дергать отдельно от delete.
 * \param destructor - true вызов произошел из дестркутора и обнулять/добавлять
 * в purged_list не нужно, по дефолту = false.
 *
 * Система в целом: там, где уже все обработано и почищено, и предполагается вызов
 * финального delete - вместо него вызывается метод purge(), в котором идет аналог
 * деструктора с полной очисткой объекта, но сама оболочка при этом сохраняется,
 * плюс инится флаг purged_, после чего по коду можно проверять старый указатель на
 * объект через метод purged() и, соответственно, не использовать его, если объект
 * по факту был удален. Таким образом мы не нарываемся на невалидные указатели, если
 * по ходу функции были спуржены чар/моб/шмотка. Гарантии ес-сно только в пределах
 * вызовов до выхода в обработку heartbeat(), где раз в минуту удаляются оболочки.
 */
void Character::purge(bool destructor)
{
	caching::character_cache.remove(this);
	if (purged_)
	{
		log("SYSERROR: double purge (%s:%d)", __FILE__, __LINE__);
		return;
	}
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
			player_table[id].level =
				(GET_REMORT(this) && !IS_IMMORTAL(this)) ? 30 : GET_LEVEL(this);
			player_table[id].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
		}
	}

	if (!IS_NPC(this) || (IS_NPC(this) && GET_MOB_RNUM(this) == -1))
	{	// if this is a player, or a non-prototyped non-player, free all
		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(this, j))
				free(GET_PAD(this, j));

		if (this->player_data.title)
			free(this->player_data.title);

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
	{	// otherwise, free strings only if the string is not pointing at proto
		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(this, j)
					&& (this->player_data.PNames[j] != mob_proto[i].player_data.PNames[j]))
				free(this->player_data.PNames[j]);

		if (this->player_data.title && this->player_data.title != mob_proto[i].player_data.title)
			free(this->player_data.title);

		if (this->player_data.long_descr && this->player_data.long_descr != mob_proto[i].player_data.long_descr)
			free(this->player_data.long_descr);

		if (this->player_data.description && this->player_data.description != mob_proto[i].player_data.description)
			free(this->player_data.description);

		if (this->mob_specials.Questor && this->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
			free(this->mob_specials.Questor);
	}

	while (this->affected)
		affect_remove(this, this->affected);

	while (this->timed)
		timed_from_char(this, this->timed);

	if (this->desc)
		this->desc->character = NULL;

	Celebrates::remove_from_mob_lists(this->id);

	// у мобов пока сохраняем это поле после пуржа, оно уберется
	// когда в Character вообще не останется этого player_specials
	bool keep_player_specials = (player_specials == &dummy_mob) ? true : false;

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
		// рецепты
		while (GET_RSKILL(this) != NULL)
		{
			im_rskill *r;
			r = GET_RSKILL(this)->link;
			free(GET_RSKILL(this));
			GET_RSKILL(this) = r;
		}
		// порталы
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

		free(this->player_specials);
		this->player_specials = NULL;	// чтобы словить ACCESS VIOLATION !!!
		if (IS_NPC(this))
			log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(this), GET_MOB_VNUM(this));
	}
	name_.clear();
	short_descr_.clear();

	if (!destructor)
	{
		// обнуляем все
		this->zero_init();
		// проставляем неподходящие из конструктора поля
		purged_ = true;
		char_specials.position = POS_DEAD;
		if (keep_player_specials)
		{
			player_specials = &dummy_mob;
		}
		// закидываем в список ожидающих делета указателей
		purged_char_list.push_back(this);
	}
}

// * Скилл с учетом всех плюсов и минусов от шмоток/яда.
int Character::get_skill(int skill_num) const
{
	int skill = get_trained_skill(skill_num) + get_equipped_skill(skill_num);
	if (AFF_FLAGGED(this, AFF_SKILLS_REDUCE))
	{
		skill -= skill * GET_POISON(this) / 100;
	}
	return normolize_skill(skill);
}

// * Скилл со шмоток.
int Character::get_equipped_skill(int skill_num) const
{
	int skill = 0;

// мобам и тем классам, у которых скилл является родным, учитываем скилл с каждой шмотки полностью,
// всем остальным -- не более 5% с шмотки
    // Пока что отменим это дело, народ морально не готов отказаться от автосников.
	//int is_native = IS_NPC(this) || skill_info[skill_num].classknow[chclass_][(int) GET_KIN(this)] == KNOW_SKILL;
	int is_native = true;
	for (int i = 0; i < NUM_WEARS; ++i)
	{
		if (equipment[i])
		{
			if (is_native)
				skill += equipment[i]->get_skill(skill_num);
			else
				skill += (MIN(5, equipment[i]->get_skill(skill_num)));
		}
	}
	skill += obj_bonus_.get_skill(skill_num);

	return skill;
}

// * Родной тренированный скилл чара.
int Character::get_inborn_skill(int skill_num)
{
	if (Privilege::check_skills(this))
	{
		CharSkillsType::iterator it = skills.find(skill_num);
		if (it != skills.end())
		{
			return normolize_skill(it->second);
		}
	}
	return 0;
}

int Character::get_trained_skill(int skill_num) const
{
	if (Privilege::check_skills(this))
	{
		return normolize_skill(current_morph_->get_trained_skill(skill_num));
	}
	return 0;
}

// * Нулевой скилл мы не сетим, а при обнулении уже имеющегося удалем эту запись.
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

void Character::set_morphed_skill(int skill_num, int percent)
{
	current_morph_->set_skill(skill_num, percent);
};

void Character::clear_skills()
{
	skills.clear();
}

int Character::get_skills_count() const
{
	return skills.size();
}

int Character::get_obj_slot(int slot_num)
{
	if (slot_num >= 0 && slot_num < MAX_ADD_SLOTS)
	{
		return add_abils.obj_slot[slot_num];
	}
	return 0;
}

void Character::add_obj_slot(int slot_num, int count)
{
	if (slot_num >= 0 && slot_num < MAX_ADD_SLOTS)
	{
		add_abils.obj_slot[slot_num] += count;
	}
}

void Character::set_touching(CHAR_DATA *vict)
{
	touching_ = vict;
	check_fighting_list();
}

CHAR_DATA * Character::get_touching() const
{
	return touching_;
}

void Character::set_protecting(CHAR_DATA *vict)
{
	protecting_ = vict;
	check_fighting_list();
}

CHAR_DATA * Character::get_protecting() const
{
	return protecting_;
}

void Character::set_fighting(CHAR_DATA *vict)
{
	fighting_ = vict;
	check_fighting_list();
}

CHAR_DATA * Character::get_fighting() const
{
	return fighting_;
}

void Character::set_extra_attack(int skill, CHAR_DATA *vict)
{
	extra_attack_.used_skill = skill;
	extra_attack_.victim = vict;
	check_fighting_list();
}

int Character::get_extra_skill() const
{
	return extra_attack_.used_skill;
}

CHAR_DATA * Character::get_extra_victim() const
{
	return extra_attack_.victim;
}

void Character::set_cast(int spellnum, int spell_subst, CHAR_DATA *tch, OBJ_DATA *tobj, ROOM_DATA *troom)
{
	cast_attack_.spellnum = spellnum;
	cast_attack_.spell_subst = spell_subst;
	cast_attack_.tch = tch;
	cast_attack_.tobj = tobj;
	cast_attack_.troom = troom;
	check_fighting_list();
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

void Character::check_fighting_list()
{
	/*
	if (!in_fighting_list_)
	{
		in_fighting_list_ = true;
		fighting_list.push_back(this);
	}
	*/
}

void Character::clear_fighing_list()
{
	if (in_fighting_list_)
	{
		in_fighting_list_ = false;
		std::list<CHAR_DATA *>::iterator it =  std::find(fighting_list.begin(), fighting_list.end(), this);
		if (it != fighting_list.end())
		{
			fighting_list.erase(it);
		}
	}
}

// * Внутри цикла чар нигде не пуржится и сам список соответственно не меняется.
void change_fighting(CHAR_DATA * ch, int need_stop)
{
	/*
	for (std::list<CHAR_DATA *>::const_iterator it = fighting_list.begin(); it != fighting_list.end(); ++it)
	{
		CHAR_DATA *k = *it;
		if (k->get_touching() == ch)
		{
			k->set_touching(0);
			CLR_AF_BATTLE(k, EAF_PROTECT); // сомнительно
		}
		if (k->get_protecting() == ch)
		{
			k->set_protecting(0);
			CLR_AF_BATTLE(k, EAF_PROTECT);
		}
		if (k->get_extra_victim() == ch)
		{
			k->set_extra_attack(0, 0);
		}
		if (k->get_cast_char() == ch)
		{
			k->set_cast(0, 0, 0, 0, 0);
		}
		if (k->get_fighting() == ch && IN_ROOM(k) != NOWHERE)
		{
			log("[Change fighting] Change victim");
			CHAR_DATA *j;
			for (j = world[IN_ROOM(ch)]->people; j; j = j->next_in_room)
				if (j->get_fighting() == k)
				{
					act("Вы переключили внимание на $N3.", FALSE, k, 0, j, TO_CHAR);
					act("$n переключил$u на вас!", FALSE, k, 0, j, TO_VICT);
					k->set_fighting(j);
					break;
				}
			if (!j && need_stop)
				stop_fighting(k, FALSE);
		}
	}
	*/

	CHAR_DATA *k, *j, *temp;
	for (k = character_list; k; k = temp)
	{
		temp = k->next;
		if (k->get_protecting() == ch)
		{
			k->set_protecting(0);
			CLR_AF_BATTLE(k, EAF_PROTECT);
		}
		if (k->get_touching() == ch)
		{
			k->set_touching(0);
			CLR_AF_BATTLE(k, EAF_PROTECT);
		}
		if (k->get_extra_victim() == ch)
		{
			k->set_extra_attack(0, 0);
		}
		if (k->get_cast_char() == ch)
		{
			k->set_cast(0, 0, 0, 0, 0);
		}
		if (k->get_fighting() == ch && IN_ROOM(k) != NOWHERE)
		{
			log("[Change fighting] Change victim");
			for (j = world[IN_ROOM(ch)]->people; j; j = j->next_in_room)
			{
				if (j->get_fighting() == k)
				{
					act("Вы переключили внимание на $N3.", FALSE, k, 0, j, TO_CHAR);
					act("$n переключил$u на вас!", FALSE, k, 0, j, TO_VICT);
					k->set_fighting(j);
					break;
				}
			}
			if (!j && need_stop)
			{
				stop_fighting(k, FALSE);
			}
		}
	}
}

int fighting_list_size()
{
	return fighting_list.size();
}

int Character::get_serial_num()
{
	return serial_num_;
}

void Character::set_serial_num(int num)
{
	serial_num_ = num;
}

bool Character::purged() const
{
	return purged_;
}

const std::string & Character::get_name_str() const
{
	if (IS_NPC(this))
	{
		return short_descr_;
	}
	return name_;
}

const char * Character::get_name() const
{
	if (IS_NPC(this))
	{
		return get_npc_name();
	}
	return get_pc_name();
}

void Character::set_name(const char *name)
{
	if (IS_NPC(this))
	{
		set_npc_name(name);
	}
	else
	{
		set_pc_name(name);
	}
}

const char * Character::get_pc_name() const
{
	return name_.empty() ? 0 : name_.c_str();
}

void Character::set_pc_name(const char *name)
{
	if (name)
	{
		name_ = name;
	}
	else
	{
		name_.clear();
	}
}

const char * Character::get_npc_name() const
{
	return short_descr_.empty() ? 0 : short_descr_.c_str();
}

void Character::set_npc_name(const char *name)
{
	if (name)
	{
		short_descr_ = name;
	}
	else
	{
		short_descr_.clear();
	}
}

const char* Character::get_pad(unsigned pad) const
{
	if (pad < player_data.PNames.size())
	{
		return player_data.PNames[pad];
	}
	return 0;
}

void Character::set_pad(unsigned pad, const char* s)
{
	if (pad >= player_data.PNames.size())
	{
		return;
	}

	int i;
	if (GET_PAD(this, pad))
	{
		 //if this is a player, or a non-prototyped non-player
		bool f = !IS_NPC(this) || (IS_NPC(this) && GET_MOB_RNUM(this) == -1);
		//prototype mob, field modified
		if (!f) f |= (i = GET_MOB_RNUM(this)) >= 0 && GET_PAD(this, pad) != GET_PAD(&mob_proto[i], pad);
		if (f) free(GET_PAD(this, pad));
	}
	GET_PAD(this, pad) = str_dup(s);
}

const char* Character::get_long_descr() const
{
	return player_data.long_descr;
}

void Character::set_long_descr(const char* s)
{
	int i;
	if (player_data.long_descr)
	{
		bool f = !IS_NPC(this) || (IS_NPC(this) && GET_MOB_RNUM(this) == -1); //if this is a player, or a non-prototyped non-player
		if (!f) f |= (i = GET_MOB_RNUM(this)) >= 0 && player_data.long_descr != mob_proto[i].player_data.long_descr; //prototype mob, field modified
		if (f) free(player_data.long_descr);
	}
	player_data.long_descr = str_dup(s);
}

const char* Character::get_description() const
{
	return player_data.description;
}

void Character::set_description(const char* s)
{
	int i;
	if (player_data.description)
	{
		bool f = !IS_NPC(this) || (IS_NPC(this) && GET_MOB_RNUM(this) == -1); //if this is a player, or a non-prototyped non-player
		if (!f) f |= (i = GET_MOB_RNUM(this)) >= 0 && player_data.description != mob_proto[i].player_data.description; //prototype mob, field modified
		if (f) free(player_data.description);
	}
	player_data.description = str_dup(s);
}

short Character::get_class() const
{
	return chclass_;
}

void Character::set_class(short chclass)
{
	if (chclass < 0 || chclass > CLASS_LAST_NPC)
	{
		log("WARNING: chclass=%d (%s:%d %s)", chclass, __FILE__, __LINE__, __func__);
	}
	chclass_ = chclass;
}

short Character::get_level() const
{
	return level_;
}

void Character::set_level(short level)
{
	if (IS_NPC(this))
	{
		level_ = std::max(static_cast<short>(1), std::min(MAX_MOB_LEVEL, level));
	}
	else
	{
		level_ = std::max(static_cast<short>(0), std::min(LVL_IMPL, level));
	}
}

long Character::get_idnum() const
{
	return idnum_;
}

void Character::set_idnum(long idnum)
{
	idnum_ = idnum;
}

int Character::get_uid() const
{
	return uid_;
}

void Character::set_uid(int uid)
{
	uid_ = uid;
}

long Character::get_exp() const
{
	return exp_;
}

void Character::set_exp(long exp)
{
	if (exp < 0)
	{
		log("WARNING: exp=%ld name=%s (%s:%d %s)", exp,
				get_name() ? get_name() : "null", __FILE__, __LINE__, __func__);
	}
	exp_ = MAX(0, exp);
}

short Character::get_remort() const
{
	return remorts_;
}

void Character::set_remort(short num)
{
	remorts_ = MAX(0, num);
}

time_t Character::get_last_logon() const
{
	return last_logon_;
}

void Character::set_last_logon(time_t num)
{
	last_logon_ = num;
}

byte Character::get_sex() const
{
	return player_data.sex;
}

void Character::set_sex(const byte v)
{
	if (v>=0 && v<NUM_SEXES)
		player_data.sex = v;
}

ubyte Character::get_weight() const
{
	return player_data.weight;
}

void Character::set_weight(const ubyte v)
{
	player_data.weight = v;
}

ubyte Character::get_height() const
{
	return player_data.height;
}

void Character::set_height(const ubyte v)
{
	player_data.height = v;
}

ubyte Character::get_religion() const
{
	return player_data.Religion;
}

void Character::set_religion(const ubyte v)
{
	if (v < 2)
	{
		player_data.Religion = v;
	}
}

ubyte Character::get_kin() const
{
	return player_data.Kin;
}

void Character::set_kin(const ubyte v)
{
	if (v < NUM_KIN)
	{
		player_data.Kin = v;
	}
}

ubyte Character::get_race() const
{
	return player_data.Race;
}

void Character::set_race(const ubyte v)
{
	player_data.Race = v;
}

int Character::get_hit() const
{
	return points.hit;
}

void Character::set_hit(const int v)
{
	if (v>=-10)
		points.hit = v;
}

int Character::get_max_hit() const
{
	return points.max_hit;
}

void Character::set_max_hit(const int v)
{
	if (v >= 0)
		points.max_hit = v;
}

sh_int Character::get_move() const
{
	return points.move;
}

void Character::set_move(const sh_int v)
{
	if (v >= 0)
		points.move = v;
}

sh_int Character::get_max_move() const
{
	return points.max_move;
}

void Character::set_max_move(const sh_int v)
{
	if (v >= 0)
		points.max_move = v;
}

long Character::get_gold() const
{
	return gold_;
}

long Character::get_bank() const
{
	return bank_gold_;
}

long Character::get_total_gold() const
{
	return get_gold() + get_bank();
}

/**
 * Добавление денег на руках, плюсуются только положительные числа.
 * \param need_log здесь и далее - логировать или нет изменения счета (=true)
 * \param clan_tax - проверять и снимать клан-налог или нет (=false)
 */
void Character::add_gold(long num, bool need_log, bool clan_tax)
{
	if (num < 0)
	{
		log("SYSERROR: num=%ld (%s:%d %s)", num, __FILE__, __LINE__, __func__);
		return;
	}
	if (clan_tax)
	{
		num -= ClanSystem::do_gold_tax(this, num);
	}
	set_gold(get_gold() + num, need_log);
}

// * см. add_gold()
void Character::add_bank(long num, bool need_log)
{
	if (num < 0)
	{
		log("SYSERROR: num=%ld (%s:%d %s)", num, __FILE__, __LINE__, __func__);
		return;
	}
	set_bank(get_bank() + num, need_log);
}

/**
 * Сет денег на руках, отрицательные числа просто обнуляют счет с
 * логированием бывшей суммы.
 */
void Character::set_gold(long num, bool need_log)
{
	if (get_gold() == num)
	{
		// чтобы с логированием не заморачиваться
		return;
	}
	num = MAX(0, MIN(MAX_MONEY_KEPT, num));

	if (need_log && !IS_NPC(this))
	{
		long change = num - get_gold();
		if (change > 0)
		{
			log("Gold: %s add %ld", get_name(), change);
		}
		else
		{
			log("Gold: %s remove %ld", get_name(), -change);
		}
		if (IN_ROOM(this) > 0)
		{
			MoneyDropStat::add(zone_table[world[IN_ROOM(this)]->zone].number, change);
		}
	}

	gold_ = num;
}

// * см. set_gold()
void Character::set_bank(long num, bool need_log)
{
	if (get_bank() == num)
	{
		// чтобы с логированием не заморачиваться
		return;
	}
	num = MAX(0, MIN(MAX_MONEY_KEPT, num));

	if (need_log && !IS_NPC(this))
	{
		long change = num - get_bank();
		if (change > 0)
		{
			log("Gold: %s add %ld", get_name(), change);
		}
		else
		{
			log("Gold: %s remove %ld", get_name(), -change);
		}
	}

	bank_gold_ = num;
}

/**
 * Снятие находящихся на руках денег.
 * \param num - положительное число
 * \return - кол-во кун, которое не удалось снять с рук (нехватило денег)
 */
long Character::remove_gold(long num, bool need_log)
{
	if (num < 0)
	{
		log("SYSERROR: num=%ld (%s:%d %s)", num, __FILE__, __LINE__, __func__);
		return num;
	}
	if (num == 0)
	{
		return num;
	}

	long rest = 0;
	if (get_gold() >= num)
	{
		set_gold(get_gold() - num, need_log);
	}
	else
	{
		rest = num - get_gold();
		set_gold(0, need_log);
	}

	return rest;
}

// * см. remove_gold()
long Character::remove_bank(long num, bool need_log)
{
	if (num < 0)
	{
		log("SYSERROR: num=%ld (%s:%d %s)", num, __FILE__, __LINE__, __func__);
		return num;
	}
	if (num == 0)
	{
		return num;
	}

	long rest = 0;
	if (get_bank() >= num)
	{
		set_bank(get_bank() - num, need_log);
	}
	else
	{
		rest = num - get_bank();
		set_bank(0, need_log);
	}

	return rest;
}

/**
 * Попытка снятия денег с банка и, в случае остатка, с рук.
 * \return - кол-во кун, которое не удалось снять (нехватило денег в банке и на руках)
 */
long Character::remove_both_gold(long num, bool need_log)
{
	long rest = remove_bank(num, need_log);
	return remove_gold(rest, need_log);
}

// * Удача (мораль) для расчетов в скилах и вывода чару по счет все.
int Character::calc_morale() const
{
	return GET_REAL_CHA(this) / 2 + GET_MORALE(this);
//	return cha_app[GET_REAL_CHA(this)].morale + GET_MORALE(this);
}
///////////////////////////////////////////////////////////////////////////////
int Character::get_str() const
{
	check_purged(this, "get_str");
	return current_morph_->GetStr();
}

int Character::get_inborn_str() const
{
	return str_;
}

void Character::set_str(int param)
{
	str_ = MAX(1, param);
}

void Character::inc_str(int param)
{
	str_ = MAX(1, str_+param);
}

int Character::get_str_add() const
{
	return str_add_;
}

void Character::set_str_add(int param)
{
	str_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int Character::get_dex() const
{
	check_purged(this, "get_dex");
	return current_morph_->GetDex();
}

int Character::get_inborn_dex() const
{
	return dex_;
}

void Character::set_dex(int param)
{
	dex_ = MAX(1, param);
}

void Character::inc_dex(int param)
{
	dex_ = MAX(1, dex_+param);
}


int Character::get_dex_add() const
{
	return dex_add_;
}

void Character::set_dex_add(int param)
{
	dex_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int Character::get_con() const
{
	check_purged(this, "get_con");
	return current_morph_->GetCon();
}

int Character::get_inborn_con() const
{
	return con_;
}

void Character::set_con(int param)
{
	con_ = MAX(1, param);
}
void Character::inc_con(int param)
{
	con_ = MAX(1, con_+param);
}

int Character::get_con_add() const
{
	return con_add_;
}

void Character::set_con_add(int param)
{
	con_add_ = param;
}
//////////////////////////////////////

int Character::get_int() const
{
	check_purged(this, "get_int");
	return current_morph_->GetIntel();
}

int Character::get_inborn_int() const
{
	return int_;
}

void Character::set_int(int param)
{
	int_ = MAX(1, param);
}

void Character::inc_int(int param)
{
	int_ = MAX(1, int_+param);
}

int Character::get_int_add() const
{
	return int_add_;
}

void Character::set_int_add(int param)
{
	int_add_ = param;
}
////////////////////////////////////////
int Character::get_wis() const
{
	check_purged(this, "get_wis");
	return current_morph_->GetWis();
}

int Character::get_inborn_wis() const
{
	return wis_;
}

void Character::set_wis(int param)
{
	wis_ = MAX(1, param);
}

void Character::set_who_mana(unsigned int param)
{
	player_specials->saved.who_mana = param;
}

void Character::set_who_last(time_t param)
{
	char_specials.who_last = param;
}

unsigned int Character::get_who_mana()
{
	return player_specials->saved.who_mana;
}

time_t Character::get_who_last()
{
	return char_specials.who_last;
}

void Character::inc_wis(int param)
{
	wis_ = MAX(1, wis_ + param);
}


int Character::get_wis_add() const
{
	return wis_add_;
}

void Character::set_wis_add(int param)
{
	wis_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int Character::get_cha() const
{
	check_purged(this, "get_cha");
	return current_morph_->GetCha();
}

int Character::get_inborn_cha() const
{
	return cha_;
}

void Character::set_cha(int param)
{
	cha_ = MAX(1, param);
}
void Character::inc_cha(int param)
{
	cha_ = MAX(1, cha_+param);
}


int Character::get_cha_add() const
{
	return cha_add_;
}

void Character::set_cha_add(int param)
{
	cha_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////

void Character::clear_add_affects()
{
	// Clear all affect, because recalc one
	memset(&add_abils, 0, sizeof(char_played_ability_data));
	set_str_add(0);
	set_dex_add(0);
	set_con_add(0);
	set_int_add(0);
	set_wis_add(0);
	set_cha_add(0);
}
///////////////////////////////////////////////////////////////////////////////
int Character::get_zone_group() const
{
	if (IS_NPC(this) && nr >= 0 && mob_index[nr].zone >= 0)
	{
		return MAX(1, zone_table[mob_index[nr].zone].group);
	}
	return 1;
}

//===================================
//Polud формы и все что с ними связано
//===================================

bool Character::know_morph(string morph_id) const
{
	return std::find(morphs_.begin(), morphs_.end(), morph_id) != morphs_.end();
}

void Character::add_morph(string morph_id)
{
	morphs_.push_back(morph_id);
};

void Character::clear_morphs()
{
	morphs_.clear();
};


std::list<string> Character::get_morphs()
{
	return morphs_;
};

std::string Character::get_title()
{
	if (!this->player_data.title) return string();
	string tmp = string(this->player_data.title);
	size_t pos = tmp.find('/');
	if (pos == string::npos)
		return string();
	tmp = tmp.substr(0, pos);
	pos = tmp.find(';');
	if (pos == string::npos)
		return tmp;
	else
		return tmp.substr(0, pos);

};

std::string Character::get_pretitle()
{
	if (!this->player_data.title) return string();
	string tmp = string(this->player_data.title);
	size_t pos = tmp.find('/');
	if (pos == string::npos)
		return string();
	tmp = tmp.substr(0, pos);
	pos = tmp.find(';');
	if (pos == string::npos)
		return string();
	else
		return tmp.substr(pos + 1, tmp.length() - (pos+1));
};

std::string Character::get_race_name()
{
	return PlayerRace::GetRaceNameByNum(GET_KIN(this),GET_RACE(this),GET_SEX(this));
};

std::string Character::get_morph_desc()
{
	return current_morph_->GetMorphDesc();
};

std::string Character::get_morphed_name()
{
	return current_morph_->GetMorphDesc() + " - " + this->get_name();
};

std::string Character::get_morphed_title()
{
	return current_morph_->GetMorphTitle();
};

std::string Character::only_title_noclan()
{
	std::string result = string(this->get_name());
	std::string title = this->get_title();
	std::string pre_title = this->get_pretitle();

	if (!pre_title.empty())
		result = pre_title + " " + result;

	if (!title.empty() && this->get_level() >= MIN_TITLE_LEV)
		result = result + ", " + title;

	return result;
}

std::string Character::clan_for_title()
{
	std::string result = string();

	bool imm = IS_IMMORTAL(this) || PRF_FLAGGED(this, PRF_CODERINFO);

	if (CLAN(this) && !imm)
		result = result + "(" + GET_CLAN_STATUS(this) + ")";

	return result;
}

std::string Character::only_title()
{
	std::string result = this->clan_for_title();
	if (!result.empty())
		result = this->only_title_noclan() + " " + result;
	else
		result = this->only_title_noclan();

	return result;
}

std::string Character::noclan_title()
{
	std::string race = this->get_race_name();

	std::string result = this->only_title_noclan();

	if (result == string(this->get_name()))
		result = race + " " +result;

	return result;
}

std::string Character::race_or_title()
{
	std::string result = this->clan_for_title();

	if (!result.empty())
		result = this->noclan_title() + " " + result;
	else
		result = this->noclan_title();

	return result;
}

int Character::get_morphs_count() const
{
	return morphs_.size();
};

std::string Character::get_cover_desc()
{
	return current_morph_->CoverDesc();
}

void Character::set_morph(MorphPtr morph)
{
	morph->SetChar(this);
	morph->InitSkills(this->get_skill(SKILL_MORPH));
	morph->InitAbils();
	this->current_morph_ = morph;
//	SET_BIT(AFF_FLAGS(this, AFF_MORPH), AFF_MORPH);
};

void Character::reset_morph()
{
	int value = this->get_trained_skill(SKILL_MORPH);
	send_to_char(str(boost::format(current_morph_->GetMessageToChar()) % "человеком") + "\r\n", this);
	act(str(boost::format(current_morph_->GetMessageToRoom()) % "человеком").c_str(), TRUE, this, 0, 0, TO_ROOM);
	this->current_morph_ = NormalMorph::GetNormalMorph(this);
	this->set_morphed_skill(SKILL_MORPH, (MIN(MAX_EXP_PERCENT + GET_REMORT(this) * 5, value)));
//	REMOVE_BIT(AFF_FLAGS(this, AFF_MORPH), AFF_MORPH);
};

bool Character::is_morphed() const
{
	return current_morph_->Name() != "Обычная" || AFF_FLAGGED(this, AFF_MORPH);
};

void Character::set_normal_morph()
{
	current_morph_ = NormalMorph::GetNormalMorph(this);
}

bool Character::isAffected(long flag) const
{
	return current_morph_->isAffected(flag);
}

std::vector<long> Character::GetMorphAffects()
{
	return current_morph_->GetAffects();
}

//===================================
//-Polud
//===================================

bool Character::get_role(unsigned num) const
{
	bool result = false;
	if (num < role_.size())
	{
		result = role_.test(num);
	}
	else
	{
		log("SYSERROR: num=%u (%s:%d)", num, __FILE__, __LINE__);
	}
	return result;
}

void Character::set_role(unsigned num, bool flag)
{
	if (num < role_.size())
	{
		role_.set(num, flag);
	}
	else
	{
		log("SYSERROR: num=%u (%s:%d)", num, __FILE__, __LINE__);
	}
}

const boost::dynamic_bitset<>& Character::get_role_bits() const
{
	return role_;
}

// добавляет указанного ch чара в список атакующих босса с параметром type
// или обновляет его данные в этом списке
void Character::add_attacker(CHAR_DATA *ch, unsigned type, int num)
{
	if (!IS_NPC(this) || IS_NPC(ch) || !get_role(MOB_ROLE_BOSS))
	{
		return;
	}

	int uid = ch->get_uid();
	if (IS_CHARMICE(ch) && ch->master)
	{
		uid = ch->master->get_uid();
	}

	auto i = attackers_.find(uid);
	if (i != attackers_.end())
	{
		switch(type)
		{
		case ATTACKER_DAMAGE:
			i->second.damage += num;
			break;
		case ATTACKER_ROUNDS:
			i->second.rounds += num;
			break;
		}
	}
	else
	{
		attacker_node tmp_node;
		switch(type)
		{
		case ATTACKER_DAMAGE:
			tmp_node.damage = num;
			break;
		case ATTACKER_ROUNDS:
			tmp_node.rounds = num;
			break;
		}
		attackers_.insert(std::make_pair(uid, tmp_node));
	}
}

// возвращает количественный параметр по флагу type указанного ch чара
// из списка атакующих данного босса
int Character::get_attacker(CHAR_DATA *ch, unsigned type) const
{
	if (!IS_NPC(this) || IS_NPC(ch) || !get_role(MOB_ROLE_BOSS))
	{
		return -1;
	}
	auto i = attackers_.find(ch->get_uid());
	if (i != attackers_.end())
	{
		switch(type)
		{
		case ATTACKER_DAMAGE:
			return i->second.damage;
		case ATTACKER_ROUNDS:
			return i->second.rounds;
		}
	}
	return 0;
}

// поиск в списке атакующих нанесшего максимальный урон, который при этом
// находится в данный момент в этой же комнате с боссом и онлайн
std::pair<int /* uid */, int /* rounds */> Character::get_max_damager_in_room() const
{
	std::pair<int, int> damager (-1, 0);

	if (!IS_NPC(this) || !get_role(MOB_ROLE_BOSS))
	{
		return damager;
	}

	int max_dmg = 0;
	for (CHAR_DATA *i = world[this->in_room]->people;
		i; i = i->next_in_room)
	{
		if (!IS_NPC(i) && i->desc)
		{
			auto it = attackers_.find(i->get_uid());
			if (it != attackers_.end())
			{
				if (it->second.damage > max_dmg)
				{
					max_dmg = it->second.damage;
					damager.first = it->first;
					damager.second = it->second.rounds;
				}
			}
		}
	}

	return damager;
}

// обновление босса вне боя по прошествии MOB_RESTORE_TIMER секунд
void Character::restore_mob()
{
	restore_timer_ = 0;
	attackers_.clear();

	GET_HIT(this) = GET_REAL_MAX_HIT(this);
	GET_MOVE(this) = GET_REAL_MAX_MOVE(this);
	update_pos(this);

	for (int i = 0; i < MAX_SPELLS; ++i)
	{
		GET_SPELL_MEM(this, i) = GET_SPELL_MEM(&mob_proto[GET_MOB_RNUM(this)], i);
	}
	GET_CASTER(this) = GET_CASTER(&mob_proto[GET_MOB_RNUM(this)]);
}

// инкремент и проверка таймера на рестор босса,
// который находится вне боя и до этого был кем-то бит
// (т.к. имеет не нулевой список атакеров)
void Character::inc_restore_timer(int num)
{
	if (get_role(MOB_ROLE_BOSS) && !attackers_.empty() && !get_fighting())
	{
		restore_timer_ += num;
		if (restore_timer_ > num)
		{
			restore_mob();
		}
	}
}

obj_sets::activ_sum& Character::obj_bonus()
{
	return obj_bonus_;
}
