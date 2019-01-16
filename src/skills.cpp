/*************************************************************************
*   File: skills.cpp                                   Part of Bylins    *
*   Skills functions here                                                *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "skills.h"

#include "object.prototypes.hpp"
#include "obj.hpp"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "screen.h"
#include "dg_scripts.h"
#include "constants.h"
#include "im.h"
#include "features.hpp"
#include "random.hpp"
#include "char.hpp"
#include "room.hpp"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <sstream>

/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
#define DUMMY_KNIGHT 390
#define DUMMY_SHIELD 391
#define DUMMY_WEAPON 392

// Хз откуда :(

extern struct message_list fight_messages[MAX_MESSAGES];

class WeapForAct
{
public:
	enum WeapType
	{
		EWT_UNDEFINED,
		EWT_PROTOTYPE_SHARED_PTR,
		EWT_OBJECT_RAW_PTR,	// Anton Gorev (09/28/2016): We need to get rid of raw pointers in the future
		EWT_STRING
	};

	class WeaponTypeException : public std::exception
	{
	public:
		explicit WeaponTypeException(const char* what) : m_what(what) {}

		virtual const char* what() const throw() { return m_what.c_str(); }

	private:
		std::string m_what;
	};

	WeapForAct() : m_type(EWT_UNDEFINED), m_prototype_raw_ptr(nullptr) {}
	WeapForAct(const WeapForAct& from);
	void set_damage_string(const int damage);
	WeapForAct& operator=(const CObjectPrototype::shared_ptr& prototype_shared_ptr);
	WeapForAct& operator=(OBJ_DATA* prototype_raw_ptr);

	auto type() const { return m_type; }
	const auto& get_prototype_shared_ptr() const;
	auto get_prototype_raw_ptr() const;
	const auto get_object_ptr() const;
	const auto& get_string() const;

private:
	using kick_type_t = std::vector<const char*>;

	WeapForAct& operator=(const WeapForAct&);

	bool check_type(const WeapType type) const { return check_type(type, true); }
	bool check_type(const WeapType type, const bool raise_exception) const;

	WeapType m_type;
	OBJ_DATA::shared_ptr m_prototype_shared_ptr;
	OBJ_DATA* m_prototype_raw_ptr;
	std::string m_string;

	static const kick_type_t s_kick_type;	// Anton Gorev (09/28/2016): As I know, it is a duplicate. We need to reuse kick types from other place.
};

WeapForAct::WeapForAct(const WeapForAct& from) :
	m_type(from.m_type),
	m_prototype_shared_ptr(from.m_prototype_shared_ptr),
	m_prototype_raw_ptr(from.m_prototype_raw_ptr),
	m_string(from.m_string)
{
}

void WeapForAct::set_damage_string(const int damage)
{
	m_type = EWT_STRING;
	if (damage <= 5)
	{
		m_string = s_kick_type[0];
	}
	else if (damage <= 11)
	{
		m_string = s_kick_type[1];
	}
	else if (damage <= 26)
	{
		m_string = s_kick_type[2];
	}
	else if (damage <= 35)
	{
		m_string = s_kick_type[3];
	}
	else if (damage <= 45)
	{
		m_string = s_kick_type[4];
	}
	else if (damage <= 56)
	{
		m_string = s_kick_type[5];
	}
	else if (damage <= 96)
	{
		m_string = s_kick_type[6];
	}
	else if (damage <= 136)
	{
		m_string = s_kick_type[7];
	}
	else if (damage <= 176)
	{
		m_string = s_kick_type[8];
	}
	else if (damage <= 216)
	{
		m_string = s_kick_type[9];
	}
	else if (damage <= 256)
	{
		m_string = s_kick_type[10];
	}
	else if (damage <= 296)
	{
		m_string = s_kick_type[11];
	}
	else
	{
		m_string = s_kick_type[12];
	}
}

const auto& WeapForAct::get_prototype_shared_ptr() const
{
	check_type(EWT_PROTOTYPE_SHARED_PTR);
	return m_prototype_shared_ptr;
}

auto WeapForAct::get_prototype_raw_ptr() const
{
	check_type(EWT_OBJECT_RAW_PTR);
	return m_prototype_raw_ptr;
}

const auto WeapForAct::get_object_ptr() const
{
	if (check_type(EWT_OBJECT_RAW_PTR, false))
	{
		return m_prototype_raw_ptr;
	}
	else if (check_type(EWT_PROTOTYPE_SHARED_PTR, false))
	{
		return m_prototype_shared_ptr.get();
	}

	std::stringstream ss;
	ss << "Requested object ptr but actual weapon type is [" << m_type << "]";
	throw WeaponTypeException(ss.str().c_str());
}

const auto& WeapForAct::get_string() const
{
	check_type(EWT_STRING);
	return m_string;
}

bool WeapForAct::check_type(const WeapType type, const bool raise_exception) const
{
	if (type != m_type)
	{
		if (raise_exception)
		{
			std::stringstream ss;
			ss << "Type of weapon [" << m_type << "] does not match to expected [" << type << "]";
			throw WeaponTypeException(ss.str().c_str());
		}

		return false;
	}

	return true;
}

WeapForAct& WeapForAct::operator=(OBJ_DATA* prototype_raw_ptr)
{
	m_type = EWT_OBJECT_RAW_PTR;
	m_prototype_raw_ptr = prototype_raw_ptr;
	return *this;
}

WeapForAct& WeapForAct::operator=(const CObjectPrototype::shared_ptr& prototype_shared_ptr)
{
	m_type = EWT_PROTOTYPE_SHARED_PTR;
	m_prototype_shared_ptr.reset();
	if (prototype_shared_ptr)
	{
		m_prototype_shared_ptr.reset(new OBJ_DATA(*prototype_shared_ptr));
	}
	return *this;
}

const WeapForAct::kick_type_t WeapForAct::s_kick_type =
// силы пинка. полностью соответствуют наносимым поврждениям обычного удара
{
	"легонько ",		//  1..5
	"слегка ",		// 6..11
	"",			// 12..26
	"сильно ",		// 27..35
	"очень сильно ",	// 36..45
	"чрезвычайно сильно ",	// 46..55
	"БОЛЬНО ",		// 56..96
	"ОЧЕНЬ БОЛЬНО ",	// 97..136
	"ЧРЕЗВЫЧАЙНО БОЛЬНО ",	// 137..176
	"НЕВЫНОСИМО БОЛЬНО ",	// 177..216
	"ЖЕСТОКО ",	// 217..256
	"УЖАСНО ",// 257..296
	"УБИЙСТВЕННО ",	 // 297..400
	"СМЕРТЕЛЬНО " // 400+
};

struct brief_shields
{
	brief_shields(CHAR_DATA* ch_, CHAR_DATA* vict_, const WeapForAct& weap_, std::string add_);

	void act_to_char(const char *msg);
	void act_to_vict(const char *msg);
	void act_to_room(const char *msg);

	void act_with_exception_handling(const char *msg, int type) const;

	CHAR_DATA* ch;
	CHAR_DATA* vict;
	WeapForAct weap;
	std::string add;
	// флаг отражаемого дамага, который надо глушить в режиме PRF_BRIEF_SHIELDS
	bool reflect;

private:
	void act_no_add(const char *msg, int type);
	void act_add(const char *msg, int type);
};

brief_shields::brief_shields(CHAR_DATA* ch_, CHAR_DATA* vict_, const WeapForAct& weap_, std::string add_) : ch(ch_), vict(vict_), weap(weap_), add(add_), reflect(false)
{
}

void brief_shields::act_to_char(const char *msg)
{
	if (!reflect
		|| (reflect
			&& !PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS)))
	{
		if (!add.empty()
			&& PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS))
		{
			act_add(msg, TO_CHAR);
		}
		else
		{
			act_no_add(msg, TO_CHAR);
		}
	}
}

void brief_shields::act_to_vict(const char *msg)
{
	if (!reflect
		|| (reflect
			&& !PRF_FLAGGED(vict, PRF_BRIEF_SHIELDS)))
	{
		if (!add.empty()
			&& PRF_FLAGGED(vict, PRF_BRIEF_SHIELDS))
		{
			act_add(msg, TO_VICT | TO_SLEEP);
		}
		else
		{
			act_no_add(msg, TO_VICT | TO_SLEEP);
		}
	}
}

void brief_shields::act_to_room(const char *msg)
{
	if (add.empty())
	{
		if (reflect)
		{
			act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
		}
		else
		{
			act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN);
		}
	}
	else
	{
		act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
		if (!reflect)
		{
			act_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_BRIEF_SHIELDS);
		}
	}
}

void brief_shields::act_with_exception_handling(const char* msg, const int type) const
{
	try
	{
		const auto weapon_type = weap.type();
		switch (weapon_type)
		{
		case WeapForAct::EWT_STRING:
			act(msg, FALSE, ch, nullptr, vict, type, weap.get_string());
			break;

		case WeapForAct::EWT_OBJECT_RAW_PTR:
		case WeapForAct::EWT_PROTOTYPE_SHARED_PTR:
			act(msg, FALSE, ch, weap.get_object_ptr(), vict, type);
			break;

		default:
			act(msg, FALSE, ch, nullptr, vict, type);
		}
	}
	catch (const WeapForAct::WeaponTypeException& e)
	{
		mudlog(e.what(), BRF, LVL_BUILDER, ERRLOG, TRUE);
	}
}

void brief_shields::act_no_add(const char *msg, int type)
{
	act_with_exception_handling(msg, type);
}

void brief_shields::act_add(const char *msg, int type)
{
	char buf_[MAX_INPUT_LENGTH];
	snprintf(buf_, sizeof(buf_), "%s%s", msg, add.c_str());
	act_with_exception_handling(buf_, type);
}

const WeapForAct init_weap(CHAR_DATA *ch, int dam, int attacktype)
{
	// Нижеследующий код повергает в ужас
	WeapForAct weap;
	int weap_i = 0;

	switch (attacktype)
	{
	case SKILL_BACKSTAB + TYPE_HIT:
		weap = GET_EQ(ch, WEAR_WIELD);
		if (!weap.get_prototype_raw_ptr())
		{
			weap_i = real_object(DUMMY_KNIGHT);
			if (0 <= weap_i)
			{
				weap = obj_proto[weap_i];
			}
		}
		break;

	case SKILL_THROW + TYPE_HIT:
		weap = GET_EQ(ch, WEAR_WIELD);
		if (!weap.get_prototype_raw_ptr())
		{
			weap_i = real_object(DUMMY_KNIGHT);
			if (0 <= weap_i)
			{
				weap = obj_proto[weap_i];
			}
		}
		break;

	case SKILL_BASH + TYPE_HIT:
		weap = GET_EQ(ch, WEAR_SHIELD);
		if (!weap.get_prototype_raw_ptr())
		{
			weap_i = real_object(DUMMY_SHIELD);
			if (0 <= weap_i)
			{
				weap = obj_proto[weap_i];
			}
		}
		break;

	case SKILL_KICK + TYPE_HIT:
		// weap - текст силы удара
		weap.set_damage_string(dam);
		break;

	case TYPE_HIT:
		break;

	default:
		weap_i = real_object(DUMMY_WEAPON);
		if (0 <= weap_i)
		{
			weap = obj_proto[weap_i];
		}
	}

	return weap;
}

typedef std::map<ESkill, std::string> ESkill_name_by_value_t;
typedef std::map<const std::string, ESkill> ESkill_value_by_name_t;
ESkill_name_by_value_t ESkill_name_by_value;
ESkill_value_by_name_t ESkill_value_by_name;

void init_ESkill_ITEM_NAMES()
{
	ESkill_name_by_value.clear();
	ESkill_value_by_name.clear();

	ESkill_name_by_value[ESkill::SKILL_THAC0] = "SKILL_THAC0";
	ESkill_name_by_value[ESkill::SKILL_PROTECT] = "SKILL_PROTECT";
	ESkill_name_by_value[ESkill::SKILL_TOUCH] = "SKILL_TOUCH";
	ESkill_name_by_value[ESkill::SKILL_SHIT] = "SKILL_SHIT";
	ESkill_name_by_value[ESkill::SKILL_MIGHTHIT] = "SKILL_MIGHTHIT";
	ESkill_name_by_value[ESkill::SKILL_STUPOR] = "SKILL_STUPOR";
	ESkill_name_by_value[ESkill::SKILL_POISONED] = "SKILL_POISONED";
	ESkill_name_by_value[ESkill::SKILL_SENSE] = "SKILL_SENSE";
	ESkill_name_by_value[ESkill::SKILL_HORSE] = "SKILL_HORSE";
	ESkill_name_by_value[ESkill::SKILL_HIDETRACK] = "SKILL_HIDETRACK";
	ESkill_name_by_value[ESkill::SKILL_RELIGION] = "SKILL_RELIGION";
	ESkill_name_by_value[ESkill::SKILL_MAKEFOOD] = "SKILL_MAKEFOOD";
	ESkill_name_by_value[ESkill::SKILL_MULTYPARRY] = "SKILL_MULTYPARRY";
	ESkill_name_by_value[ESkill::SKILL_TRANSFORMWEAPON] = "SKILL_TRANSFORMWEAPON";
	ESkill_name_by_value[ESkill::SKILL_LEADERSHIP] = "SKILL_LEADERSHIP";
	ESkill_name_by_value[ESkill::SKILL_PUNCTUAL] = "SKILL_PUNCTUAL";
	ESkill_name_by_value[ESkill::SKILL_AWAKE] = "SKILL_AWAKE";
	ESkill_name_by_value[ESkill::SKILL_IDENTIFY] = "SKILL_IDENTIFY";
	ESkill_name_by_value[ESkill::SKILL_HEARING] = "SKILL_HEARING";
	ESkill_name_by_value[ESkill::SKILL_CREATE_POTION] = "SKILL_CREATE_POTION";
	ESkill_name_by_value[ESkill::SKILL_CREATE_SCROLL] = "SKILL_CREATE_SCROLL";
	ESkill_name_by_value[ESkill::SKILL_CREATE_WAND] = "SKILL_CREATE_WAND";
	ESkill_name_by_value[ESkill::SKILL_LOOK_HIDE] = "SKILL_LOOK_HIDE";
	ESkill_name_by_value[ESkill::SKILL_ARMORED] = "SKILL_ARMORED";
	ESkill_name_by_value[ESkill::SKILL_DRUNKOFF] = "SKILL_DRUNKOFF";
	ESkill_name_by_value[ESkill::SKILL_AID] = "SKILL_AID";
	ESkill_name_by_value[ESkill::SKILL_FIRE] = "SKILL_FIRE";
	ESkill_name_by_value[ESkill::SKILL_CREATEBOW] = "SKILL_CREATEBOW";
	ESkill_name_by_value[ESkill::SKILL_THROW] = "SKILL_THROW";
	ESkill_name_by_value[ESkill::SKILL_BACKSTAB] = "SKILL_BACKSTAB";
	ESkill_name_by_value[ESkill::SKILL_BASH] = "SKILL_BASH";
	ESkill_name_by_value[ESkill::SKILL_HIDE] = "SKILL_HIDE";
	ESkill_name_by_value[ESkill::SKILL_KICK] = "SKILL_KICK";
	ESkill_name_by_value[ESkill::SKILL_PICK_LOCK] = "SKILL_PICK_LOCK";
	ESkill_name_by_value[ESkill::SKILL_PUNCH] = "SKILL_PUNCH";
	ESkill_name_by_value[ESkill::SKILL_RESCUE] = "SKILL_RESCUE";
	ESkill_name_by_value[ESkill::SKILL_SNEAK] = "SKILL_SNEAK";
	ESkill_name_by_value[ESkill::SKILL_STEAL] = "SKILL_STEAL";
	ESkill_name_by_value[ESkill::SKILL_TRACK] = "SKILL_TRACK";
	ESkill_name_by_value[ESkill::SKILL_CLUBS] = "SKILL_CLUBS";
	ESkill_name_by_value[ESkill::SKILL_AXES] = "SKILL_AXES";
	ESkill_name_by_value[ESkill::SKILL_LONGS] = "SKILL_LONGS";
	ESkill_name_by_value[ESkill::SKILL_SHORTS] = "SKILL_SHORTS";
	ESkill_name_by_value[ESkill::SKILL_NONSTANDART] = "SKILL_NONSTANDART";
	ESkill_name_by_value[ESkill::SKILL_BOTHHANDS] = "SKILL_BOTHHANDS";
	ESkill_name_by_value[ESkill::SKILL_PICK] = "SKILL_PICK";
	ESkill_name_by_value[ESkill::SKILL_SPADES] = "SKILL_SPADES";
	ESkill_name_by_value[ESkill::SKILL_SATTACK] = "SKILL_SATTACK";
	ESkill_name_by_value[ESkill::SKILL_DISARM] = "SKILL_DISARM";
	ESkill_name_by_value[ESkill::SKILL_PARRY] = "SKILL_PARRY";
	ESkill_name_by_value[ESkill::SKILL_HEAL] = "SKILL_HEAL";
	ESkill_name_by_value[ESkill::SKILL_MORPH] = "SKILL_MORPH";
	ESkill_name_by_value[ESkill::SKILL_BOWS] = "SKILL_BOWS";
	ESkill_name_by_value[ESkill::SKILL_ADDSHOT] = "SKILL_ADDSHOT";
	ESkill_name_by_value[ESkill::SKILL_CAMOUFLAGE] = "SKILL_CAMOUFLAGE";
	ESkill_name_by_value[ESkill::SKILL_DEVIATE] = "SKILL_DEVIATE";
	ESkill_name_by_value[ESkill::SKILL_BLOCK] = "SKILL_BLOCK";
	ESkill_name_by_value[ESkill::SKILL_LOOKING] = "SKILL_LOOKING";
	ESkill_name_by_value[ESkill::SKILL_CHOPOFF] = "SKILL_CHOPOFF";
	ESkill_name_by_value[ESkill::SKILL_REPAIR] = "SKILL_REPAIR";
	ESkill_name_by_value[ESkill::SKILL_UPGRADE] = "SKILL_UPGRADE";
	ESkill_name_by_value[ESkill::SKILL_COURAGE] = "SKILL_COURAGE";
	ESkill_name_by_value[ESkill::SKILL_MANADRAIN] = "SKILL_MANADRAIN";
	ESkill_name_by_value[ESkill::SKILL_NOPARRYHIT] = "SKILL_NOPARRYHIT";
	ESkill_name_by_value[ESkill::SKILL_TOWNPORTAL] = "SKILL_TOWNPORTAL";
	ESkill_name_by_value[ESkill::SKILL_MAKE_STAFF] = "SKILL_MAKE_STAFF";
	ESkill_name_by_value[ESkill::SKILL_MAKE_BOW] = "SKILL_MAKE_BOW";
	ESkill_name_by_value[ESkill::SKILL_MAKE_WEAPON] = "SKILL_MAKE_WEAPON";
	ESkill_name_by_value[ESkill::SKILL_MAKE_ARMOR] = "SKILL_MAKE_ARMOR";
	ESkill_name_by_value[ESkill::SKILL_MAKE_JEWEL] = "SKILL_MAKE_JEWEL";
	ESkill_name_by_value[ESkill::SKILL_MAKE_WEAR] = "SKILL_MAKE_WEAR";
	ESkill_name_by_value[ESkill::SKILL_MAKE_POTION] = "SKILL_MAKE_POTION";
	ESkill_name_by_value[ESkill::SKILL_DIG] = "SKILL_DIG";
	ESkill_name_by_value[ESkill::SKILL_INSERTGEM] = "SKILL_INSERTGEM";
	ESkill_name_by_value[ESkill::SKILL_WARCRY] = "SKILL_WARCRY";
	ESkill_name_by_value[ESkill::SKILL_TURN_UNDEAD] = "SKILL_TURN_UNDEAD";
	ESkill_name_by_value[ESkill::SKILL_IRON_WIND] = "SKILL_IRON_WIND";
	ESkill_name_by_value[ESkill::SKILL_STRANGLE] = "SKILL_STRANGLE";
	ESkill_name_by_value[ESkill::SKILL_AIR_MAGIC] = "SKILL_AIR_MAGIC";
	ESkill_name_by_value[ESkill::SKILL_FIRE_MAGIC] = "SKILL_FIRE_MAGIC";
	ESkill_name_by_value[ESkill::SKILL_WATER_MAGIC] = "SKILL_WATER_MAGIC";
	ESkill_name_by_value[ESkill::SKILL_EARTH_MAGIC] = "SKILL_EARTH_MAGIC";
	ESkill_name_by_value[ESkill::SKILL_LIGHT_MAGIC] = "SKILL_LIGHT_MAGIC";
	ESkill_name_by_value[ESkill::SKILL_DARK_MAGIC] = "SKILL_DARK_MAGIC";
	ESkill_name_by_value[ESkill::SKILL_MIND_MAGIC] = "SKILL_MIND_MAGIC";
	ESkill_name_by_value[ESkill::SKILL_LIFE_MAGIC] = "SKILL_LIFE_MAGIC";
	ESkill_name_by_value[ESkill::SKILL_MAKE_AMULET] = "SKILL_MAKE_AMULET";

	for (const auto& i : ESkill_name_by_value)
	{
		ESkill_value_by_name[i.second] = i.first;
	}
}

template <>
ESkill ITEM_BY_NAME(const std::string& name)
{
	if (ESkill_name_by_value.empty())
	{
		init_ESkill_ITEM_NAMES();
	}
	return ESkill_value_by_name.at(name);
}

template <>
const std::string& NAME_BY_ITEM<ESkill>(const ESkill item)
{
	if (ESkill_name_by_value.empty())
	{
		init_ESkill_ITEM_NAMES();
	}
	return ESkill_name_by_value.at(item);
}

std::array<ESkill, MAX_SKILL_NUM - SKILL_FIRST> AVAILABLE_SKILLS =
{
	SKILL_PROTECT,
	SKILL_TOUCH,
	SKILL_SHIT,
	SKILL_MIGHTHIT,
	SKILL_STUPOR,
	SKILL_POISONED,
	SKILL_SENSE,
	SKILL_HORSE,
	SKILL_HIDETRACK,
	SKILL_RELIGION,
	SKILL_MAKEFOOD,
	SKILL_MULTYPARRY,
	SKILL_TRANSFORMWEAPON,
	SKILL_LEADERSHIP,
	SKILL_PUNCTUAL,
	SKILL_AWAKE,
	SKILL_IDENTIFY,
	SKILL_HEARING,
	SKILL_CREATE_POTION,
	SKILL_CREATE_SCROLL,
	SKILL_CREATE_WAND,
	SKILL_LOOK_HIDE,
	SKILL_ARMORED,
	SKILL_DRUNKOFF,
	SKILL_AID,
	SKILL_FIRE,
	SKILL_CREATEBOW,
	SKILL_THROW,
	SKILL_BACKSTAB,
	SKILL_BASH,
	SKILL_HIDE,
	SKILL_KICK,
	SKILL_PICK_LOCK,
	SKILL_PUNCH,
	SKILL_RESCUE,
	SKILL_SNEAK,
	SKILL_STEAL,
	SKILL_TRACK,
	SKILL_CLUBS,
	SKILL_AXES,
	SKILL_LONGS,
	SKILL_SHORTS,
	SKILL_NONSTANDART,
	SKILL_BOTHHANDS,
	SKILL_PICK,
	SKILL_SPADES,
	SKILL_SATTACK,
	SKILL_DISARM,
	SKILL_PARRY,
	SKILL_HEAL,
	SKILL_MORPH,
	SKILL_BOWS,
	SKILL_ADDSHOT,
	SKILL_CAMOUFLAGE,
	SKILL_DEVIATE,
	SKILL_BLOCK,
	SKILL_LOOKING,
	SKILL_CHOPOFF,
	SKILL_REPAIR,
	SKILL_UPGRADE,
	SKILL_COURAGE,
	SKILL_MANADRAIN,
	SKILL_NOPARRYHIT,
	SKILL_TOWNPORTAL,
	SKILL_MAKE_STAFF,
	SKILL_MAKE_BOW,
	SKILL_MAKE_WEAPON,
	SKILL_MAKE_ARMOR,
	SKILL_MAKE_JEWEL,
	SKILL_MAKE_WEAR,
	SKILL_MAKE_POTION,
	SKILL_DIG,
	SKILL_INSERTGEM,
	SKILL_WARCRY,
	SKILL_TURN_UNDEAD,
	SKILL_IRON_WIND,
	SKILL_STRANGLE,
	SKILL_AIR_MAGIC,
	SKILL_FIRE_MAGIC,
	SKILL_WATER_MAGIC,
	SKILL_EARTH_MAGIC,
	SKILL_LIGHT_MAGIC,
	SKILL_DARK_MAGIC,
	SKILL_MIND_MAGIC,
	SKILL_LIFE_MAGIC,
	SKILL_STUN,
	SKILL_MAKE_AMULET
};

///
/// \param add = "", строка для добавления после основного сообщения (краткий режим щитов)
///
int skill_message(int dam, CHAR_DATA * ch, CHAR_DATA * vict, int attacktype, std::string add)
{
	int i, j, nr;
	struct message_type *msg;

	// log("[SKILL MESSAGE] Message for skill %d",attacktype);
	for (i = 0; i < MAX_MESSAGES; i++)
	{
		if (fight_messages[i].a_type == attacktype)
		{
			nr = dice(1, fight_messages[i].number_of_attacks);
			// log("[SKILL MESSAGE] %d(%d)",fight_messages[i].number_of_attacks,nr);
			for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
			{
				msg = msg->next;
			}

			const auto weap = init_weap(ch, dam, attacktype);
			brief_shields brief(ch, vict, weap, add);
			if (attacktype == SPELL_FIRE_SHIELD
				|| attacktype == SPELL_MAGICGLASS)
			{
				brief.reflect = true;
			}

			if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT) && !PLR_FLAGGED((ch), PLR_WRITING))
			{
				switch (attacktype)
				{
				case SKILL_BACKSTAB + TYPE_HIT:
				case SKILL_THROW + TYPE_HIT:
				case SKILL_BASH + TYPE_HIT:
				case SKILL_KICK + TYPE_HIT:
					send_to_char("&W&q", ch);
					break;

				default:
					send_to_char("&y&q", ch);
					break;
				}
				// ch
				brief.act_to_char(msg->god_msg.attacker_msg);
				send_to_char("&Q&n", ch);
				// victim
				brief.act_to_vict(msg->god_msg.victim_msg);
				// room
				brief.act_to_room(msg->god_msg.room_msg);
			}
			else if (dam != 0)
			{
				if (GET_POS(vict) == POS_DEAD)
				{
					// ch
					send_to_char("&Y&q", ch);
					brief.act_to_char(msg->die_msg.attacker_msg);
					send_to_char("&Q&n", ch);
					// vict
					send_to_char("&R&q", vict);
					brief.act_to_vict(msg->die_msg.victim_msg);
					send_to_char("&Q&n", vict);
					// room
					brief.act_to_room(msg->die_msg.room_msg);
				}
				else
				{
					// ch
					send_to_char("&Y&q", ch);
					brief.act_to_char(msg->hit_msg.attacker_msg);
					send_to_char("&Q&n", ch);
					// vict
					send_to_char("&R&q", vict);
					brief.act_to_vict(msg->hit_msg.victim_msg);
					send_to_char("&Q&n", vict);
					// room
					brief.act_to_room(msg->hit_msg.room_msg);
				}
			}
			else if (ch != vict)  	// Dam == 0
			{
				switch (attacktype)
				{
				case SKILL_BACKSTAB + TYPE_HIT:
				case SKILL_THROW + TYPE_HIT:
				case SKILL_BASH + TYPE_HIT:
				case SKILL_KICK + TYPE_HIT:
					send_to_char("&W&q", ch);
					break;

				default:
					send_to_char("&y&q", ch);
					break;
				}
				//ch
				brief.act_to_char(msg->miss_msg.attacker_msg);
				send_to_char("&Q&n", ch);
				// vict
				send_to_char("&r&q", vict);
				brief.act_to_vict(msg->miss_msg.victim_msg);
				send_to_char("&Q&n", vict);
				// room
				brief.act_to_room(msg->miss_msg.room_msg);
			}
			return (1);
		}
	}
	return (0);
}

// *** This function return chance of skill
int calculate_skill(CHAR_DATA * ch, const ESkill skill_no, CHAR_DATA * vict)
{
	//Зачем здесь было SAVING_REFLEX? Оно же просто число
	int skill_is, percent = 0, victim_sav = 0, victim_modi = 0; // текущее значение умения(прокачанность) / вычисляемый итоговый процент / савис жертвы,
																	// который влияет на прохождение скила / другие модификаторы, влияющие на прохождение
	int morale = 0, max_percent = 200, bonus = 0, size = 0, fail_limit = 950;  // удача пациента, максимально возможный процент скила, бонус от дополнительных параметров.
	bool pass_mod = 0; // в данный момент для доп.выстрела, чтобы оставить его как скилл,
	// но не применять к нему левых штрафов и плюсов, плюсуется только от инты немного

	if (skill_no < SKILL_FIRST
		|| skill_no > MAX_SKILL_NUM)  	// log("ERROR: ATTEMPT USING UNKNOWN SKILL <%d>", skill_no);
	{
		return 0;
	}

	if ((skill_is = ch->get_skill(skill_no)) <= 0)
	{
		return 0;                                         // если скила нет возвращаем 0.
	}

	if (!IS_NPC(ch) && !ch->affected.empty())
	{
		for (const auto& aff : ch->affected)
		{
			if (aff->location == APPLY_BONUS_SKILLS) // скушал свиток с эксп умелкой
			{
				skill_is += aff->modifier;
			}

			if (aff->location == APPLY_PLAQUE)
			{
				skill_is -= number(ch->get_skill(skill_no) * 0.4, ch->get_skill(skill_no) * 0.05); // при лихорадке умелки ухудшаются на 5-30% рандом
			}
		}
	}

	skill_is += int_app[GET_REAL_INT(ch)].to_skilluse;
	if (!IS_NPC(ch))
	{
		size = (MAX(0, GET_SIZE(ch) - 50));
	}
	else
	{
		size = size_app[GET_POS_SIZE(ch)].interpolate / 2;
	}

	switch (skill_no)
	{
	case SKILL_HIDETRACK:          // замести следы
		{
		bonus = (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);
		break;
		}

	case SKILL_BACKSTAB:	//заколоть
		{
		//victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		victim_sav = -GET_REAL_SAVING_REFLEX(vict);
		bonus = dex_bonus(GET_REAL_DEX(ch)) * 2;
		if (awake_others(ch)
			|| equip_in_metall(ch))
		{
			bonus -= 50;
		}

		if (vict)
		{
			if (!CAN_SEE(vict, ch))
			{
				bonus += 25;
			}

			if (GET_POS(vict) < POS_FIGHTING)
			{
				bonus += (20 * (POS_FIGHTING - GET_POS(vict)));
			}
			else if (AFF_FLAGGED(vict, EAffectFlag::AFF_AWARNESS))
			{
				victim_modi -= 30;
			}
			victim_modi += size_app[GET_POS_SIZE(vict)].ac;
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
		}
		break;
		}

	case SKILL_BASH:	//сбить
		{
		victim_sav = -GET_REAL_SAVING_REFLEX(vict);
		bonus = size
			+ dex_bonus(GET_REAL_DEX(ch))
			+ (GET_EQ(ch, WEAR_SHIELD)
				? weapon_app[MIN(35, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD))))].bashing
				: 0);
		if (vict)
		{
			if (GET_POS(vict) < POS_FIGHTING && GET_POS(vict) > POS_SLEEPING)
			{
				victim_modi -= 20;
			}
			if (PRF_FLAGGED(vict, PRF_AWAKE))
			{
				victim_modi -= calculate_awake_mod(ch, vict);
			}
		}
		break;
		}

	case SKILL_HIDE:	//спрятаться
		{
		bonus = dex_bonus(GET_REAL_DEX(ch))
			- size_app[GET_POS_SIZE(ch)].ac
			+ (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);

		if (awake_others(ch) || equip_in_metall(ch))
		{
			bonus -= 50;
		}

		if (IS_DARK(ch->in_room))
		{
			bonus += 25;
		}

		if (SECT(ch->in_room) == SECT_INSIDE)
		{
			bonus += 20;
		}
		else if (SECT(ch->in_room) == SECT_CITY)
		{
			bonus -= 15;
		}
		else if (SECT(ch->in_room) == SECT_FOREST)
		{
			bonus += 20;
		}
		else if (SECT(ch->in_room) == SECT_HILLS
			|| SECT(ch->in_room) == SECT_MOUNTAIN)
		{
			bonus += 10;
		}

		if (vict)
		{
			if (AWAKE(vict))
			{
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
			}
		}
		break;
		}

	case SKILL_KICK:	//пнуть
		{
		//victim_sav = GET_SAVE(vict, SAVING_STABILITY) - dex_bonus(GET_REAL_CON(vict));
		victim_sav = -GET_REAL_SAVING_STABILITY(vict);
		bonus = dex_bonus(GET_REAL_DEX(ch)) + dex_bonus(GET_REAL_STR(ch));
		if (vict)
		{
			victim_modi += size_app[GET_POS_SIZE(vict)].interpolate;
			victim_modi -= GET_REAL_CON(vict);
			if (PRF_FLAGGED(vict, PRF_AWAKE))
			{
				victim_modi -= calculate_awake_mod(ch, vict);
			}
		}
		break;
		}

	case SKILL_PICK_LOCK:	// взлом
		{
		bonus = dex_bonus(GET_REAL_DEX(ch))
			+ (can_use_feat(ch, NIMBLE_FINGERS_FEAT) ? 5 : 0);
		break;
		}

	case SKILL_PUNCH:	//удар левой рукой
		{
		//victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		victim_sav = -GET_REAL_SAVING_REFLEX(vict);
		break;
		}

	case SKILL_RESCUE:	//спасти
		{
		bonus = dex_bonus(GET_REAL_DEX(ch));
		//		victim_modi = 100;
		break;
		}
		
	case SKILL_SNEAK:	// Подкрасться
		{
		bonus = dex_bonus(GET_REAL_DEX(ch))
			+ (can_use_feat(ch, STEALTHY_FEAT) ? 10 : 0);

		if (awake_others(ch) || equip_in_metall(ch))
			bonus -= 50;

		if (SECT(ch->in_room) == SECT_CITY)
			bonus -= 10;
		if (IS_DARK(ch->in_room))
			bonus += 20;

		if (vict)
		{
			if (GET_LEVEL(vict) > 35)
				bonus -= 50;
			if (!CAN_SEE(vict, ch))
				bonus += 25;
			if (AWAKE(vict))
			{
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
			}
		}
		break;
		}

	case SKILL_STEAL:	// Украсть
		{
		bonus = dex_bonus(GET_REAL_DEX(ch))
			+ (can_use_feat(ch, NIMBLE_FINGERS_FEAT) ? 5 : 0);

		if (awake_others(ch) || equip_in_metall(ch))
			bonus -= 50;
		/* работает только для блестящих предметов, так как в темное татям и купцам другое не видно*/
		if (IS_DARK(ch->in_room))
			bonus += 20;

		if (vict)
		{
			//victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
			victim_sav = -GET_REAL_SAVING_REFLEX(vict);
			if (!CAN_SEE(vict, ch))
				bonus += 25;
			if (AWAKE(vict))
			{
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
				if (AFF_FLAGGED(vict, EAffectFlag::AFF_AWARNESS))
					bonus -= 30;
			}
		}
		break;
		}

	case SKILL_TRACK:       //выследить
		{
		percent = skill_is + int_app[GET_REAL_INT(ch)].observation
			+ (can_use_feat(ch, TRACKER_FEAT) ? 10 : 0);

		if (SECT(ch->in_room) == SECT_FOREST || SECT(ch->in_room) == SECT_FIELD)
			percent += 10;

		percent = complex_skill_modifier(ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS, percent);

		if (SECT(ch->in_room) == SECT_WATER_SWIM ||
			SECT(ch->in_room) == SECT_WATER_NOSWIM ||
			SECT(ch->in_room) == SECT_FLYING ||
			SECT(ch->in_room) == SECT_UNDERWATER ||
			SECT(ch->in_room) == SECT_SECRET
			|| ROOM_FLAGGED(ch->in_room, ROOM_NOTRACK)) percent = 0;


		if (vict)
		{
			victim_modi += GET_REAL_CON(vict) / 2;
			if (AFF_FLAGGED(vict, EAffectFlag::AFF_NOTRACK)
				|| ROOM_FLAGGED(ch->in_room, ROOM_NOTRACK))
			{
				victim_modi = -100;
			}
		}
		break;
		}

	case SKILL_SENSE: // найти
		{
		bonus = int_app[GET_REAL_INT(ch)].observation
			+ (can_use_feat(ch, TRACKER_FEAT) ? 20 : 0);
		break;
		}

	case SKILL_MULTYPARRY:  // веерная защита
	case SKILL_PARRY:	//парировать
		{
		victim_sav = dex_bonus(GET_REAL_DEX(vict));
		bonus = dex_bonus(GET_REAL_DEX(ch));
		if (GET_AF_BATTLE(ch, EAF_AWAKE))
		{
			bonus += ch->get_skill(SKILL_AWAKE);
		}

		if (GET_EQ(ch, WEAR_HOLD)
			&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON)
		{
			bonus += weapon_app[MAX(0, MIN(50, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))))].parrying;
		}
		victim_modi = 100;
		break;
		}

	case SKILL_BLOCK:	//закрыться щитом
	{		// по 10 бонусом со щита (21-30) и дексы (21-50)
		int shield_mod = GET_EQ(ch, WEAR_SHIELD) ? MIN(10, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD)) - 20)) : 0;
		int dex_mod = MAX(0, (GET_REAL_DEX(ch) - 20) / 3);
		bonus = dex_mod + shield_mod;
		break;
	}

	case SKILL_TOUCH:	//захватить противника
		{
		victim_sav = dex_bonus(GET_REAL_DEX(vict));
		bonus = dex_bonus(GET_REAL_DEX(ch)) +
			size_app[GET_POS_SIZE(vict)].interpolate;

		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
			victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
		}
		break;
		}

	case SKILL_PROTECT:	//прикрыть грудью
		{
		bonus = dex_bonus(GET_REAL_DEX(ch)) + size;
		victim_modi = 50;
		break;
		}

	case SKILL_BOWS:	//луки
		{
		bonus = dex_bonus(GET_REAL_DEX(ch));
		break;
		}

	case SKILL_BOTHHANDS:	//двуручники
	case SKILL_LONGS:	//длинные лезвия
	case SKILL_SPADES:	//копья и пики
	case SKILL_SHORTS:	//короткие лезвия
	case SKILL_CLUBS:	//палицы и дубины
	case SKILL_PICK:	//проникающее
	case SKILL_NONSTANDART:	//разнообразное оружие
	case SKILL_AXES:	//секиры
		//victim_sav = GET_SAVE(vict, SAVING_REFLEX);
		break;

	case SKILL_SATTACK:	//атака второй рукой
		{
		//victim_sav = GET_SAVE(vict, SAVING_REFLEX);
		victim_sav = -GET_REAL_SAVING_REFLEX(vict) + dex_bonus(GET_REAL_DEX(ch)); //equal
		break;
		}

	case SKILL_LOOKING:	//приглядеться
		{
		bonus = int_app[GET_REAL_INT(ch)].observation;
		break;
		}

	case SKILL_HEARING:	//прислушаться
		{
		bonus = int_app[GET_REAL_INT(ch)].observation;
		break;
		}

	case SKILL_DISARM:
		{
		//victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		victim_sav = -GET_REAL_SAVING_REFLEX(vict);
		bonus = dex_bonus(GET_REAL_DEX(ch)) + dex_bonus(GET_REAL_STR(ch));
		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_STR(ch));
			if (GET_EQ(vict, WEAR_BOTHS))
				victim_modi -= 10;
			if (PRF_FLAGGED(vict, PRF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
		}
		break;
		}

		//насколько я понимаю не используемое умение
	case SKILL_HEAL:   // лечить
		break;

	case SKILL_ADDSHOT:   // дополнительный выстрел
		{
		if (equip_in_metall(ch))
			bonus -= 5;
		pass_mod = 1;
		break;
		}

	case SKILL_NOPARRYHIT:
		{
		bonus = dex_bonus(GET_REAL_DEX(ch));
		break;
		}

	case SKILL_CAMOUFLAGE: // маскировка
		{
		bonus = dex_bonus(GET_REAL_DEX(ch)) - size_app[GET_POS_SIZE(ch)].ac
			+ (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);

		if (awake_others(ch))
			bonus -= 100;

		if (IS_DARK(ch->in_room))
			bonus += 15;

		if (SECT(ch->in_room) == SECT_CITY)
			bonus -= 15;
		else if (SECT(ch->in_room) == SECT_FOREST)
			bonus += 10;
		else if (SECT(ch->in_room) == SECT_HILLS
			|| SECT(ch->in_room) == SECT_MOUNTAIN) bonus += 5;
		if (equip_in_metall(ch))
			bonus -= 30;

		if (vict)
		{
			if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
		}

	case SKILL_DEVIATE:  // уклониться
		{
		bonus = -size_app[GET_POS_SIZE(ch)].ac +
			dex_bonus(GET_REAL_DEX(ch));

		if (equip_in_metall(ch))
			bonus -= 40;

		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
		}
		break;
		}

	case SKILL_CHOPOFF:  // подножка
		{
		//victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		victim_sav = -GET_REAL_SAVING_REFLEX(vict);
		bonus = dex_bonus(GET_REAL_DEX(ch)) + ((dex_bonus(GET_REAL_DEX(ch)) * 5) / 10)  + size_app[GET_POS_SIZE(ch)].ac; // тест х3 признан вредительским
		if (equip_in_metall(ch))
			bonus -= 10;
		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				bonus += 10;
			if (GET_POS(vict) < POS_SITTING)
				bonus -= 50;
			if (AWAKE(vict) || AFF_FLAGGED(vict, EAffectFlag::AFF_AWARNESS) || MOB_FLAGGED(vict, MOB_AWAKE))
				victim_modi -= 20;
			if (PRF_FLAGGED(vict, PRF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
			// в пинке режем дамаг
			//			if (GET_AF_BATTLE(vict, EAF_AWAKE))
			//				victim_modi -= calculate_awake_mod(ch, vict);
			//			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
		}

	case SKILL_REPAIR: // починка
		break;
	case SKILL_UPGRADE:  //заточить
	case SKILL_WARCRY:
		break;
	case SKILL_COURAGE:
		break;
	case SKILL_SHIT:
		break;
	case SKILL_MIGHTHIT: // богатырский молот
		{
		//victim_sav = GET_SAVE(vict, SAVING_STABILITY) - dex_bonus(GET_REAL_CON(vict));
		victim_sav = -GET_REAL_SAVING_STABILITY(vict);
		//bonus = size + dex_bonus(GET_REAL_STR(ch));
		bonus = size + dex_bonus(GET_REAL_STR(ch));

		if (IS_NPC(vict))
			victim_modi -= (size_app[GET_POS_SIZE(vict)].shocking) / 2;
		else
			victim_modi -= size_app[GET_POS_SIZE(vict)].shocking;
		break;
		}

	case SKILL_STUPOR:  // оглушить
		{
		//victim_sav = GET_SAVE(vict, SAVING_STABILITY) - dex_bonus(GET_REAL_CON(vict));
		victim_sav = -GET_REAL_SAVING_STABILITY(vict);
		bonus = dex_bonus(GET_REAL_STR(ch));
		if (GET_EQ(ch, WEAR_WIELD))
			bonus +=
			weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))].shocking;
		else if (GET_EQ(ch, WEAR_BOTHS))
			bonus +=
			weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))].shocking;

		if (vict)
		{
			victim_modi -= GET_REAL_CON(vict);
		}
		break;
		}

	case SKILL_POISONED:  // отравление
		break;
	case SKILL_LEADERSHIP: //лидерство
		{
		bonus = cha_app[GET_REAL_CHA(ch)].leadership;
		break;
		}

	case SKILL_PUNCTUAL:  // точный стиль
		{
		//victim_sav = GET_SAVE(vict, SAVING_CRITICAL) - dex_bonus(GET_REAL_CON(vict));
		victim_sav = -GET_REAL_SAVING_CRITICAL(vict);
		bonus = dex_bonus(GET_REAL_INT(ch));
		if (GET_EQ(ch, WEAR_WIELD))
			bonus += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 18
			+ MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 25
			+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 30;
		if (GET_EQ(ch, WEAR_HOLD))
			bonus += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 18
			+ MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 25
			+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 30;
		if (GET_EQ(ch, WEAR_BOTHS))
			bonus += MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))) - 25
			+ MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))) - 30;
		if (vict)
		{
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
		}

	case SKILL_AWAKE:  // осторожный стиль
	{
		const size_t real_dex = static_cast<size_t>(GET_REAL_DEX(ch));
		if (real_dex < INT_APP_SIZE)
		{
			bonus = int_app[real_dex].observation;

			if (vict)
			{
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
			}
		}
		else
		{
			log("SYSERR: Global buffer overflow for SKILL_AWAKE. Requested real_dex is %zd, but maximal allowed is %zd.",
				real_dex, INT_APP_SIZE);
		}
	}
	break;

	case SKILL_IDENTIFY:
		{
		bonus = int_app[GET_REAL_INT(ch)].observation
			+ (can_use_feat(ch, CONNOISEUR_FEAT) ? 20 : 0);
		break;
		}

	case SKILL_CREATE_POTION:
	case SKILL_CREATE_SCROLL:
	case SKILL_CREATE_WAND:
		break;

	case SKILL_LOOK_HIDE:
		{
		bonus = cha_app[GET_REAL_CHA(ch)].illusive;
		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				bonus += 50;
			else if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(ch)].observation;
		}
		break;
		}

	case SKILL_ARMORED:
		break;

	case SKILL_DRUNKOFF:
		{
		bonus = -GET_REAL_CON(ch) / 2
			+ (can_use_feat(ch, DRUNKARD_FEAT) ? 20 : 0);
		break;
		}

	case SKILL_AID:    // лечить
		{
		bonus = (can_use_feat(ch, HEALER_FEAT) ? 10 : 0);
		break;
		}

	case SKILL_FIRE:
		{
		if (get_room_sky(ch->in_room) == SKY_RAINING)
			bonus -= 50;
		else if (get_room_sky(ch->in_room) != SKY_LIGHTNING)
			bonus -= number(10, 25);
		break;
		}

	case SKILL_HORSE: // верховая езда
		{
		bonus = cha_app[GET_REAL_CHA(ch)].leadership;
		break;
		}

	case SKILL_TURN_UNDEAD:  // изгнать нежить
		{
		bonus = (can_use_feat(ch, EXORCIST_FEAT) ? 20 : 0);
		break;
		}
		
	case SKILL_MORPH:
		break;
	case SKILL_STRANGLE: // удавить
		{
		//victim_sav = GET_SAVE(vict, SAVING_REFLEX) -dex_bonus(GET_REAL_DEX(vict));
		victim_sav = -GET_REAL_SAVING_REFLEX(vict);
		bonus = dex_bonus(GET_REAL_DEX(ch));
		//pass_mod = 1;
		if (GET_MOB_HOLD(vict))
		{
			bonus += (skill_is + bonus) / 2;
		}
		else
		{
			if (!CAN_SEE(ch, vict))
				bonus += (skill_is + bonus) / 5;
			//if (vict->get_fighting() || (MOB_FLAGGED(vict, MOB_AWARE) || AFF_FLAGGED(vict, AFF_AWARNESS) || AWAKE(vict) ))
				//victim_modi -= 40;
			if (PRF_FLAGGED(vict, PRF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
		}
		break;
		}
		
	case SKILL_STUN: //ошеломить
		{
		//victim_sav = GET_SAVE(vict, SAVING_STABILITY) - dex_bonus(GET_REAL_CON(vict)) - GET_LEVEL(vict);
		victim_sav = -GET_REAL_SAVING_STABILITY(vict);

		if (!IS_NPC(vict))
			victim_sav *= 2;
		else
			victim_sav -= GET_LEVEL(vict);

		bonus = dex_bonus(GET_REAL_STR(ch));
		if (GET_EQ(ch, WEAR_WIELD))
			bonus +=
			weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))].shocking;
		else if (GET_EQ(ch, WEAR_BOTHS))
			bonus +=
			weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))].shocking;

		if (PRF_FLAGGED(vict, PRF_AWAKE))
			victim_modi -= calculate_awake_mod(ch, vict);

		// Полель не убираем учет удачи
		//pass_mod = 1; //Убираем учет удачи  
		break;
	}

	default:
		break;
	}

	//        if(IS_NPC(ch))
	//        bonus = 0;
	if (skill_no == SKILL_TRACK)
	{
		return percent;
	}
	else
	{
		percent = skill_is + bonus + victim_sav + victim_modi / 2;   // вычисление процента прохождения скила
	}
	if (ch && vict && !IS_NPC(ch) && (skill_no == SKILL_BASH || skill_no == SKILL_STRANGLE || skill_no == SKILL_MIGHTHIT
		|| skill_no == SKILL_STUPOR || skill_no == SKILL_CHOPOFF || skill_no == SKILL_BACKSTAB || skill_no == SKILL_KICK
		|| skill_no == SKILL_PUNCTUAL || skill_no == SKILL_STUN|| skill_no == SKILL_SENSE) && PRF_FLAGGED(ch, PRF_TESTER))
	{
		sprintf(buf, "&CПротивник %s: скилл == %d, итоговыйрасчетскилла == %d, бонус == %d, сэйвыпротивника == %d, модификаторпротивника == %d&n\r\n", GET_NAME(vict), skill_is, percent, bonus, victim_sav, victim_modi/2);
		send_to_char(buf, ch);
	}

	if (PRF_FLAGGED(ch, PRF_AWAKE) && (skill_no == SKILL_BASH))
		percent /= 2;

	// не все умения надо модифицировать из-за внешних факторов и морали
	if (!pass_mod)
	{
		//		percent = complex_skill_modifier(ch, skill_no, GAPPLY_SKILL_SUCCESS, percent);
		morale = ch->calc_morale();

		//		if (vict && percent > skill_info[skill_no].max_percent)
		//			victim_modi += percent - skill_info[skill_no].max_percent;

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_DEAFNESS))
			morale -= 20;	// у глухого мораль на 20 меньше
		// Обработка способности "боевой дух"
		if (vict && can_use_feat(vict, SPIRIT_WARRIOR_FEAT))
			morale -= 10;

		const int prob = number(0, 999);

		int morale_bonus = morale;
		if (morale < 0) {
			morale_bonus = morale * 10;
		}
		const int bonus_limit = MIN(150, morale * 10);
		fail_limit = MIN(990, 950 + morale_bonus * 10 / 6);
		// Если prob попадает в полуинтервал [0, bonus_limit) - бонус в виде макс. процента и
		// игнора спас-бросков, если в отрезок [fail_limit, 999] - способность фэйлится. Иначе
		// все решают спас-броски.
		if (morale >= 50)   // от 50 удачи абсолютный фейл не работает
			fail_limit = 999;
		if (prob >= fail_limit)
		{   // Абсолютный фейл 4.9 процента
			percent = 0;
		}
		else if (prob < bonus_limit)
		{
			percent = max_percent + bonus;
		}
	}

	// иммские флаги и прокла влияют на все
	if (IS_IMMORTAL(ch))
		percent = MAX(percent, skill_info[skill_no].max_percent);
	else if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		percent = 0;
	else if (vict && GET_GOD_FLAG(vict, GF_GODSCURSE))
		percent = MAX(percent, max_percent);
	else
		percent = MIN(MAX(0, percent), max_percent);
