#include "structs.h"
#include "char.hpp"
#include "spells.h"
#include "utils.h"
#include "logger.hpp"
#include "msdp.hpp"
#include "msdp.constants.hpp"

void asciiflag_conv(const char *flag, void *to)
{
	int *flags = (int *)to;
	unsigned int is_number = 1, block = 0, i;
	const char *p;

	for (p = flag; *p; p += i + 1)
	{
		i = 0;
		if (islower(*p))
		{
			if (*(p + 1) >= '0' && *(p + 1) <= '9')
			{
				block = (int)* (p + 1) - '0';
				i = 1;
			}
			else
				block = 0;
			*(flags + block) |= (0x3FFFFFFF & (1 << (*p - 'a')));
		}
		else if (isupper(*p))
		{
			if (*(p + 1) >= '0' && *(p + 1) <= '9')
			{
				block = (int)* (p + 1) - '0';
				i = 1;
			}
			else
				block = 0;
			*(flags + block) |= (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
		}
		if (!isdigit(*p))
			is_number = 0;
	}

	if (is_number)
	{
		is_number = atol(flag);
		block = is_number < INT_ONE ? 0 : is_number < INT_TWO ? 1 : is_number < INT_THREE ? 2 : 3;
		*(flags + block) = is_number & 0x3FFFFFFF;
	}
}

int ext_search_block(const char *arg, const char * const * const list, int exact)
{
	int i, j, o;

	if (exact)
	{
		for (i = j = 0, o = 1; j != 1 && **(list + i); i++)	// shapirus: попытка в лоб убрать креш
		{
			if (**(list + i) == '\n')
			{
				o = 1;
				switch (j)
				{
				case 0:
					j = INT_ONE;
					break;

				case INT_ONE:
					j = INT_TWO;
					break;

				case INT_TWO:
					j = INT_THREE;
					break;

				default:
					j = 1;
					break;
				}
			}
			else if (!str_cmp(arg, *(list + i)))
			{
				return j | o;
			}
			else
			{
				o <<= 1;
			}
		}
	}
	else
	{
		size_t l = strlen(arg);
		if (!l)
		{
			l = 1;	// Avoid "" to match the first available string
		}
		for (i = j = 0, o = 1; j != 1 && **(list + i); i++)	// shapirus: попытка в лоб убрать креш
		{
			if (**(list + i) == '\n')
			{
				o = 1;
				switch (j)
				{
				case 0:
					j = INT_ONE;
					break;
				case INT_ONE:
					j = INT_TWO;
					break;
				case INT_TWO:
					j = INT_THREE;
					break;
				default:
					j = 1;
					break;
				}
			}
			else if (!strn_cmp(arg, *(list + i), l))
			{
				return j | o;
			}
			else
			{
				o <<= 1;
			}
		}
	}

	return 0;
}

void FLAG_DATA::from_string(const char *flag)
{
	uint32_t is_number = 1;
	int i;

	for (const char* p = flag; *p; p += i + 1)
	{
		i = 0;
		if (islower(*p))
		{
			size_t block = 0;
			if (*(p + 1) >= '0' && *(p + 1) <= '9')
			{
				block = (int)* (p + 1) - '0';
				i = 1;
			}
			m_flags[block] |= (0x3FFFFFFF & (1 << (*p - 'a')));
		}
		else if (isupper(*p))
		{
			size_t block = 0;
			if (*(p + 1) >= '0' && *(p + 1) <= '9')
			{
				block = (int)* (p + 1) - '0';
				i = 1;
			}
			m_flags[block] |= (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
		}

		if (!isdigit(*p))
		{
			is_number = 0;
		}
	}

	if (is_number)
	{
		is_number = atol(flag);
		const size_t block = is_number >> 30;
		m_flags[block] = is_number & 0x3FFFFFFF;
	}
}

void FLAG_DATA::tascii(int num_planes, char* ascii) const
{
	bool found = false;

	for (int i = 0; i < num_planes; i++)
	{
		for (int c = 0; c < 30; c++)
		{
			if (m_flags[i] & (1 << c))
			{
				found = true;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strcat(ascii, found ? " " : "0 ");
}

void tascii(const uint32_t* pointer, int num_planes, char* ascii)
{
	bool found = false;

	for (int i = 0; i < num_planes; i++)
	{
		for (int c = 0; c < 30; c++)
		{
			if (pointer[i] & (1 << c))
			{
				found = true;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strcat(ascii, found ? " " : "0 ");
}

const char* nothing_string = "ничего";

bool sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div, const int print_flag)
{
	
	long nr = 0;
	int fail = 0;
	int plane = 0;
	char c = 'a';

	*result = '\0';

	fail = bitvector >> 30;
	bitvector &= 0x3FFFFFFF;
	while (fail)
	{
		if (*names[nr] == '\n')
		{
			fail--;
			plane++;
		}
		nr++;
	}

	bool can_show = true;
	for (; bitvector; bitvector >>= 1)
	{
		if (IS_SET(bitvector, 1))
		{
			can_show = ((*names[nr]!='*') || (print_flag&4));

			if (*result != '\0' && can_show)
				strcat(result,div);

			if (*names[nr] != '\n')
			{
				if (print_flag & 1)
				{
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				if ((print_flag & 2) && (!strcmp(names[nr], "UNUSED")))
				{
					sprintf(result + strlen(result), "%ld:", nr + 1);
				}
				if (can_show)
					strcat(result, (*names[nr]!='*'?names[nr]:names[nr]+1));
			}
			else
			{
				if (print_flag & 2)
				{
					sprintf(result + strlen(result), "%ld:", nr + 1);
				}
				else if (print_flag & 1)
				{
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				strcat(result, "UNDEF");
			}
		}

		if (print_flag & 1)
		{
			c++;
			if (c > 'z')
			{
				c = 'A';
			}
		}
		if (*names[nr] != '\n')
		{
			nr++;
		}
	}

	if ('\0' == *result)
	{
		strcat(result, nothing_string);
		return false;
	}

	return true;
}

bool FLAG_DATA::sprintbits(const char *names[], char *result, const char *div, const int print_flag) const
{
	bool have_flags = false;
	char buffer[MAX_STRING_LENGTH];
	*result = '\0';

	for (int i = 0; i < 4; i++)
	{
		if (sprintbitwd(m_flags[i] | (i << 30), names, buffer, div, print_flag))
		{
			if ('\0' != *result)	// We don't need to calculate length of result. We just want to know if it is not empty.
			{
				strcat(result, div);
			}
			strcat(result, buffer);
			have_flags = true;
		}
	}

	if ('\0' == *result)
	{
		strcat(result, nothing_string);
	}

	return have_flags;
}

void FLAG_DATA::gm_flag(const char *subfield, const char * const * const list, char *res)
{
	strcpy(res, "0");

	if ('\0' == *subfield)
	{
		return;
	}

	if (*subfield == '-')
	{
		const int flag = ext_search_block(subfield + 1, list, FALSE);
		if (flag)
		{
			unset(flag);
			if (plane_not_empty(flag))	// looks like an error: we should check flag, but not whole plane
			{
				strcpy(res, "1");
			}
		}
	}
	else if (*subfield == '+')
	{
		const int flag = ext_search_block(subfield + 1, list, FALSE);
		if (flag)
		{
			set(flag);
			if (plane_not_empty(flag))	// looks like an error: we should check flag, but not whole plane
			{
				strcpy(res, "1");
			}
		}
	}
	else
	{
		const int flag = ext_search_block(subfield, list, FALSE);
		if (flag && get(flag))
		{
			strcpy(res, "1");
		}
	}

	return;
}

const religion_names_t religion_name =
{
	religion_genders_t{ "Язычник", "Язычник", "Язычница", "Язычники" },
	religion_genders_t{ "Христианин", "Христианин", "Христианка", "Христиане" },
	religion_genders_t{ "", "", "", "" }		// for undefined religion
};

std::unordered_map<int, std::string> SECTOR_TYPE_BY_VALUE = {
	{ SECT_INSIDE, "inside" },
	{ SECT_CITY, "city" },
	{ SECT_FIELD, "field" },
	{ SECT_FOREST, "forest" },
	{ SECT_HILLS, "hills" },
	{ SECT_MOUNTAIN, "mountain" },
	{ SECT_WATER_SWIM, "swim water" },
	{ SECT_WATER_NOSWIM, "no swim water" },
	{ SECT_FLYING, "flying" },
	{ SECT_UNDERWATER, "underwater" },
	{ SECT_SECRET, "secret" },
	{ SECT_STONEROAD, "stone road" },
	{ SECT_ROAD, "road" },
	{ SECT_WILDROAD, "wild road" },
	{ SECT_FIELD_SNOW, "snow field" },
	{ SECT_FIELD_RAIN, "rain field" },
	{ SECT_FOREST_SNOW, "snow forest" },
	{ SECT_FOREST_RAIN, "rain forest" },
	{ SECT_HILLS_SNOW, "snow hills" },
	{ SECT_HILLS_RAIN, "rain hills" },
	{ SECT_MOUNTAIN_SNOW, "snow mountain" },
	{ SECT_THIN_ICE, "thin ice" },
	{ SECT_NORMAL_ICE, "normal ice" },
	{ SECT_THICK_ICE, "thick ice" }
};

typedef std::map<ESex, std::string> ESex_name_by_value_t;
typedef std::map<const std::string, ESex> ESex_value_by_name_t;
ESex_name_by_value_t ESex_name_by_value;
ESex_value_by_name_t ESex_value_by_name;

void init_ESex_ITEM_NAMES()
{
	ESex_name_by_value.clear();
	ESex_value_by_name.clear();

	ESex_name_by_value[ESex::SEX_NEUTRAL] = "NEUTRAL";
	ESex_name_by_value[ESex::SEX_MALE] = "MALE";
	ESex_name_by_value[ESex::SEX_FEMALE] = "FEMALE";
	ESex_name_by_value[ESex::SEX_POLY] = "POLY";

	for (const auto& i : ESex_name_by_value)
	{
		ESex_value_by_name[i.second] = i.first;
	}
}

template <>
ESex ITEM_BY_NAME(const std::string& name)
{
	if (ESex_name_by_value.empty())
	{
		init_ESex_ITEM_NAMES();
	}
	return ESex_value_by_name.at(name);
}

template <>
const std::string& NAME_BY_ITEM(const ESex item)
{
	if (ESex_name_by_value.empty())
	{
		init_ESex_ITEM_NAMES();
	}
	return ESex_name_by_value.at(item);
}

typedef std::map<EWeaponAffectFlag, std::string> EWeaponAffectFlag_name_by_value_t;
typedef std::map<const std::string, EWeaponAffectFlag> EWeaponAffectFlag_value_by_name_t;
EWeaponAffectFlag_name_by_value_t EWeaponAffectFlag_name_by_value;
EWeaponAffectFlag_value_by_name_t EWeaponAffectFlag_value_by_name;

void init_EWeaponAffectFlag_ITEM_NAMES()
{
	EWeaponAffectFlag_name_by_value.clear();
	EWeaponAffectFlag_value_by_name.clear();

	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLINDNESS] = "WAFF_BLINDNESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_INVISIBLE] = "WAFF_INVISIBLE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_ALIGN] = "WAFF_DETECT_ALIGN";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_INVISIBLE] = "WAFF_DETECT_INVISIBLE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_MAGIC] = "WAFF_DETECT_MAGIC";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SENSE_LIFE] = "WAFF_SENSE_LIFE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_WATER_WALK] = "WAFF_WATER_WALK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SANCTUARY] = "WAFF_SANCTUARY";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_CURSE] = "WAFF_CURSE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_INFRAVISION] = "WAFF_INFRAVISION";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_POISON] = "WAFF_POISON";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PROTECT_EVIL] = "WAFF_PROTECT_EVIL";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PROTECT_GOOD] = "WAFF_PROTECT_GOOD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SLEEP] = "WAFF_SLEEP";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_NOTRACK] = "WAFF_NOTRACK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLESS] = "WAFF_BLESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SNEAK] = "WAFF_SNEAK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HIDE] = "WAFF_HIDE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLD] = "WAFF_HOLD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FLY] = "WAFF_FLY";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SILENCE] = "WAFF_SILENCE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AWARENESS] = "WAFF_AWARENESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLINK] = "WAFF_BLINK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_NOFLEE] = "WAFF_NOFLEE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SINGLE_LIGHT] = "WAFF_SINGLE_LIGHT";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLY_LIGHT] = "WAFF_HOLY_LIGHT";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLY_DARK] = "WAFF_HOLY_DARK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_POISON] = "WAFF_DETECT_POISON";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SLOW] = "WAFF_SLOW";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HASTE] = "WAFF_HASTE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_WATER_BREATH] = "WAFF_WATER_BREATH";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HAEMORRAGIA] = "WAFF_HAEMORRAGIA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_CAMOUFLAGE] = "WAFF_CAMOUFLAGE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SHIELD] = "WAFF_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AIR_SHIELD] = "WAFF_AIR_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FIRE_SHIELD] = "WAFF_FIRE_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_ICE_SHIELD] = "WAFF_ICE_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_MAGIC_GLASS] = "WAFF_MAGIC_GLASS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_STONE_HAND] = "WAFF_STONE_HAND";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PRISMATIC_AURA] = "WAFF_PRISMATIC_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AIR_AURA] = "WAFF_AIR_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FIRE_AURA] = "WAFF_FIRE_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_ICE_AURA] = "WAFF_ICE_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DEAFNESS] = "WAFF_DEAFNESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_COMMANDER] = "WAFF_COMMANDER";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_EARTHAURA] = "WAFF_EARTHAURA";
	for (const auto& i : EWeaponAffectFlag_name_by_value)
	{
		EWeaponAffectFlag_value_by_name[i.second] = i.first;
	}
}

