// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "help.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm/remove_if.hpp>

#include "obj_prototypes.h"
#include "modify.h"
#include "house.h"
#include "game_mechanics/sets_drop.h"
#include "color.h"
#include "structs/global_objects.h"

extern char *help;
extern int top_imrecipes;

namespace PrintActivators {

// распечатка активов
struct dup_node {
	// строка профессий
	std::string clss;
	// все аффекты
	std::string afct;
};

void sum_skills(CObjectPrototype::skills_t &target, const CObjectPrototype::skills_t &add) {
	for (auto i : add) {
		if (i.second != 0) {
			auto ii = target.find(i.first);
			if (ii != target.end()) {
				ii->second += i.second;
			} else {
				target[i.first] = i.second;
			}
		}
	}
}

void sum_skills(CObjectPrototype::skills_t &target, const CObjectPrototype::skills_t::value_type &add) {
	if (MUD::Skills().IsValid(add.first) && add.second != 0) {
		auto i = target.find(add.first);
		if (i != target.end()) {
			i->second += add.second;
		} else {
			target[add.first] = add.second;
		}
	}
}

void sum_skills(CObjectPrototype::skills_t &target, const CObjectPrototype *obj) {
	if (obj->has_skills()) {
		CObjectPrototype::skills_t tmp_skills;
		obj->get_skills(tmp_skills);
		sum_skills(target, tmp_skills);
	}
}

inline bool bit_is_set(const uint32_t flags, const int bit) {
	return 0 != (flags & (1 << bit));
}

// проверка обратная flag_data_by_num()
bool check_num_in_unique_bit_flag_data(const unique_bit_flag_data &data, const int num) {
	return (0 <= num && num < 120) ? data.get_flag(num / 30, 1 << num) : false;
}

std::string print_skill(const CObjectPrototype::skills_t::value_type &skill, bool activ) {
	std::string out;
	if (skill.second != 0) {
		out += boost::str(boost::format("%s%s%s%s%s%s%d%%%s\r\n")
							  % (activ ? " +    " : "   ") % KCYN
							  % MUD::Skill(skill.first).GetName() % KNRM
							  % KCYN % (skill.second < 0 ? " ухудшает на " : " улучшает на ")
							  % abs(skill.second) % KNRM);
	}
	return out;
}

/// распечатка массива скилов с " + " перед активаторами
/// \param header = true (печатать или нет заголовок 'Меняет умения')
std::string print_skills(const CObjectPrototype::skills_t &skills, bool activ, bool header) {
	std::string out;
	for (const auto &i : skills) {
		out += print_skill(i, activ);
	}

	if (!out.empty() && header) {
		std::string head = activ ? " + " : "   ";
		return head + "Меняет умения :\r\n" + out;
	}

	return out;
}

// распечатка важных в контексте сетов родных стат предмета
std::string print_obj_affects(const CObjectPrototype *const obj) {
	std::stringstream out;

	out << GET_OBJ_PNAME(obj, 0) << "\r\n";

	if (obj->get_no_flags().sprintbits(no_bits, buf2, ",")) {
		out << "Неудобства : " << buf2 << "\r\n";
	}

	if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
		const int drndice = GET_OBJ_VAL(obj, 1);
		const int drsdice = GET_OBJ_VAL(obj, 2);
		out << boost::format("Наносимые повреждения '%dD%d' среднее %.1f\r\n")
			% drndice % drsdice % ((drsdice + 1) * drndice / 2.0);
	}

	if (GET_OBJ_TYPE(obj) == EObjType::kWeapon
		|| CAN_WEAR(obj, EWearFlag::kShield)
		|| CAN_WEAR(obj, EWearFlag::kHands)) {
		out << "Вес : " << GET_OBJ_WEIGHT(obj) << "\r\n";
	}

	if (GET_OBJ_AFFECTS(obj).sprintbits(weapon_affects, buf2, ",")) {
		out << "Аффекты : " << buf2 << "\r\n";
	}

	std::string tmp_str;
	for (int i = 0; i < kMaxObjAffect; i++) {
		if (obj->get_affected(i).modifier != 0) {
			tmp_str += "   " + print_obj_affects(obj->get_affected(i));
		}
	}

	if (!tmp_str.empty()) {
		out << "Свойства :\r\n" << tmp_str;
	}

	if (obj->has_skills()) {
		CObjectPrototype::skills_t skills;
		obj->get_skills(skills);
		out << print_skills(skills, false);
	}

	return out.str();
}

// распечатка конкретного активатора предмета
std::string print_activator(class_to_act_map::const_iterator &activ, const CObjectPrototype *const obj) {
	std::stringstream out;

	out << " + Профессии :";
	for (const auto &char_class : MUD::Classes()) {
		if (check_num_in_unique_bit_flag_data(activ->first, to_underlying(char_class.GetId()))) {
			if (char_class.IsAvailable()) {
				out << " " << char_class.GetName();
			} else if (char_class.GetId() > ECharClass::kLast && char_class.GetId() <= ECharClass::kNpcLast) {
				out << " чармисы";
			}
		}
	}
/*
 *  Старый код пока оставлен на случай глюков - ABYRVALG.
 	for (int i = 0; i <= kNumPlayerClasses * kNumKins; ++i) {
		if (check_num_in_unique_bit_flag_data(activ->first, i)) {
			if (i < kNumPlayerClasses * kNumKins) {
				out << " " << class_name[i];
			} else {
				out << " чармисы";
			}
		}
	}*/
	out << std::endl;

	FlagData affects = activ->second.get_affects();
	if (affects.sprintbits(weapon_affects, buf2, ",")) {
		out << " + Аффекты : " << buf2 << std::endl;
	}

	std::array<obj_affected_type, kMaxObjAffect> affected = activ->second.get_affected();
	std::string tmp_str;
	for (int i = 0; i < kMaxObjAffect; i++) {
		if (affected[i].modifier != 0) {
			tmp_str += " +    " + print_obj_affects(affected[i]);
		}
	}
	if (!tmp_str.empty()) {
		out << " + Свойства :\r\n" << tmp_str;
	}

	if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
		int drndice = 0, drsdice = 0;
		activ->second.get_dices(drsdice, drndice);
		if (drsdice > 0 && drndice > 0) {
			out << boost::format(" + Устанавливает наносимые повреждения '%dD%d' среднее %.1f\r\n")
				% drndice % drsdice % ((drsdice + 1) * drndice / 2.0);
		}
	}

	const int weight = activ->second.get_weight();
	if (weight > 0) {
		out << " + Устанавливает вес: " << weight << "\r\n";
	}

	if (activ->second.has_skills()) {
		CObjectPrototype::skills_t skills;
		activ->second.get_skills(skills);
		out << print_skills(skills, true);
	}

	return out.str();
}

