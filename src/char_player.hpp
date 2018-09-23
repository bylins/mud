// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_HPP_INCLUDED
#define CHAR_PLAYER_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "quested.hpp"
#include "mobmax.hpp"
#include "remember.hpp"
#include "char.hpp"
#include "dps.hpp"
#include "map.hpp"
#include "reset_stats.hpp"
#include "boards.types.hpp"
#include "quest.hpp"
#include "stigmas.hpp"

#include <string>
#include <array>
#include <vector>
#include <bitset>

// кол-во сохраняемых стартовых статов в файле
const int START_STATS_TOTAL = 6;

// для одноразовых флагов
enum
{
	DIS_OFFTOP_MESSAGE,
	DIS_EXCHANGE_MESSAGE,
	DIS_TOTAL_NUM
};


class Player : public CHAR_DATA
{
public:
	using cities_t = boost::dynamic_bitset<std::size_t>;

	Player();

	int get_pfilepos() const;
	void set_pfilepos(int pfilepos);

	room_rnum get_was_in_room() const;
	void set_was_in_room(room_rnum was_in_room);

	std::string const & get_passwd() const;
	void set_passwd(std::string const & passwd);

	room_rnum get_from_room() const;
	void set_from_room(room_rnum was_in_room);
	
	void remort();
	void reset();

	int get_start_stat(int num);
	void set_start_stat(int stat_num, int number);

	void set_last_tell(const char *text);
	std::string const & get_last_tell();

	void set_answer_id(int id);
	int get_answer_id() const;

	// обертка на CharRemember
	virtual void remember_add(const std::string& text, int flag);
	std::string remember_get(int flag) const;
	bool remember_set_num(unsigned int num);
	unsigned int remember_get_num() const;

	// обертка на Quested
	void quested_add(CHAR_DATA *ch, int vnum, char *text);
	bool quested_remove(int vnum);
	bool quested_get(int vnum) const;
	std::string quested_get_text(int vnum) const;
	std::string quested_print() const;
	void quested_save(FILE *saved) const; // TODO мб убрать

	// обертка на MobMax
	int mobmax_get(int vnum) const;
	void mobmax_add(CHAR_DATA *ch, int vnum, int count, int level);
	void mobmax_load(CHAR_DATA *ch, int vnum, int count, int level);
	void mobmax_remove(int vnum);
	void mobmax_save(FILE *saved) const; // TODO мб убрать

	// обертка на Dps
	void dps_add_dmg(int type, int dmg, int over_dmg, CHAR_DATA *ch = 0);
	void dps_clear(int type);
	void dps_print_stats(CHAR_DATA *coder = 0);
	void dps_print_group_stats(CHAR_DATA *ch, CHAR_DATA *coder = 0);
	void dps_set(DpsSystem::Dps *dps);
	void dps_copy(CHAR_DATA *ch);
	void dps_end_round(int type, CHAR_DATA *ch = 0);
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
	void map_print_to_snooper(CHAR_DATA *imm);
	void map_text_olc(const char *arg);
	const MapSystem::Options * get_map_options() const;

	int get_ext_money(unsigned type) const;
	void set_ext_money(unsigned type, int num, bool write_log = true);
	int get_today_torc();
	void add_today_torc(int num);

	int get_reset_stats_cnt(ResetStats::Type type) const;
	void inc_reset_stats_cnt(ResetStats::Type type);

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

	std::shared_ptr<Account> get_account();

private:
	// показывает, является ли чар турнирным или нет
	bool arena_player = false;
	// порядковый номер в файле плеер-листа (не особо нужен, но бывает удобно видеть по кто)
	// TODO: вообще его можно пользовать вместо постоянного поиска по имени при сейвах чара и т.п. вещах, пользующих
	// get_ptable_by_name или find_name (дублирование кода кстати) и всякие поиски по ид/уид, если уже имеем чар-дату
	int pfilepos_;
	// комната, в которой был чар до того, как его поместили в странную (linkdrop)
	room_rnum was_in_room_;
	// хэш пароля
	std::string passwd_;
	// комната, где был чар до вызова char_from_room (was_in_room_ под это использовать не оч хорошо)
	// в данный момент поле нужно для проверки чара на бд при входе на арену любым способом, но может и еще потом пригодиться
	room_rnum from_room_;
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
	std::array<int, ExtMoney::TOTAL_TYPES> ext_money_;
	// сколько гривн, в пересчете на бронзу, сегодня уже собрано
	std::pair<uint8_t /* day 1-31 */, int> today_torc_;
	// кол-во сбросов характеристик через меню
	std::array<int, ResetStats::Type::TOTAL_NUM> reset_stats_cnt_;
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
	// id задания и сколько раз было выполненно задание
	std::map<int, int> daily_quest;
	// сколько гривен было потрачено в магазине
	int spent_hryvn;
	// айдишник + когда последний раз выполняли данный квест
	std::map<int, time_t> daily_quest_timed;
	// Аккаунт
	std::shared_ptr<Account> account;
};



namespace PlayerSystem
{

int con_natural_hp(CHAR_DATA *ch);
int con_add_hp(CHAR_DATA *ch);
int con_total_hp(CHAR_DATA *ch);
unsigned weight_dex_penalty(CHAR_DATA* ch);

} // namespace PlayerSystem

#endif // CHAR_PLAYER_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