template <>
EWeaponAffectFlag ITEM_BY_NAME(const std::string& name)
{
	if (EWeaponAffectFlag_name_by_value.empty())
	{
		init_EWeaponAffectFlag_ITEM_NAMES();
	}
	return EWeaponAffectFlag_value_by_name.at(name);
}

template <>
const std::string& NAME_BY_ITEM(const EWeaponAffectFlag item)
{
	if (EWeaponAffectFlag_name_by_value.empty())
	{
		init_EWeaponAffectFlag_ITEM_NAMES();
	}
	return EWeaponAffectFlag_name_by_value.at(item);
}

typedef std::map<EWearFlag, std::string> EWearFlag_name_by_value_t;
typedef std::map<const std::string, EWearFlag> EWearFlag_value_by_name_t;
EWearFlag_name_by_value_t EWearFlag_name_by_value;
EWearFlag_value_by_name_t EWearFlag_value_by_name;

void init_EWearFlag_ITEM_NAMES()
{
	EWearFlag_name_by_value.clear();
	EWearFlag_value_by_name.clear();

	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_TAKE] = "ITEM_WEAR_TAKE";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_FINGER] = "ITEM_WEAR_FINGER";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_NECK] = "ITEM_WEAR_NECK";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_BODY] = "ITEM_WEAR_BODY";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_HEAD] = "ITEM_WEAR_HEAD";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_LEGS] = "ITEM_WEAR_LEGS";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_FEET] = "ITEM_WEAR_FEET";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_HANDS] = "ITEM_WEAR_HANDS";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_ARMS] = "ITEM_WEAR_ARMS";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_SHIELD] = "ITEM_WEAR_SHIELD";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_ABOUT] = "ITEM_WEAR_ABOUT";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_WAIST] = "ITEM_WEAR_WAIST";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_WRIST] = "ITEM_WEAR_WRIST";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_WIELD] = "ITEM_WEAR_WIELD";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_HOLD] = "ITEM_WEAR_HOLD";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_BOTHS] = "ITEM_WEAR_BOTHS";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_QUIVER] = "ITEM_WEAR_QUIVER";

	for (const auto& i : EWearFlag_name_by_value)
	{
		EWearFlag_value_by_name[i.second] = i.first;
	}
}

