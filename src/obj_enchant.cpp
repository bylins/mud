// Copyright (c) 2012 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <sstream>

#include "obj_enchant.hpp"
#include "obj.hpp"
#include "utils.h"
#include "screen.h"
#include "db.h"
#include "char.hpp"
#include "constants.h"
#include "spells.h"

namespace obj
{

enchant::enchant()
	: type_(-1), weight_(0), ndice_(0), sdice_(0)
{
	affects_flags_ = clear_flags;
	extra_flags_ = clear_flags;
	no_flags_ = clear_flags;
}

enchant::enchant(OBJ_DATA *obj)
{
	name_ = GET_OBJ_PNAME(obj, 4) ? GET_OBJ_PNAME(obj, 4) : "<null>";
	type_ = ENCHANT_FROM_OBJ;

	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != APPLY_NONE
			&& obj->affected[i].modifier != 0)
		{
			affected_.push_back(obj->affected[i]);
		}
	}

	affects_flags_ = GET_OBJ_AFFECTS(obj);
	extra_flags_ = obj->obj_flags.extra_flags;
	REMOVE_BIT(GET_FLAG(extra_flags_, ITEM_TICKTIMER), ITEM_TICKTIMER);
	no_flags_ = GET_OBJ_NO(obj);
	weight_ = GET_OBJ_VAL(obj, 0);
	ndice_ = 0;
	sdice_ = 0;
}

