// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "char_player.hpp"

#include "logger.hpp"
#include "utils.h"
#include "db.h"
#include "dg_scripts.h"
#include "handler.h"
#include "boards.h"
#include "file_crc.hpp"
#include "spells.h"
#include "constants.h"
#include "skills.h"
#include "ignores.loader.hpp"
#include "im.h"
#include "olc.h"
#include "comm.h"
#include "pk.h"
#include "diskio.h"
#include "interpreter.h"
#include "genchar.h"
#include "AffectHandler.hpp"
#include "player_races.hpp"
#include "morph.hpp"
#include "features.hpp"
#include "screen.h"
#include "ext_money.hpp"
#include "temp_spells.hpp"
#include "conf.h"
#include "accounts.hpp"
#include "zone.table.hpp"
#include "daily_quest.hpp"

#include <boost/lexical_cast.hpp>

#ifdef _WIN32
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <ctime>
#include <sstream>
#include <bitset>


int level_exp(CHAR_DATA * ch, int level);
extern std::vector<City> cities;
extern std::string default_str_cities;
namespace
{

uint8_t get_day_today()
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	return timeinfo->tm_mday;
}

} // namespace

Player::Player():
	pfilepos_(-1),
	was_in_room_(NOWHERE),
	from_room_(0),
	answer_id_(NOBODY),
	motion_(true),
	ice_currency(0),
	hryvn(0),
	spent_hryvn(0)
{
	for (int i = 0; i < START_STATS_TOTAL; ++i)
	{
		start_stats_.at(i) = 0;
	}

	// на 64 битном центосе с оптимизацией - падает или прямо здесь,
	// или в деструкторе чар-даты на делете самого класса в недрах шареда
	// при сборке без оптимизаций - не падает
	// и я не очень в теме, чем этот инит отличается от инита в чар-дате,
	// с учетом того, что здесь у нас абсолютно пустой плеер и внутри set_morph
	// на деле инит ровно тоже самое, может на перспективу это все было
	//set_morph(NormalMorph::GetNormalMorph(this));

	for (unsigned i = 0; i < ext_money_.size(); ++i)
	{
		ext_money_[i] = 0;
	}

	for (unsigned i = 0; i < reset_stats_cnt_.size(); ++i)
	{
		reset_stats_cnt_.at(i) = 0;
	}

	// чтобы не вываливать новому игроку все мессаги на досках как непрочитанные
	const time_t now = time(0);
	board_date_.fill(now);
}

int Player::get_pfilepos() const
{
	return pfilepos_;
}

void Player::set_pfilepos(int pfilepos)
{
	pfilepos_ = pfilepos;
}

room_rnum Player::get_was_in_room() const
{
	return was_in_room_;
}

void Player::set_was_in_room(room_rnum was_in_room)
{
	was_in_room_ = was_in_room;
}

std::string const & Player::get_passwd() const
{
	return passwd_;
}

void Player::set_passwd(std::string const & passwd)
{
	passwd_ = passwd;
}

void Player::remort()
{
	quested_.clear();
	mobmax_.clear();
	count_death_zone.clear();
}

void Player::reset()
{
	remember_.reset();
	last_tell_ = "";
	answer_id_ = NOBODY;
	CHAR_DATA::reset();
}

room_rnum Player::get_from_room() const
{
	return from_room_;
}

void Player::set_from_room(room_rnum from_room)
{
	from_room_ = from_room;
}

int Player::get_start_stat(int stat_num)
{
	int stat = 0;
	try
	{
		stat = start_stats_.at(stat_num);
	}
	catch (...)
	{
		log("SYSERROR : bad start_stat %d (%s %s %d)", stat_num, __FILE__, __func__, __LINE__);
	}
	return stat;
}

void Player::set_start_stat(int stat_num, int number)
{
	try
	{
		start_stats_.at(stat_num) = number;
	}
	catch (...)
	{
		log("SYSERROR : bad start_stat num %d (%s %s %d)", stat_num, __FILE__, __func__, __LINE__);
	}
}

void Player::set_answer_id(int id)
{
	answer_id_ = id;
}

int Player::get_answer_id() const
{
	return answer_id_;
}

void Player::remember_add(const std::string& text, int flag)
{
	remember_.add_str(text, flag);
}

std::string Player::remember_get(int flag) const
{
	return remember_.get_text(flag);
}

bool Player::remember_set_num(unsigned int num)
{
	return remember_.set_num_str(num);
}

unsigned int Player::remember_get_num() const
{
	return remember_.get_num_str();
}

void Player::set_last_tell(const char *text)
{
	if (text)
	{
		last_tell_ = text;
	}
}

void Player::str_to_cities(std::string str)
{
	decltype(cities) tmp_bitset(str);
	this->cities = tmp_bitset;
}

int Player::get_hryvn()
{
	return this->hryvn;
}

void Player::set_hryvn(int value)
{
	if (value > 1200)
		value = 1200;
	this->hryvn = value;
}


void Player::sub_hryvn(int value)
{
	this->hryvn -= value;
}

void Player::add_hryvn(int value)
{
	if (GET_REMORT(this) < 6)
	{
		send_to_char(this, "Глянув на непонятный слиток, Вы решили выкинуть его...\r\n");
		return;
	}
	else if ((this->get_hryvn() + value) > 1200)
	{
		value = 1200 - this->get_hryvn();
		send_to_char(this, "Вы получили только %ld %s, так как в вашу копилку больше не лезет...\r\n",
			static_cast<long>(value), desc_count(value, WHAT_TORCu));
	}
	else
	{
		send_to_char(this, "Вы получили %ld %s.\r\n", 
			static_cast<long>(value), desc_count(value, WHAT_TORCu));
	}
	log("Персонаж %s получил %d [гривны].", GET_NAME(this), value);
	this->hryvn += value;
}

void Player::dquest(const int id)
{
	const auto quest = d_quest.find(id);

	if(quest == d_quest.end())
	{
		log("Quest ID: %d - не найден", id);
		return;
	}

	if (!this->account->quest_is_available(id))
	{
		send_to_char(this, "Сегодня вы уже получали гривны за выполнение этого задания.\r\n");
		return;
	}
	int value = quest->second.reward + number(1, 3);
	const int zone_lvl = zone_table[world[this->in_room]->zone].mob_level;
	if (zone_lvl < 11
		&& 20 <= (GET_LEVEL(this) + GET_REMORT(this) / 5))
	{
		value = 0;
	}
	else if (zone_lvl < 25
		&& zone_lvl <= (GET_LEVEL(this) + GET_REMORT(this) / 5))
	{
		value /= 2;
	}

	this->add_hryvn(value);

	this->account->complete_quest(id);
}

void Player::mark_city(const size_t index)
{
	if (index < cities.size())
	{
		cities[index] = true;
	}
}

bool Player::check_city(const size_t index)
{
	if (index < cities.size())
	{
		return cities[index];
	}

	return false;
}

std::string Player::cities_to_str()
{
	std::string return_value;
	boost::to_string(this->cities, return_value);
	return return_value;
}

std::string const & Player::get_last_tell()
{
	return last_tell_;
}

void Player::quested_add(CHAR_DATA *ch, int vnum, char *text)
{
	quested_.add(ch, vnum, text);
}

bool Player::quested_remove(int vnum)
{
	return quested_.remove(vnum);
}

bool Player::quested_get(int vnum) const
{
	return quested_.get(vnum);
}

std::string Player::quested_get_text(int vnum) const
{
	return quested_.get_text(vnum);
}

std::string Player::quested_print() const
{
	return quested_.print();
}

void Player::quested_save(FILE *saved) const
{
	quested_.save(saved);
}

int Player::mobmax_get(int vnum) const
{
	return mobmax_.get_kill_count(vnum);
}

void Player::mobmax_add(CHAR_DATA *ch, int vnum, int count, int level)
{
	mobmax_.add(ch, vnum, count, level);
}

void Player::mobmax_load(CHAR_DATA *ch, int vnum, int count, int level)
{
	mobmax_.load(ch, vnum, count, level);
}

void Player::mobmax_remove(int vnum)
{
	mobmax_.remove(vnum);
}

void Player::mobmax_save(FILE *saved) const
{
	mobmax_.save(saved);
}

void Player::dps_add_dmg(int type, int dmg, int over_dmg, CHAR_DATA *ch)
{
	dps_.add_dmg(type, ch, dmg, over_dmg);
}

void Player::dps_clear(int type)
{
	dps_.clear(type);
}

void Player::dps_print_stats(CHAR_DATA *coder)
{
	dps_.print_stats(this, coder);
}

void Player::dps_print_group_stats(CHAR_DATA *ch, CHAR_DATA *coder)
{
	dps_.print_group_stats(ch, coder);
}

// * Для dps_copy.
void Player::dps_set(DpsSystem::Dps *dps)
{
	dps_ = *dps;
}

// * Нужно только для копирования всего этого дела при передаче лидера.
void Player::dps_copy(CHAR_DATA *ch)
{
	ch->dps_set(&dps_);
}

void Player::dps_end_round(int type, CHAR_DATA *ch)
{
	dps_.end_round(type, ch);
}

void Player::dps_add_exp(int exp, bool battle)
{
	if (battle)
	{
		dps_.add_battle_exp(exp);
	}
	else
	{
		dps_.add_exp(exp);
	}
}

// не дергать wear/remove триги при скрытом раздевании/одевании чара во время сейва
#define NO_EXTRANEOUS_TRIGGERS