template <> const std::string& NAME_BY_ITEM<EWearFlag>(const EWearFlag item)
{
	if (EWearFlag_name_by_value.empty())
	{
		init_EWearFlag_ITEM_NAMES();
	}
	return EWearFlag_name_by_value.at(item);
}

template <> EWearFlag ITEM_BY_NAME<EWearFlag>(const std::string& name)
{
	if (EWearFlag_name_by_value.empty())
	{
		init_EWearFlag_ITEM_NAMES();
	}
	return EWearFlag_value_by_name.at(name);
}

typedef std::map<EExtraFlag, std::string> EExtraFlag_name_by_value_t;
typedef std::map<const std::string, EExtraFlag> EExtraFlag_value_by_name_t;
EExtraFlag_name_by_value_t EExtraFlag_name_by_value;
EExtraFlag_value_by_name_t EExtraFlag_value_by_name;

void init_EExtraFlag_ITEM_NAMES()
{
	EExtraFlag_name_by_value.clear();
	EExtraFlag_value_by_name.clear();

	EExtraFlag_name_by_value[EExtraFlag::ITEM_GLOW] = "ITEM_GLOW";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_HUM] = "ITEM_HUM";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NORENT] = "ITEM_NORENT";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NODONATE] = "ITEM_NODONATE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOINVIS] = "ITEM_NOINVIS";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_INVISIBLE] = "ITEM_INVISIBLE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_MAGIC] = "ITEM_MAGIC";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NODROP] = "ITEM_NODROP";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_BLESS] = "ITEM_BLESS";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOSELL] = "ITEM_NOSELL";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_DECAY] = "ITEM_DECAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_ZONEDECAY] = "ITEM_ZONEDECAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NODISARM] = "ITEM_NODISARM";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NODECAY] = "ITEM_NODECAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_POISONED] = "ITEM_POISONED";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SHARPEN] = "ITEM_SHARPEN";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_ARMORED] = "ITEM_ARMORED";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_DAY] = "ITEM_DAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NIGHT] = "ITEM_NIGHT";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_FULLMOON] = "ITEM_FULLMOON";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_WINTER] = "ITEM_WINTER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SPRING] = "ITEM_SPRING";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SUMMER] = "ITEM_SUMMER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_AUTUMN] = "ITEM_AUTUMN";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SWIMMING] = "ITEM_SWIMMING";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_FLYING] = "ITEM_FLYING";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_THROWING] = "ITEM_THROWING";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_TICKTIMER] = "ITEM_TICKTIMER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_FIRE] = "ITEM_FIRE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_REPOP_DECAY] = "ITEM_REPOP_DECAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOLOCATE] = "ITEM_NOLOCATE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_TIMEDLVL] = "ITEM_TIMEDLVL";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOALTER] = "ITEM_NOALTER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_WITH1SLOT] = "ITEM_WITH1SLOT";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_WITH2SLOTS] = "ITEM_WITH2SLOTS";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_WITH3SLOTS] = "ITEM_WITH3SLOTS";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SETSTUFF] = "ITEM_SETSTUFF";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NO_FAIL] = "ITEM_NO_FAIL";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NAMED] = "ITEM_NAMED";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_BLOODY] = "ITEM_BLOODY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_1INLAID] = "ITEM_1INLAID";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_2INLAID] = "ITEM_2INLAID";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_3INLAID] = "ITEM_3INLAID";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOPOUR] = "ITEM_NOPOUR";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_UNIQUE] = "ITEM_UNIQUE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_TRANSFORMED] = "ITEM_TRANSFORMED";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOT_DEPEND_RPOTO] = "ITEM_NOT_DEPEND_RPOTO";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOT_UNLIMIT_TIMER] = "ITEM_NOT_UNLIMIT_TIMER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_UNIQUE_WHEN_PURCHASE] = "ITEM_UNIQUE_WHEN_PURCHASE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOT_ONE_CLANCHEST] = "ITEM_NOT_ONE_CLANCHEST";

	for (const auto& i : EExtraFlag_name_by_value)
	{
		EExtraFlag_value_by_name[i.second] = i.first;
	}
}

