// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include "corpse.hpp"
#include "db.h"
#include "xmlParser.h"
#include "utils.h"
#include "char.hpp"
#include "comm.h"
#include "handler.h"
#include "dg_scripts.h"
#include "im.h"

extern int max_npc_corpse_time, max_pc_corpse_time;
extern int magic_repair_dropped;
extern int magic_repair_chance;
extern MobRaceListType mobraces_list;

namespace GlobalDrop
{

struct global_drop
{
	int vnum; // внум шмотки
	int lvl;  // мин левел моба
	int exp;  // мин экспа за моба
	int prc;  // шансы дропа (1 к Х)
};

typedef std::vector<global_drop> DropListType;
DropListType drop_list;

const char *CONFIG_FILE = LIB_MISC"global_drop.xml";

void init()
{
	XMLResults result;
	XMLNode xMainNode=XMLNode::parseFile(CONFIG_FILE, "globaldrop", &result);
	if (result.error != eXMLErrorNone)
	{
		log("SYSERROR: Ошибка чтения файла %s: %s", CONFIG_FILE, XMLNode::getError(result.error));
		return;
	}

	int nodes = xMainNode.nChildNode("drop");
	for (int i = 0; i < nodes; i++)
	{
		global_drop drop_node;
		XMLNode xNodeRace = xMainNode.getChildNode("drop", i);
		drop_node.vnum = xmltoi(xNodeRace.getAttribute("obj_vnum"));
		drop_node.lvl = xmltoi(xNodeRace.getAttribute("mob_level"));
		drop_node.exp = xmltoi(xNodeRace.getAttribute("mob_exp"));
		drop_node.prc = xmltoi(xNodeRace.getAttribute("chance"));
		drop_list.push_back(drop_node);
	}
}

/**
* Глобальный дроп с мобов заданных параметров.
* TODO: если что-то еще добавится - выносить в конфиг.
*/
void check_mob(CHAR_DATA *mob)
{
	if (!IS_NPC(mob))
	{
		sprintf(buf, "SYSERROR: получили на входе персонажа. (%s %s %d)", __FILE__, __func__, __LINE__);
		mudlog(buf, DEF, LVL_GOD, SYSLOG, TRUE);
		return;
	}

	for (DropListType::iterator it = drop_list.begin(); it != drop_list.end(); ++it)
	{
		if (GET_LEVEL(mob) >= it->lvl && GET_EXP(mob) >= it->exp)
		{
			if (number(0, it->prc) == it->prc/2)
			{
				OBJ_DATA *obj = read_object(it->vnum, VIRTUAL);
				if (obj)
				{
					obj_to_char(obj, mob);
					++magic_repair_dropped;
				}
			}
			else
			{
				++magic_repair_chance;
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
	corpse->name = str_dup(buf2);

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
	GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
	GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
	GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch);
	GET_OBJ_RENT(corpse) = 100000;
	GET_OBJ_TIMER(corpse) = max_pc_corpse_time * 2;
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

OBJ_DATA *make_corpse(CHAR_DATA * ch)
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
	GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
	GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
	GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
	GET_OBJ_RENT(corpse) = 100000;
	if (IS_NPC(ch))
		GET_OBJ_TIMER(corpse) = max_npc_corpse_time * 2;
	else
		GET_OBJ_TIMER(corpse) = max_pc_corpse_time * 2;

	if (IS_NPC(ch))
	{
		GlobalDrop::check_mob(ch);
	}

	/* transfer character's equipment to the corpse */
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i))
		{
			remove_otrigger(GET_EQ(ch, i), ch);
			obj_to_char(unequip_char(ch, i), ch);
		}
	// Считаем вес шмоток после того как разденем чара
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);

	/* transfer character's inventory to the corpse */
	corpse->contains = ch->carrying;
	for (o = corpse->contains; o != NULL; o = o->next_content)
	{
		o->in_obj = corpse;
	}
	object_list_new_owner(corpse, NULL);


	/* transfer gold */
	if (get_gold(ch) > 0)  	/* following 'if' clause added to fix gold duplication loophole */
	{
		if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc))
		{
			money = create_money(get_gold(ch));
			obj_to_obj(money, corpse);
		}
		set_gold(ch, 0);
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
				ingr_to_load_list[2*j+1] = it->second->ingrlist[j].prob[GET_LEVEL(ch)];
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
	/*  if (IS_NPC (ch))
	    dl_load_obj (corpse, ch); */

	obj_to_room(corpse, IN_ROOM(ch));
	return corpse;
}
