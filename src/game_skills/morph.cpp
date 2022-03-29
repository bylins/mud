#include "game_skills/morph.hpp"

#include "entities/obj_data.h"
#include "color.h"
#include "handler.h"
#include "game_magic/magic_utils.h"
#include "utils/pugixml/pugixml.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

MorphListType IdToMorphMap;

short MIN_WIS_FOR_MORPH = 0;
void RemoveEquipment(CharData *ch, int pos);

std::string AnimalMorph::GetMorphDesc() const {
	std::string desc = "Неведома зверушка";
	for (DescListType::const_iterator it = descList_.begin(); it != descList_.end(); ++it) {
		if (it->fromLevel <= GetRealLevel(ch_) + ch_->get_remort()) {
			desc = it->desc;
		} else {
			break;
		}
	}

	return desc;
}

std::string NormalMorph::GetMorphDesc() const {
	return ch_->get_name();
}

std::string NormalMorph::GetMorphTitle() const {
	return ch_->race_or_title();
};

std::string AnimalMorph::GetMorphTitle() const {
	return ch_->get_name() + " - " + GetMorphDesc();
};

int NormalMorph::get_trained_skill(const ESkill skill_num) {
	return ch_->get_inborn_skill(skill_num);
}

int AnimalMorph::get_trained_skill(const ESkill skill_num) {
	auto it = skills_.find(skill_num);
	return it != skills_.end() ? it->second : 0;
}

void NormalMorph::set_skill(const ESkill skill_num, int percent) {
	ch_->set_skill(skill_num, percent);
}

void AnimalMorph::set_skill(const ESkill skill_num, int percent) {
	auto it = skills_.find(skill_num);
	if (it != skills_.end()) {
		int diff = percent - it->second;//Polud	пока все снижения уровня скилов в звериной форме уходят в /dev/null
		if (diff > 0 && number(1, 2) == 2)//Polud в звериной форме вся прокачка идет в оборотничество
		{
			sprintf(buf,
					"%sВаши успехи сделали вас опытнее в оборотничестве.%s\r\n",
					CCICYN(ch_, C_NRM),
					CCINRM(ch_, C_NRM));
			send_to_char(buf, ch_);
			skills_[ESkill::kMorph] += diff;
		}
	}
}

int NormalMorph::GetStr() const { return ch_->get_inborn_str(); }
void NormalMorph::SetStr(int str) { ch_->set_str(str); }
int NormalMorph::GetIntel() const { return ch_->get_inborn_int(); }
void NormalMorph::SetIntel(int intel) { ch_->set_int(intel); }
int NormalMorph::GetWis() const { return ch_->get_inborn_wis(); }
void NormalMorph::SetWis(int wis) { ch_->set_wis(wis); }
int NormalMorph::GetDex() const { return ch_->get_inborn_dex(); }
void NormalMorph::SetDex(int dex) { ch_->set_dex(dex); }
int NormalMorph::GetCha() const { return ch_->get_inborn_cha(); }
void NormalMorph::SetCha(int cha) { ch_->set_cha(cha); }
int NormalMorph::GetCon() const { return ch_->get_inborn_con(); }
void NormalMorph::SetCon(int con) { ch_->set_con(con); }

void ShowKnownMorphs(CharData *ch) {
	if (ch->is_morphed()) {
		send_to_char("Чтобы вернуть себе человеческий облик - нужно 'обернуться назад'\r\n", ch);
		return;
	}
	const CharData::morphs_list_t &knownMorphs = ch->get_morphs();
	if (knownMorphs.empty()) {
		send_to_char("На сей момент никакие формы вам неизвестны...\r\n", ch);
	} else {
		send_to_char("Вы можете обернуться:\r\n", ch);

		for (const auto &it : knownMorphs) {
			send_to_char("   " + IdToMorphMap[it]->PadName() + "\r\n", ch);
		}
	}
}

std::string FindMorphId(CharData *ch, char *arg) {
	std::list<std::string> morphsList = ch->get_morphs();
	for (std::list<std::string>::const_iterator it = morphsList.begin(); it != morphsList.end(); ++it) {
		if (utils::IsAbbrev(arg, IdToMorphMap[*it]->PadName().c_str())
			|| utils::IsAbbrev(arg, IdToMorphMap[*it]->Name().c_str())) {
			return *it;
		}
	}
	return std::string();
}

std::string GetMorphIdByName(char *arg) {
	for (MorphListType::const_iterator it = IdToMorphMap.begin(); it != IdToMorphMap.end(); ++it) {
		if (utils::IsAbbrev(arg, it->second->PadName().c_str())
			|| utils::IsAbbrev(arg, it->second->Name().c_str())) {
			return it->first;
		}
	}
	return std::string();
}

void AnimalMorph::InitSkills(int value) {
	for (auto it = skills_.begin(); it != skills_.end(); ++it) {
		if (value) {
			it->second = value;
		}
	}
	skills_[ESkill::kMorph] = value;
};