void Player::save_char()
{
	FILE *saved;
	char filename[MAX_STRING_LENGTH];
	int i;
	time_t li;
	OBJ_DATA *char_eq[NUM_WEARS];
	struct timed_type *skj;
	struct char_portal_type *prt;
	int tmp = time(0) - this->player_data.time.logon;

	if (IS_NPC(this) || this->get_pfilepos() < 0)
		return;
	if (this->account == nullptr)
	{
		this->account = Account::get_account(GET_EMAIL(this));
		if (this->account == nullptr)
		{
			const auto temp_account = std::make_shared<Account>(GET_EMAIL(this));
			accounts.emplace(GET_EMAIL(this), temp_account);
			this->account = temp_account;
		}
	}
	this->account->save_to_file();
	log("Save char %s", GET_NAME(this));
	save_char_vars(this);

	// Запись чара в новом формате
	get_filename(GET_NAME(this), filename, PLAYERS_FILE);
	if (!(saved = fopen(filename, "w")))
	{
		perror("Unable open charfile");
		return;
	}
	// подготовка
	// снимаем все возможные аффекты
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(this, i))
		{
			char_eq[i] = unequip_char(this, i | 0x80);
#ifndef NO_EXTRANEOUS_TRIGGERS
			remove_otrigger(char_eq[i], this);
#endif
		}
		else
			char_eq[i] = NULL;
	}

	AFFECT_DATA<EApplyLocation> tmp_aff[MAX_AFFECT];
	{
		auto aff_i = affected.begin();
		for (i = 0; i < MAX_AFFECT; i++)
		{
			if (aff_i != affected.end())
			{
				const auto& aff = *aff_i;
				if (aff->type == SPELL_ARMAGEDDON
					|| aff->type < 1
					|| aff->type > SPELLS_COUNT)
				{
					i--;
				}
				else
				{
					tmp_aff[i] = *aff;
				}
				++aff_i;
			}
			else
			{
				tmp_aff[i].type = 0;	// Zero signifies not used
				tmp_aff[i].duration = 0;
				tmp_aff[i].modifier = 0;
				tmp_aff[i].location = EApplyLocation::APPLY_NONE;
				tmp_aff[i].bitvector = 0;
			}
		}

		if ((i >= MAX_AFFECT) && aff_i != affected.end())
		{
			log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");
		}

		/*
		 * remove the affections so that the raw values are stored; otherwise the
		 * effects are doubled when the char logs back in.
		 */
		while (!affected.empty())
		{
			affect_remove(affected.begin());
		}
	}

	// первыми идут поля, необходимые при ребуте мада, тут без необходимости трогать ничего не надо
	if (!get_name().empty())
	{
		fprintf(saved, "Name: %s\n", GET_NAME(this));
	}
	fprintf(saved, "Levl: %d\n", GET_LEVEL(this));
	fprintf(saved, "Clas: %d\n", GET_CLASS(this));
	fprintf(saved, "UIN : %d\n", GET_UNIQUE(this));
	fprintf(saved, "LstL: %ld\n", static_cast<long int>(LAST_LOGON(this)));
	if (this->desc)//edited WorM 2010.08.27 перенесено чтоб грузилось для сохранения в индексе игроков
	{
		strcpy(buf, this->desc->host);
	}
	else//по сути так должен норм сохраняцо последний айпи
	{
		if (!LOGON_LIST(this).empty())
		{
			logon_data* last_logon = &LOGON_LIST(this).at(0);
			for(auto& cur_log : LOGON_LIST(this))
			{
				if (cur_log.lasttime > last_logon->lasttime)
				{
					last_logon = &cur_log;
				}
			}
			strcpy(buf, last_logon->ip);
		}
		else
		{
			strcpy(buf, "Unknown");
		}
	}

	fprintf(saved, "Host: %s\n", buf);
	free(player_table[this->get_pfilepos()].last_ip);
	player_table[this->get_pfilepos()].last_ip = str_dup(buf);
	fprintf(saved, "Id  : %ld\n", GET_IDNUM(this));
	fprintf(saved, "Exp : %ld\n", GET_EXP(this));
	if (GET_REMORT(this) > 0)
	{
		fprintf(saved, "Rmrt: %d\n", GET_REMORT(this));
	}
	// флаги
	*buf = '\0';
	PLR_FLAGS(this).tascii(4, buf);
	fprintf(saved, "Act : %s\n", buf);
	if (GET_EMAIL(this))//edited WorM 2010.08.27 перенесено чтоб грузилось для сохранения в индексе игроков
	{
		fprintf(saved, "EMal: %s\n", GET_EMAIL(this));
	}
	// это пишем обязательно посленим, потому что после него ничего не прочитается
	fprintf(saved, "Rebt: следующие далее поля при перезагрузке не парсятся\n\n");
	// дальше пишем как хотим и что хотим

	fprintf(saved, "NmI : %s\n", GET_PAD(this, 0));
	fprintf(saved, "NmR : %s\n", GET_PAD(this, 1));
	fprintf(saved, "NmD : %s\n", GET_PAD(this, 2));
	fprintf(saved, "NmV : %s\n", GET_PAD(this, 3));
	fprintf(saved, "NmT : %s\n", GET_PAD(this, 4));
	fprintf(saved, "NmP : %s\n", GET_PAD(this, 5));
	if (!this->get_passwd().empty())
		fprintf(saved, "Pass: %s\n", this->get_passwd().c_str());
	if (!this->player_data.title.empty())
		fprintf(saved, "Titl: %s\n", this->player_data.title.c_str());
	if (!this->player_data.description.empty())
	{
		strcpy(buf, this->player_data.description.c_str());
		kill_ems(buf);
		fprintf(saved, "Desc:\n%s~\n", buf);
	}
	if (POOFIN(this))
		fprintf(saved, "PfIn: %s\n", POOFIN(this));
	if (POOFOUT(this))
		fprintf(saved, "PfOt: %s\n", POOFOUT(this));
	fprintf(saved, "Sex : %d %s\n", static_cast<int>(GET_SEX(this)), genders[(int) GET_SEX(this)]);
	fprintf(saved, "Kin : %d %s\n", GET_KIN(this), PlayerRace::GetKinNameByNum(GET_KIN(this),GET_SEX(this)).c_str());
	li = this->player_data.time.birth;
	fprintf(saved, "Brth: %ld %s\n", static_cast<long int>(li), ctime(&li));
	// Gunner
	tmp += this->player_data.time.played;
	fprintf(saved, "Plyd: %d\n", tmp);
	// Gunner end
	li = this->player_data.time.logon;
	fprintf(saved, "Last: %ld %s\n", static_cast<long int>(li), ctime(&li));
	fprintf(saved, "Hite: %d\n", GET_HEIGHT(this));
	fprintf(saved, "Wate: %d\n", GET_WEIGHT(this));
	fprintf(saved, "Size: %d\n", GET_SIZE(this));
	// структуры
	fprintf(saved, "Alin: %d\n", GET_ALIGNMENT(this));
	*buf = '\0';
	AFF_FLAGS(this).tascii(4, buf);
	fprintf(saved, "Aff : %s\n", buf);

	// дальше не по порядку
	// статсы
	fprintf(saved, "Str : %d\n", this->get_inborn_str());
	fprintf(saved, "Int : %d\n", this->get_inborn_int());
	fprintf(saved, "Wis : %d\n", this->get_inborn_wis());
	fprintf(saved, "Dex : %d\n", this->get_inborn_dex());
	fprintf(saved, "Con : %d\n", this->get_inborn_con());
	fprintf(saved, "Cha : %d\n", this->get_inborn_cha());

	// способности - added by Gorrah
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "Feat:\n");
		for (i = 1; i < MAX_FEATS; i++)
		{
			if (HAVE_FEAT(this, i))
				fprintf(saved, "%d %s\n", i, feat_info[i].name);
		}
		fprintf(saved, "0 0\n");
	}

	// Задержки на cпособности
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "FtTm:\n");
		for (skj = this->timed_feat; skj; skj = skj->next)
		{
			fprintf(saved, "%d %d %s\n", skj->skill, skj->time, feat_info[skj->skill].name);
		}
		fprintf(saved, "0 0\n");
	}

	// скилы
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "Skil:\n");
		int skill;
		for (const auto i : AVAILABLE_SKILLS)
		{
			skill = this->get_inborn_skill(i);
			if (skill)
			{
			    fprintf(saved, "%d %d %s\n", i, skill, skill_info[i].name);
			}
		}
		fprintf(saved, "0 0\n");
	}

	// города
	fprintf(saved, "Cits: %s\n", this->cities_to_str().c_str());

	// Задержки на скилы
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "SkTm:\n");
		for (skj = this->timed; skj; skj = skj->next)
		{
			fprintf(saved, "%d %d\n", skj->skill, skj->time);
		}
		fprintf(saved, "0 0\n");
	}

	// спелы
	// волхвам всеравно известны тупо все спеллы, смысла их писать не вижу
	if (GET_LEVEL(this) < LVL_IMMORT && GET_CLASS(this) != CLASS_DRUID)
	{
		fprintf(saved, "Spel:\n");
		for (i = 1; i <= MAX_SPELLS; i++)
			if (GET_SPELL_TYPE(this, i))
				fprintf(saved, "%d %d %s\n", i, GET_SPELL_TYPE(this, i), spell_info[i].name);
		fprintf(saved, "0 0\n");
	}

	if (GET_LEVEL(this) < LVL_IMMORT && GET_CLASS(this) != CLASS_DRUID)
	{
		fprintf(saved, "TSpl:\n");
		for (auto it = this->temp_spells.begin(); it != this->temp_spells.end(); ++it)
		{
			fprintf(saved, "%d %ld %ld %s\n", it->first, static_cast<long int>(it->second.set_time), static_cast<long int>(it->second.duration), spell_info[it->first].name);
		}
		fprintf(saved, "0 0 0\n");
	}

	// Замемленые спелы
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "SpMe:\n");
		for (i = 1; i <= MAX_SPELLS; i++)
		{
			if (GET_SPELL_MEM(this, i))
				fprintf(saved, "%d %d\n", i, GET_SPELL_MEM(this, i));
		}
		fprintf(saved, "0 0\n");
	}

	// Мемящиеся спелы
	if (GET_LEVEL(this) < LVL_IMMORT)
	{
		fprintf(saved, "SpTM:\n");
		for (struct spell_mem_queue_item * qi = this->MemQueue.queue; qi != NULL; qi = qi->link)
			fprintf(saved, "%d\n", qi->spellnum);
		fprintf(saved, "0\n");
	}

	// Рецепты
