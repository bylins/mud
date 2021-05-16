// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "spells.h"

#include "obj.hpp"
#include "comm.h"
#include "screen.h"
#include "poison.hpp"
#include "chars/character.h"
#include "db.h"
#include "room.hpp"
#include "logger.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "char_obj_utils.inl"
#include "skills_info.h"
#include "spells.info.h"

#include <boost/format.hpp>

#include <sstream>
#include <string>

/*
Система следующая:
хотим что-то сделать при касте на шмотку - пишем в mag_alter_objs()
надо что-то сделать при снятии обкаста - check_spell_remove()
если надо постоянный обкаст - ставим таймер на -1 в timed_spell.add()
надо проверить есть ли каст на шмотке - timed_spell.check_spell(spell_num)
*/

////////////////////////////////////////////////////////////////////////////////
namespace
{

///
/// Удаление временного флага со шмотки obj (с проверкой прототипа).
/// \param flag - ITEM_XXX
///
void remove_tmp_extra(OBJ_DATA *obj, EExtraFlag flag)
{
	auto proto = get_object_prototype(GET_OBJ_VNUM(obj));
	if (!proto->get_extra_flag(flag))
	{
		obj->unset_extraflag(flag);
	}
}

/**
 * Проверка надо ли что-то делать со шмоткой или писать чару
 * при снятии заклинания со шмотки.
 */
void check_spell_remove(OBJ_DATA *obj, int spell, bool send_message)
{
	if (!obj)
	{
		log("SYSERROR: NULL object %s:%d, spell = %d", __FILE__, __LINE__, spell);
		return;
	}

	// если что-то надо сделать со шмоткой при снятии обкаста
	switch (spell)
	{
	case SPELL_ACONITUM_POISON:
	case SPELL_SCOPOLIA_POISON:
	case SPELL_BELENA_POISON:
	case SPELL_DATURA_POISON:
		break;

	case SPELL_FLY:
		remove_tmp_extra(obj, EExtraFlag::ITEM_FLYING);
		break;

	case SPELL_LIGHT:
		remove_tmp_extra(obj, EExtraFlag::ITEM_GLOW);
		break;
	} // switch

	// онлайн уведомление чару
	if (send_message
		&& (obj->get_carried_by()
			|| obj->get_worn_by()))
	{
		CHAR_DATA *ch = obj->get_carried_by() ? obj->get_carried_by() : obj->get_worn_by();
		switch (spell)
		{
		case SPELL_ACONITUM_POISON:
		case SPELL_SCOPOLIA_POISON:
		case SPELL_BELENA_POISON:
		case SPELL_DATURA_POISON:
			send_to_char(ch, "С %s испарились последние капельки яда.\r\n",
				GET_OBJ_PNAME(obj, 1).c_str());
			break;

		case SPELL_FLY:
			send_to_char(ch, "Ваш%s %s перестал%s парить в воздухе.\r\n",
				GET_OBJ_VIS_SUF_7(obj, ch),
				GET_OBJ_PNAME(obj, 0).c_str(),
				GET_OBJ_VIS_SUF_1(obj, ch));
			break;

		case SPELL_LIGHT:
			send_to_char(ch, "Ваш%s %s перестал%s светиться.\r\n",
				GET_OBJ_VIS_SUF_7(obj, ch),
				GET_OBJ_PNAME(obj, 0).c_str(),
				GET_OBJ_VIS_SUF_1(obj, ch));
			break;
		}
	}
}

// * Распечатка строки с заклинанием и таймером при осмотре шмотки.
std::string print_spell_str(CHAR_DATA *ch, int spell, int timer)
{
	if (spell < 1
		|| spell > SPELLS_COUNT)
	{
		log("SYSERROR: %s, spell = %d, time = %d", __func__, spell, timer);
		return "";
	}

	std::string out;
	switch (spell)
	{
	case SPELL_ACONITUM_POISON:
	case SPELL_SCOPOLIA_POISON:
	case SPELL_BELENA_POISON:
	case SPELL_DATURA_POISON:
		out = boost::str(boost::format("%1%Отравлено %2% еще %3% %4%.%5%\r\n")
			% CCGRN(ch, C_NRM)
			% get_poison_by_spell(spell)
			% timer
			% desc_count(timer, WHAT_MINu)
			% CCNRM(ch, C_NRM));
		break;

	default:
		if (timer == -1)
		{
			out = boost::str(boost::format("%1%Наложено постоянное заклинание '%2%'.%3%\r\n")
				% CCCYN(ch, C_NRM)
				% (spell_info[spell].name ? spell_info[spell].name : "<null>")
				% CCNRM(ch, C_NRM));
		}
		else
		{
			out = boost::str(boost::format("%1%Наложено заклинание '%2%' (%3%).%4%\r\n")
				% CCCYN(ch, C_NRM)
				% (spell_info[spell].name ? spell_info[spell].name : "<null>")
				% time_format(timer, true)
				% CCNRM(ch, C_NRM));
		}
		break;
	}
	return out;
}

} // namespace
////////////////////////////////////////////////////////////////////////////////