////////////////////////////////////////////////////////////////////////////////
struct activators_obj {
	activators_obj() {
		native_no_flag = clear_flags;
		native_affects = clear_flags;
	};

	// номер профы и ее суммарные активы
	std::map<int, clss_activ_node> clss_list;
	// суммарные статы шмоток
	FlagData native_no_flag;
	FlagData native_affects;
	std::vector<obj_affected_type> native_affected;
	CObjectPrototype::skills_t native_skills;

	// заполнение массива clss_list номерами проф
	void fill_class(set_info::const_iterator k);
	// проход по активаторам всех предметов с поиском проф из clss_list
	void fill_node(const set_info &set);
	// распечатка clss_list со слиянием идентичных текстов активаторов у разных проф
	std::string print();
};

void activators_obj::fill_class(set_info::const_iterator k) {
	for (const auto & m : k->second) {
		for (const auto & q : m.second) {
			for (int i = 0; i <= kNumPlayerClasses * kNumKins; ++i) {
				if (check_num_in_unique_bit_flag_data(q.first, i)) {
					struct clss_activ_node tmp_node;
					clss_list[i] = tmp_node;
				}
			}
		}
	}
}

void activators_obj::fill_node(const set_info &set) {
	for (const auto & k : set) {
		// перебираем полученные ранее профы
		for (auto & w : clss_list) {
			// идем по кол-ву активаторов с конца от максимального
			for (auto m = k.second.rbegin(), mend = k.second.rend(); m != mend; ++m) {
				bool found = false;
				// до первого совпадения по профе
				for (const auto & q : m->second) {
					if (check_num_in_unique_bit_flag_data(q.first, w.first)) {
						// суммирование активаторов для данной профы
						w.second.total_affects += q.second.get_affects();
						sum_apply(w.second.affected, q.second.get_affected());
						// скилы
						CObjectPrototype::skills_t tmp_skills;
						q.second.get_skills(tmp_skills);
						sum_skills(w.second.skills, tmp_skills);
						found = true;
						break;
					}
				}
				if (found) {
					break;
				}
			}
		}
	}
}

std::string activators_obj::print() {
	std::vector<dup_node> dup_list;

	for (auto & cls_it : clss_list) {
		// распечатка аффектов каждой профы
		dup_node node;
		// ABYRVALG временно сделаем так (старый код ниже). Потом надо поменять в списке активов ид класса на emum
		//node.clss += cls_it.first < kNumPlayerClasses * kNumKins ? class_name[cls_it.first] : "чармисы";
		auto class_id = static_cast<ECharClass>(cls_it.first);
		if (MUD::Classes().IsAvailable(class_id)) {
			node.clss += MUD::Class(class_id).GetName();
		} else if (class_id > ECharClass::kLast && class_id <= ECharClass::kNpcLast) {
			node.clss += "чармисы";
		}

		// affects
		cls_it.second.total_affects += native_affects;
		if (cls_it.second.total_affects.sprintbits(weapon_affects, buf2, ",")) {
			node.afct += " + Аффекты : " + std::string(buf2) + "\r\n";
		}
		// affected
		sum_apply(cls_it.second.affected, native_affected);
		// сортировка для более удобного сравнения статов по распечатке
		std::sort(cls_it.second.affected.begin(), cls_it.second.affected.end(),
				  [](const obj_affected_type &lrs, const obj_affected_type &rhs) {
					  return lrs.location < rhs.location;
				  });

		std::string tmp_str;
		for (auto i : cls_it.second.affected) {
			tmp_str += " +    " + print_obj_affects(i);
		}
		if (!tmp_str.empty()) {
			node.afct += " + Свойства :\r\n" + tmp_str;
		}
		// скилы
		sum_skills(cls_it.second.skills, native_skills);
		node.afct += print_skills(cls_it.second.skills, true);

		// слияние одинаковых по аффектам проф
		auto i = std::find_if(dup_list.begin(), dup_list.end(),
														 [&](const dup_node &x) {
															 return x.afct == node.afct;
														 });

		if (i != dup_list.end()) {
			i->clss += ", " + node.clss;
		} else {
			dup_list.push_back(node);
		}
	}

	std::string out_str;
	for (const auto & i : dup_list) {
		out_str += "Профессии : " + i.clss + "\r\n" + i.afct;
	}
	return out_str;
}
// activators_obj
////////////////////////////////////////////////////////////////////////////////

std::string print_fullset_stats(const set_info &set) {
	std::stringstream out;
	activators_obj activ;

	// первый проход - родные статы предметов + инит проф в clss_list
	for (auto k = set.begin(), kend = set.end(); k != kend; ++k) {
		const int rnum = real_object(k->first);
		if (rnum < 0) {
			continue;
		}
		const auto &obj = obj_proto[rnum];

		// суммируем родные статы со шмоток
		activ.native_no_flag += GET_OBJ_NO(obj);
		activ.native_affects += GET_OBJ_AFFECTS(obj);
		sum_apply(activ.native_affected, obj->get_all_affected());
		sum_skills(activ.native_skills, obj.get());

		// иним профы
		activ.fill_class(k);
	}

	// иним активаторы по профам
	activ.fill_node(set);

	// печатаем все, что получилось
	out << "Суммарные свойства набора: \r\n";

	if (activ.native_no_flag.sprintbits(no_bits, buf2, ",")) {
		out << "Неудобства : " << buf2 << "\r\n";
	}

	out << activ.print();

	return out.str();
}