//    if (GET_LEVEL(this) < LVL_IMMORT)
	{
		im_rskill *rs;
		im_recipe *r;
		fprintf(saved, "Rcps:\n");
		for (rs = GET_RSKILL(this); rs; rs = rs->link)
		{
			if (rs->perc <= 0)
				continue;
			r = &imrecipes[rs->rid];
			fprintf(saved, "%d %d %s\n", r->id, rs->perc, r->name);
		}
		fprintf(saved, "-1 -1\n");
	}

	fprintf(saved, "Hrol: %d\n", GET_HR(this));
	fprintf(saved, "Drol: %d\n", GET_DR(this));
	fprintf(saved, "Ac  : %d\n", GET_AC(this));
	fprintf(saved, "Hry : %d\n", this->get_hryvn());
	fprintf(saved, "Hit : %d/%d\n", GET_HIT(this), GET_MAX_HIT(this));
	fprintf(saved, "Mana: %d/%d\n", GET_MEM_COMPLETED(this), GET_MEM_TOTAL(this));
	fprintf(saved, "Move: %d/%d\n", GET_MOVE(this), GET_MAX_MOVE(this));
	fprintf(saved, "Gold: %ld\n", get_gold());
	fprintf(saved, "Bank: %ld\n", get_bank());
	fprintf(saved, "ICur: %d\n", get_ice_currency());
	fprintf(saved, "Ruble: %ld\n", get_ruble());
	fprintf(saved, "Wimp: %d\n", GET_WIMP_LEV(this));
	fprintf(saved, "Frez: %d\n", GET_FREEZE_LEV(this));
	fprintf(saved, "Invs: %d\n", GET_INVIS_LEV(this));
	fprintf(saved, "Room: %d\n", GET_LOADROOM(this));
//	li = this->player_data.time.birth;
//	fprintf(saved, "Brth: %ld %s\n", static_cast<long int>(li), ctime(&li));
	fprintf(saved, "Lexc: %ld\n", static_cast<long>(LAST_EXCHANGE(this)));
	fprintf(saved, "Badp: %d\n", GET_BAD_PWS(this));

	for (unsigned i = 0; i < board_date_.size(); ++i)
	{
		fprintf(saved, "Br%02u: %llu\n", i + 1, static_cast<unsigned long long>(board_date_.at(i)));
	}

	for (int i = 0; i < START_STATS_TOTAL; ++i)
		fprintf(saved, "St%02d: %i\n", i, this->get_start_stat(i));

	if (GET_LEVEL(this) < LVL_IMMORT)
		fprintf(saved, "Hung: %d\n", GET_COND(this, FULL));
	if (GET_LEVEL(this) < LVL_IMMORT)
		fprintf(saved, "Thir: %d\n", GET_COND(this, THIRST));
	if (GET_LEVEL(this) < LVL_IMMORT)
		fprintf(saved, "Drnk: %d\n", GET_COND(this, DRUNK));

	fprintf(saved, "Reli: %d %s\n", GET_RELIGION(this), religion_name[GET_RELIGION(this)][(int) GET_SEX(this)]);
	fprintf(saved, "Race: %d %s\n", GET_RACE(this), PlayerRace::GetRaceNameByNum(GET_KIN(this),GET_RACE(this),GET_SEX(this)).c_str());
	fprintf(saved, "DrSt: %d\n", GET_DRUNK_STATE(this));
	fprintf(saved, "Olc : %d\n", GET_OLC_ZONE(this));
	*buf = '\0';
	PRF_FLAGS(this).tascii(4, buf);
	fprintf(saved, "Pref: %s\n", buf);

	if (MUTE_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_MUTE))
		fprintf(saved, "PMut: %ld %d %ld %s~\n", MUTE_DURATION(this), GET_MUTE_LEV(this), MUTE_GODID(this), MUTE_REASON(this));
	if (NAME_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_NAMED))
		fprintf(saved, "PNam: %ld %d %ld %s~\n", NAME_DURATION(this), GET_NAME_LEV(this), NAME_GODID(this), NAME_REASON(this));
	if (DUMB_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_DUMB))
		fprintf(saved, "PDum: %ld %d %ld %s~\n", DUMB_DURATION(this), GET_DUMB_LEV(this), DUMB_GODID(this), DUMB_REASON(this));
	if (HELL_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_HELLED))
		fprintf(saved, "PHel: %ld %d %ld %s~\n", HELL_DURATION(this), GET_HELL_LEV(this), HELL_GODID(this), HELL_REASON(this));
	if (GCURSE_DURATION(this) > 0)
		fprintf(saved, "PGcs: %ld %d %ld %s~\n", GCURSE_DURATION(this), GET_GCURSE_LEV(this), GCURSE_GODID(this), GCURSE_REASON(this));
	if (FREEZE_DURATION(this) > 0 && PLR_FLAGGED(this, PLR_FROZEN))
		fprintf(saved, "PFrz: %ld %d %ld %s~\n", FREEZE_DURATION(this), GET_FREEZE_LEV(this), FREEZE_GODID(this), FREEZE_REASON(this));
	if (UNREG_DURATION(this) > 0)
		fprintf(saved, "PUnr: %ld %d %ld %s~\n", UNREG_DURATION(this), GET_UNREG_LEV(this), UNREG_GODID(this), UNREG_REASON(this));


	if (KARMA(this))
	{
		strcpy(buf, KARMA(this));
		kill_ems(buf);
		fprintf(saved, "Karm:\n%s~\n", buf);
	}
	if (!LOGON_LIST(this).empty())
	{
		log("Saving logon list.");
		std::ostringstream buffer;
		for(const auto& logon : LOGON_LIST(this))
		{
			buffer << logon.ip << " " << logon.count << " " << logon.lasttime << "\n";
		}
		fprintf(saved, "LogL:\n%s~\n", buffer.str().c_str());
	}
	fprintf(saved, "GdFl: %ld\n", this->player_specials->saved.GodsLike);
	fprintf(saved, "NamG: %d\n", NAME_GOD(this));
	fprintf(saved, "NaID: %ld\n", NAME_ID_GOD(this));
	fprintf(saved, "StrL: %d\n", STRING_LENGTH(this));
	fprintf(saved, "StrW: %d\n", STRING_WIDTH(this));
	fprintf(saved, "NtfE: %ld\n", NOTIFY_EXCH_PRICE(this)); //Polud мин. цена для оффлайн-оповещений

	if (this->remember_get_num() != Remember::DEF_REMEMBER_NUM)
	{
		fprintf(saved, "Rmbr: %u\n", this->remember_get_num());
	}

	if (EXCHANGE_FILTER(this))
		fprintf(saved, "ExFl: %s\n", EXCHANGE_FILTER(this));

	for (const auto& cur : get_ignores())
	{
		if (0 != cur->id)
		{
			fprintf(saved, "Ignr: [%lu]%ld\n", cur->mode, cur->id);
		}
	}

	// affected_type
	if (tmp_aff[0].type > 0)
	{
		fprintf(saved, "Affs:\n");
		for (i = 0; i < MAX_AFFECT; i++)
		{
			const auto& aff = &tmp_aff[i];
			if (aff->type)
			{
				fprintf(saved, "%d %d %d %d %d %d %s\n", aff->type, aff->duration,
					aff->modifier, aff->location, static_cast<int>(aff->bitvector),
					static_cast<int>(aff->battleflag), spell_name(aff->type));
			}
		}
		fprintf(saved, "0 0 0 0 0 0\n");
	}

	// порталы
	for (prt = GET_PORTALS(this); prt; prt = prt->next)
	{
		fprintf(saved, "Prtl: %d\n", prt->vnum);
	}

	for (auto x : this->daily_quest)
	{
		std::stringstream buffer;
		const auto it = this->daily_quest_timed.find(x.first);
		if (it != this->daily_quest_timed.end())
			buffer << "DaiQ: " << x.first << " " << x.second << " " << it->second << "\n";
		else
			buffer << "DaiQ: " << x.first << " " << x.second << " 0\n";
		fprintf(saved, "%s", buffer.str().c_str());
	}

	for (i = 0; i < 1 + LAST_LOG; ++i)
	{
		if (!GET_LOGS(this))
		{
			log("SYSERR: Saving NULL logs for char %s", GET_NAME(this));
			break;
		}
		fprintf(saved, "Logs: %d %d\n", i, GET_LOGS(this)[i]);
	}

	fprintf(saved, "Disp: %ld\n", disposable_flags_.to_ulong());

	fprintf(saved, "Ripa: %d\n", GET_RIP_ARENA(this)); //Rip_arena
	fprintf(saved, "Wina: %d\n", GET_WIN_ARENA(this)); //Win_arena
	fprintf(saved, "Expa: %llu\n", GET_EXP_ARENA(this)); //Exp_arena
	fprintf(saved, "Ripm: %d\n", GET_RIP_MOB(this)); //Rip_mob
	fprintf(saved, "Expm: %llu\n", GET_EXP_MOB(this)); //Exp_mob
	fprintf(saved, "Ripd: %d\n", GET_RIP_DT(this)); //Rip_dt
	fprintf(saved, "Expd: %llu\n", GET_EXP_DT(this));//Exp_dt
	fprintf(saved, "Ripo: %d\n", GET_RIP_OTHER(this));//Rip_other
	fprintf(saved, "Expo: %llu\n", GET_EXP_OTHER(this));//Exp_other
	fprintf(saved, "Ripp: %d\n", GET_RIP_PK(this));//Rip_pk
	fprintf(saved, "Expp: %llu\n", GET_EXP_PK(this)); //Exp_pk
	fprintf(saved, "Rimt: %d\n", GET_RIP_MOBTHIS(this)); //Rip_mob_this
	fprintf(saved, "Exmt: %llu\n", GET_EXP_MOBTHIS(this));//Exp_mob_this
	fprintf(saved, "Ridt: %d\n", GET_RIP_DTTHIS(this)); //Rip_dt_this
	fprintf(saved, "Exdt: %llu\n", GET_EXP_DTTHIS(this)); //Exp_dt_this
	fprintf(saved, "Riot: %d\n", GET_RIP_OTHERTHIS(this)); //Rip_other_this
	fprintf(saved, "Exot: %llu\n", GET_EXP_OTHERTHIS(this)); ////Exp_other_this
	fprintf(saved, "Ript: %d\n", GET_RIP_PKTHIS(this)); //Rip_pk_this
	fprintf(saved, "Expt: %llu\n", GET_EXP_PKTHIS(this));//Exp_pk_this

	// не забываем рестить ману и при сейве
	this->set_who_mana(MIN(WHO_MANA_MAX,
	                       this->get_who_mana() + (time(0) - this->get_who_last()) * WHO_MANA_REST_PER_SECOND));
	fprintf(saved, "Wman: %u\n", this->get_who_mana());

	// added by WorM (Видолюб) 2010.06.04 бабки потраченные на найм(возвращаются при креше)
	i = 0;
	if (this->followers
		&& can_use_feat(this, EMPLOYER_FEAT)
		&& !IS_IMMORTAL(this))
	{
		struct follow_type *k = NULL;
		for (k = this->followers; k; k = k->next)
		{
			if (k->follower
				&& AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER)
				&& AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM))
			{
				break;
			}
		}

		if (k
			&& k->follower
			&& !k->follower->affected.empty())
		{
			for (const auto& aff : k->follower->affected)
			{
				if (aff->type == SPELL_CHARM)
				{
					if(k->follower->mob_specials.hire_price == 0)
					{
						break;
					}

					int i = ((aff->duration-1)/2)*k->follower->mob_specials.hire_price;
					if(i != 0)
					{
						fprintf(saved, "GldH: %d\n", i);
					}
					break;
				}
			}
		}
	}

	this->quested_save(saved);
	this->mobmax_save(saved);
	save_pkills(this, saved);
	morphs_save(this, saved);

	fprintf(saved, "Map : %s\n", map_options_.bit_list_.to_string().c_str());

	fprintf(saved, "TrcG: %d\n", ext_money_[ExtMoney::TORC_GOLD]);
	fprintf(saved, "TrcS: %d\n", ext_money_[ExtMoney::TORC_SILVER]);
	fprintf(saved, "TrcB: %d\n", ext_money_[ExtMoney::TORC_BRONZE]);
	fprintf(saved, "TrcL: %d %d\n", today_torc_.first, today_torc_.second);

	if (get_reset_stats_cnt(ResetStats::Type::MAIN_STATS) > 0)
	{
		fprintf(saved, "CntS: %d\n", get_reset_stats_cnt(ResetStats::Type::MAIN_STATS));
	}

	if (get_reset_stats_cnt(ResetStats::Type::RACE) > 0)
	{
		fprintf(saved, "CntR: %d\n", get_reset_stats_cnt(ResetStats::Type::RACE));
	}

	if (get_reset_stats_cnt(ResetStats::Type::FEATS) > 0)
	{
		fprintf(saved, "CntF: %d\n", get_reset_stats_cnt(ResetStats::Type::FEATS));
	}

	fclose(saved);
	FileCRC::check_crc(filename, FileCRC::UPDATE_PLAYER, GET_UNIQUE(this));

	// восстанавливаем аффекты
	// add spell and eq affections back in now
	for (i = 0; i < MAX_AFFECT; i++)
	{
		if (tmp_aff[i].type)
		{
			affect_to_char(this, tmp_aff[i]);
		}
	}

	for (i = 0; i < NUM_WEARS; i++)
	{
		if (char_eq[i])
		{
#ifndef NO_EXTRANEOUS_TRIGGERS
			if (wear_otrigger(char_eq[i], this, i))
#endif
				equip_char(this, char_eq[i], i | 0x80 | 0x40);
#ifndef NO_EXTRANEOUS_TRIGGERS
			else
				obj_to_char(char_eq[i], this);
#endif
		}
	}
	affect_total(this);

	i = get_ptable_by_name(GET_NAME(this));
	if (i >= 0)
	{
		player_table[i].last_logon = LAST_LOGON(this);
		player_table[i].level = GET_LEVEL(this);
		player_table[i].remorts = get_remort();
		//added by WorM 2010.08.27 в индексе добавляем мыло
		if(player_table[i].mail)
			free(player_table[i].mail);
		player_table[i].mail = str_dup(GET_EMAIL(this));
		player_table[i].unique = GET_UNIQUE(this);
		//end by WorM
	}
}

