// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include <fstream>
#include <map>
#include <vector>
#include "corpse.hpp"
#include "db.h"
#include "utils.h"
#include "char.hpp"
#include "comm.h"
#include "handler.h"
#include "dg_scripts.h"
#include "im.h"
#include "room.hpp"
#include "pugixml.hpp"
#include "modify.h"
#include "house.h"
#include "parse.hpp"
#include "obj.hpp"

extern int max_npc_corpse_time, max_pc_corpse_time;
extern MobRaceListType mobraces_list;
extern void obj_to_corpse(OBJ_DATA *corpse, CHAR_DATA *ch, int rnum, bool setload);

namespace GlobalDrop
{

typedef std::map<int /* vnum */, int /* rnum*/> OlistType;

struct global_drop
{
	global_drop() : vnum(0), mob_lvl(0), max_mob_lvl(0), prc(0), mobs(0), rnum(-1) {};
	int vnum; // внум шмотки, если число отрицательное - есть список внумов
	int mob_lvl;  // мин левел моба
	int max_mob_lvl; // макс. левел моба (0 - не учитывается)
	int prc;  // шансы дропа (каждые Х мобов)
	int mobs; // убито подходящих мобов
	int rnum; // рнум шмотки, если vnum валидный
	// список внумов с общим дропом (дропается первый возможный)
	// для внумов из списка учитывается поле максимума в мире
	OlistType olist;
};

typedef std::vector<global_drop> DropListType;
DropListType drop_list;

const char *CONFIG_FILE = LIB_MISC"global_drop.xml";
const char *STAT_FILE = LIB_PLRSTUFF"global_drop.tmp";

void init()
{
	// на случай релоада
	drop_list.clear();

	// конфиг
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node node_list = doc.child("globaldrop");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<globaldrop> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	for (pugi::xml_node node = node_list.child("drop"); node; node = node.next_sibling("drop"))
	{
		int obj_vnum = Parse::attr_int(node, "obj_vnum");
		int mob_lvl = Parse::attr_int(node, "mob_lvl");
		int max_mob_lvl = Parse::attr_int(node, "max_mob_lvl");
		int chance = Parse::attr_int(node, "chance");

		if (obj_vnum == -1 || mob_lvl <= 0 || chance <= 0 || max_mob_lvl < 0)
		{
			snprintf(buf, MAX_STRING_LENGTH,
					"...bad drop attributes (obj_vnum=%d, mob_lvl=%d, chance=%d, max_mob_lvl=%d)",
					obj_vnum, mob_lvl, chance, max_mob_lvl);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}

		global_drop tmp_node;
		tmp_node.vnum = obj_vnum;
		tmp_node.mob_lvl = mob_lvl;
		tmp_node.max_mob_lvl = max_mob_lvl;
		tmp_node.prc = chance;

		if (obj_vnum >= 0)
		{
			int obj_rnum = real_object(obj_vnum);
			if (obj_rnum < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH, "...incorrect obj_vnum=%d", obj_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
			tmp_node.rnum = obj_rnum;
		}
		else
		{
			// список шмоток с единым дропом
			for (pugi::xml_node item = node.child("obj"); item; item = item.next_sibling("obj"))
			{
				int item_vnum = Parse::attr_int(item, "vnum");
				if (item_vnum <= 0)
				{
					snprintf(buf, MAX_STRING_LENGTH,
						"...bad shop attributes (item_vnum=%d)", item_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					return;
				}
				// проверяем шмотку
				int item_rnum = real_object(item_vnum);
				if (item_rnum < 0)
				{
					snprintf(buf, MAX_STRING_LENGTH, "...incorrect item_vnum=%d", item_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					return;
				}
				tmp_node.olist[item_vnum] = item_rnum;
			}
			if (tmp_node.olist.empty())
			{
				snprintf(buf, MAX_STRING_LENGTH, "...item list empty (obj_vnum=%d)", obj_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
		}
		drop_list.push_back(tmp_node);
	}

	// сохраненные статы по убитым ранее мобам
	std::ifstream file(STAT_FILE);
	if (!file.is_open())
	{
		log("SYSERROR: не удалось открыть файл на чтение: %s", STAT_FILE);
		return;
	}
	int vnum, mobs;
	while (file >> vnum >> mobs)
	{
		for (DropListType::iterator i = drop_list.begin(); i != drop_list.end(); ++i)
		{
			if (i->vnum == vnum)
			{
				i->mobs = mobs;
			}
		}
	}
}

void save()
{
	std::ofstream file(STAT_FILE);
	if (!file.is_open())
	{
		log("SYSERROR: не удалось открыть файл на запись: %s", STAT_FILE);
		return;
	}
	for (DropListType::iterator i = drop_list.begin(); i != drop_list.end(); ++i)
	{
		file << i->vnum << " " << i->mobs << "\n";
	}
}

/**
 * Поиск шмотки для дропа из списка с учетом макс в мире.
 * \return rnum
 */
int get_obj_to_drop(DropListType::iterator &i)
{
	std::vector<int> tmp_list;
	for (OlistType::iterator k = i->olist.begin(), kend = i->olist.end(); k != kend; ++k)
	{
		if (obj_index[k->second].stored + obj_index[k->second].number < GET_OBJ_MIW(obj_proto[k->second]))
		{
			tmp_list.push_back(k->second);
		}
	}
	if (!tmp_list.empty())
	{
		int rnd = number(0, tmp_list.size() - 1);
		return tmp_list.at(rnd);
	}
	return -1;
}

/**
 * Глобальный дроп с мобов заданных параметров.
 * Если vnum отрицательный, то поиск идет по списку общего дропа.
 */
bool check_mob(OBJ_DATA *corpse, CHAR_DATA *ch)
{
	for (DropListType::iterator i = drop_list.begin(), iend = drop_list.end(); i != iend; ++i)
	{
		if (GET_LEVEL(ch) >= i->mob_lvl
			&& (!i->max_mob_lvl || GET_LEVEL(ch) <= i->max_mob_lvl))
		{
			++(i->mobs);
			if (i->mobs >= i->prc)
			{
				int obj_rnum = i->vnum > 0 ? i->rnum : get_obj_to_drop(i);
				if (obj_rnum >= 0)
				{
					obj_to_corpse(corpse, ch, obj_rnum, false);
				}
				i->mobs = 0;
				return true;
			}
		}
	}
	return false;
}

void renumber_obj_rnum(int rnum)
{
	for (DropListType::iterator i = drop_list.begin(); i != drop_list.end(); ++i)
	{
		if (i->vnum >= 0 && i->rnum >= rnum)
		{
			i->rnum += 1;
		}
		else if (i->vnum < 0)
		{
			for (OlistType::iterator k = i->olist.begin(), kend = i->olist.end();
				k != kend; ++k)
			{
				if (k->second >= rnum)
				{
					k->second += 1;
				}
			}
		}
	}
}

} // namespace GlobalDrop

void make_arena_corpse(CHAR_DATA * ch, CHAR_DATA * killer)
{
	OBJ_DATA *corpse;
	EXTRA_DESCR_DATA *exdesc;

	corpse = create_obj();
	GET_OBJ_SEX(corpse) = SEX_POLY;

	sprintf(buf2, "Останки %s лежат на земле.", GET_PAD(ch, 1));
	corpse->description = str_dup(buf2);

	sprintf(buf2, "останки %s", GET_PAD(ch, 1));
	corpse->short_description = str_dup(buf2);

	sprintf(buf2, "останки %s", GET_PAD(ch, 1));
	corpse->PNames[0] = str_dup(buf2);
	corpse->aliases = str_dup(buf2);

	sprintf(buf2, "останков %s", GET_PAD(ch, 1));
	corpse->PNames[1] = str_dup(buf2);
	sprintf(buf2, "останкам %s", GET_PAD(ch, 1));
	corpse->PNames[2] = str_dup(buf2);
	sprintf(buf2, "останки %s", GET_PAD(ch, 1));
	corpse->PNames[3] = str_dup(buf2);
	sprintf(buf2, "останками %s", GET_PAD(ch, 1));
	corpse->PNames[4] = str_dup(buf2);
	sprintf(buf2, "останках %s", GET_PAD(ch, 1));
	corpse->PNames[5] = str_dup(buf2);

	GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
	SET_BIT(GET_OBJ_EXTRA(corpse, ITEM_NODONATE), ITEM_NODONATE);
	SET_BIT(GET_OBJ_EXTRA(corpse, ITEM_NOSELL), ITEM_NOSELL);
	GET_OBJ_VAL(corpse, 0) = 0;	// You can't store stuff in a corpse
	GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
	GET_OBJ_VAL(corpse, 3) = 1;	// corpse identifier
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch);
	corpse->set_rent(100000);
	corpse->set_timer(max_pc_corpse_time * 2);
	CREATE(exdesc, EXTRA_DESCR_DATA, 1);
	exdesc->keyword = str_dup(corpse->PNames[0]);	// косметика
	if (killer)
		sprintf(buf, "Убит%s на арене %s.\r\n", GET_CH_SUF_6(ch), GET_PAD(killer, 4));
	else
		sprintf(buf, "Умер%s на арене.\r\n", GET_CH_SUF_4(ch));
	exdesc->description = str_dup(buf);	// косметика
	exdesc->next = corpse->ex_description;
	corpse->ex_description = exdesc;
	obj_to_room(corpse, IN_ROOM(ch));
}

OBJ_DATA *make_corpse(CHAR_DATA * ch, CHAR_DATA * killer)
{
	OBJ_DATA *corpse, *o;
	OBJ_DATA *money;
	int i;

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_CORPSE))
		return NULL;

	sprintf(buf2, "труп %s", GET_PAD(ch, 1));
	corpse = create_obj(buf2);

	GET_OBJ_SEX(corpse) = SEX_MALE;

	sprintf(buf2, "Труп %s лежит здесь.", GET_PAD(ch, 1));
	corpse->description = str_dup(buf2);

	sprintf(buf2, "труп %s", GET_PAD(ch, 1));
	corpse->short_description = str_dup(buf2);

	sprintf(buf2, "труп %s", GET_PAD(ch, 1));
	corpse->PNames[0] = str_dup(buf2);
	sprintf(buf2, "трупа %s", GET_PAD(ch, 1));
	corpse->PNames[1] = str_dup(buf2);
	sprintf(buf2, "трупу %s", GET_PAD(ch, 1));
	corpse->PNames[2] = str_dup(buf2);
	sprintf(buf2, "труп %s", GET_PAD(ch, 1));
	corpse->PNames[3] = str_dup(buf2);
	sprintf(buf2, "трупом %s", GET_PAD(ch, 1));
	corpse->PNames[4] = str_dup(buf2);
	sprintf(buf2, "трупе %s", GET_PAD(ch, 1));
	corpse->PNames[5] = str_dup(buf2);

	GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
	SET_BIT(GET_OBJ_EXTRA(corpse, ITEM_NODONATE), ITEM_NODONATE);
	SET_BIT(GET_OBJ_EXTRA(corpse, ITEM_NOSELL), ITEM_NOSELL);
	GET_OBJ_VAL(corpse, 0) = 0;	// You can't store stuff in a corpse
	GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
	GET_OBJ_VAL(corpse, 3) = 1;	// corpse identifier
	corpse->set_rent(100000);

	if (IS_NPC(ch))
	{
		corpse->set_timer(max_npc_corpse_time * 2);
	}
	else
	{
		corpse->set_timer(max_pc_corpse_time * 2);
	}

	// transfer character's equipment to the corpse
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			remove_otrigger(GET_EQ(ch, i), ch);
			if (GET_EQ(ch, i)->purged()) continue;

			obj_to_char(unequip_char(ch, i), ch);
		}
	}

