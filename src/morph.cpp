#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "pugixml.hpp"
#include "utils.h"
#include "screen.h"
#include "structs.h"
#include "interpreter.h"
#include "handler.h"
#include "skills.h"
#include "spells.h"
#include "char.hpp"
#include "comm.h"
#include "db.h"
#include "morph.hpp"

MorphListType MorphList;

short MIN_WIS_FOR_MORPH=0;
void perform_remove(CHAR_DATA * ch, int pos);

string AnimalMorph::GetMorphDesc()
{
	string desc = "Неведома зверушка";
	for (DescListType::const_iterator it = descList_.begin();it != descList_.end(); ++it)
		if (it->fromLevel <= ch_->get_level()+ch_->get_remort())
			desc = it->desc;
		else break;

	return desc;
}

string NormalMorph::GetMorphDesc()
{
	return ch_->get_name();
}

string NormalMorph::GetMorphTitle()
{
	return ch_->race_or_title();
};

string AnimalMorph::GetMorphTitle()
{
	return string(ch_->get_name()) + " - " + GetMorphDesc();
};

int NormalMorph::get_trained_skill(int skill_num)
{
	return ch_->get_inborn_skill(skill_num);
}

int AnimalMorph::get_trained_skill(int skill_num)
{
	CharSkillsType::iterator it = skills_.find(skill_num);
	if (it != skills_.end())
	{
		return it->second;
	}
	return 0;
}

void NormalMorph::set_skill(int skill_num, int percent)
{
	ch_->set_skill(skill_num, percent);
}

void AnimalMorph::set_skill(int skill_num, int percent)
{
	CharSkillsType::iterator it = skills_.find(skill_num);
	if (it != skills_.end())
	{
		int diff = percent - it->second;//Polud	пока все снижения уровня скилов в звериной форме уходят в /dev/null
		if (diff > 0 && number(1,2)==2)//Polud в звериной форме вся прокачка идет в оборотничество
		{
			sprintf(buf, "%sВаши успехи сделали вас опытнее в оборотничестве.%s\r\n", CCICYN(ch_, C_NRM), CCINRM(ch_, C_NRM));
			send_to_char(buf, ch_);
			skills_[SKILL_MORPH]+= diff;
		}
	}
}

int NormalMorph::GetStr() const {return ch_->get_inborn_str();}
void NormalMorph::SetStr(int str) {ch_->set_str(str);}
int NormalMorph::GetIntel() const  {return ch_->get_inborn_int();}
void NormalMorph::SetIntel(int intel) {ch_->set_int(intel);}
int NormalMorph::GetWis()const  {return ch_->get_inborn_wis();}
void NormalMorph::SetWis(int wis) {ch_->set_wis(wis);}
int NormalMorph::GetDex()const  {return ch_->get_inborn_dex();}
void NormalMorph::SetDex(int dex) {ch_->set_dex(dex);}
int NormalMorph::GetCha() const {return ch_->get_inborn_cha();}
void NormalMorph::SetCha(int cha) {ch_->set_cha(cha);}
int NormalMorph::GetCon() const {return ch_->get_inborn_con();}
void NormalMorph::SetCon(int con) {ch_->set_con(con);}


void ShowKnownMorphs(CHAR_DATA *ch)
{
	if (ch->is_morphed())
	{
		send_to_char("Чтобы вернуть себе человеческий облик - нужно 'обернуться назад'\r\n", ch);
		return;
	}
	std::list<string> knownMorphs = ch->get_morphs();
	if (knownMorphs.empty())
		send_to_char("На сей момент никакие формы вам неизвестны...\r\n", ch);
	else
		send_to_char("Вы можете обернуться:\r\n", ch);

	for(std::list<string>::const_iterator it = knownMorphs.begin(); it!= knownMorphs.end(); ++it)
		send_to_char("   "+MorphList[*it]->PadName()+"\r\n", ch);
};

string FindMorphId(CHAR_DATA * ch, char *arg)
{
	std::list<string> morphsList = ch->get_morphs();
	for(std::list<string>::const_iterator it = morphsList.begin(); it != morphsList.end(); ++it)
	{
		if (is_abbrev(arg, MorphList[*it]->PadName().c_str()) || is_abbrev(arg, MorphList[*it]->Name().c_str()))
			return *it;
	}
	return string();
}

string GetMorphIdByName(char *arg)
{
	for(MorphListType::const_iterator it = MorphList.begin(); it != MorphList.end(); ++it)
	{
		if (is_abbrev(arg, it->second->PadName().c_str()) || is_abbrev(arg, it->second->Name().c_str()))
			return it->first;
	}
	return string();
}

void AnimalMorph::InitSkills(int value)
{
	for (CharSkillsType::iterator it = skills_.begin(); it != skills_.end();++it)
	{
		if (value)
			it->second = value;
	}
	skills_[SKILL_MORPH]=value;
};