template <>
const std::string& NAME_BY_ITEM(const EExtraFlag item)
{
	if (EExtraFlag_name_by_value.empty())
	{
		init_EExtraFlag_ITEM_NAMES();
	}
	return EExtraFlag_name_by_value.at(item);
}

template <>
EExtraFlag ITEM_BY_NAME(const std::string& name)
{
    if (EExtraFlag_name_by_value.empty())
    {
        init_EExtraFlag_ITEM_NAMES();
    }
    return EExtraFlag_value_by_name.at(name);
}

typedef std::map<EAffectFlag, std::string> EAffectFlag_name_by_value_t;
typedef std::map<const std::string, EAffectFlag> EAffectFlag_value_by_name_t;
EAffectFlag_name_by_value_t EAffectFlag_name_by_value;
EAffectFlag_value_by_name_t EAffectFlag_value_by_name;
void init_EAffectFlag_ITEM_NAMES()
{
	EAffectFlag_value_by_name.clear();
	EAffectFlag_name_by_value.clear();

	EAffectFlag_name_by_value[EAffectFlag::AFF_BLIND] = "AFF_BLIND";
	EAffectFlag_name_by_value[EAffectFlag::AFF_INVISIBLE] = "AFF_INVISIBLE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DETECT_ALIGN] = "AFF_DETECT_ALIGN";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DETECT_INVIS] = "AFF_DETECT_INVIS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DETECT_MAGIC] = "AFF_DETECT_MAGIC";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SENSE_LIFE] = "AFF_SENSE_LIFE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_WATERWALK] = "AFF_WATERWALK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SANCTUARY] = "AFF_SANCTUARY";
	EAffectFlag_name_by_value[EAffectFlag::AFF_GROUP] = "AFF_GROUP";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CURSE] = "AFF_CURSE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_INFRAVISION] = "AFF_INFRAVISION";
	EAffectFlag_name_by_value[EAffectFlag::AFF_POISON] = "AFF_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_PROTECT_EVIL] = "AFF_PROTECT_EVIL";
	EAffectFlag_name_by_value[EAffectFlag::AFF_PROTECT_GOOD] = "AFF_PROTECT_GOOD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SLEEP] = "AFF_SLEEP";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOTRACK] = "AFF_NOTRACK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_TETHERED] = "AFF_TETHERED";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BLESS] = "AFF_BLESS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SNEAK] = "AFF_SNEAK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HIDE] = "AFF_HIDE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_COURAGE] = "AFF_COURAGE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CHARM] = "AFF_CHARM";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HOLD] = "AFF_HOLD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_FLY] = "AFF_FLY";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SILENCE] = "AFF_SILENCE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_AWARNESS] = "AFF_AWARNESS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BLINK] = "AFF_BLINK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HORSE] = "AFF_HORSE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOFLEE] = "AFF_NOFLEE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SINGLELIGHT] = "AFF_SINGLELIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HOLYLIGHT] = "AFF_HOLYLIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HOLYDARK] = "AFF_HOLYDARK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DETECT_POISON] = "AFF_DETECT_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DRUNKED] = "AFF_DRUNKED";
	EAffectFlag_name_by_value[EAffectFlag::AFF_ABSTINENT] = "AFF_ABSTINENT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STOPRIGHT] = "AFF_STOPRIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STOPLEFT] = "AFF_STOPLEFT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STOPFIGHT] = "AFF_STOPFIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HAEMORRAGIA] = "AFF_HAEMORRAGIA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CAMOUFLAGE] = "AFF_CAMOUFLAGE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_WATERBREATH] = "AFF_WATERBREATH";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SLOW] = "AFF_SLOW";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HASTE] = "AFF_HASTE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SHIELD] = "AFF_SHIELD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_AIRSHIELD] = "AFF_AIRSHIELD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_FIRESHIELD] = "AFF_FIRESHIELD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_ICESHIELD] = "AFF_ICESHIELD";
	EAffectFlag_name_by_value[EAffectFlag::AFF_MAGICGLASS] = "AFF_MAGICGLASS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STAIRS] = "AFF_STAIRS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STONEHAND] = "AFF_STONEHAND";
	EAffectFlag_name_by_value[EAffectFlag::AFF_PRISMATICAURA] = "AFF_PRISMATICAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_HELPER] = "AFF_HELPER";
	EAffectFlag_name_by_value[EAffectFlag::AFF_EVILESS] = "AFF_EVILESS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_AIRAURA] = "AFF_AIRAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_FIREAURA] = "AFF_FIREAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_ICEAURA] = "AFF_ICEAURA";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DEAFNESS] = "AFF_DEAFNESS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CRYING] = "AFF_CRYING";
	EAffectFlag_name_by_value[EAffectFlag::AFF_PEACEFUL] = "AFF_PEACEFUL";
	EAffectFlag_name_by_value[EAffectFlag::AFF_MAGICSTOPFIGHT] = "AFF_MAGICSTOPFIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BERSERK] = "AFF_BERSERK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_LIGHT_WALK] = "AFF_LIGHT_WALK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BROKEN_CHAINS] = "AFF_BROKEN_CHAINS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_CLOUD_OF_ARROWS] = "AFF_CLOUD_OF_ARROWS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SHADOW_CLOAK] = "AFF_SHADOW_CLOAK";
	EAffectFlag_name_by_value[EAffectFlag::AFF_GLITTERDUST] = "AFF_GLITTERDUST";
	EAffectFlag_name_by_value[EAffectFlag::AFF_AFFRIGHT] = "AFF_AFFRIGHT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SCOPOLIA_POISON] = "AFF_SCOPOLIA_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_DATURA_POISON] = "AFF_DATURA_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_SKILLS_REDUCE] = "AFF_SKILLS_REDUCE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOT_SWITCH] = "AFF_NOT_SWITCH";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BELENA_POISON] = "AFF_BELENA_POISON";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOTELEPORT] = "AFF_NOTELEPORT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_LACKY] = "AFF_LACKY";
	EAffectFlag_name_by_value[EAffectFlag::AFF_BANDAGE] = "AFF_BANDAGE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NO_BANDAGE] = "AFF_NO_BANDAGE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_MORPH] = "AFF_MORPH";
	EAffectFlag_name_by_value[EAffectFlag::AFF_STRANGLED] = "AFF_STRANGLED";
	EAffectFlag_name_by_value[EAffectFlag::AFF_RECALL_SPELLS] = "AFF_RECALL_SPELLS";
	EAffectFlag_name_by_value[EAffectFlag::AFF_NOOB_REGEN] = "AFF_NOOB_REGEN";
	EAffectFlag_name_by_value[EAffectFlag::AFF_VAMPIRE]   = "AFF_VAMPIRE";
	EAffectFlag_name_by_value[EAffectFlag::AFF_EXPEDIENT] = "AFF_EXPEDIENT";
	EAffectFlag_name_by_value[EAffectFlag::AFF_COMMANDER] = "AFF_COMMANDER";
	EAffectFlag_name_by_value[EAffectFlag::AFF_EARTHAURA] = "AFF_EARTHAURA";
	for (const auto& i : EAffectFlag_name_by_value)
	{
		EAffectFlag_value_by_name[i.second] = i.first;
	}
}

