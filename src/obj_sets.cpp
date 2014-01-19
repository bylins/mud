// Copyright (c) 2014 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <string>
#include <vector>
#include <map>
#include <array>
#include <algorithm>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
// GCC 4.4
#include <boost/tr1/unordered_map.hpp>

#include "obj_sets.hpp"
#include "obj_sets_stuff.hpp"
#include "structs.h"
#include "obj.hpp"
#include "db.h"
#include "pugixml.hpp"
#include "parse.hpp"
#include "constants.h"
#include "handler.h"
#include "char_player.hpp"
#include "skills.h"
#include "screen.h"
#include "modify.h"
#include "spells.h"
#include "help.hpp"

extern obj_rnum top_of_objt;

namespace obj_sets
{

int set_node::uid_cnt = 0;

const char *OBJ_SETS_FILE = LIB_MISC"obj_sets.xml";
/// мин/макс кол-во активаторов для валидного сета
const unsigned MIN_ACTIVE_SIZE = 2;
/// вобщем-то равняется кол-ву допустимых для сетов слотов
const unsigned MAX_ACTIVE_SIZE = 10;
/// сколько предметов может быть в сете
const unsigned MAX_OBJ_LIST = 20;
/// основной список сетов
std::vector<boost::shared_ptr<set_node>> sets_list;
/// дефолтные сообщения всех сетов, инятся в init_global_msg()
msg_node global_msg;

/// форматирование строки с разбиением на строки не длиннее \param len,
/// строка разбивается полными словами по разделителю \param sep
/// \param base_offset = 0 - смещение первой строки
std::string line_split_str(const std::string &str, const std::string &sep,
	size_t len, size_t base_offset)
{
	std::string out;
	out.reserve(str.size());
	const size_t sep_len = sep.length();
	size_t idx = 0, cur_line = base_offset, prev_idx = 0;

	while ((idx = str.find(sep, idx)) != std::string::npos)
	{
		const size_t tok_len = idx + sep_len - prev_idx;
		if (cur_line + tok_len > len)
		{
			out += "\r\n";
			cur_line = 0;
		}
		out += str.substr(prev_idx, tok_len);
		cur_line += tok_len;
		idx += sep_len;
		prev_idx = idx;
	}
	out += str.substr(prev_idx);

	return out;
}

/// \return индекс сета через внум любого его предмета
size_t setidx_by_objvnum(int vnum)
{
	for (size_t i = 0; i < sets_list.size(); ++i)
	{
		if (sets_list.at(i)->obj_list.find(vnum) !=
			sets_list.at(i)->obj_list.end())
		{
			return i;
		}
	}
	return -1;
}

/// \return индекс сета через его уид (олц)
size_t setidx_by_uid(int uid)
{
	for (size_t i = 0; i < sets_list.size(); ++i)
	{
		if (sets_list.at(i)->uid == uid)
		{
			return i;
		}
	}
	return -1;
}

/// проверка предмета на наличие в других сетах
/// \param set_uid - чтобы не считать свой же сет за дубль
bool is_duplicate(int set_uid, int vnum)
{
	for (size_t i = 0; i < sets_list.size(); ++i)
	{
		if (sets_list.at(i)->uid != set_uid
			&& sets_list.at(i)->obj_list.find(vnum)
				!= sets_list.at(i)->obj_list.end())
		{
			return true;
		}
	}
	return false;
}

/// запись индексов сетов в список индексов предметов (не в OBJ_DATA)
/// здесь же обновляется справка по активаторам сетов
void init_obj_index()
{
	// obj vnum, obj_index idx
	// GCC 4.4
	//std::unordered_map<int, int> tmp;
	boost::unordered_map<int, int> tmp;
	tmp.reserve(top_of_objt + 1);

	for (int i = 0; i <= top_of_objt; ++i)
	{
		obj_index[i].set_idx = -1;
		tmp.emplace(obj_index[i].vnum, i);
	}

	for (size_t i = 0; i < sets_list.size(); ++i)
	{
		for (auto k = sets_list.at(i)->obj_list.begin();
			k != sets_list.at(i)->obj_list.end(); ++k)
		{
			auto m = tmp.find(k->first);
			if (m != tmp.end())
			{
				obj_index[m->second].set_idx = i;
			}
		}
	}
	HelpSystem::reload(HelpSystem::STATIC);
}

/// сеты не вешаются на: кольца, ожерелья, браслеты, свет
bool verify_wear_flag(OBJ_DATA *obj)
{
	if (CAN_WEAR(obj, ITEM_WEAR_FINGER)
		|| CAN_WEAR(obj, ITEM_WEAR_NECK)
		|| CAN_WEAR(obj, ITEM_WEAR_WRIST)
		|| GET_OBJ_TYPE(obj) == ITEM_LIGHT)
	{
		return false;
	}
	return true;
}

/// проверка сета после олц или лоада из конфига, при необходимости
/// помечает сет как неактивный (сам сет или другие его поля не трогаются)
void verify_set(set_node &set)
{
	const size_t num = setidx_by_uid(set.uid) + 1;

	if (set.obj_list.size() < 2 || set.activ_list.size() < 1)
	{
		err_log("сет #%zu: incomplete (objs=%zu, activs=%zu)",
			num, set.obj_list.size(), set.activ_list.size());
		set.enabled = false;
	}
	if (set.obj_list.size() > MAX_OBJ_LIST)
	{
		err_log("сет #%zu: too many objects (size=%zu)",
			num, set.obj_list.size());
		set.enabled = false;
	}

	for (auto i = set.obj_list.begin(); i != set.obj_list.end(); ++i)
	{
		const int rnum = real_object(i->first);
		if (rnum < 0)
		{
			err_log("сет #%zu: empty obj proto (vnum=%d)", num, i->first);
			set.enabled = false;
		}
		else if (OBJ_FLAGGED(obj_proto[rnum], ITEM_SETSTUFF))
		{
			err_log("сет #%zu: obj have ITEM_SETSTUFF flag (vnum=%d)",
				num, i->first);
			set.enabled = false;
		}
		else if (!verify_wear_flag(obj_proto[rnum]))
		{
			err_log("сет #%zu: obj have invalid wear flag (vnum=%d)",
				num, i->first);
			set.enabled = false;
		}
		if (is_duplicate(set.uid, i->first))
		{
			err_log("сет #%zu: dublicate obj (vnum=%d)", num, i->first);
			set.enabled = false;
		}
	}

	std::bitset<NUM_CLASSES> prof_bits;
	bool prof_restrict = false;
	for (auto i = set.activ_list.begin(); i != set.activ_list.end(); ++i)
	{
		if (i->first < MIN_ACTIVE_SIZE || i->first > MAX_ACTIVE_SIZE)
		{
			err_log("сет #%zu: некорректный размер активатора (activ=%d)",
				num, i->first);
			set.enabled = false;
		}
		if (i->first > set.obj_list.size())
		{
			err_log("сет #%zu: активатор больше списка предметов (activ=%d)",
				num, i->first);
			set.enabled = false;
		}
		for (auto k = i->second.apply.begin(); k != i->second.apply.end(); ++k)
		{
			if (k->location < 0 || k->location >= NUM_APPLIES)
			{
				err_log(
					"сет #%zu: некорректный номер apply аффекта (loc=%d, mod=%d, activ=%d)",
					num, k->location, k->modifier, i->first);
				set.enabled = false;
			}
		}
		// можно просетить скилл в минус
		if (i->second.skill.num > MAX_SKILL_NUM
			|| i->second.skill.num < 0
			|| i->second.skill.val > 200
			|| i->second.skill.val < -200)
		{
			err_log(
				"сет #%zu: некорректные номер или значение умения (num=%d, val=%d, activ=%d)",
				num, i->second.skill.num, i->first);
			set.enabled = false;
		}
		if (i->second.prof.none())
		{
			err_log("сет #%zu: пустой список профессий активатора (activ=%d)",
				num, i->first);
			set.enabled = false;
		}
		if (i->second.empty())
		{
			err_log("сет #%zu: пустой активатор (activ=%d)", num, i->first);
			set.enabled = false;
		}
		if (i->second.prof.count() != i->second.prof.size())
		{
			if (!prof_restrict)
			{
				prof_bits = i->second.prof;
				prof_restrict = true;
			}
			else if (i->second.prof != prof_bits)
			{
				err_log(
					"сет #%zu: несовпадение ограниченного списка профессий активатора (activ=%d)",
					num, i->first);
				set.enabled = false;
			}
		}
	}
}

/// помимо уровня глобальных сообщений из конфига - есть вариант хардкодом
void init_global_msg()
{
	if (global_msg.char_on_msg.empty())
	{
		global_msg.char_on_msg = "&W$o0 засветил$U мягким сиянием.&n";
	}
	if (global_msg.char_off_msg.empty())
	{
		global_msg.char_off_msg = "&WСияние $o1 медленно угасло.&n";
	}
	if (global_msg.room_on_msg.empty())
	{
		global_msg.room_on_msg = "&W$o0 $n1 засветил$U мягким сиянием.&n";
	}
	if (global_msg.room_off_msg.empty())
	{
		global_msg.room_off_msg = "&WСияние $o1 $n1 медленно угасло.&n";
	}
}

/// инит структуры сообщений из конфига
/// отсутствие какой-то из строк (всех) не является ошибкой
void init_msg_node(msg_node &node, const pugi::xml_node &xml_msg)
{
	node.char_on_msg = xml_msg.child_value("char_on_msg");
	node.char_off_msg = xml_msg.child_value("char_off_msg");
	node.room_on_msg = xml_msg.child_value("room_on_msg");
	node.room_off_msg = xml_msg.child_value("room_off_msg");
}

/// лоад при старте мада, релоад через 'reload obj_sets.xml'
void load()
{
	log("Loadind %s: start", OBJ_SETS_FILE);
	sets_list.clear();
	init_global_msg();

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(OBJ_SETS_FILE);
	if (!result)
	{
		err_log("%s", result.description());
		return;
	}
    pugi::xml_node xml_obj_sets = doc.child("obj_sets");
    if (!xml_obj_sets)
    {
		err_log("<obj_sets> read fail");
		return;
    }
	// <messages>
	pugi::xml_node xml_msg = xml_obj_sets.child("messages");
	if (xml_msg)
	{
		init_msg_node(global_msg, xml_msg);
	}
	// <set>
	for (pugi::xml_node xml_set = xml_obj_sets.child("set"); xml_set;
		xml_set = xml_set.next_sibling("set"))
	{
		boost::shared_ptr<set_node> tmp_set = boost::make_shared<set_node>();
		// имя, алиас и камент не обязательны
		tmp_set->name = xml_set.attribute("name").value();
		tmp_set->alias = xml_set.attribute("alias").value();
		tmp_set->comment = xml_set.attribute("comment").value();
		// enabled не обязателен, по дефолту сет включен
		tmp_set->enabled =
			(xml_set.attribute("enabled").as_int(1) == 1 ? true : false);
		// <obj>
		for (pugi::xml_node xml_obj = xml_set.child("obj"); xml_obj;
			xml_obj = xml_obj.next_sibling("obj"))
		{
			// <messages>
			struct msg_node tmp_msg;
			pugi::xml_node xml_msg = xml_obj.child("messages");
			if (xml_msg)
			{
				init_msg_node(tmp_msg, xml_msg);
			}
			// GCC 4.4
			//tmp_set->obj_list.emplace(Parse::attr_int(xml_obj, "vnum"), tmp_msg);
			tmp_set->obj_list.insert(std::make_pair(Parse::attr_int(xml_obj, "vnum"), tmp_msg));
		}
		// <activ>
		for (pugi::xml_node xml_activ = xml_set.child("activ"); xml_activ;
			xml_activ = xml_activ.next_sibling("activ"))
		{
			activ_node tmp_activ;
			// <affects>
			asciiflag_conv(xml_activ.child_value("affects"), &tmp_activ.affects);
			// <apply>
			for (pugi::xml_node xml_apply = xml_activ.child("apply"); xml_apply;
				xml_apply = xml_apply.next_sibling("apply"))
			{
				// заполняются только первые MAX_OBJ_AFFECT
				for (auto i = tmp_activ.apply.begin();
					i != tmp_activ.apply.end(); ++i)
				{
					if (i->location <= 0)
					{
						i->location = Parse::attr_int(xml_apply, "loc");
						i->modifier = Parse::attr_int(xml_apply, "mod");
						break;
					}
				}
			}
			// <skill>
			pugi::xml_node xml_skill = xml_activ.child("skill");
			if (xml_skill)
			{
				tmp_activ.skill.num = Parse::attr_int(xml_skill, "num");
				tmp_activ.skill.val = Parse::attr_int(xml_skill, "val");
			}
			// если нет атрибута prof - значит актив на все профы
			pugi::xml_attribute xml_prof = xml_activ.attribute("prof");
			if (xml_prof)
			{
				std::bitset<NUM_CLASSES> tmp_p(std::string(xml_prof.value()));
				tmp_activ.prof = tmp_p;
			}
			tmp_set->activ_list[Parse::attr_int(xml_activ, "size")] = tmp_activ;
		}
		// <messages>
		pugi::xml_node xml_msg = xml_set.child("messages");
		if (xml_msg)
		{
			init_msg_node(tmp_set->messages, xml_msg);
		}
		sets_list.push_back(tmp_set);
		verify_set(*tmp_set);
	}

	init_obj_index();
	log("Loadind %s: done", OBJ_SETS_FILE);
	save();
}

/// сохранения структуры сообщений в конфиг
void save_messages(pugi::xml_node &xml, msg_node &msg)
{
	if (!msg.char_on_msg.empty()
		|| !msg.char_off_msg.empty()
		|| !msg.room_on_msg.empty()
		|| !msg.room_off_msg.empty())
	{
		xml.append_child("messages");
	}
	else
	{
		return;
	}

	pugi::xml_node xml_messages = xml.last_child();
	if (!msg.char_on_msg.empty())
	{
		pugi::xml_node xml_msg = xml_messages.append_child("char_on_msg");
		xml_msg.append_child(pugi::node_pcdata).set_value(msg.char_on_msg.c_str());
	}
	if (!msg.char_off_msg.empty())
	{
		pugi::xml_node xml_msg = xml_messages.append_child("char_off_msg");
		xml_msg.append_child(pugi::node_pcdata).set_value(msg.char_off_msg.c_str());
	}
	if (!msg.room_on_msg.empty())
	{
		pugi::xml_node xml_msg = xml_messages.append_child("room_on_msg");
		xml_msg.append_child(pugi::node_pcdata).set_value(msg.room_on_msg.c_str());
	}
	if (!msg.room_off_msg.empty())
	{
		pugi::xml_node xml_msg = xml_messages.append_child("room_off_msg");
		xml_msg.append_child(pugi::node_pcdata).set_value(msg.room_off_msg.c_str());
	}
}

/// сохранение конфига, пишутся и выключенные, и пустые сеты
void save()
{
	log("Saving %s: start", OBJ_SETS_FILE);
	char buf_[256];
	pugi::xml_document doc;
	pugi::xml_node xml_obj_sets = doc.append_child("obj_sets");
	// obj_sets/messages
	save_messages(xml_obj_sets, global_msg);

	for (auto i = sets_list.begin(); i != sets_list.end(); ++i)
	{
		// obj_sets/set
		pugi::xml_node xml_set = xml_obj_sets.append_child("set");
		if (!(*i)->name.empty())
		{
			xml_set.append_attribute("name") = (*i)->name.c_str();
		}
		if (!(*i)->alias.empty())
		{
			xml_set.append_attribute("alias") = (*i)->alias.c_str();
		}
		if (!(*i)->comment.empty())
		{
			xml_set.append_attribute("comment") = (*i)->comment.c_str();
		}
		if (!(*i)->enabled)
		{
			xml_set.append_attribute("enabled") = 0;
		}
		// set/messages
		save_messages(xml_set, (*i)->messages);
		// set/obj
		for (auto o = (*i)->obj_list.begin(); o != (*i)->obj_list.end(); ++o)
		{
			pugi::xml_node xml_obj = xml_set.append_child("obj");
			xml_obj.append_attribute("vnum") = o->first;
			save_messages(xml_obj, o->second);
		}
		// set/activ
		for (auto k = (*i)->activ_list.begin(); k != (*i)->activ_list.end(); ++k)
		{
			pugi::xml_node xml_activ = xml_set.append_child("activ");
			xml_activ.append_attribute("size") = k->first;
			if (k->second.prof.count() != k->second.prof.size()) // !all()
			{
				xml_activ.append_attribute("prof")
					= k->second.prof.to_string().c_str();
			}
			// set/activ/affects
			if (!k->second.affects.empty())
			{
				pugi::xml_node xml_affects = xml_activ.append_child("affects");
				*buf_ = '\0';
				tascii(&GET_FLAG(k->second.affects, 0), 4, buf_);
				xml_affects.append_child(pugi::node_pcdata).set_value(buf_);
			}
			// set/activ/apply
			for (auto m = k->second.apply.begin(); m != k->second.apply.end(); ++m)
			{
				if (m->location > 0 && m->location < NUM_APPLIES && m->modifier)
				{
					pugi::xml_node xml_apply = xml_activ.append_child("apply");
					xml_apply.append_attribute("loc") = m->location;
					xml_apply.append_attribute("mod") = m->modifier;
				}
			}
			// set/activ/skill
			if (k->second.skill.num > 0)
			{
				pugi::xml_node xml_skill = xml_activ.append_child("skill");
				xml_skill.append_attribute("num") = k->second.skill.num;
				xml_skill.append_attribute("val") = k->second.skill.val;
			}
		}
	}

	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "koi8-r";
	doc.save_file(OBJ_SETS_FILE);
	log("Saving %s: done", OBJ_SETS_FILE);
}

/// активатор сетится аналогично взятию аффектов со шмоток
void apply_activator(CHAR_DATA *ch, const activ_node &activ)
{
	for (auto i = activ.apply.begin(); i != activ.apply.end(); ++i)
	{
		affect_modify(ch, i->location, i->modifier, 0, TRUE);
	}
	for (int j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
	{
		if (weapon_affect[j].aff_bitvector != 0
			&& IS_SET(GET_FLAG(activ.affects, weapon_affect[j].aff_pos), weapon_affect[j].aff_pos))
		{
			affect_modify(ch, APPLY_NONE, 0, weapon_affect[j].aff_bitvector, TRUE);
		}
	}
	if (activ.skill.num > 0 && activ.skill.val > 0)
	{
		ch->add_obj_skill(activ.skill.num, activ.skill.val);
	}
}

/// распечатка сообщения чару и в комнату
/// \param activated - печатать активатор или деактиватор
/// сообщение берется по первому найденному в цепочке:
/// предмет -> набор -> глобальные сообщения
void print_msg(CHAR_DATA *ch, OBJ_DATA *obj, size_t set_idx, bool activated)
{
	if (set_idx >= sets_list.size()) return;

	const char *char_on_msg = 0;
	const char *room_on_msg = 0;
	const char *char_off_msg = 0;
	const char *room_off_msg = 0;

	boost::shared_ptr<set_node> &curr_set = sets_list.at(set_idx);
	auto i = curr_set->obj_list.find(GET_OBJ_VNUM(obj));
	if (i != curr_set->obj_list.end())
	{
		// сообщения у предмета
		if (!i->second.char_on_msg.empty())
			char_on_msg = i->second.char_on_msg.c_str();
		if (!i->second.room_on_msg.empty())
			room_on_msg = i->second.room_on_msg.c_str();
		if (!i->second.char_off_msg.empty())
			char_off_msg = i->second.char_off_msg.c_str();
		if (!i->second.room_off_msg.empty())
			room_off_msg = i->second.room_off_msg.c_str();
	}
	// сообщения у сета
	if (!char_on_msg && !curr_set->messages.char_on_msg.empty())
		char_on_msg = curr_set->messages.char_on_msg.c_str();
	if (!room_on_msg && !curr_set->messages.room_on_msg.empty())
		room_on_msg = curr_set->messages.room_on_msg.c_str();
	if (!char_off_msg && !curr_set->messages.char_off_msg.empty())
		char_off_msg = curr_set->messages.char_off_msg.c_str();
	if (!room_off_msg && !curr_set->messages.room_off_msg.empty())
		room_off_msg = curr_set->messages.room_off_msg.c_str();
	// глобальные сообщения
	if (!char_on_msg)
		char_on_msg = global_msg.char_on_msg.c_str();
	if (!room_on_msg)
		room_on_msg = global_msg.room_on_msg.c_str();
	if (!char_off_msg)
		char_off_msg = global_msg.char_off_msg.c_str();
	if (!room_off_msg)
		room_off_msg = global_msg.room_off_msg.c_str();
	// что-то в любом случае распечатаем
	if (activated)
	{
		act(char_on_msg, FALSE, ch, obj, 0, TO_CHAR);
		act(room_on_msg, TRUE, ch, obj, 0, TO_ROOM);
	}
	else
	{
		act(char_off_msg, FALSE, ch, obj, 0, TO_CHAR);
		act(room_off_msg, TRUE, ch, obj, 0, TO_ROOM);
	}
}

/// сообщение деактивации предмета
void print_off_msg(CHAR_DATA *ch, OBJ_DATA *obj)
{
	const int set_idx = GET_OBJ_RNUM(obj) >= 0
		? obj_index[GET_OBJ_RNUM(obj)].set_idx : -1;
	if (set_idx >= 0)
	{
		obj_sets::print_msg(ch, obj, set_idx, false);
	}
}

/// предмет может быть не активирован, но на чаре висит активатор
/// уже работающий от других предметов данного сета, т.е. например надето
/// 5 предметов, а работает сетовый активатор от 4 предметов, один остается
/// про запас, чтобы в случае снятия одного предмета не спамить активом
/// подменившего его предмета, а просто тихо продолжить учитывать активатор
/// т.е. после прохода в affect_total() у всех сетовых шмоток в get_activator()
/// содержится не только инфа о том, в активе она или нет, но и размер
/// максимального работающего в данный момент активатора с ее сета
void check_activated(CHAR_DATA *ch, int activ, idx_node &node)
{
	int need_msg = activ - node.activated_cnt;
	for (auto i = node.obj_list.begin(); i != node.obj_list.end(); ++i)
	{
		OBJ_DATA *obj = *i;
		if (need_msg > 0 && !obj->get_activator().first)
		{
			if (obj->get_activator().second < activ)
			{
				print_msg(ch, obj, node.set_idx, true);
			}
			obj->set_activator(true, activ);
			++node.activated_cnt;
			--need_msg;
		}
		else
		{
			obj->set_activator(obj->get_activator().first,
				std::max(activ, obj->get_activator().second));
		}
	}
}

/// дергается после проверок активаторов сета, чтобы учесть шмотки, которые
/// перестали работать и распечатать их сообщения о деактивации
void check_deactivated(CHAR_DATA *ch, int max_activ, idx_node &node)
{
	int need_msg = node.activated_cnt - max_activ;
	if (need_msg > 0)
	{
		for (auto i = node.obj_list.begin();
			i != node.obj_list.end() && need_msg > 0; ++i)
		{
			OBJ_DATA *obj = *i;
			if (obj->get_activator().first)
			{
				print_msg(ch, obj, node.set_idx, false);
				obj->set_activator(false, max_activ);
				--node.activated_cnt;
				--need_msg;
			}
		}
	}
}

/// распечатка предметов набора в два столбца со своим форматирование в
/// каждом из них, для экономии места на экране при опознании
std::string print_obj_list(const set_node &set)
{
	char buf_[128];
	std::string out;
	std::vector<int> rnum_list;
	size_t r_max_name = 0, l_max_name = 0;
	bool left = true;

	for (auto i = set.obj_list.begin(); i != set.obj_list.end(); ++i)
	{
		const int rnum = real_object(i->first);
		if (rnum < 0 || !obj_proto[rnum]->short_description) continue;

		const size_t curr_name =
			strlen_no_colors(obj_proto[rnum]->short_description);
		if (left)
		{
			l_max_name = std::max(l_max_name, curr_name);
		}
		else
		{
			r_max_name = std::max(r_max_name, curr_name);
		}
		rnum_list.push_back(rnum);
		left = !left;
	}

	left = true;
	for (auto i = rnum_list.begin(); i != rnum_list.end(); ++i)
	{
		snprintf(buf_, sizeof(buf_), "   %s",
			colored_name(obj_proto[*i]->short_description,
			left ? l_max_name : r_max_name, true));
		out += buf_;
		if (!left)
		{
			out += "\r\n";
		}
		left = !left;
	}

	boost::trim_right(out);
	return out + "\r\n";
}

/// опознание сетового предмета
void print_identify(CHAR_DATA *ch, const OBJ_DATA *obj)
{
	const size_t set_idx = GET_OBJ_RNUM(obj) >= 0
		? obj_index[GET_OBJ_RNUM(obj)].set_idx : sets_list.size();
	if (set_idx < sets_list.size())
	{
		const set_node &cur_set = *(sets_list.at(set_idx));
		if (!cur_set.enabled) return;

		std::string out;
		char buf_[256], buf_2[128];

		snprintf(buf_, sizeof(buf_), "%sЧасть набора предметов: %s%s%s\r\n",
			CCNRM(ch, C_NRM), CCWHT(ch, C_NRM),
			cur_set.name.c_str(), CCNRM(ch, C_NRM));
		out += buf_;
		out += print_obj_list(cur_set);

		auto i = obj->get_activator();
		if (i.second > 0)
		{
			snprintf(buf_2, sizeof(buf_2), " (активно %d %s)",
				i.second, desc_count(i.second, WHAT_OBJECT));
		}

		snprintf(buf_, sizeof(buf_),
			"Свойства набора%s: %sсправка активсет%d%s\r\n",
			(i.second > 0 ? buf_2 : ""), CCWHT(ch, C_NRM),
			set_idx + 1, CCNRM(ch, C_NRM));
		out += buf_;

		send_to_char(out, ch);
	}
}

/// список сетов имму с распечаткой номеров, по которым дергается sed и справка
void do_slist(CHAR_DATA *ch)
{
	std::string out("Зарегистрированные наборы предметов:\r\n");
	char buf_[256];

	int idx = 1;
	for (auto i = sets_list.begin(); i != sets_list.end(); ++i)
	{
		char comment[128];
		snprintf(comment, sizeof(comment), " (%s)", (*i)->comment.c_str());
		char status[64];
		snprintf(status, sizeof(status), "Статус: %sотключен%s, ",
			CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));

		snprintf(buf_, sizeof(buf_),
			"%3d) %s%s\r\n"
			"     %sПредметы: ", idx++,
			((*i)->name.empty() ? "Безымянный набор" : (*i)->name.c_str()),
			((*i)->comment.empty() ? "" : comment),
			((*i)->enabled ? "" : status));
		out += buf_;
		for (auto o = (*i)->obj_list.begin(); o !=  (*i)->obj_list.end(); ++o)
		{
			out += boost::lexical_cast<std::string>(o->first) + " ";
		}
		out += "\r\n";
	}
	page_string(ch->desc, out);
}

/// распечатка аффектов активатора для справки с форматирование по 80 символов
std::string print_affects_help(const FLAG_DATA &aff)
{
	char buf_[2048];
	if (sprintbits(aff, weapon_affects, buf_, ","))
	{
		// весь этот изврат, чтобы вывести аффекты с разбивкой на строки
		// по 80 символов (не разбивая слова), при этом подписать впереди
		// каждой строки " + " и выделить сами аффекты цветом
		std::string aff_str(" + Аффекты :\r\n");
		aff_str += line_split_str(buf_, ",", 74, 0);
		boost::trim_right(aff_str);
		char filler[64];
		snprintf(filler, sizeof(filler), "\n%s +    %s", KNRM, KCYN);
		boost::replace_all(aff_str, "\n", filler);
		aff_str += KNRM;
		aff_str += "\r\n";

		return aff_str;
	}
	return "";
}

/// для распечатки apply аффектов, которые могут быть как в std::array, так и
/// в std::vector, хотя внутри там абсолютно одно и тоже
template <class T>
std::string print_apply_help(const T &list)
{
	std::string out(" + Свойства :\r\n");
	bool print = false;

	for (auto i = list.begin(); i != list.end(); ++i)
	{
		if (i->location > 0)
		{
			out += " +    " + print_obj_affects(*i);
			print = true;
		}
	}

	return (print ? out : "");
}

/// распечатка справки по активатору с суммированием аффектов, если активаторов
/// больше одного - вывод суммы аффектов
std::string print_activ_help(const set_node &set)
{
	std::string out, prof_list;
	char buf_[2048];

	snprintf(buf_, sizeof(buf_),
		"--------------------------------------------------------------------------------\r\n"
		"%sНабор предметов: %s%s%s%s\r\n",
		KNRM, KWHT, set.name.c_str(), KNRM,
		set.enabled ? "" : " (в данный момент отключен)");
	out += buf_ + print_obj_list(set) +
		"--------------------------------------------------------------------------------\r\n";

	for (auto i = set.activ_list.begin(); i != set.activ_list.end(); ++i)
	{
		if (i->second.prof.count() != i->second.prof.size())
		{
			// активатор на ограниченный список проф (распечатка закладывается
			// на то, что у валидных сетов списки проф должны быть одинаковые)
			if (prof_list.empty())
			{
				print_bitset(i->second.prof, pc_class_name, ",", prof_list);
			}
			snprintf(buf_, sizeof(buf_), "%d %s (%s)\r\n",
				i->first, desc_count(i->first, WHAT_OBJECT), prof_list.c_str());
		}
		else
		{
			snprintf(buf_, sizeof(buf_), "%d %s\r\n",
				i->first, desc_count(i->first, WHAT_OBJECT));
		}
		out += buf_;
		// affects
		out += print_affects_help(i->second.affects);
		// apply
		out += print_apply_help(i->second.apply);
		// skill
		if (i->second.skill.num > 0)
		{
			std::map<int, int> skills;
			skills[i->second.skill.num] = i->second.skill.val;
			out += PrintActivators::print_skills(skills, true);
		}
	}

	if (set.activ_list.size() > 1)
	{
		out += print_total_activ(set);
	}

	return out;
}

/// выриант print_activ_help только для суммы активаторов (олц)
std::string print_total_activ(const set_node &set)
{
	std::string out, prof_list;

	PrintActivators::clss_activ_node summ, prof_summ;
	for (auto i = set.activ_list.begin(); i != set.activ_list.end(); ++i)
	{
		if (i->second.prof.count() != i->second.prof.size())
		{
			// активатор на ограниченный список проф (распечатка закладывается
			// на то, что у валидных сетов списки проф должны быть одинаковые)
			if (prof_list.empty())
			{
				print_bitset(i->second.prof, pc_class_name, ",", prof_list);
			}
			// affects
			prof_summ.total_affects += i->second.affects;
			// apply
			PrintActivators::sum_affected(prof_summ.affected, i->second.apply);
			// skill
			if (i->second.skill.num > 0)
			{
				std::map<int, int> skills;
				skills[i->second.skill.num] = i->second.skill.val;
				PrintActivators::sum_skills(prof_summ.skills, skills);
			}
		}
		else
		{
			// affects
			summ.total_affects += i->second.affects;
			// apply
			PrintActivators::sum_affected(summ.affected, i->second.apply);
			// skill
			if (i->second.skill.num > 0)
			{
				std::map<int, int> skills;
				skills[i->second.skill.num] = i->second.skill.val;
				PrintActivators::sum_skills(summ.skills, skills);
			}
		}
	}
	out += "--------------------------------------------------------------------------------\r\n";
	out += "Суммарный бонус:\r\n";
	out += print_affects_help(summ.total_affects);
	out += print_apply_help(summ.affected);
	out += PrintActivators::print_skills(summ.skills, true);
	if (!prof_list.empty())
	{
		out += "Профессии: " + prof_list + "\r\n";
		out += print_affects_help(prof_summ.total_affects);
		out += print_apply_help(prof_summ.affected);
		out += PrintActivators::print_skills(prof_summ.skills, true);
	}
	out += "--------------------------------------------------------------------------------\r\n";

	return out;
}

/// генерация справки по активаторам через индексы сетов, которые потом видны
/// через опознание любой сетины
void init_xhelp()
{
	char buf_[128];
	for (size_t i = 0; i < sets_list.size(); ++i)
	{
		snprintf(buf_, sizeof(buf_), "активсет%d", i + 1);
		HelpSystem::add_static(buf_, print_activ_help(*(sets_list.at(i))), 0, true);
	}
}

/// временная фигня для глобал-дропа сетов - имя сета через индекс
std::string get_name(size_t idx)
{
	if (idx < sets_list.size())
	{
		return sets_list.at(idx)->name;
	}
	return "";
}

/// очистка списка сетин на чаре перед очередным заполнением
void WornSets::clear()
{
	for (auto i = idx_list_.begin(); i != idx_list_.end(); ++i)
	{
		i->set_idx = -1;
		i->obj_list.clear();
		i->activated_cnt = 0;
	}
}

/// добавление сетины (для игроков и чармисов) на последующую обработку
/// одновременно сразу же считается кол-во активированных шмоток в каждом сете
void WornSets::add(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_NPC(ch) && !IS_CHARMICE(ch)) return;