#undef NO_EXTRANEOUS_TRIGGERS

// на счет reboot: используется только при старте мада в вызовах из entrycount
// при включенном флаге файл читается только до поля Rebt, все остальные поля пропускаются
// поэтому при каких-то изменениях в entrycount, must_be_deleted и TopPlayer::Refresh следует
// убедиться, что изменный код работает с действительно проинициализированными полями персонажа
// на данный момент это: PLR_FLAGS, GET_CLASS, GET_EXP, GET_IDNUM, LAST_LOGON, GET_LEVEL, GET_NAME, GET_REMORT, GET_UNIQUE, GET_EMAIL
// * \param reboot - по дефолту = false
int Player::load_char_ascii(const char *name, bool reboot, const bool find_id /*= true*/)
{
	int id, num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0, i;
	long int lnum = 0, lnum3 = 0;
	unsigned long long llnum = 0;
	FBFILE *fl = NULL;
	char filename[40];
	char buf[MAX_RAW_INPUT_LENGTH], line[MAX_RAW_INPUT_LENGTH], tag[6];
	char line1[MAX_RAW_INPUT_LENGTH];
	struct timed_type timed;
	*filename = '\0';
	log("Load ascii char %s", name);
	if (!find_id)
	{
		id = 1;
	}
	else
	{
		id = find_name(name);
	}

	bool result = id >= 0;
	result = result && get_filename(name, filename, PLAYERS_FILE);
	result = result && (fl = fbopen(filename, FB_READ));
	if (!result)
	{
		const std::size_t BUFFER_SIZE = 1024;
		char buffer[BUFFER_SIZE];
		log("Can't load ascii. ID: %d; File name: \"%s\"; Current directory: \"%s\")", id, filename, getcwd(buffer, BUFFER_SIZE));
		return -1;
	}

///////////////////////////////////////////////////////////////////////////////

	// первыми иним и парсим поля для ребута до поля "Rebt", если reboot на входе = 1, то на этом парс и кончается
	if (!this->player_specials)
	{
		this->player_specials = std::make_shared<player_special_data>();
	}

	set_level(1);
	set_class(1);
	set_uid(0);
	set_last_logon(time(0));
	set_idnum(0);
	set_exp(0);
	set_remort(0);
	GET_LASTIP(this)[0] = 0;
	GET_EMAIL(this)[0] = 0;
	PLR_FLAGS(this).from_string("");	// suspicious line: we should clear flags.. Loading from "" does not clear flags.

	bool skip_file = 0;

	do
	{
		if (!fbgetline(fl, line))
		{
			log("SYSERROR: Wrong file ascii %d %s", id, filename);
			return (-1);
		}

		tag_argument(line, tag);
		for (i = 0; !(line[i] == ' ' || line[i] == '\0'); i++)
		{
			line1[i] = line[i];
		}
		line1[i] = '\0';
		num = atoi(line1);
		lnum = atol(line1);
		try
		{
			llnum = boost::lexical_cast<unsigned long long>(line1);
		}
		catch(boost::bad_lexical_cast &)
        {
			llnum = 0;
		}


		switch (*tag)
		{
		case 'A':
			if (!strcmp(tag, "Act "))
			{
				PLR_FLAGS(this).from_string(line);
			}
			break;
		case 'C':
			if (!strcmp(tag, "Clas"))
			{
				set_class(num);
			}
			break;
		case 'E':
			if (!strcmp(tag, "Exp "))
			{
				set_exp(lnum);
			}
			//added by WorM 2010.08.27 лоадим мыло и айпи даже при ребуте
			else if (!strcmp(tag, "EMal"))
				strcpy(GET_EMAIL(this), line);
			break;
		case 'H':
			if (!strcmp(tag, "Host"))
			{
				strcpy(GET_LASTIP(this), line);
			}
			//end by WorM
		  break;
		case 'I':
			if (!strcmp(tag, "Id  "))
			{
				set_idnum(lnum);
			}
			break;
		case 'L':
			if (!strcmp(tag, "LstL"))
			{
				set_last_logon(lnum);
			}
			else if (!strcmp(tag, "Levl"))
			{
				set_level(num);
			}
			break;
		case 'N':
			if (!strcmp(tag, "Name"))
			{
				set_name(line);
			}
			break;
		case 'R':
			if (!strcmp(tag, "Rebt"))
				skip_file = 1;
			else if (!strcmp(tag, "Rmrt"))
			{
				set_remort(num);
			}
			break;
		case 'U':
			if (!strcmp(tag, "UIN "))
			{
				set_uid(num);
			}
			break;
		default:
			sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
		}
	}
	while (!skip_file);

	//added by WorM 2010.08.27 лоадим мыло и последний ip даже при считывании индексов
	while((reboot) && (!*GET_EMAIL(this) || !*GET_LASTIP(this)))
	{
		if (!fbgetline(fl, line))
		{
			log("SYSERROR: Wrong file ascii %d %s", id, filename);
			return (-1);
		}

		tag_argument(line, tag);

		if (!strcmp(tag, "EMal"))
			strcpy(GET_EMAIL(this), line);
		else if (!strcmp(tag, "Host"))
			strcpy(GET_LASTIP(this), line);
	}
	//end by WorM

	// если с загруженными выше полями что-то хочется делать после лоада - делайте это здесь

	//Indexing experience - if his exp is lover than required for his level - set it to required
	if (GET_EXP(this) < level_exp(this, GET_LEVEL(this)))
	{
		set_exp(level_exp(this, GET_LEVEL(this)));
	}

	if (reboot)
	{
		fbclose(fl);
		return id;
	}
	this->str_to_cities(default_str_cities);
	// если происходит обычный лоад плеера, то читаем файл дальше и иним все остальные поля

///////////////////////////////////////////////////////////////////////////////


	// character init
	// initializations necessary to keep some things straight

	this->set_npc_name(0);
	this->player_data.long_descr = "";

	this->real_abils.Feats.reset();

	// волхвам сетим все спеллы на рунах, остальные инит нулями
	if (GET_CLASS(this) != CLASS_DRUID)
		for (i = 1; i <= MAX_SPELLS; i++)
			GET_SPELL_TYPE(this, i) = 0;
	else
		for (i = 1; i <= MAX_SPELLS; i++)
			GET_SPELL_TYPE(this, i) = SPELL_RUNES;

	for (i = 1; i <= MAX_SPELLS; i++)
		GET_SPELL_MEM(this, i) = 0;
	this->char_specials.saved.affected_by = clear_flags;
	POOFIN(this) = NULL;
	POOFOUT(this) = NULL;
	GET_RSKILL(this) = NULL;	// рецептов не знает
	this->char_specials.carry_weight = 0;
	this->char_specials.carry_items = 0;
	this->real_abils.armor = 100;
	GET_MEM_TOTAL(this) = 0;
	GET_MEM_COMPLETED(this) = 0;
	MemQ_init(this);

	GET_AC(this) = 10;
	GET_ALIGNMENT(this) = 0;
	GET_BAD_PWS(this) = 0;
	this->player_data.time.birth = time(0);
	GET_KIN(this) = 0;

	this->set_str(10);
	this->set_dex(10);
	this->set_con(10);
	this->set_int(10);
	this->set_wis(10);
	this ->set_cha(10);

	GET_COND(this, DRUNK) = 0;
	GET_DRUNK_STATE(this) = 0;

// Punish Init
	DUMB_DURATION(this) = 0;
	DUMB_REASON(this) = 0;
	GET_DUMB_LEV(this) = 0;
	DUMB_GODID(this) = 0;

	MUTE_DURATION(this) = 0;
	MUTE_REASON(this) = 0;
	GET_MUTE_LEV(this) = 0;
	MUTE_GODID(this) = 0;

	HELL_DURATION(this) = 0;
	HELL_REASON(this) = 0;
	GET_HELL_LEV(this) = 0;
	HELL_GODID(this) = 0;

	FREEZE_DURATION(this) = 0;
	FREEZE_REASON(this) = 0;
	GET_FREEZE_LEV(this) = 0;
	FREEZE_GODID(this) = 0;

	GCURSE_DURATION(this) = 0;
	GCURSE_REASON(this) = 0;
	GET_GCURSE_LEV(this) = 0;
	GCURSE_GODID(this) = 0;

	NAME_DURATION(this) = 0;
	NAME_REASON(this) = 0;
	GET_NAME_LEV(this) = 0;
	NAME_GODID(this) = 0;

	UNREG_DURATION(this) = 0;
	UNREG_REASON(this) = 0;
	GET_UNREG_LEV(this) = 0;
	UNREG_GODID(this) = 0;

// End punish init

	GET_DR(this) = 0;

	set_gold(0, false);
	set_bank(0, false);
	set_ruble(0);
	this->player_specials->saved.GodsLike = 0;
	GET_HIT(this) = 21;
	GET_MAX_HIT(this) = 21;
	GET_HEIGHT(this) = 50;
	GET_HR(this) = 0;
	GET_COND(this, FULL) = 0;
	SET_INVIS_LEV(this, 0);
	this->player_data.time.logon = time(0);
	GET_MOVE(this) = 44;
	GET_MAX_MOVE(this) = 44;
	KARMA(this) = 0;
	LOGON_LIST(this).clear();
	NAME_GOD(this) = 0;
	STRING_LENGTH(this) = 80;
	STRING_WIDTH(this) = 30;
	NAME_ID_GOD(this) = 0;
	GET_OLC_ZONE(this) = 0;
	this->player_data.time.played = 0;
	GET_LOADROOM(this) = NOWHERE;
	GET_RELIGION(this) = 1;
	GET_RACE(this) = 1;
	GET_SEX(this) = ESex::SEX_NEUTRAL;
	GET_COND(this, THIRST) = NORM_COND_VALUE;
	GET_WEIGHT(this) = 50;
	GET_WIMP_LEV(this) = 0;
	PRF_FLAGS(this).from_string("");	// suspicious line: we should clear flags.. Loading from "" does not clear flags.
	AFF_FLAGS(this).from_string("");	// suspicious line: we should clear flags.. Loading from "" does not clear flags.
	GET_PORTALS(this) = NULL;
	EXCHANGE_FILTER(this) = NULL;
	clear_ignores();
	CREATE(GET_LOGS(this), 1 + LAST_LOG);
	NOTIFY_EXCH_PRICE(this) = 0;
	this->player_specials->saved.HiredCost = 0;
	this->set_who_mana(WHO_MANA_MAX);
	this->set_who_last(time(0));

	while (fbgetline(fl, line))
	{
		tag_argument(line, tag);
		for (i = 0; !(line[i] == ' ' || line[i] == '\0'); i++)
		{
			line1[i] = line[i];
		}
		line1[i] = '\0';
		num = atoi(line1);
		lnum = atol(line1);
		try
		{
			llnum = std::stoull(line1, nullptr, 10);
		}
		catch (const std::invalid_argument &)
		{
			llnum = 0;
		}
		catch (const std::out_of_range &)
		{
		    llnum = 0;
		}
		switch (*tag)
		{
		case 'A':
			if (!strcmp(tag, "Ac  "))
			{
				GET_AC(this) = num;
			}
			else if (!strcmp(tag, "Aff "))
			{
				AFF_FLAGS(this).from_string(line);
			}
			else if (!strcmp(tag, "Affs"))
			{
				i = 0;
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
					if (num > 0)
					{
						AFFECT_DATA<EApplyLocation> af;
						af.type = num;
						af.duration = num2;
						af.modifier = num3;
						af.location = static_cast<EApplyLocation>(num4);
						af.bitvector = num5;
						af.battleflag = num6;
						if (af.type == SPELL_LACKY)
						{
							af.handler.reset(new LackyAffectHandler());
						}
						affect_to_char(this, af);
						i++;
					}
				} while (num != 0);
				/* do not load affects */
			}
			else if (!strcmp(tag, "Alin"))
			{
				GET_ALIGNMENT(this) = num;
			}
			break;

		case 'B':
			if (!strcmp(tag, "Badp"))
			{
				GET_BAD_PWS(this) = num;
			}
			else if (!strcmp(tag, "Bank"))
			{
				set_bank(lnum, false);
			}
			else if (!strcmp(tag, "Br01"))
				set_board_date(Boards::GENERAL_BOARD, llnum);
			else if (!strcmp(tag, "Br02"))
				set_board_date(Boards::NEWS_BOARD, llnum);
			else if (!strcmp(tag, "Br03"))
				set_board_date(Boards::IDEA_BOARD, llnum);
			else if (!strcmp(tag, "Br04"))
				set_board_date(Boards::ERROR_BOARD, llnum);
			else if (!strcmp(tag, "Br05"))
				set_board_date(Boards::GODNEWS_BOARD, llnum);
			else if (!strcmp(tag, "Br06"))
				set_board_date(Boards::GODGENERAL_BOARD, llnum);
			else if (!strcmp(tag, "Br07"))
				set_board_date(Boards::GODBUILD_BOARD, llnum);
			else if (!strcmp(tag, "Br08"))
				set_board_date(Boards::GODCODE_BOARD, llnum);
			else if (!strcmp(tag, "Br09"))
				set_board_date(Boards::GODPUNISH_BOARD, llnum);
			else if (!strcmp(tag, "Br10"))
				set_board_date(Boards::PERS_BOARD, llnum);
			else if (!strcmp(tag, "Br11"))
				set_board_date(Boards::CLAN_BOARD, llnum);
			else if (!strcmp(tag, "Br12"))
				set_board_date(Boards::CLANNEWS_BOARD, llnum);
			else if (!strcmp(tag, "Br13"))
				set_board_date(Boards::NOTICE_BOARD, llnum);
			else if (!strcmp(tag, "Br14"))
				set_board_date(Boards::MISPRINT_BOARD, llnum);
			else if (!strcmp(tag, "Br15"))
				set_board_date(Boards::SUGGEST_BOARD, llnum);
			else if (!strcmp(tag, "Br16"))
				set_board_date(Boards::CODER_BOARD, llnum);

			else if (!strcmp(tag, "Brth"))
				this->player_data.time.birth = lnum;
			break;

		case 'C':
			if (!strcmp(tag, "Cha "))
				this->set_cha(num);
			else if (!strcmp(tag, "Con "))
				this->set_con(num);
			else if (!strcmp(tag, "CntS"))
				this->reset_stats_cnt_[ResetStats::Type::MAIN_STATS] = num;
			else if (!strcmp(tag, "CntR"))
				this->reset_stats_cnt_[ResetStats::Type::RACE] = num;
			else if (!strcmp(tag, "CntF"))
				this->reset_stats_cnt_[ResetStats::Type::FEATS] = num;
			else if (!strcmp(tag, "Cits"))
			{
				std::string buffer_cities = std::string(line);
				// это на тот случай, если вдруг количество городов поменялось
				if (buffer_cities.size() != ::cities.size())
				{
					// если меньше
					if (buffer_cities.size() < ::cities.size())
					{
						const size_t b_size = buffer_cities.size();
						// то добиваем нулями
						for (unsigned int i = 0; i < ::cities.size() - b_size; i++)
							buffer_cities += "0";
					}
					else
					{
						// режем строку
						buffer_cities.resize(buffer_cities.size() - (buffer_cities.size() - ::cities.size()));
					}
				}
				this->str_to_cities(std::string(buffer_cities));
			}
			break;

		case 'D':
			if (!strcmp(tag, "Desc"))
			{
				const auto ptr = fbgetstring(fl);
				this->player_data.description = ptr ? ptr : "";
			}
			else if (!strcmp(tag, "Disp"))
			{
				std::bitset<DIS_TOTAL_NUM> tmp_flags(lnum);
				disposable_flags_ = tmp_flags;
			}
			else if (!strcmp(tag, "Dex "))
				this->set_dex(num);
			else if (!strcmp(tag, "Drnk"))
				GET_COND(this, DRUNK) = num;
			else if (!strcmp(tag, "DrSt"))
				GET_DRUNK_STATE(this) = num;
			else if (!strcmp(tag, "Drol"))
				GET_DR(this) = num;
			else if (!strcmp(tag, "DaiQ"))
			{

				if (sscanf(line, "%d %d %ld", &num, &num2, &lnum) == 2)
				{
					this->add_daily_quest(num, num2);
				}
				else
				{
					this->add_daily_quest(num, num2);
					this->set_time_daily_quest(num, lnum);
				}
				
			}
			break;

		case 'E':
			if (!strcmp(tag, "ExFl"))
				EXCHANGE_FILTER(this) = str_dup(line);
			else if (!strcmp(tag, "EMal"))
				strcpy(GET_EMAIL(this), line);
//29.11.09. (c) Василиса
//edited by WorM 2011.05.21
			else if (!strcmp(tag, "Expa"))
				GET_EXP_ARENA(this) = llnum;
			else if (!strcmp(tag, "Expm"))
				GET_EXP_MOB(this) = llnum;
			else if (!strcmp(tag, "Exmt"))
				GET_EXP_MOBTHIS(this) = llnum;
			else if (!strcmp(tag, "Expp"))
				GET_EXP_PK(this) = llnum;
			else if (!strcmp(tag, "Expt"))
				GET_EXP_PKTHIS(this) = llnum;
			else if (!strcmp(tag, "Expo"))
				GET_EXP_OTHER(this) = llnum;
			else if (!strcmp(tag, "Exot"))
				GET_EXP_OTHERTHIS(this) = llnum;
			else if (!strcmp(tag, "Expd"))
				GET_EXP_DT(this) = llnum;
			else if (!strcmp(tag, "Exdt"))
				GET_EXP_DTTHIS(this) = llnum;
//end by WorM
//Конец правки (с) Василиса
			break;

		case 'F':
			// Оставлено для совместимости со старым форматом наказаний
			if (!strcmp(tag, "Frez"))
				GET_FREEZE_LEV(this) = num;
			else if (!strcmp(tag, "Feat"))
			{
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d", &num);
					if (num > 0 && num < MAX_FEATS)
						if (feat_info[num].classknow[(int) GET_CLASS(this)][(int) GET_KIN(this)] || PlayerRace::FeatureCheck((int)GET_KIN(this),(int)GET_RACE(this),num))
							SET_FEAT(this, num);
				}
				while (num != 0);
			}
			else if (!strcmp(tag, "FtTm"))
			{
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0)
					{
						timed.skill = num;
						timed.time = num2;
						timed_feat_to_char(this, &timed);
					}
				}
				while (num != 0);
			}
			break;

		case 'G':
			if (!strcmp(tag, "Gold"))
			{
				set_gold(lnum, false);
			}
			else if (!strcmp(tag, "GodD"))
				GCURSE_DURATION(this) = lnum;
			else if (!strcmp(tag, "GdFl"))
				this->player_specials->saved.GodsLike = lnum;
			// added by WorM (Видолюб) 2010.06.04 бабки потраченные на найм(возвращаются при креше)
			else if (!strcmp(tag, "GldH"))
			{
				if(num != 0 && !IS_IMMORTAL(this) && can_use_feat(this, EMPLOYER_FEAT))
				{
					this->player_specials->saved.HiredCost = num;
				}
			}
			// end by WorM
			break;

		case 'H':
			if (!strcmp(tag, "Hit "))
			{
				sscanf(line, "%d/%d", &num, &num2);
				GET_HIT(this) = num;
				GET_MAX_HIT(this) = num2;
			}
			else if (!strcmp(tag, "Hite"))
				GET_HEIGHT(this) = num;
			else if (!strcmp(tag, "Hrol"))
				GET_HR(this) = num;
			else if (!strcmp(tag, "Hung"))
				GET_COND(this, FULL) = num;
			else if (!strcmp(tag, "Hry "))
			{
				if (num > 1200)
					num = 1200;
				this->set_hryvn(num);
			}
			else if (!strcmp(tag, "Host"))
				strcpy(GET_LASTIP(this), line);
			break;

		case 'I':
			if (!strcmp(tag, "Int "))
				this->set_int(num);
			else if (!strcmp(tag, "Invs"))
			{
				SET_INVIS_LEV(this, num);
			}
			else if (!strcmp(tag, "Ignr"))
			{
				IgnoresLoader ignores_loader(this);
				ignores_loader.load_from_string(line);
			}
			else if (!strcmp(tag, "ICur"))
			{
				this->set_ice_currency(num);
			}
			break;

		case 'K':
			if (!strcmp(tag, "Kin "))
				GET_KIN(this) = num;
			else if (!strcmp(tag, "Karm"))
				KARMA(this) = fbgetstring(fl);
			break;
		case 'L':
			if (!strcmp(tag, "LogL"))
			{
				long  lnum, lnum2;
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%s %ld %ld", &buf[0], &lnum, &lnum2);
					if (buf[0] != '~')
					{
						const logon_data cur_log = { str_dup(buf), lnum, lnum2, false };
						LOGON_LIST(this).push_back(cur_log);
					}
					else break;
				}
				while (true);

				if (!LOGON_LIST(this).empty())
				{
					LOGON_LIST(this).at(0).is_first = true;
					std::sort(LOGON_LIST(this).begin(), LOGON_LIST(this).end(),
						[](const logon_data& a, const logon_data& b)
					{
						return a.lasttime < b.lasttime;
					});
				}
			}