template <>
const std::string& NAME_BY_ITEM(const EAffectFlag item)
{
	if (EAffectFlag_name_by_value.empty())
	{
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_name_by_value.at(item);
}

template <>
EAffectFlag ITEM_BY_NAME(const std::string& name)
{
	if (EAffectFlag_name_by_value.empty())
	{
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_value_by_name.at(name);
}

typedef std::map<ENoFlag, std::string> ENoFlag_name_by_value_t;
typedef std::map<const std::string, ENoFlag> ENoFlag_value_by_name_t;
ENoFlag_name_by_value_t ENoFlag_name_by_value;
ENoFlag_value_by_name_t ENoFlag_value_by_name;
void init_ENoFlag_ITEM_NAMES()
{
	ENoFlag_value_by_name.clear();
	ENoFlag_name_by_value.clear();

	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MONO] = "ITEM_NO_MONO";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_POLY] = "ITEM_NO_POLY";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_NEUTRAL] = "ITEM_NO_NEUTRAL";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MAGIC_USER] = "ITEM_NO_MAGIC_USER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_CLERIC] = "ITEM_NO_CLERIC";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_THIEF] = "ITEM_NO_THIEF";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_WARRIOR] = "ITEM_NO_WARRIOR";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_ASSASINE] = "ITEM_NO_ASSASINE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_GUARD] = "ITEM_NO_GUARD";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_PALADINE] = "ITEM_NO_PALADINE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_RANGER] = "ITEM_NO_RANGER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_SMITH] = "ITEM_NO_SMITH";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MERCHANT] = "ITEM_NO_MERCHANT";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_DRUID] = "ITEM_NO_DRUID";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_BATTLEMAGE] = "ITEM_NO_BATTLEMAGE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_CHARMMAGE] = "ITEM_NO_CHARMMAGE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_DEFENDERMAGE] = "ITEM_NO_DEFENDERMAGE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_NECROMANCER] = "ITEM_NO_NECROMANCER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_KILLER] = "ITEM_NO_KILLER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_COLORED] = "ITEM_NO_COLORED";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_BD] = "ITEM_NO_BD";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MALE] = "ITEM_NO_MALE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_FEMALE] = "ITEM_NO_FEMALE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_CHARMICE] = "ITEM_NO_CHARMICE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_POLOVCI] = "ITEM_NO_POLOVCI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_PECHENEGI] = "ITEM_NO_PECHENEGI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MONGOLI] = "ITEM_NO_MONGOLI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_YIGURI] = "ITEM_NO_YIGURI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_KANGARI] = "ITEM_NO_KANGARI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_XAZARI] = "ITEM_NO_XAZARI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_SVEI] = "ITEM_NO_SVEI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_DATCHANE] = "ITEM_NO_DATCHANE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_GETTI] = "ITEM_NO_GETTI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_UTTI] = "ITEM_NO_UTTI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_XALEIGI] = "ITEM_NO_XALEIGI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_NORVEZCI] = "ITEM_NO_NORVEZCI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_RUSICHI] = "ITEM_NO_RUSICHI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_STEPNYAKI] = "ITEM_NO_STEPNYAKI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_VIKINGI] = "ITEM_NO_VIKINGI";

	for (const auto& i : ENoFlag_name_by_value)
	{
		ENoFlag_value_by_name[i.second] = i.first;
	}
}

