// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "help.h"

#include <third_party_libs/fmt/include/fmt/format.h>

#include "obj_prototypes.h"
#include "engine/ui/modify.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/sets_drop.h"
#include "engine/ui/color.h"
#include "global_objects.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/mechanics/obj_sets_stuff.h"
#include "gameplay/mechanics/player_races.h"

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
	if (skill.second != 0) {
		char buf[128];

		sprintf(buf, "%s%s%s%s%d%s\r\n",
				(activ ? " +    " : "   "),
				kColorCyn,
				MUD::Skill(skill.first).GetName(),
				(skill.second < 0 ? " ухудшает на " : " улучшает на "),
				abs(skill.second),
				kColorNrm);
		return buf;
	}
	return "";
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

	out << obj->get_PName(ECase::kNom) << "\r\n";

	if (obj->get_no_flags().sprintbits(no_bits, buf2, ",")) {
		out << "Неудобства : " << buf2 << "\r\n";
	}

	if (obj->get_type() == EObjType::kWeapon) {
		const int drndice = GET_OBJ_VAL(obj, 1);
		const int drsdice = GET_OBJ_VAL(obj, 2);
		out << fmt::format("Наносимые повреждения '{}D{}' среднее {:.1}\r\n",
			drndice, drsdice, (drsdice + 1) * drndice / 2.0);
	}

	if (obj->get_type() == EObjType::kWeapon
		|| CAN_WEAR(obj, EWearFlag::kShield)
		|| CAN_WEAR(obj, EWearFlag::kHands)) {
		out << "Вес : " << obj->get_weight() << "\r\n";
	}

	if (obj->get_affect_flags().sprintbits(weapon_affects, buf2, ",")) {
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
	out << "\r\n";

	FlagData affects = activ->second.get_affects();
	if (affects.sprintbits(weapon_affects, buf2, ",")) {
		out << " + Аффекты : " << buf2 << "\r\n";
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

	if (obj->get_type() == EObjType::kWeapon) {
		int drndice = 0, drsdice = 0;
		activ->second.get_dices(drsdice, drndice);
		if (drsdice > 0 && drndice > 0) {
			out << fmt::format(" + Устанавливает наносимые повреждения '{}D{}' среднее {:.1}\r\n",
				drndice, drsdice, (drsdice + 1) * drndice / 2.0);
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
		const int rnum = GetObjRnum(k->first);
		if (rnum < 0) {
			continue;
		}
		const auto &obj = obj_proto[rnum];

		// суммируем родные статы со шмоток
		activ.native_no_flag += obj->get_no_flags();
		activ.native_affects += obj->get_affect_flags();
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
			const int rnum = GetObjRnum(k.first);
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
			HelpSystem::add_static(utils::EraseAllAny(set_name, " ,."), out.str(), 0, true);
		} else {
			std::string alias = it.second.get_alias();
			for (auto & k : utils::Split(alias, ',')) {
				HelpSystem::add_static(set_name + "сет" + utils::EraseAllAny(k, " ,."), out.str(), 0, true);
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
	std::string tmpentry = utils::SubstStrToUpper(key) + "\r\n\r\n" + entry;
	help_node tmp_node(key, tmpentry);
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

	for (std::size_t rnum = 0, i = 1; rnum < zone_table.size(); ++rnum) {
		if (!zone_table[rnum].location.empty()) {
			out << fmt::format("  {:<2} - {}. Расположена: {}. Группа: {}. Примерный уровень: {}.\r\n",
					i, zone_table[rnum].name, zone_table[rnum].location,
					zone_table[rnum].group, zone_table[rnum].level);
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

std::string OutAllSkillsHelp() {
	std::stringstream out;
	std::vector<std::string> skills_list;
	std::string tmpstr;
	int columns = 0;

	out << "Список возможных умений:\r\n";
	for (const auto &skill : MUD::Skills()) {
		if (skill.IsInvalid())
			continue;
		skills_list.push_back(skill.GetName());
	}
	utils::SortKoiString(skills_list);
	for (auto it : skills_list) {
		tmpstr = !(++columns % 3) ? "\r\n" : "\t";
		out << fmt::format("\t{:<30} {}", it, tmpstr);
	}
	if (out.str().back() == '\t')
		out << "\r\n";
	return out.str();
}

std::string OutAllRecipiesHelp() {
	std::stringstream out;
	std::vector<std::string> recipies_list;
	std::string tmpstr;
	int columns = 0;

	out << "Список возможных рецептов:\r\n";
	for (int sortpos = 0; sortpos <= top_imrecipes; sortpos++) {
		recipies_list.push_back(imrecipes[sortpos].name);
	}
	utils::SortKoiString(recipies_list);
	for (auto it : recipies_list) {
		tmpstr = !(++columns % 3) ? "\r\n" : "\t";
		out << fmt::format("\t{:<30} {}", it, tmpstr);
	}
	if (out.str().back() == '\t')
		out << "\r\n";
	return out.str();
}

std::string OutAllFeaturesHelp() {
	std::stringstream out;
	std::vector<std::string> feats_list;
	std::string tmpstr;
	int columns = 0;

	out << "Список возможных способносей:\r\n";
	for (const auto &feat : MUD::Feats()) {
		if (feat.IsUnavailable()) {
			continue;
		}
		feats_list.push_back(MUD::Feat(feat.GetId()).GetCName());
	}
	utils::SortKoiString(feats_list);
	for (auto it : feats_list) {
		tmpstr = !(++columns % 3) ? "\r\n" : "\t";
		out << fmt::format("\t{:<30} {}", it, tmpstr);
	}
	if (out.str().back() == '\t')
		out << "\r\n";
	return out.str();
}

std::string OutAllSpellsHelp() {
	std::stringstream out;
	std::vector<std::string> spells_list;
	std::string tmpstr;
	int columns = 0;

	out << "Список возможных заклинаний:\r\n";
	for (auto &it : MUD::Spells()) {
		auto spell_id = it.GetId();

		if (MUD::Spell(spell_id).IsInvalid())
			continue;
		spells_list.push_back(it.GetName());
	}
	utils::SortKoiString(spells_list);
	for (auto it : spells_list) {
		tmpstr = !(++columns % 3) ? "\r\n" : "\t";
		out << fmt::format("\t{:<30} {}", it, tmpstr);
	}
	if (out.str().back() == '\t')
		out << "\r\n";
	return out.str();
}

std::string OutSkillsHelp(ECharClass ch_class) {
	std::stringstream out, out2;
	std::string tmpstr;
	int columns = 0, columns2 = 0;
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

std::string OutMagusSpellsHelp() {
	std::string out, out2, out3;
	std::string tmpstr;
	std::vector<std::string> spells_list;
	std::vector<std::string> spells_list2;
	std::vector<std::string> spells_list3;

	out = "Обычные заклинания:\r\n";
	out2 = "\r\n&GУникальные для какого-то одного класса заклинания:&n\r\n";
	out3 = "\r\n&BУникальные заклинания:&n\r\n";
	for (const auto &spl_info : MUD::Spells()) {
		auto spell_id = spl_info.GetId();
		int num = 0;

		if (MUD::Spell(spell_id).IsInvalid())
			continue;
		if (!spell_create.contains(spell_id))
			continue;
		for (const auto &char_class: MUD::Classes()) {
			if (char_class.IsAvailable()) {
				if (char_class.GetId() == ECharClass::kMagus) 
					continue;
				for (const auto &spell : char_class.spells) {
					if (spell.GetId() == spell_id) {
						++num;
					}
				}
			}
		}
		if (num > 1) {
			spells_list.push_back(MUD::Spell(spell_id).GetCName());
		}
		if (num == 1) {
			spells_list2.push_back(MUD::Spell(spell_id).GetCName());
		}
		if (num == 0) {
			spells_list3.push_back(MUD::Spell(spell_id).GetCName());
		}
	}
	int columns = 0;

	utils::SortKoiString(spells_list);
	for (auto it : spells_list) {
		tmpstr = !(++columns % 3) ? "\r\n" : "\t";
		out += fmt::format("\t{:<30} {}", it, tmpstr);
	}
	columns = 0;
	utils::SortKoiString(spells_list2);
	for (auto it : spells_list2) {
		tmpstr = !(++columns % 3) ? "\r\n" : "\t";
		out2 += fmt::format("\t{:<30} {}", it, tmpstr);
	}
	columns = 0;
	utils::SortKoiString(spells_list3);
	for (auto it : spells_list3) {
		tmpstr = !(++columns % 3) ? "\r\n" : "\t";
		out3 += fmt::format("\t{:<30} {}", it, tmpstr);
	}
	if (out.back() == '\t')
		out += "\r\n";
	if (out2.back() == '\t')
		out2 += "\r\n";
	if (out3.back() == '\t')
		out3 += "\r\n";
	out += out2 + out3;
	return out;
}

std::string OutCasterSpellsHelp(ECharClass ch_class) {
	std::string out, out2, out3, out4;
	std::string tmpstr;
	std::vector<std::string> spells_list;
	std::vector<std::string> spells_list2;
	std::vector<std::string> spells_list3;
	std::vector<std::string> spells_list4;

	out = "Обычные заклинания:\r\n";
	out2 = "\r\n&GЗаклинания, доступные после одного или нескольких перевоплощений:&n\r\n";
	out3 = "\r\n&RЗаклинания вашего класса, начертанные рунами:&n\r\n";
	out4 = "\r\n&WУникальные заклинания:&n\r\n";
	for (auto class_spell : MUD::Class(ch_class).spells) {
		int num = 0;

		if (class_spell.IsUnavailable()) {
			continue;
		}
		if (spell_create.contains(class_spell.GetId())) {
			spells_list3.push_back(MUD::Spell(class_spell.GetId()).GetCName());
		}
		if (class_spell.GetMinRemort() > 0) {
			spells_list2.push_back(MUD::Spell(class_spell.GetId()).GetCName());
			continue;
		}
		for (const auto &char_class: MUD::Classes()) {
			if (char_class.IsAvailable()) {
				for (const auto &spell : char_class.spells) {
					if (spell.GetId() == class_spell.GetId()) {
						++num;
					}
				}
			}
		}
		if (num == 1) {
			spells_list4.push_back(MUD::Spell(class_spell.GetId()).GetCName());
			continue;
		}
		spells_list.push_back(MUD::Spell(class_spell.GetId()).GetCName());
	}
	int columns = 0;

	utils::SortKoiString(spells_list);
	for (auto it : spells_list) {
		tmpstr = !(++columns % 2) ? "\r\n" : "\t";
		out += fmt::format("\t{:<30} {}", it, tmpstr);
	}
	columns = 0;
	utils::SortKoiString(spells_list2);
	for (auto it : spells_list2) {
		tmpstr = !(++columns % 2) ? "\r\n" : "\t";
		out2 += fmt::format("\t{:<30} {}", it, tmpstr);
	}
	columns = 0;
	utils::SortKoiString(spells_list3);
	for (auto it : spells_list3) {
		tmpstr = !(++columns % 2) ? "\r\n" : "\t";
		out3 += fmt::format("\t{:<30} {}", it, tmpstr);
	}
	columns = 0;
	utils::SortKoiString(spells_list4);
	for (auto it : spells_list4) {
		tmpstr = !(++columns % 2) ? "\r\n" : "\t";
		out4 += fmt::format("\t{:<30} {}", it, tmpstr);
	}
	if (out.back() == '\t')
		out += "\r\n";
	if (out2.back() == '\t')
		out2 += "\r\n";
	if (out3.back() == '\t')
		out3 += "\r\n";
	if (out4.back() == '\t')
		out4 += "\r\n";
	out += out2 + out4 + out3;
	return out;
}

void CasterSpellslHelp() {
	std::string out;

	out = OutCasterSpellsHelp(ECharClass::kSorcerer);
	out += "\r\nСм. также: &CЛЕКАРЬ, УМЕНИЯЛЕКАРЯ, СПОСОБНОСТИЛЕКАРЯ, ОТВАРЫЛЕКАРЯ";
	add_static("ЗАКЛИНАНИЯЛЕКАРЯ", out, 0, true);

	out = OutCasterSpellsHelp(ECharClass::kConjurer);
	out += "\r\nСм. также: &CКОЛДУН, УМЕНИЯКОЛДУНА, СПОСОБНОСТИКОЛДУНА, ОТВАРЫКОЛДУНА&n";
	add_static("ЗАКЛИНАНИЯКОЛДУНА", out, 0, true);

	out = OutCasterSpellsHelp(ECharClass::kCharmer);
	out += "\r\nСм. также: &CКУДЕСНИК, УМЕНИЯКУДЕСНИКА, СПОСОБНОСТИКУДЕСНИКА, ОТВАРЫКУДЕСНИКА&n";
	add_static("ЗАКЛИНАНИЯКУДЕСНИКА", out, 0, true);

	out = OutCasterSpellsHelp(ECharClass::kWizard);
	out += "\r\nСм. также: &CВОЛШЕБНИК, УМЕНИЯВОЛШЕБНИКА, СПОСОБНОСТИВОЛШЕБНИКА, ОТВАРЫВОЛШЕБНИКА&n";
	add_static("ЗАКЛИНАНИЯВОЛШЕБНИКА", out, 0, true);

	out = OutCasterSpellsHelp(ECharClass::kNecromancer);
	out += "\r\nСм. также: &CЧЕРНОКНИЖНИК, УМЕНИЯЧЕРНОКНИЖНИКА, СПОСОБНОСТИЧЕРНОКНИЖНИКА, ОТВАРЫЧЕРНОКНИЖНИКА&n";
	add_static("ЗАКЛИНАНИЯЧЕРНОКНИЖНИКА", out, 0, true);

	out = OutCasterSpellsHelp(ECharClass::kPaladine);
	out += "\r\nСм. также: &CВИТЯЗЬ, УМЕНИЯВИТЯЗЯ, СПОСОБНОСТИВИТЯЗЯ, ОТВАРЫВИТЯЗЯ&n";
	add_static("ЗАКЛИНАНИЯВИТЯЗЯ", out, 0, true);

	out = OutCasterSpellsHelp(ECharClass::kMerchant);
	out += "\r\nСм. также: &CКУПЕЦ, УМЕНИЯКУПЦА, СПОСОБНОСТИКУПЦА, ОТВАРЫКУПЦА&n";
	add_static("ЗАКЛИНАНИЯКУПЦА", out, 0, true);

	out = OutMagusSpellsHelp();
	out += "\r\nСм. также: &CВОЛХВ, УМЕНИЯВОЛХВА, СПОСОБНОСТИВОЛХВА, ОТВАРЫВОЛХВА&n";
	add_static("ЗАКЛИНАНИЯВОЛХВА", out, 0, true);
}

std::string OutFeatureHelp(ECharClass ch_class) {
	std::stringstream out, out2, out3;
	std::string tmpstr;
	int columns = 0, columns2 = 0, columns3 = 0;
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

	out << OutFeatureHelp(ECharClass::kSorcerer);
	out << "\r\nСм. также: &CЛЕКАРЬ, ЗАКЛИНАНИЯЛЕКАРЯ, УМЕНИЯЛЕКАРЯ, ОТВАРЫЛЕКАРЯ&n";
	add_static("СПОСОБНОСТИЛЕКАРЯ", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kConjurer);
	out << "\r\nСм. также: &CКОЛДУН, ЗАКЛИНАНИЯКОЛДУНА, УМЕНИЯКОЛДУНА, ОТВАРЫКОЛДУНА&n";
	add_static("СПОСОБНОСТИКОЛДУНА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kThief);
	out << "\r\nСм. также: &CТАТЬ, УМЕНИЯТАТЯ, ОТВАРЫТАТЯ&n";
	add_static("СПОСОБНОСТИТАТЯ", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kWarrior);
	out << "\r\nСм. также: &CБОГАТЫРЬ, УМЕНИЯБОГАТЫРЯ, ОТВАРЫБОГАТЫРЯ&n";
	add_static("СПОСОБНОСТИБОГАТЫРЯ", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kAssasine);
	out << "\r\nСм. также: &CНАЕМНИК, УМЕНИЯНАЕМНИКА, ОТВАРЫНАЕМНИКА&n";
	add_static("СПОСОБНОСТИНАЕМНИКА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kGuard);
	out << "\r\nСм. также: &CДРУЖИННИК, УМЕНИЯДРУЖИННИКА, ОТВАРЫДРУЖИННИКА&n";
	add_static("СПОСОБНОСТИДРУЖИННИКА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kCharmer);
	out << "\r\nСм. также:КУДЕСНИК, ОТВАРЫКУДЕСНИКА, ЗАКЛИНАНИЯКУДЕСНИКА, УМЕНИЯКУДЕСНИКА&n";
	add_static("СПОСОБНОСТИКУДЕСНИКА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kWizard);
	out << "\r\nСм. также: &CВОЛШЕБНИК, ОТВАРЫВОЛШЕБНИКА, ЗАКЛИНАНИЯВОЛШЕБНИКА, УМЕНИЯВОЛШЕБНИКА&n";
	add_static("СПОСОБНОСТИВОЛШЕБНИКА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kNecromancer);
	out << "\r\nСм. также: &CЧЕРНОКНИЖНИК, ОТВАРЫЧЕРНОКНИЖНИКА, ЗАКЛИНАНИЯЧЕРНОКНИЖНИКА, УМЕНИЯЧЕРНОКНИЖНИКА&n";
	add_static("СПОСОБНОСТИЧЕРНОКНИЖНИКА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kPaladine);
	out << "\r\nСм. также: &CВИТЯЗЬ, ОТВАРЫВИТЯЗЯ, ЗАКЛИНАНИЯВИТЯЗЯ, УМЕНИЯВИТЯЗЯ&n";
	add_static("СПОСОБНОСТИВИТЯЗЯ", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kRanger);
	out << "\r\nСм. также: &CОХОТНИК, ОТВАРЫОХОТНИКА, УМЕНИЯОХОТНИКА&n";
	add_static("СПОСОБНОСТИОХОТНИКА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kVigilant);
	out << "\r\nСм. также: &CКУЗНЕЦ, ОТВАРЫКУЗНЕЦА, УМЕНИЯКУЗНЕЦА&n";
	add_static("СПОСОБНОСТИКУЗНЕЦА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kMerchant);
	out << "\r\nСм. также: &CКУПЕЦ, ОТВАРЫКУПЦА, ЗАКЛИНАНИЯКУПЦА, УМЕНИЯКУПЦА&n";
	add_static("СПОСОБНОСТИКУПЦА", out.str(), 0, true);

	out.str("");
	out << OutFeatureHelp(ECharClass::kMagus);
	out << "\r\nСм. также: &CВОЛХВ, ОТВАРЫВОЛХВА, ЗАКЛИНАНИЯВОЛХВА, УМЕНИЯВОЛХВА&n";
	add_static("СПОСОБНОСТИВОЛХВА", out.str(), 0, true);
}

void AllHelp() {
	std::stringstream out;

	out << OutAllSkillsHelp();
	out << "\r\nСм. также: &CЗАКЛИНАНИЯВСЕ&n, &CОТВАРЫВСЕ&n, &CСПОСОБНОСТИВСЕ&n";
	add_static("УМЕНИЯВСЕ", out.str(), 0, true);

	out.str("");
	out << OutAllSpellsHelp();
	out << "\r\nСм. также: &CУМЕНИЯВСЕ&n, &CОТВАРЫВСЕ&n, &CСПОСОБНОСТИВСЕ&n";
	add_static("ЗАКЛИНАНИЯВСЕ", out.str(), 0, true);

	out.str("");
	out << OutAllFeaturesHelp();
	out << "\r\nСм. также: &CУМЕНИЯВСЕ&n, &CЗАКЛИНАНИЯВСЕ&n, &CОТВАРЫВСЕ&n";
	add_static("СПОСОБНОСТИВСЕ", out.str(), 0, true);

	out.str("");
	out << OutAllRecipiesHelp();
	out << "\r\nСм. также: &CУМЕНИЯВСЕ&n, &CЗАКЛИНАНИЯВСЕ&n, &CСПОСОБНОСТИВСЕ&n";
	add_static("ОТВАРЫВСЕ", out.str(), 0, true);
}


void ClassSkillHelp() {
	std::stringstream out;

	out << OutSkillsHelp(ECharClass::kSorcerer);
	out << "\r\nСм. также: &CЛЕКАРЬ, ЗАКЛИНАНИЯЛЕКАРЯ, СПОСОБНОСТИЛЕКАРЯ, ОТВАРЫЛЕКАРЯ&n";
	add_static("УМЕНИЯЛЕКАРЯ", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kConjurer);
	out << "\r\nСм. также: &CКОЛДУН, ЗАКЛИНАНИЯКОЛДУНА, СПОСОБНОСТИКОЛДУНА, ОТВАРЫКОЛДУНА&n";
	add_static("УМЕНИЯКОЛДУНА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kThief);
	out << "\r\nСм. также: &CТАТЬ, СПОСОБНОСТИТАТЯ, ОТВАРЫТАТЯ&n";
	add_static("УМЕНИЯТАТЯ", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kWarrior);
	out << "\r\nСм. также: &CБОГАТЫРЬ, СПОСОБНОСТИБОГАТЫРЯ, ОТВАРЫБОГАТЫРЯ&n";
	add_static("УМЕНИЯБОГАТЫРЯ", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kAssasine);
	out << "\r\nСм. также: &CНАЕМНИК, СПОСОБНОСТИНАЕМНИКА, ОТВАРЫНАЕМНИКА&n";
	add_static("УМЕНИЯНАЕМНИКА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kGuard);
	out << "\r\nСм. также: &CДРУЖИННИК, СПОСОБНОСТИДРУЖИННИКА, ОТВАРЫДРУЖИННИКА&n";
	add_static("УМЕНИЯДРУЖИННИКА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kCharmer);
	out << "\r\nСм. также:КУДЕСНИК, ОТВАРЫКУДЕСНИКА, ЗАКЛИНАНИЯКУДЕСНИКА, СПОСОБНОСТИКУДЕСНИКА&n";
	add_static("УМЕНИЯКУДЕСНИКА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kWizard);
	out << "\r\nСм. также: &CВОЛШЕБНИК, ОТВАРЫВОЛШЕБНИКА, ЗАКЛИНАНИЯВОЛШЕБНИКА, СПОСОБНОСТИВОЛШЕБНИКА&n";
	add_static("УМЕНИЯВОЛШЕБНИКА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kNecromancer);
	out << "\r\nСм. также: &CЧЕРНОКНИЖНИК, ОТВАРЫЧЕРНОКНИЖНИКА, ЗАКЛИНАНИЯЧЕРНОКНИЖНИКА, СПОСОБНОСТИЧЕРНОКНИЖНИКА&n";
	add_static("УМЕНИЯЧЕРНОКНИЖНИКА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kPaladine);
	out << "\r\nСм. также: &CВИТЯЗЬ, ОТВАРЫВИТЯЗЯ, ЗАКЛИНАНИЯВИТЯЗЯ, СПОСОБНОСТИВИТЯЗЯ&n";
	add_static("УМЕНИЯВИТЯЗЯ", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kRanger);
	out << "\r\nСм. также: &CОХОТНИК, ОТВАРЫОХОТНИКА, СПОСОБНОСТИОХОТНИКА&n";
	add_static("УМЕНИЯОХОТНИКА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kVigilant);
	out << "\r\nСм. также: &CКУЗНЕЦ, ОТВАРЫКУЗНЕЦА, СПОСОБНОСТИКУЗНЕЦА&n";
	add_static("УМЕНИЯКУЗНЕЦА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kMerchant);
	out << "\r\nСм. также: &CКУПЕЦ, ОТВАРЫКУПЦА, ЗАКЛИНАНИЯКУПЦА, СПОСОБНОСТИКУПЦА&n";
	add_static("УМЕНИЯКУПЦА", out.str(), 0, true);

	out.str("");
	out << OutSkillsHelp(ECharClass::kMagus	);
	out << "\r\nСм. также: &CВОЛХВ, ОТВАРЫВОЛХВА, ЗАКЛИНАНИЯВОЛХВА, СПОСОБНОСТИВОЛХВА&n";
	add_static("УМЕНИЯВОЛХВА", out.str(), 0, true);
}

void ClassRecipiesHelp() {
	std::stringstream out;

	out << OutRecipiesHelp(ECharClass::kSorcerer);
	out << "\r\nСм. также: &CЛЕКАРЬ, УМЕНИЯЛЕКАРЯ, ЗАКЛИНАНИЯЛЕКАРЯ, СПОСОБНОСТИЛЕКАРЯ&n";
	add_static("РЕЦЕПТЫЛЕКАРЯ", out.str(), 0, true);
	add_static("ОТВАРЫЛЕКАРЯ", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kConjurer);
	out << "\r\nСм. также: &CКОЛДУН, УМЕНИЯКОЛДУНА, ЗАКЛИНАНИЯКОЛДУНА, СПОСОБНОСТИКОЛДУНА&n";
	add_static("РЕЦЕПТЫКОЛДУНА", out.str(), 0, true);
	add_static("ОТВАРЫКОЛДУНА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kThief);
	out << "\r\nСм. также: &CТАТЬ, УМЕНИЯТАТЯ, СПОСОБНОСТИТАТЯ&n";
	add_static("РЕЦЕПТЫТАТЯ", out.str(), 0, true);
	add_static("ОТВАРЫТАТЯ", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kWarrior);
	out << "\r\nСм. также: &CБОГАТЫРЬ, УМЕНИЯБОГАТЫРЯ, СПОСОБНОСТИБОГАТЫРЯ&n";
	add_static("РЕЦЕПТЫБОГАТЫРЯ", out.str(), 0, true);
	add_static("ОТВАРЫБОГАТЫРЯ", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kAssasine);
	out << "\r\nСм. также: &CНАЕМНИК, УМЕНИЯНАЕМНИКА, СПОСОБНОСТИНАЕМНИКА&n";
	add_static("РЕЦЕПТЫНАЕМНИКА", out.str(), 0, true);
	add_static("ОТВАРЫНАЕМНИКА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kGuard);
	out << "\r\nСм. также: &CДРУЖИННИК, УМЕНИЯДРУЖИННИКА, СПОСОБНОСТИДРУЖИННИКА&n";
	add_static("РЕЦЕПТЫДРУЖИННИКА", out.str(), 0, true);
	add_static("ОТВАРЫДРУЖИННИКА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kCharmer);
	out << "\r\nСм. также:КУДЕСНИК, УМЕНИЯКУДЕСНИКА, ЗАКЛИНАНИЯКУДЕСНИКА, СПОСОБНОСТИКУДЕСНИКА&n";
	add_static("РЕЦЕПТЫКУДЕСНИКА", out.str(), 0, true);
	add_static("ОТВАРЫКУДЕСНИКА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kWizard);
	out << "\r\nСм. также: &CВОЛШЕБНИК, УМЕНИЯВОЛШЕБНИКА, ЗАКЛИНАНИЯВОЛШЕБНИКА, СПОСОБНОСТИВОЛШЕБНИКА&n";
	add_static("РЕЦЕПТЫВОЛШЕБНИКА", out.str(), 0, true);
	add_static("ОТВАРЫВОЛШЕБНИКА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kNecromancer);
	out << "\r\nСм. также: &CЧЕРНОКНИЖНИК, УМЕНИЯЧЕРНОКНИЖНИКА, ЗАКЛИНАНИЯЧЕРНОКНИЖНИКА, СПОСОБНОСТИЧЕРНОКНИЖНИКА&n";
	add_static("РЕЦЕПТЫЧЕРНОКНИЖНИКА", out.str(), 0, true);
	add_static("ОТВАРЫЧЕРНОКНИЖНИКА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kPaladine);
	out << "\r\nСм. также: &CВИТЯЗЬ, УМЕНИЯВИТЯЗЯ, ЗАКЛИНАНИЯВИТЯЗЯ, СПОСОБНОСТИВИТЯЗЯ&n";
	add_static("РЕЦЕПТЫВИТЯЗЯ", out.str(), 0, true);
	add_static("ОТВАРЫВИТЯЗЯ", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kRanger);
	out << "\r\nСм. также: &CОХОТНИК, УМЕНИЯОХОТНИКА, СПОСОБНОСТИОХОТНИКА&n";
	add_static("РЕЦЕПТЫОХОТНИКА", out.str(), 0, true);
	add_static("ОТВАРЫОХОТНИКА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kVigilant);
	out << "\r\nСм. также: &CКУЗНЕЦ, УМЕНИЯКУЗНЕЦА, СПОСОБНОСТИКУЗНЕЦА&n";
	add_static("РЕЦЕПТЫКУЗНЕЦА", out.str(), 0, true);
	add_static("ОТВАРЫКУЗНЕЦА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kMerchant);
	out << "\r\nСм. также: &CКУПЕЦ, УМЕНИЯКУПЦА, ЗАКЛИНАНИЯКУПЦА, СПОСОБНОСТИКУПЦА&n";
	add_static("РЕЦЕПТЫКУПЦА", out.str(), 0, true);
	add_static("ОТВАРЫКУПЦА", out.str(), 0, true);

	out.str("");
	out << OutRecipiesHelp(ECharClass::kMagus	);
	out << "\r\nСм. также: &CВОЛХВ, УМЕНИЯВОЛХВА, ЗАКЛИНАНИЯВОЛХВА, СПОСОБНОСТИВОЛХВА&n";
	add_static("РЕЦЕПТЫВОЛХВА", out.str(), 0, true);
	add_static("ОТВАРЫВОЛХВА", out.str(), 0, true);
}

void SetsHelp() {
	std::ostringstream out;
	std::string str_out;
	int count = 1;
	bool class_num[kNumPlayerClasses];

	out <<  "Много в мире различных вещей, но сколько бы их не одел смертный - никогда ему не бывать богом...\r\n" <<
			"Правда можно приблизиться к могуществу древнейших - собирая комплекты(сеты) - чем больше частей\r\n" <<
			"комплекта соберешь, тем ближе будешь!\r\n";
	table_wrapper::Table table;
	table << table_wrapper::kHeader << "#" << "Сокращение" << "Название" << "Профессии" << table_wrapper::kEndRow;
	for (auto &it : obj_sets::sets_list) {
		int count_class = 0;

		if (!it->enabled)
			continue;
		for (auto &k : it->activ_list) {
			for (unsigned i = 0; i < kNumPlayerClasses; ++i) {
				if (k.second.prof.test(i) == true) {
					class_num[i] = true;
				} else
					class_num[i] = false;
			}
		}
		table << count++ << utils::FirstWordOnString(it->alias, " ,;") << utils::RemoveColors(it->name);
		str_out.clear();
		for (unsigned i = 0; i < kNumPlayerClasses; i++) {
			if (class_num[i]) {
				count_class++;
				str_out += MUD::Classes().FindAvailableItem(static_cast<int>(i)).GetName() + (!(count_class % 4) ? "\n" : " ");
			}
		}
		if (count_class ==  kNumPlayerClasses)
			str_out = "все";
		else if (count_class == 0)
			str_out = "никто";
		if (str_out.back() == '\n')
			str_out.back() = '\0';
		table << str_out << table_wrapper::kSeparator << table_wrapper::kEndRow;
	}
	table.SetColumnAlign(0, table_wrapper::align::kRight);
	table_wrapper::PrintTableToStream(out, table);
	add_static("СЕТЫ", out.str(), 0, true);
}

void init_group_zones() {
	std::stringstream out;

	for (std::size_t rnum = 0; rnum < zone_table.size(); ++rnum) {
		const auto group = zone_table[rnum].group;
		if (group > 1) {
			out << fmt::format("  {:2} - {} (гр. {}+).\r\n", (1 + rnum), zone_table[rnum].name, group);
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
			world_loader.BootIndex(DB_BOOT_HLP);
			init_group_zones();
			init_zone_all();
			ClassRecipiesHelp();
			ClassSkillHelp();
			ClassFeatureHelp();
			CasterSpellslHelp();
			SetsHelp();
			AllHelp();
			PrintActivators::process();
			obj_sets::init_xhelp();
			// итоговая сортировка массива через дефолтное < для строковых ключей 
//			for (auto &recode : static_help) {
//				utils::ConvertKtoW(recode.keyword);
//			}
			std::sort(static_help.begin(), static_help.end(),
					  [](const help_node &lrs, const help_node &rhs) {
						  return lrs.keyword < rhs.keyword;
					  });
//			for (auto &recode : static_help) {
//				sprintf(buf, "sort keyword=%s|", recode.keyword.c_str());
//				mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
//			}
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
	std::string name = arg; 

	if (strong) {

		sprintf(buf, "strong arg=%s| text=%s|",arg.c_str(), text.c_str());
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return arg == text;
	}
	return utils::IsAbbr(name, text);
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
		out << fmt::format("|&C {:<23.23} &n|", key_list[i]->keyword);
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
	std::vector<help_node> cont1 = cont;
//  в help_node список сортирован в кодировке win но сам в кои
	auto i = std::lower_bound(cont.begin(), cont.end(), arg_str, [](const help_node &h, const std::string& arg) {
//		sprintf(buf, "arg_str=%s| keyword=%s|",arg.c_str(), h.keyword.c_str());
//		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return h.keyword < arg;
	});
	while (i != cont.end()) {
		// проверка текущего кея с учетом флага строгости
//		sprintf(buf, "2222 arg_str=%s| keyword=%s|",arg_str.c_str(), i->keyword.c_str());
//		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		if (!help_compare(arg_str, i->keyword, strong)) {
			return;
		}
//sprintf(buf, "compare");
//mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);

		// уровень топика (актуально для статик справки)
		if (level < i->min_level) {
			++i;
			continue;
		}
//sprintf(buf, "level");
//mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
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
	std::string arg_str(argument);

	if (!ch->desc) {
		return;
	}
	// печатаем экран справки если нет аргументов
	if (!*argument) {
		page_string(ch->desc, help, 0);
		return;
	}
	if (arg_str.size() > 100) {
		SendMsgToChar(ch, "Слишком длинная строка в запросе справки.\r\n");
		return;
	}
	UserSearch user_search(ch);
	// trust_level справки для демигодов - kLevelImmortal
	user_search.level = GET_GOD_FLAG(ch, EGf::kDemigod) ? kLvlImmortal : GetRealLevel(ch);
	utils::ConvertToLow(arg_str);
	// Парсинг topic_num
	size_t dot_pos = arg_str.find('.');
    
	if (dot_pos != std::string::npos) {
		try {
			user_search.topic_num = std::stoi(arg_str.substr(0, dot_pos));
			arg_str = arg_str.substr(dot_pos + 1);
		} catch (...) {
			user_search.topic_num = 0;
		}
	}
	// если последний символ аргумента '!' -- включаем строгий поиск
	if (arg_str.size() > 1 && arg_str.back() == '!') {
		user_search.strong = true;
		user_search.arg_str = arg_str;
	} else {
		user_search.arg_str = arg_str;
	}
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
