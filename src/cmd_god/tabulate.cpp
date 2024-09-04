//
// Created by Sventovit on 04.09.2024.
//

#include "entities/char_data.h"
#include "game_magic/magic_utils.h"
#include "modify.h"
#include "obj_prototypes.h"
#include "olc/olc.h"
#include "structs/global_objects.h"
#include "utils/utils_string.h"

#include <third_party_libs/fmt/include/fmt/format.h>

int TabulateMobsByName(char *searchname, CharData *ch);
int TabulateObjsByAliases(char *searchname, CharData *ch);
int TabulateObjsByFlagName(char *searchname, CharData *ch);
int TabulateRoomsByName(char *searchname, CharData *ch);
int TabulateTrigsByObjLoad(char *searchname, CharData *ch);
int TabulateMobsByDeadLoad(char *vnum, CharData *ch);

void DoTabulate(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2
		|| (!utils::IsAbbr(buf, "mob")
			&& !utils::IsAbbr(buf, "obj")
			&& !utils::IsAbbr(buf, "room")
			&& !utils::IsAbbr(buf, "flag")
			&& !utils::IsAbbr(buf, "существо")
			&& !utils::IsAbbr(buf, "предмет")
			&& !utils::IsAbbr(buf, "флаг")
			&& !utils::IsAbbr(buf, "комната")
			&& !utils::IsAbbr(buf, "trig")
			&& !utils::IsAbbr(buf, "триггер")
			&& !utils::IsAbbr(buf, "load"))) {
		SendMsgToChar("Usage: vnum { obj | mob | flag | room | trig | load} <name>\r\n", ch);
		return;
	}

	if ((utils::IsAbbr(buf, "mob")) || (utils::IsAbbr(buf, "существо"))) {
		if (!TabulateMobsByName(buf2, ch)) {
			SendMsgToChar("Нет существа с таким именем.\r\n", ch);
		}
	}

	if ((utils::IsAbbr(buf, "obj")) || (utils::IsAbbr(buf, "предмет"))) {
		if (!TabulateObjsByAliases(buf2, ch)) {
			SendMsgToChar("Нет предмета с таким названием.\r\n", ch);
		}
	}

	if ((utils::IsAbbr(buf, "flag")) || (utils::IsAbbr(buf, "флаг"))) {
		if (!TabulateObjsByFlagName(buf2, ch)) {
			SendMsgToChar("Нет объектов с таким флагом.\r\n", ch);
		}
	}

	if ((utils::IsAbbr(buf, "room")) || (utils::IsAbbr(buf, "комната"))) {
		if (!TabulateRoomsByName(buf2, ch)) {
			SendMsgToChar("Нет объектов с таким флагом.\r\n", ch);
		}
	}

	if (utils::IsAbbr(buf, "trig") || utils::IsAbbr(buf, "триггер")) {
		if (!TabulateTrigsByObjLoad(buf2, ch)) {
			SendMsgToChar("Нет триггеров, загружающих такой объект.\r\n", ch);
		}
	}

	if (utils::IsAbbr(buf, "load") || utils::IsAbbr(buf, "загрузка")) {
		if (!TabulateMobsByDeadLoad(buf2, ch)) {
			SendMsgToChar("Нет мобов, загружаюющих такой объект по списку dead load.\r\n", ch);
		}
	}
}

int TabulateMobsByName(char *searchname, CharData *ch) {
	int nr, found = 0;

	for (nr = 0; nr <= top_of_mobt; nr++) {
		if (isname(searchname, mob_proto[nr].GetCharAliases())) {
			sprintf(buf, "%3d. [%5d] %-30s (%s)\r\n", ++found, mob_index[nr].vnum, mob_proto[nr].get_npc_name().c_str(),
					npc_race_types[mob_proto[nr].player_data.Race - ENpcRace::kBasic]);
			SendMsgToChar(buf, ch);
		}
	}
	return (found);
}

int TabulateObjsByAliases(char *searchname, CharData *ch) {
	int found = 0;

	for (const auto &nr : obj_proto) {
		if (isname(searchname, nr->get_aliases())) {
			++found;
			sprintf(buf, "%3d. [%5d] %s\r\n",
					found, nr->get_vnum(),
					nr->get_short_description().c_str());
			SendMsgToChar(buf, ch);
		}
	}
	return (found);
}

int TabulateObjsByFlagName(char *searchname, CharData *ch) {
	int found = 0, plane = 0, counter = 0, plane_offset = 0;
	bool f = false;
	std::string out;

// ---------------------- extra_bits
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*extra_bits[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		}
		if (utils::IsAbbr(searchname, extra_bits[counter])) {
			f = true;
			break;
		}
		plane_offset++;
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->has_flag(plane, 1 << plane_offset)) {
				snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : %s\r\n",
						 ++found, i->get_vnum(),
						 utils::RemoveColors(i->get_short_description()).c_str(),
						 extra_bits[counter]);
				out += buf;
			}
		}
	}
