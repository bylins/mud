/* ************************************************************************
*   File: interpreter.cpp                               Part of Bylins    *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#define __INTERPRETER_C__

#include "interpreter.h"

#include "world.characters.hpp"
#include "object.prototypes.hpp"
#include "logger.hpp"
#include "craft.commands.hpp"
#include "heartbeat.commands.hpp"
#include "obj.hpp"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "house.h"
#include "mail.h"
#include "screen.h"
#include "olc.h"
#include "dg_scripts.h"
#include "pk.h"
#include "genchar.h"
#include "ban.hpp"
#include "item.creation.hpp"
#include "features.hpp"
#include "boards.h"
#include "top.h"
#include "title.hpp"
#include "names.hpp"
#include "password.hpp"
#include "privilege.hpp"
#include "depot.hpp"
#include "glory.hpp"
#include "char.hpp"
#include "char_player.hpp"
#include "parcel.hpp"
#include "liquid.hpp"
#include "name_list.hpp"
#include "modify.h"
#include "room.hpp"
#include "glory_const.hpp"
#include "glory_misc.hpp"
#include "named_stuff.hpp"
#if defined WITH_SCRIPTING
#include "scripting.hpp"
#endif
#include "player_races.hpp"
#include "birth_places.hpp"
#include "help.hpp"
#include "map.hpp"
#include "ext_money.hpp"
#include "noob.hpp"
#include "reset_stats.hpp"
#include "obj_sets.hpp"
#include "utils.h"
#include "temp_spells.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "bonus.h"
#include "debug.utils.hpp"
#include "global.objects.hpp"
#include "accounts.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <stdexcept>
#include <algorithm>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;
extern const char *class_menu;
extern const char *class_menu_vik;
extern const char *class_menu_step;
extern const char *religion_menu;
extern char *motd;
extern char *rules;
extern char *background;
extern const char *MENU;
extern const char *WELC_MESSG;
extern const char *START_MESSG;
extern DESCRIPTOR_DATA *descriptor_list;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern INDEX_DATA *mob_index;
extern const char *default_race[];
extern void add_karma(CHAR_DATA * ch, const char * punish , const char * reason);
extern struct pclean_criteria_data pclean_criteria[];

extern char *GREETINGS;
extern const char *pc_class_types[];
extern const char *pc_class_types_vik[];
extern const char *pc_class_types_step[];
//extern const char *race_types[];
extern const char *race_types_step[];
extern const char *race_types_vik[];
extern const char *kin_types[];
extern struct set_struct set_fields[];
extern struct show_struct show_fields[];
extern char *name_rules;

// external functions
void do_start(CHAR_DATA * ch, int newbie);
int parse_class(char arg);
int parse_class_vik(char arg);
int parse_class_step(char arg);
int Valid_Name(char *newname);
int Is_Valid_Name(char *newname);
int Is_Valid_Dc(char *newname);
void read_aliases(CHAR_DATA * ch);
void read_saved_vars(CHAR_DATA * ch);
void oedit_parse(DESCRIPTOR_DATA * d, char *arg);
void redit_parse(DESCRIPTOR_DATA * d, char *arg);
void zedit_parse(DESCRIPTOR_DATA * d, char *arg);
void medit_parse(DESCRIPTOR_DATA * d, char *arg);
void trigedit_parse(DESCRIPTOR_DATA * d, char *arg);
int find_social(char *name);
void do_aggressive_room(CHAR_DATA * ch, int check_sneak);
extern int CheckProxy(DESCRIPTOR_DATA * ch);
extern void check_max_hp(CHAR_DATA *ch);
// local functions
int perform_dupe_check(DESCRIPTOR_DATA * d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int perform_alias(DESCRIPTOR_DATA * d, char *orig);
int reserved_word(const char *argument);
int _parse_name(char *arg, char *name);
void add_logon_record(DESCRIPTOR_DATA * d);
// prototypes for all do_x functions.
int find_action(char *cmd);
int do_social(CHAR_DATA * ch, char *argument);
void init_warcry(CHAR_DATA *ch);

void do_advance(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_alias(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_antigods(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_assist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_at(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_affects(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_backstab(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_ban(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_bash(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_beep(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_cast(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_warcry(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_clanstuff(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_create(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_expedient_cut(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mixture(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_courage(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_commands(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_consider(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_credits(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_date(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_dc(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_diagnose(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_display(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_drink(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_drunkoff(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_features(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_featset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_findhelpee(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_firstaid(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_fire(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_drop(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_eat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_echo(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_enter(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_manadrain(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_equipment(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_examine(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_revenge(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_remort(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_remember_char(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_exit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_exits(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_flee(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_follow(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_horseon(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_horseoff(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_horseput(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_horseget(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_horsetake(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_hidetrack(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_hidemove(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_fit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_force(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_extinguish(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_forcetime(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_glory(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gecho(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gen_comm(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mobshout(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gen_door(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gen_ps(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_get(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_give(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_givehorse(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gold(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_goto(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_grab(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_group(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gsay(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_hide(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_hit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_info(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_inspect(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_insult(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_inventory(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_invis(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_kick(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_kill(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_last(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mode(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mark(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_makefood(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_disarm(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_chopoff(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_deviate(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_levels(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_liblist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_lightwalk(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_load(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_loadstat(CHAR_DATA *ch, char *argument, int cmd, int subbcmd);
void do_look(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sides(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_not_here(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_offer(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_olc(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_order(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_page(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_pray(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_poofset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_pour(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_skills(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_statistic(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_spells(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_spellstat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_remember(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_learn(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_forget(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_purge(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_put(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_quit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_reboot(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_remove(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_rent(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_reply(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_report(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_refill(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_rescue(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_stopfight(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_setall(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_stophorse(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_rest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_restore(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_return(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_save(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_say(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_score(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sdemigod(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_send(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_set(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_show(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_shutdown(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_skillset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sleep(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sneak(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_snoop(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_spec_comm(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_spell_capable(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_split(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_split(CHAR_DATA *ch, char *argument, int cmd, int subcmd,int currency);
void do_stand(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_fry(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_stat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_steal(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_switch(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_syslog(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_throw(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_teleport(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_tell(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_time(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_toggle(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_track(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sense(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_unban(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_ungroup(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_use(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_users(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_visible(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_vnum(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_vstat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wake(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wear(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_weather(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_where(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_who(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wield(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wimpy(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wizlock(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wiznet(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wizutil(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_write(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_zreset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_parry(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_multyparry(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_style(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_poisoned(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_repair(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_camouflage(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_stupor(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mighthit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_block(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_touch(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_transform_weapon(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_protect(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_dig(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_insertgem(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_ignore(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_proxy(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_turn_undead(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_iron_wind(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_exchange(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_godtest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_print_armor(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_relocate(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_strangle(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_custom_label(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_quest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_check(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
// DG Script ACMD's
void do_attach(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_detach(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_tlist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_tstat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_vdelete(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_hearing(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_looking(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_ident(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_upgrade(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_armored(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_recall(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_pray_gods(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_rset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_recipes(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_cook(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_forgive(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_imlist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_townportal(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoHouse(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoClanChannel(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoClanList(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoShowPolitics(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoShowWars(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_show_alliance(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoHcontrol(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoWhoClan(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoClanPkList(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoStoreHouse(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_clanstuff(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void DoBest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_offtop(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_dmeter(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_mystat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_zone(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_bandage(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sanitize(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_morph(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_morphset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_console(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_shops_list(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_unfreeze(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void Bonus::do_bonus_by_character(CHAR_DATA*, char*, int, int);
void do_summon(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_check_occupation(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_delete_obj(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_arena_restore(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void Bonus::do_bonus_info(CHAR_DATA*, char*, int, int);
void do_stun(CHAR_DATA*, char*, int, int);
void do_showzonestats(CHAR_DATA*, char*, int, int);
void do_overstuff(CHAR_DATA *ch, char*, int, int);
void do_cities(CHAR_DATA *ch, char*, int, int);
void do_send_text_to_char(CHAR_DATA *ch, char*, int, int);
void do_add_wizard(CHAR_DATA *ch, char*, int, int);
void do_touch_stigma(CHAR_DATA *ch, char*, int, int);

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

#define MAGIC_NUM 419
#define MAGIC_LEN 8

// здесь храним коды, которые отправили игрокам на почту
// строка - это мыло, если один чар вошел с необычного места, то блочим сразу всех чаров на этом мыле,
// пока не введет код (или до ребута)
std::map<std::string, int> new_loc_codes;

// имя чара на код, отправленный на почту для подтверждения мыла при создании
std::map<std::string, int> new_char_codes;

void do_debug_queues(CHAR_DATA * /*ch*/, char *argument, int /*cmd*/, int /*subcmd*/)
{
	std::stringstream ss;
	if (argument && *argument)
	{
		debug::log_queue(argument).print_queue(ss, argument);

		return;
	}

	for (const auto& q : debug::log_queues())
	{
		q.second.print_queue(ss, q.first);
	}

	mudlog(ss.str().c_str(), DEF, LVL_GOD, ERRLOG, TRUE);
}