	// Считаем вес шмоток после того как разденем чара
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);

	// transfer character's inventory to the corpse
	corpse->contains = ch->carrying;
	for (o = corpse->contains; o != NULL; o = o->next_content)
	{
		o->in_obj = corpse;
	}
	object_list_new_owner(corpse, NULL);


	// transfer gold
	// following 'if' clause added to fix gold duplication loophole
	if (ch->get_gold() > 0)
	{
		if (IS_NPC(ch))
		{
			money = create_money(ch->get_gold());
			obj_to_obj(money, corpse);
		}
		else
		{
			const int amount = ch->get_gold();
			money = create_money(amount);
			OBJ_DATA *purse = 0;
			if (amount >= 100)
			{
				purse = system_obj::create_purse(ch, amount);
				if (purse)
				{
					obj_to_obj(money, purse);
					obj_to_obj(purse, corpse);
				}
			}
			if (!purse)
			{
				obj_to_obj(money, corpse);
			}
		}
		ch->set_gold(0);
	}

	ch->carrying = NULL;
	IS_CARRYING_N(ch) = 0;
	IS_CARRYING_W(ch) = 0;

//Polud привязываем загрузку ингров к расе (типу) моба

	if (IS_NPC(ch) && GET_RACE(ch)>NPC_RACE_BASIC && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
	{
		MobRaceListType::iterator it = mobraces_list.find(GET_RACE(ch));
		if (it != mobraces_list.end())
		{
			int *ingr_to_load_list, j;
			int num_inrgs = it->second->ingrlist.size();
			CREATE(ingr_to_load_list, int, num_inrgs * 2 + 1);
			for (j=0; j < num_inrgs; j++)
			{
				ingr_to_load_list[2*j] = im_get_idx_by_type(it->second->ingrlist[j].imtype);
				ingr_to_load_list[2*j+1] = it->second->ingrlist[j].prob.at(GET_LEVEL(ch)-1);
				ingr_to_load_list[2*j+1] |= (GET_LEVEL(ch) << 16);
			}
			ingr_to_load_list[2*j] = -1;
			im_make_corpse(corpse, ingr_to_load_list, 1000);
		}
		else
			if (mob_proto[GET_MOB_RNUM(ch)].ing_list)
				im_make_corpse(corpse, mob_proto[GET_MOB_RNUM(ch)].ing_list, 100);
	}

	// Загружаю шмотки по листу. - перемещено в raw_kill
	//if (IS_NPC (ch))
	//	dl_load_obj (corpse, ch);

	// если чармис убит палачом или на арене(и владелец не в бд) то труп попадает не в клетку а в инвентарь к владельцу чармиса
	if(IS_CHARMICE(ch) && !MOB_FLAGGED(ch, MOB_CORPSE) && ch->master && ((killer && PRF_FLAGGED(killer, PRF_EXECUTOR))
		|| (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) && !RENTABLE(ch->master))))
	{
		obj_to_char(corpse, ch->master);
		return NULL;
	}
	else
	{
		obj_to_room(corpse, IN_ROOM(ch));
		return corpse;
	}
}
