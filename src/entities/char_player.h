// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_HPP_INCLUDED
#define CHAR_PLAYER_HPP_INCLUDED

#include <string>
#include <array>
#include <vector>
#include <bitset>

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"
#include "game_quests/quest.h"
#include "game_quests/quested.h"
#include "mobmax.h"
#include "communication/remember.h"
#include "char_data.h"
#include "statistics/dps.h"
#include "mapsystem.h"
#include "administration/reset_stats.h"
#include "boards/boards_types.h"
#include "cmd/mercenary.h"

// кол-во сохраняемых стартовых статов в файле
const int START_STATS_TOTAL = 6;

// для одноразовых флагов
enum {
	DIS_OFFTOP_MESSAGE,
	DIS_EXCHANGE_MESSAGE,
	DIS_TOTAL_NUM
};

class Player : public CharData {
 public:
	using cities_t = boost::dynamic_bitset<std::size_t>;

	Player();

	int get_pfilepos() const;
	void set_pfilepos(int pfilepos);

	RoomRnum get_was_in_room() const;
	void set_was_in_room(RoomRnum was_in_room);

	std::string const &get_passwd() const;
	void set_passwd(std::string const &passwd);

	RoomRnum get_from_room() const;
	void set_from_room(RoomRnum was_in_room);

	void remort();
	void reset();

	int get_start_stat(int num);
	void set_start_stat(int stat_num, int number);

	void set_last_tell(const char *text);
	std::string const &get_last_tell();

	void set_answer_id(int id);
	int get_answer_id() const;

	// обертка на CharRemember
	virtual void remember_add(const std::string &text, int flag);
	std::string remember_get(int flag) const;
	bool remember_set_num(unsigned int num);
	unsigned int remember_get_num() const;

	// обертка на Quested
	void quested_add(CharData *ch, int vnum, char *text);
	bool quested_remove(int vnum);
	bool quested_get(int vnum) const;
	std::string quested_get_text(int vnum) const;
	std::string quested_print() const;
	void quested_save(FILE *saved) const; // TODO мб убрать

	// обертка на MobMax
	int mobmax_get(int vnum) const;
	void mobmax_add(CharData *ch, int vnum, int count, int level);
	void mobmax_load(CharData *ch, int vnum, int count, int level);
	void mobmax_remove(int vnum);
	void mobmax_save(FILE *saved) const; ///< TODO мб убрать
	void show_mobmax();

	// обертка на Dps
	void dps_add_dmg(int type, int dmg, int over_dmg, CharData *ch = nullptr);
	void dps_clear(int type);
	void dps_print_stats(CharData *coder = nullptr);
	void dps_print_group_stats(CharData *ch, CharData *coder = nullptr);
	void dps_set(DpsSystem::Dps *dps);
	void dps_copy(CharData *ch);
	void dps_end_round(int type, CharData *ch = nullptr);
	void dps_add_exp(int exp, bool battle = false);

	void save_char();
	int load_char_ascii(const char *name, bool reboot = 0, const bool find_id = true);

	bool get_disposable_flag(int num);
	void set_disposable_flag(int num);

	// пока только как обертка на motion_, но может быть допилено
	bool is_active() const;
	void set_motion(bool flag);

	void map_olc();
	void map_olc_save();
	bool map_check_option(int num) const;
	void map_set_option(unsigned num);
	void map_print_to_snooper(CharData *imm);
	void map_text_olc(const char *arg);
	const MapSystem::Options *get_map_options() const;

	int get_ext_money(unsigned type) const;
	void set_ext_money(unsigned type, int num, bool write_log = true);
	int get_today_torc();
	void add_today_torc(int num);

	int get_reset_stats_cnt(stats_reset::Type type) const;
	void inc_reset_stats_cnt(stats_reset::Type type);

	time_t get_board_date(Boards::BoardTypes type) const;
	void set_board_date(Boards::BoardTypes type, time_t date);

	int get_ice_currency();
	void set_ice_currency(int value);
	void add_ice_currency(int value);
	void sub_ice_currency(int value);

	int death_player_count();

	bool is_arena_player();

	int get_count_daily_quest(int id);
	void add_daily_quest(int id, int count);
	void set_time_daily_quest(int id, time_t time);
	time_t get_time_daily_quest(int id);
	void spent_hryvn_sub(int value);
	int get_spent_hryvn();
	void reset_daily_quest();