cpp_extern const struct command_info cmd_info[] =
{
	{"RESERVED", 0, 0, 0, 0, 0},	// this must be first -- for specprocs

	// directions must come before other commands but after RESERVED
	{"север", POS_STANDING, do_move, 0, SCMD_NORTH, -2},
	{"восток", POS_STANDING, do_move, 0, SCMD_EAST, -2},
	{"юг", POS_STANDING, do_move, 0, SCMD_SOUTH, -2},
	{"запад", POS_STANDING, do_move, 0, SCMD_WEST, -2},
	{"вверх", POS_STANDING, do_move, 0, SCMD_UP, -2},
	{"вниз", POS_STANDING, do_move, 0, SCMD_DOWN, -2},
	{"north", POS_STANDING, do_move, 0, SCMD_NORTH, -2},
	{"east", POS_STANDING, do_move, 0, SCMD_EAST, -2},
	{"south", POS_STANDING, do_move, 0, SCMD_SOUTH, -2},
	{"west", POS_STANDING, do_move, 0, SCMD_WEST, -2},
	{"up", POS_STANDING, do_move, 0, SCMD_UP, -2},
	{"down", POS_STANDING, do_move, 0, SCMD_DOWN, -2},

	{"аффекты", POS_DEAD, do_affects, 0, SCMD_AUCTION, 0},
	{"авторы", POS_DEAD, do_gen_ps, 0, SCMD_CREDITS, 0},
	{"атаковать", POS_FIGHTING, do_hit, 0, SCMD_MURDER, -1},
	{"аукцион", POS_RESTING, do_gen_comm, 0, SCMD_AUCTION, 100},
	{"анонсы", POS_DEAD, Boards::DoBoard, 1, Boards::NOTICE_BOARD, -1},

	{"базар", POS_RESTING, do_exchange, 1, 0, -1},
	{"баланс", POS_STANDING, do_not_here, 1, 0, 0},
	{"баги", POS_DEAD, Boards::DoBoard, 1, Boards::ERROR_BOARD, 0},
	{"бежать", POS_FIGHTING, do_flee, 1, 0, -1},
	{"бинтовать", POS_RESTING, do_bandage, 0, 0, 0},
	{"билдер", POS_DEAD, Boards::DoBoard, 1,Boards:: GODBUILD_BOARD, -1},
	{"блок", POS_FIGHTING, do_block, 0, 0, -1},
	{"блокнот", POS_DEAD, Boards::DoBoard, 1, Boards::PERS_BOARD, -1},
	{"боги", POS_DEAD, do_gen_ps, 0, SCMD_IMMLIST, 0},
	{"божества", POS_DEAD, Boards::DoBoard, 1, Boards::GODGENERAL_BOARD, -1},
	{"болтать", POS_RESTING, do_gen_comm, 0, SCMD_GOSSIP, -1},
	{"бонус", POS_DEAD, Bonus::do_bonus_by_character, LVL_IMPL, 0, 0},
	{"бонусинфо", POS_DEAD, Bonus::do_bonus_info, LVL_IMPL, 0, 0 },
	{"бросить", POS_RESTING, do_drop, 0, SCMD_DROP, -1},
	{"варить", POS_RESTING, do_cook, 0, 0, 200},
	{"версия", POS_DEAD, do_gen_ps, 0, SCMD_VERSION, 0},
	{"вече", POS_DEAD, Boards::DoBoard, 1, Boards::GENERAL_BOARD, -1},
	{"взять", POS_RESTING, do_get, 0, 0, 200},
	{"взглянуть", POS_RESTING, do_diagnose, 0, 0, 100},
	{"взломать", POS_STANDING, do_gen_door, 1, SCMD_PICK, -1},
	{"вихрь", POS_FIGHTING, do_iron_wind, 0, 0, -1},
	{"вложить", POS_STANDING, do_not_here, 1, 0, -1},
	{"вернуть", POS_STANDING, do_not_here, 0, 0, -1},
	{"вернуться", POS_DEAD, do_return, 0, 0, -1},
	{"войти", POS_STANDING, do_enter, 0, 0, -2},
	{"война", POS_RESTING, DoShowWars, 0, 0, 0},
	{"вооружиться", POS_RESTING, do_wield, 0, 0, 200},
	{"возврат", POS_RESTING, do_recall, 0, 0, -1},
	{"воззвать", POS_DEAD, do_pray_gods, 0, 0, -1},
	{"вплавить", POS_STANDING, do_insertgem, 0, SKILL_INSERTGEM, -1},
	{"время", POS_DEAD, do_time, 0, 0, 0},
	{"врата", POS_SITTING, do_townportal, 1, 0, -1},
	{"вскочить", POS_FIGHTING, do_horseon, 0, 0, 500},
	{"встать", POS_RESTING, do_stand, 0, 0, 500},
	{"вспомнить", POS_DEAD, do_remember_char, 0, 0, 0},
	{"выбросить", POS_RESTING, do_drop, 0, 0 /*SCMD_DONATE */ , 300},
	{"выследить", POS_STANDING, do_track, 0, 0, 500},
	{"вылить", POS_STANDING, do_pour, 0, SCMD_POUR, 500},
	{"выходы", POS_RESTING, do_exits, 0, 0, 0},

	{"говорить", POS_RESTING, do_say, 0, 0, -1},
	{"ггруппа", POS_SLEEPING, do_gsay, 0, 0, 500},
	{"гговорить", POS_SLEEPING, do_gsay, 0, 0, 500},
	{"гдругам", POS_SLEEPING, DoClanChannel, 0, SCMD_CHANNEL, 0},
	{"где", POS_RESTING, do_where, LVL_IMMORT, 0, 0},
	{"гдея", POS_RESTING, do_zone, 0, 0, 0},
	{"глоток", POS_RESTING, do_drink, 0, SCMD_SIP, 200},
	{"города", POS_DEAD, do_cities, 0, 0, 0 },
	{"группа", POS_SLEEPING, do_group, 1, 0, -1},
	{"гсоюзникам", POS_SLEEPING, DoClanChannel, 0, SCMD_ACHANNEL, 0},
	{"гэхо", POS_DEAD, do_gecho, LVL_GOD, 0, 0},
	{"гбогам", POS_DEAD, do_wiznet, LVL_IMMORT, 0, 0},

	{"дать", POS_RESTING, do_give, 0, 0, 500},
	{"дата", POS_DEAD, do_date, 0, SCMD_DATE, 0},
	{"делить", POS_RESTING, do_split, 1, 0, 200},
	{"держать", POS_RESTING, do_grab, 0, 0, 300},
	{"дметр", POS_DEAD, do_dmeter, 0, 0, 0},
	{"доложить", POS_RESTING, do_report, 0, 0, 500},
	{"доски", POS_DEAD, Boards::DoBoardList, 0, 0, 0},
	{"дружины", POS_DEAD, DoClanList, 0, 0, 0},
	{"дрновости", POS_DEAD, Boards::DoBoard, 1, Boards::CLANNEWS_BOARD, -1},
	{"дрвече", POS_DEAD, Boards::DoBoard, 1, Boards::CLAN_BOARD, -1},
	{"дрлист", POS_DEAD, DoClanPkList, 0, 1, 0},
	//{"добавить", POS_DEAD, do_add_wizard, LVL_IMPL, 0, 0 },
	{"есть", POS_RESTING, do_eat, 0, SCMD_EAT, 500},

	{"жертвовать", POS_STANDING, do_pray, 1, SCMD_DONATE, -1},

	{"заколоть", POS_STANDING, do_backstab, 1, 0, 1},
	{"забыть", POS_RESTING, do_forget, 0, 0, 0},
	{"задержать", POS_STANDING, do_not_here, 1, 0, -1},
	{"заклинания", POS_SLEEPING, do_spells, 0, 0, 0},
	{"заклстат", POS_DEAD, do_spellstat, LVL_GRGOD, 0, 0},
	{"закрыть", POS_SITTING, do_gen_door, 0, SCMD_CLOSE, 500},
	{"замести", POS_STANDING, do_hidetrack, 1, 0, -1},
	{"замолчать", POS_DEAD, do_wizutil, LVL_GOD, SCMD_MUTE, 0},
	{"заморозить", POS_DEAD, do_wizutil, LVL_FREEZE, SCMD_FREEZE, 0},
	{"занятость", POS_DEAD, do_check_occupation, LVL_GOD, 0, 0},
	{"запомнить", POS_RESTING, do_remember, 0, 0, 0},
	{"запереть", POS_SITTING, do_gen_door, 0, SCMD_LOCK, 500},
	{"запрет", POS_DEAD, do_ban, LVL_GRGOD, 0, 0},
	{"заснуть", POS_SLEEPING, do_sleep, 0, 0, -1},
	{"заставка", POS_DEAD, do_gen_ps, 0, SCMD_MOTD, 0},
	{"заставить", POS_SLEEPING, do_force, LVL_GRGOD, 0, 0},
	{"затоптать", POS_STANDING, do_extinguish, 0, 0, 0},
	{"заточить", POS_RESTING, do_upgrade, 0, 0, 500},
	{"заучить", POS_RESTING, do_remember, 0, 0, 0},
	{"зачитать", POS_RESTING, do_use, 0, SCMD_RECITE, 500},
	{"зачаровать", POS_STANDING, do_spell_capable, 1, 0, 0},
	{"зачистить", POS_DEAD, do_sanitize, LVL_GRGOD, 0, 0},
	{"золото", POS_RESTING, do_gold, 0, 0, 0},
	{"зона", POS_RESTING, do_zone, 0, 0, 0},
	{"зоныстат", POS_DEAD, do_showzonestats, LVL_IMMORT, 0, 0 },
	{"инвентарь", POS_SLEEPING, do_inventory, 0, 0, 0},
	{"игнорировать", POS_DEAD, do_ignore, 0, 0, 0},
	{"идеи", POS_DEAD, Boards::DoBoard, 1, Boards::IDEA_BOARD, 0},
	{"изгнать нежить", POS_RESTING, do_turn_undead, 0, 0, -1},
	{"изучить", POS_SITTING, do_learn, 0, 0, 0},
	{"информация", POS_SLEEPING, do_gen_ps, 0, SCMD_INFO, 0},
	{"испить", POS_RESTING, do_use, 0, SCMD_QUAFF, 500},
	{"имя", POS_SLEEPING, do_name, LVL_IMMORT, 0, 0},

	{"колдовать", POS_SITTING, do_cast, 1, 0, -1},
	{"казна", POS_RESTING, do_not_here, 1, 0, 0},
	{"карта", POS_RESTING, do_map, 0, 0, 0},
	{"клан", POS_RESTING, DoHouse, 0, 0, 0},
	{"клич", POS_FIGHTING, do_warcry, 1, 0, -1},
	{"кодер", POS_DEAD, Boards::DoBoard, 1, Boards::CODER_BOARD, -1},
	{"команды", POS_DEAD, do_commands, 0, SCMD_COMMANDS, 0},
	{"коне", POS_SLEEPING, do_quit, 0, 0, 0},
	{"конец", POS_SLEEPING, do_quit, 0, SCMD_QUIT, 0},
	{"копать", POS_STANDING, do_dig, 0, SKILL_DIG, -1},
	{"красться", POS_STANDING, do_hidemove, 1, 0, -2},
	{"кричать", POS_RESTING, do_gen_comm, 0, SCMD_SHOUT, -1},
	{"кто", POS_RESTING, do_who, 0, 0, 0},
	{"ктодружина", POS_RESTING, DoWhoClan, 0, 0, 0},
	{"ктоя", POS_DEAD, do_gen_ps, 0, SCMD_WHOAMI, 0},
	{"купить", POS_STANDING, do_not_here, 0, 0, -1},

	{"леваярука", POS_RESTING, do_grab, 1, 0, 300},
	{"лечить", POS_STANDING, do_firstaid, 0, 0, -1},
	{"лить", POS_STANDING, do_pour, 0, SCMD_POUR, 500},
	{"лошадь", POS_STANDING, do_not_here, 1, 0, -1},
	{"лучшие", POS_DEAD, DoBest, 0, 0, 0},

	{"маскировка", POS_RESTING, do_camouflage, 0, 0, 500},
	{"магазины", POS_DEAD, do_shops_list, LVL_IMMORT, 0, 0},
	{"метнуть", POS_FIGHTING, do_throw, 0, 0, -1},
	{"менять", POS_STANDING, do_not_here, 0, 0, -1},
	{"месть", POS_RESTING, do_revenge, 0, 0, 0},
	{"молот", POS_FIGHTING, do_mighthit, 0, 0, -1},
	{"молиться", POS_STANDING, do_pray, 1, SCMD_PRAY, -1},
	{"моястатистика", POS_DEAD, do_mystat, 0, 0, 0},
	{"мысл", POS_DEAD, do_quit, 0, 0, 0},
	{"мысль", POS_DEAD, Boards::report_on_board, 0, Boards::SUGGEST_BOARD, 0},

	{"наказания", POS_DEAD, Boards::DoBoard, 1, Boards::GODPUNISH_BOARD, -1},
	{"налить", POS_STANDING, do_pour, 0, SCMD_FILL, 500},
	{"наполнить", POS_STANDING, do_pour, 0, SCMD_FILL, 500},
	{"найти", POS_STANDING, do_sense, 0, 0, 500},
	{"нанять", POS_STANDING, do_findhelpee, 0, SCMD_BUYHELPEE, -1},
	{"новичок", POS_SLEEPING, do_gen_ps, 0, SCMD_INFO, 0},
	{"новости", POS_DEAD, Boards::DoBoard, 1, Boards::NEWS_BOARD, -1},
	{"надеть", POS_RESTING, do_wear, 0, 0, 500},
	{"нацарапать", POS_RESTING, do_custom_label, 0, 0, 0},

	{"обезоружить", POS_FIGHTING, do_disarm, 0, 0, -1},
	{"обернуться", POS_STANDING, do_morph, 0, 0, -1},
	{"облачить", POS_RESTING, do_wear, 0, 0, 500},
	{"обмен", POS_STANDING, do_not_here, 0, 0, 0},
	{"обменять", POS_STANDING, do_not_here, 0, 0, 0},
	{"оглядеться", POS_RESTING, do_sides, 0, 0, 0},
	{"оглушить", POS_FIGHTING, do_stupor, 0, 0, -1},
	{"одеть", POS_RESTING, do_wear, 0, 0, 500},
	{"опознать", POS_RESTING, do_ident, 0, 0, 500},
	{"опохмелиться", POS_RESTING, do_drunkoff, 0, 0, -1},
	{"опечатк", POS_DEAD, do_quit, 0, 0, 0},
	{"опечатка", POS_DEAD, Boards::report_on_board, 0, Boards::MISPRINT_BOARD, 0},
	{"опустить", POS_RESTING, do_put, 0, 0, 500},
	{"орать", POS_RESTING, do_gen_comm, 1, SCMD_HOLLER, -1},
	{"осмотреть", POS_RESTING, do_examine, 0, 0, 0},
	{"оседлать", POS_STANDING, do_horsetake, 1, 0, -1},
	{"оскорбить", POS_RESTING, do_insult, 0, 0, -1},
	{"осушить", POS_RESTING, do_use, 0, SCMD_QUAFF, 300},
	{"освежевать", POS_STANDING, do_makefood, 0, 0, -1},
	{"ответить", POS_RESTING, do_reply, 0, 0, -1},
	{"отразить", POS_FIGHTING, do_multyparry, 0, 0, -1},
	{"отвязать", POS_DEAD, do_horseget, 0, 0, -1},
	{"отдохнуть", POS_RESTING, do_rest, 0, 0, -1},
	{"открыть", POS_SITTING, do_gen_door, 0, SCMD_OPEN, 500},
	{"отпереть", POS_SITTING, do_gen_door, 0, SCMD_UNLOCK, 500},
	{"отпустить", POS_SITTING, do_stophorse, 0, 0, -1},
	{"отравить", POS_FIGHTING, do_poisoned, 0, 0, -1},
	{"отринуть", POS_RESTING, do_antigods, 1, 0, -1},
	{"отступить", POS_FIGHTING, do_stopfight, 1, 0, -1},
	{"отправить", POS_STANDING, do_not_here, 1, 0, -1},
	{"оффтоп", POS_DEAD, do_offtop, 0, 0, -1},
	{"ошеломить", POS_STANDING, do_stun, 1, 0, -1},
	{"оценить", POS_STANDING, do_not_here, 0, 0, 500},
	{"очки", POS_DEAD, do_score, 0, 0, 0},
	{"очепятки", POS_DEAD, Boards::DoBoard, 1, Boards::MISPRINT_BOARD, 0},
	{"очистить", POS_DEAD, do_not_here, 0, SCMD_CLEAR, -1},
	{"ошибк", POS_DEAD, do_quit, 0, 0, 0},
	{"ошибка", POS_DEAD, Boards::report_on_board, 0, Boards::ERROR_BOARD, 0},

	{"парировать", POS_FIGHTING, do_parry, 0, 0, -1},
	{"перехватить", POS_FIGHTING, do_touch, 0, 0, -1},
	{"перековать", POS_STANDING, do_transform_weapon, 0, SKILL_TRANSFORMWEAPON, -1},
	{"передать", POS_STANDING, do_givehorse, 0, 0, -1},
	{"перевести", POS_STANDING, do_not_here, 1, 0, -1},
	{"переместиться", POS_STANDING, do_relocate, 1, 0, 0},
	{"перевоплотитьс", POS_STANDING, do_remort, 0, 0, -1},
	{"перевоплотиться", POS_STANDING, do_remort, 0, 1, -1},
	{"перелить", POS_STANDING, do_pour, 0, SCMD_POUR, 500},
	{"перешить", POS_RESTING, do_fit, 0, SCMD_MAKE_OVER, 500},
	{"пить", POS_RESTING, do_drink, 0, SCMD_DRINK, 400},
	{"писать", POS_STANDING, do_write, 1, 0, -1},
	{"пклист", POS_SLEEPING, DoClanPkList, 0, 0, 0},
	{"пнуть", POS_FIGHTING, do_kick, 1, 0, -1},
	{"погода", POS_RESTING, do_weather, 0, 0, 0},
	{"подкрасться", POS_STANDING, do_sneak, 1, 0, 500},
	{"подножка", POS_FIGHTING, do_chopoff, 0, 0, 500},
	{"подняться", POS_RESTING, do_stand, 0, 0, -1},
	{"поджарить", POS_RESTING, do_fry, 0, 0, -1},
	{"перевязать", POS_RESTING, do_bandage, 0, 0, 0},
	{"переделать", POS_RESTING, do_fit, 0, SCMD_DO_ADAPT, 500},
	{"подсмотреть", POS_RESTING, do_look, 0, SCMD_LOOK_HIDE, 0},
	{"положить", POS_RESTING, do_put, 0, 0, 400},
	{"получить", POS_STANDING, do_not_here, 1, 0, -1},
	{"политика", POS_SLEEPING, DoShowPolitics, 0, 0, 0},
	{"помочь", POS_FIGHTING, do_assist, 1, 0, -1},
	{"помощь", POS_DEAD, do_help, 0, 0, 0},
	{"пометить", POS_DEAD, do_mark, LVL_IMPL, 0, 0},
	{"порез", POS_FIGHTING, do_expedient_cut, 0, 0, -1},
	{"поселиться", POS_STANDING, do_not_here, 1, 0, -1},
	{"постой", POS_STANDING, do_not_here, 1, 0, -1},
	{"почта", POS_STANDING, do_not_here, 1, 0, -1},
	{"пополнить", POS_STANDING, do_refill, 0, 0, 300},
	{"поручения", POS_RESTING, do_quest, 1, 0, -1},
	{"появиться", POS_RESTING, do_visible, 1, 0, -1},
	{"правила", POS_DEAD, do_gen_ps, 0, SCMD_POLICIES, 0},
	{"предложение", POS_STANDING, do_not_here, 1, 0, 500},
	//{"призвать", POS_STANDING, do_summon, 1, 0, -1},
	{"приказ", POS_RESTING, do_order, 1, 0, -1},
	{"привязать", POS_RESTING, do_horseput, 0, 0, 500},
	{"приглядеться", POS_RESTING, do_looking, 0, 0, 250},
	{"прикрыть", POS_FIGHTING, do_protect, 0, 0, -1},
	{"применить", POS_SITTING, do_use, 1, SCMD_USE, 400},
	//{"прикоснуться", POS_STANDING, do_touch_stigma, 0, 0, -1},
	{"присесть", POS_RESTING, do_sit, 0, 0, -1},
	{"прислушаться", POS_RESTING, do_hearing, 0, 0, 300},
	{"присмотреться", POS_RESTING, do_looking, 0, 0, 250},
	{"придумки", POS_DEAD, Boards::DoBoard, 0, Boards::SUGGEST_BOARD, 0},
	{"проверить", POS_DEAD, do_check, 0, 0, 0},
	{"проснуться", POS_SLEEPING, do_wake, 0, SCMD_WAKE, -1},
	{"простить", POS_RESTING, do_forgive, 0, 0, 0},
	{"пробовать", POS_RESTING, do_eat, 0, SCMD_TASTE, 300},
	{"сожрать", POS_RESTING, do_eat, 0, SCMD_DEVOUR, 300},
	{"продать", POS_STANDING, do_not_here, 0, 0, -1},
	{"фильтровать", POS_STANDING, do_not_here, 0, 0, -1},
	{"прыжок", POS_SLEEPING, do_goto, LVL_GOD, 0, 0},

	{"разбудить", POS_RESTING, do_wake, 0, SCMD_WAKEUP, -1},
	{"разгруппировать", POS_DEAD, do_ungroup, 0, 0, 500},
	{"разделить", POS_RESTING, do_split, 1, 0, 500},
	{"разделы", POS_RESTING, do_help, 1, 0, 500},
	{"разжечь", POS_STANDING, do_fire, 0, 0, -1},
	{"распустить", POS_DEAD, do_ungroup, 0, 0, 500},
	{"рассмотреть", POS_STANDING, do_not_here, 0, 0, -1},
	{"рассчитать", POS_RESTING, do_findhelpee, 0, SCMD_FREEHELPEE, -1},
	{"режим", POS_DEAD, do_mode, 0, 0, 0},
	{"ремонт", POS_RESTING, do_repair, 0, 0, -1},
	{"рецепты", POS_RESTING, do_recipes, 0, 0, 0},
	{"рекорды", POS_DEAD, DoBest, 0, 0, 0},
	{"руны", POS_FIGHTING, do_mixture, 0, SCMD_RUNES, -1},

	{"сбить", POS_FIGHTING, do_bash, 1, 0, -1},
	{"свойства", POS_STANDING, do_not_here, 0, 0, -1},
	{"сгруппа", POS_SLEEPING, do_gsay, 0, 0, -1},
	{"сглазить", POS_FIGHTING, do_manadrain, 0, 0, -1},
	{"сесть", POS_RESTING, do_sit, 0, 0, -1},
	{"синоним", POS_DEAD, do_alias, 0, 0, 0},
	{"сдемигодам", POS_DEAD, do_sdemigod, LVL_IMMORT, 0, 0},
	{"сказать", POS_RESTING, do_tell, 0, 0, -1},
	{"скользить", POS_STANDING, do_lightwalk, 0, 0, 0},
	{"следовать", POS_RESTING, do_follow, 0, 0, 500},
	{"сложить", POS_FIGHTING, do_mixture, 0, SCMD_RUNES, -1},
	{"слава", POS_STANDING, Glory::do_spend_glory, 0, 0, 0},
	{"слава2", POS_STANDING, GloryConst::do_spend_glory, 0, 0, 0},
	{"смотреть", POS_RESTING, do_look, 0, SCMD_LOOK, 0},
	{"смешать", POS_STANDING, do_mixture, 0, SCMD_ITEMS, -1},
//  { "смастерить",     POS_STANDING, do_transform_weapon, 0, SKILL_CREATEBOW, -1 },
	{"снять", POS_RESTING, do_remove, 0, 0, 500},
	{"создать", POS_SITTING, do_create, 0, 0, -1},
	{"сон", POS_SLEEPING, do_sleep, 0, 0, -1},
	{"соскочить", POS_FIGHTING, do_horseoff, 0, 0, -1},
	{"состав", POS_RESTING, do_create, 0, SCMD_RECIPE, 0},
	{"сохранить", POS_SLEEPING, do_save, 0, 0, 0},
	{"союзы", POS_RESTING, do_show_alliance, 0, 0, 0},
	{"социалы", POS_DEAD, do_commands, 0, SCMD_SOCIALS, 0},
	{"спать", POS_SLEEPING, do_sleep, 0, 0, -1},
	{"спасти", POS_FIGHTING, do_rescue, 1, 0, -1},
	{"способности", POS_SLEEPING, do_features, 0, 0, 0},
	{"список", POS_STANDING, do_not_here, 0, 0, -1},
	{"справка", POS_DEAD, do_help, 0, 0, 0},
	{"спросить", POS_RESTING, do_spec_comm, 0, SCMD_ASK, -1},
	{"спрятаться", POS_STANDING, do_hide, 1, 0, 500},
	{"сравнить", POS_RESTING, do_consider, 0, 0, 500},
	{"ставка", POS_STANDING, do_not_here, 0, 0, -1},
	{"статус", POS_DEAD, do_display, 0, 0, 0},
	{"статистика", POS_DEAD, do_statistic, 0, 0, 0},
	{"стереть", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, 0},
	{"стиль", POS_RESTING, do_style, 0, 0, 0},
	{"строка", POS_DEAD, do_display, 0, 0, 0},
	{"счет", POS_DEAD, do_score, 0, 0, 0},
	{"титул", POS_DEAD, TitleSystem::do_title, 0, 0, 0},
	{"трусость", POS_DEAD, do_wimpy, 0, 0, 0},
	{"убить", POS_FIGHTING, do_kill, 0, 0, -1},
	{"убрать", POS_RESTING, do_remove, 0, 0, 400},
	{"ударить", POS_FIGHTING, do_hit, 0, SCMD_HIT, -1},
	{"удавить", POS_FIGHTING, do_strangle, 0, 0, -1},
	{"удалить", POS_STANDING, do_delete_obj, LVL_IMPL, 0, 0 },
	{"уклониться", POS_FIGHTING, do_deviate, 1, 0, -1},
	{"украсть", POS_STANDING, do_steal, 1, 0, 0},
	{"укрепить", POS_RESTING, do_armored, 0, 0, -1},
	{"умения", POS_SLEEPING, do_skills, 0, 0, 0},
	{"уровень", POS_DEAD, do_score, 0, 0, 0},
	{"уровни", POS_DEAD, do_levels, 0, 0, 0},
	{"учить", POS_STANDING, do_not_here, 0, 0, -1},
	{"хранилище", POS_DEAD, DoStoreHouse, 0, 0, 0},
	{"характеристики", POS_STANDING, do_not_here, 0, 0, -1},
	{"кланстаф", POS_STANDING, do_clanstuff, 0, 0, 0},
	{"чинить", POS_STANDING, do_not_here, 0, 0, -1},
	{"читать", POS_RESTING, do_look, 0, SCMD_READ, 200},
	{"шептать", POS_RESTING, do_spec_comm, 0, SCMD_WHISPER, -1},
	{"экипировка", POS_SLEEPING, do_equipment, 0, 0, 0},
	{"эмоция", POS_RESTING, do_echo, 1, SCMD_EMOTE, -1},
	{"эхо", POS_SLEEPING, do_echo, LVL_IMMORT, SCMD_ECHO, -1},
	{"ярость", POS_RESTING, do_courage, 0, 0, -1},

	// God commands for listing
	{"мсписок", POS_DEAD, do_liblist, LVL_GOD, SCMD_MLIST, 0},
	{"осписок", POS_DEAD, do_liblist, LVL_GOD, SCMD_OLIST, 0},
	{"ксписок", POS_DEAD, do_liblist, LVL_GOD, SCMD_RLIST, 0},
	{"зсписок", POS_DEAD, do_liblist, LVL_GOD, SCMD_ZLIST, 0},
	{"исписок", POS_DEAD, do_imlist, LVL_IMPL, 0, 0},

	{"'", POS_RESTING, do_say, 0, 0, -1},
	{":", POS_RESTING, do_echo, 1, SCMD_EMOTE, -1},
	{";", POS_DEAD, do_wiznet, LVL_IMMORT, 0, -1},
	{"advance", POS_DEAD, do_advance, LVL_IMPL, 0, 0},
	{"alias", POS_DEAD, do_alias, 0, 0, 0},
	{"alter", POS_RESTING, do_fit, 0, SCMD_MAKE_OVER, 500},
	{"ask", POS_RESTING, do_spec_comm, 0, SCMD_ASK, -1},
	{"assist", POS_FIGHTING, do_assist, 1, 0, -1},
	{"attack", POS_FIGHTING, do_hit, 0, SCMD_MURDER, -1},
	{"auction", POS_RESTING, do_gen_comm, 0, SCMD_AUCTION, -1},
	{"arenarestore", POS_SLEEPING, do_arena_restore, LVL_GOD, 0, 0},
	{"backstab", POS_STANDING, do_backstab, 1, 0, 1},
	{"balance", POS_STANDING, do_not_here, 1, 0, -1},
	{"ban", POS_DEAD, do_ban, LVL_GRGOD, 0, 0},
	{"bash", POS_FIGHTING, do_bash, 1, 0, -1},
	{"beep", POS_DEAD, do_beep, LVL_IMMORT, 0, 0},
	{"block", POS_FIGHTING, do_block, 0, 0, -1},
	{"bug", POS_DEAD, Boards::report_on_board, 0, Boards::ERROR_BOARD, 0},
	{"buy", POS_STANDING, do_not_here, 0, 0, -1},
	{"best", POS_DEAD, DoBest, 0, 0, 0},
	{"cast", POS_SITTING, do_cast, 1, 0, -1},
	{"check", POS_STANDING, do_not_here, 1, 0, -1},
	{"chopoff", POS_FIGHTING, do_chopoff, 0, 0, 500},
	{"clear", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, 0},
	{"close", POS_SITTING, do_gen_door, 0, SCMD_CLOSE, 500},
	{"cls", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, 0},
	{"commands", POS_DEAD, do_commands, 0, SCMD_COMMANDS, 0},
	{"consider", POS_RESTING, do_consider, 0, 0, 500},
	{"credits", POS_DEAD, do_gen_ps, 0, SCMD_CREDITS, 0},
	{"date", POS_DEAD, do_date, LVL_IMMORT, SCMD_DATE, 0},
	{"dc", POS_DEAD, do_dc, LVL_GRGOD, 0, 0},
	{"deposit", POS_STANDING, do_not_here, 1, 0, 500},
	{"deviate", POS_FIGHTING, do_deviate, 0, 0, -1},
	{"diagnose", POS_RESTING, do_diagnose, 0, 0, 500},
	{"dig", POS_STANDING, do_dig, 0, SKILL_DIG, -1},
	{"disarm", POS_FIGHTING, do_disarm, 0, 0, -1},
	{"display", POS_DEAD, do_display, 0, 0, 0},
	{"drink", POS_RESTING, do_drink, 0, SCMD_DRINK, 500},
	{"drop", POS_RESTING, do_drop, 0, SCMD_DROP, 500},
	{"dumb", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_DUMB, 0},
	{"eat", POS_RESTING, do_eat, 0, SCMD_EAT, 500},
	{"devour", POS_RESTING, do_eat, 0, SCMD_DEVOUR, 300},
	{"echo", POS_SLEEPING, do_echo, LVL_IMMORT, SCMD_ECHO, 0},
	{"emote", POS_RESTING, do_echo, 1, SCMD_EMOTE, -1},
	{"enter", POS_STANDING, do_enter, 0, 0, -2},
	{"equipment", POS_SLEEPING, do_equipment, 0, 0, 0},
	{"examine", POS_RESTING, do_examine, 0, 0, 500},
//F@N|
	{"exchange", POS_RESTING, do_exchange, 1, 0, -1},
	{"exits", POS_RESTING, do_exits, 0, 0, 500},
	{"featset", POS_SLEEPING, do_featset, LVL_IMPL, 0, 0},
	{"features", POS_SLEEPING, do_features, 0, 0, 0},
	{"fill", POS_STANDING, do_pour, 0, SCMD_FILL, 500},
	{"fit", POS_RESTING, do_fit, 0, SCMD_DO_ADAPT, 500},
	{"flee", POS_FIGHTING, do_flee, 1, 0, -1},
	{"follow", POS_RESTING, do_follow, 0, 0, -1},
	{"force", POS_SLEEPING, do_force, LVL_GRGOD, 0, 0},
	{"forcetime", POS_DEAD, do_forcetime, LVL_IMPL, 0, 0},
	{"freeze", POS_DEAD, do_wizutil, LVL_FREEZE, SCMD_FREEZE, 0},
	{"gecho", POS_DEAD, do_gecho, LVL_GOD, 0, 0},
	{"get", POS_RESTING, do_get, 0, 0, 500},
	{"give", POS_RESTING, do_give, 0, 0, 500},
	{"godnews", POS_DEAD, Boards::DoBoard, 1, Boards::GODNEWS_BOARD, -1},
	{"gold", POS_RESTING, do_gold, 0, 0, 0},
	{"glide", POS_STANDING, do_lightwalk, 0, 0, 0},
	{"glory", POS_RESTING, GloryConst::do_glory, LVL_IMPL, 0, 0},
	{"glorytemp", POS_RESTING, do_glory, LVL_BUILDER, 0, 0},
	{"gossip", POS_RESTING, do_gen_comm, 0, SCMD_GOSSIP, -1},
	{"goto", POS_SLEEPING, do_goto, LVL_GOD, 0, 0},
	{"grab", POS_RESTING, do_grab, 0, 0, 500},
	{"group", POS_RESTING, do_group, 1, 0, 500},
	{"gsay", POS_SLEEPING, do_gsay, 0, 0, -1},
	{"gtell", POS_SLEEPING, do_gsay, 0, 0, -1},
	{"handbook", POS_DEAD, do_gen_ps, LVL_IMMORT, SCMD_HANDBOOK, 0},
	{"hcontrol", POS_DEAD, DoHcontrol, LVL_GRGOD, 0, 0},
	{"help", POS_DEAD, do_help, 0, 0, 0},
	{"hell", POS_DEAD, do_wizutil, LVL_GOD, SCMD_HELL, 0},
	{"hide", POS_STANDING, do_hide, 1, 0, 0},
	{"hit", POS_FIGHTING, do_hit, 0, SCMD_HIT, -1},
	{"hold", POS_RESTING, do_grab, 1, 0, 500},
	{"holler", POS_RESTING, do_gen_comm, 1, SCMD_HOLLER, -1},
	{"horse", POS_STANDING, do_not_here, 0, 0, -1},
	{"house", POS_RESTING, DoHouse, 0, 0, 0},
	{"huk", POS_FIGHTING, do_mighthit, 0, 0, -1},
	{"idea", POS_DEAD, Boards::DoBoard, 1, Boards::IDEA_BOARD, 0},
	{"ignore", POS_DEAD, do_ignore, 0, 0, 0},
	{"immlist", POS_DEAD, do_gen_ps, 0, SCMD_IMMLIST, 0},
	{"index", POS_RESTING, do_help, 1, 0, 500},
	{"info", POS_SLEEPING, do_gen_ps, 0, SCMD_INFO, 0},
	{"insert", POS_STANDING, do_insertgem, 0, SKILL_INSERTGEM, -1},
	{"inspect", POS_DEAD, do_inspect, LVL_BUILDER, 0, 0},
	{"insult", POS_RESTING, do_insult, 0, 0, -1},
	{"inventory", POS_SLEEPING, do_inventory, 0, 0, 0},
	{"invis", POS_DEAD, do_invis, LVL_GOD, 0, -1},
	{"kick", POS_FIGHTING, do_kick, 1, 0, -1},
	{"kill", POS_FIGHTING, do_kill, 0, 0, -1},
	{"last", POS_DEAD, do_last, LVL_GOD, 0, 0},
	{"levels", POS_DEAD, do_levels, 0, 0, 0},
	{"list", POS_STANDING, do_not_here, 0, 0, -1},
	{"load", POS_DEAD, do_load, LVL_BUILDER, 0, 0},
	{"loadstat", POS_DEAD, do_loadstat, LVL_IMPL, 0, 0 },
	{"look", POS_RESTING, do_look, 0, SCMD_LOOK, 200},
	{"lock", POS_SITTING, do_gen_door, 0, SCMD_LOCK, 500},
	{"map", POS_RESTING, do_map, 0, 0, 0},
	{"mail", POS_STANDING, do_not_here, 1, 0, -1},
	{"mode", POS_DEAD, do_mode, 0, 0, 0},
	{"mshout", POS_RESTING, do_mobshout, 0, 0, -1},
	{"motd", POS_DEAD, do_gen_ps, 0, SCMD_MOTD, 0},
	{"murder", POS_FIGHTING, do_hit, 0, SCMD_MURDER, -1},
	{"mute", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_MUTE, 0},
	{"medit", POS_DEAD, do_olc, 0, SCMD_OLC_MEDIT, 0},
	{"name", POS_DEAD, do_wizutil, LVL_GOD, SCMD_NAME, 0},
	{"nedit", POS_RESTING, NamedStuff::do_named, LVL_BUILDER, SCMD_NAMED_EDIT, 0}, //Именной стаф редактирование
	{"news", POS_DEAD, Boards::DoBoard, 1, Boards::NEWS_BOARD, -1},
	{"nlist", POS_RESTING, NamedStuff::do_named, LVL_BUILDER, SCMD_NAMED_LIST, 0}, //Именной стаф список
	{"notitle", POS_DEAD, do_wizutil, LVL_GRGOD, SCMD_NOTITLE, 0},
	{"odelete", POS_STANDING, do_delete_obj, LVL_IMPL, 0, 0 },
	{"oedit", POS_DEAD, do_olc, 0, SCMD_OLC_OEDIT, 0},
	{"offer", POS_STANDING, do_not_here, 1, 0, 0},
	{"olc", POS_DEAD, do_olc, LVL_GOD, SCMD_OLC_SAVEINFO, 0},
	{"open", POS_SITTING, do_gen_door, 0, SCMD_OPEN, 500},
	{"order", POS_RESTING, do_order, 1, 0, -1},
	{"overstuff", POS_DEAD, do_overstuff, LVL_GRGOD, 0, 0 },
	{"page", POS_DEAD, do_page, LVL_GOD, 0, 0},
	{"parry", POS_FIGHTING, do_parry, 0, 0, -1},
	{"pick", POS_STANDING, do_gen_door, 1, SCMD_PICK, -1},
	{"poisoned", POS_FIGHTING, do_poisoned, 0, 0, -1},
	{"policy", POS_DEAD, do_gen_ps, 0, SCMD_POLICIES, 0},
	{"poofin", POS_DEAD, do_poofset, LVL_GOD, SCMD_POOFIN, 0},
	{"poofout", POS_DEAD, do_poofset, LVL_GOD, SCMD_POOFOUT, 0},
	{"pour", POS_STANDING, do_pour, 0, SCMD_POUR, -1},
	{"practice", POS_STANDING, do_not_here, 0, 0, -1},
	{"prompt", POS_DEAD, do_display, 0, 0, 0},
	{"proxy", POS_DEAD, do_proxy, LVL_GRGOD, 0, 0},
	{"purge", POS_DEAD, do_purge, LVL_GOD, 0, 0},
	{"put", POS_RESTING, do_put, 0, 0, 500},
//	{"python", POS_DEAD, do_console, LVL_GOD, 0, 0},
	{"quaff", POS_RESTING, do_use, 0, SCMD_QUAFF, 500},
	{"qui", POS_SLEEPING, do_quit, 0, 0, 0},
	{"quit", POS_SLEEPING, do_quit, 0, SCMD_QUIT, -1},
	{"read", POS_RESTING, do_look, 0, SCMD_READ, 200},
	{"receive", POS_STANDING, do_not_here, 1, 0, -1},
	{"recipes", POS_RESTING, do_recipes, 0, 0, 0},
	{"recite", POS_RESTING, do_use, 0, SCMD_RECITE, 500},
	{"redit", POS_DEAD, do_olc, 0, SCMD_OLC_REDIT, 0},
	{"register", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_REGISTER, 0},
	{"unregister", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_UNREGISTER, 0},
	{"reload", POS_DEAD, do_reboot, LVL_IMPL, 0, 0},
	{"remove", POS_RESTING, do_remove, 0, 0, 500},
	{"rent", POS_STANDING, do_not_here, 1, 0, -1},
	{"reply", POS_RESTING, do_reply, 0, 0, -1},
	{"report", POS_RESTING, do_report, 0, 0, -1},
	{"reroll", POS_DEAD, do_wizutil, LVL_GRGOD, SCMD_REROLL, 0},
	{"rescue", POS_FIGHTING, do_rescue, 1, 0, -1},
	{"rest", POS_RESTING, do_rest, 0, 0, -1},
	{"restore", POS_DEAD, do_restore, LVL_GRGOD, SCMD_RESTORE_GOD, 0},
	{"return", POS_DEAD, do_return, 0, 0, -1},
	{"rset", POS_SLEEPING, do_rset, LVL_BUILDER, 0, 0},
	{"rules", POS_DEAD, do_gen_ps, LVL_IMMORT, SCMD_RULES, 0},
	{"runes", POS_FIGHTING, do_mixture, 0, SCMD_RUNES, -1},
	{"save", POS_SLEEPING, do_save, 0, 0, 0},
	{"say", POS_RESTING, do_say, 0, 0, -1},
	{"scan", POS_RESTING, do_sides, 0, 0, 500},
	{"score", POS_DEAD, do_score, 0, 0, 0},
	{"sell", POS_STANDING, do_not_here, 0, 0, -1},
	{"send", POS_SLEEPING, do_send, LVL_GRGOD, 0, 0},
	{"sense", POS_STANDING, do_sense, 0, 0, 500},
	{"set", POS_DEAD, do_set, LVL_IMMORT, 0, 0},
	{"settle", POS_STANDING, do_not_here, 1, 0, -1},
	{"shout", POS_RESTING, do_gen_comm, 0, SCMD_SHOUT, -1},
	{"show", POS_DEAD, do_show, LVL_IMMORT, 0, 0},
	
	{"shutdown", POS_DEAD, do_shutdown, LVL_IMPL, SCMD_SHUTDOWN, 0},
	{"sip", POS_RESTING, do_drink, 0, SCMD_SIP, 500},
	{"sit", POS_RESTING, do_sit, 0, 0, -1},
	{"skills", POS_RESTING, do_skills, 0, 0, 0},
	{"skillset", POS_SLEEPING, do_skillset, LVL_IMPL, 0, 0},
	{"morphset", POS_SLEEPING, do_morphset, LVL_IMPL, 0, 0},
	{"setall", POS_DEAD, do_setall, LVL_IMPL, 0, 0},
	{"sleep", POS_SLEEPING, do_sleep, 0, 0, -1},
	{"sneak", POS_STANDING, do_sneak, 1, 0, -2},
	{"snoop", POS_DEAD, do_snoop, LVL_GRGOD, 0, 0},
	{"socials", POS_DEAD, do_commands, 0, SCMD_SOCIALS, 0},
	{"spells", POS_RESTING, do_spells, 0, 0, 0},
	{"split", POS_RESTING, do_split, 1, 0, 0},
	{"stand", POS_RESTING, do_stand, 0, 0, -1},
	{"stat", POS_DEAD, do_stat, LVL_GOD, 0, 0},
	{"steal", POS_STANDING, do_steal, 1, 0, 300},
	{"strangle", POS_FIGHTING, do_strangle, 0, 0, -1},
	{"stupor", POS_FIGHTING, do_stupor, 0, 0, -1},
	{"switch", POS_DEAD, do_switch, LVL_GRGOD, 0, 0},
	{"syslog", POS_DEAD, do_syslog, LVL_IMMORT, SYSLOG, 0},
	{"suggest", POS_DEAD, Boards::report_on_board, 0, Boards::SUGGEST_BOARD, 0},
	{"slist", POS_DEAD, do_slist, LVL_IMPL, 0, 0},
	{"sedit", POS_DEAD, do_sedit, LVL_IMPL, 0, 0},
	{"errlog", POS_DEAD, do_syslog, LVL_BUILDER, ERRLOG, 0},
	{"imlog", POS_DEAD, do_syslog, LVL_BUILDER, IMLOG, 0},
	{"take", POS_RESTING, do_get, 0, 0, 500},
	{"taste", POS_RESTING, do_eat, 0, SCMD_TASTE, 500},
	{"t2c", POS_RESTING, do_send_text_to_char, LVL_GRGOD, 0, -1 },
	{"teleport", POS_DEAD, do_teleport, LVL_GRGOD, 0, -1},
	{"tell", POS_RESTING, do_tell, 0, 0, -1},
	{"time", POS_DEAD, do_time, 0, 0, 0},
	{"title", POS_DEAD, TitleSystem::do_title, 0, 0, 0},
	{"touch", POS_FIGHTING, do_touch, 0, 0, -1},
	{"track", POS_STANDING, do_track, 0, 0, -1},
	{"transfer", POS_STANDING, do_not_here, 1, 0, -1},
	{"trigedit", POS_DEAD, do_olc, 0, SCMD_OLC_TRIGEDIT, 0},
	{"turn undead", POS_RESTING, do_turn_undead, 0, 0, -1},
	{"typo", POS_DEAD, Boards::report_on_board, 0, Boards::MISPRINT_BOARD, 0},
	{"unaffect", POS_DEAD, do_wizutil, LVL_GRGOD, SCMD_UNAFFECT, 0},
	{"unban", POS_DEAD, do_unban, LVL_GRGOD, 0, 0},
	{"unfreeze", POS_DEAD, do_unfreeze, LVL_IMPL, 0, 0},
	{"ungroup", POS_DEAD, do_ungroup, 0, 0, -1},
	{"unlock", POS_SITTING, do_gen_door, 0, SCMD_UNLOCK, 500},
	{"uptime", POS_DEAD, do_date, LVL_IMMORT, SCMD_UPTIME, 0},
	{"use", POS_SITTING, do_use, 1, SCMD_USE, 500},
	{"users", POS_DEAD, do_users, LVL_IMMORT, 0, 0},
	{"value", POS_STANDING, do_not_here, 0, 0, -1},
	{"version", POS_DEAD, do_gen_ps, 0, SCMD_VERSION, 0},
	{"visible", POS_RESTING, do_visible, 1, 0, -1},
	{"vnum", POS_DEAD, do_vnum, LVL_GRGOD, 0, 0},
	{"вномер", POS_DEAD, do_vnum, LVL_GRGOD, 0, 0},  //тупой копипаст для использования русского синтаксиса
	{"vstat", POS_DEAD, do_vstat, LVL_GRGOD, 0, 0},
	{"wake", POS_SLEEPING, do_wake, 0, 0, -1},
	{"warcry", POS_FIGHTING, do_warcry, 1, 0, -1},
	{"wear", POS_RESTING, do_wear, 0, 0, 500},
	{"weather", POS_RESTING, do_weather, 0, 0, 0},
	{"where", POS_RESTING, do_where, LVL_IMMORT, 0, 0},
	{"whirl", POS_FIGHTING, do_iron_wind, 0, 0, -1},
	{"whisper", POS_RESTING, do_spec_comm, 0, SCMD_WHISPER, -1},
	{"who", POS_RESTING, do_who, 0, 0, 0},
	{"whoami", POS_DEAD, do_gen_ps, 0, SCMD_WHOAMI, 0},
	{"wield", POS_RESTING, do_wield, 0, 0, 500},
	{"wimpy", POS_DEAD, do_wimpy, 0, 0, 0},
	{"withdraw", POS_STANDING, do_not_here, 1, 0, -1},
	{"wizhelp", POS_SLEEPING, do_commands, LVL_IMMORT, SCMD_WIZHELP, 0},
	{"wizlock", POS_DEAD, do_wizlock, LVL_IMPL, 0, 0},
	{"wiznet", POS_DEAD, do_wiznet, LVL_IMMORT, 0, 0},
	{"wizat", POS_DEAD, do_at, LVL_GRGOD, 0, 0},
	{"write", POS_STANDING, do_write, 1, 0, -1},
	{"zedit", POS_DEAD, do_olc, 0, SCMD_OLC_ZEDIT, 0},
	{"zone", POS_RESTING, do_zone, 0, 0, 0},
	{"zreset", POS_DEAD, do_zreset, LVL_GRGOD, 0, 0},

	// test command for gods
	{"godtest", POS_DEAD, do_godtest, LVL_IMPL, 0, 0},
	{"armor", POS_DEAD, do_print_armor, LVL_IMPL, 0, 0},

	// Команды крафтинга - для тестига пока уровня имма
	{"mrlist", POS_DEAD, do_list_make, LVL_BUILDER, 0, 0},
	{"mredit", POS_DEAD, do_edit_make, LVL_BUILDER, 0, 0},
	{"сшить", POS_STANDING, do_make_item, 0, MAKE_WEAR, 0},
	{"выковать", POS_STANDING, do_make_item, 0, MAKE_METALL, 0},
	{"смастерить", POS_STANDING, do_make_item, 0, MAKE_CRAFT, 0},

	// God commands for listing
	{"mlist", POS_DEAD, do_liblist, LVL_GOD, SCMD_MLIST, 0},
	{"olist", POS_DEAD, do_liblist, LVL_GOD, SCMD_OLIST, 0},
	{"rlist", POS_DEAD, do_liblist, LVL_GOD, SCMD_RLIST, 0},
	{"zlist", POS_DEAD, do_liblist, LVL_GOD, SCMD_ZLIST, 0},
	{"clist", POS_DEAD, do_liblist, LVL_GOD, SCMD_CLIST, 0},

	{ "attach", POS_DEAD, do_attach, LVL_IMPL, 0, 0 },
	{ "detach", POS_DEAD, do_detach, LVL_IMPL, 0, 0 },
	{ "tlist", POS_DEAD, do_tlist, LVL_GRGOD, 0, 0 },
	{ "tstat", POS_DEAD, do_tstat, LVL_GRGOD, 0, 0 },
	{ "vdelete", POS_DEAD, do_vdelete, LVL_IMPL, 0, 0 },
	{ "debug_queues", POS_DEAD, do_debug_queues, LVL_IMPL, 0, 0 },

	{ heartbeat::cmd::HEARTBEAT_COMMAND, heartbeat::cmd::MINIMAL_POSITION, heartbeat::cmd::do_heartbeat, heartbeat::cmd::MINIMAL_LEVEL, heartbeat::SCMD_NOTHING, heartbeat::cmd::UNHIDE_PROBABILITY },
	//{craft::cmd::CRAFT_COMMAND, craft::cmd::MINIMAL_POSITION, craft::cmd::do_craft, craft::cmd::MINIMAL_LEVEL, craft::SCMD_NOTHING, craft::cmd::UNHIDE_PROBABILITY},
	{"\n", 0, 0, 0, 0, 0}
};				// this must be last

