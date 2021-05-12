// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PLAYER_I_HPP_INCLUDED
#define PLAYER_I_HPP_INCLUDED

#include "reset_stats.hpp"
#include "boards/boards.types.hpp"
#include "structs.h"
#include "conf.h"
#include "sysdep.h"
#include <map>
#include <string>
#include <cmd/mercenary.h>

struct MERCDATA;

class Account;
namespace DpsSystem {
class Dps;
}

namespace MapSystem {
struct Options;
}

extern room_rnum r_helled_start_room;

/**
* Интерфейс плеера с пустыми функциями, находящийся у моба.
* Соответственно здесь должны повторяться все открытые методы Player.
* TODO: логирование на mob using как раньше в плеере.
*/
class PlayerI {
 public:
  virtual int get_pfilepos() const { return -1; };
  virtual void set_pfilepos(int/* pfilepos*/) {};

  virtual room_rnum get_was_in_room() const { return NOWHERE; };
  virtual void set_was_in_room(room_rnum/* was_in_room*/) {};

  virtual std::string const &get_passwd() const { return empty_const_str; };
  virtual void set_passwd(std::string const &/* passwd*/) {};

  virtual room_rnum get_from_room() const { return r_helled_start_room; };
  virtual void set_from_room(room_rnum/* was_in_room*/) {};

  virtual void remort() {};
  virtual void reset() {};

  virtual int get_start_stat(int/* num*/) { return 0; };
  virtual void set_start_stat(int/* stat_num*/, int/* number*/) {};

  virtual void set_last_tell(const char * /*text*/) {};
  virtual std::string const &get_last_tell() { return empty_const_str; };

  virtual void set_answer_id(int/* id*/) {};
  virtual int get_answer_id() const { return NOBODY; };

  virtual void remember_add(const std::string &/* text*/, int/* flag*/) {};
  virtual std::string remember_get(int/* flag*/) const { return ""; };
  virtual bool remember_set_num(unsigned int/* num*/) { return false; };
  virtual unsigned int remember_get_num() const { return 0; };

  virtual void quested_add(CHAR_DATA * /*ch*/, int/* vnum*/, char * /*text*/) {};
  virtual bool quested_remove(int/* vnum*/) { return false; };
  virtual bool quested_get(int/* vnum*/) const { return false; };
  virtual std::string quested_get_text(int/* vnum*/) const { return ""; };
  virtual std::string quested_print() const { return ""; };
  virtual void quested_save(FILE * /*saved*/) const {};

  virtual int mobmax_get(int/* vnum*/) const { return 0; };
  virtual void mobmax_add(CHAR_DATA * /*ch*/, int/* vnum*/, int/* count*/, int/* level*/) {};
  virtual void mobmax_load(CHAR_DATA * /*ch*/, int/* vnum*/, int/* count*/, int/* level*/) {};
  virtual void mobmax_remove(int/* vnum*/) {};
  virtual void mobmax_save(FILE * /*saved*/) const {};

  virtual void dps_add_dmg(int/* type*/, int/* dmg*/, int/* over_dmg*/ = 0, CHAR_DATA * /*ch*/ = 0) {};
  virtual void dps_clear(int/* type*/) {};
  virtual void dps_print_stats(CHAR_DATA * /*coder*/ = 0) {};
  virtual void dps_print_group_stats(CHAR_DATA * /*ch*/, CHAR_DATA * /*coder*/ = 0) {};
  virtual void dps_set(DpsSystem::Dps * /*dps*/) {};
  virtual void dps_copy(CHAR_DATA * /*ch*/) {};
  virtual void dps_end_round(int/* type*/, CHAR_DATA * /*ch*/ = 0) {};
  virtual void dps_add_exp(int/* exp*/, bool/* battle*/ = false) {};

  virtual void save_char() {};
  virtual int load_char_ascii(const char * /*name*/, bool/* reboot*/ = 0, const bool /*find_id*/ = true) { return -1; };

  virtual bool get_disposable_flag(int/* num*/) { return false; };
  virtual void set_disposable_flag(int/* num*/) {};

  virtual bool is_active() const { return false; };
  virtual void set_motion(bool/* flag*/) {};

  virtual void map_olc() {};
  virtual void map_olc_save() {};
  virtual bool map_check_option(int/* num*/) const { return false; };
  virtual void map_set_option(unsigned/* num*/) {};
  virtual void map_print_to_snooper(CHAR_DATA * /*imm*/) {};
  virtual void map_text_olc(const char * /*arg*/) {};
  virtual const MapSystem::Options *get_map_options() const { return &empty_map_options; };

  virtual int get_ext_money(unsigned/* type*/) const { return 0; };
  virtual void set_ext_money(unsigned/* type*/, int/* num*/, bool/* write_log*/ = true) {};

  virtual int get_today_torc() { return 0; };
  virtual void add_today_torc(int/* num*/) {};

  virtual int get_reset_stats_cnt(ResetStats::Type/* type*/) const { return 0; };
  virtual void inc_reset_stats_cnt(ResetStats::Type/* type*/) {};

  virtual time_t get_board_date(Boards::BoardTypes/* type*/) const { return 0; };
  virtual void set_board_date(Boards::BoardTypes/* type*/, time_t/* date*/) {};

  virtual int get_ice_currency() { return 0; };
  virtual void set_ice_currency(int  /* value */) {};
  virtual void add_ice_currency(int /* value */) {};
  virtual void sub_ice_currency(int /* value */) {};
  virtual int get_hryvn() { return 0; }
  virtual void set_hryvn(int /* value */) {};
  virtual void sub_hryvn(int /* value */) {};
  virtual void add_hryvn(int /* value */) {};
  virtual void dquest(int /*id */) {};
  int get_count_daily_quest(int /*id*/) { return 0; };
  void add_daily_quest(int /*id*/, int /*count*/) {};
  void spent_hryvn_sub(int /*value*/) {};
  int get_spent_hryvn() { return 0; };
  void reset_daily_quest() {};
  virtual void add_value_cities(bool /* value */) {};

  virtual void str_to_cities(std::string /*value*/) {};
  std::string cities_to_str() { return ""; };
  virtual bool check_city(const size_t) { return false; };
  virtual void mark_city(const size_t) {};
  /*virtual void touch_stigma(char* buf) {};
  virtual void add_stigma(int wear, int id_stigma) {}*/
  virtual int death_player_count() {
    return 1;
  };
  virtual std::shared_ptr<Account> get_account() { return nullptr; };
  virtual void updateCharmee(int /*vnum*/, int /*gold*/) {};
  virtual std::map<int, MERCDATA> *getMercList() { return nullptr; };

  virtual void setTelegramId(unsigned long int) {};
  virtual unsigned long int getTelegramId() { return 0; };
  virtual void setGloryRespecTime(time_t) {};
  virtual time_t getGloryRespecTime() { return 0; };
 protected:
  PlayerI() {};
  virtual ~PlayerI() {};

 private:
  static std::string empty_const_str;
  static MapSystem::Options empty_map_options;
};

#endif // PLAYER_I_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