// Gunner
			else if (!strcmp(tag, "Logs"))
			{
				sscanf(line, "%d %d", &num, &num2);
				if (num >= 0 && num < 1 + LAST_LOG)
					GET_LOGS(this)[num] = num2;
			}
			else if (!strcmp(tag, "Lexc"))
				this->set_last_exchange(num);
			break;

		case 'M':
			if (!strcmp(tag, "Mana"))
			{
				sscanf(line, "%d/%d", &num, &num2);
				GET_MEM_COMPLETED(this) = num;
				GET_MEM_TOTAL(this) = num2;
			}
			else if (!strcmp(tag, "Map "))
			{
				std::string str(line);
				std::bitset<MapSystem::TOTAL_MAP_OPTIONS> tmp(str);
				map_options_.bit_list_ = tmp;
			}
			else if (!strcmp(tag, "Move"))
			{
				sscanf(line, "%d/%d", &num, &num2);
				GET_MOVE(this) = num;
				GET_MAX_MOVE(this) = num2;
			}
			else if (!strcmp(tag, "Mobs"))
			{
				do
				{
					if (!fbgetline(fl, line))
						break;
					if (*line == '~')
						break;
					sscanf(line, "%d %d", &num, &num2);
					this->mobmax_load(this, num, num2, MobMax::get_level_by_vnum(num));
				}
				while (true);
			}
			else if (!strcmp(tag, "Mrph"))
			{
				morphs_load(this, std::string(line));
			}
			break;
		case 'N':
			if (!strcmp(tag, "NmI "))
				this->player_data.PNames[0] = std::string(line);
			else if (!strcmp(tag, "NmR "))
				this->player_data.PNames[1] = std::string(line);
			else if (!strcmp(tag, "NmD "))
				this->player_data.PNames[2] = std::string(line);
			else if (!strcmp(tag, "NmV "))
				this->player_data.PNames[3] = std::string(line);
			else if (!strcmp(tag, "NmT "))
				this->player_data.PNames[4] = std::string(line);
			else if (!strcmp(tag, "NmP "))
				this->player_data.PNames[5] = std::string(line);
			else if (!strcmp(tag, "NamD"))
				NAME_DURATION(this) = lnum;
			else if (!strcmp(tag, "NamG"))
				NAME_GOD(this) = num;
			else if (!strcmp(tag, "NaID"))
				NAME_ID_GOD(this) = lnum;
			else if (!strcmp(tag, "NtfE"))//Polud мин. цена для оффлайн-оповещений
				NOTIFY_EXCH_PRICE(this) = lnum;
			break;

		case 'O':
			if (!strcmp(tag, "Olc "))
				GET_OLC_ZONE(this) = num;
			break;


		case 'P':
			if (!strcmp(tag, "Pass"))
				this->set_passwd(line);
			else if (!strcmp(tag, "Plyd"))
				this->player_data.time.played = num;
			else if (!strcmp(tag, "PfIn"))
				POOFIN(this) = str_dup(line);
			else if (!strcmp(tag, "PfOt"))
				POOFOUT(this) = str_dup(line);
			else if (!strcmp(tag, "Pref"))
			{
				PRF_FLAGS(this).from_string(line);
			}
			else if (!strcmp(tag, "Pkil"))
			{
				do
				{
					if (!fbgetline(fl, line))
						break;
					if (*line == '~')
						break;
					sscanf(line, "%ld %d", &lnum, &num);

					if (lnum < 0 || !correct_unique(lnum))
						continue;
					struct PK_Memory_type * pk_one = NULL;
					for (pk_one = this->pk_list; pk_one; pk_one = pk_one->next)
						if (pk_one->unique == lnum)
							break;
					if (pk_one)
					{
						log("SYSERROR: duplicate entry pkillers data for %d %s", id, filename);
						continue;
					}

					CREATE(pk_one, 1);
					pk_one->unique = lnum;
					pk_one->kill_num = num;
					pk_one->next = this->pk_list;
					this->pk_list = pk_one;
				}
				while (true);
			}
			else if (!strcmp(tag, "Prtl"))
				add_portal_to_char(this, num);
			// Loads Here new punishment strings
			else if (!strcmp(tag, "PMut"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				MUTE_DURATION(this) = lnum;
				GET_MUTE_LEV(this) = num2;
				MUTE_GODID(this) = lnum3;
				MUTE_REASON(this) = str_dup(buf);
			}
			else if (!strcmp(tag, "PHel"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				HELL_DURATION(this) = lnum;
				GET_HELL_LEV(this) = num2;
				HELL_GODID(this) = lnum3;
				HELL_REASON(this) = str_dup(buf);
			}
			else if (!strcmp(tag, "PDum"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				DUMB_DURATION(this) = lnum;
				GET_DUMB_LEV(this) = num2;
				DUMB_GODID(this) = lnum3;
				DUMB_REASON(this) = str_dup(buf);
			}
			else if (!strcmp(tag, "PNam"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				NAME_DURATION(this) = lnum;
				GET_NAME_LEV(this) = num2;
				NAME_GODID(this) = lnum3;
				NAME_REASON(this) = str_dup(buf);
			}
			else if (!strcmp(tag, "PFrz"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				FREEZE_DURATION(this) = lnum;
				GET_FREEZE_LEV(this) = num2;
				FREEZE_GODID(this) = lnum3;
				FREEZE_REASON(this) = str_dup(buf);
			}
			else if (!strcmp(tag, "PGcs"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				GCURSE_DURATION(this) = lnum;
				GET_GCURSE_LEV(this) = num2;
				GCURSE_GODID(this) = lnum3;
				GCURSE_REASON(this) = str_dup(buf);
			}
			else if (!strcmp(tag, "PUnr"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				UNREG_DURATION(this) = lnum;
				GET_UNREG_LEV(this) = num2;
				UNREG_GODID(this) = lnum3;
				UNREG_REASON(this) = str_dup(buf);
			}

			break;

		case 'Q':
			if (!strcmp(tag, "Qst "))
			{
				buf[0] = '\0';
				sscanf(line, "%d %[^~]", &num, &buf[0]);
				this->quested_add(this, num, buf);
			}
			break;

		case 'R':
			if (!strcmp(tag, "Room"))
				GET_LOADROOM(this) = num;
//29.11.09. (c) Василиса
			else if (!strcmp(tag, "Ripa"))
				GET_RIP_ARENA(this) = num;
			else if (!strcmp(tag, "Ripm"))
				GET_RIP_MOB(this) = num;
			else if (!strcmp(tag, "Rimt"))
				GET_RIP_MOBTHIS(this) = num;
			else if (!strcmp(tag, "Ruble"))
				this->set_ruble(num);
			else if (!strcmp(tag, "Ripp"))
				GET_RIP_PK(this) = num;
			else if (!strcmp(tag, "Ript"))
				GET_RIP_PKTHIS(this) = num;
			else if (!strcmp(tag, "Ripo"))
				GET_RIP_OTHER(this) = num;
			else if (!strcmp(tag, "Riot"))
				GET_RIP_OTHERTHIS(this) = num;
			else if (!strcmp(tag, "Ripd"))
				GET_RIP_DT(this) = num;
			else if (!strcmp(tag, "Ridt"))
				GET_RIP_DTTHIS(this) = num;
//(с) Василиса
			else if (!strcmp(tag, "Rmbr"))
				this->remember_set_num(num);
			else if (!strcmp(tag, "Reli"))
				GET_RELIGION(this) = num;
			else if (!strcmp(tag, "Race"))
				GET_RACE(this) = num;
			else if (!strcmp(tag, "Rcps"))
			{
				im_rskill *last = NULL;
				for (;;)
				{
					im_rskill *rs;
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num < 0)
						break;
					num = im_get_recipe(num);
// +newbook.patch (Alisher)
					if (num < 0 || imrecipes[num].classknow[(int) GET_CLASS(this)] != KNOW_RECIPE)
// -newbook.patch (Alisher)
						continue;
					CREATE(rs,  1);
					rs->rid = num;
					rs->perc = num2;
					rs->link = NULL;
					if (last)
						last->link = rs;
					else
						GET_RSKILL(this) = rs;
					last = rs;
				}
			}
			break;

		case 'S':
			if (!strcmp(tag, "Size"))
				GET_SIZE(this) = num;
			else if (!strcmp(tag, "Sex "))
			{
				GET_SEX(this) = static_cast<ESex>(num);
			}
			else if (!strcmp(tag, "Skil"))
			{
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0)
                    {
                        if (skill_info[num].classknow[(int)GET_CLASS(this)][(int)GET_KIN(this)] == KNOW_SKILL)
                        {
                            this->set_skill(static_cast<ESkill>(num), num2);
                        }
                    }
				}
				while (num != 0);
			}
			else if (!strcmp(tag, "SkTm"))
			{
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0)
					{
						timed.skill = num;
						timed.time = num2;
						timed_to_char(this, &timed);
					}
				}
				while (num != 0);
			}
			else if (!strcmp(tag, "Spel"))
			{
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0 && spell_info[num].name)
						GET_SPELL_TYPE(this, num) = num2;
				}
				while (num != 0);
			}
			else if (!strcmp(tag, "SpMe"))
			{
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0)
						GET_SPELL_MEM(this, num) = num2;
				}
				while (num != 0);
			}
			else if (!strcmp(tag, "SpTM"))
			{
				struct spell_mem_queue_item *qi_cur, ** qi = &MemQueue.queue;
				while (*qi)
					qi = &((*qi)->link);
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d", &num);
					if (num != 0)
					{
						CREATE(qi_cur, 1);
						*qi = qi_cur;
						qi_cur->spellnum = num;
						qi_cur->link = NULL;
						qi = &qi_cur->link;
					}
				}
				while (num != 0);
			}
			else if (!strcmp(tag, "Str "))
				this->set_str(num);
			else if (!strcmp(tag, "StrL"))
				STRING_LENGTH(this) = num;
			else if (!strcmp(tag, "StrW"))
				STRING_WIDTH(this) = num;
			else if (!strcmp(tag, "St00"))
				this->set_start_stat(G_STR, lnum);
			else if (!strcmp(tag, "St01"))
				this->set_start_stat(G_DEX, lnum);
			else if (!strcmp(tag, "St02"))
				this->set_start_stat(G_INT, lnum);
			else if (!strcmp(tag, "St03"))
				this->set_start_stat(G_WIS, lnum);
			else if (!strcmp(tag, "St04"))
				this->set_start_stat(G_CON, lnum);
			else if (!strcmp(tag, "St05"))
				this->set_start_stat(G_CHA, lnum);
			break;

		case 'T':
			if (!strcmp(tag, "Thir"))
				GET_COND(this, THIRST) = num;
			else if (!strcmp(tag, "Titl"))
				GET_TITLE(this) = std::string(str_dup(line));
			else if (!strcmp(tag, "TrcG"))
				set_ext_money(ExtMoney::TORC_GOLD, num, false);
			else if (!strcmp(tag, "TrcS"))
				set_ext_money(ExtMoney::TORC_SILVER, num, false);
			else if (!strcmp(tag, "TrcB"))
				set_ext_money(ExtMoney::TORC_BRONZE, num, false);
			else if (!strcmp(tag, "TrcL"))
			{
				sscanf(line, "%d %d", &num, &num2);
				today_torc_.first = num;
				today_torc_.second = num2;
			}
			else if (!strcmp(tag, "TSpl"))
			{
				do
				{
					fbgetline(fl, line);
					sscanf(line, "%d %ld %ld", &num, &lnum, &lnum3);
					if (num != 0 && spell_info[num].name)
					{
						Temporary_Spells::add_spell(this, num, lnum, lnum3);
					}
				} while (num != 0);
			}
			break;

		case 'W':
			if (!strcmp(tag, "Wate"))
				GET_WEIGHT(this) = num;
			else if (!strcmp(tag, "Wimp"))
				GET_WIMP_LEV(this) = num;
			else if (!strcmp(tag, "Wis "))
				this->set_wis(num);