const char *dir_fill[] = { "in",
						   "from",
						   "with",
						   "the",
						   "on",
						   "at",
						   "to",
						   "\n"
						 };

const char *reserved[] = { "a",
						   "an",
						   "self",
						   "me",
						   "all",
						   "room",
						   "someone",
						   "something",
						   "\n"
						 };

void check_hiding_cmd(CHAR_DATA * ch, int percent)
{
	int remove_hide = FALSE;
	if (affected_by_spell(ch, SPELL_HIDE))
	{
		if (percent == -2)
		{
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK))
			{
				remove_hide = number(1, skill_info[SKILL_SNEAK].max_percent) >
					calculate_skill(ch, SKILL_SNEAK, 0);
			}
			else
			{
				percent = 500;
			}
		}

		if (percent == -1)
		{
			remove_hide = TRUE;
		}
		else if (percent > 0)
		{
			remove_hide = number(1, percent) > calculate_skill(ch, SKILL_HIDE, 0);
		}

		if (remove_hide)
		{
			affect_from_char(ch, SPELL_HIDE);
			if (!AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE))
			{
				send_to_char("Вы прекратили прятаться.\r\n", ch);
				act("$n прекратил$g прятаться.", FALSE, ch, 0, 0, TO_ROOM);
			}
		}
	}
}

bool check_frozen_cmd(CHAR_DATA* /*ch*/, int cmd)
{
	if (!strcmp(cmd_info[cmd].command, "предложение")
		|| !strcmp(cmd_info[cmd].command, "offer")
		|| !strcmp(cmd_info[cmd].command, "постой")
		|| !strcmp(cmd_info[cmd].command, "rent")
		|| !strcmp(cmd_info[cmd].command, "поселиться")
		|| !strcmp(cmd_info[cmd].command, "settle")
		|| !strcmp(cmd_info[cmd].command, "ктоя")
		|| !strcmp(cmd_info[cmd].command, "whoami")
		|| !strcmp(cmd_info[cmd].command, "справка")
		|| !strcmp(cmd_info[cmd].command, "help")
		|| !strcmp(cmd_info[cmd].command, "помощь")
		|| !strcmp(cmd_info[cmd].command, "разделы")
		|| !strcmp(cmd_info[cmd].command, "очки")
		|| !strcmp(cmd_info[cmd].command, "счет")
		|| !strcmp(cmd_info[cmd].command, "уровень")
		|| !strcmp(cmd_info[cmd].command, "score"))
	{
		return true;
	}
	return false;
}

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(CHAR_DATA * ch, char *argument)
{
	int cmd, social = FALSE, hardcopy = FALSE;
	char *line;

	// just drop to next line for hitting CR
	CHECK_AGRO(ch) = 0;
	skip_spaces(&argument);

	if (!*argument)
		return;

	if (!IS_NPC(ch))
	{
		log("<%s, %d> {%5d} [%s]", GET_NAME(ch), GlobalObjects::heartbeat().pulse_number(), GET_ROOM_VNUM(ch->in_room), argument);
		if (GET_LEVEL(ch) >= LVL_IMMORT || GET_GOD_FLAG(ch, GF_PERSLOG) || GET_GOD_FLAG(ch, GF_DEMIGOD))
			pers_log(ch, "<%s> {%5d} [%s]", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), argument);
	}

	//Polud спешиал для спешиалов добавим обработку числового префикса перед именем команды

	int fnum = get_number(&argument);

	/*
	 * special case to handle one-character, non-alphanumeric commands;
	 * requested by many people so "'hi" or ";godnet test" is possible.
	 * Patch sent by Eric Green and Stefan Wasilewski.
	 */
	if (!a_isalpha(*argument))
	{
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
	}
	else
	{
		line = any_one_arg(argument, arg);
	}

	const size_t length = strlen(arg);
	if (1 < length && *(arg + length - 1) == '!')
	{
		hardcopy = TRUE;
		*(arg + length - 1) = '\0';
		*(argument + length - 1) = ' ';
	}

        if (!IS_NPC(ch)
                && !GET_INVIS_LEV(ch)
                && !GET_MOB_HOLD(ch)
                && !AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
                && !AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)
                && !(IS_GOD(ch) && !strcmp(arg, "invis")))  // let immortals switch to wizinvis to avoid broken command triggers
	{
		int cont;	// continue the command checks
		cont = command_wtrigger(ch, arg, line);
		if (!cont)
			cont = command_mtrigger(ch, arg, line);
		if (!cont)
			cont = command_otrigger(ch, arg, line);
		if (cont)
		{
			check_hiding_cmd(ch, -1);
			return;	// command trigger took over
		}
	}