/*
	if (ch && vict && !IS_NPC(ch) && (skill_no == SKILL_BASH || skill_no == SKILL_STRANGLE || skill_no == SKILL_MIGHTHIT
		|| skill_no == SKILL_STUPOR || skill_no == SKILL_CHOPOFF || skill_no == SKILL_BACKSTAB || skill_no == SKILL_KICK
		|| skill_no == SKILL_PUNCTUAL || skill_no == SKILL_STUN) && PRF_FLAGGED(ch, PRF_TESTER))
	{
		//sprintf(buf, "Противник %s: скилл == %d, итоговый == %d,бонус == %d, сэйвы == %d, сэйвы/2 == %d\r\n", GET_NAME(vict), skill_is, percent, bonus, victim_sav, victim_modi);
		//mudlog(buf, LGH, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		if (absolute_fail)
			send_to_char(ch, "попали в Абсолютный фейл\r\n");
		else if (try_morale)
			send_to_char(ch, "&Cпопали в удачу. итоговый prob = %d, скилл = %d, бонус = %d, сэйвы = %d, сэйвы/2 = %d, мораль = %d&n\r\n", percent, skill_is, bonus, victim_sav, victim_modi / 2, morale);
		else
			send_to_char(ch, "&Cитоговый prob = %d, скилл = %d, бонус = %d, сэйвы = %d, сэйвы/2 = %d&n\r\n", percent, skill_is, bonus, victim_sav, victim_modi / 2);
	}
	if (skill_no == SKILL_PROTECT)
		log("SKILL_PROTECT: Игрок %s , Percent == %d, fail_limit == %d, Morale == %d, Bonus == %d\r\n", GET_NAME(ch), percent, fail_limit, morale, bonus);
*/
	return (percent);
}