template <> const std::string& NAME_BY_ITEM<ENoFlag>(const ENoFlag item)
{
	if (ENoFlag_name_by_value.empty())
	{
		init_ENoFlag_ITEM_NAMES();
	}
	return ENoFlag_name_by_value.at(item);
}

template <> ENoFlag ITEM_BY_NAME<ENoFlag>(const std::string& name)
{
	if (ENoFlag_name_by_value.empty())
	{
		init_ENoFlag_ITEM_NAMES();
	}
	return ENoFlag_value_by_name.at(name);
}

typedef std::map<EAntiFlag, std::string> EAntiFlag_name_by_value_t;
typedef std::map<const std::string, EAntiFlag> EAntiFlag_value_by_name_t;
EAntiFlag_name_by_value_t EAntiFlag_name_by_value;
EAntiFlag_value_by_name_t EAntiFlag_value_by_name;
void init_EAntiFlag_ITEM_NAMES()
{
    EAntiFlag_value_by_name.clear();
    EAntiFlag_name_by_value.clear();

    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MONO] = "ITEM_AN_MONO";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_POLY] = "ITEM_AN_POLY";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_NEUTRAL] = "ITEM_AN_NEUTRAL";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MAGIC_USER] = "ITEM_AN_MAGIC_USER";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_CLERIC] = "ITEM_AN_CLERIC";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_THIEF] = "ITEM_AN_THIEF";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_WARRIOR] = "ITEM_AN_WARRIOR";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_ASSASINE] = "ITEM_AN_ASSASINE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_GUARD] = "ITEM_AN_GUARD";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_PALADINE] = "ITEM_AN_PALADINE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_RANGER] = "ITEM_AN_RANGER";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_SMITH] = "ITEM_AN_SMITH";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MERCHANT] = "ITEM_AN_MERCHANT";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_DRUID] = "ITEM_AN_DRUID";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_BATTLEMAGE] = "ITEM_AN_BATTLEMAGE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_CHARMMAGE] = "ITEM_AN_CHARMMAGE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_DEFENDERMAGE] = "ITEM_AN_DEFENDERMAGE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_NECROMANCER] = "ITEM_AN_NECROMANCER";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_KILLER] = "ITEM_AN_KILLER";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_COLORED] = "ITEM_AN_COLORED";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_BD] = "ITEM_AN_BD";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_SEVERANE] = "ITEM_AN_SEVERANE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_POLANE] = "ITEM_AN_POLANE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_KRIVICHI] = "ITEM_AN_KRIVICHI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_VATICHI] = "ITEM_AN_VATICHI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_VELANE] = "ITEM_AN_VELANE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_DREVLANE] = "ITEM_AN_DREVLANE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MALE] = "ITEM_AN_MALE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_FEMALE] = "ITEM_AN_FEMALE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_CHARMICE] = "ITEM_AN_CHARMICE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_POLOVCI] = "ITEM_AN_POLOVCI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_PECHENEGI] = "ITEM_AN_PECHENEGI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MONGOLI] = "ITEM_AN_MONGOLI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_YIGURI] = "ITEM_AN_YIGURI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_KANGARI] = "ITEM_AN_KANGARI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_XAZARI] = "ITEM_AN_XAZARI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_SVEI] = "ITEM_AN_SVEI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_DATCHANE] = "ITEM_AN_DATCHANE";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_GETTI] = "ITEM_AN_GETTI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_UTTI] = "ITEM_AN_UTTI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_XALEIGI] = "ITEM_AN_XALEIGI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_NORVEZCI] = "ITEM_AN_NORVEZCI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_RUSICHI] = "ITEM_AN_RUSICHI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_STEPNYAKI] = "ITEM_AN_STEPNYAKI";
    EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_VIKINGI] = "ITEM_AN_VIKINGI";

    for (const auto& i : EAntiFlag_name_by_value)
    {
        EAntiFlag_value_by_name[i.second] = i.first;
    }
}

