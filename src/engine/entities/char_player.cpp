// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "char_player.h"

#include <sys/stat.h>

#include "utils/file_crc.h"
#include "utils/backtrace.h"

#include "gameplay/communication/ignores_loader.h"
#include "engine/olc/olc.h"
#include "utils/diskio.h"
#include "gameplay/core/genchar.h"
#include "engine/core/handler.h"
#include "engine/db/global_objects.h"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/economics/ext_money.h"
#include "gameplay/magic/magic_temp_spells.h"
#include "administration/accounts.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/mechanics/cities.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/dungeons.h"
#include "engine/ui/cmd/do_who.h"
#include "engine/db/player_index.h"

#ifdef _WIN32
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


long GetExpUntilNextLvl(CharData *ch, int level);

namespace {

uint8_t get_day_today() {
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	return timeinfo->tm_mday;
}

} // namespace

Player::Player() :
	pfilepos_(-1),
	was_in_room_(kNowhere),
	from_room_(0),
	answer_id_(kNobody),
	motion_(true),
	ice_currency(0),
	hryvn(0),
	nogata(0),
	spent_hryvn(0) {
	for (int i = 0; i < START_STATS_TOTAL; ++i) {
		start_stats_.at(i) = 0;
	}

	// на 64 битном центосе с оптимизацией - падает или прямо здесь,
	// или в деструкторе чар-даты на делете самого класса в недрах шареда
	// при сборке без оптимизаций - не падает
	// и я не очень в теме, чем этот инит отличается от инита в чар-дате,
	// с учетом того, что здесь у нас абсолютно пустой плеер и внутри set_morph
	// на деле инит ровно тоже самое, может на перспективу это все было
	//set_morph(NormalMorph::GetNormalMorph(this));

	for (unsigned i = 0; i < ext_money_.size(); ++i) {
		ext_money_[i] = 0;
	}

	for (unsigned i = 0; i < reset_stats_cnt_.size(); ++i) {
		reset_stats_cnt_.at(i) = 0;
	}

	// чтобы не вываливать новому игроку все мессаги на досках как непрочитанные
	const time_t now = time(nullptr);
	board_date_.fill(now);
}

int Player::get_pfilepos() const {
	return pfilepos_;
}

void Player::set_pfilepos(int pfilepos) {
	pfilepos_ = pfilepos;
}

RoomRnum Player::get_was_in_room() const {
	return was_in_room_;
}

void Player::set_was_in_room(RoomRnum was_in_room) {
	was_in_room_ = was_in_room;
}

std::string const &Player::get_passwd() const {
	return passwd_;
}

void Player::set_passwd(std::string const &passwd) {
	passwd_ = passwd;
}

void Player::remort() {
	quested_.clear();
	mobmax_.clear();
	count_death_zone.clear();
}

void Player::reset() {
	remember_.reset();
	last_tell_ = "";
	answer_id_ = kNobody;
	CharData::reset();
}

RoomRnum Player::get_from_room() const {
	return from_room_;
}

void Player::set_from_room(RoomRnum from_room) {
	from_room_ = from_room;
}

int Player::get_start_stat(int stat_num) {
	int stat = 0;
	try {
		stat = start_stats_.at(stat_num);
	}
	catch (...) {
		log("SYSERROR : bad start_stat %d (%s %s %d)", stat_num, __FILE__, __func__, __LINE__);
	}
	return stat;
}

void Player::set_start_stat(int stat_num, int number) {
	try {
		start_stats_.at(stat_num) = number;
	}
	catch (...) {
		log("SYSERROR : bad start_stat num %d (%s %s %d)", stat_num, __FILE__, __func__, __LINE__);
	}
}

void Player::set_answer_id(int id) {
	answer_id_ = id;
}

int Player::get_answer_id() const {
	return answer_id_;
}

void Player::remember_add(const std::string &text, int flag) {
	remember_.add_str(text, flag);
}

std::string Player::remember_get(int flag) const {
	return remember_.get_text(flag);
}

bool Player::remember_set_num(unsigned int num) {
	return remember_.set_num_str(num);
}

unsigned int Player::remember_get_num() const {
	return remember_.get_num_str();
}

void Player::set_last_tell(const char *text) {
	if (text) {
		last_tell_ = text;
	}
}


int Player::get_hryvn() {
	return this->hryvn;
}

short cap_hryvn = 1500;

void Player::set_hryvn(int value) {
	if (value > cap_hryvn)
		value = cap_hryvn;
	this->hryvn = value;
}

void Player::sub_hryvn(int value) {
	this->hryvn -= value;
}

int Player::get_nogata() {
	return this->nogata;
}

void Player::set_nogata(int value) {
	this->nogata = value;
}

void Player::sub_nogata(int value) {
	this->nogata -= value;
}

void Player::add_nogata(int value) {
	this->nogata += value;
	SendMsgToChar(this, "Вы получили %ld %s.\r\n", static_cast<long>(value),
				  GetDeclensionInNumber(value, EWhat::kNogataU));

}

void Player::add_hryvn(int value) {
	if (GetRealRemort(this) < 6) {
		SendMsgToChar(this, "Глянув на непонятный слиток, Вы решили выкинуть его...\r\n");
		return;
	} 
	if (zone_table[world[this->in_room]->zone_rn].under_construction) {
		SendMsgToChar(this, "Зона тестовая, вашу гривну отобрали боги.\r\n");
		return;
	}
	if ((this->get_hryvn() + value) > cap_hryvn) {
		value = cap_hryvn - this->get_hryvn();
		SendMsgToChar(this, "Вы получили только %ld %s, так как в вашу копилку больше не лезет...\r\n",
					  static_cast<long>(value), GetDeclensionInNumber(value, EWhat::kTorcU));
	} else if (value > 0) {
		SendMsgToChar(this, "Вы получили %ld %s.\r\n",
					  static_cast<long>(value), GetDeclensionInNumber(value, EWhat::kTorcU));
	} else if (value == 0) {
		return;
	}
	log("Персонаж %s получил %d [гривны].", GET_NAME(this), value);
	this->hryvn += value;
}

void Player::complete_quest(const int id) {
	this->account->complete_quest(id);
}

void Player::dquest(const int id) {
	const auto quest = MUD::daily_quests().find(id);

	if (quest == MUD::daily_quests().end()) {
		log("Quest Id: %d - не найден", id);
		return;
	}
	if (!this->account->quest_is_available(id)) {
		SendMsgToChar(this, "Сегодня вы уже получали гривны за выполнение этого задания.\r\n");
		return;
	}
	int value = quest->second.reward + number(1, 3);
	const int zone_lvl = zone_table[world[this->in_room]->zone_rn].mob_level;
	value = this->account->zero_hryvn(this, value);
	if (zone_lvl < 25
		&& zone_lvl <= (GetRealLevel(this) + GetRealRemort(this) / 5)) {
		value /= 2;
	}
	this->add_hryvn(value);
	this->account->complete_quest(id);
}

void Player::mark_city(const size_t index) {
	if (index < cities_visited_.size()) {
		cities_visited_[index] = true;
	}
}

bool Player::check_city(const size_t index) {
	if (index < cities_visited_.size()) {
		return cities_visited_[index];
	}

	return false;
}

void Player::str_to_cities(std::string str) {
	this->cities_visited_.clear();
	for (auto &it : reverse(str)) {
		if (it == '1')
			this->cities_visited_.push_back(true);
		else
			this->cities_visited_.push_back(false);
	}
}

std::string Player::cities_to_str() {
	std::string value = "";

	for (auto it : reverse(this->cities_visited_)) {
		if (it)
			value += "1";
		else
			value += "0";
	}
	return value;
}

std::string const &Player::get_last_tell() {
	return last_tell_;
}

void Player::quested_add(CharData *ch, int vnum, char *text) {
	quested_.add(ch, vnum, text);
}

bool Player::quested_remove(int vnum) {
	return quested_.remove(vnum);
}

bool Player::quested_get(int vnum) const {
	return quested_.get(vnum);
}

std::string Player::quested_get_text(int vnum) const {
	return quested_.get_text(vnum);
}

std::string Player::quested_print() const {
	return quested_.print();
}

void Player::quested_save(FILE *saved) const {
	quested_.save(saved);
}

int Player::mobmax_get(int vnum) const {
	return mobmax_.get_kill_count(vnum);
}

void Player::mobmax_add(CharData *ch, int vnum, int count, int level) {
	mobmax_.add(ch, vnum, count, level);
}

void Player::mobmax_load(CharData *ch, int vnum, int count, int level) {
	mobmax_.load(ch, vnum, count, level);
}

void Player::mobmax_remove(int vnum) {
	mobmax_.remove(vnum);
}

void Player::mobmax_save(FILE *saved) const {
	mobmax_.save(saved);
}

void Player::show_mobmax() {
	MobMax::mobmax_stats_t stats;
	mobmax_.get_stats(stats);
	int i = 0;
	for (const auto &item : stats) {
		SendMsgToChar(this,
					  "%2d. Уровень: %d; Убито: %d; Всего до размакса: %d\n",
					  ++i,
					  item.first,
					  item.second,
					  get_max_kills(item.first));
	}
}