void AnimalMorph::InitAbils() {
	int extraWis = ch_->get_inborn_wis() - MIN_WIS_FOR_MORPH;
	wis_ = MIN(ch_->get_inborn_wis(), MIN_WIS_FOR_MORPH);
	if (extraWis > 0) {
		str_ = ch_->get_inborn_str() + extraWis * toStr_ / 100;
		dex_ = ch_->get_inborn_dex() + extraWis * toDex_ / 100;
		con_ = ch_->get_inborn_con() + extraWis * toCon_ / 100;
		cha_ = ch_->get_inborn_cha() + extraWis * toCha_ / 100;
		intel_ = ch_->get_inborn_int() + extraWis * toInt_ / 100;
	} else {
		str_ = ch_->get_inborn_str();
		dex_ = ch_->get_inborn_dex();
		con_ = ch_->get_inborn_con();
		cha_ = ch_->get_inborn_cha();
		intel_ = ch_->get_inborn_int();
	}
}

void AnimalMorph::SetChar(CharData *ch) {
	ch_ = ch;
};

bool AnimalMorph::isAffected(const EAffect flag) const {
	return affects_.find(flag) != affects_.end();
}

void AnimalMorph::AddAffect(const EAffect flag) {
	if (affects_.find(flag) == affects_.end()) {
		affects_.insert(flag);
	}
}

const AnimalMorph::affects_list_t &AnimalMorph::GetAffects() {
	return affects_;
}

void AnimalMorph::SetAffects(const affects_list_t &affs) {
	affects_ = affs;
}

MorphPtr GetNormalMorphNew(CharData *ch) {
	return MorphPtr(new NormalMorph(ch));
}

void do_morph(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->is_npc())
		return;
	if (!ch->get_skill(ESkill::kMorph)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}

	if (!*argument || ch->get_morphs_count() == 0) {
		ShowKnownMorphs(ch);
		return;
	}

	skip_spaces(&argument);
	one_argument(argument, arg);

	if (ch->is_morphed()) {
		if (utils::IsAbbrev(arg, "назад")) {
			ch->reset_morph();
			WAIT_STATE(ch, kPulseViolence);
			return;
		}
		send_to_char("Когти подстригите сначала...\r\n", ch);
		return;
	}

	std::string morphId = FindMorphId(ch, arg);
	if (morphId.empty()) {
		send_to_char("Обернуться в ЭТО вы не можете!\r\n", ch);
		return;
	}

	MorphPtr newMorph = MorphPtr(new AnimalMorph(*IdToMorphMap[morphId]));

	send_to_char(str(boost::format(newMorph->GetMessageToChar()) % newMorph->PadName()) + "\r\n", ch);
	act(str(boost::format(newMorph->GetMessageToRoom()) % newMorph->PadName()).c_str(), true, ch, 0, 0, kToRoom);

	ch->set_morph(newMorph);
	if (ch->equipment[EEquipPos::kBoths]) {
		send_to_char("Вы не можете держать в лапах " + ch->equipment[EEquipPos::kBoths]->get_PName(3) + ".\r\n", ch);
		RemoveEquipment(ch, EEquipPos::kBoths);
	}
	if (ch->equipment[EEquipPos::kWield]) {
		send_to_char("Ваша правая лапа бессильно опустила " + ch->equipment[EEquipPos::kWield]->get_PName(3) + ".\r\n", ch);
		RemoveEquipment(ch, EEquipPos::kWield);
	}
	if (ch->equipment[EEquipPos::kHold]) {
		send_to_char("Ваша левая лапа не удержала " + ch->equipment[EEquipPos::kHold]->get_PName(3) + ".\r\n", ch);
		RemoveEquipment(ch, EEquipPos::kHold);
	}
	WAIT_STATE(ch, 3 * kPulseViolence);
}

void PrintAllMorphsList(CharData *ch) {
	send_to_char("Существующие формы: \r\n", ch);
	for (MorphListType::const_iterator it = IdToMorphMap.begin(); it != IdToMorphMap.end(); ++it) {
		send_to_char("   " + it->second->Name() + "\r\n", ch);
	}
}

void do_morphset(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	argument = one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Формат команды: morphset <имя игрока (только онлайн)> <название формы> \r\n", ch);
		PrintAllMorphsList(ch);
		return;
	}

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) {
		send_to_char(NOPERSON, ch);
		return;
	}

	skip_spaces(&argument);

	std::string morphId = GetMorphIdByName(argument);

	if (morphId.empty()) {
		send_to_char("Форма '" + std::string(argument) + "' не найдена. \r\n", ch);
		PrintAllMorphsList(ch);
		return;
	}

	if (vict->know_morph(morphId)) {
		send_to_char(vict->get_name() + " уже знает эту форму. \r\n", ch);
		return;
	}

	vict->add_morph(morphId);

	sprintf(buf2, "%s add morph %s to %s.", GET_NAME(ch), IdToMorphMap[morphId]->Name().c_str(), GET_NAME(vict));
	mudlog(buf2, BRF, -1, SYSLOG, true);
	imm_log("%s add morph %s to %s.", GET_NAME(ch), IdToMorphMap[morphId]->Name().c_str(), GET_NAME(vict));

	send_to_char(std::string("Вы добавили форму '") + IdToMorphMap[morphId]->Name() + "' персонажу " + vict->get_name()
					 + " \r\n", ch);
}

