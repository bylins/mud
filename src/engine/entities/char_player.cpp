// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "char_player.h"
#include "gameplay/core/experience.h"
#include "administration/privilege.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/quests/daily_quest.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/condition.h"
#include "utils/grammar/declensions.h"

#include <sys/stat.h>

#include "utils/file_crc.h"
#include "utils/utils_time.h"
#include "utils/buffered_file_writer.h"
#include "utils/backtrace.h"

#include "gameplay/communication/ignores_loader.h"
#include "engine/olc/olc.h"
#include "utils/diskio.h"
#include "gameplay/core/genchar.h"
#include "engine/core/char_equip_flags.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/db/global_objects.h"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/magic/magic_temp_spells.h"
#include "administration/accounts.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/mechanics/cities.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/dungeons.h"
#include "engine/ui/cmd/do_who.h"
#include "engine/ui/mapsystem.h"
#include "engine/db/player_index.h"
#include "gameplay/core/remort.h"

#ifdef _WIN32
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif




Player::Player() :
	pfilepos_(-1),
	was_in_room_(kNowhere),
	from_room_(0),
	answer_id_(kNobody),
	motion_(true),
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



void Player::complete_quest(const int id) {
	this->account->complete_quest(id);
}

void Player::dquest(const int id) {
	DailyQuest::DoQuest(this, id);
}

void Player::mark_city(const std::string &id) {
	cities_visited_.insert(id);
}

bool Player::check_city(const std::string &id) {
	return cities_visited_.find(id) != cities_visited_.end();
}

// issue.cities: visited cities are stored as a comma-separated list of city text-ids.
void Player::str_to_cities(std::string str) {
	cities_visited_.clear();
	size_t start = 0;
	while (start <= str.size()) {
		const size_t comma = str.find(',', start);
		const std::string item = (comma == std::string::npos)
			? str.substr(start) : str.substr(start, comma - start);
		if (!item.empty()) {
			cities_visited_.insert(item);
		}
		if (comma == std::string::npos) {
			break;
		}
		start = comma + 1;
	}
}