void Player::dps_add_dmg(int type, int dmg, int over_dmg, CharData *ch) {
	dps_.AddDmg(type, ch, dmg, over_dmg);
}

void Player::dps_clear(int type) {
	dps_.Clear(type);
}

void Player::dps_print_stats(CharData *coder) {
	dps_.PrintStats(this, coder);
}

void Player::dps_print_group_stats(CharData *ch, CharData *coder) {
	dps_.PrintGroupStats(ch, coder);
}

// * Для dps_copy.
void Player::dps_set(DpsSystem::Dps *dps) {
	dps_ = *dps;
}

// * Нужно только для копирования всего этого дела при передаче лидера.
void Player::dps_copy(CharData *ch) {
	ch->dps_set(&dps_);
}

void Player::dps_end_round(int type, CharData *ch) {
	dps_.EndRound(type, ch);
}

void Player::dps_add_exp(int exp, bool battle) {
	if (battle) {
		dps_.AddBattleExp(exp);
	} else {
		dps_.AddExp(exp);
	}
}

// не дергать wear/remove триги при скрытом раздевании/одевании чара во время сейва
#define NO_EXTRANEOUS_TRIGGERS

void Player::save_char() {
	FILE *saved;
	char filename[kMaxStringLength];
	int i;
	time_t li;
	ObjData *char_eq[EEquipPos::kNumEquipPos];
	struct TimedSkill *skj;

	int tmp = time(0) - this->player_data.time.logon;

	if (this->IsNpc() || this->get_pfilepos() < 0)
		return;
	if (this->account == nullptr) {
		this->account = Account::get_account(GET_EMAIL(this));
		if (this->account == nullptr) {
			const auto temp_account = std::make_shared<Account>(GET_EMAIL(this));
			accounts.emplace(GET_EMAIL(this), temp_account);
			this->account = temp_account;
		}
	}
	this->account->save_to_file();
	log("Save char %s", GET_NAME(this));
	save_char_vars(this);

	// Запись чара в новом формате
	get_filename(GET_NAME(this), filename, kPlayersFile);
	if (!(saved = fopen(filename, "w"))) {
		perror("Unable open charfile");
		return;
	}
	// подготовка
	// снимаем все возможные аффекты
	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(this, i)) {
			char_eq[i] = UnequipChar(this, i, CharEquipFlag::skip_total);
#ifndef NO_EXTRANEOUS_TRIGGERS
			remove_otrigger(char_eq[i], this);
#endif
		} else
			char_eq[i] = nullptr;
	}
	// очистим аффекты, сохраним, потом восстановим
	auto tmp_aff = this->affected;
	affected.clear();

	if (!get_name().empty()) {
		fprintf(saved, "Name: %s\n", GET_NAME(this));
	}
	fprintf(saved, "Levl: %d\n", this->GetLevel());
	fprintf(saved, "Clas: %d\n", to_underlying(this->GetClass()));
	fprintf(saved, "LstL: %ld\n", static_cast<long int>(LAST_LOGON(this)));
	// сохраняем last_ip, который должен содержать айпишник с последнего удачного входа
	if (player_table[this->get_pfilepos()].last_ip) {
		strcpy(buf, player_table[this->get_pfilepos()].last_ip);
	} else {
		strcpy(buf, "Unknown");
	}
	fprintf(saved, "Host: %s\n", buf);
	free(player_table[this->get_pfilepos()].last_ip);
	player_table[this->get_pfilepos()].last_ip = str_dup(buf);
	fprintf(saved, "Id  : %ld\n", this->get_uid());
	fprintf(saved, "Exp : %ld\n", this->get_exp());
	fprintf(saved, "Rmrt: %d\n", this->get_remort());
	// флаги
	*buf = '\0';
	char_specials.saved.act.tascii(FlagData::kPlanesNumber, buf);
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
	if (!this->player_data.description.empty()) {
		strcpy(buf, this->player_data.description.c_str());
		kill_ems(buf);
		fprintf(saved, "Desc:\n%s~\n", buf);
	}
	if (POOFIN(this))
		fprintf(saved, "PfIn: %s\n", POOFIN(this));
	if (POOFOUT(this))
		fprintf(saved, "PfOt: %s\n", POOFOUT(this));
	fprintf(saved, "Sex : %d %s\n", static_cast<int>(this->get_sex()), genders[(int) this->get_sex()]);
	fprintf(saved, "Kin : %d %s\n", GET_KIN(this), PlayerRace::GetKinNameByNum(GET_KIN(this), this->get_sex()).c_str());
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
	AFF_FLAGS(this).tascii(FlagData::kPlanesNumber, buf);
	fprintf(saved, "Aff : %s\n", buf);

	// дальше не по порядку
	// статсы
	fprintf(saved, "Str : %d\n", this->GetInbornStr());
	fprintf(saved, "Int : %d\n", this->GetInbornInt());
	fprintf(saved, "Wis : %d\n", this->GetInbornWis());
	fprintf(saved, "Dex : %d\n", this->GetInbornDex());
	fprintf(saved, "Con : %d\n", this->GetInbornCon());
	fprintf(saved, "Cha : %d\n", this->GetInbornCha());

	if (GetRealLevel(this) < kLvlImmortal) {
		fprintf(saved, "Feat:\n");
		for (auto feat : MUD::Class(this->GetClass()).feats) {
			if (this->HaveFeat(feat.GetId())) {
				fprintf(saved, "%d %s\n", to_underlying(feat.GetId()), MUD::Feat(feat.GetId()).GetCName());
			}
		}
		fprintf(saved, "0 0\n");
	}

	if (GetRealLevel(this) < kLvlImmortal) {
		fprintf(saved, "FtTm:\n");
		for (auto tf = this->timed_feat; tf; tf = tf->next) {
			fprintf(saved, "%d %d %s\n", to_underlying(tf->feat), tf->time, MUD::Feat(tf->feat).GetCName());
		}
		fprintf(saved, "0 0\n");
	}

	// скилы
	if (GetRealLevel(this) < kLvlImmortal) {
		fprintf(saved, "Skil:\n");
		int skill_val;
		for (const auto &skill : MUD::Skills()) {
			if (skill.IsAvailable()) {
				skill_val = this->GetTrainedSkill(skill.GetId());
				if (skill_val) {
					fprintf(saved, "%d %d %s\n", to_underlying(skill.GetId()), skill_val, skill.GetName());
				}
			}
		}
		fprintf(saved, "0 0\n");
	}

	// города
	fprintf(saved, "Cits: %s\n", this->cities_to_str().c_str());

	// Задержки на скилы
	if (GetRealLevel(this) < kLvlImmortal) {
		fprintf(saved, "SkTm:\n");
		for (skj = this->timed; skj; skj = skj->next) {
			fprintf(saved, "%d %d\n", to_underlying(skj->skill), skj->time);
		}
		fprintf(saved, "0 0\n");
	}

	if (GetRealLevel(this) < kLvlImmortal && !IS_MANA_CASTER(this)) {
		fprintf(saved, "Spel:\n");
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (GET_SPELL_TYPE(this, spell_id)) {
				fprintf(saved, "%d %d %s\n", to_underlying(spell_id),
						GET_SPELL_TYPE(this, spell_id), MUD::Spell(spell_id).GetCName());
			}
		}
		fprintf(saved, "0 0\n");
	}

	if (GetRealLevel(this) < kLvlImmortal && !IS_MANA_CASTER(this)) {
		fprintf(saved, "TSpl:\n");
		for (auto & temp_spell : this->temp_spells) {
			fprintf(saved,
					"%d %ld %ld %s\n",
					to_underlying(temp_spell.first),
					static_cast<long int>(temp_spell.second.set_time),
					static_cast<long int>(temp_spell.second.duration),
					MUD::Spell(temp_spell.first).GetCName());
		}
		fprintf(saved, "0 0 0\n");
	}

	// Замемленые спелы
	if (GetRealLevel(this) < kLvlImmortal) {
		fprintf(saved, "SpMe:\n");
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (GET_SPELL_MEM(this, spell_id))
				fprintf(saved, "%d %d\n", to_underlying(spell_id), GET_SPELL_MEM(this, spell_id));
		}
		fprintf(saved, "0 0\n");
	}

	// Мемящиеся спелы
	if (GetRealLevel(this) < kLvlImmortal) {
		fprintf(saved, "SpTM:\n");
		for (struct SpellMemQueueItem *qi = this->mem_queue.queue; qi != nullptr; qi = qi->next)
			fprintf(saved, "%d\n", to_underlying(qi->spell_id));
		fprintf(saved, "0\n");
	}

	// Рецепты