	if (GET_OBJ_RNUM(obj) >= 0
		&& obj_index[GET_OBJ_RNUM(obj)].set_idx != static_cast<size_t>(-1))
	{
		const size_t cur_idx = obj_index[GET_OBJ_RNUM(obj)].set_idx;
		for (auto i = idx_list_.begin(); i != idx_list_.end(); ++i)
		{
			if (i->set_idx == static_cast<size_t>(-1))
			{
				i->set_idx = cur_idx;
			}
			if (i->set_idx == cur_idx)
			{
				i->obj_list.push_back(obj);
				if (obj->get_activator().first)
				{
					i->activated_cnt += 1;
				}
				return;
			}
		}
	}
}

/// проверка списка сетов на чаре и применение их активаторов
void WornSets::check(CHAR_DATA *ch)
{
    for (auto i = idx_list_.begin(); i != idx_list_.end(); ++i)
	{
		if (i->set_idx >= sets_list.size()) return;

		boost::shared_ptr<set_node> &cur_set = sets_list.at(i->set_idx);

		int max_activ = 0;
		if (cur_set->enabled)
		{
			for (auto k = cur_set->activ_list.cbegin();
				k != cur_set->activ_list.cend(); ++k)
			{
				const size_t prof_bit = GET_CLASS(ch);
				// k->first - кол-во для активации,
				// i->obj_list.size() - одето на чаре
				if (k->first <= i->obj_list.size()
					&& prof_bit < k->second.prof.size()
					&& k->second.prof.test(prof_bit))
				{
					apply_activator(ch, k->second);
					max_activ = k->first;
					check_activated(ch, k->first, *i);
				}
			}
		}
		// на деактивацию проверять надо даже выключенные сеты
		check_deactivated(ch, max_activ, *i);
	}
}

} // namespace obj_sets

/// иммский slist
ACMD(do_slist)
{
	if (IS_NPC(ch))
	{
		return;
	}
	obj_sets::do_slist(ch);
}