// инициация распечатки справки по активаторам
void process() {
	for (const auto & it : ObjData::set_table) {
		std::stringstream out;
		// it->first = int_id, it->second = set_info
		out << "---------------------------------------------------------------------------\r\n";
		out << it.second.get_name() << "\r\n";
		out << "---------------------------------------------------------------------------\r\n";
		out << print_fullset_stats(it.second);
		for (const auto & k : it.second) {
			out << "---------------------------------------------------------------------------\r\n";
			// k->first = int_obj_vnum, k->second = qty_to_camap_map
			const int rnum = real_object(k.first);
			if (rnum < 0) {
				log("SYSERROR: wrong obj vnum: %d (%s %s %d)", k.first, __FILE__, __func__, __LINE__);
				continue;
			}

			const auto &obj = obj_proto[rnum];
			out << print_obj_affects(obj.get());

			for (const auto & m : k.second) {
				// m->first = num_activators, m->second = class_to_act_map
				for (auto q = m.second.begin(); q != m.second.end(); ++q) {
					out << "Предметов для активации: " << m.first << "\r\n";
					out << print_activator(q, obj.get());
				}
			}
		}
		// генерация алиасов для справки
		std::string set_name = "актив";
		if (it.second.get_alias().empty()) {
			set_name += it.second.get_name();
			set_name.erase(boost::remove_if(set_name, boost::is_any_of(" ,.")), set_name.end());
			HelpSystem::add_static(set_name, out.str(), 0, true);
		} else {
			std::string alias = it.second.get_alias();
			std::vector<std::string> str_list;
			//boost::split(str_list, alias, boost::is_any_of(","));
			utils::Split(str_list, alias, ',');
			for (auto & k : str_list) {
				k.erase(boost::remove_if(k, boost::is_any_of(" ,.")), k.end());
				HelpSystem::add_static(set_name + "сет" + k, out.str(), 0, true);
			}
		}
	}
}

} // namespace PrintActivators
using namespace PrintActivators;

////////////////////////////////////////////////////////////////////////////////

namespace HelpSystem {

struct help_node {
	help_node(const std::string &key, const std::string &val)
		: keyword(key), entry(val), min_level(0),
		  sets_drop_page(false), no_immlog(false) {
		utils::ConvertToLow(keyword);
	};

	// ключ для поиска
	std::string keyword;
	// текст справки
	std::string entry;
	// требуемый уровень для чтения (демигоды могут читать kLevelImmortal)
	int min_level;
	// для сгенерированных страниц дропа сетов
	// не спамят в иммлог при чтении, выводят перед страницей таймер
	bool sets_drop_page;
	// не спамить иммам использование справки
	bool no_immlog;
};

// справка, подгружаемая из файлов на старте (STATIC)
std::vector<help_node> static_help;
// справка для всего, что нужно часто релоадить
// сеты, сайты дружин, групповые зоны (DYNAMIC)
std::vector<help_node> dynamic_help;
// флаг для проверки необходимости обновления dynamic_help по таймеру раз в минуту
bool need_update = false;

const char *HELP_USE_EXMAPLES =
	"&cПримеры:&n\r\n"
	"\t\"справка 3.защита\"\r\n"
	"\t\"справка 4.защита\"\r\n"
	"\t\"справка защитаоттьмы\"\r\n"
	"\t\"справка защита!\"\r\n"
	"\t\"справка 3.защита!\"\r\n"
	"\r\nСм. также: &CИСПОЛЬЗОВАНИЕСПРАВКИ&n\r\n";

class UserSearch {
 public:
	explicit UserSearch(CharData *in_ch)
		: strong(false), stop(false), diff_keys(false), level(0), topic_num(0), curr_topic_num(0) { ch = in_ch; };

	// ищущий чар
	CharData *ch;
	// строгий поиск (! на конце)
	bool strong;
	// флаг остановки прохода по спискам справок
	bool stop;
	// флаг наличия двух и более разных топиков в key_list
	// если топик 1 с несколькими дублями ключей - печатается просто этот топик
	// если топиков два и более - печатается список всех ключей
	bool diff_keys;
	// уровень справки для просмотра данным чаром
	int level;
	// номер из х.поисковая_фраза
	int topic_num;
	// счетчик поиска при topic_num != 0
	int curr_topic_num;
	// поисковая фраза
	std::string arg_str;
	// формирующийся список подходящих топиков
	std::vector<std::vector<help_node>::const_iterator> key_list;