void load_morphs() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_MISC"morphs.xml");
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node node_list = doc.child("animalMorphs");
	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...morphs list read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	MIN_WIS_FOR_MORPH = node_list.attribute("minWis").as_int();

	for (pugi::xml_node morph = node_list.child("morph"); morph; morph = morph.next_sibling("morph")) {
		std::string id = std::string(morph.attribute("id").value());
		if (id.empty()) {
			snprintf(buf, kMaxStringLength, "...morph id read fail");
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}
		DescListType descList;
		for (pugi::xml_node desc = morph.child("description"); desc; desc = desc.next_sibling("description")) {
			DescNode node;
			node.desc = std::string(desc.child_value());
			node.fromLevel = desc.attribute("lvl").as_int();
			descList.push_back(node);
		}
		MorphSkillsList skills;
		IMorph::affects_list_t affs;
		pugi::xml_node skillsList = morph.child("skills");
		pugi::xml_node affectsList = morph.child("affects");
		pugi::xml_node messagesList = morph.child("messages");
		std::string name = morph.child_value("name");
		std::string padName = morph.child_value("padName");
		std::string coverDesc = morph.child_value("cover");
		std::string speech = morph.child_value("speech");
		std::string messageToChar, messageToRoom;

		for (pugi::xml_node mess = messagesList.child("message"); mess; mess = mess.next_sibling("message")) {
			if (std::string(mess.attribute("destination").value()) == "room") {
				messageToRoom = std::string(mess.child_value());
			}
			if (std::string(mess.attribute("destination").value()) == "char") {
				messageToChar = std::string(mess.child_value());
			}
		}

		for (pugi::xml_node skill = skillsList.child("skill"); skill; skill = skill.next_sibling("skill")) {
			std::string strt(skill.child_value());
			const ESkill skillNum = FixNameFndFindSkillNum(strt);
			if (skillNum != ESkill::kIncorrect) {
				skills[skillNum] = 0;//init-им скилы нулями, потом проставим при превращении
			} else {
				snprintf(buf, kMaxStringLength, "...skills read fail for morph %s", name.c_str());
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}
		}

		for (pugi::xml_node aff = affectsList.child("affect"); aff; aff = aff.next_sibling("affect")) {
			EAffect affNum;
			const bool found = GetAffectNumByName(aff.child_value(), affNum);
			if (found) {
				affs.insert(affNum);
			} else {
				snprintf(buf, kMaxStringLength, "...affects read fail for morph %s", name.c_str());
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}
		}
		AnimalMorphPtr
			newMorph = AnimalMorphPtr(new AnimalMorph(id, name, padName, descList, skills, coverDesc, speech));
		int toStr = atoi(morph.child_value("toStr"));
		int toDex = atoi(morph.child_value("toDex"));
		int toCon = atoi(morph.child_value("toCon"));
		int toInt = atoi(morph.child_value("toInt"));
		int toCha = atoi(morph.child_value("toCha"));
		newMorph->SetAbilsParams(toStr, toDex, toCon, toInt, toCha);
		newMorph->SetAffects(affs);
		newMorph->SetMessages(messageToRoom, messageToChar);
		IdToMorphMap[id] = newMorph;
	}
};

void set_god_morphs(CharData *ch) {
	for (MorphListType::const_iterator it = IdToMorphMap.begin(); it != IdToMorphMap.end(); ++it) {
		if (!ch->know_morph(it->first))
			ch->add_morph(it->first);
	}
}

bool ExistsMorph(const std::string &morphId) {
	return IdToMorphMap.find(morphId) != IdToMorphMap.end();
}

void morphs_save(CharData *ch, FILE *saved) {
	const auto &morphs = ch->get_morphs();
	std::string line;
	for (const auto &morph : morphs) {
		line += ("#" + morph);
	}
	fprintf(saved, "Mrph: %s\n", line.c_str());
};

void morphs_load(CharData *ch, std::string line) {
	std::list<std::string> morphs;
	boost::split(morphs, line, boost::is_any_of(std::string("#")), boost::token_compress_on);
	for (const auto &it : morphs) {
		if (ExistsMorph(it)
			&& !ch->know_morph(it)) {
			ch->add_morph(it);
		}
	}
}

const IMorph::affects_list_t empty_list;
const IMorph::affects_list_t &IMorph::GetAffects() {
	return empty_list;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
