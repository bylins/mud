// Copyright (c) 2014 Krodo
// Part of Bylins http://www.mud.ru

#include <boost/lexical_cast.hpp>

//#include <iostream>

#include "entities/world_characters.h"
#include "obj_prototypes.h"
#include "obj_sets_stuff.h"
#include "utils/pugixml/pugixml.h"
#include "utils/parse.h"
#include "color.h"
#include "modify.h"
#include "help.h"
#include "game_mechanics/sets_drop.h"
#include "structs/global_objects.h"

namespace obj_sets {

int SetNode::uid_cnt = 0;

std::set<int> list_vnum;

const char *OBJ_SETS_FILE = LIB_MISC"obj_sets.xml";
/// мин/макс кол-во активаторов для валидного сета
const unsigned MIN_ACTIVE_SIZE = 2;
/// вобщем-то равняется кол-ву допустимых для сетов слотов
const unsigned MAX_ACTIVE_SIZE = 10;
/// сколько предметов может быть в сете
const unsigned MAX_OBJ_LIST = 20;
/// основной список сетов
std::vector<std::shared_ptr<SetNode>> sets_list;
/// дефолтные сообщения всех сетов, инятся в init_global_msg()
SetMsgNode global_msg;

/*
 * Распечатка списка классов, на которых активируется сет.
 * Временный костыль до изменения формата сетов.
 * (Не дело, что классы лежат в битвекторе и любое изменение
 * числа или порядка класов будет ломать все сеты разом.)
 * ABYRVALG
 */
void PrinSetClasses(const std::bitset<kNumPlayerClasses> &bits, std::string &str, bool print_num) {
	static char tmp_buf[10];
	bool first = true;

	for (unsigned i = 0; i < bits.size(); ++i) {
		if (bits.test(i) == true) {
			if (!first) {
				str += ",";
			} else {
				first = false;
			}

			if (print_num) {
				snprintf(tmp_buf, sizeof(tmp_buf), "%d:", i + 1);
				str += tmp_buf;
			}

			str += MUD::Classes().FindAvailableItem(static_cast<int>(i)).GetName();
		}
	}
}

/// форматирование строки с разбиением на строки не длиннее \param len,
/// строка разбивается полными словами по разделителю \param sep
/// \param base_offset = 0 - смещение первой строки
std::string line_split_str(const std::string &str, const std::string &sep,
						   size_t len, size_t base_offset) {
	std::string out;
	out.reserve(str.size());
	const size_t sep_len = sep.length();
	size_t idx = 0, cur_line = base_offset, prev_idx = 0;

	while ((idx = str.find(sep, idx)) != std::string::npos) {
		const size_t tok_len = idx + sep_len - prev_idx;
		if (cur_line + tok_len > len) {
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

// приводит внум из дублированных миников к оригинальному внуму
int normalize_vnum(int vnum) {
	if (vnum >= DUPLICATE_MINI_SET_VNUM)
		return vnum - DUPLICATE_MINI_SET_VNUM;
	return vnum;
}

/// \return индекс сета через внум любого его предмета
size_t setidx_by_objvnum(int vnum) {
	vnum = normalize_vnum(vnum);
	for (size_t i = 0; i < sets_list.size(); ++i) {
		if (sets_list.at(i)->obj_list.find(vnum) !=
			sets_list.at(i)->obj_list.end()) {
			return i;
		}
	}
	return -1;
}

/// \return индекс сета через его уид (олц)
size_t setidx_by_uid(int uid) {
	for (size_t i = 0; i < sets_list.size(); ++i) {
		if (sets_list.at(i)->uid == uid) {
			return i;
		}
	}
	return -1;
}

/// проверка предмета на наличие в других сетах
/// \param set_uid - чтобы не считать свой же сет за дубль
bool is_duplicate(int set_uid, int vnum) {
	for (auto & i : sets_list) {
		if (i->uid != set_uid
			&& i->obj_list.find(vnum)
				!= i->obj_list.end()) {
			return true;
		}
	}
	return false;
}

void update_char_sets() {
	for (const auto &ch : character_list) {
		if (!ch->is_npc()
			|| IS_CHARMICE(ch)) {
			ch->obj_bonus().update(ch.get());
		}
	}
}

/// запись индексов сетов в список индексов предметов (не в ObjectData)
/// здесь же обновляется справка по активаторам сетов
void init_obj_index() {
	std::unordered_map<ObjVnum, size_t> tmp;
	tmp.reserve(obj_proto.size());

	for (size_t i = 0; i < obj_proto.size(); ++i) {
		obj_proto.set_idx(i, -1);
		const auto vnum = obj_proto[i]->get_vnum();
		tmp.emplace(vnum, i);
	}

	for (size_t i = 0; i < sets_list.size(); ++i) {
		for (auto k = sets_list.at(i)->obj_list.begin();
			 k != sets_list.at(i)->obj_list.end(); ++k) {
			auto m = tmp.find(k->first);
			if (m != tmp.end()) {
				obj_proto.set_idx(m->second, i);
				SetsDrop::create_clone_miniset(k->first);

			}
		}
	}
	HelpSystem::reload(HelpSystem::STATIC);
	update_char_sets();
}

/// сеты не вешаются на: кольца, ожерелья, браслеты, свет
bool verify_wear_flag(const CObjectPrototype::shared_ptr & /*obj*/) {
/*	if (CAN_WEAR(obj, kFinger)
		|| CAN_WEAR(obj, kNeck)
		|| CAN_WEAR(obj, kWrist)
		|| GET_OBJ_TYPE(obj) == kLight)
	{
		return false;
	}
*/    return true;
}

/// проверка сета после олц или лоада из конфига, при необходимости
/// помечает сет как неактивный (сам сет или другие его поля не трогаются)
void VerifySet(SetNode &set) {
	const size_t num = setidx_by_uid(set.uid) + 1;

	if (set.obj_list.size() < 2 || set.activ_list.empty()) {
		err_log("Items set #%zu: incomplete (objs=%zu, activs=%zu).",
				num, set.obj_list.size(), set.activ_list.size());
		set.enabled = false;
	}
	if (set.obj_list.size() > MAX_OBJ_LIST) {
		err_log("Items set #%zu: too many objects (size=%zu).",	num, set.obj_list.size());
		set.enabled = false;
	}

	for (auto i = set.obj_list.begin(); i != set.obj_list.end(); ++i) {
		const int rnum = real_object(i->first);
		if (rnum < 0) {
			err_log("Items set #%zu: empty obj proto (vnum=%d).", num, i->first);
			set.enabled = false;
		} else if (!verify_wear_flag(obj_proto[rnum])) {
			err_log("Items set #%zu: obj have invalid wear flag (vnum=%d).",
					num, i->first);
			set.enabled = false;
		}
		if (is_duplicate(set.uid, i->first)) {
			err_log("Items set #%zu: dublicate obj (vnum=%d).", num, i->first);
			set.enabled = false;
		}
	}

	std::bitset<kNumPlayerClasses> prof_bits;
	bool prof_restrict = false;
	for (auto i = set.activ_list.begin(); i != set.activ_list.end(); ++i) {
		if (i->first < MIN_ACTIVE_SIZE || i->first > MAX_ACTIVE_SIZE) {
			err_log("Items set #%zu: incorrect activator size (activ=%d).",
					num, i->first);
			set.enabled = false;
		}
		if (i->first > set.obj_list.size()) {
			err_log("Items set #%zu: ativator is greater than item list size (activ=%d).",
					num, i->first);
			set.enabled = false;
		}
		for (auto k = i->second.apply.begin(); k != i->second.apply.end(); ++k) {
			if (k->location < 0 || k->location >= EApply::kNumberApplies) {
				err_log(
					"Item set #%zu: incorrect affect apply (loc=%d, mod=%d, activ=%d).",
					num, k->location, k->modifier, i->first);
				set.enabled = false;
			}
		}

		if (MUD::Skills().IsUnknown(i->second.skill.first)
			|| i->second.skill.second > 200
			|| i->second.skill.second < -200) {
			err_log("Items set #%zu (%s): incorrect skill number or value (num=%d, val=%d, activ=%d)",
				num, set.name.c_str() ,to_underlying(i->second.skill.first), i->second.skill.second, i->first);
			set.enabled = false;
		}

		if (i->second.empty()) {
			err_log("Item set #%zu: activator is empty (activ=%d)", num, i->first);
			set.enabled = false;
		}
		if (!i->second.prof.all()) {
			if (!prof_restrict) {
				prof_bits = i->second.prof;
				prof_restrict = true;
			} else if (i->second.prof != prof_bits) {
				err_log(
					"Items set #%zu: несовпадение ограниченного списка профессий активатора (activ=%d)",
					num, i->first);
				set.enabled = false;
			}
		}
		if (i->second.bonus.phys_dmg < 0 || i->second.bonus.phys_dmg > 1000) {
			err_log("Items set #%zu: некорректный бонус физ. урона (activ=%d)",
					num, i->first);
			set.enabled = false;
		}
		if (i->second.bonus.mage_dmg < 0 || i->second.bonus.mage_dmg > 1000) {
			err_log("Items set #%zu: некорректный бонус маг. урона (activ=%d)",
					num, i->first);
			set.enabled = false;
		}

		if (i->second.enchant.first < 0) {
			err_log("Items set #%zu: некорректный vnum предмета для энчанта (vnum=%d, activ=%d)",
					num, i->second.enchant.first, i->first);
			set.enabled = false;
		}
		if (i->second.enchant.first > 0 &&
			set.obj_list.find(i->second.enchant.first) == set.obj_list.end()) {
			err_log("Items set #%zu: энчант для предмета, не являющегося частью набора (vnum=%d, activ=%d)",
					num, i->second.enchant.first, i->first);
			set.enabled = false;
		}
		if (i->second.enchant.first > 0 && i->second.enchant.second.empty()) {
			err_log("Items set #%zu: пустой энчант для предмета (vnum=%d, activ=%d)",
					num, i->second.enchant.first, i->first);
			set.enabled = false;
		}
		if (i->second.enchant.second.weight < -100
			|| i->second.enchant.second.weight > 100) {
			err_log("Items set #%zu: некорректный вес для энчанта (weight=%d, activ=%d)",
					num, i->second.enchant.second.weight, i->first);
			set.enabled = false;
		}
		if (i->second.enchant.second.ndice < -100
			|| i->second.enchant.second.ndice > 100
			|| i->second.enchant.second.sdice < -100
			|| i->second.enchant.second.sdice > 100) {
			err_log("Items set #%zu: некорректные кубики для энчанта (%dD%d, activ=%d)",
					num, i->second.enchant.second.ndice,
					i->second.enchant.second.sdice, i->first);
			set.enabled = false;
		}
	}
}

/// помимо уровня глобальных сообщений из конфига - есть вариант хардкодом
void init_global_msg() {
	if (global_msg.char_on_msg.empty()) {
		global_msg.char_on_msg = "&W$o0 засветил$U мягким сиянием.&n";
	}
	if (global_msg.char_off_msg.empty()) {
		global_msg.char_off_msg = "&WСияние $o1 медленно угасло.&n";
	}
	if (global_msg.room_on_msg.empty()) {
		global_msg.room_on_msg = "&W$O0 $n1 засветил$U мягким сиянием.&n";
	}
	if (global_msg.room_off_msg.empty()) {
		global_msg.room_off_msg = "&WСияние $O1 $n1 медленно угасло.&n";
	}
}

/// инит структуры сообщений из конфига
/// отсутствие какой-то из строк (всех) не является ошибкой
void InitMsgNode(SetMsgNode &node, const pugi::xml_node &xml_msg) {
	node.char_on_msg = xml_msg.child_value("char_on_msg");
	node.char_off_msg = xml_msg.child_value("char_off_msg");
	node.room_on_msg = xml_msg.child_value("room_on_msg");
	node.room_off_msg = xml_msg.child_value("room_off_msg");
}

/// лоад при старте мада, релоад через 'reload obj_sets.xml'
void load() {
	log("Loadind %s: start", OBJ_SETS_FILE);
	sets_list.clear();
	init_global_msg();

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(OBJ_SETS_FILE);
	if (!result) {
		err_log("%s", result.description());
		return;
	}
	pugi::xml_node xml_obj_sets = doc.child("obj_sets");
	if (xml_obj_sets.empty()) {
		err_log("<obj_sets> read fail");
		return;
	}
	// <messages>
	pugi::xml_node xml_msg = xml_obj_sets.child("messages");
	if (!xml_msg.empty()) {
		InitMsgNode(global_msg, xml_msg);
	}
	// <set>
	for (const auto & set_node : xml_obj_sets.children("set")) {
		std::shared_ptr<SetNode> tmp_set = std::make_shared<SetNode>();
		// имя, алиас и камент не обязательны
		tmp_set->name = set_node.attribute("name").value();
		tmp_set->alias = set_node.attribute("alias").value();
		tmp_set->comment = set_node.attribute("comment").value();
		// enabled не обязателен, по дефолту сет включен
		tmp_set->enabled = (set_node.attribute("enabled").as_int(1) == 1 ? true : false);

		for (auto & obj_node : set_node.children("obj")) {
			struct SetMsgNode tmp_msg;
			pugi::xml_node xml_msg = obj_node.child("messages");
			if (!xml_msg.empty()) {
				InitMsgNode(tmp_msg, xml_msg);
			}
			// GCC 4.4
			tmp_set->obj_list.emplace(parse::ReadAttrAsInt(obj_node, "vnum"), tmp_msg);
			//tmp_set->obj_list.insert(std::make_pair(Parse::attr_int(obj_node, "vnum"), tmp_msg));
		}

		for (const auto & activ_node : set_node.children("activ")) {
			ActivNode tmp_activ;
			tmp_activ.affects.from_string(activ_node.child_value("affects"));
			for (const auto & apply_node : activ_node.children("apply")) {
				// заполняются только первые kMaxObjAffect
				for (auto & i : tmp_activ.apply) {
					if (i.location <= 0) {
						i.location = static_cast<EApply>(parse::ReadAttrAsInt(apply_node, "loc"));
						i.modifier = parse::ReadAttrAsInt(apply_node, "mod");
						break;
					}
				}
			}

			pugi::xml_node xml_cur = activ_node.child("skill");
			if (!xml_cur.empty()) {
				tmp_activ.skill.first = static_cast<ESkill>(parse::ReadAttrAsInt(xml_cur, "num"));
				tmp_activ.skill.second = parse::ReadAttrAsInt(xml_cur, "val");
			}

			xml_cur = activ_node.child("enchant");
			if (!xml_cur.empty()) {
				tmp_activ.enchant.first = parse::ReadAttrAsInt(xml_cur, "vnum");
				tmp_activ.enchant.second.weight =
					xml_cur.attribute("weight").as_int(0);
				tmp_activ.enchant.second.ndice =
					xml_cur.attribute("ndice").as_int(0);
				tmp_activ.enchant.second.sdice =
					xml_cur.attribute("sdice").as_int(0);
			}
			// <phys_dmg>
			xml_cur = activ_node.child("phys_dmg");
			if (!xml_cur.empty()) {
				tmp_activ.bonus.phys_dmg = parse::ReadAttrAsInt(xml_cur, "pct");
			}
			// <mage_dmg>
			xml_cur = activ_node.child("mage_dmg");
			if (!xml_cur.empty()) {
				tmp_activ.bonus.mage_dmg = parse::ReadAttrAsInt(xml_cur, "pct");
			}
			// если нет атрибута prof - значит актив на все профы
			pugi::xml_attribute xml_prof = activ_node.attribute("prof");
			if (!xml_prof.empty()) {
				std::bitset<kNumPlayerClasses> tmp_p(std::string(xml_prof.value()));
				tmp_activ.prof = tmp_p;
			}
			// активится ли сет на мобах
			pugi::xml_attribute xml_npc = activ_node.attribute("npc");
			if (!xml_npc.empty()) {
				tmp_activ.npc = xml_npc.as_bool();
			}
			tmp_set->activ_list[parse::ReadAttrAsInt(activ_node, "size")] = tmp_activ;
		}
		// <messages>
		pugi::xml_node xml_msg = set_node.child("messages");
		if (xml_msg) {
			InitMsgNode(tmp_set->messages, xml_msg);
		}
		sets_list.push_back(tmp_set);
		VerifySet(*tmp_set);
	}

	init_obj_index();
	log("Loadind %s: done", OBJ_SETS_FILE);
	save();
}

/// сохранения структуры сообщений в конфиг
void save_messages(pugi::xml_node &xml, SetMsgNode &msg) {
	if (!msg.char_on_msg.empty()
		|| !msg.char_off_msg.empty()
		|| !msg.room_on_msg.empty()
		|| !msg.room_off_msg.empty()) {
		xml.append_child("messages");
	} else {
		return;
	}

	pugi::xml_node xml_messages = xml.last_child();
	if (!msg.char_on_msg.empty()) {
		pugi::xml_node xml_msg = xml_messages.append_child("char_on_msg");
		xml_msg.append_child(pugi::node_pcdata).set_value(msg.char_on_msg.c_str());
	}
	if (!msg.char_off_msg.empty()) {
		pugi::xml_node xml_msg = xml_messages.append_child("char_off_msg");
		xml_msg.append_child(pugi::node_pcdata).set_value(msg.char_off_msg.c_str());
	}
	if (!msg.room_on_msg.empty()) {
		pugi::xml_node xml_msg = xml_messages.append_child("room_on_msg");
		xml_msg.append_child(pugi::node_pcdata).set_value(msg.room_on_msg.c_str());
	}
	if (!msg.room_off_msg.empty()) {
		pugi::xml_node xml_msg = xml_messages.append_child("room_off_msg");
		xml_msg.append_child(pugi::node_pcdata).set_value(msg.room_off_msg.c_str());
	}
}

/// сохранение конфига, пишутся и выключенные, и пустые сеты
void save() {
	log("Saving %s: start", OBJ_SETS_FILE);
	char buf_[256];
	pugi::xml_document doc;
	pugi::xml_node xml_obj_sets = doc.append_child("obj_sets");
	// obj_sets/messages
	save_messages(xml_obj_sets, global_msg);

	for (auto & i : sets_list) {
		// obj_sets/set
		pugi::xml_node xml_set = xml_obj_sets.append_child("set");
		if (!i->name.empty()) {
			xml_set.append_attribute("name") = i->name.c_str();
		}
		if (!i->alias.empty()) {
			xml_set.append_attribute("alias") = i->alias.c_str();
		}
		if (!i->comment.empty()) {
			xml_set.append_attribute("comment") = i->comment.c_str();
		}
		if (!i->enabled) {
			xml_set.append_attribute("enabled") = 0;
		}
		// set/messages
		save_messages(xml_set, i->messages);
		// set/obj
		for (auto & o : i->obj_list) {
			pugi::xml_node xml_obj = xml_set.append_child("obj");
			xml_obj.append_attribute("vnum") = o.first;
			save_messages(xml_obj, o.second);
		}
		// set/activ
		for (auto & k : i->activ_list) {
			pugi::xml_node xml_activ = xml_set.append_child("activ");
			xml_activ.append_attribute("size") = k.first;
			if (!k.second.prof.all()) {
				xml_activ.append_attribute("prof")
					= k.second.prof.to_string().c_str();
			}
			if (k.second.npc) {
				xml_activ.append_attribute("npc") = k.second.npc;
			}
			// set/activ/affects
			if (!k.second.affects.empty()) {
				pugi::xml_node xml_affects = xml_activ.append_child("affects");
				*buf_ = '\0';
				k.second.affects.tascii(4, buf_);
				xml_affects.append_child(pugi::node_pcdata).set_value(buf_);
			}
			// set/activ/apply
			for (auto & m : k.second.apply) {
				if (m.location > 0 && m.location < EApply::kNumberApplies && m.modifier) {
					pugi::xml_node xml_apply = xml_activ.append_child("apply");
					xml_apply.append_attribute("loc") = m.location;
					xml_apply.append_attribute("mod") = m.modifier;
				}
			}
			// set/activ/skill
			if (MUD::Skills().IsValid(k.second.skill.first)) {
				pugi::xml_node xml_skill = xml_activ.append_child("skill");
				xml_skill.append_attribute("num") = to_underlying(k.second.skill.first);
				xml_skill.append_attribute("val") = k.second.skill.second;
			}
			// set/activ/enchant
			if (k.second.enchant.first > 0) {
				pugi::xml_node xml_enchant = xml_activ.append_child("enchant");
				xml_enchant.append_attribute("vnum") = k.second.enchant.first;
				if (k.second.enchant.second.weight > 0) {
					xml_enchant.append_attribute("weight") =
						k.second.enchant.second.weight;
				}
				if (k.second.enchant.second.ndice > 0) {
					xml_enchant.append_attribute("ndice") =
						k.second.enchant.second.ndice;
				}
				if (k.second.enchant.second.sdice) {
					xml_enchant.append_attribute("sdice") =
						k.second.enchant.second.sdice;
				}
			}
			// set/activ/phys_dmg
			if (k.second.bonus.phys_dmg > 0) {
				pugi::xml_node xml_bonus = xml_activ.append_child("phys_dmg");
				xml_bonus.append_attribute("pct") = k.second.bonus.phys_dmg;
			}
			// set/activ/mage_dmg
			if (k.second.bonus.mage_dmg > 0) {
				pugi::xml_node xml_bonus = xml_activ.append_child("mage_dmg");
				xml_bonus.append_attribute("pct") = k.second.bonus.mage_dmg;
			}
		}
	}

	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "koi8-r";
	doc.save_file(OBJ_SETS_FILE);
	log("Saving %s: done", OBJ_SETS_FILE);
}
///Создание и заполнение списка сетового набора по 1 вещи из набора
std::set<int> vnum_list_add(int vnum) {
	list_vnum.clear();
	size_t idx = setidx_by_objvnum(vnum);

	if (static_cast<size_t>(-1) == idx) {
		mudlog("setidx_by_objvnum returned -1", BRF, kLvlBuilder, ERRLOG, true);
		return list_vnum;
	}

	for (auto & k : sets_list.at(idx)->obj_list) {
		if (k.first != vnum) {
			list_vnum.insert(k.first);
		}
	}

	return list_vnum;
}

/// распечатка сообщения чару и в комнату
/// \param activated - печатать активатор или деактиватор
/// сообщение берется по первому найденному в цепочке:
/// предмет -> набор -> глобальные сообщения
void print_msg(CharData *ch, ObjData *obj, size_t set_idx, bool activated) {
	if (set_idx >= sets_list.size()) return;

	const char *char_on_msg = nullptr;
	const char *room_on_msg = nullptr;
	const char *char_off_msg = nullptr;
	const char *room_off_msg = nullptr;

	std::shared_ptr<SetNode> &curr_set = sets_list.at(set_idx);
	auto i = curr_set->obj_list.find(GET_OBJ_VNUM(obj));
	if (i != curr_set->obj_list.end()) {
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
	if (activated) {
		act(char_on_msg, false, ch, obj, nullptr, kToChar);
		act(room_on_msg, true, ch, obj, nullptr, kToRoom);
	} else {
		act(char_off_msg, false, ch, obj, nullptr, kToChar);
		act(room_off_msg, true, ch, obj, nullptr, kToRoom);
	}
}

/// сообщение деактивации предмета
void print_off_msg(CharData *ch, ObjData *obj) {
	const auto set_idx = GET_OBJ_RNUM(obj) >= 0
						 ? obj_proto.set_idx(GET_OBJ_RNUM(obj))
						 : ~0ull;
	if (set_idx != ~0ull) {
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
void check_activated(CharData *ch, int activ, idx_node &node) {
	int need_msg = activ - node.activated_cnt;
	for (auto i = node.obj_list.begin(); i != node.obj_list.end(); ++i) {
		ObjData *obj = *i;
		if (need_msg > 0 && !obj->get_activator().first) {
			if (obj->get_activator().second < activ) {
				print_msg(ch, obj, node.set_idx, true);
			}
			obj->set_activator(true, activ);
			++node.activated_cnt;
			--need_msg;
		} else {
			obj->set_activator(obj->get_activator().first,
							   std::max(activ, obj->get_activator().second));
		}
	}
}

/// дергается после проверок активаторов сета, чтобы учесть шмотки, которые
/// перестали работать и распечатать их сообщения о деактивации
void check_deactivated(CharData *ch, int max_activ, idx_node &node) {
	int need_msg = node.activated_cnt - max_activ;
	for (auto i = node.obj_list.begin(); i != node.obj_list.end(); ++i) {
		ObjData *obj = *i;
		if (need_msg > 0 && obj->get_activator().first) {
			print_msg(ch, obj, node.set_idx, false);
			obj->set_activator(false, max_activ);
			--node.activated_cnt;
			--need_msg;
		} else {
			obj->set_activator(obj->get_activator().first, max_activ);
		}
	}
}

/// распечатка предметов набора в два столбца со своим форматирование в
/// каждом из них, для экономии места на экране при опознании
std::string print_obj_list(const SetNode &set) {
	char buf_[128];
	std::string out;
	std::vector<int> rnum_list;
	size_t r_max_name = 0, l_max_name = 0;
	bool left = true;

	for (const auto & i : set.obj_list) {
		const int rnum = real_object(i.first);
		if (rnum < 0
			|| obj_proto[rnum]->get_short_description().empty()) {
			continue;
		}

		const size_t curr_name = strlen_no_colors(obj_proto[rnum]->get_short_description().c_str());
		if (left) {
			l_max_name = std::max(l_max_name, curr_name);
		} else {
			r_max_name = std::max(r_max_name, curr_name);
		}
		rnum_list.push_back(rnum);
		left = !left;
	}

	left = true;
	for (int & i : rnum_list) {
		snprintf(buf_, sizeof(buf_), "   %s",
				 colored_name(obj_proto[i]->get_short_description().c_str(),
							  left ? l_max_name : r_max_name, true));
		out += buf_;
		if (!left) {
			out += "\r\n";
		}
		left = !left;
	}

	boost::trim_right(out);
	return out + "\r\n";
}

/// опознание сетового предмета
void print_identify(CharData *ch, const ObjData *obj) {
	const size_t set_idx = GET_OBJ_RNUM(obj) >= 0
						   ? obj_proto.set_idx(GET_OBJ_RNUM(obj))
						   : sets_list.size();
	if (set_idx < sets_list.size()) {
		const SetNode &cur_set = *(sets_list.at(set_idx));
		if (!cur_set.enabled) {
			return;
		}

		std::string out;
		char buf_[256], buf_2[128];

		snprintf(buf_, sizeof(buf_), "%sЧасть набора предметов: %s%s%s\r\n",
				 CCNRM(ch, C_NRM), CCWHT(ch, C_NRM),
				 cur_set.name.c_str(), CCNRM(ch, C_NRM));
		out += buf_;
		out += print_obj_list(cur_set);

		auto i = obj->get_activator();
		if (i.second > 0) {
			snprintf(buf_2, sizeof(buf_2), " (активно %d %s)",
					 i.second, GetDeclensionInNumber(i.second, EWhat::kObject));
		}

		snprintf(buf_, sizeof(buf_), "Свойства набора%s: %sсправка %s%s\r\n",
				 (i.second > 0 ? buf_2 : ""),
				 CCWHT(ch, C_NRM), cur_set.help.c_str(), CCNRM(ch, C_NRM));
		out += buf_;

		send_to_char(out, ch);
	}
}

/// список сетов имму с распечаткой номеров, по которым дергается sed и справка
void do_slist(CharData *ch) {
	std::string out("Зарегистрированные наборы предметов:\r\n");
	char buf_[256];

	int idx = 1;
	for (auto & i : sets_list) {
		char comment[128];
		snprintf(comment, sizeof(comment), " (%s)", i->comment.c_str());
		char status[64];
		snprintf(status, sizeof(status), "Статус: %sотключен%s, ",
				 CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));

		snprintf(buf_, sizeof(buf_),
				 "%3d) %s%s\r\n"
				 "     %sПредметы: ", idx++,
				 (i->name.empty() ? "Безымянный набор" : i->name.c_str()),
				 (i->comment.empty() ? "" : comment),
				 (i->enabled ? "" : status));
		out += buf_;
		for (auto & o : i->obj_list) {
			out += boost::lexical_cast<std::string>(o.first) + " ";
		}
		out += "\r\n";
	}
	page_string(ch->desc, out);
}

/// распечатка аффектов активатора для справки с форматирование по 80 символов
std::string print_activ_affects(const FlagData &aff) {
	char buf_[2048];
	if (aff.sprintbits(weapon_affects, buf_, ",")) {
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
template<class T>
std::string print_activ_apply(const T &list) {
	std::string out;
	for (auto i = list.begin(); i != list.end(); ++i) {
		if (i->location > 0) {
			out += " +    " + print_obj_affects(*i);
		}
	}
	return out;
}

/// распечатка уникальных сетовых бонусов у активатора или суммы активаторов
std::string print_activ_bonus(const bonus_type &bonus) {
	std::string out;
	char buf_[128];

	if (bonus.phys_dmg > 0) {
		snprintf(buf_, sizeof(buf_),
				 " +    %sувеличивает физ. урон на %d%%%s\r\n",
				 KCYN, bonus.phys_dmg, KNRM);
		out += buf_;
	}
	if (bonus.mage_dmg > 0) {
		snprintf(buf_, sizeof(buf_),
				 " +    %sувеличивает маг. урон на %d%%%s\r\n",
				 KCYN, bonus.mage_dmg, KNRM);
		out += buf_;
	}

	return out;
}

std::string print_activ_enchant(const std::pair<int, ench_type> &ench) {
	std::string out;
	char buf_[128];

	if (ench.first > 0) {
		int rnum = real_object(ench.first);
		if (rnum < 0) return "";

		if (ench.second.weight != 0) {
			snprintf(buf_, sizeof(buf_),
					 " +    %s%s вес %s на %d%s\r\n",
					 KCYN, ench.second.weight > 0 ? "увеличивает" : "уменьшает",
					 GET_OBJ_PNAME(obj_proto[rnum], 1).c_str(),
					 abs(ench.second.weight), KNRM);
			out += buf_;
		}
		if (ench.second.ndice != 0 || ench.second.sdice != 0) {
			if (ench.second.ndice >= 0 && ench.second.sdice >= 0) {
				snprintf(buf_, sizeof(buf_),
						 " +    %sувеличивает урон %s на %dD%d%s\r\n",
						 KCYN, GET_OBJ_PNAME(obj_proto[rnum], 1).c_str(),
						 abs(ench.second.ndice), abs(ench.second.sdice), KNRM);
			} else if (ench.second.ndice <= 0 && ench.second.sdice <= 0) {
				snprintf(buf_, sizeof(buf_),
						 " +    %sуменьшает урон %s на %dD%d%s\r\n",
						 KCYN, GET_OBJ_PNAME(obj_proto[rnum], 1).c_str(),
						 abs(ench.second.ndice), abs(ench.second.sdice), KNRM);
			} else {
				snprintf(buf_, sizeof(buf_),
						 " +    %sизменяет урон %s на %+dD%+d%s\r\n",
						 KCYN, GET_OBJ_PNAME(obj_proto[rnum], 1).c_str(),
						 ench.second.ndice, ench.second.sdice, KNRM);
			}
			out += buf_;
		}
	}
	return out;
}

std::string print_activ_enchants(const std::map<int, ench_type> &enchants) {
	std::string out;
	for (auto enchant : enchants) {
		out += print_activ_enchant(enchant);
	}
	return out;
}

/// распечатка всего, что подпадает под раздел 'свойства' в активаторе
std::string print_activ_properties(const ActivNode &activ) {
	std::string out;

	// apply
	out += print_activ_apply(activ.apply);
	// skill
	out += PrintActivators::print_skill(activ.skill, true);
	// bonus
	out += print_activ_bonus(activ.bonus);
	//enchant
	out += print_activ_enchant(activ.enchant);

	if (!out.empty()) {
		return " + Свойства :\r\n" + out;
	}
	return out;
}

/// распечатка справки по активатору с суммированием аффектов, если активаторов
/// больше одного - вывод суммы аффектов
std::string print_activ_help(const SetNode &set) {
	std::string out, prof_list;
	char buf_[2048];
	const int num = setidx_by_uid(set.uid) + 1;
	snprintf(buf_, sizeof(buf_),
			 "--------------------------------------------------------------------------------\r\n"
			 "%s%d) Набор предметов: %s%s%s%s\r\n",
			 KNRM, num, KWHT, set.name.c_str(), KNRM,
			 set.enabled ? "" : " (в данный момент отключен)");
	out += buf_ + print_obj_list(set) +
		"--------------------------------------------------------------------------------\r\n";

	for (const auto & i : set.activ_list) {
		if (!i.second.prof.all()) {
			// активатор на ограниченный список проф (распечатка закладывается
			// на то, что у валидных сетов списки проф должны быть одинаковые)
			if (prof_list.empty()) {
				PrinSetClasses(i.second.prof, prof_list);
			}
			snprintf(buf_, sizeof(buf_), "%d %s (%s)\r\n",
					 i.first, GetDeclensionInNumber(i.first, EWhat::kObject), prof_list.c_str());
		} else {
			snprintf(buf_, sizeof(buf_), "%d %s\r\n",
					 i.first, GetDeclensionInNumber(i.first, EWhat::kObject));
		}
		out += buf_;
		// аффекты
		out += print_activ_affects(i.second.affects);
		// свойства
		out += print_activ_properties(i.second);
	}

	if (set.activ_list.size() > 1) {
		out += print_total_activ(set);
	}

	return out;
}

/// выриант print_activ_help только для суммы активаторов (олц)
std::string print_total_activ(const SetNode &set) {
	std::string out, prof_list, properties;

	activ_sum summ, prof_summ;
	for (const auto & i : set.activ_list) {
		if (!i.second.prof.all()) {
			// активатор на ограниченный список проф (распечатка закладывается
			// на то, что у валидных сетов списки проф должны быть одинаковые)
			if (prof_list.empty()) {
				PrinSetClasses(i.second.prof, prof_list);
				if (i.second.npc) {
					prof_list += prof_list.empty() ? "npc" : ", npc";
				}
			}
			prof_summ += &(i.second);
		} else {
			summ += &(i.second);
		}
	}

	out += "--------------------------------------------------------------------------------\r\n";
	out += "Суммарный бонус:\r\n";
	out += print_activ_affects(summ.affects);
	properties += print_activ_apply(summ.apply);
	properties += PrintActivators::print_skills(summ.skills, true, false);
	properties += print_activ_bonus(summ.bonus);
	properties += print_activ_enchants(summ.enchants);
	if (!properties.empty()) {
		out += " + Свойства :\r\n" + properties;
	}

	if (!prof_list.empty()) {
		properties.clear();
		out += "Профессии: " + prof_list + "\r\n";
		out += print_activ_affects(prof_summ.affects);
		properties += print_activ_apply(prof_summ.apply);
		properties += PrintActivators::print_skills(prof_summ.skills, true, false);
		properties += print_activ_bonus(prof_summ.bonus);
		properties += print_activ_enchants(prof_summ.enchants);
		if (!properties.empty()) {
			out += " + Свойства :\r\n" + properties;
		}
	}
	out += "--------------------------------------------------------------------------------\r\n";

	return out;
}

/// генерация справки по активаторам через индексы сетов, которые потом видны
/// через опознание любой сетины
void init_xhelp() {
	char buf_[128];
	for (size_t i = 0; i < sets_list.size(); ++i) {
		const int lvl = (sets_list.at(i)->enabled ? 0 : kLvlImmortal);
		if (sets_list.at(i)->alias.empty()) {
			snprintf(buf_, sizeof(buf_), "актив%02d", static_cast<int>(i + 1));
			HelpSystem::add_static(buf_,
								   print_activ_help(*(sets_list.at(i))), lvl, true);
			sets_list.at(i)->help = buf_;
		} else {
			bool first = true;
			std::string name = "актив";
			std::vector<std::string> str_list;
			boost::split(str_list, sets_list.at(i)->alias,
						 boost::is_any_of(", "), boost::token_compress_on);
			for (auto & k : str_list) {
				if (first) {
					sets_list.at(i)->help = name + k;
					first = false;
				}
				HelpSystem::add_static(name + k,
									   print_activ_help(*(sets_list.at(i))), lvl, true);
			}
		}
	}
}

/// временная фигня для глобал-дропа сетов - имя сета через индекс
std::string get_name(size_t idx) {
	if (idx < sets_list.size()) {
		return sets_list.at(idx)->name;
	}
	return "";
}

/// очистка списка сетин на чаре перед очередным заполнением
void WornSets::clear() {
	for (auto & i : idx_list_) {
		i.set_idx = -1;
		i.obj_list.clear();
		i.activated_cnt = 0;
	}
}

/// добавление сетины (для игроков и чармисов) на последующую обработку
/// одновременно сразу же считается кол-во активированных шмоток в каждом сете
void WornSets::add(ObjData *obj) {
	if (obj && is_set_item(obj)) {
		const size_t cur_idx = obj_proto.set_idx(GET_OBJ_RNUM(obj));
		for (auto & i : idx_list_) {
			if (i.set_idx == static_cast<size_t>(-1)) {
				i.set_idx = cur_idx;
			}
			if (i.set_idx == cur_idx) {
				i.obj_list.push_back(obj);
				if (obj->get_activator().first) {
					i.activated_cnt += 1;
				}
				return;
			}
		}
	}
}

/// проверка списка сетов на чаре и применение их активаторов
void WornSets::check(CharData *ch) {
	for (auto & i : idx_list_) {
		if (i.set_idx >= sets_list.size()) return;

		std::shared_ptr<SetNode> &cur_set = sets_list.at(i.set_idx);

		int max_activ = 0;
		if (cur_set->enabled) {
			for (const auto & k : cur_set->activ_list) {
				const size_t prof_bit = GET_CLASS(ch);
				// k->first - кол-во для активации,
				// i->obj_list.size() - одето на чаре
				if (k.first > i.obj_list.size()) {
					continue;
				} else if (!ch->is_npc()
					&& prof_bit < k.second.prof.size()
					&& !k.second.prof.test(prof_bit)) {
					continue;
				} else if (ch->is_npc()
					&& (!k.second.prof.all() && !k.second.npc)) {
					continue;
				}
				// суммируем все на чаре, потом кому надо - сами дернут
				ch->obj_bonus() += &(k.second);
				max_activ = k.first;
				check_activated(ch, k.first, i);
			}
		}
		// на деактивацию проверять надо даже выключенные сеты
		check_deactivated(ch, max_activ, i);
	}
}

bool bonus_type::operator!=(const bonus_type &r) const {
	if (phys_dmg != r.phys_dmg || mage_dmg != r.mage_dmg) {
		return true;
	}
	return false;
}

bool bonus_type::operator==(const bonus_type &r) const {
	return !(*this != r);
}

bonus_type &bonus_type::operator+=(const bonus_type &r) {
	phys_dmg += r.phys_dmg;
	mage_dmg += r.mage_dmg;
	return *this;
}

bool bonus_type::empty() const {
	if (phys_dmg != 0 || mage_dmg != 0) {
		return false;
	}
	return true;
}

bool ench_type::operator!=(const ench_type &r) const {
	if (weight != r.weight
		|| ndice != r.ndice
		|| sdice != r.sdice) {
		return true;
	}
	return false;
}

bool ench_type::operator==(const ench_type &r) const {
	return !(*this != r);
}

ench_type &ench_type::operator+=(const ench_type &r) {
	weight += r.weight;
	ndice += r.ndice;
	sdice += r.sdice;
	return *this;
}

bool ench_type::empty() const {
	if (weight != 0 || ndice != 0 || sdice != 0) {
		return false;
	}
	return true;
}

activ_sum &activ_sum::operator+=(const ActivNode *r) {
	affects += r->affects;
	PrintActivators::sum_apply(apply, r->apply);
	PrintActivators::add_pair(skills, r->skill);
	bonus += r->bonus;
	PrintActivators::add_pair(enchants, r->enchant);

	return *this;
}

bool activ_sum::operator!=(const activ_sum &r) const {
	if (affects != r.affects
		|| apply != r.apply
		|| skills != r.skills
		|| bonus != r.bonus
		|| enchants != r.enchants) {
		return true;
	}
	return false;
}

bool activ_sum::operator==(const activ_sum &r) const {
	return !(*this != r);
}

bool activ_sum::empty() const {
	if (!affects.empty()
		|| !apply.empty()
		|| !skills.empty()
		|| !bonus.empty()
		|| !enchants.empty()) {
		return false;
	}
	return true;
}

void activ_sum::clear() {
	affects = clear_flags;
	apply.clear();
	skills.clear();
	bonus.phys_dmg = 0;
	bonus.mage_dmg = 0;
	enchants.clear();
}

WornSets worn_sets;

void check_enchants(CharData *ch) {
	ObjData *obj;
	for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
		obj = GET_EQ(ch, i);
		if (obj) {
			auto i = ch->obj_bonus().enchants.find(normalize_vnum(GET_OBJ_VNUM(obj)));
			if (i != ch->obj_bonus().enchants.end()) {
				obj->update_enchants_set_bonus(i->second);
			} else {
				obj->remove_set_bonus();
			}
		}
	}
}

void activ_sum::update(CharData *ch) {
	this->clear();
	worn_sets.clear();
	for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
		worn_sets.add(GET_EQ(ch, i));
	}
	worn_sets.check(ch);
	check_enchants(ch);
}

void activ_sum::apply_affects(CharData *ch) const {
	for (const auto &j : weapon_affect) {
		if (j.aff_bitvector != 0
			&& affects.get(j.aff_pos)) {
			affect_modify(ch, EApply::kNone, 0, static_cast<EAffect>(j.aff_bitvector), true);
		}
	}
	for (auto &&i : apply) {
		affect_modify(ch, i.location, i.modifier, static_cast<EAffect>(0), true);
	}
}

int activ_sum::calc_phys_dmg(int dam) const {
	return dam * bonus.phys_dmg / 100;
}

int activ_sum::calc_mage_dmg(int dam) const {
	return dam * bonus.mage_dmg / 100;
}

int activ_sum::get_skill(const ESkill num) const {
	auto i = skills.find(num);
	if (i != skills.end()) {
		return i->second;
	}
	return 0;
}

bool is_set_item(ObjData *obj) {
	if (GET_OBJ_RNUM(obj) >= 0
		&& obj_proto.set_idx(GET_OBJ_RNUM(obj)) != static_cast<size_t>(-1)) {
		return true;
	}
	return false;
}

} // namespace obj_sets

/// иммский slist
void do_slist(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->is_npc()) {
		return;
	}
	obj_sets::do_slist(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