template <>
const std::string& NAME_BY_ITEM<EAntiFlag>(const EAntiFlag item)
{
    if (EAntiFlag_name_by_value.empty())
    {
        init_EAntiFlag_ITEM_NAMES();
    }
    return EAntiFlag_name_by_value.at(item);
}

template <>
EAntiFlag ITEM_BY_NAME(const std::string& name)
{
    if (EAntiFlag_name_by_value.empty())
    {
        init_EAntiFlag_ITEM_NAMES();
    }
    return EAntiFlag_value_by_name.at(name);
}

typedef std::map<EApplyLocation, std::string> EApplyLocation_name_by_value_t;
typedef std::map<const std::string, EApplyLocation> EApplyLocation_value_by_name_t;
EApplyLocation_name_by_value_t EApplyLocation_name_by_value;
EApplyLocation_value_by_name_t EApplyLocation_value_by_name;

void init_EApplyLocation_ITEM_NAMES() {
	EApplyLocation_name_by_value.clear();
	EApplyLocation_value_by_name.clear();

	EApplyLocation_name_by_value[EApplyLocation::APPLY_NONE] = "APPLY_NONE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_STR] = "APPLY_STR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DEX] = "APPLY_DEX";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_INT] = "APPLY_INT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_WIS] = "APPLY_WIS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CON] = "APPLY_CON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHA] = "APPLY_CHA";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CLASS] = "APPLY_CLASS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_LEVEL] = "APPLY_LEVEL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AGE] = "APPLY_AGE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHAR_WEIGHT] = "APPLY_CHAR_WEIGHT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHAR_HEIGHT] = "APPLY_CHAR_HEIGHT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MANAREG] = "APPLY_MANAREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HIT] = "APPLY_HIT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MOVE] = "APPLY_MOVE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_GOLD] = "APPLY_GOLD";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_EXP] = "APPLY_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AC] = "APPLY_AC";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HITROLL] = "APPLY_HITROLL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DAMROLL] = "APPLY_DAMROLL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_WILL] = "APPLY_SAVING_WILL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_FIRE] = "APPLY_RESIST_FIRE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_AIR] = "APPLY_RESIST_AIR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_DARK] = "APPLY_RESIST_DARK";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_CRITICAL] = "APPLY_SAVING_CRITICAL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_STABILITY] = "APPLY_SAVING_STABILITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HITREG] = "APPLY_HITREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MOVEREG] = "APPLY_MOVEREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C1] = "APPLY_C1";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C2] = "APPLY_C2";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C3] = "APPLY_C3";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C4] = "APPLY_C4";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C5] = "APPLY_C5";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C6] = "APPLY_C6";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C7] = "APPLY_C7";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C8] = "APPLY_C8";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C9] = "APPLY_C9";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SIZE] = "APPLY_SIZE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ARMOUR] = "APPLY_ARMOUR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_POISON] = "APPLY_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_REFLEX] = "APPLY_SAVING_REFLEX";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CAST_SUCCESS] = "APPLY_CAST_SUCCESS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MORALE] = "APPLY_MORALE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_INITIATIVE] = "APPLY_INITIATIVE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RELIGION] = "APPLY_RELIGION";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ABSORBE] = "APPLY_ABSORBE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_LIKES] = "APPLY_LIKES";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_WATER] = "APPLY_RESIST_WATER";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_EARTH] = "APPLY_RESIST_EARTH";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_VITALITY] = "APPLY_RESIST_VITALITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_MIND] = "APPLY_RESIST_MIND";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_IMMUNITY] = "APPLY_RESIST_IMMUNITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AR] = "APPLY_AR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MR] = "APPLY_MR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ACONITUM_POISON] = "APPLY_ACONITUM_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SCOPOLIA_POISON] = "APPLY_SCOPOLIA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BELENA_POISON] = "APPLY_BELENA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DATURA_POISON] = "APPLY_DATURA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HIT_GLORY] = "APPLY_HIT_GLORY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BONUS_EXP] = "APPLY_BONUS_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BONUS_SKILLS] = "APPLY_BONUS_SKILLS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PLAQUE] = "APPLY_PLAQUE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MADNESS] = "APPLY_MADNESS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PR] = "APPLY_PR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_DARK] = "APPLY_RESIST_DARK";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_VIEW_DT] = "APPLY_VIEW_DT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PERCENT_EXP] = "APPLY_PERCENT_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PERCENT_DAM] = "APPLY_PERCENT_DAM";	
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SPELL_BLINK] = "APPLY_SPELL_BLINK";
	EApplyLocation_name_by_value[EApplyLocation::NUM_APPLIES] = "NUM_APPLIES";
	for (const auto& i : EApplyLocation_name_by_value) {
		EApplyLocation_value_by_name[i.second] = i.first;
	}
}