/**
 * Удаление заклинания со шмотки с проверкой на действия/сообщения
 * при снятии обкаста.
 */
void TimedSpell::del(OBJ_DATA *obj, int spell, bool message)
{
	std::map<int, int>::iterator i = spell_list_.find(spell);
	if (i != spell_list_.end())
	{
		check_spell_remove(obj, spell, message);
		spell_list_.erase(i);
	}
}

/**
* Сет доп.спела с таймером на шмотку.
* \param time = -1 для постоянного обкаста
*/
void TimedSpell::add(OBJ_DATA *obj, int spell, int time)
{
	// замещение ядов друг другом
	if (spell == SPELL_ACONITUM_POISON
		|| spell == SPELL_SCOPOLIA_POISON
		|| spell == SPELL_BELENA_POISON
		|| spell == SPELL_DATURA_POISON)
	{
		del(obj, SPELL_ACONITUM_POISON, false);
		del(obj, SPELL_SCOPOLIA_POISON, false);
		del(obj, SPELL_BELENA_POISON, false);
		del(obj, SPELL_DATURA_POISON, false);
	}

	spell_list_[spell] = time;
}

// * Вывод оставшегося времени яда на пушке при осмотре.
std::string TimedSpell::diag_to_char(CHAR_DATA *ch)
{
	if (spell_list_.empty())
	{
		return "";
	}

	std::string out;
	for(std::map<int, int>::iterator i = spell_list_.begin(),
		iend = spell_list_.end(); i != iend; ++i)
	{
		out += print_spell_str(ch, i->first, i->second);
	}
	return out;
}

/**
 * Проверка на обкаст шмотки любым видом яда.
 * \return -1 если яда нет, spell_num если есть.
 */
int TimedSpell::is_spell_poisoned() const
{
	for(std::map<int, int>::const_iterator i = spell_list_.begin(),
		iend = spell_list_.end(); i != iend; ++i)
	{
		if (check_poison(i->first))
		{
			return i->first;
		}
	}
	return -1;
}

// * Для сейва обкаста.
bool TimedSpell::empty() const
{
	return spell_list_.empty();
}

// * Сохранение строки в файл.
std::string TimedSpell::print() const
{
	std::stringstream out;

	out << "TSpl: ";
	for(std::map<int, int>::const_iterator i = spell_list_.begin(),
		iend = spell_list_.end(); i != iend; ++i)
	{
		out << i->first << " " << i->second << "\n";
	}
	out << "~\n";

	return out.str();
}

// * Поиск заклинания по spell_num.
bool TimedSpell::check_spell(int spell) const
{
	std::map<int, int>::const_iterator i = spell_list_.find(spell);
	if (i != spell_list_.end())
	{
		return true;
	}
	return false;
}

/**
* Тик доп.спеллов на шмотке (раз в минуту).
* \param time по дефолту = 1.
*/
void TimedSpell::dec_timer(OBJ_DATA *obj, int time)
{
	for(std::map<int, int>::iterator i = spell_list_.begin();
		i != spell_list_.end(); /* empty */)
	{
		if (i->second != -1)
		{
			i->second -= time;
			if (i->second <= 0)
			{
				check_spell_remove(obj, i->first, true);
				spell_list_.erase(i++);
			}
			else
			{
				++i;
			}
		}
		else
		{
			++i;
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
