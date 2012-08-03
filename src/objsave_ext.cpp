// Part of Bylins http://www.mud.ru

#include <list>
#include <map>
#include "objsave.h"
#include "house.h"

namespace ObjSaveSync
{

struct NodeType
{
	// уид чара/внум клан-ренты инициатора
	int init_uid;
	// флаг для уида инициатора
	int init_type;
	// уид чара/внум клан-ренты на сейв
	int targ_uid;
	// флаг для уида на сейв
	int targ_type;
};

std::list<NodeType> save_list;

void add(int init_uid, int targ_uid, int targ_type)
{
	NodeType node;
	node.init_uid = init_uid;
	node.init_type = CHAR_SAVE;
	node.targ_uid = targ_uid;
	node.targ_type = targ_type;
	save_list.push_front(node);
}

void write_file(int uid, int type)
{
	if (type == CHAR_SAVE)
	{
		for (CHAR_DATA *ch = character_list; ch; ch = ch->next)
		{
			if (ch->get_uid() == uid)
			{
				Crash_crashsave(ch);
				return;
			}
		}
	}
	else if (type == CLAN_SAVE)
	{
		for (ClanListType::const_iterator i = Clan::ClanList.begin(),
			iend = Clan::ClanList.end(); i != iend; ++i)
		{
			if ((*i)->GetRent() == uid)
			{
				(*i)->save_chest();
				return;
			}
		}
	}
}

void check(int uid)
{
	log("ObjSaveSync::check start");
	std::map<int /* uid */, int /* type */> tmp_list;

	for (std::list<NodeType>::iterator i = save_list.begin();
		i != save_list.end(); /* empty */)
	{
		if (i->init_uid == uid)
		{
			tmp_list[i->targ_uid] = i->targ_type;
			i = save_list.erase(i);
		}
		else if (i->targ_uid == uid)
		{
			tmp_list[i->init_uid] = i->init_type;
			i = save_list.erase(i);
		}
		else
		{
			++i;
		}
	}

	for(std::map<int, int>::const_iterator i = tmp_list.begin(),
		iend = tmp_list.end(); i != iend; ++i)
	{
		write_file(i->first, i->second);
	}
	log("ObjSaveSync::check end");
}

} // namespace ObjSaveSync