//    if (GetRealLevel(this) < kLevelImmortal)
	{
		im_rskill *rs;
		im_recipe *r;
		fprintf(saved, "Rcps:\n");
		for (rs = GET_RSKILL(this); rs; rs = rs->link) {
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
	fprintf(saved, "Tglo: %ld\n", static_cast<long int>(this->getGloryRespecTime()));
	fprintf(saved, "Hit : %d/%d\n", this->get_hit(), this->get_max_hit());
	fprintf(saved, "Mana: %d/%d\n", this->mem_queue.stored, (this)->mem_queue.total);
	fprintf(saved, "Move: %d/%d\n", this->get_move(), this->get_max_move());
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

	for (unsigned i = 0; i < board_date_.size(); ++i) {
		fprintf(saved, "Br%02u: %llu\n", i + 1, static_cast<unsigned long long>(board_date_.at(i)));
	}

	for (int i = 0; i < START_STATS_TOTAL; ++i)
		fprintf(saved, "St%02d: %i\n", i, this->get_start_stat(i));

	if (GetRealLevel(this) < kLvlImmortal)
		fprintf(saved, "Hung: %d\n", GET_COND(this, FULL));
	if (GetRealLevel(this) < kLvlImmortal)
		fprintf(saved, "Thir: %d\n", GET_COND(this, THIRST));
	if (GetRealLevel(this) < kLvlImmortal)
		fprintf(saved, "Drnk: %d\n", GET_COND(this, DRUNK));

	fprintf(saved, "Reli: %d %s\n", GET_RELIGION(this), religion_name[GET_RELIGION(this)][(int) this->get_sex()]);
	fprintf(saved,
			"Race: %d %s\n",
			GET_RACE(this),
			PlayerRace::GetRaceNameByNum(GET_KIN(this), GET_RACE(this), this->get_sex()).c_str());
	fprintf(saved, "DrSt: %d\n", GET_DRUNK_STATE(this));
	fprintf(saved, "Olc : %d\n", GET_OLC_ZONE(this));
	*buf = '\0';
	this->player_specials->saved.pref.tascii(FlagData::kPlanesNumber, buf);
	fprintf(saved, "Pref: %s\n", buf);

	if (MUTE_DURATION(this) > 0 && this->IsFlagged(EPlrFlag::kMuted))
		fprintf(saved,
				"PMut: %ld %d %ld %s~\n",
				MUTE_DURATION(this),
				GET_MUTE_LEV(this),
				MUTE_GODID(this),
				MUTE_REASON(this));
	if (NAME_DURATION(this) > 0 && this->IsFlagged(EPlrFlag::kNameDenied))
		fprintf(saved,
				"PNam: %ld %d %ld %s~\n",
				NAME_DURATION(this),
				GET_NAME_LEV(this),
				NAME_GODID(this),
				NAME_REASON(this));
	if (DUMB_DURATION(this) > 0 && this->IsFlagged(EPlrFlag::kDumbed))
		fprintf(saved,
				"PDum: %ld %d %ld %s~\n",
				DUMB_DURATION(this),
				GET_DUMB_LEV(this),
				DUMB_GODID(this),
				DUMB_REASON(this));
	if (HELL_DURATION(this) > 0 && this->IsFlagged(EPlrFlag::kHelled))
		fprintf(saved,
				"PHel: %ld %d %ld %s~\n",
				HELL_DURATION(this),
				GET_HELL_LEV(this),
				HELL_GODID(this),
				HELL_REASON(this));
	if (GCURSE_DURATION(this) > 0)
		fprintf(saved,
				"PGcs: %ld %d %ld %s~\n",
				GCURSE_DURATION(this),
				GET_GCURSE_LEV(this),
				GCURSE_GODID(this),
				GCURSE_REASON(this));
	if (FREEZE_DURATION(this) > 0 && this->IsFlagged(EPlrFlag::kFrozen))
		fprintf(saved,
				"PFrz: %ld %d %ld %s~\n",
				FREEZE_DURATION(this),
				GET_FREEZE_LEV(this),
				FREEZE_GODID(this),
				FREEZE_REASON(this));
	if (UNREG_DURATION(this) > 0)
		fprintf(saved,
				"PUnr: %ld %d %ld %s~\n",
				UNREG_DURATION(this),
				GET_UNREG_LEV(this),
				UNREG_GODID(this),
				UNREG_REASON(this));

	if (KARMA(this)) {
		strcpy(buf, KARMA(this));
		kill_ems(buf);
		fprintf(saved, "Karm:\n%s~\n", buf);
	}
	if (!LOGON_LIST(this).empty()) {
		log("Saving logon list.");
		std::ostringstream buffer;
		for (const auto &logon : LOGON_LIST(this)) {
			buffer << logon.ip << " " << logon.count << " " << logon.lasttime << "\n";
		}
		fprintf(saved, "LogL:\n%s~\n", buffer.str().c_str());
	}
	fprintf(saved, "GdFl: %ld\n", this->player_specials->saved.GodsLike);
	fprintf(saved, "NamG: %d\n", NAME_GOD(this));
	fprintf(saved, "NaID: %ld\n", NAME_ID_GOD(this));
	fprintf(saved, "StrL: %d\n", STRING_LENGTH(this));
	fprintf(saved, "StrW: %d\n", STRING_WIDTH(this));
	fprintf(saved, "NtfE: %ld\n", NOTIFY_EXCH_PRICE(this));

	if (this->remember_get_num() != Remember::DEF_REMEMBER_NUM) {
		fprintf(saved, "Rmbr: %u\n", this->remember_get_num());
	}

	if (EXCHANGE_FILTER(this))
		fprintf(saved, "ExFl: %s\n", EXCHANGE_FILTER(this));

	for (const auto &cur : get_ignores()) {
		if (0 != cur->id) {
			fprintf(saved, "Ignr: [%lu]%ld\n", cur->mode, cur->id);
		}
	}

	// affected_type
	if (!tmp_aff.empty()) {
		fprintf(saved, "Affs:\n");
		for (auto &aff : tmp_aff) {
			if (aff->type >= ESpell::kFirst) {
				fprintf(saved, "%d %d %d %d %d %d %s\n", to_underlying(aff->type), aff->duration,
						aff->modifier, aff->location, static_cast<int>(aff->bitvector),
						static_cast<int>(aff->battleflag), MUD::Spell(aff->type).GetCName());
			}
		}
		fprintf(saved, "0 0 0 0 0 0\n");
	}

	// порталы
	std::ostringstream out;
	this->player_specials->runestones.Serialize(out);
	fprintf(saved, "%s", out.str().c_str());

	for (auto x : this->daily_quest) {
		std::stringstream buffer;
		const auto it = this->daily_quest_timed.find(x.first);
		if (it != this->daily_quest_timed.end())
			buffer << "DaiQ: " << x.first << " " << x.second << " " << it->second << "\n";
		else
			buffer << "DaiQ: " << x.first << " " << x.second << " 0\n";
		fprintf(saved, "%s", buffer.str().c_str());
	}

	for (i = 0; i < 1 + LAST_LOG; ++i) {
		if (!GET_LOGS(this)) {
			log("SYSERR: Saving NULL logs for char %s", GET_NAME(this));
			break;
		}
		fprintf(saved, "Logs: %d %d\n", i, GET_LOGS(this)[i]);
	}

	fprintf(saved, "Disp: %ld\n", disposable_flags_.to_ulong());

	fprintf(saved, "Ripa: %llu\n", GetStatistic(CharStat::ArenaRip));
	fprintf(saved, "Wina: %llu\n", GetStatistic(CharStat::ArenaWin));
	fprintf(saved, "Riar: %llu\n", GetStatistic(CharStat::ArenaRemortRip));
	fprintf(saved, "Wiar: %llu\n", GetStatistic(CharStat::ArenaRemortWin));
	fprintf(saved, "Riad: %llu\n", GetStatistic(CharStat::ArenaDomRip));
	fprintf(saved, "Wida: %llu\n", GetStatistic(CharStat::ArenaDomWin));
	fprintf(saved, "Ridr: %llu\n", GetStatistic(CharStat::ArenaDomRemortRip));
	fprintf(saved, "Widr: %llu\n", GetStatistic(CharStat::ArenaDomRemortWin));
	fprintf(saved, "Ripm: %llu\n", GetStatistic(CharStat::MobRip));
	fprintf(saved, "Ripd: %llu\n", GetStatistic(CharStat::DtRip));
	fprintf(saved, "Ripo: %llu\n", GetStatistic(CharStat::OtherRip));
	fprintf(saved, "Ripp: %llu\n", GetStatistic(CharStat::PkRip));
	fprintf(saved, "Rimt: %llu\n", GetStatistic(CharStat::MobRemortRip));
	fprintf(saved, "Ridt: %llu\n", GetStatistic(CharStat::DtRemortRip));
	fprintf(saved, "Riot: %llu\n", GetStatistic(CharStat::OtherRemortRip));
	fprintf(saved, "Ript: %llu\n", GetStatistic(CharStat::PkRemortRip));
	fprintf(saved, "Expa: %llu\n", GetStatistic(CharStat::ArenaExpLost));
	fprintf(saved, "Expm: %llu\n", GetStatistic(CharStat::MobExpLost));
	fprintf(saved, "Expd: %llu\n", GetStatistic(CharStat::DtExpLost));
	fprintf(saved, "Expo: %llu\n", GetStatistic(CharStat::OtherExpLost));
	fprintf(saved, "Expp: %llu\n", GetStatistic(CharStat::PkExpLost));
	fprintf(saved, "Exmt: %llu\n", GetStatistic(CharStat::MobRemortExpLost));
	fprintf(saved, "Exdt: %llu\n", GetStatistic(CharStat::DtRemortExpLost));
	fprintf(saved, "Exot: %llu\n", GetStatistic(CharStat::OtherRemortExpLost));
	fprintf(saved, "Expt: %llu\n", GetStatistic(CharStat::PkRemortExpLost));

	// не забываем рестить ману и при сейве
	this->set_who_mana(MIN(kWhoManaMax,
						   this->get_who_mana() + (time(0) - this->get_who_last()) * kWhoManaRestPerSecond));
	fprintf(saved, "Wman: %u\n", this->get_who_mana());

	// added by WorM (Видолюб) 2010.06.04 бабки потраченные на найм(возвращаются при креше)
	i = 0;
	if (this->followers
		&& CanUseFeat(this, EFeat::kEmployer)
		&& !IS_IMMORTAL(this)) {
		struct FollowerType *k = nullptr;
		for (k = this->followers; k; k = k->next) {
			if (k->follower
				&& AFF_FLAGGED(k->follower, EAffect::kHelper)
				&& AFF_FLAGGED(k->follower, EAffect::kCharmed)) {
				break;
			}
		}

		if (k
			&& k->follower
			&& !k->follower->affected.empty()) {
			for (const auto &aff : k->follower->affected) {
				if (aff->type == ESpell::kCharm) {
					if (k->follower->mob_specials.hire_price == 0) {
						break;
					}

					int i = ((aff->duration - 1) / 2) * k->follower->mob_specials.hire_price;
					if (i != 0) {
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
	fprintf(saved, "Map : %s\n", map_options_.bit_list_.to_string().c_str());
	fprintf(saved, "TrcG: %d\n", ext_money_[ExtMoney::kTorcGold]);
	fprintf(saved, "TrcS: %d\n", ext_money_[ExtMoney::kTorcSilver]);
	fprintf(saved, "TrcB: %d\n", ext_money_[ExtMoney::kTorcBronze]);
	fprintf(saved, "TrcL: %d %d\n", today_torc_.first, today_torc_.second);

	if (get_reset_stats_cnt(stats_reset::Type::MAIN_STATS) > 0) {
		fprintf(saved, "CntS: %d\n", get_reset_stats_cnt(stats_reset::Type::MAIN_STATS));
	}

	if (get_reset_stats_cnt(stats_reset::Type::RACE) > 0) {
		fprintf(saved, "CntR: %d\n", get_reset_stats_cnt(stats_reset::Type::RACE));
	}

	if (get_reset_stats_cnt(stats_reset::Type::FEATS) > 0) {
		fprintf(saved, "CntF: %d\n", get_reset_stats_cnt(stats_reset::Type::FEATS));
	}

	auto it = this->charmeeHistory.begin();
	if (this->charmeeHistory.size() > 0 &&
		(IS_SPELL_SET(this, ESpell::kCharm, ESpellType::kKnow) ||
		CanUseFeat(this, EFeat::kEmployer) || IS_IMMORTAL(this))) {
		fprintf(saved, "Chrm:\n");
		for (; it != this->charmeeHistory.end(); ++it) {
			fprintf(saved, "%d %d %d %d %d %d\n",
					it->first,
					it->second.CharmedCount,
					it->second.spentGold,
					it->second.deathCount,
					it->second.currRemortAvail,
					it->second.isFavorite);
		}
		fprintf(saved, "0 0 0 0 0 0\n");// терминирующая строчка
	}
	fprintf(saved, "Tlgr: %lu\n", this->player_specials->saved.telegram_id);
	fclose(saved);
	if (chmod(filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0) {
		std::stringstream ss;
		ss << "Error chmod file: " << filename << " (" << __FILE__ << " "<< __func__ << "  "<< __LINE__ << ")";
		mudlog(ss.str(), BRF, kLvlGod, SYSLOG, true);
	}
	FileCRC::check_crc(filename, FileCRC::UPDATE_PLAYER, this->get_uid());

	// восстанавливаем аффекты
	// add spell and eq affections back in now
	this->affected = tmp_aff;
	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (char_eq[i]) {
#ifndef NO_EXTRANEOUS_TRIGGERS
			if (wear_otrigger(char_eq[i], this, i))
#endif
			EquipObj(this, char_eq[i], i, CharEquipFlag::no_cast | CharEquipFlag::skip_total);
#ifndef NO_EXTRANEOUS_TRIGGERS
			else
				obj_to_char(char_eq[i], this);
#endif
		}
	}
	affect_total(this);

	i = GetPlayerTablePosByName(GET_NAME(this));
	if (i >= 0) {
		player_table[i].last_logon = LAST_LOGON(this);
		player_table[i].level = GetRealLevel(this);
		player_table[i].remorts = GetRealRemort(this);
		if (player_table[i].mail) {
			free(player_table[i].mail);
		}
		player_table[i].mail = str_dup(GET_EMAIL(this));
		player_table[i].set_uid(this->get_uid());
		player_table[i].plr_class = GetClass();
		//end by WorM
	}
}

#undef NO_EXTRANEOUS_TRIGGERS

// на счет reboot: используется только при старте мада в вызовах из ActualizePlayersIndex
// при включенном флаге файл читается только до поля Rebt, все остальные поля пропускаются
// поэтому при каких-то изменениях в ActualizePlayersIndex, MustBeDeleted и TopPlayer::Refresh следует
// убедиться, что изменный код работает с действительно проинициализированными полями персонажа
// на данный момент это: EPlrFlag::FLAGS, GetClass(), GET_EXP, get_uid, LAST_LOGON, GetRealLevel, GET_NAME, GetRealRemort, GET_UNIQUE, GET_EMAIL
// * \param reboot - по дефолту = false
int Player::load_char_ascii(const char *name, const int load_flags) {
	int id, num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0, i;
	long int lnum = 0, lnum3 = 0;
	unsigned long long llnum = 0;
	FBFILE *fl = nullptr;
	char filename[40];
	char line[kMaxStringLength], tag[6];
	char line1[kMaxStringLength];
	TimedSkill timed;
	TimedFeat timed_feat;
	*filename = '\0';
	log("Load ascii char %s", name);
	if (load_flags & ELoadCharFlags::kFindId) {
		id = find_name(name);
	} else {
		id = 1;
	}

	bool result = id >= 0;
	result = result && get_filename(name, filename, kPlayersFile);
	result = result && (fl = fbopen(filename, FB_READ));
	if (!result) {
		const std::size_t BUFFER_SIZE = 1024;
		char buffer[BUFFER_SIZE];
		log("Can't load ascii. Id: %d; File name: \"%s\"; Current directory: \"%s\")",
			id,
			filename,
			getcwd(buffer, BUFFER_SIZE));
		return -1;
	}

///////////////////////////////////////////////////////////////////////////////

	// первыми иним и парсим поля для ребута до поля "Rebt", если reboot на входе = 1, то на этом парс и кончается
	if (!this->player_specials) {
		this->player_specials = std::make_shared<player_special_data>();
	}

	set_level(1);
	set_class(ECharClass::kFirst);
	set_uid(0);
	set_last_logon(time(nullptr));
	set_exp(0);
	set_remort(0);
	GET_LASTIP(this)[0] = 0;
	GET_EMAIL(this)[0] = 0;
	char_specials.saved.act.from_string("");    // suspicious line: we should clear flags. Loading from "" does not clear flags.

	bool skip_file = 0;
//	log("plrname %s bool %d", get_name().c_str(), get_extracted_list());

	do {
		if (!fbgetline(fl, line)) {
			log("SYSERROR: Wrong file ascii %d %s", id, filename);
			return (-1);
		}

		ExtractTagFromArgument(line, tag);
		for (i = 0; !(line[i] == ' ' || line[i] == '\0'); i++) {
			line1[i] = line[i];
		}
		line1[i] = '\0';
		num = atoi(line1);
		lnum = atol(line1);
		errno = 0;
		char *end;
		llnum = std::strtoll(line1, &end, 10);
		if (errno != 0)
			llnum = 0;

		switch (*tag) {
			case 'A':
				if (!strcmp(tag, "Act ")) {
					char_specials.saved.act.from_string(line);
				}
				break;
			case 'C':
				if (!strcmp(tag, "Clas")) {
					set_class(static_cast<ECharClass>(num));
				}
				break;
			case 'E':
				if (!strcmp(tag, "Exp ")) {
					set_exp(lnum);
				}
				else if (!strcmp(tag, "EMal"))
					strcpy(GET_EMAIL(this), line);
				break;
			case 'H':
				if (!strcmp(tag, "Host")) {
					strcpy(GET_LASTIP(this), line);
				}
				break;
			case 'I':
				if (!strcmp(tag, "Id  ")) {
					set_uid(lnum);
				}
				break;
			case 'L':
				if (!strcmp(tag, "LstL")) {
					set_last_logon(lnum);
				} else if (!strcmp(tag, "Levl")) {
					set_level(num);
				}
				break;
			case 'N':
				if (!strcmp(tag, "Name")) {
					set_name(line);
				}
				break;
			case 'R':
				if (!strcmp(tag, "Rebt"))
					skip_file = 1;
				else if (!strcmp(tag, "Rmrt")) {
					set_remort(num);
				}
				break;
			default: sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
		}
	} while (!skip_file);

	bool reboot = (load_flags & ELoadCharFlags::kReboot);
	while ((reboot) && (!*GET_EMAIL(this) || !*GET_LASTIP(this))) {
		if (!fbgetline(fl, line)) {
			log("SYSERROR: Wrong file ascii %d %s", id, filename);
			return (-1);
		}

		ExtractTagFromArgument(line, tag);

		if (!strcmp(tag, "EMal"))
			strcpy(GET_EMAIL(this), line);
		else if (!strcmp(tag, "Host"))
			strcpy(GET_LASTIP(this), line);
	}

	// если с загруженными выше полями что-то хочется делать после лоада - делайте это здесь

	//Indexing experience - if his exp is lover than required for his level - set it to required
	if (this->get_exp() < GetExpUntilNextLvl(this, GetRealLevel(this))) {
		set_exp(GetExpUntilNextLvl(this, GetRealLevel(this)));
	}
	this->account = Account::get_account(GET_EMAIL(this));
	if (this->account == nullptr) {
//		log("Create account %s for player name %s", GET_EMAIL(this), name);
		const auto temp_account = std::make_shared<Account>(GET_EMAIL(this));
		accounts.emplace(GET_EMAIL(this), temp_account);
		this->account = temp_account;
	}
//	log("Add account %s player id %d  name %s", GET_EMAIL(this), this->get_uid(), name);
	this->account->add_player(this->get_uid());

	if (load_flags & ELoadCharFlags::kReboot) {
		fbclose(fl);
		return id;
	}
	// если происходит обычный лоад плеера, то читаем файл дальше и иним все остальные поля

///////////////////////////////////////////////////////////////////////////////


	// character init
	// initializations necessary to keep some things straight
	this->str_to_cities(cities::default_str_cities);
	this->set_npc_name(0);
	this->player_data.long_descr = "";

	this->real_abils.Feats.reset();

	if (IS_MANA_CASTER(this)) {
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_TYPE(this, spell_id) = ESpellType::kRunes;
		}
	} else {
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_TYPE(this, spell_id) = ESpellType::kUnknowm;
		}
	}

	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		GET_SPELL_MEM(this, spell_id) = 0;
	}
	this->char_specials.saved.affected_by = clear_flags;
	POOFIN(this) = nullptr;
	POOFOUT(this) = nullptr;
	GET_RSKILL(this) = nullptr;    // рецептов не знает
	this->char_specials.carry_weight = 0;
	this->char_specials.carry_items = 0;
	this->real_abils.armor = 100;
	this->mem_queue.total = 0;
	this->mem_queue.stored = 0;
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
	this->set_cha(10);

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
	this->set_hit(21);
	this->set_max_hit(21);
	GET_HEIGHT(this) = 50;
	GET_HR(this) = 0;
	GET_COND(this, FULL) = 0;
	SET_INVIS_LEV(this, 0);
	this->player_data.time.logon = time(0);
	this->set_move(44);
	this->set_max_move(44);
	KARMA(this) = 0;
	LOGON_LIST(this).clear();
	NAME_GOD(this) = 0;
	STRING_LENGTH(this) = 80;
	STRING_WIDTH(this) = 30;
	NAME_ID_GOD(this) = 0;
	GET_OLC_ZONE(this) = -1;
	this->player_data.time.played = 0;
	GET_LOADROOM(this) = kNowhere;
	GET_RELIGION(this) = 1;
	GET_RACE(this) = 1;
	this->set_sex(EGender::kNeutral);
	GET_COND(this, THIRST) = kNormCondition;
	GET_WEIGHT(this) = 50;
	GET_WIMP_LEV(this) = 0;
	this->player_specials->saved.pref.from_string("");    // suspicious line: we should clear flags.. Loading from "" does not clear flags.
	AFF_FLAGS(this).from_string("");    // suspicious line: we should clear flags.. Loading from "" does not clear flags.

	EXCHANGE_FILTER(this) = nullptr;
	clear_ignores();
	CREATE(GET_LOGS(this), 1 + LAST_LOG);
	NOTIFY_EXCH_PRICE(this) = 0;
	this->player_specials->saved.HiredCost = 0;
	this->set_who_mana(kWhoManaMax);
	this->set_who_last(time(0));

	while (fbgetline(fl, line)) {
		ExtractTagFromArgument(line, tag);
		for (i = 0; !(line[i] == ' ' || line[i] == '\0'); i++) {
			line1[i] = line[i];
		}
		line1[i] = '\0';
		num = atoi(line1);
		lnum = atol(line1);
		try {
			llnum = std::stoull(line1, nullptr, 10);
		} catch (const std::invalid_argument &) {
			llnum = 0;
		} catch (const std::out_of_range &) {
			llnum = 0;
		}
		switch (*tag) {
			case 'A':
				if (!strcmp(tag, "Ac  ")) {
					GET_AC(this) = num;
				} else if (!strcmp(tag, "Aff ")) {
					AFF_FLAGS(this).from_string(line);
				} else if (!strcmp(tag, "Affs")) {
					i = 0;
					do {
						fbgetline(fl, line);
						sscanf(line, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
						if (num > 0) {
							Affect<EApply> af;
							af.type = static_cast<ESpell>(num);;
							af.duration = num2;
							af.modifier = num3;
							af.location = static_cast<EApply>(num4);
							af.bitvector = num5;
							af.battleflag = num6;
							if (af.type == ESpell::kCombatLuck) {
								af.handler.reset(new CombatLuckAffectHandler());
							}
							affect_to_char(this, af);
							i++;
						}
					} while (num != 0);
					std::reverse(affected.begin(), affected.end());
					/* do not load affects */
				} else if (!strcmp(tag, "Alin")) {
					GET_ALIGNMENT(this) = num;
				}
				break;

			case 'B':
				if (!strcmp(tag, "Badp")) {
					GET_BAD_PWS(this) = num;
				} else if (!strcmp(tag, "Bank")) {
					set_bank(lnum, false);
				} else if (!strcmp(tag, "Br01"))
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
				else if (!strcmp(tag, "Chrm")) {
					log("Load_char: Charmees loading");
					do {
						fbgetline(fl, line);
						sscanf(line, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
						if (num / 100 < dungeons::kZoneStartDungeons && this->charmeeHistory.find(num) == this->charmeeHistory.end() && num != 0) {
							MERCDATA md = {num2, num3, num4, num5, num6}; // num - key
							this->charmeeHistory.insert(std::pair<int, MERCDATA>(num, md));
							log("Load_char: Charmees: vnum: %d", num);
						}
					} while (num != 0);
				} else if (!strcmp(tag, "Con "))
					this->set_con(num);
				else if (!strcmp(tag, "CntS"))
					this->reset_stats_cnt_[stats_reset::Type::MAIN_STATS] = num;
				else if (!strcmp(tag, "CntR"))
					this->reset_stats_cnt_[stats_reset::Type::RACE] = num;
				else if (!strcmp(tag, "CntF"))
					this->reset_stats_cnt_[stats_reset::Type::FEATS] = num;
				else if (!strcmp(tag, "Cits")) {
					std::string buffer_cities = std::string(line);
					auto cities_number = cities::CountCities();
					if (buffer_cities.size() != cities_number) {
						if (buffer_cities.size() < cities_number) {
							const size_t b_size = buffer_cities.size();
							for (unsigned int i = 0; i < cities_number - b_size; i++)
								buffer_cities += "0";
						} else {
							buffer_cities.resize(buffer_cities.size() - (buffer_cities.size() - cities_number));
						}
					}
					this->str_to_cities(std::string(buffer_cities));
				}
				break;

			case 'D':
				if (!strcmp(tag, "Desc")) {
					const auto ptr = fbgetstring(fl);
					this->player_data.description = ptr ? ptr : "";
				} else if (!strcmp(tag, "Disp")) {
					std::bitset<DIS_TOTAL_NUM> tmp_flags(lnum);
					disposable_flags_ = tmp_flags;
				} else if (!strcmp(tag, "Dex "))
					this->set_dex(num);
				else if (!strcmp(tag, "Drnk"))
					GET_COND(this, DRUNK) = num;
				else if (!strcmp(tag, "DrSt"))
					GET_DRUNK_STATE(this) = num;
				else if (!strcmp(tag, "Drol"))
					GET_DR(this) = num;
				else if (!strcmp(tag, "DaiQ")) {

					if (sscanf(line, "%d %d %ld", &num, &num2, &lnum) == 2) {
						this->add_daily_quest(num, num2);
					} else {
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
				else if (!strcmp(tag, "Expa"))
					IncreaseStatistic(CharStat::ArenaExpLost, llnum);
				else if (!strcmp(tag, "Expm"))
					IncreaseStatistic(CharStat::MobExpLost, llnum);
				else if (!strcmp(tag, "Exmt"))
					IncreaseStatistic(CharStat::MobRemortExpLost, llnum);
				else if (!strcmp(tag, "Expp"))
					IncreaseStatistic(CharStat::PkExpLost, llnum);
				else if (!strcmp(tag, "Expt"))
					IncreaseStatistic(CharStat::PkRemortExpLost, llnum);
				else if (!strcmp(tag, "Expo"))
					IncreaseStatistic(CharStat::OtherExpLost, llnum);
				else if (!strcmp(tag, "Exot"))
					IncreaseStatistic(CharStat::OtherRemortExpLost, llnum);
				else if (!strcmp(tag, "Expd"))
					IncreaseStatistic(CharStat::DtExpLost, llnum);
				else if (!strcmp(tag, "Exdt"))
					IncreaseStatistic(CharStat::DtRemortExpLost, llnum);
				break;

			case 'F':
				// Оставлено для совместимости со старым форматом наказаний
				if (!strcmp(tag, "Frez"))
					GET_FREEZE_LEV(this) = num;
				else if (!strcmp(tag, "Feat")) {
					do {
						fbgetline(fl, line);
						sscanf(line, "%d", &num);
						auto feat_id = static_cast<EFeat>(num);
						if (MUD::Feat(feat_id).IsAvailable()) {
							if (MUD::Class(this->GetClass()).feats.IsAvailable(feat_id) ||
								PlayerRace::FeatureCheck((int) GET_KIN(this), (int) GET_RACE(this), num)) {
								this->SetFeat(feat_id);
							}
						}
					} while (num != 0);
				} else if (!strcmp(tag, "FtTm")) {
					do {
						fbgetline(fl, line);
						sscanf(line, "%d %d", &num, &num2);
						if (num != 0) {
							timed_feat.feat = static_cast<EFeat>(num);
							timed_feat.time = num2;
							ImposeTimedFeat(this, &timed_feat);
						}
					} while (num != 0);
				}
				break;

			case 'G':
				if (!strcmp(tag, "Gold")) {
					set_gold(lnum, false);
				} else if (!strcmp(tag, "GodD"))
					GCURSE_DURATION(this) = lnum;
				else if (!strcmp(tag, "GdFl"))
					this->player_specials->saved.GodsLike = lnum;
					// added by WorM (Видолюб) 2010.06.04 бабки потраченные на найм(возвращаются при креше)
				else if (!strcmp(tag, "GldH")) {
					if (num != 0 && !IS_IMMORTAL(this) && CanUseFeat(this, EFeat::kEmployer)) {
						this->player_specials->saved.HiredCost = num;
					}
				}
				// end by WorM
				break;

			case 'H':
				if (!strcmp(tag, "Hit ")) {
					sscanf(line, "%d/%d", &num, &num2);
					this->set_hit(num);
					this->set_max_hit(num2);
				} else if (!strcmp(tag, "Hite"))
					GET_HEIGHT(this) = num;
				else if (!strcmp(tag, "Hrol"))
					GET_HR(this) = num;
				else if (!strcmp(tag, "Hung"))
					GET_COND(this, FULL) = num;
				else if (!strcmp(tag, "Hry ")) {
					if (num > cap_hryvn)
						num = cap_hryvn;
					this->set_hryvn(num);
				} else if (!strcmp(tag, "Host"))
					strcpy(GET_LASTIP(this), line);
				break;

			case 'I':
				if (!strcmp(tag, "Int "))
					this->set_int(num);
				else if (!strcmp(tag, "Invs")) {
					SET_INVIS_LEV(this, num);
				} else if (!strcmp(tag, "Ignr")) {
					IgnoresLoader ignores_loader(this);
					ignores_loader.load_from_string(line);
				} else if (!strcmp(tag, "ICur")) {
					this->set_ice_currency(num);
//				this->set_ice_currency(0); // чистка льда
				}
				break;

			case 'K':
				if (!strcmp(tag, "Kin "))
					GET_KIN(this) = num;
				else if (!strcmp(tag, "Karm"))
					KARMA(this) = fbgetstring(fl);
				break;
			case 'L':
				if (!strcmp(tag, "LogL")) {
					long lnum, lnum2;
					do {
						fbgetline(fl, line);
						sscanf(line, "%s %ld %ld", &buf[0], &lnum, &lnum2);
						if (buf[0] != '~') {
							const network::Logon cur_log = {str_dup(buf), lnum, lnum2, false};
							LOGON_LIST(this).push_back(cur_log);
						} else break;
					} while (true);

					if (!LOGON_LIST(this).empty()) {
						LOGON_LIST(this).at(0).is_first = true;
						std::sort(LOGON_LIST(this).begin(), LOGON_LIST(this).end(),
								  [](const network::Logon &a, const network::Logon &b) {
									  return a.lasttime < b.lasttime;
								  });
					}
				}
// Gunner
				else if (!strcmp(tag, "Logs")) {
					sscanf(line, "%d %d", &num, &num2);
					if (num >= 0 && num < 1 + LAST_LOG)
						GET_LOGS(this)[num] = num2;
				} else if (!strcmp(tag, "Lexc"))
					this->set_last_exchange(num);
				break;

			case 'M':
				if (!strcmp(tag, "Mana")) {
					sscanf(line, "%d/%d", &num, &num2);
					this->mem_queue.stored = num;
					this->mem_queue.total = num2;
				} else if (!strcmp(tag, "Map ")) {
					std::string str(line);
					std::bitset<MapSystem::TOTAL_MAP_OPTIONS> tmp(str);
					map_options_.bit_list_ = tmp;
				} else if (!strcmp(tag, "Move")) {
					sscanf(line, "%d/%d", &num, &num2);
					this->set_move(num);
					this->set_max_move(num2);
				} else if (!strcmp(tag, "Mobs")) {
					do {
						if (!fbgetline(fl, line))
							break;
						if (*line == '~')
							break;
						sscanf(line, "%d %d", &num, &num2);
						this->mobmax_load(this, num, num2, MobMax::get_level_by_vnum(num));
					} while (true);
				}
				break;
			case 'N':
				if (!strcmp(tag, "NmI "))
					this->player_data.PNames[ECase::kNom] = std::string(line);
				else if (!strcmp(tag, "NmR "))
					this->player_data.PNames[ECase::kGen] = std::string(line);
				else if (!strcmp(tag, "NmD "))
					this->player_data.PNames[ECase::kDat] = std::string(line);
				else if (!strcmp(tag, "NmV "))
					this->player_data.PNames[ECase::kAcc] = std::string(line);
				else if (!strcmp(tag, "NmT "))
					this->player_data.PNames[ECase::kIns] = std::string(line);
				else if (!strcmp(tag, "NmP "))
					this->player_data.PNames[ECase::kPre] = std::string(line);
				else if (!strcmp(tag, "NamD"))
					NAME_DURATION(this) = lnum;
				else if (!strcmp(tag, "NamG"))
					NAME_GOD(this) = num;
				else if (!strcmp(tag, "NaID"))
					NAME_ID_GOD(this) = lnum;
				else if (!strcmp(tag, "NtfE"))
					NOTIFY_EXCH_PRICE(this) = lnum;
				break;

			case 'O':
				if (!strcmp(tag, "Olc ") && (num > 0))
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
				else if (!strcmp(tag, "Pref")) {
					this->player_specials->saved.pref.from_string(line);
				} else if (!strcmp(tag, "Pkil")) {
					do {
						if (!fbgetline(fl, line))
							break;
						if (*line == '~')
							break;
						if (sscanf(line, "%ld %d %d", &lnum, &num, &num2) < 3) {
							num2 = 0;
						};
						if (lnum < 0 || !IsCorrectUnique(lnum))
							continue;
						if (num2 >= MAX_REVENGE) {
							if (--num <= 0) {
								continue;
							}
							num2 = 0;
						}
						struct PK_Memory_type *pk_one = nullptr;
						for (pk_one = this->pk_list; pk_one; pk_one = pk_one->next)
							if (pk_one->unique == lnum)
								break;
						if (pk_one) {
							log("SYSERROR: duplicate entry pkillers data for %d %s", id, filename);
							continue;
						}

						CREATE(pk_one, 1);
						pk_one->unique = lnum;
						pk_one->kill_num = num;
						pk_one->revenge_num = num2;
						pk_one->next = this->pk_list;
						this->pk_list = pk_one;
					} while (true);
				} else if (!strcmp(tag, "Prtl")) {
					if (num > 0) {
						auto portal = MUD::Runestones().FindRunestone(num);
						if (portal.IsAllowed()) {
							this->player_specials->runestones.AddRunestone(portal);
						}
					}
					// Loads Here new punishment strings
				} else if (!strcmp(tag, "PMut")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					MUTE_DURATION(this) = lnum;
					GET_MUTE_LEV(this) = num2;
					MUTE_GODID(this) = lnum3;
					MUTE_REASON(this) = str_dup(buf);
				} else if (!strcmp(tag, "PHel")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					HELL_DURATION(this) = lnum;
					GET_HELL_LEV(this) = num2;
					HELL_GODID(this) = lnum3;
					HELL_REASON(this) = str_dup(buf);
				} else if (!strcmp(tag, "PDum")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					DUMB_DURATION(this) = lnum;
					GET_DUMB_LEV(this) = num2;
					DUMB_GODID(this) = lnum3;
					DUMB_REASON(this) = str_dup(buf);
				} else if (!strcmp(tag, "PNam")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					NAME_DURATION(this) = lnum;
					GET_NAME_LEV(this) = num2;
					NAME_GODID(this) = lnum3;
					NAME_REASON(this) = str_dup(buf);
				} else if (!strcmp(tag, "PFrz")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					FREEZE_DURATION(this) = lnum;
					GET_FREEZE_LEV(this) = num2;
					FREEZE_GODID(this) = lnum3;
					FREEZE_REASON(this) = str_dup(buf);
				} else if (!strcmp(tag, "PGcs")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					GCURSE_DURATION(this) = lnum;
					GET_GCURSE_LEV(this) = num2;
					GCURSE_GODID(this) = lnum3;
					GCURSE_REASON(this) = str_dup(buf);
				} else if (!strcmp(tag, "PUnr")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					UNREG_DURATION(this) = lnum;
					GET_UNREG_LEV(this) = num2;
					UNREG_GODID(this) = lnum3;
					UNREG_REASON(this) = str_dup(buf);
				}

				break;

			case 'Q':
				if (!strcmp(tag, "Qst ")) {
					buf[0] = '\0';
					sscanf(line, "%d %[^~]", &num, &buf[0]);
					this->quested_add(this, num, buf);
				}
				break;

			case 'R':
				if (!strcmp(tag, "Room"))
					GET_LOADROOM(this) = num;
				else if (!strcmp(tag, "Ripa"))
					IncreaseStatistic(CharStat::ArenaRip, num);
				else if (!strcmp(tag, "Riar"))
					IncreaseStatistic(CharStat::ArenaRemortRip, num);
				else if (!strcmp(tag, "Riad"))
					IncreaseStatistic(CharStat::ArenaDomRip, num);
				else if (!strcmp(tag, "Ridr"))
					IncreaseStatistic(CharStat::ArenaDomRemortRip, num);
				else if (!strcmp(tag, "Ripm"))
					IncreaseStatistic(CharStat::MobRip, num);
				else if (!strcmp(tag, "Rimt"))
					IncreaseStatistic(CharStat::MobRemortRip, num);
				else if (!strcmp(tag, "Ruble"))
					this->set_ruble(num);
				else if (!strcmp(tag, "Ripp"))
					IncreaseStatistic(CharStat::PkRip, num);
				else if (!strcmp(tag, "Ript"))
					IncreaseStatistic(CharStat::PkRemortRip, num);
				else if (!strcmp(tag, "Ripo"))
					IncreaseStatistic(CharStat::OtherRip, num);
				else if (!strcmp(tag, "Riot"))
					IncreaseStatistic(CharStat::OtherRemortRip, num);
				else if (!strcmp(tag, "Ripd"))
					IncreaseStatistic(CharStat::DtRip, num);
				else if (!strcmp(tag, "Ridt"))
					IncreaseStatistic(CharStat::DtRemortRip, num);
				else if (!strcmp(tag, "Rmbr"))
					this->remember_set_num(num);
				else if (!strcmp(tag, "Reli"))
					GET_RELIGION(this) = num;
				else if (!strcmp(tag, "Race"))
					GET_RACE(this) = num;
				else if (!strcmp(tag, "Rcps")) {
					im_rskill *last = nullptr;
					for (;;) {
						im_rskill *rs;
						fbgetline(fl, line);
						sscanf(line, "%d %d", &num, &num2);
						if (num < 0)
							break;
						num = im_get_recipe(num);
// +newbook.patch (Alisher)
						if (num < 0 || imrecipes[num].classknow[(int) this->GetClass()] != kKnownRecipe)
// -newbook.patch (Alisher)
							continue;
						CREATE(rs, 1);
						rs->rid = num;
						rs->perc = num2;
						rs->link = nullptr;
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
				else if (!strcmp(tag, "Sex ")) {
					this->set_sex(static_cast<EGender>(num));
				} else if (!strcmp(tag, "Skil")) {
					do {
						fbgetline(fl, line);
						sscanf(line, "%d %d", &num, &num2);
						if (num != 0) {
							auto skill_id = static_cast<ESkill>(num);
							if (MUD::Class(this->GetClass()).skills[skill_id].IsAvailable()) {
								this->set_skill(skill_id, num2);
							}
						}
					} while (num != 0);
				} else if (!strcmp(tag, "SkTm")) {
					do {
						fbgetline(fl, line);
						sscanf(line, "%d %d", &num, &num2);
						if (num != 0) {
							timed.skill = static_cast<ESkill>(num);
							timed.time = num2;
							ImposeTimedSkill(this, &timed);
						}
					} while (num != 0);
				} else if (!strcmp(tag, "Spel")) {
					do {
						fbgetline(fl, line);
						sscanf(line, "%d %d", &num, &num2);
						auto spell_id = static_cast<ESpell>(num);
						if (spell_id > ESpell::kUndefined && MUD::Spell(spell_id).IsValid()) {
							GET_SPELL_TYPE(this, spell_id) = num2;
						}
					} while (num != 0);
				} else if (!strcmp(tag, "SpMe")) {
					do {
						fbgetline(fl, line);
						sscanf(line, "%d %d", &num, &num2);
						auto spell_id = static_cast<ESpell>(num);
						if (spell_id > ESpell::kUndefined) {
							GET_SPELL_MEM(this, spell_id) = num2;
						}
					} while (num != 0);
				} else if (!strcmp(tag, "SpTM")) {
					struct SpellMemQueueItem *qi_cur, **qi = &mem_queue.queue;
					while (*qi)
						qi = &((*qi)->next);
					do {
						fbgetline(fl, line);
						sscanf(line, "%d", &num);
						if (num != 0) {
							CREATE(qi_cur, 1);
							*qi = qi_cur;
							qi_cur->spell_id = static_cast<ESpell>(num);
							qi_cur->next = nullptr;
							qi = &qi_cur->next;
						}
					} while (num != 0);
				} else if (!strcmp(tag, "Str "))
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
					this->SetTitleStr(line);
				else if (!strcmp(tag, "TrcG"))
					set_ext_money(ExtMoney::kTorcGold, num, false);
				else if (!strcmp(tag, "TrcS"))
					set_ext_money(ExtMoney::kTorcSilver, num, false);
				else if (!strcmp(tag, "TrcB"))
					set_ext_money(ExtMoney::kTorcBronze, num, false);
				else if (!strcmp(tag, "TrcL")) {
					sscanf(line, "%d %d", &num, &num2);
					today_torc_.first = num;
					today_torc_.second = num2;
				} else if (!strcmp(tag, "Tglo")) {
					this->setGloryRespecTime(static_cast<time_t>(num));
				} else if (!strcmp(tag, "Tlgr")) {
					if (lnum <= 10000000000000) {
						this->player_specials->saved.telegram_id = lnum;
					} else  // зачищаем остатки старой баги
						this->player_specials->saved.telegram_id = 0;
				} else if (!strcmp(tag, "TSpl")) {
					do {
						fbgetline(fl, line);
						sscanf(line, "%d %ld %ld", &num, &lnum, &lnum3);
						auto spell_id = static_cast<ESpell>(num);
						if (num != 0 && MUD::Spell(spell_id).IsValid()) {
							temporary_spells::AddSpell(this, spell_id, lnum, lnum3);
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
				else if (!strcmp(tag, "Wina"))
					IncreaseStatistic(CharStat::ArenaWin, num);
				else if (!strcmp(tag, "Wiar"))
					IncreaseStatistic(CharStat::ArenaRemortWin, num);
				else if (!strcmp(tag, "Wida"))
					IncreaseStatistic(CharStat::ArenaDomWin, num);
				else if (!strcmp(tag, "Widr"))
					IncreaseStatistic(CharStat::ArenaDomRemortWin, num);
				else if (!strcmp(tag, "Wman"))
					this->set_who_mana(num);
				break;

			default: sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
		}
	}
	this->SetFlag(EPrf::kColor2); //всегда цвет полный
	// initialization for imms
	if (GetRealLevel(this) >= kLvlImmortal) {
		SetGodSkills(this);
		GET_COND(this, FULL) = -1;
		GET_COND(this, THIRST) = -1;
		GET_COND(this, DRUNK) = -1;
		GET_LOADROOM(this) = kNowhere;
	}

	SetInbornAndRaceFeats(this);

	if (IS_GRGOD(this)) {
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_TYPE(this, spell_id) = GET_SPELL_TYPE(this, spell_id) |
				ESpellType::kItemCast | ESpellType::kKnow | ESpellType::kRunes | ESpellType::kScrollCast
				| ESpellType::kPotionCast | ESpellType::kWandCast;
		}
	} else if (!IS_IMMORTAL(this)) {
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			const auto spell = MUD::Class(this->GetClass()).spells[spell_id];
			if (spell.GetCircle() == kMaxMemoryCircle) {
				REMOVE_BIT(GET_SPELL_TYPE(this, spell.GetId()), ESpellType::kKnow | ESpellType::kTemp);
			}
			if (GetRealRemort(this) < spell.GetMinRemort()) {
				GET_SPELL_MEM(this, spell_id) = 0;
			}
		}
	}

	/*
	 * If you're not poisioned and you've been away for more than an hour of
	 * real time, we'll set your HMV back to full
	 */
	if (!AFF_FLAGGED(this, EAffect::kPoisoned) && (((long) (time(0) - LAST_LOGON(this))) >= kSecsPerRealHour)) {
		this->set_hit(this->get_real_max_hit());
		this->set_move(this->get_real_max_move());
	} else
		this->set_hit(std::min(this->get_hit(), this->get_real_max_hit()));

	fbclose(fl);
	// здесь мы закладываемся на то, что при ребуте это все сейчас пропускается и это нормально,
	// иначе в таблице crc будут пустые имена, т.к. сама плеер-таблица еще не сформирована
	// и в любом случае при ребуте это все пересчитывать не нужно
	if (!(load_flags & ELoadCharFlags::kNoCrcCheck)) {
		FileCRC::check_crc(filename, FileCRC::PLAYER, this->get_uid());
	}

	return (id);
}

bool Player::get_disposable_flag(int num) {
	if (num < 0 || num >= DIS_TOTAL_NUM) {
		log("SYSERROR: num=%d (%s %s:%d)", num, __func__, __FILE__, __LINE__);
		return false;
	}
	return disposable_flags_[num];
}

void Player::set_disposable_flag(int num) {
	if (num < 0 || num >= DIS_TOTAL_NUM) {
		log("SYSERROR: num=%d (%s %s:%d)", num, __func__, __FILE__, __LINE__);
		return;
	}
	disposable_flags_.set(num);
}

bool Player::is_active() const {
	return motion_;
}

void Player::set_motion(bool flag) {
	motion_ = flag;
}

void Player::map_olc() {
	std::shared_ptr<MapSystem::Options> tmp(new MapSystem::Options);
	this->desc->map_options = tmp;

	*(this->desc->map_options) = map_options_;

	this->desc->state = EConState::kMapMenu;
	this->desc->map_options->olc_menu(this);
}

void Player::map_olc_save() {
	map_options_ = *(this->desc->map_options);
}

bool Player::map_check_option(int num) const {
	return map_options_.bit_list_.test(num);
}

void Player::map_set_option(unsigned num) {
	if (num < map_options_.bit_list_.size()) {
		map_options_.bit_list_.set(num);
	}
}

void Player::map_text_olc(const char *arg) {
	map_options_.text_olc(this, arg);
}

const MapSystem::Options *Player::get_map_options() const {
	return &map_options_;
}

void Player::map_print_to_snooper(CharData *imm) {
	MapSystem::Options tmp;
	tmp = map_options_;
	map_options_ = *(imm->get_map_options());
	// подменяем флаги карты на снуперские перед распечаткой ему карты
	MapSystem::print_map(this, imm);
	map_options_ = tmp;
}

int Player::get_ext_money(unsigned type) const {
	if (type < ext_money_.size()) {
		return ext_money_[type];
	}
	return 0;
}

void Player::set_ext_money(unsigned type, int num, bool write_log) {
	if (num < 0 || num > kMaxMoneyKept) {
		return;
	}
	if (type < ext_money_.size()) {
		const int diff = num - ext_money_[type];
		ext_money_[type] = num;
		if (diff != 0 && write_log) {
			ExtMoney::player_drop_log(this, type, diff);
		}
	}
}

int Player::get_today_torc() {
	uint8_t day = get_day_today();
	if (today_torc_.first != day) {
		today_torc_.first = day;
		today_torc_.second = 0;
	}

	return today_torc_.second;
}

void Player::add_today_torc(int num) {
	uint8_t day = get_day_today();
	if (today_torc_.first == day) {
		today_torc_.second += num;
	} else {
		today_torc_.first = day;
		today_torc_.second = num;
	}
}

int Player::get_reset_stats_cnt(stats_reset::Type type) const {
	return reset_stats_cnt_.at(type);
}

int Player::get_ice_currency() {
	return this->ice_currency;
}

void Player::set_ice_currency(int value) {
	this->ice_currency = value;
}

void Player::add_ice_currency(int value) {
	this->ice_currency += value;
}

void Player::sub_ice_currency(int value) {
	this->ice_currency = MAX(0, ice_currency - value);
}

bool Player::is_arena_player() {
	return this->arena_player;
}

int Player::get_count_daily_quest(int id) {
	if (this->daily_quest.count(id))
		return this->daily_quest[id];
	return 0;

}

time_t Player::get_time_daily_quest(int id) {
	if (this->daily_quest_timed.count(id))
		return this->daily_quest_timed[id];
	return 0;
}

void Player::add_value_cities(bool v) {
	this->cities_visited_.push_back(v);
}

void Player::reset_daily_quest() {
	this->daily_quest.clear();
	this->daily_quest_timed.clear();
	log("Персонаж: %s. Были сброшены гривны.", GET_NAME(this));
}

std::shared_ptr<Account> Player::get_account() {
	return this->account;
}

void Player::set_time_daily_quest(int id, time_t time) {
	if (this->daily_quest_timed.count(id)) {
		this->daily_quest_timed[id] = time;
	} else {
		this->daily_quest_timed.insert(std::pair<int, int>(id, time));
	}
}

void Player::add_daily_quest(int id, int count) {
	if (this->daily_quest.count(id)) {
		this->daily_quest[id] += count;
	} else {
		this->daily_quest.insert(std::pair<int, int>(id, count));
	}
	time_t now = time(nullptr);
	if (this->daily_quest_timed.count(id)) {
		this->daily_quest_timed[id] = now;
	} else {
		this->daily_quest_timed.insert(std::pair<int, time_t>(id, now));
	}
}

void Player::spent_hryvn_sub(int value) {
	this->spent_hryvn += value;
}

int Player::get_spent_hryvn() {
	return this->spent_hryvn;
}

int Player::death_player_count() {
	const int zone_vnum = zone_table[world[this->in_room]->zone_rn].vnum;
	auto it = this->count_death_zone.find(zone_vnum);
	if (it != this->count_death_zone.end()) {
		count_death_zone.at(zone_vnum) += 1;
	} else {
		count_death_zone.insert(std::pair<int, int>(zone_vnum, 1));
		return 1;
	}
	return (*it).second;
}

void Player::inc_reset_stats_cnt(stats_reset::Type type) {
	reset_stats_cnt_.at(type) += 1;
}

time_t Player::get_board_date(Boards::BoardTypes type) const {
	return board_date_.at(type);
}

void Player::set_board_date(Boards::BoardTypes type, time_t date) {
	board_date_.at(type) = date;
}

namespace PlayerSystem {

///
/// \return кол-во хп, втыкаемых чару от родного тела
///
int con_natural_hp(CharData *ch) {
	double add_hp_per_level = MUD::Class(ch->GetClass()).applies.base_con
		+ (ClampBaseStat(ch, EBaseStat::kCon, ch->get_con()) - MUD::Class(ch->GetClass()).applies.base_con)
			* MUD::Class(ch->GetClass()).applies.koef_con / 100.0 + 3;
	return 10 + static_cast<int>(add_hp_per_level * GetRealLevel(ch));
}

///
/// \return кол-во хп, втыкаемых чару от добавленного шмотом/аффектами тела
///
int con_add_hp(CharData *ch) {
	int con_add = std::max(0, GetRealCon(ch) - ch->get_con());
	return MUD::Class(ch->GetClass()).applies.koef_con * con_add * GetRealLevel(ch) / 100;
}

///
/// \return кол-во хп, втыкаемых чару от общего кол-ва тела
///
int con_total_hp(CharData *ch) {
	return con_natural_hp(ch) + con_add_hp(ch);
}

///
/// \return величина минуса к дексе в случае перегруза (case -> проценты от макс)
///
unsigned weight_dex_penalty(CharData *ch) {
	int n = 0;
	switch (ch->GetCarryingWeight() * 10 / std::max(1, CAN_CARRY_W(ch))) {
		case 10:
		case 9:
		case 8: n = 2;
			break;
		case 7:
		case 6:
		case 5: n = 1;
			break;
	}
	return n;
}

} // namespace PlayerSystem

// апдейт истории, при почарме освежает в памяти
void Player::updateCharmee(int vnum, int gold) {
	if (vnum < 1) {
		log("[ERROR] Player::updateCharmee. Вызов функции с vnum < 1, %s", this->get_name().c_str());
		return;
	}
	std::map<int, MERCDATA>::iterator it;
	MERCDATA md = {1, gold, 0, 1, 1};
	it = this->charmeeHistory.find(vnum);
	if (it == this->charmeeHistory.end()) {
		this->charmeeHistory.insert(std::pair<int, MERCDATA>(vnum, md));
	} else {
		++it->second.CharmedCount;
		it->second.spentGold += gold;
		it->second.currRemortAvail = 1;
	}
}

std::map<int, MERCDATA> *Player::getMercList() {
	return &this->charmeeHistory;
}

void Player::setTelegramId(unsigned long chat_id) {
	this->player_specials->saved.telegram_id = chat_id;
}

unsigned long int Player::getTelegramId() {
	return this->player_specials->saved.telegram_id;
}

void Player::setGloryRespecTime(time_t param) {
	this->player_specials->saved.lastGloryRespecTime = MAX(1, param);
}

time_t Player::getGloryRespecTime() {
	return this->player_specials->saved.lastGloryRespecTime;
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