// --------------------- apply_types
	f = false;
	for (counter = 0; *apply_types[counter] != '\n'; counter++) {
		if (utils::IsAbbr(searchname, apply_types[counter])) {
			f = true;
			break;
		}
	}
	if (f) {
		for (const auto &i : obj_proto) {
			for (plane = 0; plane < kMaxObjAffect; plane++) {
				if (i->get_affected(plane).location == static_cast<EApply>(counter)) {
					snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : %s, значение: %d\r\n",
							 ++found, i->get_vnum(),
							 utils::RemoveColors(i->get_short_description()).c_str(),
							 apply_types[counter], i->get_affected(plane).modifier);
					out += buf;
					continue;
				}
			}
		}
	}
// --------------------- weapon affects
	f = false;
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*weapon_affects[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		}
		if (utils::IsAbbr(searchname, weapon_affects[counter])) {
			f = true;
			break;
		}
		plane_offset++;
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->get_affect_flags().get_flag(plane, 1 << plane_offset)) {
				snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : %s\r\n",
						 ++found, i->get_vnum(),
						 utils::RemoveColors(i->get_short_description()).c_str(),
						 weapon_affects[counter]);
				out += buf;
			}
		}
	}
// --------------------- anti_bits
	f = false;
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*anti_bits[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		}
		if (utils::IsAbbr(searchname, anti_bits[counter])) {
			f = true;
			break;
		}
		plane_offset++;
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->get_affect_flags().get_flag(plane, 1 << plane_offset)) {
				snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : запрещен для: %s\r\n",
						 ++found, i->get_vnum(),
						 utils::RemoveColors(i->get_short_description()).c_str(),
						 anti_bits[counter]);
				out += buf;
			}
		}
	}
// --------------------- no_bits
	f = false;
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*no_bits[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		}
		if (utils::IsAbbr(searchname, no_bits[counter])) {
			f = true;
			break;
		}
		plane_offset++;
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->get_affect_flags().get_flag(plane, 1 << plane_offset)) {
				snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : неудобен для: %s\r\n",
						 ++found, i->get_vnum(),
						 utils::RemoveColors(i->get_short_description()).c_str(),
						 no_bits[counter]);
				out += buf;
			}
		}
	}
//--------------------------------- skills
	f = false;
	ESkill skill_id;
	for (skill_id = ESkill::kFirst; skill_id <= ESkill::kLast; ++skill_id) {
		if (FixNameAndFindSkillId(searchname) == skill_id) {
			f = true;
			break;
		}
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->has_skills()) {
				auto it = i->get_skills().find(skill_id);
				if (it != i->get_skills().end()) {
					snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : %s, значение: %d\r\n",
							 ++found, i->get_vnum(),
							 utils::RemoveColors(i->get_short_description()).c_str(),
							 MUD::Skill(skill_id).GetName(), it->second);
					out += buf;
				}
			}
		}
	}
	if (!out.empty()) {
		page_string(ch->desc, out);
	}
	return found;
}

int TabulateRoomsByName(char *searchname, CharData *ch) {
	int nr, found = 0;

	for (nr = 0; nr <= top_of_world; nr++) {
		if (isname(searchname, world[nr]->name)) {
			SendMsgToChar(fmt::format("{:>3}. [{:<7}] {}\r\n", ++found, world[nr]->vnum, world[nr]->name), ch);
		}
	}
	return found;
}

int TabulateTrigsByObjLoad(char *searchname, CharData *ch) {
	int num;
	if ((num = atoi(searchname)) == 0) {
		return 0;
	}

	const auto trigger = obj2triggers.find(num);
	if (trigger == obj2triggers.end()) {
		return 0;
	}

	int found = 0;
	for (const auto &t : trigger->second) {
		TrgRnum rnum = GetTriggerRnum(t);
		SendMsgToChar(fmt::format("{:<3}. [{:>5}] {}\r\n",
								  ++found, trig_index[rnum]->vnum, trig_index[rnum]->proto->get_name()), ch);
	}

	return found;
}

int TabulateMobsByDeadLoad(char *vnum, CharData *ch) {
	auto mvn = atoi(vnum);
	int found = 0;
	if (mvn == 0)
		return 0;
	for (auto i = 0; i <= top_of_mobt; i++) {
		if (mob_proto[i].dl_list == nullptr) {
			continue;
		}
		auto predicate = [mvn](struct dead_load::LoadingItem *item) { return (item->obj_vnum == mvn); };
		auto it = std::find_if(mob_proto[i].dl_list->begin(), mob_proto[i].dl_list->end(), predicate);
		if (it != mob_proto[i].dl_list->end()) {
			auto msg = fmt::format("{:<3}. [{:<5}] ({},{},{},{}) {}\r\n",
								   ++found, mob_index[i].vnum, (*it)->obj_vnum, (*it)->load_prob,
								   (*it)->load_type, (*it)->spec_param, mob_proto[i].get_npc_name());
			SendMsgToChar(msg, ch);
		}
	}
	return found;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
