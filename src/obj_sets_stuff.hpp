// Copyright (c) 2014 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_SETS_STUFF_HPP_INCLUDED
#define OBJ_SETS_STUFF_HPP_INCLUDED

#include "conf.h"

#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"
#include "chars/char_player.hpp"
#include "classes/constants.hpp"

#include <array>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace obj_sets
{

/// сообщения активации/деактивации для чара и комнаты
/// могут быть глобальными на все сеты, на один сет, на один предмет
struct msg_node
{
	// сообщения чару при активации/деактивации
	std::string char_on_msg;
	std::string char_off_msg;
	// теже сообщения, но в комнату
	std::string room_on_msg;
	std::string room_off_msg;

	// для сравнения в sedit
	bool operator!=(const msg_node &r) const
	{
		if (char_on_msg != r.char_on_msg
			|| char_off_msg != r.char_off_msg
			|| room_on_msg != r.room_on_msg
			|| room_off_msg != r.room_off_msg)
		{
			return true;
		}
		return false;
	}
	bool operator==(const msg_node &r) const
	{
		return !(*this != r);
	}
};

/// сетовый активатор с аффектами и прочими бонусами
struct activ_node
{
	activ_node() : skill(SKILL_INVALID, 0)
	{
		affects = clear_flags;
		prof.set();
		enchant.first = 0;
		npc = false;
	};
	// активится ли миник на мобах
	bool npc;
	// аффекты (obj_flags.affects)
	FLAG_DATA affects;
	// APPLY_XXX аффекты (affected[MAX_OBJ_AFFECT])
	std::array<obj_affected_type, MAX_OBJ_AFFECT> apply;
	// изменение умения. идет в bonus, но в активаторах юзается это поле
	// а не bonus::skills, которое юзается для справки и складывании на чаре
	std::pair<CObjectPrototype::skills_t::key_type, CObjectPrototype::skills_t::mapped_type> skill;
	// список проф, на которых этот активатор сработает (по дефолту - все)
	std::bitset<NUM_PLAYER_CLASSES> prof;
	// числовые сетовые бонусы
	bonus_type bonus;
	// энчант на шмотку
	std::pair<int, ench_type> enchant;

	// для сравнения в sedit
	bool operator!=(const activ_node &r) const
	{
		if (affects != r.affects
			|| apply != r.apply
			|| skill != r.skill
			|| prof != r.prof
			|| bonus != r.bonus
			|| enchant != r.enchant)
		{
			return true;
		}
		return false;
	}
	bool operator==(const activ_node &r) const
	{
		return !(*this != r);
	}
	bool empty() const
	{
		if (!affects.empty()
			|| skill.first > 0
			|| !bonus.empty()
			|| !enchant.second.empty())
		{
			return false;
		}
		for (auto i = apply.begin(); i != apply.end(); ++i)
		{
			if (i->location > 0)
			{
				return false;
			}
		}
		return true;
	}
};

/// собственно структура сетов
struct set_node
{
	set_node() : enabled(true), uid(uid_cnt++) {};

	// статус сета: вкл/выкл, при лоаде из конфига и после олц идет проверка
	// валидности сета, в которой может быть принудительно просталено выкл
	bool enabled;
	// имя сета - опознание вещей, справка сеты
	std::string name;
	// алиас, подменяющий имя сета в генерации справки для глобал дропа
	std::string alias;
	// сгенеренное имя/первый алиас, который пошел в справку
	// будет видно при опознании сетин
	std::string help;
	// любая доп инфа, видимая только в slist
	std::string comment;
	// внумы предметов, активаций/деактиваций (опционально)
	std::map<int, msg_node> obj_list;
	// first - кол-во предметов для активации
	std::map<unsigned, activ_node> activ_list;
	// сообщения активаций/деактиваций на весь сет (опционально)
	msg_node messages;
	// уид сета для сохранений/обновлений в олц
	int uid;

	// счетчик уидов
	static int uid_cnt;

	// для сравнения в sedit
	bool operator!=(const set_node &r) const
	{
		if (enabled != r.enabled
			|| name != r.name
			|| alias != r.alias
			|| comment != r.comment
			|| obj_list != r.obj_list
			|| activ_list != r.activ_list
			|| messages != r.messages
			|| uid != r.uid)
		{
			return true;
		}
		return false;
	}
	bool operator==(const set_node &r) const
	{
		return !(*this != r);
	}
};

extern std::vector<std::shared_ptr<set_node>> sets_list;
extern msg_node global_msg;
extern const unsigned MIN_ACTIVE_SIZE;
extern const unsigned MAX_ACTIVE_SIZE;
extern const unsigned MAX_OBJ_LIST;

size_t setidx_by_objvnum(int vnum);
size_t setidx_by_uid(int uid);
std::string line_split_str(const std::string &str, const std::string &sep,
	size_t len, size_t base_offset = 0);
void init_obj_index();
bool verify_wear_flag(const CObjectPrototype::shared_ptr&);
void verify_set(set_node &set);
bool is_duplicate(int set_uid, int vnum);
std::string print_total_activ(const set_node &set);

} // namespace obj_sets

#endif // OBJ_SETS_STUFF_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