void enchant::print(CHAR_DATA *ch) const
{
	send_to_char(ch, "Зачаровано %s :%s\r\n", name_.c_str(), CCCYN(ch, C_NRM));

	for (std::vector<obj_affected_type>::const_iterator i = affected_.begin(),
		iend = affected_.end(); i != iend; ++i)
	{
		print_obj_affects(ch, *i);
	}

	if (sprintbits(affects_flags_, weapon_affects, buf2, ","))
	{
		send_to_char(ch, "%s   аффекты: %s%s\r\n",
			CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	}

	if (sprintbits(extra_flags_, extra_bits, buf2, ","))
	{
		send_to_char(ch, "%s   экстрафлаги: %s%s\r\n",
			CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	}

	if (sprintbits(no_flags_, no_bits, buf2, ","))
	{
		send_to_char(ch, "%s   неудобен: %s%s\r\n",
			CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	}

	if (weight_ != 0)
	{
		send_to_char(ch, "%s   %s вес на %d%s\r\n", CCCYN(ch, C_NRM),
			weight_ > 0 ? "увеличивает" : "уменьшает",
			abs(weight_), CCNRM(ch, C_NRM));
	}

	if (ndice_ != 0 || sdice_ != 0)
	{
		if (ndice_ >= 0 && sdice_ >= 0)
		{
			send_to_char(ch, "%s   увеличивает урон на %dD%d%s\r\n",
				CCCYN(ch, C_NRM), abs(ndice_), abs(sdice_), CCNRM(ch, C_NRM));
		}
		else if (ndice_ <= 0 && sdice_ <= 0)
		{
			send_to_char(ch, "%s   уменьшает урон на %dD%d%s\r\n",
				CCCYN(ch, C_NRM), abs(ndice_),  abs(sdice_), CCNRM(ch, C_NRM));
		}
		else
		{
			send_to_char(ch, "%s   изменяет урон на %+dD%+d%s\r\n",
				CCCYN(ch, C_NRM), ndice_, sdice_, CCNRM(ch, C_NRM));
		}
	}
}

std::string enchant::print_to_file() const
{
	std::stringstream out;
	out << "Ench: I " << name_ << "\n" << " T "<< type_ << "\n";

	for (std::vector<obj_affected_type>::const_iterator i = affected_.begin(),
		iend = affected_.end(); i != iend; ++i)
	{
		out << " A " << i->location << " " << i->modifier << "\n";
	}

	*buf = '\0';
	tascii(&GET_FLAG(affects_flags_, 0), 4, buf);
	out << " F " << buf << "\n";

	*buf = '\0';
	tascii(&GET_FLAG(extra_flags_, 0), 4, buf);
	out << " E " << buf << "\n";

	*buf = '\0';
	tascii(&GET_FLAG(no_flags_, 0), 4, buf);
	out << " N " << buf << "\n";

	out << " W " << weight_ << "\n";
	out << " B " << ndice_ << "\n";
	out << " C " << sdice_ << "\n";
	out << "~\n";

	return out.str();
}

void correct_values(OBJ_DATA *obj)
{
	GET_OBJ_WEIGHT(obj) = std::max(1, GET_OBJ_WEIGHT(obj));
	GET_OBJ_VAL(obj, 1) = std::max(0, GET_OBJ_VAL(obj, 1));
	GET_OBJ_VAL(obj, 2) = std::max(0, GET_OBJ_VAL(obj, 2));
}

void enchant::apply_to_obj(OBJ_DATA *obj) const
{
	for (auto i = affected_.cbegin(), iend = affected_.cend(); i != iend; ++i)
	{
		for (int k = 0; k < MAX_OBJ_AFFECT; k++)
		{
			if (obj->affected[k].location == i->location)
			{
				obj->affected[k].modifier += i->modifier;
				break;
			}
			else if (obj->affected[k].location == APPLY_NONE)
			{
				obj->affected[k].location = i->location;
				obj->affected[k].modifier = i->modifier;
				break;
			}
		}
	}

	GET_OBJ_AFFECTS(obj) += affects_flags_;
	obj->obj_flags.extra_flags += extra_flags_;
	obj->obj_flags.no_flag += no_flags_;

	GET_OBJ_WEIGHT(obj) += weight_;

	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	{
		GET_OBJ_VAL(obj, 1) += ndice_;
		GET_OBJ_VAL(obj, 2) += sdice_;
	}

	correct_values(obj);
	obj->enchants.add(*this);
}

void Enchants::add(const enchant &ench)
{
	list_.push_back(ench);
}

bool Enchants::check(int type) const
{
	for (auto i = list_.cbegin(), iend = list_.cend(); i != iend; ++i)
	{
		if (i->type_ == type)
		{
			return true;
		}
	}
	return false;
}

std::string Enchants::print_to_file() const
{
	std::string out;
	for (auto i = list_.cbegin(), iend = list_.cend(); i != iend; ++i)
	{
		out += i->print_to_file();
	}
	return out;
}

void Enchants::print(CHAR_DATA *ch) const
{
	for (auto i = list_.cbegin(), iend = list_.cend(); i != iend; ++i)
	{
		i->print(ch);
	}
}

bool Enchants::empty() const
{
	return list_.empty();
}

void Enchants::update_set_bonus(OBJ_DATA *obj, const obj_sets::ench_type *set_ench)
{
	for (auto i = list_.begin(); i != list_.end(); ++i)
	{
		if (i->type_ == ENCHANT_FROM_SET)
		{
			if (i->weight_ != set_ench->weight
				|| i->ndice_ != set_ench->ndice
				|| i->sdice_ != set_ench->sdice)
			{
				// вес
				GET_OBJ_WEIGHT(obj) += set_ench->weight - i->weight_;
				// дайсы пушек
				if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
				{
					GET_OBJ_VAL(obj, 1) += set_ench->ndice - i->ndice_;
					GET_OBJ_VAL(obj, 2) += set_ench->sdice - i->sdice_;
				}
				correct_values(obj);
				i->weight_ = set_ench->weight;
				i->ndice_ = set_ench->ndice;
				i->sdice_ = set_ench->sdice;
			}
			return;
		}
	}

	obj::enchant tmp;
	tmp.type_ = obj::ENCHANT_FROM_SET;
	tmp.name_ = "набором предметов";
	tmp.weight_ = set_ench->weight;
	tmp.ndice_ = set_ench->ndice;
	tmp.sdice_ = set_ench->sdice;
	tmp.apply_to_obj(obj);
}

void Enchants::remove_set_bonus(OBJ_DATA *obj)
{
	for (auto i = list_.begin(); i != list_.end(); ++i)
	{
		if (i->type_ == ENCHANT_FROM_SET)
		{
			GET_OBJ_WEIGHT(obj) -= i->weight_;
			GET_OBJ_VAL(obj, 1) -= i->ndice_;
			GET_OBJ_VAL(obj, 2) -= i->sdice_;
			correct_values(obj);
			list_.erase(i);
			return;
		}
	}
}

} // obj