template <>
EApplyLocation ITEM_BY_NAME(const std::string& name)
{
	if (EApplyLocation_name_by_value.empty())
	{
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_value_by_name.at(name);
}

template <>
const std::string& NAME_BY_ITEM(const EApplyLocation item)
{
	if (EApplyLocation_name_by_value.empty())
	{
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_name_by_value.at(item);
}

void DelegatedStringWriter::set_string(const char* string)
{
	const size_t l = strlen(string);
	if (nullptr == m_delegated_string)
	{
		CREATE(m_delegated_string, l + 1);
	}
	else
	{
		RECREATE(m_delegated_string, l + 1);
	}
	strcpy(m_delegated_string, string);
}

void DelegatedStringWriter::append_string(const char* string)
{
	const size_t l = length() + strlen(string);
	if (nullptr == m_delegated_string)
	{
		CREATE(m_delegated_string, l + 1);
		*m_delegated_string = '\0';
	}
	else
	{
		RECREATE(m_delegated_string, l + 1);
	}
	strcat(m_delegated_string, string);
}

void DelegatedStringWriter::clear()
{
	if (m_delegated_string)
	{
		free(m_delegated_string);
	}
	m_delegated_string = nullptr;
}

DESCRIPTOR_DATA::DESCRIPTOR_DATA() : bad_pws(0),
	idle_tics(0),
	connected(0),
	desc_num(0),
	input_time(0),
	login_time(0),
	showstr_head(0),
	showstr_vector(0),
	showstr_count(0),
	showstr_page(0),
	max_str(0),
	backstr(0),
	mail_to(0),
	has_prompt(0),
	output(0),
	history(0),
	history_pos(0),
	bufptr(0),
	bufspace(0),
	large_outbuf(0),
	character(0),
	original(0),
	snooping(0),
	snoop_by(0),
	next(0),
	olc(0),
	keytable(0),
	options(0),
	deflate(nullptr),
	mccp_version(0),
	ip(0),
	registered_email(0),
	pers_log(0),
	cur_vnum(0),
	old_vnum(0),
	snoop_with_map(0),
	m_msdp_support(false),
	m_msdp_last_max_hit(0),
	m_msdp_last_max_move(0)
{
	host[0] = 0;
	inbuf[0] = 0;
	last_input[0] = 0;
	small_outbuf[0] = 0;
}

void DESCRIPTOR_DATA::msdp_support(bool on)
{
	log("INFO: MSDP support enabled for client %s.\n", host);
	m_msdp_support = on;
}

void DESCRIPTOR_DATA::msdp_report(const std::string& name)
{
	if (m_msdp_support && msdp_need_report(name))
	{
		msdp::report(this, name);
	}
}

// Should be called periodically to update changing msdp variables.
// this is mostly to overcome complication of hunting every possible place affect are added/removed to/from char.
void DESCRIPTOR_DATA::msdp_report_changed_vars()
{
	if (!m_msdp_support || !character)
	{
		return;
	}

	if (m_msdp_last_max_hit != GET_REAL_MAX_HIT(character))
	{
		msdp_report(msdp::constants::MAX_HIT);
		m_msdp_last_max_hit = GET_REAL_MAX_HIT(character);
	}

	if (m_msdp_last_max_move != GET_REAL_MAX_MOVE(character))
	{
		msdp_report(msdp::constants::MAX_MOVE);
		m_msdp_last_max_move = GET_REAL_MAX_MOVE(character);
	}
}

void DESCRIPTOR_DATA::string_to_client_encoding(const char* input, char* output) const
{
	switch (keytable)
	{
	case KT_ALT:
		for (; *input; *output = KtoA(*input), input++, output++);
		break;

	case KT_WIN:
		for (; *input; input++, output++)
		{
			*output = KtoW(*input);

			// 0xFF is cp1251 'я' and Telnet IAC, so escape it with another IAC
			if (*output == '\xFF')
			{
				*++output = '\xFF';
			}
		}
		break;

	case KT_WINZ_OLD:
	case KT_WINZ_Z:
		// zMUD before 6.39 or after for backward compatibility  - replace я with z
		for (; *input; *output = KtoW2(*input), input++, output++);
		break;

	case KT_WINZ:
		// zMUD after 6.39 and CMUD support 'я' but with some issues
		for (; *input; input++, output++)
		{
			*output = KtoW(*input);

			// 0xFF is cp1251 'я' and Telnet IAC, so escape it with antother IAC
			// also there is a bug in zMUD, meaning we need to add an extra byte
			if (*output == '\xFF')
			{
				*++output = '\xFF';
				// make it obvious for other clients something is wrong
				*++output = '?';
			}
		}
		break;

	case KT_UTF8:
		// Anton Gorev (2016-04-25): we have to be careful. String in UTF-8 encoding may
		// contain character with code 0xff which telnet interprets as IAC.
		// II:  FE and FF were never defined for any purpose in UTF-8, we are safe
		koi_to_utf8(const_cast<char *>(input), output);
		break;

	default:
		for (; *input; *output = *input, input++, output++);
		break;
	}

	if (keytable != KT_UTF8)
	{
		*output = '\0';
	}
}

void EXTRA_DESCR_DATA::set_keyword(std::string const& value)
{
	if (keyword != nullptr)
		free(keyword);
	keyword = str_dup(value.c_str());
}

void EXTRA_DESCR_DATA::set_description(std::string const& value)
{
	if (description != nullptr)
		free(description);
	description = str_dup(value.c_str());
}

EXTRA_DESCR_DATA::~EXTRA_DESCR_DATA()
{
	if (nullptr != keyword)
	{
		free(keyword);
	}

	if (nullptr != description)
	{
		free(description);
	}

	// we don't take care of items in list. So, we don't do anything with the next field.
}

template <>
bool AFFECT_DATA<EApplyLocation>::removable() const
{
	return !spell_info[type].name
		|| *spell_info[type].name == '!'
		|| type == SPELL_SLEEP
		|| type == SPELL_POISON
		|| type == SPELL_WEAKNESS
		|| type == SPELL_CURSE
		|| type == SPELL_PLAQUE
		|| type == SPELL_SILENCE
		|| type == SPELL_POWER_SILENCE
		|| type == SPELL_BLINDNESS
		|| type == SPELL_POWER_BLINDNESS
		|| type == SPELL_HAEMORRAGIA
		|| type == SPELL_HOLD
		|| type == SPELL_POWER_HOLD
		|| type == SPELL_PEACEFUL
		|| type == SPELL_CONE_OF_COLD
		|| type == SPELL_DEAFNESS
		|| type == SPELL_POWER_DEAFNESS
		|| type == SPELL_BATTLE;
}

punish_data::punish_data() : duration(0), reason(nullptr), level(0), godid(0)
{
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