void AnimalMorph::InitAbils()
{
	int extraWis = ch_->get_inborn_wis() - MIN_WIS_FOR_MORPH;
	wis_= MIN(ch_->get_inborn_wis(), MIN_WIS_FOR_MORPH);
	if (extraWis > 0)
	{
		str_ = ch_->get_inborn_str() + extraWis * toStr_ /100;
		dex_ = ch_->get_inborn_dex() + extraWis * toDex_ /100;
		con_ = ch_->get_inborn_con() + extraWis * toCon_ /100;
		cha_ = ch_->get_inborn_cha() + extraWis * toCha_ /100;
		intel_ = ch_->get_inborn_int() + extraWis * toInt_ /100;
	}else
	{
		str_ = ch_->get_inborn_str();
		dex_ = ch_->get_inborn_dex();
		con_ = ch_->get_inborn_con();
		cha_ = ch_->get_inborn_cha();
		intel_ = ch_->get_inborn_int();
	}
}

void AnimalMorph::SetChar(CHAR_DATA *ch)
{
	ch_=ch;
};

bool AnimalMorph::isAffected(long flag) const
{
	return std::find(affects_.begin(), affects_.end(), flag) != affects_.end();
}

void AnimalMorph::AddAffect(long flag)
{
	if (std::find(affects_.begin(), affects_.end(), flag) == affects_.end())
		affects_.push_back(flag);
}

std::vector<long> AnimalMorph::GetAffects()
{
	return affects_;
}

void AnimalMorph::SetAffects(std::vector<long> affs)
{
	affects_ = affs;
}

ACMD(do_morph)
{
	if (IS_NPC(ch))
		return;
	if (!ch->get_skill(SKILL_MORPH))
	{
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}

	if (!*argument || ch->get_morphs_count() == 0)
	{
		ShowKnownMorphs(ch);
		return;
	}

	skip_spaces(&argument);
	one_argument(argument, arg);


	if (ch->is_morphed())
	{
		if (is_abbrev(arg, "назад"))
		{
			ch->reset_morph();
			WAIT_STATE(ch, PULSE_VIOLENCE);
			return;
		}
		send_to_char("Когти подстригите сначала...\r\n", ch);
		return;
	}

	string morphId = FindMorphId(ch, arg);
	if (morphId.empty())
	{
		send_to_char("Обернуться в ЭТО вы не можете!\r\n", ch);
		return;
	}

	MorphPtr newMorph = MorphPtr(new AnimalMorph(*MorphList[morphId]));

	send_to_char(str(boost::format(newMorph->GetMessageToChar()) % newMorph->PadName()) + "\r\n", ch);
	act(str(boost::format(newMorph->GetMessageToRoom()) % newMorph->PadName()).c_str(), TRUE, ch, 0, 0, TO_ROOM);

	ch->set_morph(newMorph);
	if (ch->equipment[WEAR_BOTHS])
	{
		send_to_char("Вы не можете держать в лапах " + string(ch->equipment[WEAR_BOTHS]->PNames[3])+".\r\n", ch);
		perform_remove(ch, WEAR_BOTHS);
	}
	if (ch->equipment[WEAR_WIELD])
	{
		send_to_char("Ваша правая лапа бессильно опустила " + string(ch->equipment[WEAR_WIELD]->PNames[3])+".\r\n", ch);
		perform_remove(ch, WEAR_WIELD);
	}
	if (ch->equipment[WEAR_HOLD])
	{
		send_to_char("Ваша левая лапа не удержала " + string(ch->equipment[WEAR_HOLD]->PNames[3])+".\r\n", ch);
		perform_remove(ch, WEAR_HOLD);
	}
	WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
}

void PrintAllMorphsList(CHAR_DATA *ch)
{
	send_to_char("Существующие формы: \r\n", ch);
	for (MorphListType::const_iterator it = MorphList.begin();it != MorphList.end();++it)
		send_to_char("   " + it->second->Name() + "\r\n", ch);
}

ACMD(do_morphset)
{
	CHAR_DATA * vict;

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Формат команды: morphset <имя игрока (только онлайн)> <название формы> \r\n", ch);
		PrintAllMorphsList(ch);
		return;
	}

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
	{
		send_to_char(NOPERSON, ch);
		return;
	}

	skip_spaces(&argument);

	string morphId = GetMorphIdByName(argument);

	if (morphId.empty())
	{
		send_to_char("Форма '"+string(argument)+"' не найдена. \r\n", ch);
		PrintAllMorphsList(ch);
		return;
	}

	if (vict->know_morph(morphId))
	{
		send_to_char(string(vict->get_name()) + " уже знает эту форму. \r\n", ch);
		return;
	}

	vict->add_morph(morphId);

	sprintf(buf2, "%s add morph %s to %s.", GET_NAME(ch), MorphList[morphId]->Name().c_str(), GET_NAME(vict));
	mudlog(buf2, BRF, -1, SYSLOG, TRUE);
	imm_log("%s add morph %s to %s.", GET_NAME(ch), MorphList[morphId]->Name().c_str(), GET_NAME(vict));

	send_to_char("Вы добавили форму '"+ MorphList[morphId]->Name() +"' персонажу "+string(vict->get_name())+" \r\n", ch);
}