//29.11.09 (c) Василиса
			else if (!strcmp(tag, "Wina"))
				GET_WIN_ARENA(this) = num;
//конец правки (с) Василиса
			else if (!strcmp(tag, "Wman"))
				this->set_who_mana(num);
			break;

		default:
			sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
		}
	}
	PRF_FLAGS(this).set(PRF_COLOR_2); //всегда цвет полный
	// initialization for imms
	if (GET_LEVEL(this) >= LVL_IMMORT)
	{
		set_god_skills(this);
		set_god_morphs(this);
		GET_COND(this, FULL) = -1;
		GET_COND(this, THIRST) = -1;
		GET_COND(this, DRUNK) = -1;
		GET_LOADROOM(this) = NOWHERE;
	}

	// Set natural & race features - added by Gorrah
	set_natural_feats(this);

	if (IS_GRGOD(this))
	{
		for (i = 0; i <= MAX_SPELLS; i++)
			GET_SPELL_TYPE(this, i) = GET_SPELL_TYPE(this, i) |
									SPELL_ITEMS | SPELL_KNOW | SPELL_RUNES | SPELL_SCROLL | SPELL_POTION | SPELL_WAND;
	}
	else if (!IS_IMMORTAL(this))
	{
		for (i = 0; i <= MAX_SPELLS; i++)
		{
			if (spell_info[i].slot_forc[(int) GET_CLASS(this)][(int) GET_KIN(this)] == MAX_SLOT)
				REMOVE_BIT(GET_SPELL_TYPE(this, i), SPELL_KNOW | SPELL_TEMP);
// shapirus: изученное не убираем на всякий случай, но из мема выкидываем,
// если мортов мало
			if (GET_REMORT(this) < MIN_CAST_REM(spell_info[i], this))
				GET_SPELL_MEM(this, i) = 0;
		}
	}

	/*
	 * If you're not poisioned and you've been away for more than an hour of
	 * real time, we'll set your HMV back to full
	 */
	if (!AFF_FLAGGED(this, EAffectFlag::AFF_POISON) && (((long)(time(0) - LAST_LOGON(this))) >= SECS_PER_REAL_HOUR))
	{
		GET_HIT(this) = GET_REAL_MAX_HIT(this);
		GET_MOVE(this) = GET_REAL_MAX_MOVE(this);
	}
	else
		GET_HIT(this) = MIN(GET_HIT(this), GET_REAL_MAX_HIT(this));

	fbclose(fl);
	// здесь мы закладываемся на то, что при ребуте это все сейчас пропускается и это нормально,
	// иначе в таблице crc будут пустые имена, т.к. сама плеер-таблица еще не сформирована
	// и в любом случае при ребуте это все пересчитывать не нужно
	FileCRC::check_crc(filename, FileCRC::PLAYER, GET_UNIQUE(this));
	
	this->account = Account::get_account(GET_EMAIL(this));
	if (this->account == nullptr)
	{
		const auto temp_account = std::make_shared<Account>(GET_EMAIL(this));
		accounts.emplace(GET_EMAIL(this), temp_account);
		this->account = temp_account;
	}
	this->account->add_player(GET_UNIQUE(this));
	return (id);
}