#if defined WITH_SCRIPTING
	// Try scripting
	if (scripting::execute_player_command(ch, arg, line))
		return;
#endif

	// otherwise, find the command
	for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
	{
		if (hardcopy)
		{
			if (!strcmp(cmd_info[cmd].command, arg))
				if (Privilege::can_do_priv(ch, std::string(cmd_info[cmd].command), cmd, 0))
					break;
		}
		else
		{
			if (!strncmp(cmd_info[cmd].command, arg, length))
				if (Privilege::can_do_priv(ch, std::string(cmd_info[cmd].command), cmd, 0))
					break;
		}
	}

	if (*cmd_info[cmd].command == '\n')
	{
		if (find_action(arg) >= 0)
			social = TRUE;
		else
		{
			send_to_char("Чаво?\r\n", ch);
			return;
		}
	}

	if (((!IS_NPC(ch)
				&& (GET_FREEZE_LEV(ch) > GET_LEVEL(ch))
				&& (PLR_FLAGGED(ch, PLR_FROZEN)))
			|| GET_MOB_HOLD(ch)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT))
		&& !check_frozen_cmd(ch, cmd))
	{
		send_to_char("Вы попытались, но не смогли сдвинуться с места...\r\n", ch);
		return;
	}

	if (!social && cmd_info[cmd].command_pointer == NULL)
	{
		send_to_char("Извините, не смог разобрать команду.\r\n", ch);
		return;
	}

	if (!social && IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
	{
		send_to_char("Вы еще не БОГ, чтобы делать это.\r\n", ch);
		return;
	}

	if (!social && GET_POS(ch) < cmd_info[cmd].minimum_position)
	{
		switch (GET_POS(ch))
		{
		case POS_DEAD:
			send_to_char("Очень жаль - ВЫ МЕРТВЫ!!! :-(\r\n", ch);
			break;
		case POS_INCAP:
		case POS_MORTALLYW:
			send_to_char("Вы в критическом состоянии и не можете ничего делать!\r\n", ch);
			break;
		case POS_STUNNED:
			send_to_char("Вы слишком слабы, чтобы сделать это!\r\n", ch);
			break;
		case POS_SLEEPING:
			send_to_char("Сделать это в ваших снах?\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("Нет... Вы слишком расслаблены...\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("Пожалуй, вам лучше встать на ноги.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Ни за что! Вы сражаетесь за свою жизнь!\r\n", ch);
			break;
		default:
			send_to_char("Вы не в том состоянии, чтобы это сделать...\r\n", ch);
			break;
		}
		return;
	}
	if (social)
	{
		check_hiding_cmd(ch, -1);
		do_social(ch, argument);
	}
	else if (no_specials || !special(ch, cmd, line, fnum))
	{
		check_hiding_cmd(ch, cmd_info[cmd].unhide_percent);
		(*cmd_info[cmd].command_pointer)(ch, line, cmd, cmd_info[cmd].subcmd);
		if (ch->purged())
		{
			return;
		}
		if (!IS_NPC(ch) && ch->in_room != NOWHERE && CHECK_AGRO(ch))
		{
			CHECK_AGRO(ch) = FALSE;
			do_aggressive_room(ch, FALSE);
			if (ch->purged())
			{
				return;
			}
		}
	}
}

// ************************************************************************
// * Routines to handle aliasing                                          *
// ************************************************************************
struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
	while (alias_list != NULL)
	{
		if (*str == *alias_list->alias)	// hey, every little bit counts :-)
			if (!strcmp(str, alias_list->alias))
				return (alias_list);

		alias_list = alias_list->next;
	}

	return (NULL);
}

void free_alias(struct alias_data *a)
{
	if (a->alias)
		free(a->alias);
	if (a->replacement)
		free(a->replacement);
	free(a);
}

// The interface to the outside world: do_alias
void do_alias(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char *repl;
	struct alias_data *a;

	if (IS_NPC(ch))
		return;

	repl = any_one_arg(argument, arg);

	if (!*arg)  		// no argument specified -- list currently defined aliases
	{
		send_to_char("Определены следующие алиасы:\r\n", ch);
		if ((a = GET_ALIASES(ch)) == NULL)
			send_to_char(" Нет алиасов.\r\n", ch);
		else
		{
			while (a != NULL)
			{
				sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
				send_to_char(buf, ch);
				a = a->next;
			}
		}
	}
	else  		// otherwise, add or remove aliases
	{
		// is this an alias we've already defined?
		if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL)
		{
			REMOVE_FROM_LIST(a, GET_ALIASES(ch));
			free_alias(a);
		}
		// if no replacement string is specified, assume we want to delete
		if (!*repl)
		{
			if (a == NULL)
				send_to_char("Такой алиас не определен.\r\n", ch);
			else
				send_to_char("Алиас успешно удален.\r\n", ch);
		}
		else  	// otherwise, either add or redefine an alias
		{
			if (!str_cmp(arg, "alias"))
			{
				send_to_char("Вы не можете определить алиас 'alias'.\r\n", ch);
				return;
			}
			CREATE(a, 1);
			a->alias = str_dup(arg);
			delete_doubledollar(repl);
			a->replacement = str_dup(repl);
			if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
				a->type = ALIAS_COMPLEX;
			else
				a->type = ALIAS_SIMPLE;
			a->next = GET_ALIASES(ch);
			GET_ALIASES(ch) = a;
			send_to_char("Алиас успешно добавлен.\r\n", ch);
		}
	}
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
	struct txt_q temp_queue;
	char *tokens[NUM_TOKENS], *temp, *write_point;
	int num_of_tokens = 0, num;

	// First, parse the original string
	temp = strtok(strcpy(buf2, orig), " ");
	while (temp != NULL && num_of_tokens < NUM_TOKENS)
	{
		tokens[num_of_tokens++] = temp;
		temp = strtok(NULL, " ");
	}

	// initialize
	write_point = buf;
	temp_queue.head = temp_queue.tail = NULL;

	// now parse the alias
	for (temp = a->replacement; *temp; temp++)
	{
		if (*temp == ALIAS_SEP_CHAR)
		{
			*write_point = '\0';
			buf[MAX_INPUT_LENGTH - 1] = '\0';
			write_to_q(buf, &temp_queue, 1);
			write_point = buf;
		}
		else if (*temp == ALIAS_VAR_CHAR)
		{
			temp++;
			if ((num = *temp - '1') < num_of_tokens && num >= 0)
			{
				strcpy(write_point, tokens[num]);
				write_point += strlen(tokens[num]);
			}
			else if (*temp == ALIAS_GLOB_CHAR)
			{
				strcpy(write_point, orig);
				write_point += strlen(orig);
			}
			else if ((*(write_point++) = *temp) == '$')	// redouble $ for act safety
				*(write_point++) = '$';
		}
		else
			*(write_point++) = *temp;
	}

	*write_point = '\0';
	buf[MAX_INPUT_LENGTH - 1] = '\0';
	write_to_q(buf, &temp_queue, 1);

	// push our temp_queue on to the _front_ of the input queue
	if (input_q->head == NULL)
		*input_q = temp_queue;
	else
	{
		temp_queue.tail->next = input_q->head;
		input_q->head = temp_queue.head;
	}
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(DESCRIPTOR_DATA * d, char *orig)
{
	char first_arg[MAX_INPUT_LENGTH], *ptr;
	struct alias_data *a, *tmp;

	// Mobs don't have alaises. //
	if (IS_NPC(d->character))
		return (0);

	// bail out immediately if the guy doesn't have any aliases //
	if ((tmp = GET_ALIASES(d->character)) == NULL)
		return (0);

	// find the alias we're supposed to match //
	ptr = any_one_arg(orig, first_arg);

	// bail out if it's null //
	if (!*first_arg)
		return (0);

	// if the first arg is not an alias, return without doing anything //
	if ((a = find_alias(tmp, first_arg)) == NULL)
		return (0);

	if (a->type == ALIAS_SIMPLE)
	{
		strcpy(orig, a->replacement);
		return (0);
	}
	else
	{
		perform_complex_alias(&d->input, ptr, a);
		return (1);
	}
}



// ***************************************************************************
// * Various other parsing utilities                                         *
// ***************************************************************************

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(const char *arg, const char **list, int exact)
{
	int i;
	size_t l = strlen(arg);

	if (exact)
	{
		for (i = 0; **(list + i) != '\n'; i++)
		{
			if (!str_cmp(arg, *(list + i)))
			{
				return i;
			}
		}
	}
	else
	{
		if (0 == l)
		{
			l = 1;	// Avoid "" to match the first available string
		}
		for (i = 0; **(list + i) != '\n'; i++)
		{
			if (!strn_cmp(arg, *(list + i), l))
			{
				return i;
			}
		}
	}

	return -1;
}

int search_block(const std::string &arg, const char **list, int exact)
{
	int i;
	std::string::size_type l = arg.length();

	if (exact)
	{
		for (i = 0; **(list + i) != '\n'; i++)
			if (!str_cmp(arg, *(list + i)))
				return (i);
	}
	else
	{
		if (!l)
			l = 1;	// Avoid "" to match the first available string
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strn_cmp(arg, *(list + i), l))
				return (i);
	}

	return (-1);
}

int is_number(const char *str)
{
	while (*str)
	{
		if (!a_isdigit(*(str++)))
		{
			return 0;
		}
	}

	return 1;
}

/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
	char *read, *write;

	// If the string has no dollar signs, return immediately //
	if ((write = strchr(string, '$')) == NULL)
		return (string);

	// Start from the location of the first dollar sign //
	read = write;


	while (*read)		// Until we reach the end of the string... //
		if ((*(write++) = *(read++)) == '$')	// copy one char //
			if (*read == '$')
				read++;	// skip if we saw 2 $'s in a row //

	*write = '\0';

	return (string);
}


int fill_word(const char *argument)
{
	return (search_block(argument, dir_fill, TRUE) >= 0);
}


int reserved_word(const char *argument)
{
	return (search_block(argument, reserved, TRUE) >= 0);
}

int is_abbrev(const char *arg1, const char *arg2)
{
	if (!*arg1)
		return (0);

	for (; *arg1 && *arg2; arg1++, arg2++)
		if (LOWER(*arg1) != LOWER(*arg2))
			return (0);

	if (!*arg1)
		return (1);
	else
		return (0);
}