std::string Player::cities_to_str() {
	std::string value;
	for (const auto &id : cities_visited_) {
		if (!value.empty()) {
			value += ",";
		}
		value += id;
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

void Player::quested_save(BufferedFileWriter &saved) const {
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

void Player::mobmax_save(BufferedFileWriter &saved) const {
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
	BufferedFileWriter saved;
	char filename[kMaxStringLength];
	int i;
	time_t li;
	ObjData *char_eq[EEquipPos::kNumEquipPos];

	int tmp = time(0) - this->player_data.time.logon;

	if (this->IsNpc() || this->get_pfilepos() < 0)
		return;

	// Профилировка save_char (#3440): общий таймер функции + фазы.
	utils::CExecutionTimer fn_timer;
	double d_account = 0.0, d_vars = 0.0, d_write = 0.0;

	if (this->account == nullptr) {
		this->account = Account::get_account(GET_EMAIL(this));
		if (this->account == nullptr) {
			const auto temp_account = std::make_shared<Account>(GET_EMAIL(this));
			accounts.emplace(GET_EMAIL(this), temp_account);
			this->account = temp_account;
		}
	}
	{ utils::CExecutionTimer t; this->account->save_to_file(); d_account = t.delta().count(); }
	log("Save char %s", GET_NAME(this));
	{ utils::CExecutionTimer t; save_char_vars(this); d_vars = t.delta().count(); }

	// Запись чара в новом формате
	get_filename(GET_NAME(this), filename, kPlayersFile);
	// Пишем в буфер в памяти (BufferedFileWriter), файл создаётся в конце.
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
		saved.printf("Name: %s\n", GET_NAME(this));
	}
	saved.printf("Levl: %d\n", this->GetLevel());
	saved.printf("Clas: %d\n", to_underlying(this->GetClass()));
	saved.printf("LstL: %ld\n", static_cast<long int>(this->get_last_logon()));
	// сохраняем last_ip, который должен содержать айпишник с последнего удачного входа
	if (player_table[this->get_pfilepos()].last_ip.empty()) {
		player_table[this->get_pfilepos()].last_ip = "Unknown";
	}
	saved.printf("Host: %s\n", player_table[this->get_pfilepos()].last_ip.c_str());
	saved.printf("Id  : %ld\n", this->get_uid());
	saved.printf("Exp : %ld\n", this->get_exp());
	saved.printf("Rmrt: %d\n", this->get_remort());
	// флаги
	*buf = '\0';
	char_specials.saved.act.tascii(FlagData::kPlanesNumber, buf, sizeof(buf));
	saved.printf("Act : %s\n", buf);
	if (GET_EMAIL(this))//edited WorM 2010.08.27 перенесено чтоб грузилось для сохранения в индексе игроков
	{
		saved.printf("EMal: %s\n", GET_EMAIL(this));
	}
	// это пишем обязательно посленим, потому что после него ничего не прочитается
	saved.printf("Rebt: следующие далее поля при перезагрузке не парсятся\n\n");
	// дальше пишем как хотим и что хотим

	saved.printf("NmI : %s\n", GET_PAD(this, 0));
	saved.printf("NmR : %s\n", GET_PAD(this, 1));
	saved.printf("NmD : %s\n", GET_PAD(this, 2));
	saved.printf("NmV : %s\n", GET_PAD(this, 3));
	saved.printf("NmT : %s\n", GET_PAD(this, 4));
	saved.printf("NmP : %s\n", GET_PAD(this, 5));
	if (!this->get_passwd().empty())
		saved.printf("Pass: %s\n", this->get_passwd().c_str());
	if (!this->player_data.title.empty())
		saved.printf("Titl: %s\n", this->player_data.title.c_str());
	if (!this->player_data.description.empty()) {
		snprintf(buf, sizeof(buf), "%s", this->player_data.description.c_str());
		kill_ems(buf);
		saved.printf("Desc:\n%s~\n", buf);
	}
	if (POOFIN(this))
		saved.printf("PfIn: %s\n", POOFIN(this));
	if (POOFOUT(this))
		saved.printf("PfOt: %s\n", POOFOUT(this));
	saved.printf("Sex : %d %s\n", static_cast<int>(this->get_sex()), genders[(int) this->get_sex()]);
	li = this->player_data.time.birth;
	saved.printf("Brth: %ld %s\n", static_cast<long int>(li), ctime(&li));
	// Gunner
	tmp += this->player_data.time.played;
	saved.printf("Plyd: %d\n", tmp);
	// Gunner end
	li = this->player_data.time.logon;
	saved.printf("Last: %ld %s\n", static_cast<long int>(li), ctime(&li));
	saved.printf("Hite: %d\n", GET_HEIGHT(this));
	saved.printf("Wate: %d\n", GET_WEIGHT(this));
	saved.printf("Size: %d\n", GET_SIZE(this));
	// структуры
	saved.printf("Alin: %d\n", alignment::GetAlignment(this));
	*buf = '\0';
	AFF_FLAGS(this).tascii(FlagData::kPlanesNumber, buf, sizeof(buf));
	saved.printf("Aff : %s\n", buf);

	// дальше не по порядку
	// статсы
	saved.printf("Str : %d\n", this->GetInbornStr());
	saved.printf("Int : %d\n", this->GetInbornInt());
	saved.printf("Wis : %d\n", this->GetInbornWis());
	saved.printf("Dex : %d\n", this->GetInbornDex());
	saved.printf("Con : %d\n", this->GetInbornCon());
	saved.printf("Cha : %d\n", this->GetInbornCha());

	if (GetRealLevel(this) < kLvlImmortal) {
		saved.printf("Feat:\n");
		for (auto feat : MUD::Class(this->GetClass()).feats) {
			if (this->HaveFeat(feat.GetId())) {
				saved.printf("%d %s\n", to_underlying(feat.GetId()), MUD::Feat(feat.GetId()).GetCName());
			}
		}
		saved.printf("0 0\n");
	}

	if (GetRealLevel(this) < kLvlImmortal) {
		saved.printf("FtTm:\n");
		for (auto tf : this->timed_feat) {
			saved.printf("%d %ld %s\n", to_underlying(tf.first), std::max(0L, static_cast<long>(tf.second - time(0))), MUD::Feat(tf.first).GetCName());
		}
		saved.printf("0 0\n");
	}

	// скилы
	if (GetRealLevel(this) < kLvlImmortal) {
		saved.printf("Skil:\n");
		int skill_val;
		for (const auto &skill : MUD::Skills()) {
			if (skill.IsAvailable()) {
				skill_val = GetTrainedSkill(this, skill.GetId());
				if (skill_val) {
					saved.printf("%d %d %s\n", to_underlying(skill.GetId()), skill_val, skill.GetName());
				}
			}
		}
		saved.printf("0 0\n");
	}

	// города
	saved.printf("Cits: %s\n", this->cities_to_str().c_str());

	// Задержки на скилы
	if (GetRealLevel(this) < kLvlImmortal) {
		saved.printf("SkTm:\n");
		for (auto skj : this->timed_skill) {
			saved.printf("%d %ld %s\n", to_underlying(skj.first), std::max(static_cast<long int>(0), static_cast<long int>(skj.second - time(0))), MUD::Skill(skj.first).GetName());
		}
		saved.printf("0 0\n");
	}

	if (GetRealLevel(this) < kLvlImmortal && !IS_MANA_CASTER(this)) {
		saved.printf("Spel:\n");
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (GET_SPELL_TYPE(this, spell_id)) {
				saved.printf("%d %d %s\n", to_underlying(spell_id),
						GET_SPELL_TYPE(this, spell_id), MUD::Spell(spell_id).GetCName());
			}
		}
		saved.printf("0 0\n");
	}

	if (GetRealLevel(this) < kLvlImmortal && !IS_MANA_CASTER(this)) {
		saved.printf("TSpl:\n");
		for (auto & temp_spell : this->temp_spells) {
			saved.printf(
					"%d %ld %ld %s\n",
					to_underlying(temp_spell.first),
					static_cast<long int>(temp_spell.second.set_time),
					static_cast<long int>(temp_spell.second.duration),
					MUD::Spell(temp_spell.first).GetCName());
		}
		saved.printf("0 0 0\n");
	}

	// Замемленые спелы
	if (GetRealLevel(this) < kLvlImmortal) {
		saved.printf("SpMe:\n");
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (GET_SPELL_MEM(this, spell_id))
				saved.printf("%d %d\n", to_underlying(spell_id), GET_SPELL_MEM(this, spell_id));
		}
		saved.printf("0 0\n");
	}

	// Мемящиеся спелы
	if (GetRealLevel(this) < kLvlImmortal) {
		saved.printf("SpTM:\n");
		for (struct SpellMemQueueItem *qi = this->mem_queue.queue; qi != nullptr; qi = qi->next)
			saved.printf("%d\n", to_underlying(qi->spell_id));
		saved.printf("0\n");
	}

	// Рецепты
//    if (GetRealLevel(this) < kLevelImmortal)
	{
		im_rskill *rs;
		im_recipe *r;
		saved.printf("Rcps:\n");
		for (rs = GET_RSKILL(this); rs; rs = rs->link) {
			if (rs->perc <= 0)
				continue;
			r = &imrecipes[rs->rid];
			saved.printf("%d %d %s\n", r->id, rs->perc, r->name);
		}
		saved.printf("-1 -1\n");
	}

	saved.printf("Hrol: %d\n", GET_HR(this));
	saved.printf("Drol: %d\n", GET_DR(this));
	saved.printf("Ac  : %d\n", GET_AC(this));
	saved.printf("Tglo: %ld\n", static_cast<long int>(this->getGloryRespecTime()));
	saved.printf("Hit : %d/%d\n", this->get_hit(), this->get_max_hit());
	saved.printf("Mana: %d/%d\n", this->mem_queue.stored, (this)->mem_queue.total);
	saved.printf("Move: %d/%d\n", this->get_move(), this->get_max_move());
	// All character currencies live in the unified container; persist each generically by text_id.
	for (const auto &[cur_id, cur_amounts] : this->currency_storage().data()) {
		if (cur_amounts.hand != 0 || cur_amounts.bank != 0) {
			saved.printf("Curr: %s %ld %ld\n", cur_id.c_str(), cur_amounts.hand, cur_amounts.bank);
		}
	}
	saved.printf("Wimp: %d\n", GET_WIMP_LEV(this));
	saved.printf("Frez: %d\n", punishments::Get(this, punishments::EType::kFreeze).level);
	saved.printf("Invs: %d\n", GET_INVIS_LEV(this));
	saved.printf("Room: %d\n", GET_LOADROOM(this));
//	li = this->player_data.time.birth;
//	saved.printf("Brth: %ld %s\n", static_cast<long int>(li), ctime(&li));
	saved.printf("Lexc: %ld\n", static_cast<long>(this->get_last_exchange()));
	saved.printf("Badp: %d\n", GET_BAD_PWS(this));

	for (unsigned i = 0; i < board_date_.size(); ++i) {
		saved.printf("Br%02u: %llu\n", i + 1, static_cast<unsigned long long>(board_date_.at(i)));
	}

	for (int i = 0; i < START_STATS_TOTAL; ++i)
		saved.printf("St%02d: %i\n", i, this->get_start_stat(i));

	if (GetRealLevel(this) < kLvlImmortal)
		saved.printf("Hung: %d\n", GET_COND(this, condition::kFull));
	if (GetRealLevel(this) < kLvlImmortal)
		saved.printf("Thir: %d\n", GET_COND(this, condition::kThirst));
	if (GetRealLevel(this) < kLvlImmortal)
		saved.printf("Drnk: %d\n", GET_COND(this, condition::kDrunk));

	saved.printf("Reli: %d %s\n", GET_RELIGION(this), religion_name[GET_RELIGION(this)][(int) this->get_sex()]);
	saved.printf(
			"Race: %d %s\n",
			GET_RACE(this),
			MUD::RaceMessages().GetMessage(GET_RACE(this), this->get_sex()).c_str());
	saved.printf("DrSt: %d\n", GET_DRUNK_STATE(this));
	saved.printf("Olc : %d\n", GET_OLC_ZONE(this));
	*buf = '\0';
	this->player_specials->saved.pref.tascii(FlagData::kPlanesNumber, buf, sizeof(buf));
	saved.printf("Pref: %s\n", buf);
	saved.printf("MgSh: %d\n", static_cast<int>(GetBriefShieldsMode()));

	if (punishments::Get(this, punishments::EType::kMute).duration > 0 && this->IsFlagged(EPlrFlag::kMuted))
		saved.printf(
				"PMut: %ld %d %ld %s~\n",
				punishments::Get(this, punishments::EType::kMute).duration,
				punishments::Get(this, punishments::EType::kMute).level,
				punishments::Get(this, punishments::EType::kMute).godid,
				punishments::Get(this, punishments::EType::kMute).reason.c_str());
	if (punishments::Get(this, punishments::EType::kName).duration > 0 && this->IsFlagged(EPlrFlag::kNameDenied))
		saved.printf(
				"PNam: %ld %d %ld %s~\n",
				punishments::Get(this, punishments::EType::kName).duration,
				punishments::Get(this, punishments::EType::kName).level,
				punishments::Get(this, punishments::EType::kName).godid,
				punishments::Get(this, punishments::EType::kName).reason.c_str());
	if (punishments::Get(this, punishments::EType::kDumb).duration > 0 && this->IsFlagged(EPlrFlag::kDumbed))
		saved.printf(
				"PDum: %ld %d %ld %s~\n",
				punishments::Get(this, punishments::EType::kDumb).duration,
				punishments::Get(this, punishments::EType::kDumb).level,
				punishments::Get(this, punishments::EType::kDumb).godid,
				punishments::Get(this, punishments::EType::kDumb).reason.c_str());
	if (punishments::Get(this, punishments::EType::kHell).duration > 0 && this->IsFlagged(EPlrFlag::kHelled))
		saved.printf(
				"PHel: %ld %d %ld %s~\n",
				punishments::Get(this, punishments::EType::kHell).duration,
				punishments::Get(this, punishments::EType::kHell).level,
				punishments::Get(this, punishments::EType::kHell).godid,
				punishments::Get(this, punishments::EType::kHell).reason.c_str());
	if (punishments::Get(this, punishments::EType::kGcurse).duration > 0)
		saved.printf(
				"PGcs: %ld %d %ld %s~\n",
				punishments::Get(this, punishments::EType::kGcurse).duration,
				punishments::Get(this, punishments::EType::kGcurse).level,
				punishments::Get(this, punishments::EType::kGcurse).godid,
				punishments::Get(this, punishments::EType::kGcurse).reason.c_str());
	if (punishments::Get(this, punishments::EType::kFreeze).duration > 0 && this->IsFlagged(EPlrFlag::kFrozen))
		saved.printf(
				"PFrz: %ld %d %ld %s~\n",
				punishments::Get(this, punishments::EType::kFreeze).duration,
				punishments::Get(this, punishments::EType::kFreeze).level,
				punishments::Get(this, punishments::EType::kFreeze).godid,
				punishments::Get(this, punishments::EType::kFreeze).reason.c_str());
	if (punishments::Get(this, punishments::EType::kUnreg).duration > 0)
		saved.printf(
				"PUnr: %ld %d %ld %s~\n",
				punishments::Get(this, punishments::EType::kUnreg).duration,
				punishments::Get(this, punishments::EType::kUnreg).level,
				punishments::Get(this, punishments::EType::kUnreg).godid,
				punishments::Get(this, punishments::EType::kUnreg).reason.c_str());

	if (KARMA(this)) {
		snprintf(buf, sizeof(buf), "%s", KARMA(this));
		kill_ems(buf);
		saved.printf("Karm:\n%s~\n", buf);
	}
	if (!LOGON_LIST(this).empty()) {
		log("Saving logon list.");
		std::ostringstream buffer;
		for (const auto &logon : LOGON_LIST(this)) {
			buffer << logon.ip << " " << logon.count << " " << logon.lasttime << "\n";
		}
		saved.printf("LogL:\n%s~\n", buffer.str().c_str());
	}
	saved.printf("GdFl: %ld\n", this->player_specials->saved.GodsLike);
	saved.printf("NamG: %d\n", (this)->player_specials->saved.NameGod);
	saved.printf("NaID: %ld\n", (this)->player_specials->saved.NameIDGod);
	saved.printf("StrL: %d\n", (this)->player_specials->saved.stringLength);
	saved.printf("StrW: %d\n", (this)->player_specials->saved.stringWidth);
	saved.printf("NtfE: %ld\n", (this)->player_specials->saved.ntfyExchangePrice);

	if (this->remember_get_num() != Remember::DEF_REMEMBER_NUM) {
		saved.printf("Rmbr: %u\n", this->remember_get_num());
	}

	if (EXCHANGE_FILTER(this))
		saved.printf("ExFl: %s\n", EXCHANGE_FILTER(this));

	for (const auto &cur : get_ignores()) {
		if (0 != cur->id) {
			saved.printf("Ignr: [%lu]%ld\n", cur->mode, cur->id);
		}
	}

	// affected_type
	if (!tmp_aff.empty()) {
		// issue.affect-migration: affects are persisted by their OWN identity (affect_type), not by
		// the casting spell. Fields: affect_type duration modifier location battleflag potency
		// stacks, then the affect's name as a readable comment (ignored on load). The Aff3 tag replaces
		// the spell-keyed Aff2 block; old Aff2 blocks are dropped on load -- active buffs recast in game.
		saved.printf("Aff3:\n");
		for (auto &aff : tmp_aff) {
			if (aff->affect_type != EAffect::kUndefined) {
				saved.printf("%d %d %d %d %d %f %d %s\n", static_cast<int>(aff->affect_type),
						aff->duration, aff->modifier, aff->location, static_cast<int>(aff->battleflag),
						aff->potency, aff->stacks,
						NAME_BY_ITEM<EAffect>(aff->affect_type).c_str());
			}
		}
		saved.printf("0 0 0 0 0\n");
	}

	// порталы
	std::ostringstream out;
	this->player_specials->runestones.Serialize(out);
	saved.printf("%s", out.str().c_str());

	for (auto x : this->daily_quest) {
		std::stringstream buffer;
		const auto it = this->daily_quest_timed.find(x.first);
		if (it != this->daily_quest_timed.end())
			buffer << "DaiQ: " << x.first << " " << x.second << " " << it->second << "\n";
		else
			buffer << "DaiQ: " << x.first << " " << x.second << " 0\n";
		saved.printf("%s", buffer.str().c_str());
	}

	for (i = 0; i < 1 + LAST_LOG; ++i) {
		if (!GET_LOGS(this)) {
			log("SYSERR: Saving NULL logs for char %s", GET_NAME(this));
			break;
		}
		saved.printf("Logs: %d %d\n", i, GET_LOGS(this)[i]);
	}

	saved.printf("Disp: %ld\n", disposable_flags_.to_ulong());

	saved.printf("Ripa: %llu\n", GetStatistic(CharStat::ArenaRip));
	saved.printf("Wina: %llu\n", GetStatistic(CharStat::ArenaWin));
	saved.printf("Riar: %llu\n", GetStatistic(CharStat::ArenaRemortRip));
	saved.printf("Wiar: %llu\n", GetStatistic(CharStat::ArenaRemortWin));
	saved.printf("Riad: %llu\n", GetStatistic(CharStat::ArenaDomRip));
	saved.printf("Wida: %llu\n", GetStatistic(CharStat::ArenaDomWin));
	saved.printf("Ridr: %llu\n", GetStatistic(CharStat::ArenaDomRemortRip));
	saved.printf("Widr: %llu\n", GetStatistic(CharStat::ArenaDomRemortWin));
	saved.printf("Ripm: %llu\n", GetStatistic(CharStat::MobRip));
	saved.printf("Ripd: %llu\n", GetStatistic(CharStat::DtRip));
	saved.printf("Ripo: %llu\n", GetStatistic(CharStat::OtherRip));
	saved.printf("Ripp: %llu\n", GetStatistic(CharStat::PkRip));
	saved.printf("Rimt: %llu\n", GetStatistic(CharStat::MobRemortRip));
	saved.printf("Ridt: %llu\n", GetStatistic(CharStat::DtRemortRip));
	saved.printf("Riot: %llu\n", GetStatistic(CharStat::OtherRemortRip));
	saved.printf("Ript: %llu\n", GetStatistic(CharStat::PkRemortRip));
	saved.printf("Expa: %llu\n", GetStatistic(CharStat::ArenaExpLost));
	saved.printf("Expm: %llu\n", GetStatistic(CharStat::MobExpLost));
	saved.printf("Expd: %llu\n", GetStatistic(CharStat::DtExpLost));
	saved.printf("Expo: %llu\n", GetStatistic(CharStat::OtherExpLost));
	saved.printf("Expp: %llu\n", GetStatistic(CharStat::PkExpLost));
	saved.printf("Exmt: %llu\n", GetStatistic(CharStat::MobRemortExpLost));
	saved.printf("Exdt: %llu\n", GetStatistic(CharStat::DtRemortExpLost));
	saved.printf("Exot: %llu\n", GetStatistic(CharStat::OtherRemortExpLost));
	saved.printf("Expt: %llu\n", GetStatistic(CharStat::PkRemortExpLost));

	// не забываем рестить ману и при сейве
	this->set_who_mana(MIN(kWhoManaMax,
						   this->get_who_mana() + (time(0) - this->get_who_last()) * kWhoManaRestPerSecond));
	saved.printf("Wman: %u\n", this->get_who_mana());

	// added by WorM (Видолюб) 2010.06.04 бабки потраченные на найм(возвращаются при креше)
	i = 0;
	if (!this->followers.empty()
		&& CanUseFeat(this, EFeat::kEmployer)
		&& !privilege::IsImmortal(this)) {
		CharData *found_follower = nullptr;
		for (auto *k : this->followers) {
			if (k
				&& AFF_FLAGGED(k, EAffect::kHelper)
				&& AFF_FLAGGED(k, EAffect::kCharmed)) {
				found_follower = k;
				break;
			}
		}

		if (found_follower
			&& !found_follower->affected.empty()) {
			for (const auto &aff : found_follower->affected) {
				if (IS_SET(aff->battleflag, kAfCharmBond)) {
					if (found_follower->mob_specials.hire_price == 0) {
						break;
					}

					int i = ((aff->duration - 1) / 2) * found_follower->mob_specials.hire_price;
					if (i != 0) {
						saved.printf("GldH: %d\n", i);
					}
					break;
				}
			}
		}
	}

	this->quested_save(saved);
	this->mobmax_save(saved);
	save_pkills(this, saved);
	saved.printf("Map : %s\n", map_options_.bit_list_.to_string().c_str());

	if (get_reset_stats_cnt(stats_reset::Type::MAIN_STATS) > 0) {
		saved.printf("CntS: %d\n", get_reset_stats_cnt(stats_reset::Type::MAIN_STATS));
	}

	if (get_reset_stats_cnt(stats_reset::Type::RACE) > 0) {
		saved.printf("CntR: %d\n", get_reset_stats_cnt(stats_reset::Type::RACE));
	}

	if (get_reset_stats_cnt(stats_reset::Type::FEATS) > 0) {
		saved.printf("CntF: %d\n", get_reset_stats_cnt(stats_reset::Type::FEATS));
	}

	auto it = this->charmeeHistory.begin();
	if (this->charmeeHistory.size() > 0 &&
		(IS_SPELL_SET(this, ESpell::kCharm, ESpellType::kKnow) ||
		CanUseFeat(this, EFeat::kEmployer) || privilege::IsImmortal(this))) {
		saved.printf("Chrm:\n");
		for (; it != this->charmeeHistory.end(); ++it) {
			saved.printf("%d %d %d %d %d %d\n",
					it->first,
					it->second.CharmedCount,
					it->second.spentGold,
					it->second.deathCount,
					it->second.currRemortAvail,
					it->second.isFavorite);
		}
		saved.printf("0 0 0 0 0 0\n");// терминирующая строчка
	}
	saved.printf("Tlgr: %lu\n", this->player_specials->saved.telegram_id);

	// Накопленный буфер пишем на диск в бинарном режиме (байты файла == байты
	// буфера на всех платформах) и из него же считаем CRC -- без перечитывания
	// только что записанного файла.
	const std::string &pfile = saved.str();
	utils::CExecutionTimer wt;
	FILE *pf = fopen(filename, "wb");
	if (!pf) {
		perror("Unable open charfile");
		return;
	}
	fwrite(pfile.data(), 1, pfile.size(), pf);
	fclose(pf);
#ifndef _WIN32
	if (chmod(filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0) {
		std::stringstream ss;
		ss << "Error chmod file: " << filename << " (" << __FILE__ << " "<< __func__ << "  "<< __LINE__ << ")";
		mudlog(ss.str(), BRF, kLvlGod, SYSLOG, true);
	}
#endif
	d_write = wt.delta().count();
	FileCRC::update_from_content(this->get_uid(), FileCRC::kPlayer, pfile.data(), pfile.size());

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
		player_table[i].last_logon = this->get_last_logon();
		player_table[i].level = GetRealLevel(this);
		player_table[i].remorts = remort::GetRealRemort(this);
		player_table[i].mail = GET_EMAIL(this);
		player_table[i].set_uid(this->get_uid());
		player_table[i].plr_class = GetClass();
		//end by WorM
	}

	const double fn_sec = fn_timer.delta().count();
	const double other_sec = fn_sec - d_account - d_vars - d_write;
	if (fn_sec > 0.02) {
		log("save_char prof: %s account=%.4f vars=%.4f write=%.4f other=%.4f total=%.4f",
			GET_NAME(this), d_account, d_vars, d_write, other_sec, fn_sec);
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
	this->player_specials->saved.LastIP[0] = 0;
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
					strcpy(this->player_specials->saved.LastIP, line);
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
	while ((reboot) && (!*GET_EMAIL(this) || !*this->player_specials->saved.LastIP)) {
		if (!fbgetline(fl, line)) {
			log("SYSERROR: Wrong file ascii %d %s", id, filename);
			return (-1);
		}

		ExtractTagFromArgument(line, tag);

		if (!strcmp(tag, "EMal"))
			strcpy(GET_EMAIL(this), line);
		else if (!strcmp(tag, "Host"))
			strcpy(this->player_specials->saved.LastIP, line);
	}

	// если с загруженными выше полями что-то хочется делать после лоада - делайте это здесь

	//Indexing experience - if his exp is lover than required for his level - set it to required
	if (this->get_exp() < experience::GetExpUntilNextLvl(this, GetRealLevel(this))) {
		set_exp(experience::GetExpUntilNextLvl(this, GetRealLevel(this)));
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
	this->cities_visited_.clear();
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
	this->char_specials.saved.affected_by.clear();
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
	alignment::SetAlignment(this, 0);
	GET_BAD_PWS(this) = 0;
	this->player_data.time.birth = time(0);

	this->set_str(10);
	this->set_dex(10);
	this->set_con(10);
	this->set_int(10);
	this->set_wis(10);
	this->set_cha(10);

	GET_COND(this, condition::kDrunk) = 0;
	GET_DRUNK_STATE(this) = 0;

// Punish Init
	punishments::Get(this, punishments::EType::kDumb).duration = 0;
	punishments::Get(this, punishments::EType::kDumb).reason.clear();
	punishments::Get(this, punishments::EType::kDumb).level = 0;
	punishments::Get(this, punishments::EType::kDumb).godid = 0;

	punishments::Get(this, punishments::EType::kMute).duration = 0;
	punishments::Get(this, punishments::EType::kMute).reason.clear();
	punishments::Get(this, punishments::EType::kMute).level = 0;
	punishments::Get(this, punishments::EType::kMute).godid = 0;

	punishments::Get(this, punishments::EType::kHell).duration = 0;
	punishments::Get(this, punishments::EType::kHell).reason.clear();
	punishments::Get(this, punishments::EType::kHell).level = 0;
	punishments::Get(this, punishments::EType::kHell).godid = 0;

	punishments::Get(this, punishments::EType::kFreeze).duration = 0;
	punishments::Get(this, punishments::EType::kFreeze).reason.clear();
	punishments::Get(this, punishments::EType::kFreeze).level = 0;
	punishments::Get(this, punishments::EType::kFreeze).godid = 0;

	punishments::Get(this, punishments::EType::kGcurse).duration = 0;
	punishments::Get(this, punishments::EType::kGcurse).reason.clear();
	punishments::Get(this, punishments::EType::kGcurse).level = 0;
	punishments::Get(this, punishments::EType::kGcurse).godid = 0;

	punishments::Get(this, punishments::EType::kName).duration = 0;
	punishments::Get(this, punishments::EType::kName).reason.clear();
	punishments::Get(this, punishments::EType::kName).level = 0;
	punishments::Get(this, punishments::EType::kName).godid = 0;

	punishments::Get(this, punishments::EType::kUnreg).duration = 0;
	punishments::Get(this, punishments::EType::kUnreg).reason.clear();
	punishments::Get(this, punishments::EType::kUnreg).level = 0;
	punishments::Get(this, punishments::EType::kUnreg).godid = 0;

// End punish init

	GET_DR(this) = 0;

	currencies::SetHand(*this, currencies::kGold, 0, false);
	currencies::SetBank(*this, currencies::kGold, 0, false);
	this->player_specials->saved.GodsLike = 0;
	this->set_hit(21);
	this->set_max_hit(21);
	GET_HEIGHT(this) = 50;
	GET_HR(this) = 0;
	GET_COND(this, condition::kFull) = 0;
	SET_INVIS_LEV(this, 0);
	this->player_data.time.logon = time(0);
	this->set_move(44);
	this->set_max_move(44);
	KARMA(this) = 0;
	LOGON_LIST(this).clear();
	(this)->player_specials->saved.NameGod = 0;
	(this)->player_specials->saved.stringLength = 80;
	(this)->player_specials->saved.stringWidth = 30;
	(this)->player_specials->saved.NameIDGod = 0;
	GET_OLC_ZONE(this) = -1;
	this->player_data.time.played = 0;
	GET_LOADROOM(this) = kNowhere;
	GET_RELIGION(this) = 1;
	GET_RACE(this) = 1;
	this->set_sex(EGender::kNeutral);
	GET_COND(this, condition::kThirst) = kNormCondition;
	GET_WEIGHT(this) = 50;
	GET_WIMP_LEV(this) = 0;
	this->player_specials->saved.pref.from_string("");    // suspicious line: we should clear flags.. Loading from "" does not clear flags.
	AFF_FLAGS(this).from_string("");    // suspicious line: we should clear flags.. Loading from "" does not clear flags.

	EXCHANGE_FILTER(this) = nullptr;
	clear_ignores();
	CREATE(GET_LOGS(this), 1 + LAST_LOG);
	(this)->player_specials->saved.ntfyExchangePrice = 0;
	this->player_specials->saved.HiredCost = 0;
	this->player_specials->saved.brief_shields_mode = EBriefShieldsMode::kBrief;
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
				} else if (!strcmp(tag, "Aff2")) {
					// issue.affect-migration: legacy spell-keyed temp-effect block. The format is now
					// affect-identity-keyed (Aff3); drop the old block -- active buffs recast in game
					// (kvirund: no back-compat for the temp-effects format).
					do {
						fbgetline(fl, line);
						num = atoi(line);
					} while (num != 0);
				} else if (!strcmp(tag, "Aff3")) {
					i = 0;
					do {
						fbgetline(fl, line);
						// Fields: affect_type duration modifier location battleflag potency stacks.
						// The trailing affect name is a readable comment, ignored here. af.type is gone --
						// the affect is identified by affect_type; the funnel re-sources battleflag from it
						// (preserving the per-instance kAfFailed / kAfCharmBond bits from the saved value).
						float af_potency = 0.0f;
						int af_stacks = 1;
						const int parsed = sscanf(line, "%d %d %d %d %d %f %d",
								&num, &num2, &num3, &num4, &num6, &af_potency, &af_stacks);
						if (num > 0) {
							Affect<EApply> af;
							af.affect_type = static_cast<EAffect>(num);
							af.duration = num2;
							af.modifier = num3;
							af.location = static_cast<EApply>(num4);
							af.battleflag = num6;
							if (parsed >= 6) {
								af.potency = af_potency;
							}
							if (parsed >= 7) {
								af.stacks = std::max(1, af_stacks);
							}
							affect_to_char(this, af);
							i++;
						}
					} while (num != 0);
					std::reverse(affected.begin(), affected.end());
				} else if (!strcmp(tag, "Affs")) {
					// Legacy pre-renumber affect block: affect_type was stored as a packed value that no
					// longer maps to the renumbered EAffect. Drop these (active spell buffs); recast in game.
					do {
						fbgetline(fl, line);
						num = atoi(line);
					} while (num != 0);
				} else if (!strcmp(tag, "Alin")) {
					alignment::SetAlignment(this, num);
				}
				break;

			case 'B':
				if (!strcmp(tag, "Badp")) {
					GET_BAD_PWS(this) = num;
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
				if (!strcmp(tag, "Curr")) {
					char cur_id[128] = {0};
					long cur_hand = 0, cur_bank = 0;
					if (sscanf(line, "%127s %ld %ld", cur_id, &cur_hand, &cur_bank) >= 1) {
						currencies::SetHand(*this, cur_id, cur_hand, false);
						currencies::SetBank(*this, cur_id, cur_bank, false);
					}
				} else if (!strcmp(tag, "Cha "))
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
					this->str_to_cities(std::string(line));
				}
				break;

			case 'D':
				if (!strcmp(tag, "Desc")) {
					const auto ptr = fbgetstring(fl);
					this->player_data.description = ptr ? ptr : "";
					free(ptr);    // issue #3574: fbgetstring malloc'ит; текст уже скопирован в std::string
				} else if (!strcmp(tag, "Disp")) {
					std::bitset<DIS_TOTAL_NUM> tmp_flags(lnum);
					disposable_flags_ = tmp_flags;
				} else if (!strcmp(tag, "Dex "))
					this->set_dex(num);
				else if (!strcmp(tag, "Drnk"))
					GET_COND(this, condition::kDrunk) = num;
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
					punishments::Get(this, punishments::EType::kFreeze).level = num;
				else if (!strcmp(tag, "Feat")) {
					do {
						fbgetline(fl, line);
						sscanf(line, "%d", &num);
						auto feat_id = static_cast<EFeat>(num);
						if (MUD::Feat(feat_id).IsAvailable()) {
							if (MUD::Class(this->GetClass()).feats.IsAvailable(feat_id) ||
								MUD::PcRaces()[GET_RACE(this)].HasFeature(static_cast<EFeat>(num))) {
								this->SetFeat(feat_id);
							}
						}
					} while (num != 0);
				} else if (!strcmp(tag, "FtTm")) {
					do {
						long int num3;
						fbgetline(fl, line);
						sscanf(line, "%d %ld", &num, &num3);
						if (num != 0) {
							this->timed_feat[static_cast<EFeat>(num)] = time(0) + num3;
						}
					} while (num != 0);
				}
				break;

			case 'G':
				if (!strcmp(tag, "GodD"))
					punishments::Get(this, punishments::EType::kGcurse).duration = lnum;
				else if (!strcmp(tag, "GdFl"))
					this->player_specials->saved.GodsLike = lnum;
					// added by WorM (Видолюб) 2010.06.04 бабки потраченные на найм(возвращаются при креше)
				else if (!strcmp(tag, "GldH")) {
					if (num != 0 && !privilege::IsImmortal(this) && CanUseFeat(this, EFeat::kEmployer)) {
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
					GET_COND(this, condition::kFull) = num;
				else if (!strcmp(tag, "Host"))
					strcpy(this->player_specials->saved.LastIP, line);
				break;

			case 'I':
				if (!strcmp(tag, "Int "))
					this->set_int(num);
				else if (!strcmp(tag, "Invs")) {
					SET_INVIS_LEV(this, num);
				} else if (!strcmp(tag, "Ignr")) {
					IgnoresLoader ignores_loader(this);
					ignores_loader.load_from_string(line);
				}
				break;

			case 'K':
				if (!strcmp(tag, "Karm"))
					KARMA(this) = fbgetstring(fl);
				break;
			case 'L':
				if (!strcmp(tag, "LogL")) {
					long lnum, lnum2;
					do {
						fbgetline(fl, line);
						sscanf(line, "%s %ld %ld", &buf[0], &lnum, &lnum2);
						if (buf[0] != '~') {
							const network::Logon cur_log = {buf, lnum, lnum2, false};
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
				} else if (!strcmp(tag, "MgSh")) {
					if (num >= static_cast<int>(EBriefShieldsMode::kOff)
						&& num <= static_cast<int>(EBriefShieldsMode::kCompressed)) {
						this->player_specials->saved.brief_shields_mode = static_cast<EBriefShieldsMode>(num);
					} else {
						this->player_specials->saved.brief_shields_mode = EBriefShieldsMode::kBrief;
					}
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
					this->player_data.PNames[grammar::ECase::kNom] = std::string(line);
				else if (!strcmp(tag, "NmR "))
					this->player_data.PNames[grammar::ECase::kGen] = std::string(line);
				else if (!strcmp(tag, "NmD "))
					this->player_data.PNames[grammar::ECase::kDat] = std::string(line);
				else if (!strcmp(tag, "NmV "))
					this->player_data.PNames[grammar::ECase::kAcc] = std::string(line);
				else if (!strcmp(tag, "NmT "))
					this->player_data.PNames[grammar::ECase::kIns] = std::string(line);
				else if (!strcmp(tag, "NmP "))
					this->player_data.PNames[grammar::ECase::kPre] = std::string(line);
				else if (!strcmp(tag, "NamD"))
					punishments::Get(this, punishments::EType::kName).duration = lnum;
				else if (!strcmp(tag, "NamG"))
					(this)->player_specials->saved.NameGod = num;
				else if (!strcmp(tag, "NaID"))
					(this)->player_specials->saved.NameIDGod = lnum;
				else if (!strcmp(tag, "NtfE"))
					(this)->player_specials->saved.ntfyExchangePrice = lnum;
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
						if (this->pk_map.count(lnum)) {
							log("SYSERROR: duplicate entry pkillers data for %d %s", id, filename);
							continue;
						}
						auto &pk_one = this->pk_map[lnum];
						pk_one.unique = lnum;
						pk_one.kill_num = num;
						pk_one.revenge_num = num2;
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
					punishments::Get(this, punishments::EType::kMute).duration = lnum;
					punishments::Get(this, punishments::EType::kMute).level = num2;
					punishments::Get(this, punishments::EType::kMute).godid = lnum3;
					punishments::Get(this, punishments::EType::kMute).reason = buf;
				} else if (!strcmp(tag, "PHel")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					punishments::Get(this, punishments::EType::kHell).duration = lnum;
					punishments::Get(this, punishments::EType::kHell).level = num2;
					punishments::Get(this, punishments::EType::kHell).godid = lnum3;
					punishments::Get(this, punishments::EType::kHell).reason = buf;
				} else if (!strcmp(tag, "PDum")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					punishments::Get(this, punishments::EType::kDumb).duration = lnum;
					punishments::Get(this, punishments::EType::kDumb).level = num2;
					punishments::Get(this, punishments::EType::kDumb).godid = lnum3;
					punishments::Get(this, punishments::EType::kDumb).reason = buf;
				} else if (!strcmp(tag, "PNam")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					punishments::Get(this, punishments::EType::kName).duration = lnum;
					punishments::Get(this, punishments::EType::kName).level = num2;
					punishments::Get(this, punishments::EType::kName).godid = lnum3;
					punishments::Get(this, punishments::EType::kName).reason = buf;
				} else if (!strcmp(tag, "PFrz")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					punishments::Get(this, punishments::EType::kFreeze).duration = lnum;
					punishments::Get(this, punishments::EType::kFreeze).level = num2;
					punishments::Get(this, punishments::EType::kFreeze).godid = lnum3;
					punishments::Get(this, punishments::EType::kFreeze).reason = buf;
				} else if (!strcmp(tag, "PGcs")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					punishments::Get(this, punishments::EType::kGcurse).duration = lnum;
					punishments::Get(this, punishments::EType::kGcurse).level = num2;
					punishments::Get(this, punishments::EType::kGcurse).godid = lnum3;
					punishments::Get(this, punishments::EType::kGcurse).reason = buf;
				} else if (!strcmp(tag, "PUnr")) {
					sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
					punishments::Get(this, punishments::EType::kUnreg).duration = lnum;
					punishments::Get(this, punishments::EType::kUnreg).level = num2;
					punishments::Get(this, punishments::EType::kUnreg).godid = lnum3;
					punishments::Get(this, punishments::EType::kUnreg).reason = buf;
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
// issue.class-recipes: владение рецептом - свойство класса (cfg/classes/pc_*.xml).
						if (num < 0 || !MUD::Class(this->GetClass()).FindIngredientRecipe(imrecipes[num].str_id))
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
								SetSkill(this, skill_id, num2);
							}
						}
					} while (num != 0);
				} else if (!strcmp(tag, "SkTm")) {
					do {
						long int num3;
						fbgetline(fl, line);
						sscanf(line, "%d %ld", &num, &num3);
						if (num != 0) {
							this->timed_skill[static_cast<ESkill>(num)] = time(0) + num3;
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
					(this)->player_specials->saved.stringLength = num;
				else if (!strcmp(tag, "StrW"))
					(this)->player_specials->saved.stringWidth = num;
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
					GET_COND(this, condition::kThirst) = num;
				else if (!strcmp(tag, "Titl"))
					this->SetTitleStr(line);
				else if (!strcmp(tag, "Tglo")) {
					this->setGloryRespecTime(static_cast<time_t>(num));
				} else if (!strcmp(tag, "Tlgr")) {
					if (llnum <= 10000000000000ULL) {
						this->player_specials->saved.telegram_id = static_cast<unsigned long>(llnum);
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
		GET_COND(this, condition::kFull) = -1;
		GET_COND(this, condition::kThirst) = -1;
		GET_COND(this, condition::kDrunk) = -1;
		GET_LOADROOM(this) = kNowhere;
	}

//	SetInbornAndRaceFeats(this); ставим в do_entergame

	if (privilege::IsGrGod(this)) {
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_TYPE(this, spell_id) = GET_SPELL_TYPE(this, spell_id) |
				ESpellType::kItemCast | ESpellType::kKnow | ESpellType::kRunes | ESpellType::kScrollCast
				| ESpellType::kPotionCast | ESpellType::kWandCast;
		}
	} else if (!privilege::IsImmortal(this)) {
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			const auto spell = MUD::Class(this->GetClass()).spells[spell_id];
			if (spell.GetCircle() == kMaxMemoryCircle) {
				REMOVE_BIT(GET_SPELL_TYPE(this, spell.GetId()), ESpellType::kKnow | ESpellType::kTemp);
			}
			if (remort::GetRealRemort(this) < spell.GetMinRemort()) {
				GET_SPELL_MEM(this, spell_id) = 0;
			}
		}
	}

	/*
	 * If you're not poisioned and you've been away for more than an hour of
	 * real time, we'll set your HMV back to full
	 */
	if (!AFF_FLAGGED(this, EAffect::kPoisoned) && (((long) (time(0) - this->get_last_logon())) >= kSecsPerRealHour)) {
		this->set_hit(this->get_real_max_hit());
		this->set_move(this->get_real_max_move());
	} else
		this->set_hit(std::min(this->get_hit(), this->get_real_max_hit()));

	// Сверка CRC из буфера fb (весь файл уже в памяти) -- до fbclose, который
	// освобождает буфер; без повторного чтения только что прочитанного файла.
	// здесь мы закладываемся на то, что при ребуте это все сейчас пропускается и это нормально,
	// иначе в таблице crc будут пустые имена, т.к. сама плеер-таблица еще не сформирована
	// и в любом случае при ребуте это все пересчитывать не нужно
	if (!(load_flags & ELoadCharFlags::kNoCrcCheck)) {
		FileCRC::verify_from_content(this->get_uid(), FileCRC::kPlayer, fl->buf, fl->size);
	}
	fbclose(fl);

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

int Player::get_reset_stats_cnt(stats_reset::Type type) const {
	return reset_stats_cnt_.at(type);
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