void load_morphs()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_MISC"morphs.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	pugi::xml_node node_list = doc.child("animalMorphs");
	if (!node_list)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...morphs list read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	MIN_WIS_FOR_MORPH = node_list.attribute("minWis").as_int();

	for (pugi::xml_node morph = node_list.child("morph");morph; morph = morph.next_sibling("morph"))
	{
		string id = string(morph.attribute("id").value());
		if (id.empty())
		{
			snprintf(buf, MAX_STRING_LENGTH, "...morph id read fail");
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
		DescListType descList;
		for (pugi::xml_node desc = morph.child("description"); desc; desc = desc.next_sibling("description"))
		{
			DescNode node;
			node.desc = string(desc.child_value());
			node.fromLevel = desc.attribute("lvl").as_int();
			descList.push_back(node);
		}
		CharSkillsType skills;
		std::vector<long> affs;
		pugi::xml_node skillsList=morph.child("skills");
		pugi::xml_node affectsList=morph.child("affects");
		pugi::xml_node messagesList=morph.child("messages");
		string name = morph.child_value("name");
		string padName = morph.child_value("padName");
		string coverDesc = morph.child_value("cover");
		string speech = morph.child_value("speech");
		string messageToChar, messageToRoom;

		for (pugi::xml_node mess = messagesList.child("message"); mess; mess = mess.next_sibling("message"))
		{
			if (string(mess.attribute("destination").value()) == "room")
				messageToRoom = string(mess.child_value());
			if (string(mess.attribute("destination").value()) == "char")
				messageToChar = string(mess.child_value());
		}


		for (pugi::xml_node skill = skillsList.child("skill"); skill; skill = skill.next_sibling("skill"))
		{
			int skillNum = find_skill_num(skill.child_value());
			if (skillNum != -1)
				skills[skillNum]=0;//init-им скилы нулями, потом проставим при превращении
			else
			{
				snprintf(buf, MAX_STRING_LENGTH, "...skills read fail for morph %s", name.c_str());
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
		}

		for (pugi::xml_node aff = affectsList.child("affect"); aff; aff = aff.next_sibling("affect"))
		{
			int affNum = GetAffectNumByName(aff.child_value());
			if (affNum != -1)
				affs.push_back(affNum);
			else
			{
				snprintf(buf, MAX_STRING_LENGTH, "...affects read fail for morph %s", name.c_str());
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
		}
		AnimalMorphPtr newMorph = AnimalMorphPtr(new AnimalMorph(id, name, padName, descList, skills, coverDesc, speech));
		int toStr = atoi(morph.child_value("toStr"));
		int toDex = atoi(morph.child_value("toDex"));
		int toCon = atoi(morph.child_value("toCon"));
		int toInt = atoi(morph.child_value("toInt"));
		int toCha = atoi(morph.child_value("toCha"));
		newMorph->SetAbilsParams(toStr, toDex, toCon, toInt, toCha);
		newMorph->SetAffects(affs);
		newMorph->SetMessages(messageToRoom, messageToChar);
		MorphList[id] = newMorph;
	}
};

void set_god_morphs(CHAR_DATA *ch)
{
	for (MorphListType::const_iterator it= MorphList.begin(); it != MorphList.end();++it)
	{
		if (!ch->know_morph(it->first))
			ch->add_morph(it->first);
	}
}

bool ExistsMorph(string morphId)
{
	for (MorphListType::const_iterator it = MorphList.begin(); it != MorphList.end(); ++it)
		if (it->first == morphId) return true;
	return false;
}

void morphs_save(CHAR_DATA* ch, FILE* saved)
{
	std::list<string> morphs = ch->get_morphs();
	string line;
	for (std::list<string>::const_iterator morph = morphs.begin(); morph != morphs.end(); ++morph)
		line += ("#"+(*morph));
	fprintf(saved, "Mrph: %s\n", line.c_str());
};

void morphs_load(CHAR_DATA* ch, std::string line)
{
	std::vector<string> morphs;
	std::vector<string>::const_iterator it;
	boost::split( morphs, line, boost::is_any_of(std::string("#")), boost::token_compress_on );
	for (it = morphs.begin(), ++it;it!=morphs.end();++it)
	{
		if (ExistsMorph(*it) && !ch->know_morph(*it))
			ch->add_morph(*it);
	}
};