bool Player::get_disposable_flag(int num)
{
	if (num < 0 || num >= DIS_TOTAL_NUM)
	{
		log("SYSERROR: num=%d (%s %s:%d)", num, __func__, __FILE__, __LINE__);
		return false;
	}
	return disposable_flags_[num];
}

void Player::set_disposable_flag(int num)
{
	if (num < 0 || num >= DIS_TOTAL_NUM)
	{
		log("SYSERROR: num=%d (%s %s:%d)", num, __func__, __FILE__, __LINE__);
		return;
	}
	disposable_flags_.set(num);
}

bool Player::is_active() const
{
	return motion_;
}

void Player::set_motion(bool flag)
{
	motion_ = flag;
}

void Player::map_olc()
{
	std::shared_ptr<MapSystem::Options> tmp(new MapSystem::Options);
	this->desc->map_options = tmp;

	*(this->desc->map_options) = map_options_;

	STATE(this->desc) = CON_MAP_MENU;
	this->desc->map_options->olc_menu(this);
}

void Player::map_olc_save()
{
	map_options_ = *(this->desc->map_options);
}

bool Player::map_check_option(int num) const
{
	return map_options_.bit_list_.test(num);
}

void Player::map_set_option(unsigned num)
{
	if (num < map_options_.bit_list_.size())
	{
		map_options_.bit_list_.set(num);
	}
}