	// инициация поиска через search в нужном массиве
	void process(int flag);
	// собственно сам поиск справки в конкретном массиве
	void search(const std::vector<help_node> &cont);
	// распечатка чару когда ничего не нашлось
	void print_not_found() const;
	// распечатка чару конкретного топика справки
	void print_curr_topic(const help_node &node) const;
	// распечатка чару топика или списка кеев
	// в зависимости от состояния key_list и diff_keys
	void print_key_list() const;
};

/// \param min_level = 0, \param no_immlog = false
void add_static(const std::string &key, const std::string &entry,
				int min_level, bool no_immlog) {
	if (key.empty() || entry.empty()) {
		log("SYSERROR: empty str '%s' -> '%s' (%s:%d %s)",
			key.c_str(), entry.c_str(), __FILE__, __LINE__, __func__);
		return;
	}

	help_node tmp_node(key, entry);
	tmp_node.min_level = min_level;
	tmp_node.no_immlog = no_immlog;
	static_help.push_back(tmp_node);
}

/// \param min_level = 0, no_immlog = true
void add_dynamic(const std::string &key, const std::string &entry) {
	if (key.empty() || entry.empty()) {
		log("SYSERROR: empty str '%s' -> '%s' (%s:%d %s)",
			key.c_str(), entry.c_str(), __FILE__, __LINE__, __func__);
		return;
	}

	help_node tmp_node(key, entry);
	tmp_node.no_immlog = true;
	dynamic_help.push_back(tmp_node);
}

void add_sets_drop(const std::string &key, const std::string &entry) {
	if (key.empty() || entry.empty()) {
		log("SYSERROR: empty str '%s' -> '%s' (%s %s %d)",
			key.c_str(), entry.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	help_node tmp_node(key, entry);
	tmp_node.sets_drop_page = true;
	tmp_node.no_immlog = true;

	dynamic_help.push_back(tmp_node);
}
void init_zone_all() {
	std::stringstream out;

	out << "ЗОНЫ\r\n\r\n";
	for (std::size_t rnum = 0, i = 1; rnum < zone_table.size(); ++rnum) {
		if (zone_table[rnum].location) {
			out << boost::format("  %2d - %s. Расположена: %s. Группа: %d. Примерный уровень: %d.\r\n") % i
				% zone_table[rnum].name % zone_table[rnum].location % zone_table[rnum].group % zone_table[rnum].level;
			++i;
		}
	}
	out << "\r\nСм. также: &CГРУППОВЫЕЗОНЫ&n";
	add_static("зоны", out.str(), 0, true);
}

std::string OutRecipiesHelp(ECharClass ch_class) {
	std::stringstream out, out2;
	std::string tmpstr;
	int columns = 0, columns2 = 0;
	std::vector<std::string> skills_list;
	std::vector<std::string> skills_list2;

	out << "Список доступных рецептов:\r\n";
	out2 << "\r\n&GРецепты, доступные после одного или нескольких перевоплощений:&n\r\n";
	for (int sortpos = 0; sortpos <= top_imrecipes; sortpos++) {
		if (!imrecipes[sortpos].classknow[to_underlying(ch_class)]) {
				continue;
		}
		if (imrecipes[sortpos].remort > 0) {
			skills_list2.push_back(imrecipes[sortpos].name);
			continue;
		}
		skills_list.push_back(imrecipes[sortpos].name);
	}
	utils::SortKoiString(skills_list);
	for (auto it : skills_list) {
		tmpstr = !(++columns % 2) ? "\r\n" : "\t";
		out << "\t" << std::left << std::setw(30) << it << tmpstr;
	}
	utils::SortKoiString(skills_list2);
	for (auto it : skills_list2) {
			tmpstr = !(++columns2 % 2) ? "\r\n" : "\t";
			out2 << "\t" << "&C" << std::left << std::setw(30) << it << "&n" << tmpstr;
	}
	if (out.str().back() == '\t')
		out << "\r\n";
	if (out2.str().back() == '\t')
		out2 << "\r\n";
	out << out2.str();
	return out.str();
}

std::string OutSkillsHelp(ECharClass ch_class) {
	std::stringstream out, out2;
	std::string tmpstr;
	int columns, columns2 = 0;
	std::vector<std::string> skills_list;
	std::vector<std::string> skills_list2;

	out2 << "\r\n&GУникальные умения:&n\r\n";
	for (const auto &skill : MUD::Skills()) {
		if (MUD::Class(ch_class).skills[skill.GetId()].IsInvalid()) {
			continue;
		}
		int num = 0;
		for (const auto &char_class: MUD::Classes()) {
			if (char_class.IsAvailable()) {
				if (char_class.skills[skill.GetId()].IsValid()) {
					++num;
				}
			}
		}
		if (num == 1) {
			skills_list2.push_back(utils::SubstKtoW(skill.GetName()));
			continue;
		}
		skills_list.push_back(utils::SubstKtoW(skill.GetName()));
	}
	std::sort(skills_list.begin(), skills_list.end());
	for (auto it : skills_list) {
		tmpstr = !(++columns % 2) ? "\r\n" : "\t";
		out << "\t" << std::left << std::setw(30) << utils::SubstWtoK(it) << tmpstr;
	}
	std::sort(skills_list2.begin(), skills_list2.end());
	for (auto it : skills_list2) {
			tmpstr = !(++columns2 % 2) ? "\r\n" : "\t";
			out2 << "\t" << "&C" << std::left << std::setw(30) << utils::SubstWtoK(it) << "&n" << tmpstr;
	}
	if (out.str().back() == '\t')
		out << "\r\n";
	if (out2.str().back() == '\t')
		out2 << "\r\n";
	out << out2.str();
	return out.str();
}

std::string OutFeatureHelp(ECharClass ch_class) {
	std::stringstream out, out2, out3;
	std::string tmpstr;
	int columns, columns2, columns3 = 0;
	std::vector<std::string> feat_list;
	std::vector<std::string> feat_list2;
	std::vector<std::string> feat_list3;

	out << "Приобретаемые способности:\r\n";
	out2 << "\r\n&WУникальные приобретаемые способности:&n\r\n";
	out3 << "\r\n&GВрожденные способности:&n\r\n";
	for (const auto &feat : MUD::Class(ch_class).feats) {
		int num = 0;

		if (feat.IsUnavailable()) {
			continue;
		}
		if (feat.IsInborn()) {
			feat_list3.push_back(MUD::Feat(feat.GetId()).GetCName());
			continue;
		}
		for (const auto &char_class: MUD::Classes()) {
			if (char_class.IsAvailable()) {
				for (const auto &feat2 : char_class.feats) {
					if (feat2.GetId() == feat.GetId()) {
						++num;
					}
				}
			}
		}
		if (num == 1) {
			feat_list2.push_back(MUD::Feat(feat.GetId()).GetCName());
			continue;
		}
		feat_list.push_back(MUD::Feat(feat.GetId()).GetCName());
	}
	utils::SortKoiString(feat_list);
	for (auto it : feat_list) {
		tmpstr = !(++columns % 2) ? "\r\n" : "\t";
		out << "\t" << std::left << std::setw(30) << it << tmpstr;
	}
	utils::SortKoiString(feat_list2);
	for (auto it : feat_list2) {
		tmpstr = !(++columns2 % 2) ? "\r\n" : "\t";
		out2 << "\t" << "&C" << std::left << std::setw(30) << it << "&n" << tmpstr;
	}
	utils::SortKoiString(feat_list3);
	for (auto it : feat_list3) {
		tmpstr = !(++columns3 % 2) ? "\r\n" : "\t";
		out3 << "\t" << "&C" << std::left << std::setw(30) << it << "&n" << tmpstr;
	}
	if (out.str().back() == '\t')
		out << "\r\n";
	if (out2.str().back() == '\t')
		out2 << "\r\n";
	if (out3.str().back() == '\t')
		out3 << "\r\n";
	out << out2.str() << out3.str();
	return out.str();
}

void ClassFeatureHelp() {
	std::stringstream out;

	out << "СПОСОБНОСТИЛЕКАРЯ\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kSorcerer);
	out << "\r\nСм. также: &CЛЕКАРЬ, ЗАКЛИНАНИЯЛЕКАРЯ, УМЕНИЯЛЕКАРЯ, ОТВАРЫЛЕКАРЯ&n";
	add_static("СПОСОБНОСТИЛЕКАРЯ", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИКОЛДУНА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kConjurer);
	out << "\r\nСм. также: &CКОЛДУН, ЗАКЛИНАНИЯКОЛДУНА, УМЕНИЯКОЛДУНА, ОТВАРЫКОЛДУНА&n";
	add_static("СПОСОБНОСТИКОЛДУНА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИТАТЯ\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kThief);
	out << "\r\nСм. также: &CТАТЬ, УМЕНИЯТАТЯ, ОТВАРЫТАТЯ&n";
	add_static("СПОСОБНОСТИТАТЯ", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИБОГАТЫРЯ\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kWarrior);
	out << "\r\nСм. также: &CБОГАТЫРЬ, УМЕНИЯБОГАТЫРЯ, ОТВАРЫБОГАТЫРЯ&n";
	add_static("СПОСОБНОСТИБОГАТЫРЯ", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИНАЕМНИКА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kAssasine);
	out << "\r\nСм. также: &CНАЕМНИК, УМЕНИЯНАЕМНИКА, ОТВАРЫНАЕМНИКА&n";
	add_static("СПОСОБНОСТИНАЕМНИКА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИДРУЖИННИКА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kGuard);
	out << "\r\nСм. также: &CДРУЖИННИК, УМЕНИЯДРУЖИННИКА, ОТВАРЫДРУЖИННИКА&n";
	add_static("СПОСОБНОСТИДРУЖИННИКА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИКУДЕСНИКА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kCharmer);
	out << "\r\nСм. также:КУДЕСНИК, ОТВАРЫКУДЕСНИКА, ЗАКЛИНАНИЯКУДЕСНИКА, УМЕНИЯКУДЕСНИКА&n";
	add_static("СПОСОБНОСТИКУДЕСНИКА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИВОЛШЕБНИКА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kWizard);
	out << "\r\nСм. также: &CВОЛШЕБНИК, ОТВАРЫВОЛШЕБНИКА, ЗАКЛИНАНИЯВОЛШЕБНИКА, УМЕНИЯВОЛШЕБНИКА&n";
	add_static("СПОСОБНОСТИВОЛШЕБНИКА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИЧЕРНОКНИЖНИКА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kNecromancer);
	out << "\r\nСм. также: &CЧЕРНОКНИЖНИК, ОТВАРЫЧЕРНОКНИЖНИКА, ЗАКЛИНАНИЯЧЕРНОКНИЖНИКА, УМЕНИЯЧЕРНОКНИЖНИКА&n";
	add_static("СПОСОБНОСТИЧЕРНОКНИЖНИКА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИВИТЯЗЯ\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kPaladine);
	out << "\r\nСм. также: &CВИТЯЗЬ, ОТВАРЫВИТЯЗЯ, ЗАКЛИНАНИЯВИТЯЗЯ, УМЕНИЯВИТЯЗЯ&n";
	add_static("СПОСОБНОСТИВИТЯЗЯ", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИОХОТНИКА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kRanger);
	out << "\r\nСм. также: &CОХОТНИК, ОТВАРЫОХОТНИКА, УМЕНИЯОХОТНИКА&n";
	add_static("СПОСОБНОСТИОХОТНИКА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИКУЗНЕЦА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kVigilant);
	out << "\r\nСм. также: &CКУЗНЕЦ, ОТВАРЫКУЗНЕЦА, УМЕНИЯКУЗНЕЦА&n";
	add_static("СПОСОБНОСТИКУЗНЕЦА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИКУПЦА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kMerchant);
	out << "\r\nСм. также: &CКУПЕЦ, ОТВАРЫКУПЦА, ЗАКЛИНАНИЯКУПЦА, УМЕНИЯКУПЦА&n";
	add_static("СПОСОБНОСТИКУПЦА", out.str(), 0, true);

	out.str("");
	out << "СПОСОБНОСТИВОЛХВА\r\n\r\n";
	out << OutFeatureHelp(ECharClass::kMagus);
	out << "\r\nСм. также: &CВОЛХВ, ОТВАРЫВОЛХВА, ЗАКЛИНАНИЯВОЛХВА, УМЕНИЯВОЛХВА&n";
	add_static("СПОСОБНОСТИВОЛХВА", out.str(), 0, true);
}

void ClassSlillHelp() {
	std::stringstream out;

	out << "УМЕНИЯЛЕКАРЯ\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kSorcerer);
	out << "\r\nСм. также: &CЛЕКАРЬ, ЗАКЛИНАНИЯЛЕКАРЯ, СПОСОБНОСТИЛЕКАРЯ, ОТВАРЫЛЕКАРЯ&n";
	add_static("УМЕНИЯЛЕКАРЯ", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯКОЛДУНА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kConjurer);
	out << "\r\nСм. также: &CКОЛДУН, ЗАКЛИНАНИЯКОЛДУНА, СПОСОБНОСТИКОЛДУНА, ОТВАРЫКОЛДУНА&n";
	add_static("УМЕНИЯКОЛДУНА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯТАТЯ\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kThief);
	out << "\r\nСм. также: &CТАТЬ, СПОСОБНОСТИТАТЯ, ОТВАРЫТАТЯ&n";
	add_static("УМЕНИЯТАТЯ", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯБОГАТЫРЯ\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kWarrior);
	out << "\r\nСм. также: &CБОГАТЫРЬ, СПОСОБНОСТИБОГАТЫРЯ, ОТВАРЫБОГАТЫРЯ&n";
	add_static("УМЕНИЯБОГАТЫРЯ", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯНАЕМНИКА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kAssasine);
	out << "\r\nСм. также: &CНАЕМНИК, СПОСОБНОСТИНАЕМНИКА, ОТВАРЫНАЕМНИКА&n";
	add_static("УМЕНИЯНАЕМНИКА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯДРУЖИННИКА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kGuard);
	out << "\r\nСм. также: &CДРУЖИННИК, СПОСОБНОСТИДРУЖИННИКА, ОТВАРЫДРУЖИННИКА&n";
	add_static("УМЕНИЯДРУЖИННИКА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯКУДЕСНИКА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kCharmer);
	out << "\r\nСм. также:КУДЕСНИК, ОТВАРЫКУДЕСНИКА, ЗАКЛИНАНИЯКУДЕСНИКА, СПОСОБНОСТИКУДЕСНИКА&n";
	add_static("УМЕНИЯКУДЕСНИКА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯВОЛШЕБНИКА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kWizard);
	out << "\r\nСм. также: &CВОЛШЕБНИК, ОТВАРЫВОЛШЕБНИКА, ЗАКЛИНАНИЯВОЛШЕБНИКА, СПОСОБНОСТИВОЛШЕБНИКА&n";
	add_static("УМЕНИЯВОЛШЕБНИКА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯЧЕРНОКНИЖНИКА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kNecromancer);
	out << "\r\nСм. также: &CЧЕРНОКНИЖНИК, ОТВАРЫЧЕРНОКНИЖНИКА, ЗАКЛИНАНИЯЧЕРНОКНИЖНИКА, СПОСОБНОСТИЧЕРНОКНИЖНИКА&n";
	add_static("УМЕНИЯЧЕРНОКНИЖНИКА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯВИТЯЗЯ\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kPaladine);
	out << "\r\nСм. также: &CВИТЯЗЬ, ОТВАРЫВИТЯЗЯ, ЗАКЛИНАНИЯВИТЯЗЯ, СПОСОБНОСТИВИТЯЗЯ&n";
	add_static("УМЕНИЯВИТЯЗЯ", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯОХОТНИКА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kRanger);
	out << "\r\nСм. также: &CОХОТНИК, ОТВАРЫОХОТНИКА, СПОСОБНОСТИОХОТНИКА&n";
	add_static("УМЕНИЯОХОТНИКА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯКУЗНЕЦА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kVigilant);
	out << "\r\nСм. также: &CКУЗНЕЦ, ОТВАРЫКУЗНЕЦА, СПОСОБНОСТИКУЗНЕЦА&n";
	add_static("УМЕНИЯКУЗНЕЦА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯКУПЦА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kMerchant);
	out << "\r\nСм. также: &CКУПЕЦ, ОТВАРЫКУПЦА, ЗАКЛИНАНИЯКУПЦА, СПОСОБНОСТИКУПЦА&n";
	add_static("УМЕНИЯКУПЦА", out.str(), 0, true);

	out.str("");
	out << "УМЕНИЯВОЛХВА\r\n\r\n";
	out << OutSkillsHelp(ECharClass::kMagus	);
	out << "\r\nСм. также: &CВОЛХВ, ОТВАРЫВОЛХВА, ЗАКЛИНАНИЯВОЛХВА, СПОСОБНОСТИВОЛХВА&n";
	add_static("УМЕНИЯВОЛХВА", out.str(), 0, true);
}

void ClassRecipiesHelp() {
	std::stringstream out;

	out << "ОТВАРЫЛЕКАРЯ РЕЦЕПТЫЛЕКАРЯ\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kSorcerer);
	out << "\r\nСм. также: &CЛЕКАРЬ, УМЕНИЯЛЕКАРЯ, ЗАКЛИНАНИЯЛЕКАРЯ, СПОСОБНОСТИЛЕКАРЯ&n";
	add_static("РЕЦЕПТЫЛЕКАРЯ", out.str(), 0, true);
	add_static("ОТВАРЫЛЕКАРЯ", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫКОЛДУНА РЕЦЕПТЫКОЛДУНА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kConjurer);
	out << "\r\nСм. также: &CКОЛДУН, УМЕНИЯКОЛДУНА, ЗАКЛИНАНИЯКОЛДУНА, СПОСОБНОСТИКОЛДУНА&n";
	add_static("РЕЦЕПТЫКОЛДУНА", out.str(), 0, true);
	add_static("ОТВАРЫКОЛДУНА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫТАТЯ РЕЦЕПТЫТАТЯ\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kThief);
	out << "\r\nСм. также: &CТАТЬ, УМЕНИЯТАТЯ, СПОСОБНОСТИТАТЯ&n";
	add_static("РЕЦЕПТЫТАТЯ", out.str(), 0, true);
	add_static("ОТВАРЫТАТЯ", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫБОГАТЫРЯ РЕЦЕПТЫБОГАТЫРЯ\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kWarrior);
	out << "\r\nСм. также: &CБОГАТЫРЬ, УМЕНИЯБОГАТЫРЯ, СПОСОБНОСТИБОГАТЫРЯ&n";
	add_static("РЕЦЕПТЫБОГАТЫРЯ", out.str(), 0, true);
	add_static("ОТВАРЫБОГАТЫРЯ", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫНАЕМНИКА РЕЦЕПТЫНАЕМНИКА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kAssasine);
	out << "\r\nСм. также: &CНАЕМНИК, УМЕНИЯНАЕМНИКА, СПОСОБНОСТИНАЕМНИКА&n";
	add_static("РЕЦЕПТЫНАЕМНИКА", out.str(), 0, true);
	add_static("ОТВАРЫНАЕМНИКА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫДРУЖИННИКА РЕЦЕПТЫДРУЖИННИКА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kGuard);
	out << "\r\nСм. также: &CДРУЖИННИК, УМЕНИЯДРУЖИННИКА, СПОСОБНОСТИДРУЖИННИКА&n";
	add_static("РЕЦЕПТЫДРУЖИННИКА", out.str(), 0, true);
	add_static("ОТВАРЫДРУЖИННИКА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫКУДЕСНИКА РЕЦЕПТЫКУДЕСНИКА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kCharmer);
	out << "\r\nСм. также:КУДЕСНИК, УМЕНИЯКУДЕСНИКА, ЗАКЛИНАНИЯКУДЕСНИКА, СПОСОБНОСТИКУДЕСНИКА&n";
	add_static("РЕЦЕПТЫКУДЕСНИКА", out.str(), 0, true);
	add_static("ОТВАРЫКУДЕСНИКА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫВОЛШЕБНИКА РЕЦЕПТЫВОЛШЕБНИКА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kWizard);
	out << "\r\nСм. также: &CВОЛШЕБНИК, УМЕНИЯВОЛШЕБНИКА, ЗАКЛИНАНИЯВОЛШЕБНИКА, СПОСОБНОСТИВОЛШЕБНИКА&n";
	add_static("РЕЦЕПТЫВОЛШЕБНИКА", out.str(), 0, true);
	add_static("ОТВАРЫВОЛШЕБНИКА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫЧЕРНОКНИЖНИКА РЕЦЕПТЫЧЕРНОКНИЖНИКА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kNecromancer);
	out << "\r\nСм. также: &CЧЕРНОКНИЖНИК, УМЕНИЯЧЕРНОКНИЖНИКА, ЗАКЛИНАНИЯЧЕРНОКНИЖНИКА, СПОСОБНОСТИЧЕРНОКНИЖНИКА&n";
	add_static("РЕЦЕПТЫЧЕРНОКНИЖНИКА", out.str(), 0, true);
	add_static("ОТВАРЫЧЕРНОКНИЖНИКА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫВИТЯЗЯ РЕЦЕПТЫВИТЯЗЯ\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kPaladine);
	out << "\r\nСм. также: &CВИТЯЗЬ, УМЕНИЯВИТЯЗЯ, ЗАКЛИНАНИЯВИТЯЗЯ, СПОСОБНОСТИВИТЯЗЯ&n";
	add_static("РЕЦЕПТЫВИТЯЗЯ", out.str(), 0, true);
	add_static("ОТВАРЫВИТЯЗЯ", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫОХОТНИКА РЕЦЕПТЫОХОТНИКА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kRanger);
	out << "\r\nСм. также: &CОХОТНИК, УМЕНИЯОХОТНИКА, СПОСОБНОСТИОХОТНИКА&n";
	add_static("РЕЦЕПТЫОХОТНИКА", out.str(), 0, true);
	add_static("ОТВАРЫОХОТНИКА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫКУЗНЕЦА РЕЦЕПТЫКУЗНЕЦА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kVigilant);
	out << "\r\nСм. также: &CКУЗНЕЦ, УМЕНИЯКУЗНЕЦА, СПОСОБНОСТИКУЗНЕЦА&n";
	add_static("РЕЦЕПТЫКУЗНЕЦА", out.str(), 0, true);
	add_static("ОТВАРЫКУЗНЕЦА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫКУПЦА РЕЦЕПТЫКУПЦА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kMerchant);
	out << "\r\nСм. также: &CКУПЕЦ, УМЕНИЯКУПЦА, ЗАКЛИНАНИЯКУПЦА, СПОСОБНОСТИКУПЦА&n";
	add_static("РЕЦЕПТЫКУПЦА", out.str(), 0, true);
	add_static("ОТВАРЫКУПЦА", out.str(), 0, true);

	out.str("");
	out << "ОТВАРЫВОЛХВА РЕЦЕПТЫВОЛХВА\r\n\r\n";
	out << OutRecipiesHelp(ECharClass::kMagus	);
	out << "\r\nСм. также: &CВОЛХВ, УМЕНИЯВОЛХВА, ЗАКЛИНАНИЯВОЛХВА, СПОСОБНОСТИВОЛХВА&n";
	add_static("РЕЦЕПТЫВОЛХВА", out.str(), 0, true);
	add_static("ОТВАРЫВОЛХВА", out.str(), 0, true);
}

void init_group_zones() {
	std::stringstream out;

	out << "ГРУППОВЫЕЗОНЫ\r\n\r\n";
	for (std::size_t rnum = 0; rnum < zone_table.size(); ++rnum) {
		const auto group = zone_table[rnum].group;
		if (group > 1) {
			out << boost::format("  %2d - %s (гр. %d+).\r\n") % (1 + rnum) % zone_table[rnum].name % group;
		}
	}
	out << "\r\nСм. также: &CЗОНЫ&n";
	add_static("групповыезоны", out.str(), 0, true);
}

void check_update_dynamic() {
	if (need_update) {
		need_update = false;
		reload(DYNAMIC);
	}
}

void reload(Flags flag) {
	switch (flag) {
		case STATIC: static_help.clear();
			world_loader.index_boot(DB_BOOT_HLP);
			init_group_zones();
			init_zone_all();
			ClassRecipiesHelp();
			ClassSlillHelp();
			ClassFeatureHelp();
			PrintActivators::process();
			obj_sets::init_xhelp();
			// итоговая сортировка массива через дефолтное < для строковых ключей
			std::sort(static_help.begin(), static_help.end(),
					  [](const help_node &lrs, const help_node &rhs) {
						  return lrs.keyword < rhs.keyword;
					  });
			break;
		case DYNAMIC: dynamic_help.clear();
			SetsDrop::init_xhelp();
			SetsDrop::init_xhelp_full();
			ClanSystem::init_xhelp();
			need_update = false;
			std::sort(dynamic_help.begin(), dynamic_help.end(),
					  [](const help_node &lrs, const help_node &rhs) {
						  return lrs.keyword < rhs.keyword;
					  });
			break;
		default: log("SYSERROR: wrong flag = %d (%s %s %d)", flag, __FILE__, __func__, __LINE__);
	};
}

void reload_all() {
	reload(STATIC);
	reload(DYNAMIC);
}

bool help_compare(const std::string &arg, const std::string &text, bool strong) {
	if (strong) {
		return arg == text;
	}

	return isname(arg, text);
}

void UserSearch::process(int flag) {
	switch (flag) {
		case STATIC: search(static_help);
			break;
		case DYNAMIC: search(dynamic_help);
			break;
		default: log("SYSERROR: wrong flag = %d (%s %s %d)", flag, __FILE__, __func__, __LINE__);
	};
}

void UserSearch::print_not_found() const {
	snprintf(buf, sizeof(buf), "%s uses command HELP: %s (not found)", GET_NAME(ch), arg_str.c_str());
	mudlog(buf, LGH, kLvlImmortal, SYSLOG, true);
	snprintf(buf, sizeof(buf),
			 "&WПо вашему запросу '&w%s&W' ничего не было найдено.&n\r\n"
			 "\r\n&cИнформация:&n\r\n"
			 "Если применять команду \"справка\" без параметров, будут отображены основные команды,\r\n"
			 "особенно необходимые новичкам. Кроме того полезно ознакомиться с разделом &CНОВИЧОК&n.\r\n\r\n"
			 "Справочная система позволяет использовать в запросе индексацию разделов и строгий поиск.\r\n\r\n"
			 "%s",
			 arg_str.c_str(),
			 HELP_USE_EXMAPLES);
	SendMsgToChar(buf, ch);
}

void UserSearch::print_curr_topic(const help_node &node) const {
	if (node.sets_drop_page) {
		// распечатка таймера до следующего релоада таблицы дропа сетов
		SetsDrop::print_timer_str(ch);
	}
	if (!node.no_immlog) {
		snprintf(buf, sizeof(buf), "%s uses command HELP: %s (read)",
				 GET_NAME(ch), arg_str.c_str());
		mudlog(buf, LGH, kLvlImmortal, SYSLOG, true);
	}
	page_string(ch->desc, node.entry);
}

void UserSearch::print_key_list() const {
	// конкретный раздел справки
	// печатается если нашлось всего одно вхождение
	// или все вхождения были дублями одного топика с разными ключами
	if (!key_list.empty() && (!diff_keys || key_list.size() == 1)) {
		print_curr_topic(*(key_list[0]));
		return;
	}
	// список найденных топиков
	std::stringstream out;
	out << "&WПо вашему запросу '&w" << arg_str << "&W' найдены следующие разделы справки:&n\r\n\r\n";
	for (unsigned i = 0, count = 1; i < key_list.size(); ++i, ++count) {
		out << boost::format("|&C %-23.23s &n|") % key_list[i]->keyword;
		if ((count % 3) == 0) {
			out << "\r\n";
		}
	}

	out << "\r\n\r\n"
		   "Для получения справки по интересующему разделу, введите его название полностью,\r\n"
		   "либо воспользуйтесь индексацией или строгим поиском.\r\n\r\n"
		<< HELP_USE_EXMAPLES;

	snprintf(buf, sizeof(buf), "%s uses command HELP: %s (list)", GET_NAME(ch), arg_str.c_str());
	mudlog(buf, LGH, kLvlImmortal, SYSLOG, true);
	page_string(ch->desc, out.str());
}

void UserSearch::search(const std::vector<help_node> &cont) {
	// поиск в сортированном по ключам массиве через lower_bound
	auto i =
		std::lower_bound(cont.begin(), cont.end(), arg_str,
						 [](const help_node &h, const std::string& arg) {
							 return h.keyword < arg;
						 });

	while (i != cont.end()) {
		// проверка текущего кея с учетом флага строгости
		if (!help_compare(arg_str, i->keyword, strong)) {
			return;
		}
		// уровень топика (актуально для статик справки)
		if (level < i->min_level) {
			++i;
			continue;
		}
		// key_list заполняется в любом случае, если поиск
		// идет без индекса topic_num, уникальность содержимого
		// для последующих проверок отражается через diff_keys
		for (auto & k : key_list) {
			if (k->entry != i->entry) {
				diff_keys = true;
				break;
			}
		}

		if (!topic_num) {
			key_list.push_back(i);
		} else {
			++curr_topic_num;
			// нашли запрос вида х.строка
			if (curr_topic_num == topic_num) {
				print_curr_topic(*i);
				stop = true;
				return;
			}
		}
		++i;
	}
}

} // namespace HelpSystem
////////////////////////////////////////////////////////////////////////////////

using namespace HelpSystem;

void do_help(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->desc) {
		return;
	}

	skip_spaces(&argument);

	// печатаем экран справки если нет аргументов
	if (!*argument) {
		page_string(ch->desc, help, 0);
		return;
	}

	UserSearch user_search(ch);
	// trust_level справки для демигодов - kLevelImmortal
	user_search.level = GET_GOD_FLAG(ch, EGf::kDemigod) ? kLvlImmortal : GetRealLevel(ch);
	// первый аргумент без пробелов, заодно в нижний регистр
	one_argument(argument, arg);
	// Получаем topic_num для индексации топика
	sscanf(arg, "%d.%s", &user_search.topic_num, arg);
	// если последний символ аргумента '!' -- включаем строгий поиск
	if (strlen(arg) > 1 && *(arg + strlen(arg) - 1) == '!') {
		user_search.strong = true;
		*(arg + strlen(arg) - 1) = '\0';
	}
	user_search.arg_str = arg;

	// поиск по всем массивам или до стопа по флагу
	for (int i = STATIC; i < TOTAL_NUM && !user_search.stop; ++i) {
		user_search.process(i);
	}
	// если вышли по флагу, то чару уже был распечатан найденный топик вида х.строка
	// иначе распечатываем чару что-то в зависимости от состояния key_list
	if (!user_search.stop) {
		if (user_search.key_list.empty()) {
			user_search.print_not_found();
		} else {
			user_search.print_key_list();
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