void improove_skill(CHAR_DATA * ch, const ESkill skill_no, int success, CHAR_DATA * victim)
{
	const int trained_skill = ch->get_trained_skill(skill_no);
	if (trained_skill == 0 || trained_skill == 200)
	{
		// скила может и не быть, если получен только со шмоток
		return;
	}

	if (IS_NPC(ch))
	{
		return;
	}

	if (victim && 
		(MOB_FLAGGED(victim, MOB_MOUNTING)|| MOB_FLAGGED(victim, MOB_NOTRAIN)))
	{
		return;
	}

	int skill_is, diff = 0, prob, div;

	if (IS_IMMORTAL(ch)
		|| ((!victim || OK_GAIN_EXP(ch, victim))
			&& ch->in_room != NOWHERE
			&& !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)
			// Стрибог
			&& !ROOM_FLAGGED(ch->in_room, ROOM_ARENA)
			//Свентовит
			&& !ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)
			&& !ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM)
			&& (diff = wis_bonus(GET_REAL_WIS(ch), WIS_MAX_LEARN_L20) * GET_LEVEL(ch) / 20 - trained_skill) > 0
			&& trained_skill < MAX_EXP_RMRT_PERCENT(ch)))
	{
		// Success - multy by 2
		prob = success ? 20000 : 15000;


	// Если чар нуб, то до 50% скиллы качаются гораздо быстрее
	int INT_PLAYER = (ch->get_trained_skill(skill_no) < 51 && (AFF_FLAGGED(ch, EAffectFlag::AFF_NOOB_REGEN))) ? 50 : GET_REAL_INT(ch);
	div = int_app[INT_PLAYER].improove /* + diff */;

		if ((int)GET_CLASS(ch) >= 0 && (int)GET_CLASS(ch) < NUM_PLAYER_CLASSES)
		{
			div += (skill_info[skill_no].k_improove[(int)GET_CLASS(ch)][(int)GET_KIN(ch)] / 100);
		}

		prob /= (MAX(1, div));
		// вариант бонуса мудрости без штрафов за кол-во умений
		prob -= 5 * wis_bonus(GET_REAL_WIS(ch), WIS_MAX_SKILLS);
		prob += number(1, trained_skill * 5);

		skill_is = number(1, MAX(1, prob));

		//if (!IS_NPC(ch))
		//log("Player %s skill '%d' - need to improove %d(%d-%d)",
		//GET_NAME(ch), skill_no, skill_is, div, prob);
		if ((skill_no == SKILL_STEAL) && (!IS_NPC(victim)))
		{
			return;
		}
		if ((victim && skill_is <= GET_REAL_INT(ch) * GET_LEVEL(victim) / GET_LEVEL(ch))
			|| (!victim && skill_is <= GET_REAL_INT(ch)))
		{
			if (success)
			{
				sprintf(buf, "%sВы повысили уровень умения \"%s\".%s\r\n",
					CCICYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
			}
			else
			{
				sprintf(buf, "%sПоняв свои ошибки, вы повысили уровень умения \"%s\".%s\r\n",
					CCICYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
			}
			send_to_char(buf, ch);
			ch->set_morphed_skill(skill_no, (trained_skill + number(1, 2)));
			if (!IS_IMMORTAL(ch))
			{
				ch->set_morphed_skill(skill_no, (MIN(MAX_EXP_PERCENT + GET_REMORT(ch) * 5, ch->get_trained_skill(skill_no))));
			}
			// скилл прокачался, помечаю моба (если он есть)
			if (victim && IS_NPC(victim))
			{
				MOB_FLAGS(victim).set(MOB_NOTRAIN);
			}
		}
	}
}

int train_skill(CHAR_DATA * ch, const ESkill skill_no, int max_value, CHAR_DATA * vict)
{
	int percent = 0;

	percent = calculate_skill(ch, skill_no, vict);
	if (!IS_NPC(ch))
	{
		if (skill_no != SKILL_SATTACK
			&& ch->get_trained_skill(skill_no) > 0
			&& (!vict
				|| (IS_NPC(vict)
					&& !MOB_FLAGGED(vict, MOB_PROTECT)
					&& !MOB_FLAGGED(vict, MOB_NOTRAIN)
					&& !AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
					&& !IS_HORSE(vict))))
		{
			improove_skill(ch, skill_no, percent >= max_value, vict);
		}
	}
	else if (!IS_CHARMICE(ch))
	{
		if (ch->get_skill(skill_no) > 0
			&& GET_REAL_INT(ch) <= number(0, 1000 - 20 * GET_REAL_WIS(ch))
			&& ch->get_skill(skill_no) < skill_info[skill_no].max_percent)
		{
			ch->set_skill(skill_no, ch->get_skill(skill_no) + 1);
		}
	}

	return (percent);
}

/**
* Расчет влияния осторожки у victim против умений killer.
* В данный момент учитывается случай 'игрок против игрока', где осторожка считается как скилл/2
*/
int calculate_awake_mod(CHAR_DATA *killer, CHAR_DATA *victim)
{
	int result = 0;
	if (!killer || !victim)
	{
		log("SYSERROR: zero character in calculate_awake_mod.");
	}
	else if (IS_NPC(killer) || IS_NPC(victim))
	{
		result = victim->get_skill(SKILL_AWAKE);
	}
	else
	{
		result = victim->get_skill(SKILL_AWAKE) / 2;
	}
	return result;
}

int find_weapon_focus_by_skill(ESkill skill)
{
	switch (skill)
	{
	case SKILL_PUNCH:
		return PUNCH_FOCUS_FEAT;
	break;
	case SKILL_CLUBS:
		return CLUB_FOCUS_FEAT;
	break;
	case SKILL_AXES:
		return AXES_FOCUS_FEAT;
	break;
	case SKILL_LONGS:
		return LONGS_FOCUS_FEAT;
	break;
	case SKILL_SHORTS:
		return SHORTS_FOCUS_FEAT;
	break;
	case SKILL_NONSTANDART:
		return NONSTANDART_FOCUS_FEAT;
	break;
	case SKILL_BOTHHANDS:
		return BOTHHANDS_FOCUS_FEAT;
	break;
	case SKILL_PICK:
		return PICK_FOCUS_FEAT;
	break;
	case SKILL_SPADES:
		return SPADES_FOCUS_FEAT;
	break;
	case SKILL_BOWS:
		return BOWS_FOCUS_FEAT;
	break;
	default:
		return THAC0_FEAT;
	}
}

int find_weapon_master_by_skill(ESkill skill)
{
	switch (skill)
	{
	case SKILL_PUNCH:
		return PUNCH_MASTER_FEAT;
	break;
	case SKILL_CLUBS:
		return CLUBS_MASTER_FEAT;
	break;
	case SKILL_AXES:
		return AXES_MASTER_FEAT;
	break;
	case SKILL_LONGS:
		return LONGS_MASTER_FEAT;
	break;
	case SKILL_SHORTS:
		return SHORTS_MASTER_FEAT;
	break;
	case SKILL_NONSTANDART:
		return NONSTANDART_MASTER_FEAT;
	break;
	case SKILL_BOTHHANDS:
		return BOTHHANDS_MASTER_FEAT;
	break;
	case SKILL_PICK:
		return PICK_MASTER_FEAT;
	break;
	case SKILL_SPADES:
		return SPADES_MASTER_FEAT;
	break;
	case SKILL_BOWS:
		return BOWS_MASTER_FEAT;
	break;
	default:
		return THAC0_FEAT;
	}
}

//Определим мин уровень для изучения скилла из книги
//req_lvl - требуемый уровень из книги
int min_skill_level_with_req(CHAR_DATA *ch, int skill, int req_lvl)
{
	int min_lvl = MAX(req_lvl, skill_info[skill].min_level[ch->get_class()][ch->get_kin()])
		- MAX(0, ch->get_remort() / skill_info[skill].level_decrement[ch->get_class()][ch->get_kin()]);

	return MAX(1, min_lvl);
};

/*
 * функция определяет, может ли персонаж илучить скилл
 * впоследствии ее нужно будет перенести в класс "инфа о классе персонажа"
 */
int min_skill_level(CHAR_DATA *ch, int skill)
{
	int min_lvl = skill_info[skill].min_level[ch->get_class()][ch->get_kin()]
		- MAX(0, ch->get_remort() / skill_info[skill].level_decrement[ch->get_class()][ch->get_kin()]);

	return MAX(1, min_lvl);
};

bool can_get_skill_with_req(CHAR_DATA *ch, int skill, int req_lvl)
{
	if (ch->get_remort() < skill_info[skill].min_remort[ch->get_class()][ch->get_kin()]
		|| (skill_info[skill].classknow[ch->get_class()][ch->get_kin()] != KNOW_SKILL))
	{
		return FALSE;
	}
	if (ch->get_level() < min_skill_level_with_req(ch, skill, req_lvl))
	{
		return FALSE;
	}

	return TRUE;
}

bool can_get_skill(CHAR_DATA *ch, int skill)
{
	if (ch->get_remort() < skill_info[skill].min_remort[ch->get_class()][ch->get_kin()]
		|| (skill_info[skill].classknow[ch->get_class()][ch->get_kin()] != KNOW_SKILL))
	{
		return FALSE;
	}
	if (ch->get_level() < min_skill_level(ch, skill))
	{
		return FALSE;
	}

	return TRUE;
}

//  Реализация класса Skill

//Объявляем глобальный скиллист
SkillListType Skill::SkillList;

// Конструктор
Skill::Skill() : _Name(SKILL_NAME_UNDEFINED), _Number(SKILL_UNDEFINED), _MaxPercent(0)
{
// Инициализация от греха подальше
};

// Парсим блок описания скилла
void Skill::ParseSkill(pugi::xml_node SkillNode)
{
	std::string SkillID;
	pugi::xml_node CurNode;
	SkillPtr TmpSkill(new Skill);

	// Базовые параметры скилла (а пока боле ничего и нет)
	SkillID = SkillNode.attribute("id").value();

	CurNode = SkillNode.child("number");
	TmpSkill->_Number = CurNode.attribute("val").as_int();
	CurNode = SkillNode.child("name");
	TmpSkill->_Name = CurNode.attribute("text").value();
	CurNode = SkillNode.child("max_percent");
	TmpSkill->_MaxPercent = CurNode.attribute("val").as_int();

	// Добавляем новый скилл в лист
	Skill::SkillList.insert(make_pair(SkillID, TmpSkill));
};

// Парсинг скиллов
// Вынесено в отдельную функцию, чтобы, если нам передали кривой XML лист, не создавался кривой скилл
void Skill::Load(const pugi::xml_node& XMLSkillList)
{
	pugi::xml_node CurNode;

	for (CurNode = XMLSkillList.child("skill"); CurNode; CurNode = CurNode.next_sibling("skill"))
	{
		Skill::ParseSkill(CurNode);
	}
};

// Получаем номер скилла по его иду
// Отрыжка совместимости со старым кодом
int Skill::GetNumByID(const std::string& ID)
{
	SkillPtr TmpSkill = Skill::SkillList[ID];

	if (TmpSkill)
	{
		return TmpSkill->_Number;
	}

	return SKILL_UNDEFINED;
};

// Конец (увы) реализации класса Skill

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