void Player::map_text_olc(const char *arg)
{
	map_options_.text_olc(this, arg);
}

const MapSystem::Options * Player::get_map_options() const
{
	return &map_options_;
}

void Player::map_print_to_snooper(CHAR_DATA *imm)
{
	MapSystem::Options tmp;
	tmp = map_options_;
	map_options_ = *(imm->get_map_options());
	// подменяем флаги карты на снуперские перед распечаткой ему карты
	MapSystem::print_map(this, imm);
	map_options_ = tmp;
}

int Player::get_ext_money(unsigned type) const
{
	if (type < ext_money_.size())
	{
		return ext_money_[type];
	}
	return 0;
}

void Player::set_ext_money(unsigned type, int num, bool write_log)
{
	if (num < 0 || num > MAX_MONEY_KEPT)
	{
		return;
	}
	if (type < ext_money_.size())
	{
		const int diff = num - ext_money_[type];
		ext_money_[type] = num;
		if (diff != 0 && write_log)
		{
			ExtMoney::player_drop_log(this, type, diff);
		}
	}
}

int Player::get_today_torc()
{
	uint8_t day = get_day_today();
	if (today_torc_.first != day)
	{
		today_torc_.first = day;
		today_torc_.second = 0;
	}

	return today_torc_.second;
}

void Player::add_today_torc(int num)
{
	uint8_t day = get_day_today();
	if (today_torc_.first == day)
	{
		today_torc_.second += num;
	}
	else
	{
		today_torc_.first = day;
		today_torc_.second = num;
	}
}

int Player::get_reset_stats_cnt(ResetStats::Type type) const
{
	return reset_stats_cnt_.at(type);
}

int Player::get_ice_currency()
{
	return this->ice_currency;
}

void Player::set_ice_currency(int value)
{
	this->ice_currency = value;
}

void Player::add_ice_currency(int value)
{
	this->ice_currency += value;
}

void Player::sub_ice_currency(int value)
{
	this->ice_currency = MAX(0, ice_currency - value);
}

bool Player::is_arena_player()
{
	return this->arena_player;
}


int Player::get_count_daily_quest(int id)
{
	if (this->daily_quest.count(id))
		return this->daily_quest[id];
	return 0;
	
}

time_t Player::get_time_daily_quest(int id)
{
	if (this->daily_quest_timed.count(id))
		return this->daily_quest_timed[id];
	return 0;
}

void Player::add_value_cities(bool v)
{
	this->cities.push_back(v);
}

void Player::reset_daily_quest()
{
	this->daily_quest.clear();
	this->daily_quest_timed.clear();
	log("Персонаж: %s. Были сброшены гривны.", GET_NAME(this));
}

std::shared_ptr<Account> Player::get_account()
{
	return this->account;
}


void Player::set_time_daily_quest(int id, time_t time)
{
	if (this->daily_quest_timed.count(id))
	{
		this->daily_quest_timed[id] = time;
	}
	else
	{
		this->daily_quest_timed.insert(std::pair<int, int>(id, time));
	}
}

void Player::add_daily_quest(int id, int count)
{
	if (this->daily_quest.count(id))
	{
		this->daily_quest[id] += count;
	}
	else
	{
		this->daily_quest.insert(std::pair<int, int>(id, count));
	}
	time_t now = time(0);
	if (this->daily_quest_timed.count(id))
	{
		this->daily_quest_timed[id] = now;
	}
	else
	{
		this->daily_quest_timed.insert(std::pair<int, time_t>(id, now));
	}
}

void Player::spent_hryvn_sub(int value)
{
	this->spent_hryvn += value;
}

int Player::get_spent_hryvn()
{
	return this->spent_hryvn;
}

int Player::death_player_count()
{
	const int zone_vnum = zone_table[world[this->in_room]->zone].number;
	auto it = this->count_death_zone.find(zone_vnum);
	if (it != this->count_death_zone.end())
	{
		count_death_zone.at(zone_vnum) += 1;
	}
	else
	{
		count_death_zone.insert(std::pair<int, int>(zone_vnum, 1));
		return 1;
	}
	return (*it).second;
}


void Player::inc_reset_stats_cnt(ResetStats::Type type)
{
	reset_stats_cnt_.at(type) += 1;
}

time_t Player::get_board_date(Boards::BoardTypes type) const
{
	return board_date_.at(type);
}

void Player::set_board_date(Boards::BoardTypes type, time_t date)
{
	board_date_.at(type) = date;
}

namespace PlayerSystem
{

///
/// \return кол-во хп, втыкаемых чару от родного тела
///
int con_natural_hp(CHAR_DATA *ch)
{
	double add_hp_per_level = class_app[GET_CLASS(ch)].base_con
		+ (VPOSI_MOB(ch, 2, ch->get_con()) - class_app[GET_CLASS(ch)].base_con)
		* class_app[GET_CLASS(ch)].koef_con / 100.0 + 3;
	return 10 + static_cast<int>(add_hp_per_level * GET_LEVEL(ch));
}

///
/// \return кол-во хп, втыкаемых чару от добавленного шмотом/аффектами тела
///
int con_add_hp(CHAR_DATA *ch)
{
	int con_add = MAX(0, GET_REAL_CON(ch) - ch->get_con());
	return class_app[(int)GET_CLASS(ch)].koef_con * con_add * GET_LEVEL(ch) / 100;
}

///
/// \return кол-во хп, втыкаемых чару от общего кол-ва тела
///
int con_total_hp(CHAR_DATA *ch)
{
	return con_natural_hp(ch) + con_add_hp(ch);
}

///
/// \return величина минуса к дексе в случае перегруза (case -> проценты от макс)
///
unsigned weight_dex_penalty(CHAR_DATA* ch)
{
	int n = 0;
	switch (IS_CARRYING_W(ch) * 10 / MAX(1, CAN_CARRY_W(ch)))
	{
	case 10:
	case 9:
	case 8:
		n = 2;
		break;
	case 7:
	case 6:
	case 5:
		n = 1;
		break;
	}
	return n;
}

} // namespace PlayerSystem

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