template <typename T>
T one_argument_template(T argument, char *first_arg)
{
	char *begin = first_arg;

	if (!argument)
	{
		log("SYSERR: one_argument received a NULL pointer!");
		*first_arg = '\0';
		return (NULL);
	}

	do
	{
		skip_spaces(&argument);

		first_arg = begin;
		while (*argument && !a_isspace(*argument))
		{
			*(first_arg++) = a_lcc(*argument);
			argument++;
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	return (argument);
}

template <typename T>
T any_one_arg_template(T argument, char* first_arg)
{
	if (!argument)
	{
		log("SYSERR: any_one_arg() passed a NULL pointer.");
		return 0;
	}
	skip_spaces(&argument);

	unsigned num = 0;
	while (*argument && !a_isspace(*argument) && num < MAX_INPUT_LENGTH - 1)
	{
		*first_arg = a_lcc(*argument);
		++first_arg;
		++argument;
		++num;
	}
	*first_arg = '\0';

	return argument;
}

char* one_argument(char* argument, char *first_arg) { return one_argument_template(argument, first_arg); }
const char* one_argument(const char* argument, char *first_arg) { return one_argument_template(argument, first_arg); }
char* any_one_arg(char* argument, char* first_arg) { return any_one_arg_template(argument, first_arg); }
const char* any_one_arg(const char* argument, char* first_arg) { return any_one_arg_template(argument, first_arg); }

// return first space-delimited token in arg1; remainder of string in arg2 //
void half_chop(const char* string, char* arg1, char* arg2)
{
	const char* temp = any_one_arg_template(string, arg1);
	skip_spaces(&temp);
	strl_cpy(arg2, temp, MAX_INPUT_LENGTH);
}

// Used in specprocs, mostly.  (Exactly) matches "command" to cmd number //
int find_command(const char *command)
{
	int cmd;

	for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
		if (!strcmp(cmd_info[cmd].command, command))
			return (cmd);

	return (-1);
}

// int fnum - номер найденного в комнате спешиал-моба, для обработки нескольких спешиал-мобов в одной комнате //
int special(CHAR_DATA * ch, int cmd, char *arg, int fnum)
{
	if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE))
	{
		const auto clan = Clan::GetClanByRoom(ch->in_room);
		if (!clan)
		{
			return 0;
		}
	}

	OBJ_DATA *i;
	int j;

	// special in room? //
	if (GET_ROOM_SPEC(ch->in_room) != NULL)
	{
		if (GET_ROOM_SPEC(ch->in_room)(ch, world[ch->in_room], cmd, arg))
		{
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in equipment list? //
	for (j = 0; j < NUM_WEARS; j++)
	{
		if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
		{
			if (GET_OBJ_SPEC(GET_EQ(ch, j))(ch, GET_EQ(ch, j), cmd, arg))
			{
				check_hiding_cmd(ch, -1);
				return (1);
			}
		}
	}

	// special in inventory? //
	for (i = ch->carrying; i; i = i->get_next_content())
	{
		if (GET_OBJ_SPEC(i) != NULL
			&& GET_OBJ_SPEC(i)(ch, i, cmd, arg))
		{
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in mobile present? //
//Polud чтобы продавцы не мешали друг другу в одной комнате, предусмотрим возможность различать их по номеру
	int specialNum = 1; //если номер не указан - по умолчанию берется первый
	for (const auto k : world[ch->in_room]->people)
	{
		if (GET_MOB_SPEC(k) != NULL
			&& (fnum == 1
				|| fnum == specialNum++)
			&& GET_MOB_SPEC(k)(ch, k, cmd, arg))
		{
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in object present? //
	for (i = world[ch->in_room]->contents; i; i = i->get_next_content())
	{
		auto spec = GET_OBJ_SPEC(i);
		if (spec != NULL
			&& spec(ch, i, cmd, arg))
		{
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	return (0);
}

// **************************************************************************
// *  Stuff for controlling the non-playing sockets (get name, pwd etc)     *
// **************************************************************************

// locate entry in p_table with entry->name == name. -1 mrks failed search
int find_name(const char *name)
{
	const auto index = player_table.get_by_name(name);
	return PlayersIndex::NOT_FOUND == index ? -1 : static_cast<int>(index);
}

int _parse_name(char *arg, char *name)
{
	int i;

	// skip whitespaces
	for (i = 0; (*name = (i ? LOWER(*arg) : UPPER(*arg))); arg++, i++, name++)
	{
		if (*arg == 'ё'
			|| *arg == 'Ё'
			|| !a_isalpha(*arg)
			|| *arg > 0)
		{
			return (1);
		}
	}

	if (!i)
	{
		return (1);
	}

	return (0);
}

/**
* Вобщем это пока дублер старого _parse_name для уже созданных ранее чаров,
* чтобы их в игру вообще пускало, а новых с Ё/ё соответственно брило.
*/
int parse_exist_name(char *arg, char *name)
{
	int i;

	// skip whitespaces
	for (i = 0; (*name = (i ? LOWER(*arg) : UPPER(*arg))); arg++, i++, name++)
		if (!a_isalpha(*arg) || *arg > 0)
			return (1);

	if (!i)
		return (1);

	return (0);
}

enum Mode
{
	UNDEFINED,
	RECON,
	USURP,
	UNSWITCH
};

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
int perform_dupe_check(DESCRIPTOR_DATA * d)
{
	DESCRIPTOR_DATA *k, *next_k;
	Mode mode = UNDEFINED;

	int id = GET_IDNUM(d->character);

	/*
	 * Now that this descriptor has successfully logged in, disconnect all
	 * other descriptors controlling a character with the same ID number.
	 */

	CHAR_DATA::shared_ptr target;
	for (k = descriptor_list; k; k = next_k)
	{
		next_k = k->next;
		if (k == d)
		{
			continue;
		}

		if (k->original && (GET_IDNUM(k->original) == id))  	// switched char
		{
			if (str_cmp(d->host, k->host))
			{
				sprintf(buf, "ПОВТОРНЫЙ ВХОД !!! ID = %ld Персонаж = %s Хост = %s(был %s)",
						GET_IDNUM(d->character), GET_NAME(d->character), d->host, k->host);
				mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
				//send_to_gods(buf);
			}

			SEND_TO_Q("\r\nПопытка второго входа - отключаемся.\r\n", k);
			STATE(k) = CON_CLOSE;

			if (!target)
			{
				target = k->original;
				mode = UNSWITCH;
			}

			if (k->character)
			{
				k->character->desc = NULL;
			}

			k->character = NULL;
			k->original = NULL;
		}
		else if (k->character && (GET_IDNUM(k->character) == id))
		{
			if (str_cmp(d->host, k->host))
			{
				sprintf(buf, "ПОВТОРНЫЙ ВХОД !!! ID = %ld Name = %s Host = %s(was %s)",
						GET_IDNUM(d->character), GET_NAME(d->character), d->host, k->host);
				mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
				//send_to_gods(buf);
			}

			if (!target && STATE(k) == CON_PLAYING)
			{
				SEND_TO_Q("\r\nВаше тело уже кем-то занято!\r\n", k);
				target = k->character;
				mode = USURP;
			}
			k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
			SEND_TO_Q("\r\nПопытка второго входа - отключаемся.\r\n", k);
			STATE(k) = CON_CLOSE;
		}
	}

	/*
	 * now, go through the character list, deleting all characters that
	 * are not already marked for deletion from the above step (i.e., in the
	 * CON_HANGUP state), and have not already been selected as a target for
	 * switching into.  In addition, if we haven't already found a target,
	 * choose one if one is available (while still deleting the other
	 * duplicates, though theoretically none should be able to exist).
	 */

	character_list.foreach_on_copy([&](const CHAR_DATA::shared_ptr& ch)
	{
		if (IS_NPC(ch))
		{
			return;
		}

		if (GET_IDNUM(ch) != id)
		{
			return;
		}

		// ignore chars with descriptors (already handled by above step) //
		if (ch->desc)
			return;

		// don't extract the target char we've found one already //
		if (ch == target)
			return;

		// we don't already have a target and found a candidate for switching //
		if (!target)
		{
			target = ch;
			mode = RECON;
			return;
		}

		// we've found a duplicate - blow him away, dumping his eq in limbo. //
		if (ch->in_room != NOWHERE)
		{
			char_from_room(ch);
		}
		char_to_room(ch, STRANGE_ROOM);
		extract_char(ch.get(), FALSE);
	});

	// no target for switching into was found - allow login to continue //
	if (!target)
	{
		return 0;
	}

	// Okay, we've found a target.  Connect d to target. //

	d->character = target;
	d->character->desc = d;
	d->original = NULL;
	d->character->char_specials.timer = 0;
	PLR_FLAGS(d->character).unset(PLR_MAILING);
	PLR_FLAGS(d->character).unset(PLR_WRITING);
	STATE(d) = CON_PLAYING;

	switch (mode)
	{
	case RECON:
		SEND_TO_Q("Пересоединяемся.\r\n", d);
		check_light(d->character.get(), LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, 1);
		act("$n восстановил$g связь.", TRUE, d->character.get(), 0, 0, TO_ROOM);
		sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
		login_change_invoice(d->character.get());
		break;

	case USURP:
		SEND_TO_Q("Ваша душа вновь вернулась в тело, которое так ждало ее возвращения!\r\n", d);
		act("$n надломил$u от боли, окруженн$w белой аурой...\r\n"
			"Тело $s было захвачено новым духом!", TRUE, d->character.get(), 0, 0, TO_ROOM);
		sprintf(buf, "%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
		break;

	case UNSWITCH:
		SEND_TO_Q("Пересоединяемся для перевключения игрока.", d);
		sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
		break;

	default:
		// ??? what does this case mean ???
		break;
	}

	add_logon_record(d);
	return 1;
}

int pre_help(CHAR_DATA * ch, char *arg)
{
	char command[MAX_INPUT_LENGTH], topic[MAX_INPUT_LENGTH];

	half_chop(arg, command, topic);

	if (!*command || strlen(command) < 2 || !*topic || strlen(topic) < 2)
		return (0);
	if (isname(command, "помощь help справка"))
	{
		do_help(ch, topic, 0, 0);
		return (1);
	}
	return (0);
}

// вобщем флажок для зареганных ип, потому что при очередной автопроверке, если превышен
// лимит коннектов с ип - сядут все сместе, что выглядит имхо странно, может там комп новый воткнули
// и просто еще до иммов не достучались лимит поднять... вобщем сидит тот, кто не успел Ж)
int check_dupes_host(DESCRIPTOR_DATA * d, bool autocheck = 0)
{
	if (!d->character || IS_IMMORTAL(d->character))
		return 1;

	// в случае авточекалки нужная проверка уже выполнена до входа в функцию
	if (!autocheck)
	{
		if (RegisterSystem::is_registered(d->character.get()))
		{
			return 1;
		}

		if (RegisterSystem::is_registered_email(GET_EMAIL(d->character)))
		{
			d->registered_email = 1;
			return 1;
		}
	}

	for (DESCRIPTOR_DATA* i = descriptor_list; i; i = i->next)
	{
		if (i != d
			&& i->ip == d->ip
			&& i->character
			&& !IS_IMMORTAL(i->character)
			&& (STATE(i) == CON_PLAYING
				|| STATE(i) == CON_MENU))
		{
			switch (CheckProxy(d))
			{
			case 0:
				// если уже сидим в проксе, то смысла спамить никакого
				if (IN_ROOM(d->character) == r_unreg_start_room
					|| d->character->get_was_in_room() == r_unreg_start_room)
				{
					return 0;
				}
				send_to_char(d->character.get(),
							 "&RВы вошли с игроком %s с одного IP(%s)!\r\n"
							 "Вам необходимо обратиться к Богам для регистрации.\r\n"
							 "Пока вы будете помещены в комнату для незарегистрированных игроков.&n\r\n",
							 GET_PAD(i->character, 4), i->host);
				sprintf(buf,
						"! ВХОД С ОДНОГО IP ! незарегистрированного игрока.\r\n"
						"Вошел - %s, в игре - %s, IP - %s.\r\n"
						"Игрок помещен в комнату незарегистрированных игроков.",
						GET_NAME(d->character), GET_NAME(i->character), d->host);
				mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
				return 0;

			case 1:
				if (autocheck)
				{
					return 1;
				}
				send_to_char("&RС вашего IP адреса находится максимально допустимое количество игроков.\r\n"
							 "Обратитесь к Богам для увеличения лимита игроков с вашего адреса.&n", d->character.get());
				return 0;

			default:
				return 1;
			}
		}
	}
	return 1;
}

int check_dupes_email(DESCRIPTOR_DATA * d)
{
	if (!d->character
		|| IS_IMMORTAL(d->character))
	{
		return (1);
	}

	for (const auto ch : character_list)
	{
		if (ch == d->character
			|| IS_NPC(ch))
		{
			continue;
		}

		if (!IS_IMMORTAL(ch)
			&& (!str_cmp(GET_EMAIL(ch), GET_EMAIL(d->character))))
		{
			sprintf(buf, "Персонаж с таким email уже находится в игре, вы не можете войти одновременно с ним!");
			send_to_char(buf, d->character.get());
			return (0);
		}
	}

	return 1;
}

void add_logon_record(DESCRIPTOR_DATA * d)
{
	log("Enter logon list");
	// Добавляем запись в LOG_LIST
	d->character->get_account()->add_login(std::string(d->host));

	const auto logon = std::find_if(LOGON_LIST(d->character).begin(), LOGON_LIST(d->character).end(),
		[&](const logon_data& l) -> bool
		{
			return !strcmp(l.ip, d->host);
		});

	if (logon == LOGON_LIST(d->character).end())
	{
		const logon_data cur_log = { str_dup(d->host), 1, time(0), false };
		LOGON_LIST(d->character).push_back(cur_log);			
	}
	else
	{
		++logon->count;
		logon->lasttime = time(0);
	}

	int pos = get_ptable_by_unique(GET_UNIQUE(d->character));
	if (pos >= 0) {
		if (player_table[pos].last_ip)
			free(player_table[pos].last_ip);
		player_table[pos].last_ip = str_dup(d->host);
		player_table[pos].last_logon = LAST_LOGON(d->character);
	}
	log("Exit logon list");
}

// * Проверка на доступные религии конкретной профе (из текущей генерации чара).
void check_religion(CHAR_DATA *ch)
{
	if (class_religion[ch->get_class()] == RELIGION_POLY && GET_RELIGION(ch) != RELIGION_POLY)
	{
		GET_RELIGION(ch) = RELIGION_POLY;
		log("Change religion to poly: %s", ch->get_name().c_str());
	}
	else if (class_religion[ch->get_class()] == RELIGION_MONO && GET_RELIGION(ch) != RELIGION_MONO)
	{
		GET_RELIGION(ch) = RELIGION_MONO;
		log("Change religion to mono: %s", ch->get_name().c_str());
	}
}

void do_entergame(DESCRIPTOR_DATA * d)
{
	int load_room, cmd, flag = 0;

	d->character->reset();
	read_aliases(d->character.get());

	if (GET_LEVEL(d->character) == LVL_IMMORT)
	{
		d->character->set_level(LVL_GOD);
	}

	if (GET_LEVEL(d->character) > LVL_IMPL)
	{
		d->character->set_level(1);
	}

	if (GET_INVIS_LEV(d->character) > LVL_IMPL
		|| GET_INVIS_LEV(d->character) < 0)
	{
		SET_INVIS_LEV(d->character, 0);
	}

	if (GET_LEVEL(d->character) > LVL_IMMORT
			&& GET_LEVEL(d->character) < LVL_BUILDER
			&& (d->character->get_gold() > 0 || d->character->get_bank() > 0))
	{
		d->character->set_gold(0);
		d->character->set_bank(0);
	}

	if (GET_LEVEL(d->character) >= LVL_IMMORT && GET_LEVEL(d->character) < LVL_IMPL)
	{
		for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
		{
			if (!strcmp(cmd_info[cmd].command, "syslog"))
			{
				if (Privilege::can_do_priv(d->character.get(), std::string(cmd_info[cmd].command), cmd, 0))
				{
					flag = 1;
					break;
				}
			}
		}

		if (!flag)
		{
			GET_LOGS(d->character)[0] = 0;
		}
	}

	if (GET_LEVEL(d->character) < LVL_IMPL)
	{
		if (PLR_FLAGGED(d->character, PLR_INVSTART))
		{
			SET_INVIS_LEV(d->character, LVL_IMMORT);
		}
		if (GET_INVIS_LEV(d->character) > GET_LEVEL(d->character))
		{
			SET_INVIS_LEV(d->character, GET_LEVEL(d->character));
		}

		if (PRF_FLAGGED(d->character, PRF_CODERINFO))
		{
			PRF_FLAGS(d->character).unset(PRF_CODERINFO);
		}
		if (GET_LEVEL(d->character) < LVL_GOD)
		{
			if (PRF_FLAGGED(d->character, PRF_HOLYLIGHT))
			{
				PRF_FLAGS(d->character).unset(PRF_HOLYLIGHT);
			}
		}
		if (GET_LEVEL(d->character) < LVL_GOD)
		{
			if (PRF_FLAGGED(d->character, PRF_NOHASSLE))
			{
				PRF_FLAGS(d->character).unset(PRF_NOHASSLE);
			}
			if (PRF_FLAGGED(d->character, PRF_ROOMFLAGS))
			{
				PRF_FLAGS(d->character).unset(PRF_ROOMFLAGS);
			}
		}

		if (GET_INVIS_LEV(d->character) > 0
			&& GET_LEVEL(d->character) < LVL_IMMORT)
		{
			SET_INVIS_LEV(d->character, 0);
		}
	}

	OfftopSystem::set_flag(d->character.get());
	// пересчет максимального хп, если нужно
	check_max_hp(d->character.get());
	// проверка и сет религии
	check_religion(d->character.get());

	/*
	 * We have to place the character in a room before equipping them
	 * or equip_char() will gripe about the person in NOWHERE.
	 */
	if (PLR_FLAGGED(d->character, PLR_HELLED))
		load_room = r_helled_start_room;
	else if (PLR_FLAGGED(d->character, PLR_NAMED))
		load_room = r_named_start_room;
	else if (PLR_FLAGGED(d->character, PLR_FROZEN))
		load_room = r_frozen_start_room;
	else if (!check_dupes_host(d))
		load_room = r_unreg_start_room;
	else
	{
		if ((load_room = GET_LOADROOM(d->character)) == NOWHERE)
		{
			load_room = calc_loadroom(d->character.get());
		}
		load_room = real_room(load_room);

		if (!Clan::MayEnter(d->character.get(), load_room, HCE_PORTAL))
		{
			load_room = Clan::CloseRent(load_room);
		}

		if (!is_rent(load_room))
		{
			load_room = NOWHERE;
		}
	}

	// If char was saved with NOWHERE, or real_room above failed...
	if (load_room == NOWHERE)
	{
		if (GET_LEVEL(d->character) >= LVL_IMMORT)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}

	send_to_char(WELC_MESSG, d->character.get());

	CHAR_DATA* character = nullptr;
	for (const auto character_i : character_list)
	{
		if (character_i == d->character)
		{
			character = character_i.get();
			break;
		}
	}

	if (!character)
	{
		character_list.push_front(d->character);
	}
	else
	{
		MOB_FLAGS(character).unset(MOB_DELETE);
		MOB_FLAGS(character).unset(MOB_FREE);
	}

	log("Player %s enter at room %d", GET_NAME(d->character), GET_ROOM_VNUM(load_room));
	char_to_room(d->character, load_room);

	// а потом уже вычитаем за ренту
	if (GET_LEVEL(d->character) != 0)
	{
		Crash_load(d->character.get());
		d->character->obj_bonus().update(d->character.get());
	}

	Depot::enter_char(d->character.get());
	Glory::check_freeze(d->character.get());
	Clan::clan_invoice(d->character.get(), true);

	// Чистим стили если не знаем их
	if (PRF_FLAGS(d->character).get(PRF_PUNCTUAL)
		&& !d->character->get_skill(SKILL_PUNCTUAL))
	{
		PRF_FLAGS(d->character).unset(PRF_PUNCTUAL);
	}

	if (PRF_FLAGS(d->character).get(PRF_AWAKE)
		&& !d->character->get_skill(SKILL_AWAKE))
	{
		PRF_FLAGS(d->character).unset(PRF_AWAKE);
	}

	if (PRF_FLAGS(d->character).get(PRF_POWERATTACK)
		&& !can_use_feat(d->character.get(), POWER_ATTACK_FEAT))
	{
		PRF_FLAGS(d->character).unset(PRF_POWERATTACK);
	}

	if (PRF_FLAGS(d->character).get(PRF_GREATPOWERATTACK)
		&& !can_use_feat(d->character.get(), GREAT_POWER_ATTACK_FEAT))
	{
		PRF_FLAGS(d->character).unset(PRF_GREATPOWERATTACK);
	}

	if (PRF_FLAGS(d->character).get(PRF_AIMINGATTACK)
		&& !can_use_feat(d->character.get(), AIMING_ATTACK_FEAT))
	{
		PRF_FLAGS(d->character).unset(PRF_AIMINGATTACK);
	}

	if (PRF_FLAGS(d->character).get(PRF_GREATAIMINGATTACK)
		&& !can_use_feat(d->character.get(), GREAT_AIMING_ATTACK_FEAT))
	{
		PRF_FLAGS(d->character).unset(PRF_GREATAIMINGATTACK);
	}

	// Gorrah: сбрасываем флаг от скилла, если он каким-то чудом засэйвился
	if (PRF_FLAGS(d->character).get(PRF_IRON_WIND))
	{
		PRF_FLAGS(d->character).unset(PRF_IRON_WIND);
	}

	// Check & remove/add natural, race & unavailable features
	for (int i = 1; i < MAX_FEATS; i++)
	{
		if (!HAVE_FEAT(d->character, i)
			|| can_get_feat(d->character.get(), i))
		{
			if (feat_info[i].natural_classfeat[(int) GET_CLASS(d->character)][(int) GET_KIN(d->character)])
			{
				SET_FEAT(d->character, i);
			}
		}
	}

	set_race_feats(d->character.get());

	//нефиг левыми скиллами размахивать если не имм
	if (!IS_IMMORTAL(d->character))
	{
		for (const auto i : AVAILABLE_SKILLS)
		{
			if (skill_info[i].classknow[(int)GET_CLASS(d->character)][(int)GET_KIN(d->character)] != KNOW_SKILL)
			{
				d->character->set_skill(i, 0);
			}
		}
	}

	//Заменяем закл !переместиться! на способность
	if (GET_SPELL_TYPE(d->character, SPELL_RELOCATE) == SPELL_KNOW && !IS_GOD(d->character))
	{
		GET_SPELL_TYPE(d->character, SPELL_RELOCATE) = 0;
		SET_FEAT(d->character, RELOCATE_FEAT);
	}

	//Проверим временные заклы пока нас не было
	Temporary_Spells::update_char_times(d->character.get(), time(0));

	// Карачун. Редкая бага. Сбрасываем явно не нужные аффекты.
	d->character->remove_affect(EAffectFlag::AFF_GROUP);
	d->character->remove_affect(EAffectFlag::AFF_HORSE);

	// изменяем порталы
	check_portals(d->character.get());

	// with the copyover patch, this next line goes in enter_player_game()
	GET_ID(d->character) = GET_IDNUM(d->character);
	GET_ACTIVITY(d->character) = number(0, PLAYER_SAVE_ACTIVITY - 1);
	d->character->set_last_logon(time(0));
	player_table[get_ptable_by_unique(GET_UNIQUE(d->character))].last_logon = LAST_LOGON(d->character);
	add_logon_record(d);
	// чтобы восстановление маны спам-контроля "кто" не шло, когда чар заходит после
	// того, как повисел на менюшке; важно, чтобы этот вызов шел раньше save_char()
	d->character->set_who_last(time(0));
	d->character->save_char();
	act("$n вступил$g в игру.", TRUE, d->character.get(), 0, 0, TO_ROOM);
	// with the copyover patch, this next line goes in enter_player_game()
	read_saved_vars(d->character.get());
	greet_mtrigger(d->character.get(), -1);
	greet_otrigger(d->character.get(), -1);
	STATE(d) = CON_PLAYING;
// режимы по дефолту у нового чара
	const bool new_char = GET_LEVEL(d->character) <= 0 ? true : false;
	if (new_char)
	{
		PRF_FLAGS(d->character).set(PRF_DRAW_MAP); //рисовать миникарту
		PRF_FLAGS(d->character).set(PRF_GOAHEAD); //IAC GA
		PRF_FLAGS(d->character).set(PRF_AUTOMEM); // автомем
		PRF_FLAGS(d->character).set(PRF_AUTOLOOT); // автолут
		PRF_FLAGS(d->character).set(PRF_PKL_MODE); // пклист
		PRF_FLAGS(d->character).set(PRF_WORKMATE_MODE); // соклан
		d->character->map_set_option(MapSystem::MAP_MODE_MOB_SPEC_SHOP);
		d->character->map_set_option(MapSystem::MAP_MODE_MOB_SPEC_RENT);
		d->character->map_set_option(MapSystem::MAP_MODE_MOB_SPEC_BANK);
		d->character->map_set_option(MapSystem::MAP_MODE_MOB_SPEC_TEACH);
		d->character->map_set_option(MapSystem::MAP_MODE_BIG);
		PRF_FLAGS(d->character).set(PRF_ENTER_ZONE);
		PRF_FLAGS(d->character).set(PRF_BOARD_MODE);
		d->character->set_last_exchange(time(0)); // когда последний раз базар
		do_start(d->character.get(), TRUE);
		GET_MANA_STORED(d->character) = 0;
		send_to_char(START_MESSG, d->character.get());
	}

	init_warcry(d->character.get());

	// На входе в игру вешаем флаг (странно, что он до этого нигде не вешался
	if (Privilege::god_list_check(GET_NAME(d->character), GET_UNIQUE(d->character)) && (GET_LEVEL(d->character) < LVL_GOD))
	{
	    SET_GOD_FLAG(d->character, GF_DEMIGOD);
	}
	// Насильственно забираем этот флаг у иммов (если он, конечно же, есть
	if ((GET_GOD_FLAG(d->character, GF_DEMIGOD) && GET_LEVEL(d->character) >= LVL_GOD))
	{
	    CLR_GOD_FLAG(d->character, GF_DEMIGOD);
	}

	switch (GET_SEX(d->character))
	{
	case ESex::SEX_NEUTRAL:
		sprintf(buf, "%s вошло в игру.", GET_NAME(d->character));
		break;

	case ESex::SEX_MALE:
		sprintf(buf, "%s вошел в игру.", GET_NAME(d->character));
		break;

	case ESex::SEX_FEMALE:
		sprintf(buf, "%s вошла в игру.", GET_NAME(d->character));
		break;

	case ESex::SEX_POLY:
		sprintf(buf, "%s вошли в игру.", GET_NAME(d->character));
		break;
	}

	mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
	look_at_room(d->character.get(), 0);
	d->has_prompt = 0;
	login_change_invoice(d->character.get());
	check_light(d->character.get(), LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, 0);

	if (new_char)
	{
		send_to_char( "\r\nВоспользуйтесь командой НОВИЧОК для получения вводной информации игроку.\r\n",
			d->character.get());
		send_to_char(
			"Включен режим автоматического показа карты, наберите 'справка карта' для ознакомления.\r\n"
			"Если вы заблудились и не можете самостоятельно найти дорогу назад - прочтите 'справка возврат'.\r\n",
			d->character.get());
	}

	Noob::check_help_message(d->character.get());
}

//По кругу проверяем корректность параметров
//Все это засунуто в одну функцию для того
//Чтобы в случае некорректности сразу нескольких параметров
//Они все были корректно обработаны
bool ValidateStats(DESCRIPTOR_DATA * d)
{
	//Требуется рерол статов
    if (!GloryMisc::check_stats(d->character.get()))
	{
		return false;
	}

    //Некорректный номер расы
    if (PlayerRace::GetKinNameByNum(GET_KIN(d->character),GET_SEX(d->character)) == KIN_NAME_UNDEFINED)
    {
        SEND_TO_Q("\r\nЧто-то заплутал ты, путник. Откуда бредешь?\r\nВыберите народ:\r\n", d);
        SEND_TO_Q(string(PlayerRace::ShowKinsMenu()).c_str(), d);
		SEND_TO_Q("\r\nВыберите племя: ", d);
		STATE(d) = CON_RESET_KIN;
        return false;
    }

    //Некорректный номер рода
    if (PlayerRace::GetRaceNameByNum(GET_KIN(d->character),GET_RACE(d->character),GET_SEX(d->character)) == RACE_NAME_UNDEFINED)
    {
		SEND_TO_Q("\r\nКакого роду-племени вы будете?\r\n", d);
		SEND_TO_Q(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
		SEND_TO_Q("\r\nИз чьих вы будете: ", d);
        STATE(d) = CON_RESET_RACE;
        return false;
    }

    // не корректный номер религии
    if (GET_RELIGION(d->character) > RELIGION_MONO)
    {
        SEND_TO_Q(religion_menu, d);
        SEND_TO_Q("\n\rРелигия :", d);
        STATE(d) = CON_RESET_RELIGION;
        return false;
    }

	return true;
}

void DoAfterPassword(DESCRIPTOR_DATA * d)
{
	int load_result;

	// Password was correct.
	load_result = GET_BAD_PWS(d->character);
	GET_BAD_PWS(d->character) = 0;
	d->bad_pws = 0;

	if (ban->is_banned(d->host) == BanList::BAN_SELECT && !PLR_FLAGGED(d->character, PLR_SITEOK))
	{
		SEND_TO_Q("Извините, вы не можете выбрать этого игрока с данного IP!\r\n", d);
		STATE(d) = CON_CLOSE;
		sprintf(buf, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
		return;
	}
	if (GET_LEVEL(d->character) < circle_restrict)
	{
		SEND_TO_Q("Игра временно приостановлена.. Ждем вас немного позже.\r\n", d);
		STATE(d) = CON_CLOSE;
		sprintf(buf, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
		return;
	}
	if (new_loc_codes.count(GET_EMAIL(d->character)) != 0)
	{
		SEND_TO_Q("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
		STATE(d) = CON_RANDOM_NUMBER;
		return;
	}
	// нам нужен массив сетей с маской /24
	std::set<uint32_t> subnets;

	const uint32_t MASK = 16777215;
	for(const auto& logon : LOGON_LIST(d->character))
	{
		uint32_t current_subnet = inet_addr(logon.ip) & MASK;
		subnets.insert(current_subnet);
	}

	if (!subnets.empty())
	{
		if (subnets.count(inet_addr(d->host) & MASK) == 0)
		{
			sprintf(buf, "Персонаж %s вошел с необычного места!", GET_NAME(d->character));
			mudlog(buf, CMP, LVL_GOD, SYSLOG, TRUE);
			if (PRF_FLAGGED(d->character, PRF_IPCONTROL)) {
				int random_number = number(1000000, 9999999);
				new_loc_codes[GET_EMAIL(d->character)] = random_number;
				std::string cmd_line =  str(boost::format("python3 send_code.py %s %d &") % GET_EMAIL(d->character) % random_number);
				auto result = system(cmd_line.c_str());
				UNUSED_ARG(result);
				SEND_TO_Q("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
				STATE(d) = CON_RANDOM_NUMBER;
				return;
			}
		}
	}
	// check and make sure no other copies of this player are logged in
	if (perform_dupe_check(d))
	{
		Clan::SetClanData(d->character.get());
		return;
	}

	// тут несколько вариантов как это проставить и все одинаково корявые с учетом релоада, без уверенности не трогать
	Clan::SetClanData(d->character.get());

	log("%s [%s] has connected.", GET_NAME(d->character), d->host);

	if (load_result)
	{
		sprintf(buf, "\r\n\r\n\007\007\007"
				"%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
				CCRED(d->character, C_SPR), load_result,
				(load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
		SEND_TO_Q(buf, d);
		GET_BAD_PWS(d->character) = 0;
	}
	time_t tmp_time = LAST_LOGON(d->character);
	sprintf(buf, "\r\nПоследний раз вы заходили к нам в %s с адреса (%s).\r\n",
			rustime(localtime(&tmp_time)), GET_LASTIP(d->character));
	SEND_TO_Q(buf, d);

	//if (!GloryMisc::check_stats(d->character))
	if (!ValidateStats(d))
	{
		return;
	}	

	SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	STATE(d) = CON_RMOTD;
}

void CreateChar(DESCRIPTOR_DATA * d)
{
	if (d->character)
	{
		return;
	}

	d->character.reset(new Player);
	d->character->player_specials = std::make_shared<player_special_data>();
	d->character->desc = d;
}

int create_unique(void)
{
	int unique;

	do
	{
		unique = (number(0, 64) << 24) + (number(0, 255) << 16) + (number(0, 255) << 8) + (number(0, 255));
	} while (correct_unique(unique));

	return (unique);
}

// initialize a new character only if class is set
void init_char(CHAR_DATA* ch, player_index_element& element)
{
	int i;

#ifdef TEST_BUILD
	if (0 == player_table.size())
	{
		// При собирании через make test первый чар в маде становится иммом 34
		ch->set_level(LVL_IMPL);
	}
#endif

	GET_PORTALS(ch) = NULL;
	CREATE(GET_LOGS(ch), 1 + LAST_LOG);
	ch->set_npc_name(0);
	ch->player_data.long_descr = "";
	ch->player_data.description = "";
	ch->player_data.time.birth = time(0);
	ch->player_data.time.played = 0;
	ch->player_data.time.logon = time(0);

	// make favors for sex
	if (ch->player_data.sex == ESex::SEX_MALE)
	{
		ch->player_data.weight = number(120, 180);
		ch->player_data.height = number(160, 200);
	}
	else
	{
		ch->player_data.weight = number(100, 160);
		ch->player_data.height = number(150, 180);
	}

	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.max_move = 82;
	ch->points.move = GET_MAX_MOVE(ch);
	ch->real_abils.armor = 100;

	ch->set_idnum(++top_idnum);
	element.set_id(ch->get_idnum());
	ch->set_uid(create_unique());
	element.unique = ch->get_uid();
	element.level = 0;
	element.remorts = 0;
	element.last_logon = -1;
	element.mail = NULL;//added by WorM mail
	element.last_ip = NULL;//added by WorM последний айпи

	if (GET_LEVEL(ch) > LVL_GOD)
	{
		set_god_skills(ch);
		set_god_morphs(ch);
	}

	for (i = 1; i <= MAX_SPELLS; i++)
	{
		if (GET_LEVEL(ch) < LVL_GRGOD)
			GET_SPELL_TYPE(ch, i) = 0;
		else
			GET_SPELL_TYPE(ch, i) = SPELL_KNOW;
	}

	ch->char_specials.saved.affected_by = clear_flags;
	for (i = 0; i < SAVING_COUNT; i++)
		GET_SAVE(ch, i) = 0;
	for (i = 0; i < MAX_NUMBER_RESISTANCE; i++)
		GET_RESIST(ch, i) = 0;

	if (GET_LEVEL(ch) == LVL_IMPL)
	{
		ch->set_str(25);
		ch->set_int(25);
		ch->set_wis(25);
		ch->set_dex(25);
		ch->set_con(25);
		ch->set_cha(25);
	}
	ch->real_abils.size = 50;

	for (i = 0; i < 3; i++)
	{
		GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : i == DRUNK ? 0 : 24);
	}
	GET_LASTIP(ch)[0] = 0;
	//	GET_LOADROOM(ch) = start_room;
	PRF_FLAGS(ch).set(PRF_DISPHP);
	PRF_FLAGS(ch).set(PRF_DISPMANA);
	PRF_FLAGS(ch).set(PRF_DISPEXITS);
	PRF_FLAGS(ch).set(PRF_DISPMOVE);
	PRF_FLAGS(ch).set(PRF_DISPEXP);
	PRF_FLAGS(ch).set(PRF_DISPFIGHT);
	PRF_FLAGS(ch).unset(PRF_SUMMONABLE);
	PRF_FLAGS(ch).set(PRF_COLOR_2);
	STRING_LENGTH(ch) = 80;
	STRING_WIDTH(ch) = 30;
	NOTIFY_EXCH_PRICE(ch) = 0;

	ch->save_char();
}

/*
* Create a new entry in the in-memory index table for the player file.
* If the name already exists, by overwriting a deleted character, then
* we re-use the old position.
*/
int create_entry(player_index_element& element)
{
	// create new save activity
	element.activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
	element.timer = NULL;

	return static_cast<int>(player_table.append(element));
}

void print_free_names(std::ostream& os, const PlayersIndex& index)
{
	constexpr int SUGGESTIONS_COUNT = 4;
	PlayersIndex::free_names_list_t names;
	index.get_free_names(SUGGESTIONS_COUNT, names);
	os << JoinRange<PlayersIndex::free_names_list_t>(names);
}

void DoAfterEmailConfirm(DESCRIPTOR_DATA *d)
{
	player_index_element element(-1, GET_PC_NAME(d->character));

	// Now GET_NAME() will work properly.
	init_char(d->character.get(), element);

	if (d->character->get_pfilepos() < 0)
	{
		d->character->set_pfilepos(create_entry(element));
	}
	d->character->save_char();
	d->character->get_account()->set_last_login();
	d->character->get_account()->add_player(GetUniqueByName(d->character->get_name()));

	// добавляем в список ждущих одобрения
	if (!(int)NAME_FINE(d->character))
	{
		sprintf(buf, "%s - новый игрок. Падежи: %s/%s/%s/%s/%s/%s Email: %s Пол: %s. ]\r\n"
					 "[ %s ждет одобрения имени.",
				GET_NAME(d->character), GET_PAD(d->character, 0),
				GET_PAD(d->character, 1), GET_PAD(d->character, 2),
				GET_PAD(d->character, 3), GET_PAD(d->character, 4),
				GET_PAD(d->character, 5), GET_EMAIL(d->character),
				genders[(int)GET_SEX(d->character)], GET_NAME(d->character));
		NewNames::add(d->character.get());
	}

	SEND_TO_Q(motd, d);
	SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	STATE(d) = CON_RMOTD;
	d->character->set_who_mana(0);
	d->character->set_who_last(time(0));

}

// Фраза 'Русская азбука: "абв...эюя".' в разных кодировках и для разных клиентов
#define ENC_HINT_KOI8R          "\xf2\xd5\xd3\xd3\xcb\xc1\xd1 \xc1\xda\xc2\xd5\xcb\xc1: \"\xc1\xc2\xd7...\xdc\xc0\xd1\"."
#define ENC_HINT_ALT            "\x90\xe3\xe1\xe1\xaa\xa0\xef \xa0\xa7\xa1\xe3\xaa\xa0: \"\xa0\xa1\xa2...\xed\xee\xef\"."
#define ENC_HINT_WIN   		    "\xd0\xf3\xf1\xf1\xea\xe0\xff\xff \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfe\xff\xff\"."
// обход ошибки с 'я' в zMUD после ver. 6.39+ и CMUD без замены 'я' на 'z'
#define ENC_HINT_WIN_ZMUD       "\xd0\xf3\xf1\xf1\xea\xe0\xff\xff? \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfe\xff\xff?\"."
// замена 'я' на 'z' в zMUD до ver. 6.39a для обхода ошибки,
// а также в zMUD после ver. 6.39a для совместимости
#define ENC_HINT_WIN_ZMUD_z     "\xd0\xf3\xf1\xf1\xea\xe0z \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfez\"."
#define ENC_HINT_WIN_ZMUD_old   ENC_HINT_WIN_ZMUD_z
#define ENC_HINT_UTF8           "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb0\xd1\x8f "\
                                "\xd0\xb0\xd0\xb7\xd0\xb1\xd1\x83\xd0\xba\xd0\xb0: "\
                                "\"\xd0\xb0\xd0\xb1\xd0\xb2...\xd1\x8d\xd1\x8e\xd1\x8f\"."

static void ShowEncodingPrompt(DESCRIPTOR_DATA *d, bool withHints = false)
{
	if (withHints)
	{
		SEND_TO_Q(
			"\r\n"
			"Using keytable           TECT. CTPOKA\r\n"
			"  0) Koi-8               " ENC_HINT_KOI8R        "\r\n"
			"  1) Alt                 " ENC_HINT_ALT          "\r\n"
			"  2) Windows(JMC,MMC)    " ENC_HINT_WIN          "\r\n"
			"  3) Windows(zMUD)       " ENC_HINT_WIN_ZMUD     "\r\n"
			"  4) Windows(zMUD 'z')   " ENC_HINT_WIN_ZMUD_z   "\r\n"
			"  5) UTF-8               " ENC_HINT_UTF8         "\r\n"
			"  6) Windows(zMUD <6.39) " ENC_HINT_WIN_ZMUD_old "\r\n"
//			"Select one : ", d);
			"\r\n"
			"KAKOE HAnuCAHuE ECTb BEPHOE, PA3yMEEMOE HA PyCCKOM? BBEguTE HOMEP : ", d);
	}
	else
	{
		SEND_TO_Q(
			"\r\n"
			"Using keytable\r\n"
			"  0) Koi-8\r\n"
			"  1) Alt\r\n"
			"  2) Windows(JMC,MMC)\r\n"
			"  3) Windows(zMUD)\r\n"
			"  4) Windows(zMUD 'z')\r\n"
			"  5) UTF-8\r\n"
			"  6) Windows(zMUD <6.39)\r\n"
			"  9) TECT...\r\n"
			"Select one : ", d);
	}
}

// deal with newcomers and other non-playing sockets
void nanny(DESCRIPTOR_DATA * d, char *arg)
{
	char buf[MAX_STRING_LENGTH];
	int player_i = 0, load_result;
	char tmp_name[MAX_INPUT_LENGTH], pwd_name[MAX_INPUT_LENGTH], pwd_pwd[MAX_INPUT_LENGTH];
	if (STATE(d) != CON_CONSOLE)
		skip_spaces(&arg);

	switch (STATE(d))
	{
	case CON_INIT:
		// just connected
		{
			int online_players = 0;
			for (auto i = descriptor_list; i; i = i->next)
			{
				online_players++;
			}
			sprintf(buf, "Online: %d\r\n", online_players);
		}
	
		SEND_TO_Q(buf, d);
		ShowEncodingPrompt(d, false);
		STATE(d) = CON_GET_KEYTABLE;
		break;

		//. OLC states .
	case CON_OEDIT:
		oedit_parse(d, arg);
		break;

	case CON_REDIT:
		redit_parse(d, arg);
		break;

	case CON_ZEDIT:
		zedit_parse(d, arg);
		break;

	case CON_MEDIT:
		medit_parse(d, arg);
		break;

	case CON_TRIGEDIT:
		trigedit_parse(d, arg);
		break;

	case CON_MREDIT:
		mredit_parse(d, arg);
		break;

	case CON_CLANEDIT:
		d->clan_olc->clan->Manage(d, arg);
		break;

	case CON_SPEND_GLORY:
		if (!Glory::parse_spend_glory_menu(d->character.get(), arg))
		{
			Glory::spend_glory_menu(d->character.get());
		}
		break;

	case CON_GLORY_CONST:
		if (!GloryConst::parse_spend_glory_menu(d->character.get(), arg))
		{
			GloryConst::spend_glory_menu(d->character.get());
		}
		break;

	case CON_NAMED_STUFF:
		if (!NamedStuff::parse_nedit_menu(d->character.get(), arg))
		{
			NamedStuff::nedit_menu(d->character.get());
		}
		break;

	case CON_MAP_MENU:
		d->map_options->parse_menu(d->character.get(), arg);
		break;

	case CON_TORC_EXCH:
		ExtMoney::torc_exch_parse(d->character.get(), arg);
		break;

	case CON_SEDIT:
	{
		try
		{
			obj_sets_olc::parse_input(d->character.get(), arg);
		}
		catch (const std::out_of_range &e)
		{
			send_to_char(d->character.get(), "Редактирование прервано: %s", e.what());
			d->sedit.reset();
			STATE(d) = CON_PLAYING;
		}
		break;
	}
		//. End of OLC states .*/

	case CON_GET_KEYTABLE:
		if (strlen(arg) > 0)
			arg[0] = arg[strlen(arg) - 1];
		if (*arg == '9')
		{
			ShowEncodingPrompt(d, true);
			return;
		};
		if (!*arg || *arg < '0' || *arg >= '0' + KT_LAST)
		{
			SEND_TO_Q("\r\nUnknown key table. Retry, please : ", d);
			return;
		};
		d->keytable = (ubyte) * arg - (ubyte) '0';
		ip_log(d->host);
		SEND_TO_Q(GREETINGS, d);
		STATE(d) = CON_GET_NAME;
		break;

	case CON_GET_NAME:	// wait for input of name
		if (!d->character)
		{
			CreateChar(d);
		}

		if (!*arg)
		{
			STATE(d) = CON_CLOSE;
		}
		else if (!str_cmp("новый", arg))
		{
			SEND_TO_Q(name_rules, d);

			std::stringstream ss;
			ss << "Введите имя";
			if (0 < player_table.free_names_count())
			{
				ss << " (примеры доступных имен : ";
				print_free_names(ss, player_table);
			ss << ")";
			}
			ss << ": ";

			SEND_TO_Q(ss.str().c_str(), d);
			STATE(d) = CON_NEW_CHAR;
			return;
		}
		else
		{
			if (sscanf(arg, "%s %s", pwd_name, pwd_pwd) == 2)
			{
				if (parse_exist_name(pwd_name, tmp_name)
					|| (player_i = load_char(tmp_name, d->character.get())) < 0)
				{
					SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
					return;
				}

				if (PLR_FLAGGED(d->character, PLR_DELETED)
					|| !Password::compare_password(d->character.get(), pwd_pwd))
				{
					SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
					if (!PLR_FLAGGED(d->character, PLR_DELETED))
					{
						sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
						mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
					}

					d->character.reset();
					return;
				}

				PLR_FLAGS(d->character).unset(PLR_MAILING);
				PLR_FLAGS(d->character).unset(PLR_WRITING);
				PLR_FLAGS(d->character).unset(PLR_CRYO);
				d->character->set_pfilepos(player_i);
				GET_ID(d->character) = GET_IDNUM(d->character);
				DoAfterPassword(d);

				return;
			}
			else
			{
				if (parse_exist_name(arg, tmp_name) ||
					strlen(tmp_name) < (MIN_NAME_LENGTH - 1) || // дабы можно было войти чарам с 4 буквами
					strlen(tmp_name) > MAX_NAME_LENGTH ||
					!Is_Valid_Name(tmp_name) || fill_word(tmp_name) || reserved_word(tmp_name))
				{
					SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
					return;
				}
				else if (!Is_Valid_Dc(tmp_name))
				{
					player_i = load_char(tmp_name, d->character.get());
					d->character->set_pfilepos(player_i);
					if (IS_IMMORTAL(d->character) || PRF_FLAGGED(d->character, PRF_CODERINFO))
					{
						SEND_TO_Q("Игрок с подобным именем является БЕССМЕРТНЫМ в игре.\r\n", d);
					}
					else
					{
						SEND_TO_Q("Игрок с подобным именем находится в игре.\r\n", d);
					}
					SEND_TO_Q("Во избежание недоразумений введите пару ИМЯ ПАРОЛЬ.\r\n", d);
					SEND_TO_Q("Имя и пароль через пробел : ", d);

					d->character.reset();
					return;
				}
			}

			player_i = load_char(tmp_name, d->character.get());
			if (player_i > -1)
			{
				d->character->set_pfilepos(player_i);
				if (PLR_FLAGGED(d->character, PLR_DELETED))  	// We get a false positive from the original deleted character.
				{
					d->character.reset();

					// Check for multiple creations...
					if (!Valid_Name(tmp_name) || _parse_name(tmp_name, tmp_name))
					{
						SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					}

					// дополнительная проверка на длину имени чара
					if (strlen(tmp_name) < (MIN_NAME_LENGTH))
					{
						SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					}

					CreateChar(d);
					d->character->set_pc_name(CAP(tmp_name));
					d->character->player_data.PNames[0] = std::string(CAP(tmp_name));
					d->character->set_pfilepos(player_i);
					sprintf(buf, "Вы действительно выбрали имя %s [ Y(Д) / N(Н) ]? ", tmp_name);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_NAME_CNFRM;
				}
				else  	// undo it just in case they are set
				{
					if (IS_IMMORTAL(d->character) || PRF_FLAGGED(d->character, PRF_CODERINFO))
					{
						SEND_TO_Q("Игрок с подобным именем является БЕССМЕРТНЫМ в игре.\r\n", d);
						SEND_TO_Q("Во избежание недоразумений введите пару ИМЯ ПАРОЛЬ.\r\n", d);
						SEND_TO_Q("Имя и пароль через пробел : ", d);
						d->character.reset();

						return;
					}

					PLR_FLAGS(d->character).unset(PLR_MAILING);
					PLR_FLAGS(d->character).unset(PLR_WRITING);
					PLR_FLAGS(d->character).unset(PLR_CRYO);
					SEND_TO_Q("Персонаж с таким именем уже существует. Введите пароль : ", d);
					d->idle_tics = 0;
					STATE(d) = CON_PASSWORD;
				}
			}
			else  	// player unknown -- make new character
			{
				// еще одна проверка
				if (strlen(tmp_name) < (MIN_NAME_LENGTH))
				{
					SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
					return;
				}

				// Check for multiple creations of a character.
				if (!Valid_Name(tmp_name) || _parse_name(tmp_name, tmp_name))
				{
					SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
					return;
				}

				if (cmp_ptable_by_name(tmp_name, MIN_NAME_LENGTH) >= 0)
				{
					SEND_TO_Q
					("Первые символы вашего имени совпадают с уже существующим персонажем.\r\n"
					 "Для исключения разных недоразумений вам необходимо выбрать другое имя.\r\n"
					 "Имя  : ", d);
					return;
				}

				d->character->set_pc_name(CAP(tmp_name));
				d->character->player_data.PNames[0] = std::string(CAP(tmp_name));
				SEND_TO_Q(name_rules, d);
				sprintf(buf, "Вы действительно выбрали имя  %s [ Y(Д) / N(Н) ]? ", tmp_name);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_NAME_CNFRM;
			}
		}
		break;

	case CON_NAME_CNFRM:	// wait for conf. of new name
		if (UPPER(*arg) == 'Y' || UPPER(*arg) == 'Д')
		{
			if (ban->is_banned(d->host) >= BanList::BAN_NEW)
			{
				sprintf(buf, "Попытка создания персонажа %s отклонена для [%s] (siteban)",
						GET_PC_NAME(d->character), d->host);
				mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
				SEND_TO_Q("Извините, создание нового персонажа для вашего IP !!! ЗАПРЕЩЕНО !!!\r\n", d);
				STATE(d) = CON_CLOSE;
				return;
			}

			if (circle_restrict)
			{
				SEND_TO_Q("Извините, вы не можете создать новый персонаж в настоящий момент.\r\n", d);
				sprintf(buf, "Попытка создания нового персонажа %s отклонена для [%s] (wizlock)",
						GET_PC_NAME(d->character), d->host);
				mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
				STATE(d) = CON_CLOSE;
				return;
			}

			switch (NewNames::auto_authorize(d))
			{
			case NewNames::AUTO_ALLOW:
				sprintf(buf, "Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
						GET_PAD(d->character, 1));
				SEND_TO_Q(buf, d);
				STATE(d) = CON_NEWPASSWD;
				return;

			case NewNames::AUTO_BAN:
				STATE(d) = CON_CLOSE;
				return;

			default:
				break;
			}

			SEND_TO_Q("Ваш пол [ М(M)/Ж(F) ]? ", d);
			STATE(d) = CON_QSEX;
			return;

		}
		else if (UPPER(*arg) == 'N' || UPPER(*arg) == 'Н')
		{
			SEND_TO_Q("Итак, чего изволите? Учтите, бананов нет :)\r\n" "Имя : ", d);
			d->character->set_pc_name(0);
			STATE(d) = CON_GET_NAME;
		}
		else
		{
			SEND_TO_Q("Ответьте Yes(Да) or No(Нет) : ", d);
		}
		break;

	case CON_NEW_CHAR:
		if (!*arg)
		{
			STATE(d) = CON_CLOSE;
			return;
		}

		if (!d->character)
		{
			CreateChar(d);
		}

		if (_parse_name(arg, tmp_name) ||
				strlen(tmp_name) < MIN_NAME_LENGTH ||
				strlen(tmp_name) > MAX_NAME_LENGTH ||
				!Is_Valid_Name(tmp_name) || fill_word(tmp_name) || reserved_word(tmp_name))
		{
			SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
			return;
		}

		player_i = load_char(tmp_name, d->character.get());
		if (player_i > -1)
		{
			if (PLR_FLAGGED(d->character, PLR_DELETED))
			{
				d->character.reset();
				CreateChar(d);
			}
			else
			{
				SEND_TO_Q("Такой персонаж уже существует. Выберите другое имя : ", d);
				d->character.reset();

				return;
			}
		}

		if (!Valid_Name(tmp_name))
		{
			SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
			return;
		}

		if (cmp_ptable_by_name(tmp_name, MIN_NAME_LENGTH + 1) >= 0)
		{
			SEND_TO_Q("Первые символы вашего имени совпадают с уже существующим персонажем.\r\n"
					  "Для исключения разных недоразумений вам необходимо выбрать другое имя.\r\n"
					  "Имя  : ", d);
			return;
		}

		d->character->set_pc_name(CAP(tmp_name));
		d->character->player_data.PNames[0] = std::string(CAP(tmp_name));
		if (ban->is_banned(d->host) >= BanList::BAN_NEW)
		{
			sprintf(buf, "Попытка создания персонажа %s отклонена для [%s] (siteban)",
					GET_PC_NAME(d->character), d->host);
			mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
			SEND_TO_Q("Извините, создание нового персонажа для вашего IP !!!ЗАПРЕЩЕНО!!!\r\n", d);
			STATE(d) = CON_CLOSE;
			return;
		}

		if (circle_restrict)
		{
			SEND_TO_Q("Извините, вы не можете создать новый персонаж в настоящий момент.\r\n", d);
			sprintf(buf,
					"Попытка создания нового персонажа %s отклонена для [%s] (wizlock)",
					GET_PC_NAME(d->character), d->host);
			mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
			STATE(d) = CON_CLOSE;
			return;
		}

		switch (NewNames::auto_authorize(d))
		{
		case NewNames::AUTO_ALLOW:
			sprintf(buf,
					"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
					GET_PAD(d->character, 1));
			SEND_TO_Q(buf, d);
			STATE(d) = CON_NEWPASSWD;
			return;

		case NewNames::AUTO_BAN:
			d->character.reset();
			SEND_TO_Q("Выберите другое имя : ", d);
			return;

		default:
			break;
		}

		SEND_TO_Q("Ваш пол [ М(M)/Ж(F) ]? ", d);
		STATE(d) = CON_QSEX;
		return;

	case CON_PASSWORD:	// get pwd for known player
		/*
		 * To really prevent duping correctly, the player's record should
		 * be reloaded from disk at this point (after the password has been
		 * typed).  However I'm afraid that trying to load a character over
		 * an already loaded character is going to cause some problem down the
		 * road that I can't see at the moment.  So to compensate, I'm going to
		 * (1) add a 15 or 20-second time limit for entering a password, and (2)
		 * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
		 */

		SEND_TO_Q("\r\n", d);

		if (!*arg)
		{
			STATE(d) = CON_CLOSE;
		}
		else
		{
			if (!Password::compare_password(d->character.get(), arg))
			{
				sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
				mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
				GET_BAD_PWS(d->character)++;
				d->character->save_char();
				if (++(d->bad_pws) >= max_bad_pws)  	// 3 strikes and you're out.
				{
					SEND_TO_Q("Неверный пароль... Отсоединяемся.\r\n", d);
					STATE(d) = CON_CLOSE;
				}
				else
				{
					SEND_TO_Q("Неверный пароль.\r\nПароль : ", d);
				}
				return;
			}
			DoAfterPassword(d);
		}
		break;

	case CON_NEWPASSWD:
	case CON_CHPWD_GETNEW:
		if (!Password::check_password(d->character.get(), arg))
		{
			sprintf(buf, "\r\n%s\r\n", Password::BAD_PASSWORD);
			SEND_TO_Q(buf, d);
			SEND_TO_Q("Пароль : ", d);
			return;
		}

		Password::set_password(d->character.get(), arg);

		SEND_TO_Q("\r\nПовторите пароль, пожалуйста : ", d);
		if (STATE(d) == CON_NEWPASSWD)
		{
			STATE(d) = CON_CNFPASSWD;
		}
		else
		{
			STATE(d) = CON_CHPWD_VRFY;
		}

		break;

	case CON_CNFPASSWD:
	case CON_CHPWD_VRFY:
		if (!Password::compare_password(d->character.get(), arg))
		{
			SEND_TO_Q("\r\nПароли не соответствуют... повторим.\r\n", d);
			SEND_TO_Q("Пароль: ", d);
			if (STATE(d) == CON_CNFPASSWD)
			{
				STATE(d) = CON_NEWPASSWD;
			}
			else
			{
				STATE(d) = CON_CHPWD_GETNEW;
			}
			return;
		}

		if (STATE(d) == CON_CNFPASSWD)
		{
			GET_KIN(d->character) = 0; // added by WorM: Выставляем расу в Русич(коммент выше)
        		SEND_TO_Q(class_menu, d);
			SEND_TO_Q("\r\nВаша профессия (Для более полной информации вы можете набрать"
				  " \r\nсправка <интересующая профессия>): ", d);
			STATE(d) = CON_QCLASS;
		}
		else
		{
			sprintf(buf, "%s заменил себе пароль.", GET_NAME(d->character));
			add_karma(d->character.get(), buf, "");
			d->character->save_char();
			SEND_TO_Q("\r\nГотово.\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
		}

		break;

	case CON_QSEX:		// query sex of new user
		if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q("\r\nВаш пол [ М(M)/Ж(F) ]? ", d);
			STATE(d) = CON_QSEX;
			return;
		}

		switch (UPPER(*arg))
		{
		case 'М':
		case 'M':
			GET_SEX(d->character) = ESex::SEX_MALE;
			break;

		case 'Ж':
		case 'F':
			GET_SEX(d->character) = ESex::SEX_FEMALE;
			break;

		default:
			SEND_TO_Q("Это может быть и пол, но явно не ваш :)\r\n" "А какой у ВАС пол? ", d);
			return;
		}
		SEND_TO_Q("Проверьте правильность склонения имени. В случае ошибки введите свой вариант.\r\n", d);
		GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 1, tmp_name);
		sprintf(buf, "Имя в родительном падеже (меч КОГО?) [%s]: ", tmp_name);
		SEND_TO_Q(buf, d);
		STATE(d) = CON_NAME2;
		return;

	case CON_QKIN:		// query rass
		if (pre_help(d->character.get(), arg))
		{
            SEND_TO_Q("\r\nКакой народ вам ближе по духу:\r\n", d);
            SEND_TO_Q(string(PlayerRace::ShowKinsMenu()).c_str(), d);
			SEND_TO_Q("\r\nПлемя: ", d);
			STATE(d) = CON_QKIN;
			return;
		}

        load_result = PlayerRace::CheckKin(arg);
		if (load_result == KIN_UNDEFINED)
		{
			SEND_TO_Q("Стыдно не помнить предков.\r\n"
					  "Какое Племя вам ближе по духу? ", d);
			return;
		}

		GET_KIN(d->character) = load_result;
		/*
		Ахтунг-партизанен!
		Пока что убраны все вызовы парсилок _классов_ для отличных от русичей _рас_.
		Сами парсилки и списки классов оставлены для потомков.
		Проверка тоже убрана, так что при создании перса другой расы ему предложат выбрать "русские" классы.
		Теоретически это конечно неправильно, но я сомневаюсь, что в ближайшем будущем кто-то станет доделывать расы.
		Если же такой садомазо найдется, то для него это всеи пишется.
		В таком варианте надо в описания _рас_ в файле playerraces.xml
		Ввести список доступных расе классов. И уже от этого списка плясать с названиями и парсом, а не городить все в 3 экземплярах
		Сами классы при этом из кода можно и не выносить ж)
		Sventovit
		 */
        SEND_TO_Q(class_menu, d);
		SEND_TO_Q("\r\nВаша профессия (Для более полной информации вы можете набрать"
				  " \r\nсправка <интересующая профессия>): ", d);
		STATE(d) = CON_QCLASS;
		break;

	case CON_RELIGION:	// query religion of new user
		if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q(religion_menu, d);
			SEND_TO_Q("\n\rРелигия :", d);
			STATE(d) = CON_RELIGION;
			return;
		}

		switch (UPPER(*arg))
		{
		case 'Я':
		case 'З':
		case 'P':
			if (class_religion[(int) GET_CLASS(d->character)] == RELIGION_MONO)
			{
				SEND_TO_Q
				("Персонаж выбранной вами профессии не желает быть язычником!\r\n"
				 "Так каким Богам вы хотите служить? ", d);
				return;
			}
			GET_RELIGION(d->character) = RELIGION_POLY;
			break;

		case 'Х':
		case 'C':
			if (class_religion[(int) GET_CLASS(d->character)] == RELIGION_POLY)
			{
				SEND_TO_Q
				("Персонажу выбранной вами профессии противно христианство!\r\n"
				 "Так каким Богам вы хотите служить? ", d);
				return;
			}
			GET_RELIGION(d->character) = RELIGION_MONO;
			break;

		default:
			SEND_TO_Q("Атеизм сейчас не моден :)\r\n" "Так каким Богам вы хотите служить? ", d);
			return;
		}

		SEND_TO_Q("\r\nКакой род вам ближе всего по духу:\r\n", d);
		SEND_TO_Q(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
		sprintf(buf, "Для вашей профессией больше всего подходит %s", default_race[GET_CLASS(d->character)]);
		SEND_TO_Q(buf, d);
		SEND_TO_Q("\r\nИз чьих вы будете : ", d);
		STATE(d) = CON_RACE;

		break;

	case CON_QCLASS:
		if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q(class_menu, d);
			SEND_TO_Q("\r\nВаша профессия : ", d);
			STATE(d) = CON_QCLASS;
			return;
		}

		load_result = parse_class(*arg);
		if (load_result == CLASS_UNDEFINED)
		{
			SEND_TO_Q("\r\nЭто не профессия.\r\nПрофессия : ", d);
			return;
		}
		else
		{
			d->character->set_class(load_result);
		}

		SEND_TO_Q(religion_menu, d);
		SEND_TO_Q("\n\rРелигия :", d);
		STATE(d) = CON_RELIGION;
		break;

	case CON_QCLASSS:
		if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q(class_menu_step, d);
			SEND_TO_Q("\r\nВаша профессия : ", d);
			STATE(d) = CON_QCLASSS;
			return;
		}

		load_result = parse_class_step(*arg);

		if (load_result == CLASS_UNDEFINED)
		{
			SEND_TO_Q("\r\nЭто не профессия.\r\nПрофессия : ", d);
			return;
		}
		else
		{
			d->character->set_class(load_result);
		}

		SEND_TO_Q(religion_menu, d);
		SEND_TO_Q("\n\rРелигия :", d);
		STATE(d) = CON_RELIGION;

		break;

	case CON_QCLASSV:
		if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q(class_menu_vik, d);
			SEND_TO_Q("\r\nВаша профессия : ", d);
			STATE(d) = CON_QCLASSV;
			return;
		}

		load_result = parse_class_vik(*arg);

		if (load_result == CLASS_UNDEFINED)
		{
			SEND_TO_Q("\r\nЭто не профессия.\r\nПрофессия : ", d);
			return;
		}
		else
		{
			d->character->set_class(load_result);
		}

		SEND_TO_Q(religion_menu, d);
		SEND_TO_Q("\n\rРелигия:", d);
		STATE(d) = CON_RELIGION;

		break;

	case CON_RACE:		// query race
		if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q("Какой род вам ближе всего по духу:\r\n", d);
            SEND_TO_Q(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
			SEND_TO_Q("\r\nРод: ", d);
			STATE(d) = CON_RACE;
			return;
		}

        load_result = PlayerRace::CheckRace(GET_KIN(d->character), arg);

		if (load_result == RACE_UNDEFINED)
		{
			SEND_TO_Q("Стыдно не помнить предков.\r\n" "Какой род вам ближе всего? ", d);
			return;
		}

		GET_RACE(d->character) = load_result;
        SEND_TO_Q(string(BirthPlace::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(d->character),GET_RACE(d->character)))).c_str(), d);
        SEND_TO_Q("\r\nГде вы хотите начать свои приключения: ", d);
		STATE(d) = CON_BIRTHPLACE;

		break;

	case CON_BIRTHPLACE:
        if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q(string(BirthPlace::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(d->character),GET_RACE(d->character)))).c_str(), d);
			SEND_TO_Q("\r\nГде вы хотите начать свои приключения: ", d);
			STATE(d) = CON_BIRTHPLACE;
			return;
		}

        load_result = PlayerRace::CheckBirthPlace(GET_KIN(d->character), GET_RACE(d->character), arg);

		if (!BirthPlace::CheckId(load_result))
		{
			SEND_TO_Q("Не уверены? Бывает.\r\n"
					  "Подумайте еще разок, и выберите:", d);
			return;
		}

		GET_LOADROOM(d->character) = calc_loadroom(d->character.get(), load_result);

		roll_real_abils(d->character.get());
		SEND_TO_Q(genchar_help, d);
		SEND_TO_Q("\r\n\r\nНажмите любую клавишу.\r\n", d);
        STATE(d) = CON_ROLL_STATS;

        break;

	case CON_ROLL_STATS:
		if (pre_help(d->character.get(), arg))
		{
			genchar_disp_menu(d->character.get());
				STATE(d) = CON_ROLL_STATS;
			return;
		}

		switch (genchar_parse(d->character.get(), arg))
		{
		case GENCHAR_CONTINUE:
			genchar_disp_menu(d->character.get());
			break;
			default:
				SEND_TO_Q("\r\nВведите ваш E-mail"
				"\r\n(ВСЕ ВАШИ ПЕРСОНАЖИ ДОЛЖНЫ ИМЕТЬ ОДИНАКОВЫЙ E-mail)."
				"\r\nНа этот адрес вам будет отправлен код для подтверждения: ", d);
				STATE(d) = CON_GET_EMAIL;
			break;
		}
		break;

	case CON_GET_EMAIL:
		if (!*arg)
		{
			SEND_TO_Q("\r\nВаш E-mail : ", d);
			return;
		}
		else if (!valid_email(arg))
			{
			SEND_TO_Q("\r\nНекорректный E-mail!" "\r\nВаш E-mail :  ", d);
			return;
		}
		#ifdef TEST_BUILD			
		strncpy(GET_EMAIL(d->character), arg, 127);
		*(GET_EMAIL(d->character) + 127) = '\0';
		lower_convert(GET_EMAIL(d->character));
		DoAfterEmailConfirm(d);
		break;
		#endif
		{
			int random_number = number(1000000, 9999999);
			new_char_codes[d->character->get_pc_name()] = random_number;
			strncpy(GET_EMAIL(d->character), arg, 127);
			*(GET_EMAIL(d->character) + 127) = '\0';
			lower_convert(GET_EMAIL(d->character));
			std::string cmd_line = str(boost::format("python3 send_code.py %s %d &") % GET_EMAIL(d->character) % random_number);
			auto result = system(cmd_line.c_str());
			UNUSED_ARG(result);
			SEND_TO_Q("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
			STATE(d) = CON_RANDOM_NUMBER;
		}
		break;

	case CON_RMOTD:	// read CR after printing motd
		if (!check_dupes_email(d))
		{
			STATE(d) = CON_CLOSE;
			break;
		}

		do_entergame(d);

		break;

	case CON_RANDOM_NUMBER:
		{
			int code_rand = atoi(arg);

			if (new_char_codes.count(d->character->get_pc_name()) != 0)
			{
				if (new_char_codes[d->character->get_pc_name()] != code_rand)
				{
					SEND_TO_Q("\r\nВы ввели неправильный код, попробуйте еще раз.\r\n", d);
					break;
				}
				new_char_codes.erase(d->character->get_pc_name());
				DoAfterEmailConfirm(d);
				break;
			}

			if (new_loc_codes.count(GET_EMAIL(d->character)) == 0)
			{
				break;
			}

			if (new_loc_codes[GET_EMAIL(d->character)] != code_rand)
			{
				SEND_TO_Q("\r\nВы ввели неправильный код, попробуйте еще раз.\r\n", d);
				STATE(d) = CON_CLOSE;
				break;
			}

			new_loc_codes.erase(GET_EMAIL(d->character));
			add_logon_record(d);
			DoAfterPassword(d);

			break;
		}

	case CON_MENU:		// get selection from main menu
		switch (*arg)
		{
		case '0':
			SEND_TO_Q("\r\nДо встречи на земле Киевской.\r\n", d);

			if (GET_REMORT(d->character) == 0
				&& GET_LEVEL(d->character) <= 25
				&& !PLR_FLAGS(d->character).get(PLR_NODELETE))
			{
				int timeout = -1;
				for (int ci = 0; GET_LEVEL(d->character) > pclean_criteria[ci].level; ci++)
				{
					//if (GET_LEVEL(d->character) == pclean_criteria[ci].level)
					timeout = pclean_criteria[ci + 1].days;
				}
				if (timeout > 0)
				{
					time_t deltime = time(NULL) + timeout * 60 * 60 * 24;
					sprintf(buf, "В случае вашего отсутствия персонаж будет храниться до %s нашей эры :).\r\n",
							rustime(localtime(&deltime)));
					SEND_TO_Q(buf, d);
				}
			};

			STATE(d) = CON_CLOSE;

			break;

		case '1':
			if (!check_dupes_email(d))
			{
				STATE(d) = CON_CLOSE;
				break;
			}

			do_entergame(d);

			break;

		case '2':
			if (d->character->player_data.description != "")
			{
				SEND_TO_Q("Ваше ТЕКУЩЕЕ описание:\r\n", d);
				SEND_TO_Q(d->character->player_data.description.c_str(), d);
				/*
				 * Don't free this now... so that the old description gets loaded
				 * as the current buffer in the editor.  Do setup the ABORT buffer
				 * here, however.
				 *
				 * free(d->character->player_data.description);
				 * d->character->player_data.description = NULL;
				 */
				d->backstr = str_dup(d->character->player_data.description.c_str());
			}

			SEND_TO_Q("Введите описание вашего героя, которое будет выводиться по команде <осмотреть>.\r\n", d);
			SEND_TO_Q("(/s сохранить /h помощь)\r\n", d);
			
			d->writer.reset(new DelegatedStdStringWriter(d->character->player_data.description));
			d->max_str = EXDSCR_LENGTH;
			STATE(d) = CON_EXDESC;

			break;

		case '3':
			page_string(d, background, 0);
			STATE(d) = CON_RMOTD;
			break;

		case '4':
			SEND_TO_Q("\r\nВведите СТАРЫЙ пароль : ", d);
			STATE(d) = CON_CHPWD_GETOLD;
			break;

		case '5':
			if (IS_IMMORTAL(d->character))
			{
				SEND_TO_Q("\r\nБоги бессмертны (с) Стрибог, просите чтоб пофризили :)))\r\n", d);
				SEND_TO_Q(MENU, d);
				break;
			}

			if (PLR_FLAGGED(d->character, PLR_HELLED)
				|| PLR_FLAGGED(d->character, PLR_FROZEN))
			{
				SEND_TO_Q("\r\nВы находитесь в АДУ!!! Амнистии подобным образом не будет.\r\n", d);
				SEND_TO_Q(MENU, d);
				break;
			}

			if (GET_REMORT(d->character) > 5)
			{
				SEND_TO_Q("\r\nНельзя удалить себя достигнув шестого перевоплощения.\r\n", d);
				SEND_TO_Q(MENU, d);
				break;
			}

			SEND_TO_Q("\r\nДля подтверждения введите свой пароль : ", d);
			STATE(d) = CON_DELCNF1;

			break;

		case '6':
			if (IS_IMMORTAL(d->character))
			{
				SEND_TO_Q("\r\nВам это ни к чему...\r\n", d);
				SEND_TO_Q(MENU, d);
				STATE(d) = CON_MENU;
			}
			else
			{
				ResetStats::print_menu(d);
				STATE(d) = CON_MENU_STATS;
			}
			break;

		case '7':
			if (!PRF_FLAGGED(d->character, PRF_BLIND))
			{
				PRF_FLAGS(d->character).set(PRF_BLIND);
				SEND_TO_Q("\r\nСпециальный режим слепого игрока ВКЛЮЧЕН.\r\n", d);
				SEND_TO_Q(MENU, d);
				STATE(d) = CON_MENU;
			}
			else
			{
				PRF_FLAGS(d->character).unset(PRF_BLIND);
				SEND_TO_Q("\r\nСпециальный режим слепого игрока ВЫКЛЮЧЕН.\r\n", d);
				SEND_TO_Q(MENU, d);
				STATE(d) = CON_MENU;
			}

			break;
		case '8':
			d->character->get_account()->show_list_players(d);
			break;

		default:
			SEND_TO_Q("\r\nЭто не есть правильный ответ!\r\n", d);
			SEND_TO_Q(MENU, d);

			break;
		}

		break;

	case CON_CHPWD_GETOLD:
		if (!Password::compare_password(d->character.get(), arg))
		{
			SEND_TO_Q("\r\nНеверный пароль.\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
		}
		else
		{
			SEND_TO_Q("\r\nВведите НОВЫЙ пароль : ", d);
			STATE(d) = CON_CHPWD_GETNEW;
		}

		return;

	case CON_DELCNF1:
		if (!Password::compare_password(d->character.get(), arg))
		{
			SEND_TO_Q("\r\nНеверный пароль.\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
		}
		else
		{
			SEND_TO_Q("\r\n!!! ВАШ ПЕРСОНАЖ БУДЕТ УДАЛЕН !!!\r\n"
					  "Вы АБСОЛЮТНО В ЭТОМ УВЕРЕНЫ?\r\n\r\n"
					  "Наберите \"YES / ДА\" для подтверждения: ", d);
			STATE(d) = CON_DELCNF2;
		}

		break;

	case CON_DELCNF2:
		if (!strcmp(arg, "yes")
			|| !strcmp(arg, "YES")
			|| !strcmp(arg, "да")
			|| !strcmp(arg, "ДА"))
		{
			if (PLR_FLAGGED(d->character, PLR_FROZEN))
			{
				SEND_TO_Q("Вы решились на суицид, но Боги остановили вас.\r\n", d);
				SEND_TO_Q("Персонаж не удален.\r\n", d);
				STATE(d) = CON_CLOSE;
				return;
			}
			if (GET_LEVEL(d->character) >= LVL_GRGOD)
				return;
			delete_char(GET_NAME(d->character));
			sprintf(buf, "Персонаж '%s' удален!\r\n" "До свидания.\r\n", GET_NAME(d->character));
			SEND_TO_Q(buf, d);
			sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
			mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
			d->character->get_account()->remove_player(GetUniqueByName(GET_NAME(d->character)));
			STATE(d) = CON_CLOSE;
			return;
		}
		else
		{
			SEND_TO_Q("\r\nПерсонаж не удален.\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
		}
		break;

	case CON_NAME2:
		skip_spaces(&arg);
		if (strlen(arg) == 0)
		{
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 1, arg);
		}
		if (!_parse_name(arg, tmp_name)
			&& strlen(tmp_name) >= MIN_NAME_LENGTH
			&& strlen(tmp_name) <= MAX_NAME_LENGTH
			&& !strn_cmp(tmp_name, GET_PC_NAME(d->character), std::min<size_t>(MIN_NAME_LENGTH, strlen(GET_PC_NAME(d->character)) - 1)))
		{
			d->character->player_data.PNames[1] = std::string(CAP(tmp_name));
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 2, tmp_name);
			sprintf(buf, "Имя в дательном падеже (отправить КОМУ?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
			STATE(d) = CON_NAME3;
		}
		else
		{
			SEND_TO_Q("Некорректно.\r\n", d);
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 1, tmp_name);
			sprintf(buf, "Имя в родительном падеже (меч КОГО?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
		};
		break;

	case CON_NAME3:
		skip_spaces(&arg);

		if (strlen(arg) == 0)
		{
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 2, arg);
		}

		if (!_parse_name(arg, tmp_name)
			&& strlen(tmp_name) >= MIN_NAME_LENGTH
			&& strlen(tmp_name) <= MAX_NAME_LENGTH
			&& !strn_cmp(tmp_name, GET_PC_NAME(d->character), std::min<size_t>(MIN_NAME_LENGTH, strlen(GET_PC_NAME(d->character)) - 1)))
		{
			d->character->player_data.PNames[2] = std::string(CAP(tmp_name));
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 3, tmp_name);
			sprintf(buf, "Имя в винительном падеже (ударить КОГО?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
			STATE(d) = CON_NAME4;
		}
		else
		{
			SEND_TO_Q("Некорректно.\r\n", d);
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 2, tmp_name);
			sprintf(buf, "Имя в дательном падеже (отправить КОМУ?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
		};

		break;

	case CON_NAME4:
		skip_spaces(&arg);

		if (strlen(arg) == 0)
		{
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 3, arg);
		}

		if (!_parse_name(arg, tmp_name)
			&& strlen(tmp_name) >= MIN_NAME_LENGTH
			&& strlen(tmp_name) <= MAX_NAME_LENGTH
			&& !strn_cmp(tmp_name, GET_PC_NAME(d->character), std::min<size_t>(MIN_NAME_LENGTH, strlen(GET_PC_NAME(d->character)) - 1)))
		{
			d->character->player_data.PNames[3] = std::string(CAP(tmp_name));
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 4, tmp_name);
			sprintf(buf, "Имя в творительном падеже (сражаться с КЕМ?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
			STATE(d) = CON_NAME5;
		}
		else
		{
			SEND_TO_Q("Некорректно.\n\r", d);
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 3, tmp_name);
			sprintf(buf, "Имя в винительном падеже (ударить КОГО?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
		};

		break;

	case CON_NAME5:
		skip_spaces(&arg);
		if (strlen(arg) == 0)
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 4, arg);
		if (!_parse_name(arg, tmp_name) &&
				strlen(tmp_name) >= MIN_NAME_LENGTH && strlen(tmp_name) <= MAX_NAME_LENGTH &&
				!strn_cmp(tmp_name, GET_PC_NAME(d->character), std::min<size_t>(MIN_NAME_LENGTH, strlen(GET_PC_NAME(d->character)) - 1))
		   )
		{
			d->character->player_data.PNames[4] = std::string(CAP(tmp_name));
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 5, tmp_name);
			sprintf(buf, "Имя в предложном падеже (говорить о КОМ?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
			STATE(d) = CON_NAME6;
		}
		else
		{
			SEND_TO_Q("Некорректно.\n\r", d);
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 4, tmp_name);
			sprintf(buf, "Имя в творительном падеже (сражаться с КЕМ?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
		};
		break;
	case CON_NAME6:
		skip_spaces(&arg);
		if (strlen(arg) == 0)
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 5, arg);
		if (!_parse_name(arg, tmp_name) &&
				strlen(tmp_name) >= MIN_NAME_LENGTH && strlen(tmp_name) <= MAX_NAME_LENGTH &&
				!strn_cmp(tmp_name, GET_PC_NAME(d->character), std::min<size_t>(MIN_NAME_LENGTH, strlen(GET_PC_NAME(d->character)) - 1))
		   )
		{
			d->character->player_data.PNames[5] = std::string(CAP(tmp_name));
			sprintf(buf,
					"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
					GET_PAD(d->character, 1));
			SEND_TO_Q(buf, d);
			STATE(d) = CON_NEWPASSWD;
		}
		else
		{
			SEND_TO_Q("Некорректно.\n\r", d);
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 5, tmp_name);
			sprintf(buf, "Имя в предложном падеже (говорить о КОМ?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
		};
		break;

	case CON_CLOSE:
		break;

	case CON_RESET_STATS:
		if (pre_help(d->character.get(), arg))
		{
			return;
		}

		switch (genchar_parse(d->character.get(), arg))
		{
		case GENCHAR_CONTINUE:
			genchar_disp_menu(d->character.get());
			break;

		default:
			// после перераспределения и сейва в genchar_parse стартовых статов надо учесть морты и славу
			GloryMisc::recalculate_stats(d->character.get());
			// статы срезетили и новые выбрали
			sprintf(buf, "\r\n%sБлагодарим за сотрудничество. Ж)%s\r\n",
					CCIGRN(d->character, C_SPR), CCNRM(d->character, C_SPR));
			SEND_TO_Q(buf, d);

            // Проверяем корректность статов
            // Если что-то некорректно, функция проверки сама вернет чара на доработку.
            if (!ValidateStats(d))
			{
				return;
			}

            SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
            STATE(d) = CON_RMOTD;
		}

		break;

    case CON_RESET_KIN:
		if (pre_help(d->character.get(), arg))
		{
            SEND_TO_Q("\r\nКакой народ вам ближе по духу:\r\n", d);
            SEND_TO_Q(string(PlayerRace::ShowKinsMenu()).c_str(), d);
			SEND_TO_Q("\r\nПлемя: ", d);
			STATE(d) = CON_RESET_KIN;
			return;
		}

        load_result = PlayerRace::CheckKin(arg);

		if (load_result == KIN_UNDEFINED)
		{
			SEND_TO_Q("Стыдно не помнить предков.\r\n"
					  "Какое Племя вам ближе по духу? ", d);
			return;
		}

		GET_KIN(d->character) = load_result;

        if (!ValidateStats(d))
		{
			return;
		}

        SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
        STATE(d) = CON_RMOTD;
        break;

    case CON_RESET_RACE:
		if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q("Какой род вам ближе всего по духу:\r\n", d);
            SEND_TO_Q(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
			SEND_TO_Q("\r\nРод: ", d);
			STATE(d) = CON_RESET_RACE;
			return;
		}

        load_result = PlayerRace::CheckRace(GET_KIN(d->character), arg);

		if (load_result == RACE_UNDEFINED)
		{
			SEND_TO_Q("Стыдно не помнить предков.\r\n" "Какой род вам ближе всего? ", d);
			return;
		}

		GET_RACE(d->character) = load_result;

        if (!ValidateStats(d))
		{
			return;
		}

		// способности нового рода проставятся дальше в do_entergame
        SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
        STATE(d) = CON_RMOTD;

        break;

	case CON_MENU_STATS:
		ResetStats::parse_menu(d, arg);
		break;

	case CON_RESET_RELIGION:
		if (pre_help(d->character.get(), arg))
		{
			SEND_TO_Q(religion_menu, d);
			SEND_TO_Q("\n\rРелигия :", d);
			return;
		}

		switch (UPPER(*arg))
		{
		case 'Я':
		case 'З':
		case 'P':
			if (class_religion[(int) GET_CLASS(d->character)] == RELIGION_MONO)
			{
				SEND_TO_Q
				("Персонаж выбранной вами профессии не желает быть язычником!\r\n"
				 "Так каким Богам вы хотите служить? ", d);
				return;
			}
			GET_RELIGION(d->character) = RELIGION_POLY;
			break;

		case 'Х':
		case 'C':
			if (class_religion[(int) GET_CLASS(d->character)] == RELIGION_POLY)
			{
				SEND_TO_Q ("Персонажу выбранной вами профессии противно христианство!\r\n"
				 "Так каким Богам вы хотите служить? ", d);
				return;
			}

			GET_RELIGION(d->character) = RELIGION_MONO;

			break;

		default:
			SEND_TO_Q("Атеизм сейчас не моден :)\r\n" "Так каким Богам вы хотите служить? ", d);
			return;
		}

		if (!ValidateStats(d))
		{
			return;
		}

		SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
		STATE(d) = CON_RMOTD;

		break;

	default:
		log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
			STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
		STATE(d) = CON_DISCONNECT;	// Safest to do.

		break;
	}
}

// берем из первой строки одно слово или подстроку в кавычках, результат удаляется из buffer
void GetOneParam(std::string & in_buffer, std::string & out_buffer)
{
	std::string::size_type beg_idx = 0, end_idx = 0;
	beg_idx = in_buffer.find_first_not_of(' ');

	if (beg_idx != std::string::npos)
	{
		// случай с кавычками
		if (in_buffer[beg_idx] == '\'')
		{
			if (std::string::npos != (beg_idx = in_buffer.find_first_not_of('\'', beg_idx)))
			{
				if (std::string::npos == (end_idx = in_buffer.find_first_of('\'', beg_idx)))
				{
					out_buffer = in_buffer.substr(beg_idx);
					in_buffer.clear();
				}
				else
				{
					out_buffer = in_buffer.substr(beg_idx, end_idx - beg_idx);
					in_buffer.erase(0, ++end_idx);
				}
			}
			// случай с одним параметром через пробел
		}
		else
		{
			if (std::string::npos != (beg_idx = in_buffer.find_first_not_of(' ', beg_idx)))
			{
				if (std::string::npos == (end_idx = in_buffer.find_first_of(' ', beg_idx)))
				{
					out_buffer = in_buffer.substr(beg_idx);
					in_buffer.clear();
				}
				else
				{
					out_buffer = in_buffer.substr(beg_idx, end_idx - beg_idx);
					in_buffer.erase(0, end_idx);
				}
			}
		}
		return;
	}

	in_buffer.clear();
	out_buffer.clear();
}

// регистронезависимое сравнение двух строк по длине первой, флаг - для учета длины строк (неравенство)
bool CompareParam(const std::string & buffer, const char *arg, bool full)
{
	if (!arg || !*arg || buffer.empty() || (full && buffer.length() != strlen(arg)))
		return 0;

	std::string::size_type i;
	for (i = 0; i != buffer.length() && *arg; ++i, ++arg)
		if (LOWER(buffer[i]) != LOWER(*arg))
			return (0);

	if (i == buffer.length())
		return (1);
	else
		return (0);
}

// тоже самое с обоими аргументами стринг
bool CompareParam(const std::string & buffer, const std::string & buffer2, bool full)
{
	if (buffer.empty() || buffer2.empty()
			|| (full && buffer.length() != buffer2.length()))
		return 0;

	std::string::size_type i;
	for (i = 0; i != buffer.length() && i != buffer2.length(); ++i)
		if (LOWER(buffer[i]) != LOWER(buffer2[i]))
			return (0);

	if (i == buffer.length())
		return (1);
	else
		return (0);
}

// ищет дескриптор игрока(онлайн состояние) по его УИДу
DESCRIPTOR_DATA *DescByUID(int uid)
{
	DESCRIPTOR_DATA *d = 0;

	for (d = descriptor_list; d; d = d->next)
		if (d->character && GET_UNIQUE(d->character) == uid)
			break;
	return (d);
}

/**
* Ищем дескриптор игрока (онлайн) по ИД чара (в основном для писем, т.к. вообще уиды на пользовать)
* \param id - ид, который ищем
* \param playing - 0 если ищем игрока в любом состоянии, 1 (дефолт) если ищем только незанятых
*/
DESCRIPTOR_DATA* get_desc_by_id(long id, bool playing)
{
	DESCRIPTOR_DATA *d = 0;

	if (playing)
	{
		for (d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && GET_IDNUM(d->character) == id)
				break;
	}
	else
	{
		for (d = descriptor_list; d; d = d->next)
			if (d->character && GET_IDNUM(d->character) == id)
				break;
	}
	return d;
}

/**
* ищет УИД игрока по его имени, второй необязательный параметр - учитывать или нет БОГОВ
* проверка уида на -1 нужна потому, что при делете чара (через меню например), его имя
* останется в плеер-листе, но все остальные параметры будут -1
* TODO: т.к. за все это время понадобилось только при добавлении в пкл - встает вопрос нафига было это городить...
* \param god по умолчанию = 0
* \return >0 - уид чара, 0 - не нашли, -1 - нашли, но это оказался бог (только при god = true)
*/
long GetUniqueByName(const std::string & name, bool god)
{
	for (std::size_t i = 0; i < player_table.size(); ++i)
	{
		if (!str_cmp(player_table[i].name(), name) && player_table[i].unique != -1)
		{
			if (!god)
				return player_table[i].unique;
			else
			{
				if (player_table[i].level < LVL_IMMORT)
					return player_table[i].unique;
				else
					return -1;
			}

		}
	}
	return 0;
}

// ищет имя игрока по его УИДу, второй необязательный параметр - учитывать или нет БОГОВ
std::string GetNameByUnique(long unique, bool god)
{
	std::string empty;

	for (std::size_t i = 0; i < player_table.size(); ++i)
	{
		if (player_table[i].unique == unique)
		{
			if (!god)
			{
				return player_table[i].name();
			}
			else
			{
				if (player_table[i].level < LVL_IMMORT)
				{
					return player_table[i].name();
				}
				else
				{
					return empty;
				}
			}
		}
	}

	return empty;
}

// замена в name русских символов на англ в нижнем регистре (для файлов)
void CreateFileName(std::string &name)
{
	for (unsigned i = 0; i != name.length(); ++i)
		name[i] = LOWER(AtoL(name[i]));
}

// вывод экспы аля диабла
std::string ExpFormat(long long exp)
{
	std::string prefix;
	if (exp < 0)
	{
		exp = -exp;
		prefix = "-";
	}
	if (exp < 1000000)
		return (prefix + boost::lexical_cast<std::string>(exp));
	else if (exp < 1000000000)
		return (prefix + boost::lexical_cast<std::string>(exp / 1000) + " тыс");
	else if (exp < 1000000000000LL)
		return (prefix + boost::lexical_cast<std::string>(exp / 1000000) + " млн");
	else
		return (prefix + boost::lexical_cast<std::string>(exp / 1000000000LL) + " млрд");
}

// * Конвертация входной строки в нижний регистр
void lower_convert(std::string& text)
{
	for (std::string::iterator it = text.begin(); it != text.end(); ++it)
		*it = LOWER(*it);
}

// * Конвертация входной строки в нижний регистр
void lower_convert(char* text)
{
	while (*text)
	{
		*text = LOWER(*text);
		text++;
	}
}

// * Конвертация имени в нижний регистр + первый сивмол в верхний (для единообразного поиска в контейнерах)
void name_convert(std::string& text)
{
	if (!text.empty())
	{
		lower_convert(text);
		*text.begin() = UPPER(*text.begin());
	}
}

// * Генерация списка неодобренных титулов и имен и вывод их имму
bool single_god_invoice(CHAR_DATA* ch)
{
	bool hasMessages = false;
	hasMessages |= TitleSystem::show_title_list(ch);
	hasMessages |= NewNames::show(ch);
	return hasMessages;
}

// * Поиск незанятых иммов онлайн для вывода им неодобренных титулов и имен раз в 5 минут
void god_work_invoice()
{
	for (DESCRIPTOR_DATA* d = descriptor_list; d; d = d->next)
	{
		if (d->character && STATE(d) == CON_PLAYING)
		{
			if (IS_IMMORTAL(d->character)
				|| GET_GOD_FLAG(d->character, GF_DEMIGOD))
			{
				single_god_invoice(d->character.get());
			}
		}
	}
}

// * Вывод оповещений о новых сообщениях на досках, письмах, (неодобренных имен и титулов для иммов) при логине и релогине
bool login_change_invoice(CHAR_DATA* ch)
{
	bool hasMessages = false;

	hasMessages |= Boards::Static::LoginInfo(ch);

	if (IS_IMMORTAL(ch))
		hasMessages |= single_god_invoice(ch);

	if (mail::has_mail(ch->get_uid()))
	{
		hasMessages = true;
		send_to_char("&RВас ожидает письмо. ЗАЙДИТЕ НА ПОЧТУ!&n\r\n", ch);
	}
	if (Parcel::has_parcel(ch))
	{
		hasMessages = true;
		send_to_char("&RВас ожидает посылка. ЗАЙДИТЕ НА ПОЧТУ!&n\r\n", ch);
	}
	hasMessages |= Depot::show_purged_message(ch);
	if (CLAN(ch))
	{
		hasMessages |= CLAN(ch)->print_mod(ch);
	}

	return hasMessages;
}

// спам-контроль для команды кто и списка по дружинам
// работает аналогично восстановлению и расходованию маны у волхвов
// константы пока определены через #define в interpreter.h
// возвращает истину, если спамконтроль сработал и игроку придется подождать
bool who_spamcontrol(CHAR_DATA *ch, unsigned short int mode = WHO_LISTALL)
{
	int cost = 0;
	time_t ctime;

	if (IS_IMMORTAL(ch))
		return false;

	ctime = time(0);

	switch (mode)
	{
		case WHO_LISTALL:
			cost = WHO_COST;
			break;
		case WHO_LISTNAME:
			cost = WHO_COST_NAME;
			break;
		case WHO_LISTCLAN:
			cost = WHO_COST_CLAN;
			break;
	}

	int mana = ch->get_who_mana();
	int last = ch->get_who_last();

#ifdef WHO_DEBUG
	send_to_char(boost::str(boost::format("\r\nСпам-контроль:\r\n  было маны: %u, расход: %u\r\n") % ch->get_who_mana() % cost).c_str(), ch);
#endif

	// рестим ману, в БД скорость реста маны удваивается
	mana = MIN(WHO_MANA_MAX, mana + (ctime - last) * WHO_MANA_REST_PER_SECOND + (ctime - last) * WHO_MANA_REST_PER_SECOND * (RENTABLE(ch) ? 1 : 0));

#ifdef WHO_DEBUG
	send_to_char(boost::str(boost::format("  прошло %u с, восстановили %u, мана после регена: %u\r\n") %
	                                      (ctime - last) % (mana - ch->get_who_mana()) % mana).c_str(), ch);
#endif

	ch->set_who_mana(mana);
	ch->set_who_last(ctime);

	if (mana < cost)
	{
		send_to_char("Запрос обрабатывается, ожидайте...\r\n", ch);
		return true;
	}
	else
	{
		mana -= cost;
		ch->set_who_mana(mana);
	}
#ifdef WHO_DEBUG
	send_to_char(boost::str(boost::format("  осталось маны: %u\r\n") % mana).c_str(), ch);
#endif
	return false;
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