	void add_value_cities(bool v);
	void str_to_cities(std::string str);
	std::string cities_to_str();
	bool check_city(const size_t index);
	void mark_city(const size_t index);
	/*void touch_stigma(char *arg);
	void add_stigma(int wear, int id_stigma);
	int get_stigma(int wear);*/
	int get_hryvn();
	void set_hryvn(int value);
	void sub_hryvn(int value);
	void add_hryvn(int value);
	void dquest(int id);
	void complete_quest(int id);
	int get_nogata();
	void set_nogata(int value);
	void sub_nogata(int value);
	void add_nogata(int value);

	std::shared_ptr<Account> get_account();
	// добавить/обновить чармиса в историю игрока
	void updateCharmee(int vnum, int gold);
	// получить список чармисов игрока
	std::map<int, MERCDATA> *getMercList();
	// метод выставления chat_id
	void setTelegramId(unsigned long chat_id) override;
	unsigned long int getTelegramId() override;
	// методы работы с последним временем респека славой
	void setGloryRespecTime(time_t param) override;
	time_t getGloryRespecTime() override;

 private:
	// показывает, является ли чар турнирным или нет
	bool arena_player = false;
	// порядковый номер в файле плеер-листа (не особо нужен, но бывает удобно видеть по кто)
	// TODO: вообще его можно пользовать вместо постоянного поиска по имени при сейвах чара и т.п. вещах, пользующих
	// get_ptable_by_name или find_name (дублирование кода кстати) и всякие поиски по ид/уид, если уже имеем чар-дату
	int pfilepos_;
	// комната, в которой был чар до того, как его поместили в странную (linkdrop)
	RoomRnum was_in_room_;
	// хэш пароля
	std::string passwd_;
	// комната, где был чар до вызова char_from_room (was_in_room_ под это использовать не оч хорошо)
	// в данный момент поле нужно для проверки чара на бд при входе на арену любым способом, но может и еще потом пригодиться
	RoomRnum from_room_;
	// стартовые статы
	std::array<int, START_STATS_TOTAL> start_stats_;
	// вспомнить
	CharRemember remember_;
	// внумы выполненных квестов с сохраненными данными
	Quested quested_;
	// замаксы по отдельным мобам
	MobMax mobmax_;
	// Квесты
	std::vector<Quest> quests;
	// последняя введенная строка (от спама)
	std::string last_tell_;
	// id последнего телявшего (для ответа)
	long answer_id_;
	// 'дметр' персональный и группы, если чар лидер
	DpsSystem::Dps dps_;
	// одноразовые флаги
	std::bitset<DIS_TOTAL_NUM> disposable_flags_;
	// false, если чар помечен как неактивный через check_idling и пока не двинется с места
	bool motion_;
	// опции отрисовки режима карты
	MapSystem::Options map_options_;
	// доп. валюты (гривны)
	std::array<int, ExtMoney::kTotalTypes> ext_money_;
	// сколько гривн, в пересчете на бронзу, сегодня уже собрано
	std::pair<uint8_t /* day 1-31 */, int> today_torc_;
	// кол-во сбросов характеристик через меню
	std::array<int, stats_reset::Type::TOTAL_NUM> reset_stats_cnt_;
	// временнЫе отметки о прочитанных сообщениях на досках
	std::array<time_t, Boards::TYPES_NUM> board_date_;
	// лед (доп. валюта)
	int ice_currency;
	// список зон, где чар умер и в каком количестве
	std::map<int, int> count_death_zone;
	// время, когда были выполнены все дейлики
	time_t time_daily;
	// сколько дней подряд выполнялись дейлики
	int count_daily_quest;
	// Отметка о том, в каких городах был наш чар
	boost::dynamic_bitset<size_t> cities;
	// здесь храним инфу о татуировках
	//std::map<unsigned int, StigmaWear> stigmas;
	// режим !бот
	bool setmode_dontbot;
	// гривны
	int hryvn;
	// ногаты
	int nogata;
	// id задания и сколько раз было выполненно задание
	std::map<int, int> daily_quest;
	// сколько гривен было потрачено в магазине
	int spent_hryvn;
	// айдишник + когда последний раз выполняли данный квест
	std::map<int, time_t> daily_quest_timed;
	// Аккаунт
	std::shared_ptr<Account> account;
	//перечень чармисов, доступных с команды наемник
	std::map<int, MERCDATA> charmeeHistory;
};

namespace PlayerSystem {

int con_natural_hp(CharData *ch);
int con_add_hp(CharData *ch);
int con_total_hp(CharData *ch);
unsigned weight_dex_penalty(CharData *ch);

} // namespace PlayerSystem

#endif // CHAR_PLAYER_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
