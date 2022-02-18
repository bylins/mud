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

#include "act_movement.h"
#include "cmd_god/ban.h"
#include "boards/boards.h"
#include "entities/char_data.h"
#include "entities/char_player.h"
#include "entities/world_characters.h"
#include "communication/insult.h"
#include "cmd_god/stat.h"
#include "cmd_god/godtest.h"
#include "cmd/follow.h"
#include "cmd/hire.h"
#include "cmd/mercenary.h"
#include "cmd/order.h"
#include "cmd/retreat.h"
#include "cmd/telegram.h"
#include "cmd/learn.h"
#include "cmd/forget.h"
#include "cmd/memorize.h"
#include "cmd/flee.h"
#include "cmd/create.h"
#include "cmd/mixture.h"
#include "cmd/cast.h"
#include "cmd/employ.h"
#include "comm.h"
#include "constants.h"
#include "crafts/craft_commands.h"
#include "crafts/jewelry.h"
#include "db.h"
#include "depot.h"
#include "dg_script/dg_scripts.h"
#include "feats.h"
#include "fightsystem/assist.h"
#include "fightsystem/mobact.h"
#include "fightsystem/pk.h"
#include "fightsystem/fight_start.h"
#include "genchar.h"
#include "game_mechanics/glory.h"
#include "game_mechanics/glory_const.h"
#include "game_mechanics/glory_misc.h"
#include "handler.h"
#include "heartbeat_commands.h"
#include "house.h"
#include "crafts/item_creation.h"
#include "liquid.h"
#include "utils/logger.h"
#include "communication/mail.h"
#include "modify.h"
#include "name_list.h"
#include "game_mechanics/named_stuff.h"
#include "names.h"
#include "entities/obj_data.h"
#include "obj_prototypes.h"
#include "olc/olc.h"
#include "communication/parcel.h"
#include "administration/password.h"
#include "administration/privilege.h"
#include "entities/room_data.h"
#include "color.h"
#include "skills.h"
#include "skills/bash.h"
#include "skills/block.h"
#include "skills/chopoff.h"
#include "skills/disarm.h"
#include "skills/ironwind.h"
#include "skills/kick.h"
#include "skills/manadrain.h"
#include "skills/mighthit.h"
#include "skills/parry.h"
#include "skills/protect.h"
#include "skills/resque.h"
#include "skills/strangle.h"
#include "skills/stun.h"
#include "skills/stupor.h"
#include "skills/throw.h"
#include "skills/track.h"
#include "skills/turnundead.h"
#include "skills/warcry.h"
#include "skills/relocate.h"
#include "magic/spells.h"
#include "time.h"
#include "title.h"
#include "top.h"
#include "skills_info.h"

#if defined WITH_SCRIPTING
#include "scripting.hpp"
#endif
#include "entities/player_races.h"
#include "birthplaces.h"
#include "help.h"
#include "mapsystem.h"
#include "game_economics/ext_money.h"
#include "noob.h"
#include "reset_stats.h"
#include "game_mechanics/obj_sets.h"
#include "utils/utils.h"
#include "magic/magic_temp_spells.h"
#include "structs/structs.h"
#include "sysdep.h"
#include "conf.h"
#include "game_mechanics/bonus.h"
#include "utils/utils_debug.h"
#include "structs/global_objects.h"
#include "administration/accounts.h"
#include "fightsystem/pk.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <stdexcept>
#include <algorithm>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

extern RoomRnum r_mortal_start_room;
extern RoomRnum r_immort_start_room;
extern RoomRnum r_frozen_start_room;
extern RoomRnum r_helled_start_room;
extern RoomRnum r_named_start_room;
extern RoomRnum r_unreg_start_room;
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
extern DescriptorData *descriptor_list;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern IndexData *mob_index;
extern const char *default_race[];
extern void add_karma(CharData *ch, const char *punish, const char *reason);
extern struct PCCleanCriteria pclean_criteria[];
extern int rent_file_timeout;

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
void do_start(CharData *ch, int newbie);
ECharClass ParseClass(char arg);
int Valid_Name(char *newname);
int Is_Valid_Name(char *newname);
int Is_Valid_Dc(char *newname);
void read_aliases(CharData *ch);
void write_aliases(CharData *ch);
void read_saved_vars(CharData *ch);
void oedit_parse(DescriptorData *d, char *arg);
void redit_parse(DescriptorData *d, char *arg);
void zedit_parse(DescriptorData *d, char *arg);
void medit_parse(DescriptorData *d, char *arg);
void trigedit_parse(DescriptorData *d, char *arg);
int find_social(char *name);
extern int CheckProxy(DescriptorData *ch);
extern void check_max_hp(CharData *ch);
// local functions
int perform_dupe_check(DescriptorData *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct TextBlocksQueue *input_q, char *orig, struct alias_data *a);
int perform_alias(DescriptorData *d, char *orig);
int reserved_word(const char *argument);
int _parse_name(char *arg, char *name);
void add_logon_record(DescriptorData *d);
// prototypes for all do_x functions.
int find_action(char *cmd);
int do_social(CharData *ch, char *argument);
void init_warcry(CharData *ch);

void do_advance(CharData *ch, char *argument, int cmd, int subcmd);
void do_alias(CharData *ch, char *argument, int cmd, int subcmd);
void do_antigods(CharData *ch, char *argument, int cmd, int subcmd);
void do_at(CharData *ch, char *argument, int cmd, int subcmd);
void do_affects(CharData *ch, char *argument, int cmd, int subcmd);
void do_backstab(CharData *ch, char *argument, int cmd, int subcmd);
void do_ban(CharData *ch, char *argument, int cmd, int subcmd);
void do_beep(CharData *ch, char *argument, int cmd, int subcmd);
void do_cast(CharData *ch, char *argument, int cmd, int subcmd);
void do_warcry(CharData *ch, char *argument, int cmd, int subcmd);
void do_clanstuff(CharData *ch, char *argument, int cmd, int subcmd);
void do_create(CharData *ch, char *argument, int cmd, int subcmd);
void DoExpedientCut(CharData *ch, char *argument, int, int);
void do_mixture(CharData *ch, char *argument, int cmd, int subcmd);
void do_courage(CharData *ch, char *argument, int cmd, int subcmd);
void do_commands(CharData *ch, char *argument, int cmd, int subcmd);
void do_consider(CharData *ch, char *argument, int cmd, int subcmd);
void do_credits(CharData *ch, char *argument, int cmd, int subcmd);
void do_date(CharData *ch, char *argument, int cmd, int subcmd);
void do_dc(CharData *ch, char *argument, int cmd, int subcmd);
void do_diagnose(CharData *ch, char *argument, int cmd, int subcmd);
void do_display(CharData *ch, char *argument, int cmd, int subcmd);
void do_drink(CharData *ch, char *argument, int cmd, int subcmd);
void do_drunkoff(CharData *ch, char *argument, int cmd, int subcmd);
void do_features(CharData *ch, char *argument, int cmd, int subcmd);
void do_featset(CharData *ch, char *argument, int cmd, int subcmd);
void do_firstaid(CharData *ch, char *argument, int cmd, int subcmd);
void do_fire(CharData *ch, char *argument, int cmd, int subcmd);
void do_drop(CharData *ch, char *argument, int cmd, int subcmd);
void do_eat(CharData *ch, char *argument, int cmd, int subcmd);
void do_echo(CharData *ch, char *argument, int cmd, int subcmd);
void do_equipment(CharData *ch, char *argument, int cmd, int subcmd);
void do_examine(CharData *ch, char *argument, int cmd, int subcmd);
void do_remort(CharData *ch, char *argument, int cmd, int subcmd);
void do_remember_char(CharData *ch, char *argument, int cmd, int subcmd);
void do_exit(CharData *ch, char *argument, int cmd, int subcmd);
void do_exits(CharData *ch, char *argument, int cmd, int subcmd);
void do_horseon(CharData *ch, char *argument, int cmd, int subcmd);
void do_horseoff(CharData *ch, char *argument, int cmd, int subcmd);
void do_horseput(CharData *ch, char *argument, int cmd, int subcmd);
void do_horseget(CharData *ch, char *argument, int cmd, int subcmd);
void do_horsetake(CharData *ch, char *argument, int cmd, int subcmd);
void do_hidemove(CharData *ch, char *argument, int cmd, int subcmd);
void do_fit(CharData *ch, char *argument, int cmd, int subcmd);
void do_force(CharData *ch, char *argument, int cmd, int subcmd);
void do_extinguish(CharData *ch, char *argument, int cmd, int subcmd);
void do_forcetime(CharData *ch, char *argument, int cmd, int subcmd);
void do_glory(CharData *ch, char *argument, int cmd, int subcmd);
void do_gecho(CharData *ch, char *argument, int cmd, int subcmd);
void do_gen_comm(CharData *ch, char *argument, int cmd, int subcmd);
void do_mobshout(CharData *ch, char *argument, int cmd, int subcmd);
void do_gen_ps(CharData *ch, char *argument, int cmd, int subcmd);
void do_get(CharData *ch, char *argument, int cmd, int subcmd);
void do_give(CharData *ch, char *argument, int cmd, int subcmd);
void do_givehorse(CharData *ch, char *argument, int cmd, int subcmd);
void do_gold(CharData *ch, char *argument, int cmd, int subcmd);
void do_goto(CharData *ch, char *argument, int cmd, int subcmd);
void do_grab(CharData *ch, char *argument, int cmd, int subcmd);
void do_group(CharData *ch, char *argument, int cmd, int subcmd);
void do_gsay(CharData *ch, char *argument, int cmd, int subcmd);
void do_hide(CharData *ch, char *argument, int cmd, int subcmd);
void do_info(CharData *ch, char *argument, int cmd, int subcmd);
void do_inspect(CharData *ch, char *argument, int cmd, int subcmd);
void do_insult(CharData *ch, char *argument, int cmd, int subcmd);
void do_inventory(CharData *ch, char *argument, int cmd, int subcmd);
void do_invis(CharData *ch, char *argument, int cmd, int subcmd);
void do_last(CharData *ch, char *argument, int cmd, int subcmd);
void do_mode(CharData *ch, char *argument, int cmd, int subcmd);
void do_mark(CharData *ch, char *argument, int cmd, int subcmd);
void do_makefood(CharData *ch, char *argument, int cmd, int subcmd);
void do_deviate(CharData *ch, char *argument, int cmd, int subcmd);
void do_levels(CharData *ch, char *argument, int cmd, int subcmd);
void do_liblist(CharData *ch, char *argument, int cmd, int subcmd);
void do_lightwalk(CharData *ch, char *argument, int cmd, int subcmd);
void do_load(CharData *ch, char *argument, int cmd, int subcmd);
void do_loadstat(CharData *ch, char *argument, int cmd, int subbcmd);
void do_look(CharData *ch, char *argument, int cmd, int subcmd);
void do_sides(CharData *ch, char *argument, int cmd, int subcmd);
void do_not_here(CharData *ch, char *argument, int cmd, int subcmd);
void do_offer(CharData *ch, char *argument, int cmd, int subcmd);
void do_olc(CharData *ch, char *argument, int cmd, int subcmd);
void do_page(CharData *ch, char *argument, int cmd, int subcmd);
void do_pray(CharData *ch, char *argument, int cmd, int subcmd);
void do_poofset(CharData *ch, char *argument, int cmd, int subcmd);
void do_pour(CharData *ch, char *argument, int cmd, int subcmd);
void do_skills(CharData *ch, char *argument, int cmd, int subcmd);
void do_statistic(CharData *ch, char *argument, int cmd, int subcmd);
void do_spells(CharData *ch, char *argument, int cmd, int subcmd);
void do_spellstat(CharData *ch, char *argument, int cmd, int subcmd);
void do_memorize(CharData *ch, char *argument, int cmd, int subcmd);
void do_learn(CharData *ch, char *argument, int cmd, int subcmd);
void do_forget(CharData *ch, char *argument, int cmd, int subcmd);
void do_purge(CharData *ch, char *argument, int cmd, int subcmd);
void do_put(CharData *ch, char *argument, int cmd, int subcmd);
void do_quit(CharData *ch, char *argument, int /* cmd */, int subcmd);
void do_reboot(CharData *ch, char *argument, int cmd, int subcmd);
void do_remove(CharData *ch, char *argument, int cmd, int subcmd);
void do_rent(CharData *ch, char *argument, int cmd, int subcmd);
void do_reply(CharData *ch, char *argument, int cmd, int subcmd);
void do_report(CharData *ch, char *argument, int cmd, int subcmd);
void do_refill(CharData *ch, char *argument, int cmd, int subcmd);
void do_setall(CharData *ch, char *argument, int cmd, int subcmd);
void do_stophorse(CharData *ch, char *argument, int cmd, int subcmd);
void do_restore(CharData *ch, char *argument, int cmd, int subcmd);
void do_return(CharData *ch, char *argument, int cmd, int subcmd);
void do_save(CharData *ch, char *argument, int cmd, int subcmd);
void do_say(CharData *ch, char *argument, int cmd, int subcmd);
void do_score(CharData *ch, char *argument, int cmd, int subcmd);
void do_sdemigod(CharData *ch, char *argument, int cmd, int subcmd);
void do_send(CharData *ch, char *argument, int cmd, int subcmd);
void do_set(CharData *ch, char *argument, int cmd, int subcmd);
void do_show(CharData *ch, char *argument, int cmd, int subcmd);
void do_shutdown(CharData *ch, char *argument, int cmd, int subcmd);
void do_skillset(CharData *ch, char *argument, int cmd, int subcmd);
void do_sneak(CharData *ch, char *argument, int cmd, int subcmd);
void do_snoop(CharData *ch, char *argument, int cmd, int subcmd);
void do_spec_comm(CharData *ch, char *argument, int cmd, int subcmd);
void do_spell_capable(CharData *ch, char *argument, int cmd, int subcmd);
void do_split(CharData *ch, char *argument, int cmd, int subcmd);
void do_split(CharData *ch, char *argument, int cmd, int subcmd, int currency);
void do_fry(CharData *ch, char *argument, int cmd, int subcmd);
void do_steal(CharData *ch, char *argument, int cmd, int subcmd);
void do_switch(CharData *ch, char *argument, int cmd, int subcmd);
void do_syslog(CharData *ch, char *argument, int cmd, int subcmd);
void do_teleport(CharData *ch, char *argument, int cmd, int subcmd);
void do_tell(CharData *ch, char *argument, int cmd, int subcmd);
void do_time(CharData *ch, char *argument, int cmd, int subcmd);
void do_toggle(CharData *ch, char *argument, int cmd, int subcmd);
void do_sense(CharData *ch, char *argument, int cmd, int subcmd);
void do_unban(CharData *ch, char *argument, int cmd, int subcmd);
void do_ungroup(CharData *ch, char *argument, int cmd, int subcmd);
void do_employ(CharData *ch, char *argument, int cmd, int subcmd);
void do_users(CharData *ch, char *argument, int cmd, int subcmd);
void do_visible(CharData *ch, char *argument, int cmd, int subcmd);
void do_vnum(CharData *ch, char *argument, int cmd, int subcmd);
void do_vstat(CharData *ch, char *argument, int cmd, int subcmd);
void do_wear(CharData *ch, char *argument, int cmd, int subcmd);
void do_weather(CharData *ch, char *argument, int cmd, int subcmd);
void do_where(CharData *ch, char *argument, int cmd, int subcmd);
void do_who(CharData *ch, char *argument, int cmd, int subcmd);
void do_wield(CharData *ch, char *argument, int cmd, int subcmd);
void do_wimpy(CharData *ch, char *argument, int cmd, int subcmd);
void do_wizlock(CharData *ch, char *argument, int cmd, int subcmd);
void do_wiznet(CharData *ch, char *argument, int cmd, int subcmd);
void do_wizutil(CharData *ch, char *argument, int cmd, int subcmd);
void do_write(CharData *ch, char *argument, int cmd, int subcmd);
void do_zreset(CharData *ch, char *argument, int cmd, int subcmd);
void do_style(CharData *ch, char *argument, int cmd, int subcmd);
void do_poisoned(CharData *ch, char *argument, int cmd, int subcmd);
void do_repair(CharData *ch, char *argument, int cmd, int subcmd);
void do_camouflage(CharData *ch, char *argument, int cmd, int subcmd);
void do_touch(CharData *ch, char *argument, int cmd, int subcmd);
void do_transform_weapon(CharData *ch, char *argument, int cmd, int subcmd);
void do_dig(CharData *ch, char *argument, int cmd, int subcmd);
void do_insertgem(CharData *ch, char *argument, int cmd, int subcmd);
void do_ignore(CharData *ch, char *argument, int cmd, int subcmd);
void do_proxy(CharData *ch, char *argument, int cmd, int subcmd);
void do_exchange(CharData *ch, char *argument, int cmd, int subcmd);
void do_godtest(CharData *ch, char *argument, int cmd, int subcmd);
void do_print_armor(CharData *ch, char *argument, int cmd, int subcmd);
void do_relocate(CharData *ch, char *argument, int cmd, int subcmd);
void do_custom_label(CharData *ch, char *argument, int cmd, int subcmd);
void do_quest(CharData *ch, char *argument, int cmd, int subcmd);
void do_check(CharData *ch, char *argument, int cmd, int subcmd);
// DG Script ACMD's
void do_attach(CharData *ch, char *argument, int cmd, int subcmd);
void do_detach(CharData *ch, char *argument, int cmd, int subcmd);
void do_tlist(CharData *ch, char *argument, int cmd, int subcmd);
void do_tstat(CharData *ch, char *argument, int cmd, int subcmd);
void do_vdelete(CharData *ch, char *argument, int cmd, int subcmd);
void do_hearing(CharData *ch, char *argument, int cmd, int subcmd);
void do_looking(CharData *ch, char *argument, int cmd, int subcmd);
void do_identify(CharData *ch, char *argument, int cmd, int subcmd);
void do_upgrade(CharData *ch, char *argument, int cmd, int subcmd);
void do_armored(CharData *ch, char *argument, int cmd, int subcmd);
void do_recall(CharData *ch, char *argument, int cmd, int subcmd);
void do_pray_gods(CharData *ch, char *argument, int cmd, int subcmd);
void do_rset(CharData *ch, char *argument, int cmd, int subcmd);
void do_recipes(CharData *ch, char *argument, int cmd, int subcmd);
void do_cook(CharData *ch, char *argument, int cmd, int subcmd);
void do_forgive(CharData *ch, char *argument, int cmd, int subcmd);
void do_townportal(CharData *ch, char *argument, int cmd, int subcmd);
void DoHouse(CharData *ch, char *argument, int cmd, int subcmd);
void DoClanChannel(CharData *ch, char *argument, int cmd, int subcmd);
void DoClanList(CharData *ch, char *argument, int cmd, int subcmd);
void DoShowPolitics(CharData *ch, char *argument, int cmd, int subcmd);
void DoShowWars(CharData *ch, char *argument, int cmd, int subcmd);
void do_show_alliance(CharData *ch, char *argument, int cmd, int subcmd);
void DoHcontrol(CharData *ch, char *argument, int cmd, int subcmd);
void DoWhoClan(CharData *ch, char *argument, int cmd, int subcmd);
void DoClanPkList(CharData *ch, char *argument, int cmd, int subcmd);
void DoStoreHouse(CharData *ch, char *argument, int cmd, int subcmd);
void do_clanstuff(CharData *ch, char *argument, int cmd, int subcmd);
void DoBest(CharData *ch, char *argument, int cmd, int subcmd);
void do_offtop(CharData *ch, char *argument, int cmd, int subcmd);
void do_dmeter(CharData *ch, char *argument, int cmd, int subcmd);
void do_mystat(CharData *ch, char *argument, int cmd, int subcmd);
void do_zone(CharData *ch, char *argument, int cmd, int subcmd);
void do_bandage(CharData *ch, char *argument, int cmd, int subcmd);
void do_sanitize(CharData *ch, char *argument, int cmd, int subcmd);
void do_morph(CharData *ch, char *argument, int cmd, int subcmd);
void do_morphset(CharData *ch, char *argument, int cmd, int subcmd);
void do_console(CharData *ch, char *argument, int cmd, int subcmd);
void do_shops_list(CharData *ch, char *argument, int cmd, int subcmd);
void do_unfreeze(CharData *ch, char *argument, int cmd, int subcmd);
void Bonus::do_bonus_by_character(CharData *, char *, int, int);
void do_summon(CharData *ch, char *argument, int cmd, int subcmd);
void do_check_occupation(CharData *ch, char *argument, int cmd, int subcmd);
void do_delete_obj(CharData *ch, char *argument, int cmd, int subcmd);
void do_arena_restore(CharData *ch, char *argument, int cmd, int subcmd);
void Bonus::do_bonus_info(CharData *, char *, int, int);
void do_showzonestats(CharData *, char *, int, int);
void do_overstuff(CharData *ch, char *, int, int);
void do_cities(CharData *ch, char *, int, int);
void do_send_text_to_char(CharData *ch, char *, int, int);
void do_add_wizard(CharData *ch, char *, int, int);
void do_touch_stigma(CharData *ch, char *, int, int);
void do_show_mobmax(CharData *ch, char *, int, int);

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

// здесь храним коды, которые отправили игрокам на почту
// строка - это мыло, если один чар вошел с необычного места, то блочим сразу всех чаров на этом мыле,
// пока не введет код (или до ребута)
std::map<std::string, int> new_loc_codes;

// имя чара на код, отправленный на почту для подтверждения мыла при создании
std::map<std::string, int> new_char_codes;

void do_debug_queues(CharData * /*ch*/, char *argument, int /*cmd*/, int /*subcmd*/) {
	std::stringstream ss;
	if (argument && *argument) {
		debug::log_queue(argument).print_queue(ss, argument);

		return;
	}

	for (const auto &q : debug::log_queues()) {
		q.second.print_queue(ss, q.first);
	}

	mudlog(ss.str().c_str(), DEF, kLevelGod, ERRLOG, true);
}

cpp_extern const struct command_info cmd_info[] =
	{
		{"RESERVED", EPosition::kDead, nullptr, 0, 0, 0},    // this must be first -- for specprocs

		// directions must come before other commands but after RESERVED
		{"север", EPosition::kStand, do_move, 0, SCMD_NORTH, -2},
		{"восток", EPosition::kStand, do_move, 0, SCMD_EAST, -2},
		{"юг", EPosition::kStand, do_move, 0, SCMD_SOUTH, -2},
		{"запад", EPosition::kStand, do_move, 0, SCMD_WEST, -2},
		{"вверх", EPosition::kStand, do_move, 0, SCMD_UP, -2},
		{"вниз", EPosition::kStand, do_move, 0, SCMD_DOWN, -2},
		{"north", EPosition::kStand, do_move, 0, SCMD_NORTH, -2},
		{"east", EPosition::kStand, do_move, 0, SCMD_EAST, -2},
		{"south", EPosition::kStand, do_move, 0, SCMD_SOUTH, -2},
		{"west", EPosition::kStand, do_move, 0, SCMD_WEST, -2},
		{"up", EPosition::kStand, do_move, 0, SCMD_UP, -2},
		{"down", EPosition::kStand, do_move, 0, SCMD_DOWN, -2},

		{"аффекты", EPosition::kDead, do_affects, 0, SCMD_AUCTION, 0},
		{"авторы", EPosition::kDead, do_gen_ps, 0, SCMD_CREDITS, 0},
		{"атаковать", EPosition::kFight, do_hit, 0, SCMD_MURDER, -1},
		{"аукцион", EPosition::kRest, do_gen_comm, 0, SCMD_AUCTION, 100},
		{"анонсы", EPosition::kDead, Boards::DoBoard, 1, Boards::NOTICE_BOARD, -1},

		{"базар", EPosition::kRest, do_exchange, 1, 0, -1},
		{"баланс", EPosition::kStand, do_not_here, 1, 0, 0},
		{"баги", EPosition::kDead, Boards::DoBoard, 1, Boards::ERROR_BOARD, 0},
		{"бежать", EPosition::kFight, do_flee, 1, 0, -1},
		{"бинтовать", EPosition::kRest, do_bandage, 0, 0, 0},
		{"билдер", EPosition::kDead, Boards::DoBoard, 1, Boards::GODBUILD_BOARD, -1},
		{"блок", EPosition::kFight, do_block, 0, 0, -1},
		{"блокнот", EPosition::kDead, Boards::DoBoard, 1, Boards::PERS_BOARD, -1},
		{"боги", EPosition::kDead, do_gen_ps, 0, SCMD_IMMLIST, 0},
		{"божества", EPosition::kDead, Boards::DoBoard, 1, Boards::GODGENERAL_BOARD, -1},
		{"болтать", EPosition::kRest, do_gen_comm, 0, SCMD_GOSSIP, -1},
		{"бонус", EPosition::kDead, Bonus::do_bonus_by_character, kLevelImplementator, 0, 0},
		{"бонусинфо", EPosition::kDead, Bonus::do_bonus_info, kLevelImplementator, 0, 0},
		{"бросить", EPosition::kRest, do_drop, 0, SCMD_DROP, -1},
		{"варить", EPosition::kRest, do_cook, 0, 0, 200},
		{"версия", EPosition::kDead, do_gen_ps, 0, SCMD_VERSION, 0},
		{"вече", EPosition::kDead, Boards::DoBoard, 1, Boards::GENERAL_BOARD, -1},
		{"взять", EPosition::kRest, do_get, 0, 0, 200},
		{"взглянуть", EPosition::kRest, do_diagnose, 0, 0, 100},
		{"взломать", EPosition::kStand, do_gen_door, 1, DOOR_SCMD::SCMD_PICK, -1},
		{"вихрь", EPosition::kFight, do_iron_wind, 0, 0, -1},
		{"вложить", EPosition::kStand, do_not_here, 1, 0, -1},
		{"вернуть", EPosition::kStand, do_not_here, 0, 0, -1},
		{"вернуться", EPosition::kDead, do_return, 0, 0, -1},
		{"войти", EPosition::kStand, do_enter, 0, 0, -2},
		{"война", EPosition::kRest, DoShowWars, 0, 0, 0},
		{"вооружиться", EPosition::kRest, do_wield, 0, 0, 200},
		{"возврат", EPosition::kRest, do_recall, 0, 0, -1},
		{"воззвать", EPosition::kDead, do_pray_gods, 0, 0, -1},
		{"вплавить", EPosition::kStand, do_insertgem, 0, 0, -1},
		{"время", EPosition::kDead, do_time, 0, 0, 0},
		{"врата", EPosition::kSit, do_townportal, 1, 0, -1},
		{"вскочить", EPosition::kFight, do_horseon, 0, 0, 500},
		{"встать", EPosition::kRest, do_stand, 0, 0, 500},
		{"вспомнить", EPosition::kDead, do_remember_char, 0, 0, 0},
		{"выбросить", EPosition::kRest, do_drop, 0, 0 /*SCMD_DONATE */ , 300},
		{"выследить", EPosition::kStand, do_track, 0, 0, 500},
		{"вылить", EPosition::kStand, do_pour, 0, SCMD_POUR, 500},
		{"выходы", EPosition::kRest, do_exits, 0, 0, 0},

		{"говорить", EPosition::kRest, do_say, 0, 0, -1},
		{"ггруппа", EPosition::kSleep, do_gsay, 0, 0, 500},
		{"гговорить", EPosition::kSleep, do_gsay, 0, 0, 500},
		{"гдругам", EPosition::kSleep, DoClanChannel, 0, SCMD_CHANNEL, 0},
		{"где", EPosition::kRest, do_where, kLevelImmortal, 0, 0},
		{"гдея", EPosition::kRest, do_zone, 0, 0, 0},
		{"глоток", EPosition::kRest, do_drink, 0, SCMD_SIP, 200},
		{"города", EPosition::kDead, do_cities, 0, 0, 0},
		{"группа", EPosition::kSleep, do_group, 1, 0, -1},
		{"гсоюзникам", EPosition::kSleep, DoClanChannel, 0, SCMD_ACHANNEL, 0},
		{"гэхо", EPosition::kDead, do_gecho, kLevelGod, 0, 0},
		{"гбогам", EPosition::kDead, do_wiznet, kLevelImmortal, 0, 0},

		{"дать", EPosition::kRest, do_give, 0, 0, 500},
		{"дата", EPosition::kDead, do_date, 0, SCMD_DATE, 0},
		{"делить", EPosition::kRest, do_split, 1, 0, 200},
		{"держать", EPosition::kRest, do_grab, 0, 0, 300},
		{"дметр", EPosition::kDead, do_dmeter, 0, 0, 0},
		{"доложить", EPosition::kRest, do_report, 0, 0, 500},
		{"доски", EPosition::kDead, Boards::DoBoardList, 0, 0, 0},
		{"дружины", EPosition::kDead, DoClanList, 0, 0, 0},
		{"дрновости", EPosition::kDead, Boards::DoBoard, 1, Boards::CLANNEWS_BOARD, -1},
		{"дрвече", EPosition::kDead, Boards::DoBoard, 1, Boards::CLAN_BOARD, -1},
		{"дрлист", EPosition::kDead, DoClanPkList, 0, 1, 0},
		//{"добавить", EPosition::kDead, do_add_wizard, kLevelImplementator, 0, 0 },
		{"есть", EPosition::kRest, do_eat, 0, SCMD_EAT, 500},

		{"жертвовать", EPosition::kStand, do_pray, 1, SCMD_DONATE, -1},

		{"заколоть", EPosition::kStand, do_backstab, 1, 0, 1},
		{"забыть", EPosition::kRest, do_forget, 0, 0, 0},
		{"задержать", EPosition::kStand, do_not_here, 1, 0, -1},
		{"заклинания", EPosition::kSleep, do_spells, 0, 0, 0},
		{"заклстат", EPosition::kDead, do_spellstat, kLevelGreatGod, 0, 0},
		{"закрыть", EPosition::kSit, do_gen_door, 0, SCMD_CLOSE, 500},
		{"замакс", EPosition::kRest, do_show_mobmax, 1, 0, -1},
		{"замести", EPosition::kStand, do_hidetrack, 1, 0, -1},
		{"замолчать", EPosition::kDead, do_wizutil, kLevelGod, SCMD_MUTE, 0},
		{"заморозить", EPosition::kDead, do_wizutil, kLevelFreeze, SCMD_FREEZE, 0},
		{"занятость", EPosition::kDead, do_check_occupation, kLevelGod, 0, 0},
		{"запомнить", EPosition::kRest, do_memorize, 0, 0, 0},
		{"запереть", EPosition::kSit, do_gen_door, 0, SCMD_LOCK, 500},
		{"запрет", EPosition::kDead, do_ban, kLevelGreatGod, 0, 0},
		{"заснуть", EPosition::kSleep, do_sleep, 0, 0, -1},
		{"заставка", EPosition::kDead, do_gen_ps, 0, SCMD_MOTD, 0},
		{"заставить", EPosition::kSleep, do_force, kLevelGreatGod, 0, 0},
		{"затоптать", EPosition::kStand, do_extinguish, 0, 0, 0},
		{"заточить", EPosition::kRest, do_upgrade, 0, 0, 500},
		{"заучить", EPosition::kRest, do_memorize, 0, 0, 0},
		{"зачитать", EPosition::kRest, do_employ, 0, SCMD_RECITE, 500},
		{"зачаровать", EPosition::kStand, do_spell_capable, 1, 0, 0},
		{"зачистить", EPosition::kDead, do_sanitize, kLevelGreatGod, 0, 0},
		{"золото", EPosition::kRest, do_gold, 0, 0, 0},
		{"зона", EPosition::kRest, do_zone, 0, 0, 0},
		{"зоныстат", EPosition::kDead, do_showzonestats, kLevelImmortal, 0, 0},
		{"инвентарь", EPosition::kSleep, do_inventory, 0, 0, 0},
		{"игнорировать", EPosition::kDead, do_ignore, 0, 0, 0},
		{"идеи", EPosition::kDead, Boards::DoBoard, 1, Boards::IDEA_BOARD, 0},
		{"изгнать нежить", EPosition::kRest, do_turn_undead, 0, 0, -1},
		{"изучить", EPosition::kSit, do_learn, 0, 0, 0},
		{"информация", EPosition::kSleep, do_gen_ps, 0, SCMD_INFO, 0},
		{"испить", EPosition::kRest, do_employ, 0, SCMD_QUAFF, 500},
		{"использовать", EPosition::kRest, do_style, 0, 0, 0},
		{"имя", EPosition::kSleep, do_name, kLevelImmortal, 0, 0},

		{"колдовать", EPosition::kSit, do_cast, 1, 0, -1},
		{"казна", EPosition::kRest, do_not_here, 1, 0, 0},
		{"карта", EPosition::kRest, do_map, 0, 0, 0},
		{"клан", EPosition::kRest, DoHouse, 0, 0, 0},
		{"клич", EPosition::kFight, do_warcry, 1, 0, -1},
		{"кодер", EPosition::kDead, Boards::DoBoard, 1, Boards::CODER_BOARD, -1},
		{"команды", EPosition::kDead, do_commands, 0, SCMD_COMMANDS, 0},
		{"коне", EPosition::kSleep, do_quit, 0, 0, 0},
		{"конец", EPosition::kSleep, do_quit, 0, SCMD_QUIT, 0},
		{"копать", EPosition::kStand, do_dig, 0, 0, -1},
		{"красться", EPosition::kStand, do_hidemove, 1, 0, -2},
		{"кричать", EPosition::kRest, do_gen_comm, 0, SCMD_SHOUT, -1},
		{"кто", EPosition::kRest, do_who, 0, 0, 0},
		{"ктодружина", EPosition::kRest, DoWhoClan, 0, 0, 0},
		{"ктоя", EPosition::kDead, do_gen_ps, 0, SCMD_WHOAMI, 0},
		{"купить", EPosition::kStand, do_not_here, 0, 0, -1},

		{"леваярука", EPosition::kRest, do_grab, 1, 0, 300},
		{"лечить", EPosition::kStand, do_firstaid, 0, 0, -1},
		{"лить", EPosition::kStand, do_pour, 0, SCMD_POUR, 500},
		{"лошадь", EPosition::kStand, do_not_here, 1, 0, -1},
		{"лучшие", EPosition::kDead, DoBest, 0, 0, 0},

		{"маскировка", EPosition::kRest, do_camouflage, 0, 0, 500},
		{"магазины", EPosition::kDead, do_shops_list, kLevelImmortal, 0, 0},
		{"метнуть", EPosition::kFight, do_throw, 0, SCMD_PHYSICAL_THROW, -1},
		{"менять", EPosition::kStand, do_not_here, 0, 0, -1},
		{"месть", EPosition::kRest, do_revenge, 0, 0, 0},
		{"молот", EPosition::kFight, do_mighthit, 0, 0, -1},
		{"молиться", EPosition::kStand, do_pray, 1, SCMD_PRAY, -1},
		{"моястатистика", EPosition::kDead, do_mystat, 0, 0, 0},
		{"мысл", EPosition::kDead, do_quit, 0, 0, 0},
		{"мысль", EPosition::kDead, Boards::report_on_board, 0, Boards::SUGGEST_BOARD, 0},

		{"наемник", EPosition::kStand, do_not_here, 1, 0, -1},
		{"наказания", EPosition::kDead, Boards::DoBoard, 1, Boards::GODPUNISH_BOARD, -1},
		{"налить", EPosition::kStand, do_pour, 0, SCMD_FILL, 500},
		{"наполнить", EPosition::kStand, do_pour, 0, SCMD_FILL, 500},
		{"найти", EPosition::kStand, do_sense, 0, 0, 500},
		{"нанять", EPosition::kStand, do_findhelpee, 0, 0, -1},
		{"новичок", EPosition::kSleep, do_gen_ps, 0, SCMD_INFO, 0},
		{"новости", EPosition::kDead, Boards::DoBoard, 1, Boards::NEWS_BOARD, -1},
		{"надеть", EPosition::kRest, do_wear, 0, 0, 500},
		{"нацарапать", EPosition::kRest, do_custom_label, 0, 0, 0},

		{"обезоружить", EPosition::kFight, do_disarm, 0, 0, -1},
		{"обернуться", EPosition::kStand, do_morph, 0, 0, -1},
		{"облачить", EPosition::kRest, do_wear, 0, 0, 500},
		{"обмен", EPosition::kStand, do_not_here, 0, 0, 0},
		{"обменять", EPosition::kStand, do_not_here, 0, 0, 0},
		{"оглядеться", EPosition::kRest, do_sides, 0, 0, 0},
		{"оглушить", EPosition::kFight, do_stupor, 0, 0, -1},
		{"одеть", EPosition::kRest, do_wear, 0, 0, 500},
		{"опознать", EPosition::kRest, do_identify, 0, 0, 500},
		{"опохмелиться", EPosition::kRest, do_drunkoff, 0, 0, -1},
		{"опечатк", EPosition::kDead, do_quit, 0, 0, 0},
		{"опечатка", EPosition::kDead, Boards::report_on_board, 0, Boards::MISPRINT_BOARD, 0},
		{"опустить", EPosition::kRest, do_put, 0, 0, 500},
		{"орать", EPosition::kRest, do_gen_comm, 1, SCMD_HOLLER, -1},
		{"осмотреть", EPosition::kRest, do_examine, 0, 0, 0},
		{"оседлать", EPosition::kStand, do_horsetake, 1, 0, -1},
		{"оскорбить", EPosition::kRest, do_insult, 0, 0, -1},
		{"осушить", EPosition::kRest, do_employ, 0, SCMD_QUAFF, 300},
		{"освежевать", EPosition::kStand, do_makefood, 0, 0, -1},
		{"ответить", EPosition::kRest, do_reply, 0, 0, -1},
		{"отразить", EPosition::kFight, do_multyparry, 0, 0, -1},
		{"отвязать", EPosition::kDead, do_horseget, 0, 0, -1},
		{"отдохнуть", EPosition::kRest, do_rest, 0, 0, -1},
		{"открыть", EPosition::kSit, do_gen_door, 0, DOOR_SCMD::SCMD_OPEN, 500},
		{"отпереть", EPosition::kSit, do_gen_door, 0, SCMD_UNLOCK, 500},
		{"отпустить", EPosition::kSit, do_stophorse, 0, 0, -1},
		{"отравить", EPosition::kFight, do_poisoned, 0, 0, -1},
		{"отринуть", EPosition::kRest, do_antigods, 1, 0, -1},
		{"отступить", EPosition::kFight, do_retreat, 1, 0, -1},
		{"отправить", EPosition::kStand, do_not_here, 1, 0, -1},
		{"оффтоп", EPosition::kDead, do_offtop, 0, 0, -1},
		{"ошеломить", EPosition::kStand, do_stun, 1, 0, -1},
		{"оценить", EPosition::kStand, do_not_here, 0, 0, 500},
		{"очки", EPosition::kDead, do_score, 0, 0, 0},
		{"очепятки", EPosition::kDead, Boards::DoBoard, 1, Boards::MISPRINT_BOARD, 0},
		{"очистить", EPosition::kDead, do_not_here, 0, SCMD_CLEAR, -1},
		{"ошибк", EPosition::kDead, do_quit, 0, 0, 0},
		{"ошибка", EPosition::kDead, Boards::report_on_board, 0, Boards::ERROR_BOARD, 0},

		{"парировать", EPosition::kFight, do_parry, 0, 0, -1},
		{"перехватить", EPosition::kFight, do_touch, 0, 0, -1},
		{"перековать", EPosition::kStand, do_transform_weapon, 0, SCMD_TRANSFORMWEAPON, -1},
		{"передать", EPosition::kStand, do_givehorse, 0, 0, -1},
		{"перевести", EPosition::kStand, do_not_here, 1, 0, -1},
		{"переместиться", EPosition::kStand, do_relocate, 1, 0, 0},
		{"перевоплотитьс", EPosition::kStand, do_remort, 0, 0, -1},
		{"перевоплотиться", EPosition::kStand, do_remort, 0, 1, -1},
		{"перелить", EPosition::kStand, do_pour, 0, SCMD_POUR, 500},
		{"перешить", EPosition::kRest, do_fit, 0, SCMD_MAKE_OVER, 500},
		{"пить", EPosition::kRest, do_drink, 0, SCMD_DRINK, 400},
		{"писать", EPosition::kStand, do_write, 1, 0, -1},
		{"пклист", EPosition::kSleep, DoClanPkList, 0, 0, 0},
		{"пнуть", EPosition::kFight, do_kick, 1, 0, -1},
		{"погода", EPosition::kRest, do_weather, 0, 0, 0},
		{"подкрасться", EPosition::kStand, do_sneak, 1, 0, 500},
		{"подножка", EPosition::kFight, do_chopoff, 0, 0, 500},
		{"подняться", EPosition::kRest, do_stand, 0, 0, -1},
		{"поджарить", EPosition::kRest, do_fry, 0, 0, -1},
		{"перевязать", EPosition::kRest, do_bandage, 0, 0, 0},
		{"переделать", EPosition::kRest, do_fit, 0, SCMD_DO_ADAPT, 500},
		{"подсмотреть", EPosition::kRest, do_look, 0, SCMD_LOOK_HIDE, 0},
		{"положить", EPosition::kRest, do_put, 0, 0, 400},
		{"получить", EPosition::kStand, do_not_here, 1, 0, -1},
		{"политика", EPosition::kSleep, DoShowPolitics, 0, 0, 0},
		{"помочь", EPosition::kFight, do_assist, 1, 0, -1},
		{"помощь", EPosition::kDead, do_help, 0, 0, 0},
		{"пометить", EPosition::kDead, do_mark, kLevelImplementator, 0, 0},
		{"порез", EPosition::kFight, DoExpedientCut, 0, 0, -1},
		{"поселиться", EPosition::kStand, do_not_here, 1, 0, -1},
		{"постой", EPosition::kStand, do_not_here, 1, 0, -1},
		{"почта", EPosition::kStand, do_not_here, 1, 0, -1},
		{"пополнить", EPosition::kStand, do_refill, 0, 0, 300},
		{"поручения", EPosition::kRest, do_quest, 1, 0, -1},
		{"появиться", EPosition::kRest, do_visible, 1, 0, -1},
		{"правила", EPosition::kDead, do_gen_ps, 0, SCMD_POLICIES, 0},
		{"предложение", EPosition::kStand, do_not_here, 1, 0, 500},
		//{"призвать", EPosition::kStand, do_summon, 1, 0, -1},
		{"приказ", EPosition::kRest, do_order, 1, 0, -1},
		{"привязать", EPosition::kRest, do_horseput, 0, 0, 500},
		{"приглядеться", EPosition::kRest, do_looking, 0, 0, 250},
		{"прикрыть", EPosition::kFight, do_protect, 0, 0, -1},
		{"применить", EPosition::kSit, do_employ, 1, SCMD_USE, 400},
		//{"прикоснуться", EPosition::kStand, do_touch_stigma, 0, 0, -1},
		{"присесть", EPosition::kRest, do_sit, 0, 0, -1},
		{"прислушаться", EPosition::kRest, do_hearing, 0, 0, 300},
		{"присмотреться", EPosition::kRest, do_looking, 0, 0, 250},
		{"придумки", EPosition::kDead, Boards::DoBoard, 0, Boards::SUGGEST_BOARD, 0},
		{"проверить", EPosition::kDead, do_check, 0, 0, 0},
		{"проснуться", EPosition::kSleep, do_wake, 0, SCMD_WAKE, -1},
		{"простить", EPosition::kRest, do_forgive, 0, 0, 0},
		{"пробовать", EPosition::kRest, do_eat, 0, SCMD_TASTE, 300},
		{"сожрать", EPosition::kRest, do_eat, 0, SCMD_DEVOUR, 300},
		{"продать", EPosition::kStand, do_not_here, 0, 0, -1},
		{"фильтровать", EPosition::kStand, do_not_here, 0, 0, -1},
		{"прыжок", EPosition::kSleep, do_goto, kLevelGod, 0, 0},

		{"разбудить", EPosition::kRest, do_wake, 0, SCMD_WAKEUP, -1},
		{"разгруппировать", EPosition::kDead, do_ungroup, 0, 0, 500},
		{"разделить", EPosition::kRest, do_split, 1, 0, 500},
		{"разделы", EPosition::kRest, do_help, 1, 0, 500},
		{"разжечь", EPosition::kStand, do_fire, 0, 0, -1},
		{"распустить", EPosition::kDead, do_ungroup, 0, 0, 500},
		{"рассмотреть", EPosition::kStand, do_not_here, 0, 0, -1},
		{"рассчитать", EPosition::kRest, do_freehelpee, 0, 0, -1},
		{"режим", EPosition::kDead, do_mode, 0, 0, 0},
		{"ремонт", EPosition::kRest, do_repair, 0, 0, -1},
		{"рецепты", EPosition::kRest, do_recipes, 0, 0, 0},
		{"рекорды", EPosition::kDead, DoBest, 0, 0, 0},
		{"руны", EPosition::kFight, do_mixture, 0, SCMD_RUNES, -1},

		{"сбить", EPosition::kFight, do_bash, 1, 0, -1},
		{"свойства", EPosition::kStand, do_not_here, 0, 0, -1},
		{"сгруппа", EPosition::kSleep, do_gsay, 0, 0, -1},
		{"сглазить", EPosition::kFight, do_manadrain, 0, 0, -1},
		{"сесть", EPosition::kRest, do_sit, 0, 0, -1},
		{"синоним", EPosition::kDead, do_alias, 0, 0, 0},
		{"сдемигодам", EPosition::kDead, do_sdemigod, kLevelImmortal, 0, 0},
		{"сказать", EPosition::kRest, do_tell, 0, 0, -1},
		{"скользить", EPosition::kStand, do_lightwalk, 0, 0, 0},
		{"следовать", EPosition::kRest, do_follow, 0, 0, 500},
		{"сложить", EPosition::kFight, do_mixture, 0, SCMD_RUNES, -1},
		{"слава", EPosition::kStand, Glory::do_spend_glory, 0, 0, 0},
		{"слава2", EPosition::kStand, GloryConst::do_spend_glory, 0, 0, 0},
		{"смотреть", EPosition::kRest, do_look, 0, SCMD_LOOK, 0},
		{"смешать", EPosition::kStand, do_mixture, 0, SCMD_ITEMS, -1},
//  { "смастерить",     EPosition::kStand, do_transform_weapon, 0, SCMD_CREATEBOW, -1 },
		{"снять", EPosition::kRest, do_remove, 0, 0, 500},
		{"создать", EPosition::kSit, do_create, 0, 0, -1},
		{"сон", EPosition::kSleep, do_sleep, 0, 0, -1},
		{"соскочить", EPosition::kFight, do_horseoff, 0, 0, -1},
		{"состав", EPosition::kRest, do_create, 0, SCMD_RECIPE, 0},
		{"сохранить", EPosition::kSleep, do_save, 0, 0, 0},
		{"союзы", EPosition::kRest, do_show_alliance, 0, 0, 0},
		{"социалы", EPosition::kDead, do_commands, 0, SCMD_SOCIALS, 0},
		{"спать", EPosition::kSleep, do_sleep, 0, 0, -1},
		{"спасти", EPosition::kFight, do_rescue, 1, 0, -1},
		{"способности", EPosition::kSleep, do_features, 0, 0, 0},
		{"список", EPosition::kStand, do_not_here, 0, 0, -1},
		{"справка", EPosition::kDead, do_help, 0, 0, 0},
		{"спросить", EPosition::kRest, do_spec_comm, 0, SCMD_ASK, -1},
		{"спрятаться", EPosition::kStand, do_hide, 1, 0, 500},
		{"сравнить", EPosition::kRest, do_consider, 0, 0, 500},
		{"ставка", EPosition::kStand, do_not_here, 0, 0, -1},
		{"статус", EPosition::kDead, do_display, 0, 0, 0},
		{"статистика", EPosition::kDead, do_statistic, 0, 0, 0},
		{"стереть", EPosition::kDead, do_gen_ps, 0, SCMD_CLEAR, 0},
		{"стиль", EPosition::kRest, do_style, 0, 0, 0},
		{"строка", EPosition::kDead, do_display, 0, 0, 0},
		{"счет", EPosition::kDead, do_score, 0, 0, 0},
		{"телега", EPosition::kDead, do_telegram, 0, 0, -1},
		{"тень", EPosition::kFight, do_throw, 0, SCMD_SHADOW_THROW, -1},
		{"титул", EPosition::kDead, TitleSystem::do_title, 0, 0, 0},
		{"трусость", EPosition::kDead, do_wimpy, 0, 0, 0},
		{"убить", EPosition::kFight, do_kill, 0, 0, -1},
		{"убрать", EPosition::kRest, do_remove, 0, 0, 400},
		{"ударить", EPosition::kFight, do_hit, 0, SCMD_HIT, -1},
		{"удавить", EPosition::kFight, do_strangle, 0, 0, -1},
		{"удалить", EPosition::kStand, do_delete_obj, kLevelImplementator, 0, 0},
		{"уклониться", EPosition::kFight, do_deviate, 1, 0, -1},
		{"украсть", EPosition::kStand, do_steal, 1, 0, 0},
		{"укрепить", EPosition::kRest, do_armored, 0, 0, -1},
		{"умения", EPosition::kSleep, do_skills, 0, 0, 0},
		{"уровень", EPosition::kDead, do_score, 0, 0, 0},
		{"уровни", EPosition::kDead, do_levels, 0, 0, 0},
		{"учить", EPosition::kStand, do_not_here, 0, 0, -1},
		{"хранилище", EPosition::kDead, DoStoreHouse, 0, 0, 0},
		{"характеристики", EPosition::kStand, do_not_here, 0, 0, -1},
		{"кланстаф", EPosition::kStand, do_clanstuff, 0, 0, 0},
		{"чинить", EPosition::kStand, do_not_here, 0, 0, -1},
		{"читать", EPosition::kRest, do_look, 0, SCMD_READ, 200},
		{"шептать", EPosition::kRest, do_spec_comm, 0, SCMD_WHISPER, -1},
		{"экипировка", EPosition::kSleep, do_equipment, 0, 0, 0},
		{"эмоция", EPosition::kRest, do_echo, 1, SCMD_EMOTE, -1},
		{"эхо", EPosition::kSleep, do_echo, kLevelImmortal, SCMD_ECHO, -1},
		{"ярость", EPosition::kRest, do_courage, 0, 0, -1},

		// God commands for listing
		{"мсписок", EPosition::kDead, do_liblist, 0, SCMD_MLIST, 0},
		{"осписок", EPosition::kDead, do_liblist, 0, SCMD_OLIST, 0},
		{"ксписок", EPosition::kDead, do_liblist, 0, SCMD_RLIST, 0},
		{"зсписок", EPosition::kDead, do_liblist, 0, SCMD_ZLIST, 0},

		{"'", EPosition::kRest, do_say, 0, 0, -1},
		{":", EPosition::kRest, do_echo, 1, SCMD_EMOTE, -1},
		{";", EPosition::kDead, do_wiznet, kLevelImmortal, 0, -1},
		{"advance", EPosition::kDead, do_advance, kLevelImplementator, 0, 0},
		{"alias", EPosition::kDead, do_alias, 0, 0, 0},
		{"alter", EPosition::kRest, do_fit, 0, SCMD_MAKE_OVER, 500},
		{"ask", EPosition::kRest, do_spec_comm, 0, SCMD_ASK, -1},
		{"assist", EPosition::kFight, do_assist, 1, 0, -1},
		{"attack", EPosition::kFight, do_hit, 0, SCMD_MURDER, -1},
		{"auction", EPosition::kRest, do_gen_comm, 0, SCMD_AUCTION, -1},
		{"arenarestore", EPosition::kSleep, do_arena_restore, kLevelGod, 0, 0},
		{"backstab", EPosition::kStand, do_backstab, 1, 0, 1},
		{"balance", EPosition::kStand, do_not_here, 1, 0, -1},
		{"ban", EPosition::kDead, do_ban, kLevelGreatGod, 0, 0},
		{"bash", EPosition::kFight, do_bash, 1, 0, -1},
		{"beep", EPosition::kDead, do_beep, kLevelImmortal, 0, 0},
		{"block", EPosition::kFight, do_block, 0, 0, -1},
		{"bug", EPosition::kDead, Boards::report_on_board, 0, Boards::ERROR_BOARD, 0},
		{"buy", EPosition::kStand, do_not_here, 0, 0, -1},
		{"best", EPosition::kDead, DoBest, 0, 0, 0},
		{"cast", EPosition::kSit, do_cast, 1, 0, -1},
		{"check", EPosition::kStand, do_not_here, 1, 0, -1},
		{"chopoff", EPosition::kFight, do_chopoff, 0, 0, 500},
		{"clear", EPosition::kDead, do_gen_ps, 0, SCMD_CLEAR, 0},
		{"close", EPosition::kSit, do_gen_door, 0, SCMD_CLOSE, 500},
		{"cls", EPosition::kDead, do_gen_ps, 0, SCMD_CLEAR, 0},
		{"commands", EPosition::kDead, do_commands, 0, SCMD_COMMANDS, 0},
		{"consider", EPosition::kRest, do_consider, 0, 0, 500},
		{"credits", EPosition::kDead, do_gen_ps, 0, SCMD_CREDITS, 0},
		{"date", EPosition::kDead, do_date, kLevelImmortal, SCMD_DATE, 0},
		{"dc", EPosition::kDead, do_dc, kLevelGreatGod, 0, 0},
		{"deposit", EPosition::kStand, do_not_here, 1, 0, 500},
		{"deviate", EPosition::kFight, do_deviate, 0, 0, -1},
		{"diagnose", EPosition::kRest, do_diagnose, 0, 0, 500},
		{"dig", EPosition::kStand, do_dig, 0, 0, -1},
		{"disarm", EPosition::kFight, do_disarm, 0, 0, -1},
		{"display", EPosition::kDead, do_display, 0, 0, 0},
		{"drink", EPosition::kRest, do_drink, 0, SCMD_DRINK, 500},
		{"drop", EPosition::kRest, do_drop, 0, SCMD_DROP, 500},
		{"dumb", EPosition::kDead, do_wizutil, kLevelImmortal, SCMD_DUMB, 0},
		{"eat", EPosition::kRest, do_eat, 0, SCMD_EAT, 500},
		{"devour", EPosition::kRest, do_eat, 0, SCMD_DEVOUR, 300},
		{"echo", EPosition::kSleep, do_echo, kLevelImmortal, SCMD_ECHO, 0},
		{"emote", EPosition::kRest, do_echo, 1, SCMD_EMOTE, -1},
		{"enter", EPosition::kStand, do_enter, 0, 0, -2},
		{"equipment", EPosition::kSleep, do_equipment, 0, 0, 0},
		{"examine", EPosition::kRest, do_examine, 0, 0, 500},
		{"exchange", EPosition::kRest, do_exchange, 1, 0, -1},
		{"exits", EPosition::kRest, do_exits, 0, 0, 500},
		{"featset", EPosition::kSleep, do_featset, kLevelImplementator, 0, 0},
		{"features", EPosition::kSleep, do_features, 0, 0, 0},
		{"fill", EPosition::kStand, do_pour, 0, SCMD_FILL, 500},
		{"fit", EPosition::kRest, do_fit, 0, SCMD_DO_ADAPT, 500},
		{"flee", EPosition::kFight, do_flee, 1, 0, -1},
		{"follow", EPosition::kRest, do_follow, 0, 0, -1},
		{"force", EPosition::kSleep, do_force, kLevelGreatGod, 0, 0},
		{"forcetime", EPosition::kDead, do_forcetime, kLevelImplementator, 0, 0},
		{"freeze", EPosition::kDead, do_wizutil, kLevelFreeze, SCMD_FREEZE, 0},
		{"gecho", EPosition::kDead, do_gecho, kLevelGod, 0, 0},
		{"get", EPosition::kRest, do_get, 0, 0, 500},
		{"give", EPosition::kRest, do_give, 0, 0, 500},
		{"godnews", EPosition::kDead, Boards::DoBoard, 1, Boards::GODNEWS_BOARD, -1},
		{"gold", EPosition::kRest, do_gold, 0, 0, 0},
		{"glide", EPosition::kStand, do_lightwalk, 0, 0, 0},
		{"glory", EPosition::kRest, GloryConst::do_glory, kLevelImplementator, 0, 0},
		{"glorytemp", EPosition::kRest, do_glory, kLevelBuilder, 0, 0},
		{"gossip", EPosition::kRest, do_gen_comm, 0, SCMD_GOSSIP, -1},
		{"goto", EPosition::kSleep, do_goto, kLevelGod, 0, 0},
		{"grab", EPosition::kRest, do_grab, 0, 0, 500},
		{"group", EPosition::kRest, do_group, 1, 0, 500},
		{"gsay", EPosition::kSleep, do_gsay, 0, 0, -1},
		{"gtell", EPosition::kSleep, do_gsay, 0, 0, -1},
		{"handbook", EPosition::kDead, do_gen_ps, kLevelImmortal, SCMD_HANDBOOK, 0},
		{"hcontrol", EPosition::kDead, DoHcontrol, kLevelGreatGod, 0, 0},
		{"help", EPosition::kDead, do_help, 0, 0, 0},
		{"hell", EPosition::kDead, do_wizutil, kLevelGod, SCMD_HELL, 0},
		{"hide", EPosition::kStand, do_hide, 1, 0, 0},
		{"hit", EPosition::kFight, do_hit, 0, SCMD_HIT, -1},
		{"hold", EPosition::kRest, do_grab, 1, 0, 500},
		{"holler", EPosition::kRest, do_gen_comm, 1, SCMD_HOLLER, -1},
		{"horse", EPosition::kStand, do_not_here, 0, 0, -1},
		{"house", EPosition::kRest, DoHouse, 0, 0, 0},
		{"huk", EPosition::kFight, do_mighthit, 0, 0, -1},
		{"idea", EPosition::kDead, Boards::DoBoard, 1, Boards::IDEA_BOARD, 0},
		{"ignore", EPosition::kDead, do_ignore, 0, 0, 0},
		{"immlist", EPosition::kDead, do_gen_ps, 0, SCMD_IMMLIST, 0},
		{"index", EPosition::kRest, do_help, 1, 0, 500},
		{"info", EPosition::kSleep, do_gen_ps, 0, SCMD_INFO, 0},
		{"insert", EPosition::kStand, do_insertgem, 0, 0, -1},
		{"inspect", EPosition::kDead, do_inspect, kLevelBuilder, 0, 0},
		{"insult", EPosition::kRest, do_insult, 0, 0, -1},
		{"inventory", EPosition::kSleep, do_inventory, 0, 0, 0},
		{"invis", EPosition::kDead, do_invis, kLevelGod, 0, -1},
		{"kick", EPosition::kFight, do_kick, 1, 0, -1},
		{"kill", EPosition::kFight, do_kill, 0, 0, -1},
		{"last", EPosition::kDead, do_last, kLevelGod, 0, 0},
		{"levels", EPosition::kDead, do_levels, 0, 0, 0},
		{"list", EPosition::kStand, do_not_here, 0, 0, -1},
		{"load", EPosition::kDead, do_load, kLevelBuilder, 0, 0},
		{"loadstat", EPosition::kDead, do_loadstat, kLevelImplementator, 0, 0},
		{"look", EPosition::kRest, do_look, 0, SCMD_LOOK, 200},
		{"lock", EPosition::kSit, do_gen_door, 0, SCMD_LOCK, 500},
		{"map", EPosition::kRest, do_map, 0, 0, 0},
		{"mail", EPosition::kStand, do_not_here, 1, 0, -1},
		{"mercenary", EPosition::kStand, do_not_here, 1, 0, -1},
		{"mode", EPosition::kDead, do_mode, 0, 0, 0},
		{"mshout", EPosition::kRest, do_mobshout, 0, 0, -1},
		{"motd", EPosition::kDead, do_gen_ps, 0, SCMD_MOTD, 0},
		{"murder", EPosition::kFight, do_hit, 0, SCMD_MURDER, -1},
		{"mute", EPosition::kDead, do_wizutil, kLevelImmortal, SCMD_MUTE, 0},
		{"medit", EPosition::kDead, do_olc, 0, SCMD_OLC_MEDIT, 0},
		{"name", EPosition::kDead, do_wizutil, kLevelGod, SCMD_NAME, 0},
		{"nedit", EPosition::kRest, NamedStuff::do_named, kLevelBuilder, SCMD_NAMED_EDIT, 0}, //Именной стаф редактирование
		{"news", EPosition::kDead, Boards::DoBoard, 1, Boards::NEWS_BOARD, -1},
		{"nlist", EPosition::kRest, NamedStuff::do_named, kLevelBuilder, SCMD_NAMED_LIST, 0}, //Именной стаф список
		{"notitle", EPosition::kDead, do_wizutil, kLevelGreatGod, SCMD_NOTITLE, 0},
		{"odelete", EPosition::kStand, do_delete_obj, kLevelImplementator, 0, 0},
		{"oedit", EPosition::kDead, do_olc, 0, SCMD_OLC_OEDIT, 0},
		{"offer", EPosition::kStand, do_not_here, 1, 0, 0},
		{"olc", EPosition::kDead, do_olc, kLevelGod, SCMD_OLC_SAVEINFO, 0},
		{"open", EPosition::kSit, do_gen_door, 0, SCMD_OPEN, 500},
		{"order", EPosition::kRest, do_order, 1, 0, -1},
		{"overstuff", EPosition::kDead, do_overstuff, kLevelGreatGod, 0, 0},
		{"page", EPosition::kDead, do_page, kLevelGod, 0, 0},
		{"parry", EPosition::kFight, do_parry, 0, 0, -1},
		{"pick", EPosition::kStand, do_gen_door, 1, SCMD_PICK, -1},
		{"poisoned", EPosition::kFight, do_poisoned, 0, 0, -1},
		{"policy", EPosition::kDead, do_gen_ps, 0, SCMD_POLICIES, 0},
		{"poofin", EPosition::kDead, do_poofset, kLevelGod, SCMD_POOFIN, 0},
		{"poofout", EPosition::kDead, do_poofset, kLevelGod, SCMD_POOFOUT, 0},
		{"pour", EPosition::kStand, do_pour, 0, SCMD_POUR, -1},
		{"practice", EPosition::kStand, do_not_here, 0, 0, -1},
		{"prompt", EPosition::kDead, do_display, 0, 0, 0},
		{"proxy", EPosition::kDead, do_proxy, kLevelGreatGod, 0, 0},
		{"purge", EPosition::kDead, do_purge, kLevelGod, 0, 0},
		{"put", EPosition::kRest, do_put, 0, 0, 500},
//	{"python", EPosition::kDead, do_console, kLevelGod, 0, 0},
		{"quaff", EPosition::kRest, do_employ, 0, SCMD_QUAFF, 500},
		{"qui", EPosition::kSleep, do_quit, 0, 0, 0},
		{"quit", EPosition::kSleep, do_quit, 0, SCMD_QUIT, -1},
		{"read", EPosition::kRest, do_look, 0, SCMD_READ, 200},
		{"receive", EPosition::kStand, do_not_here, 1, 0, -1},
		{"recipes", EPosition::kRest, do_recipes, 0, 0, 0},
		{"recite", EPosition::kRest, do_employ, 0, SCMD_RECITE, 500},
		{"redit", EPosition::kDead, do_olc, 0, SCMD_OLC_REDIT, 0},
		{"register", EPosition::kDead, do_wizutil, kLevelImmortal, SCMD_REGISTER, 0},
		{"unregister", EPosition::kDead, do_wizutil, kLevelImmortal, SCMD_UNREGISTER, 0},
		{"reload", EPosition::kDead, do_reboot, kLevelImplementator, 0, 0},
		{"remove", EPosition::kRest, do_remove, 0, 0, 500},
		{"rent", EPosition::kStand, do_not_here, 1, 0, -1},
		{"reply", EPosition::kRest, do_reply, 0, 0, -1},
		{"report", EPosition::kRest, do_report, 0, 0, -1},
		{"reroll", EPosition::kDead, do_wizutil, kLevelGreatGod, SCMD_REROLL, 0},
		{"rescue", EPosition::kFight, do_rescue, 1, 0, -1},
		{"rest", EPosition::kRest, do_rest, 0, 0, -1},
		{"restore", EPosition::kDead, do_restore, kLevelGreatGod, SCMD_RESTORE_GOD, 0},
		{"return", EPosition::kDead, do_return, 0, 0, -1},
		{"rset", EPosition::kSleep, do_rset, kLevelBuilder, 0, 0},
		{"rules", EPosition::kDead, do_gen_ps, kLevelImmortal, SCMD_RULES, 0},
		{"runes", EPosition::kFight, do_mixture, 0, SCMD_RUNES, -1},
		{"save", EPosition::kSleep, do_save, 0, 0, 0},
		{"say", EPosition::kRest, do_say, 0, 0, -1},
		{"scan", EPosition::kRest, do_sides, 0, 0, 500},
		{"score", EPosition::kDead, do_score, 0, 0, 0},
		{"sell", EPosition::kStand, do_not_here, 0, 0, -1},
		{"send", EPosition::kSleep, do_send, kLevelGreatGod, 0, 0},
		{"sense", EPosition::kStand, do_sense, 0, 0, 500},
		{"set", EPosition::kDead, do_set, kLevelImmortal, 0, 0},
		{"settle", EPosition::kStand, do_not_here, 1, 0, -1},
		{"shout", EPosition::kRest, do_gen_comm, 0, SCMD_SHOUT, -1},
		{"show", EPosition::kDead, do_show, kLevelImmortal, 0, 0},
		{"shutdown", EPosition::kDead, do_shutdown, kLevelImplementator, SCMD_SHUTDOWN, 0},
		{"sip", EPosition::kRest, do_drink, 0, SCMD_SIP, 500},
		{"sit", EPosition::kRest, do_sit, 0, 0, -1},
		{"skills", EPosition::kRest, do_skills, 0, 0, 0},
		{"skillset", EPosition::kSleep, do_skillset, kLevelImplementator, 0, 0},
		{"morphset", EPosition::kSleep, do_morphset, kLevelImplementator, 0, 0},
		{"setall", EPosition::kDead, do_setall, kLevelImplementator, 0, 0},
		{"sleep", EPosition::kSleep, do_sleep, 0, 0, -1},
		{"sneak", EPosition::kStand, do_sneak, 1, 0, -2},
		{"snoop", EPosition::kDead, do_snoop, kLevelGreatGod, 0, 0},
		{"socials", EPosition::kDead, do_commands, 0, SCMD_SOCIALS, 0},
		{"spells", EPosition::kRest, do_spells, 0, 0, 0},
		{"split", EPosition::kRest, do_split, 1, 0, 0},
		{"stand", EPosition::kRest, do_stand, 0, 0, -1},
		{"stat", EPosition::kDead, do_stat, kLevelGod, 0, 0},
		{"steal", EPosition::kStand, do_steal, 1, 0, 300},
		{"strangle", EPosition::kFight, do_strangle, 0, 0, -1},
		{"stupor", EPosition::kFight, do_stupor, 0, 0, -1},
		{"switch", EPosition::kDead, do_switch, kLevelGreatGod, 0, 0},
		{"syslog", EPosition::kDead, do_syslog, kLevelImmortal, SYSLOG, 0},
		{"suggest", EPosition::kDead, Boards::report_on_board, 0, Boards::SUGGEST_BOARD, 0},
		{"slist", EPosition::kDead, do_slist, kLevelImplementator, 0, 0},
		{"sedit", EPosition::kDead, do_sedit, kLevelImplementator, 0, 0},
		{"errlog", EPosition::kDead, do_syslog, kLevelBuilder, ERRLOG, 0},
		{"imlog", EPosition::kDead, do_syslog, kLevelBuilder, IMLOG, 0},
		{"take", EPosition::kRest, do_get, 0, 0, 500},
		{"taste", EPosition::kRest, do_eat, 0, SCMD_TASTE, 500},
		{"t2c", EPosition::kRest, do_send_text_to_char, kLevelGreatGod, 0, -1},
		{"telegram", EPosition::kDead, do_telegram, kLevelImmortal, 0, -1},
		{"teleport", EPosition::kDead, do_teleport, kLevelGreatGod, 0, -1},
		{"tell", EPosition::kRest, do_tell, 0, 0, -1},
		{"time", EPosition::kDead, do_time, 0, 0, 0},
		{"title", EPosition::kDead, TitleSystem::do_title, 0, 0, 0},
		{"touch", EPosition::kFight, do_touch, 0, 0, -1},
		{"track", EPosition::kStand, do_track, 0, 0, -1},
		{"transfer", EPosition::kStand, do_not_here, 1, 0, -1},
		{"trigedit", EPosition::kDead, do_olc, 0, SCMD_OLC_TRIGEDIT, 0},
		{"turn undead", EPosition::kRest, do_turn_undead, 0, 0, -1},
		{"typo", EPosition::kDead, Boards::report_on_board, 0, Boards::MISPRINT_BOARD, 0},
		{"unaffect", EPosition::kDead, do_wizutil, kLevelGreatGod, SCMD_UNAFFECT, 0},
		{"unban", EPosition::kDead, do_unban, kLevelGreatGod, 0, 0},
		{"unfreeze", EPosition::kDead, do_unfreeze, kLevelImplementator, 0, 0},
		{"ungroup", EPosition::kDead, do_ungroup, 0, 0, -1},
		{"unlock", EPosition::kSit, do_gen_door, 0, SCMD_UNLOCK, 500},
		{"uptime", EPosition::kDead, do_date, kLevelImmortal, SCMD_UPTIME, 0},
		{"use", EPosition::kSit, do_employ, 1, SCMD_USE, 500},
		{"users", EPosition::kDead, do_users, kLevelImmortal, 0, 0},
		{"value", EPosition::kStand, do_not_here, 0, 0, -1},
		{"version", EPosition::kDead, do_gen_ps, 0, SCMD_VERSION, 0},
		{"visible", EPosition::kRest, do_visible, 1, 0, -1},
		{"vnum", EPosition::kDead, do_vnum, kLevelGreatGod, 0, 0},
		{"вномер", EPosition::kDead, do_vnum, kLevelGreatGod, 0, 0},  //тупой копипаст для использования русского синтаксиса
		{"vstat", EPosition::kDead, do_vstat, kLevelGreatGod, 0, 0},
		{"wake", EPosition::kSleep, do_wake, 0, 0, -1},
		{"warcry", EPosition::kFight, do_warcry, 1, 0, -1},
		{"wear", EPosition::kRest, do_wear, 0, 0, 500},
		{"weather", EPosition::kRest, do_weather, 0, 0, 0},
		{"where", EPosition::kRest, do_where, kLevelImmortal, 0, 0},
		{"whirl", EPosition::kFight, do_iron_wind, 0, 0, -1},
		{"whisper", EPosition::kRest, do_spec_comm, 0, SCMD_WHISPER, -1},
		{"who", EPosition::kRest, do_who, 0, 0, 0},
		{"whoami", EPosition::kDead, do_gen_ps, 0, SCMD_WHOAMI, 0},
		{"wield", EPosition::kRest, do_wield, 0, 0, 500},
		{"wimpy", EPosition::kDead, do_wimpy, 0, 0, 0},
		{"withdraw", EPosition::kStand, do_not_here, 1, 0, -1},
		{"wizhelp", EPosition::kSleep, do_commands, kLevelImmortal, SCMD_WIZHELP, 0},
		{"wizlock", EPosition::kDead, do_wizlock, kLevelImplementator, 0, 0},
		{"wiznet", EPosition::kDead, do_wiznet, kLevelImmortal, 0, 0},
		{"wizat", EPosition::kDead, do_at, kLevelGreatGod, 0, 0},
		{"write", EPosition::kStand, do_write, 1, 0, -1},
		{"zedit", EPosition::kDead, do_olc, 0, SCMD_OLC_ZEDIT, 0},
		{"zone", EPosition::kRest, do_zone, 0, 0, 0},
		{"zreset", EPosition::kDead, do_zreset, kLevelGreatGod, 0, 0},

		// test command for gods
		{"godtest", EPosition::kDead, do_godtest, kLevelImplementator, 0, 0},
		{"armor", EPosition::kDead, do_print_armor, kLevelImplementator, 0, 0},

		// Команды крафтинга - для тестига пока уровня имма
		{"mrlist", EPosition::kDead, do_list_make, kLevelBuilder, 0, 0},
		{"mredit", EPosition::kDead, do_edit_make, kLevelBuilder, 0, 0},
		{"сшить", EPosition::kStand, do_make_item, 0, MAKE_WEAR, 0},
		{"выковать", EPosition::kStand, do_make_item, 0, MAKE_METALL, 0},
		{"смастерить", EPosition::kStand, do_make_item, 0, MAKE_CRAFT, 0},

		// God commands for listing
		{"mlist", EPosition::kDead, do_liblist, 0, SCMD_MLIST, 0},
		{"olist", EPosition::kDead, do_liblist, 0, SCMD_OLIST, 0},
		{"rlist", EPosition::kDead, do_liblist, 0, SCMD_RLIST, 0},
		{"zlist", EPosition::kDead, do_liblist, 0, SCMD_ZLIST, 0},
		{"clist", EPosition::kDead, do_liblist, kLevelGod, SCMD_CLIST, 0},

		{"attach", EPosition::kDead, do_attach, kLevelImplementator, 0, 0},
		{"detach", EPosition::kDead, do_detach, kLevelImplementator, 0, 0},
		{"tlist", EPosition::kDead, do_tlist, 0, 0, 0},
		{"tstat", EPosition::kDead, do_tstat, 0, 0, 0},
		{"vdelete", EPosition::kDead, do_vdelete, kLevelImplementator, 0, 0},
		{"debug_queues", EPosition::kDead, do_debug_queues, kLevelImplementator, 0, 0},

		{heartbeat::cmd::HEARTBEAT_COMMAND, heartbeat::cmd::MINIMAL_POSITION, heartbeat::cmd::do_heartbeat,
		 heartbeat::cmd::MINIMAL_LEVEL, heartbeat::SCMD_NOTHING, heartbeat::cmd::UNHIDE_PROBABILITY},
		//{crafts::cmd::CRAFT_COMMAND, crafts::cmd::MINIMAL_POSITION, crafts::cmd::do_craft, crafts::cmd::MINIMAL_LEVEL, crafts::SCMD_NOTHING, crafts::cmd::UNHIDE_PROBABILITY},
		{"\n", EPosition::kDead, nullptr, 0, 0, 0}
	};                // this must be last

const char *dir_fill[] = {"in",
						  "from",
						  "with",
						  "the",
						  "on",
						  "at",
						  "to",
						  "\n"
};

const char *reserved[] = {"a",
						  "an",
						  "self",
						  "me",
						  "all",
						  "room",
						  "someone",
						  "something",
						  "\n"
};

void check_hiding_cmd(CharData *ch, int percent) {
	int remove_hide = false;
	if (affected_by_spell(ch, kSpellHide)) {
		if (percent == -2) {
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK)) {
				remove_hide = number(1, MUD::Skills()[ESkill::kSneak].difficulty) >
					CalcCurrentSkill(ch, ESkill::kSneak, nullptr);
			} else {
				percent = 500;
			}
		}

		if (percent == -1) {
			remove_hide = true;
		} else if (percent > 0) {
			remove_hide = number(1, percent) > CalcCurrentSkill(ch, ESkill::kHide, 0);
		}

		if (remove_hide) {
			affect_from_char(ch, kSpellHide);
			if (!AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE)) {
				send_to_char("Вы прекратили прятаться.\r\n", ch);
				act("$n прекратил$g прятаться.", false, ch, 0, 0, kToRoom);
			}
		}
	}
}

bool check_frozen_cmd(CharData * /*ch*/, int cmd) {
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
		|| !strcmp(cmd_info[cmd].command, "score")) {
		return true;
	}
	return false;
}

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(CharData *ch, char *argument) {
	int cmd, social = false, hardcopy = false;
	char *line;

	// just drop to next line for hitting CR
	CHECK_AGRO(ch) = 0;
	skip_spaces(&argument);

	if (!*argument)
		return;

	if (!IS_NPC(ch)) {
		log("<%s, %d> {%5d} [%s]",
			GET_NAME(ch),
			GlobalObjects::heartbeat().pulse_number(),
			GET_ROOM_VNUM(ch->in_room),
			argument);
		if (GetRealLevel(ch) >= kLevelImmortal || GET_GOD_FLAG(ch, GF_PERSLOG) || GET_GOD_FLAG(ch, GF_DEMIGOD))
			pers_log(ch, "<%s> {%5d} [%s]", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), argument);
	}

	//Polud спешиал для спешиалов добавим обработку числового префикса перед именем команды

	int fnum = get_number(&argument);

	/*
	 * special case to handle one-character, non-alphanumeric commands;
	 * requested by many people so "'hi" or ";godnet test" is possible.
	 * Patch sent by Eric Green and Stefan Wasilewski.
	 */
	if (!a_isalpha(*argument)) {
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
	} else {
		line = any_one_arg(argument, arg);
	}

	const size_t length = strlen(arg);
	if (1 < length && *(arg + length - 1) == '!') {
		hardcopy = true;
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
		int cont;    // continue the command checks
		cont = command_wtrigger(ch, arg, line);
		if (!cont)
			cont = command_mtrigger(ch, arg, line);
		if (!cont)
			cont = command_otrigger(ch, arg, line);
		if (cont) {
			check_hiding_cmd(ch, -1);
			return;    // command trigger took over
		}
	}

#if defined WITH_SCRIPTING
	// Try scripting
	if (scripting::execute_player_command(ch, arg, line))
		return;
#endif

	// otherwise, find the command
	for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
		if (hardcopy) {
			if (!strcmp(cmd_info[cmd].command, arg))
				if (Privilege::can_do_priv(ch, std::string(cmd_info[cmd].command), cmd, 0))
					break;
		} else {
			if (!strncmp(cmd_info[cmd].command, arg, length))
				if (Privilege::can_do_priv(ch, std::string(cmd_info[cmd].command), cmd, 0))
					break;
		}
	}

	if (*cmd_info[cmd].command == '\n') {
		if (find_action(arg) >= 0)
			social = true;
		else {
			send_to_char("Чаво?\r\n", ch);
			return;
		}
	}

	if (((!IS_NPC(ch)
		&& (GET_FREEZE_LEV(ch) > GetRealLevel(ch))
		&& (PLR_FLAGGED(ch, PLR_FROZEN)))
		|| GET_MOB_HOLD(ch)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT))
		&& !check_frozen_cmd(ch, cmd)) {
		send_to_char("Вы попытались, но не смогли сдвинуться с места...\r\n", ch);
		return;
	}

	if (!social && cmd_info[cmd].command_pointer == nullptr) {
		send_to_char("Извините, не смог разобрать команду.\r\n", ch);
		return;
	}

	if (!social && IS_NPC(ch) && cmd_info[cmd].minimum_level >= kLevelImmortal) {
		send_to_char("Вы еще не БОГ, чтобы делать это.\r\n", ch);
		return;
	}

	if (!social && GET_POS(ch) < cmd_info[cmd].minimum_position) {
		switch (GET_POS(ch)) {
			case EPosition::kDead: send_to_char("Очень жаль - ВЫ МЕРТВЫ!!! :-(\r\n", ch);
				break;
			case EPosition::kIncap:
			case EPosition::kPerish: send_to_char("Вы в критическом состоянии и не можете ничего делать!\r\n", ch);
				break;
			case EPosition::kStun: send_to_char("Вы слишком слабы, чтобы сделать это!\r\n", ch);
				break;
			case EPosition::kSleep: send_to_char("Сделать это в ваших снах?\r\n", ch);
				break;
			case EPosition::kRest: send_to_char("Нет... Вы слишком расслаблены...\r\n", ch);
				break;
			case EPosition::kSit: send_to_char("Пожалуй, вам лучше встать на ноги.\r\n", ch);
				break;
			case EPosition::kFight: send_to_char("Ни за что! Вы сражаетесь за свою жизнь!\r\n", ch);
				break;
			default: send_to_char("Вы не в том состоянии, чтобы это сделать...\r\n", ch);
				break;
		}
		return;
	}
	if (social) {
		check_hiding_cmd(ch, -1);
		do_social(ch, argument);
	} else if (no_specials || !special(ch, cmd, line, fnum)) {
		check_hiding_cmd(ch, cmd_info[cmd].unhide_percent);
		(*cmd_info[cmd].command_pointer)(ch, line, cmd, cmd_info[cmd].subcmd);
		if (ch->purged()) {
			return;
		}
		if (!IS_NPC(ch) && ch->in_room != kNowhere && CHECK_AGRO(ch)) {
			CHECK_AGRO(ch) = false;
			do_aggressive_room(ch, false);
			if (ch->purged()) {
				return;
			}
		}
	}
}

// ************************************************************************
// * Routines to handle aliasing                                          *
// ************************************************************************
struct alias_data *find_alias(struct alias_data *alias_list, char *str) {
	while (alias_list != nullptr) {
		if (*str == *alias_list->alias)    // hey, every little bit counts :-)
			if (!strcmp(str, alias_list->alias))
				return (alias_list);

		alias_list = alias_list->next;
	}

	return (nullptr);
}

void free_alias(struct alias_data *a) {
	if (a->alias)
		free(a->alias);
	if (a->replacement)
		free(a->replacement);
	free(a);
}

// The interface to the outside world: do_alias
void do_alias(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char *repl;
	struct alias_data *a;

	if (IS_NPC(ch))
		return;

	repl = any_one_arg(argument, arg);

	if (!*arg)        // no argument specified -- list currently defined aliases
	{
		send_to_char("Определены следующие алиасы:\r\n", ch);
		if ((a = GET_ALIASES(ch)) == nullptr)
			send_to_char(" Нет алиасов.\r\n", ch);
		else {
			while (a != nullptr) {
				sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
				send_to_char(buf, ch);
				a = a->next;
			}
		}
	} else        // otherwise, add or remove aliases
	{
		// is this an alias we've already defined?
		if ((a = find_alias(GET_ALIASES(ch), arg)) != nullptr) {
			REMOVE_FROM_LIST(a, GET_ALIASES(ch));
			free_alias(a);
		}
		// if no replacement string is specified, assume we want to delete
		if (!*repl) {
			if (a == nullptr)
				send_to_char("Такой алиас не определен.\r\n", ch);
			else
				send_to_char("Алиас успешно удален.\r\n", ch);
		} else {
			if (!str_cmp(arg, "alias")) {
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
		WAIT_STATE(ch, 1 * kPulseViolence);
		write_aliases(ch);
	}
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct TextBlocksQueue *input_q, char *orig, struct alias_data *a) {
	struct TextBlocksQueue temp_queue;
	char *tokens[NUM_TOKENS], *temp, *write_point;
	int num_of_tokens = 0, num;

	// First, parse the original string
	temp = strtok(strcpy(buf2, orig), " ");
	while (temp != nullptr && num_of_tokens < NUM_TOKENS) {
		tokens[num_of_tokens++] = temp;
		temp = strtok(nullptr, " ");
	}

	// initialize
	write_point = buf;
	temp_queue.head = temp_queue.tail = nullptr;

	// now parse the alias
	for (temp = a->replacement; *temp; temp++) {
		if (*temp == ALIAS_SEP_CHAR) {
			*write_point = '\0';
			buf[kMaxInputLength - 1] = '\0';
			write_to_q(buf, &temp_queue, 1);
			write_point = buf;
		} else if (*temp == ALIAS_VAR_CHAR) {
			temp++;
			if ((num = *temp - '1') < num_of_tokens && num >= 0) {
				strcpy(write_point, tokens[num]);
				write_point += strlen(tokens[num]);
			} else if (*temp == ALIAS_GLOB_CHAR) {
				strcpy(write_point, orig);
				write_point += strlen(orig);
			} else if ((*(write_point++) = *temp) == '$')    // redouble $ for act safety
				*(write_point++) = '$';
		} else
			*(write_point++) = *temp;
	}

	*write_point = '\0';
	buf[kMaxInputLength - 1] = '\0';
	write_to_q(buf, &temp_queue, 1);

	// push our temp_queue on to the _front_ of the input queue
	if (input_q->head == nullptr)
		*input_q = temp_queue;
	else {
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
int perform_alias(DescriptorData *d, char *orig) {
	char first_arg[kMaxInputLength], *ptr;
	struct alias_data *a, *tmp;

	// Mobs don't have alaises. //
	if (IS_NPC(d->character))
		return (0);

	// bail out immediately if the guy doesn't have any aliases //
	if ((tmp = GET_ALIASES(d->character)) == nullptr)
		return (0);

	// find the alias we're supposed to match //
	ptr = any_one_arg(orig, first_arg);

	// bail out if it's null //
	if (!*first_arg)
		return (0);

	// if the first arg is not an alias, return without doing anything //
	if ((a = find_alias(tmp, first_arg)) == nullptr)
		return (0);

	if (a->type == ALIAS_SIMPLE) {
		strcpy(orig, a->replacement);
		return (0);
	} else {
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
int search_block(const char *arg, const char **list, int exact) {
	int i;
	size_t l = strlen(arg);

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!str_cmp(arg, *(list + i))) {
				return i;
			}
		}
	} else {
		if (0 == l) {
			l = 1;    // Avoid "" to match the first available string
		}
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!strn_cmp(arg, *(list + i), l)) {
				return i;
			}
		}
	}

	return -1;
}

int search_block(const std::string &arg, const char **list, int exact) {
	int i;
	std::string::size_type l = arg.length();

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++)
			if (!str_cmp(arg, *(list + i)))
				return (i);
	} else {
		if (!l)
			l = 1;    // Avoid "" to match the first available string
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strn_cmp(arg, *(list + i), l))
				return (i);
	}

	return (-1);
}

int is_number(const char *str) {
	while (*str) {
		if (!a_isdigit(*(str++))) {
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
char *delete_doubledollar(char *string) {
	char *read, *write;

	// If the string has no dollar signs, return immediately //
	if ((write = strchr(string, '$')) == nullptr)
		return (string);

	// Start from the location of the first dollar sign //
	read = write;

	while (*read)        // Until we reach the end of the string... //
		if ((*(write++) = *(read++)) == '$')    // copy one char //
			if (*read == '$')
				read++;    // skip if we saw 2 $'s in a row //

	*write = '\0';

	return (string);
}

int fill_word(const char *argument) {
	return (search_block(argument, dir_fill, true) >= 0);
}

int reserved_word(const char *argument) {
	return (search_block(argument, reserved, true) >= 0);
}

template<typename T>
T one_argument_template(T argument, char *first_arg) {
	char *begin = first_arg;

	if (!argument) {
		log("SYSERR: one_argument received a NULL pointer!");
		*first_arg = '\0';
		return (nullptr);
	}

	do {
		skip_spaces(&argument);

		first_arg = begin;
		while (*argument && !a_isspace(*argument)) {
			*(first_arg++) = a_lcc(*argument);
			argument++;
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	return (argument);
}

template<typename T>
T any_one_arg_template(T argument, char *first_arg) {
	if (!argument) {
		log("SYSERR: any_one_arg() passed a NULL pointer.");
		return 0;
	}
	skip_spaces(&argument);

	int num = 0;
//	int len = strlen(argument);
	while (*argument && !a_isspace(*argument) && num < kMaxStringLength - 1) {
		*first_arg = a_lcc(*argument);
		++first_arg;
		++argument;
		++num;
	}
	*first_arg = '\0';

	return argument;
}

char *one_argument(char *argument, char *first_arg) { return one_argument_template(argument, first_arg); }
const char *one_argument(const char *argument, char *first_arg) { return one_argument_template(argument, first_arg); }
char *any_one_arg(char *argument, char *first_arg) { return any_one_arg_template(argument, first_arg); }
const char *any_one_arg(const char *argument, char *first_arg) { return any_one_arg_template(argument, first_arg); }

void array_argument(const char *arg, std::vector<std::string> &out)
{
	char local_buf[kMaxTrglineLength];
	const char *current_arg = arg;
	out.clear();
	do {
		current_arg = one_argument(current_arg, local_buf);
		if (!*local_buf) {
			break;
		}
		out.push_back(local_buf);
	} while (*current_arg);
}

void array_argument(const char *arg, std::vector<short> &out)
{
	std::vector<std::string> tmp;
	array_argument(arg, tmp);
	for (const auto &value : tmp) {
		out.push_back(atoi(value.c_str()));
	}
}

void array_argument(const char *arg, std::vector<int> &out)
{
	std::vector<std::string> tmp;
	array_argument(arg, tmp);
	for (const auto &value : tmp) {
		out.push_back(atoi(value.c_str()));
	}
}

// return first space-delimited token in arg1; remainder of string in arg2 //
void half_chop(const char *string, char *arg1, char *arg2) {
	const char *temp = any_one_arg_template(string, arg1);
	skip_spaces(&temp);
	strl_cpy(arg2, temp, kMaxStringLength);
}

// Used in specprocs, mostly.  (Exactly) matches "command" to cmd number //
int find_command(const char *command) {
	int cmd;

	for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
		if (!strcmp(cmd_info[cmd].command, command))
			return (cmd);

	return (-1);
}

// int fnum - номер найденного в комнате спешиал-моба, для обработки нескольких спешиал-мобов в одной комнате //
int special(CharData *ch, int cmd, char *arg, int fnum) {
	if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)) {
		const auto clan = Clan::GetClanByRoom(ch->in_room);
		if (!clan) {
			return 0;
		}
	}

	ObjData *i;
	int j;

	// special in room? //
	if (GET_ROOM_SPEC(ch->in_room) != nullptr) {
		if (GET_ROOM_SPEC(ch->in_room)(ch, world[ch->in_room], cmd, arg)) {
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in equipment list? //
	for (j = 0; j < NUM_WEARS; j++) {
		if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != nullptr) {
			if (GET_OBJ_SPEC(GET_EQ(ch, j))(ch, GET_EQ(ch, j), cmd, arg)) {
				check_hiding_cmd(ch, -1);
				return (1);
			}
		}
	}

	// special in inventory? //
	for (i = ch->carrying; i; i = i->get_next_content()) {
		if (GET_OBJ_SPEC(i) != nullptr
			&& GET_OBJ_SPEC(i)(ch, i, cmd, arg)) {
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in mobile present? //
//Polud чтобы продавцы не мешали друг другу в одной комнате, предусмотрим возможность различать их по номеру
	int specialNum = 1; //если номер не указан - по умолчанию берется первый
	for (const auto k : world[ch->in_room]->people) {
		if (GET_MOB_SPEC(k) != nullptr
			&& (fnum == 1
				|| fnum == specialNum++)
			&& GET_MOB_SPEC(k)(ch, k, cmd, arg)) {
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in object present? //
	for (i = world[ch->in_room]->contents; i; i = i->get_next_content()) {
		auto spec = GET_OBJ_SPEC(i);
		if (spec != nullptr
			&& spec(ch, i, cmd, arg)) {
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
int find_name(const char *name) {
	const auto index = player_table.get_by_name(name);
	return PlayersIndex::NOT_FOUND == index ? -1 : static_cast<int>(index);
}

int _parse_name(char *arg, char *name) {
	int i;

	// skip whitespaces
	for (i = 0; (*name = (i ? LOWER(*arg) : UPPER(*arg))); arg++, i++, name++) {
		if (*arg == 'ё'
			|| *arg == 'Ё'
			|| !a_isalpha(*arg)
			|| *arg > 0) {
			return (1);
		}
	}

	if (!i) {
		return (1);
	}

	return (0);
}

/**
* Вобщем это пока дублер старого _parse_name для уже созданных ранее чаров,
* чтобы их в игру вообще пускало, а новых с Ё/ё соответственно брило.
*/
int parse_exist_name(char *arg, char *name) {
	int i;

	// skip whitespaces
	for (i = 0; (*name = (i ? LOWER(*arg) : UPPER(*arg))); arg++, i++, name++)
		if (!a_isalpha(*arg) || *arg > 0)
			return (1);

	if (!i)
		return (1);

	return (0);
}

enum Mode {
	UNDEFINED,
	RECON,
	USURP,
	UNSWITCH
};

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
int perform_dupe_check(DescriptorData *d) {
	DescriptorData *k, *next_k;
	Mode mode = UNDEFINED;

	int id = GET_IDNUM(d->character);

	/*
	 * Now that this descriptor has successfully logged in, disconnect all
	 * other descriptors controlling a character with the same ID number.
	 */

	CharData::shared_ptr target;
	for (k = descriptor_list; k; k = next_k) {
		next_k = k->next;
		if (k == d) {
			continue;
		}

		if (k->original && (GET_IDNUM(k->original) == id))    // switched char
		{
			if (str_cmp(d->host, k->host)) {
				sprintf(buf, "ПОВТОРНЫЙ ВХОД! Id = %ld Персонаж = %s Хост = %s(был %s)",
						GET_IDNUM(d->character), GET_NAME(d->character), k->host, d->host);
				mudlog(buf, BRF, MAX(kLevelImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
				//send_to_gods(buf);
			}

			SEND_TO_Q("\r\nПопытка второго входа - отключаемся.\r\n", k);
			STATE(k) = CON_CLOSE;

			if (!target) {
				target = k->original;
				mode = UNSWITCH;
			}

			if (k->character) {
				k->character->desc = nullptr;
			}

			k->character = nullptr;
			k->original = nullptr;
		} else if (k->character && (GET_IDNUM(k->character) == id)) {
			if (str_cmp(d->host, k->host)) {
				sprintf(buf, "ПОВТОРНЫЙ ВХОД! Id = %ld Name = %s Host = %s(был %s)",
						GET_IDNUM(d->character), GET_NAME(d->character), k->host, d->host);
				mudlog(buf, BRF, MAX(kLevelImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
				//send_to_gods(buf);
			}

			if (!target && STATE(k) == CON_PLAYING) {
				SEND_TO_Q("\r\nВаше тело уже кем-то занято!\r\n", k);
				target = k->character;
				mode = USURP;
			}
			k->character->desc = nullptr;
			k->character = nullptr;
			k->original = nullptr;
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

	character_list.foreach_on_copy([&](const CharData::shared_ptr &ch) {
		if (IS_NPC(ch)) {
			return;
		}

		if (GET_IDNUM(ch) != id) {
			return;
		}

		// ignore entities with descriptors (already handled by above step) //
		if (ch->desc)
			return;

		// don't extract the target char we've found one already //
		if (ch == target)
			return;

		// we don't already have a target and found a candidate for switching //
		if (!target) {
			target = ch;
			mode = RECON;
			return;
		}

		// we've found a duplicate - blow him away, dumping his eq in limbo. //
		if (ch->in_room != kNowhere) {
			char_from_room(ch);
		}
		char_to_room(ch, STRANGE_ROOM);
		extract_char(ch.get(), false);
	});

	// no target for switching into was found - allow login to continue //
	if (!target) {
		return 0;
	}

	// Okay, we've found a target.  Connect d to target. //

	d->character = target;
	d->character->desc = d;
	d->original = nullptr;
	d->character->char_specials.timer = 0;
	PLR_FLAGS(d->character).unset(PLR_MAILING);
	PLR_FLAGS(d->character).unset(PLR_WRITING);
	STATE(d) = CON_PLAYING;

	switch (mode) {
		case RECON: SEND_TO_Q("Пересоединяемся.\r\n", d);
			check_light(d->character.get(), LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, 1);
			act("$n восстановил$g связь.", true, d->character.get(), 0, 0, kToRoom);
			sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
			mudlog(buf, NRM, MAX(kLevelImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			login_change_invoice(d->character.get());
			break;

		case USURP: SEND_TO_Q("Ваша душа вновь вернулась в тело, которое так ждало ее возвращения!\r\n", d);
			act("$n надломил$u от боли, окруженн$w белой аурой...\r\n"
				"Тело $s было захвачено новым духом!", true, d->character.get(), 0, 0, kToRoom);
			sprintf(buf, "%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
			mudlog(buf, NRM, MAX(kLevelImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			break;

		case UNSWITCH: SEND_TO_Q("Пересоединяемся для перевключения игрока.", d);
			sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
			mudlog(buf, NRM, MAX(kLevelImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			break;

		default:
			// ??? what does this case mean ???
			break;
	}

	add_logon_record(d);
	return 1;
}

int pre_help(CharData *ch, char *arg) {
	char command[kMaxInputLength], topic[kMaxInputLength];

	half_chop(arg, command, topic);

	if (!*command || strlen(command) < 2 || !*topic || strlen(topic) < 2)
		return (0);
	if (isname(command, "помощь help справка")) {
		do_help(ch, topic, 0, 0);
		return (1);
	}
	return (0);
}

// вобщем флажок для зареганных ип, потому что при очередной автопроверке, если превышен
// лимит коннектов с ип - сядут все сместе, что выглядит имхо странно, может там комп новый воткнули
// и просто еще до иммов не достучались лимит поднять... вобщем сидит тот, кто не успел Ж)
int check_dupes_host(DescriptorData *d, bool autocheck = 0) {
	if (!d->character || IS_IMMORTAL(d->character))
		return 1;

	// в случае авточекалки нужная проверка уже выполнена до входа в функцию
	if (!autocheck) {
		if (RegisterSystem::is_registered(d->character.get())) {
			return 1;
		}

		if (RegisterSystem::is_registered_email(GET_EMAIL(d->character))) {
			d->registered_email = 1;
			return 1;
		}
	}

	for (DescriptorData *i = descriptor_list; i; i = i->next) {
		if (i != d
			&& i->ip == d->ip
			&& i->character
			&& !IS_IMMORTAL(i->character)
			&& (STATE(i) == CON_PLAYING
				|| STATE(i) == CON_MENU)) {
			switch (CheckProxy(d)) {
				case 0:
					// если уже сидим в проксе, то смысла спамить никакого
					if (IN_ROOM(d->character) == r_unreg_start_room
						|| d->character->get_was_in_room() == r_unreg_start_room) {
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
					mudlog(buf, NRM, MAX(kLevelImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
					return 0;

				case 1:
					if (autocheck) {
						return 1;
					}
					send_to_char("&RС вашего IP адреса находится максимально допустимое количество игроков.\r\n"
								 "Обратитесь к Богам для увеличения лимита игроков с вашего адреса.&n",
								 d->character.get());
					return 0;

				default: return 1;
			}
		}
	}
	return 1;
}

int check_dupes_email(DescriptorData *d) {
	if (!d->character
		|| IS_IMMORTAL(d->character)) {
		return (1);
	}

	for (const auto &ch : character_list) {
		if (ch == d->character
			|| IS_NPC(ch)) {
			continue;
		}

		if (!IS_IMMORTAL(ch)
			&& (!str_cmp(GET_EMAIL(ch), GET_EMAIL(d->character)))) {
			sprintf(buf, "Персонаж с таким email уже находится в игре, вы не можете войти одновременно с ним!");
			send_to_char(buf, d->character.get());
			return (0);
		}
	}

	return 1;
}

void add_logon_record(DescriptorData *d) {
	log("Enter logon list");
	// Добавляем запись в LOG_LIST
	d->character->get_account()->add_login(std::string(d->host));

	const auto logon = std::find_if(LOGON_LIST(d->character).begin(), LOGON_LIST(d->character).end(),
									[&](const Logon &l) -> bool {
										return !strcmp(l.ip, d->host);
									});

	if (logon == LOGON_LIST(d->character).end()) {
		const Logon cur_log = {str_dup(d->host), 1, time(0), false};
		LOGON_LIST(d->character).push_back(cur_log);
	} else {
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
void check_religion(CharData *ch) {
	if (class_religion[ch->get_class()] == kReligionPoly && GET_RELIGION(ch) != kReligionPoly) {
		GET_RELIGION(ch) = kReligionPoly;
		log("Change religion to poly: %s", ch->get_name().c_str());
	} else if (class_religion[ch->get_class()] == kReligionMono && GET_RELIGION(ch) != kReligionMono) {
		GET_RELIGION(ch) = kReligionMono;
		log("Change religion to mono: %s", ch->get_name().c_str());
	}
}

void do_entergame(DescriptorData *d) {
	int load_room, cmd, flag = 0;

	d->character->reset();
	read_aliases(d->character.get());

	if (GetRealLevel(d->character) == kLevelImmortal) {
		d->character->set_level(kLevelGod);
	}

	if (GetRealLevel(d->character) > kLevelImplementator) {
		d->character->set_level(1);
	}

	if (GET_INVIS_LEV(d->character) > kLevelImplementator
		|| GET_INVIS_LEV(d->character) < 0) {
		SET_INVIS_LEV(d->character, 0);
	}

	if (GetRealLevel(d->character) > kLevelImmortal
		&& GetRealLevel(d->character) < kLevelBuilder
		&& (d->character->get_gold() > 0 || d->character->get_bank() > 0)) {
		d->character->set_gold(0);
		d->character->set_bank(0);
	}

	if (GetRealLevel(d->character) >= kLevelImmortal && GetRealLevel(d->character) < kLevelImplementator) {
		for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
			if (!strcmp(cmd_info[cmd].command, "syslog")) {
				if (Privilege::can_do_priv(d->character.get(), std::string(cmd_info[cmd].command), cmd, 0)) {
					flag = 1;
					break;
				}
			}
		}

		if (!flag) {
			GET_LOGS(d->character)[0] = 0;
		}
	}

	if (GetRealLevel(d->character) < kLevelImplementator) {
		if (PLR_FLAGGED(d->character, PLR_INVSTART)) {
			SET_INVIS_LEV(d->character, kLevelImmortal);
		}
		if (GET_INVIS_LEV(d->character) > GetRealLevel(d->character)) {
			SET_INVIS_LEV(d->character, GetRealLevel(d->character));
		}

		if (PRF_FLAGGED(d->character, PRF_CODERINFO)) {
			PRF_FLAGS(d->character).unset(PRF_CODERINFO);
		}
		if (GetRealLevel(d->character) < kLevelGod) {
			if (PRF_FLAGGED(d->character, PRF_HOLYLIGHT)) {
				PRF_FLAGS(d->character).unset(PRF_HOLYLIGHT);
			}
		}
		if (GetRealLevel(d->character) < kLevelGod) {
			if (PRF_FLAGGED(d->character, PRF_NOHASSLE)) {
				PRF_FLAGS(d->character).unset(PRF_NOHASSLE);
			}
			if (PRF_FLAGGED(d->character, PRF_ROOMFLAGS)) {
				PRF_FLAGS(d->character).unset(PRF_ROOMFLAGS);
			}
		}

		if (GET_INVIS_LEV(d->character) > 0
			&& GetRealLevel(d->character) < kLevelImmortal) {
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
	 * or equip_char() will gripe about the person in kNowhere.
	 */
	if (PLR_FLAGGED(d->character, PLR_HELLED))
		load_room = r_helled_start_room;
	else if (PLR_FLAGGED(d->character, PLR_NAMED))
		load_room = r_named_start_room;
	else if (PLR_FLAGGED(d->character, PLR_FROZEN))
		load_room = r_frozen_start_room;
	else if (!check_dupes_host(d))
		load_room = r_unreg_start_room;
	else {
		if ((load_room = GET_LOADROOM(d->character)) == kNowhere) {
			load_room = calc_loadroom(d->character.get());
		}
		load_room = real_room(load_room);

		if (!Clan::MayEnter(d->character.get(), load_room, HCE_PORTAL)) {
			load_room = Clan::CloseRent(load_room);
		}

		if (!is_rent(load_room)) {
			load_room = kNowhere;
		}
	}

	// If char was saved with kNowhere, or real_room above failed...
	if (load_room == kNowhere) {
		if (GetRealLevel(d->character) >= kLevelImmortal)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}

	send_to_char(WELC_MESSG, d->character.get());

	CharData *character = nullptr;
	for (const auto &character_i : character_list) {
		if (character_i == d->character) {
			character = character_i.get();
			break;
		}
	}

	if (!character) {
		character_list.push_front(d->character);
	} else {
		MOB_FLAGS(character).unset(MOB_DELETE);
		MOB_FLAGS(character).unset(MOB_FREE);
	}

	log("Player %s enter at room %d", GET_NAME(d->character), GET_ROOM_VNUM(load_room));
	char_to_room(d->character, load_room);

	// а потом уже вычитаем за ренту
	if (GetRealLevel(d->character) != 0) {
		Crash_load(d->character.get());
		d->character->obj_bonus().update(d->character.get());
	}

	Depot::enter_char(d->character.get());
	Glory::check_freeze(d->character.get());
	Clan::clan_invoice(d->character.get(), true);

	// Чистим стили если не знаем их
	if (PRF_FLAGS(d->character).get(PRF_SHADOW_THROW)) {
		PRF_FLAGS(d->character).unset(PRF_SHADOW_THROW);
	}

	if (PRF_FLAGS(d->character).get(PRF_PUNCTUAL)
		&& !d->character->get_skill(ESkill::kPunctual)) {
		PRF_FLAGS(d->character).unset(PRF_PUNCTUAL);
	}

	if (PRF_FLAGS(d->character).get(PRF_AWAKE)
		&& !d->character->get_skill(ESkill::kAwake)) {
		PRF_FLAGS(d->character).unset(PRF_AWAKE);
	}

	if (PRF_FLAGS(d->character).get(PRF_POWERATTACK)
		&& !can_use_feat(d->character.get(), POWER_ATTACK_FEAT)) {
		PRF_FLAGS(d->character).unset(PRF_POWERATTACK);
	}

	if (PRF_FLAGS(d->character).get(PRF_GREATPOWERATTACK)
		&& !can_use_feat(d->character.get(), GREAT_POWER_ATTACK_FEAT)) {
		PRF_FLAGS(d->character).unset(PRF_GREATPOWERATTACK);
	}

	if (PRF_FLAGS(d->character).get(PRF_AIMINGATTACK)
		&& !can_use_feat(d->character.get(), AIMING_ATTACK_FEAT)) {
		PRF_FLAGS(d->character).unset(PRF_AIMINGATTACK);
	}

	if (PRF_FLAGS(d->character).get(PRF_GREATAIMINGATTACK)
		&& !can_use_feat(d->character.get(), GREAT_AIMING_ATTACK_FEAT)) {
		PRF_FLAGS(d->character).unset(PRF_GREATAIMINGATTACK);
	}
	if (PRF_FLAGS(d->character).get(PRF_DOUBLE_THROW)
		&& !can_use_feat(d->character.get(), DOUBLE_THROW_FEAT)) {
		PRF_FLAGS(d->character).unset(PRF_DOUBLE_THROW);
	}
	if (PRF_FLAGS(d->character).get(PRF_TRIPLE_THROW)
		&& !can_use_feat(d->character.get(), TRIPLE_THROW_FEAT)) {
		PRF_FLAGS(d->character).unset(PRF_TRIPLE_THROW);
	}
	if (PRF_FLAGS(d->character).get(PRF_SKIRMISHER)) {
		PRF_FLAGS(d->character).unset(PRF_SKIRMISHER);
	}
	if (PRF_FLAGS(d->character).get(PRF_IRON_WIND)) {
		PRF_FLAGS(d->character).unset(PRF_IRON_WIND);
	}

	// Check & remove/add natural, race & unavailable features
	for (int i = 1; i < kMaxFeats; i++) {
		if (!HAVE_FEAT(d->character, i)
			|| can_get_feat(d->character.get(), i)) {
			if (feat_info[i].is_inborn[(int) GET_CLASS(d->character)][(int) GET_KIN(d->character)]) {
				SET_FEAT(d->character, i);
			}
		}
	}

	SetRaceFeats(d->character.get());

	if (!IS_IMMORTAL(d->character)) {
		for (const auto &skill : MUD::Skills()) {
			if (MUD::Classes()[(d->character)->get_class()].HasntSkill(skill.GetId())) {
				d->character->set_skill(skill.GetId(), 0);
			}
		}
	}

	//Заменяем закл !переместиться! на способность
	if (GET_SPELL_TYPE(d->character, kSpellRelocate) == kSpellKnow && !IS_GOD(d->character)) {
		GET_SPELL_TYPE(d->character, kSpellRelocate) = 0;
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
	act("$n вступил$g в игру.", true, d->character.get(), 0, 0, kToRoom);
	// with the copyover patch, this next line goes in enter_player_game()
	read_saved_vars(d->character.get());
	enter_wtrigger(world[d->character.get()->in_room], d->character.get(), -1);
	greet_mtrigger(d->character.get(), -1);
	greet_otrigger(d->character.get(), -1);
	STATE(d) = CON_PLAYING;
	PRF_FLAGS(d->character).set(PRF_COLOR_2); // цвет всегда полный
// режимы по дефолту у нового чара
	const bool new_char = GetRealLevel(d->character) <= 0 ? true : false;
	if (new_char) {
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
		do_start(d->character.get(), true);
		GET_MANA_STORED(d->character) = 0;
		send_to_char(START_MESSG, d->character.get());
	}

	init_warcry(d->character.get());

	// На входе в игру вешаем флаг (странно, что он до этого нигде не вешался
	if (Privilege::god_list_check(GET_NAME(d->character), GET_UNIQUE(d->character))
		&& (GetRealLevel(d->character) < kLevelGod)) {
		SET_GOD_FLAG(d->character, GF_DEMIGOD);
	}
	// Насильственно забираем этот флаг у иммов (если он, конечно же, есть
	if ((GET_GOD_FLAG(d->character, GF_DEMIGOD) && GetRealLevel(d->character) >= kLevelGod)) {
		CLR_GOD_FLAG(d->character, GF_DEMIGOD);
	}

	switch (GET_SEX(d->character)) {
		case ESex::kLast: [[fallthrough]];
		case ESex::kNeutral: sprintf(buf, "%s вошло в игру.", GET_NAME(d->character));
			break;
		case ESex::kMale: sprintf(buf, "%s вошел в игру.", GET_NAME(d->character));
			break;
		case ESex::kFemale: sprintf(buf, "%s вошла в игру.", GET_NAME(d->character));
			break;
		case ESex::kPoly: sprintf(buf, "%s вошли в игру.", GET_NAME(d->character));
			break;
	}

	mudlog(buf, NRM, MAX(kLevelImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
	d->has_prompt = 0;
	login_change_invoice(d->character.get());
	affect_total(d->character.get());
	check_light(d->character.get(), LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, 0);
	look_at_room(d->character.get(), 0);

	if (new_char) {
		send_to_char("\r\nВоспользуйтесь командой НОВИЧОК для получения вводной информации игроку.\r\n",
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
bool ValidateStats(DescriptorData *d) {
	//Требуется рерол статов
	if (!GloryMisc::check_stats(d->character.get())) {
		return false;
	}

	//Некорректный номер расы
	if (PlayerRace::GetKinNameByNum(GET_KIN(d->character), GET_SEX(d->character)) == KIN_NAME_UNDEFINED) {
		SEND_TO_Q("\r\nЧто-то заплутал ты, путник. Откуда бредешь?\r\nВыберите народ:\r\n", d);
		SEND_TO_Q(string(PlayerRace::ShowKinsMenu()).c_str(), d);
		SEND_TO_Q("\r\nВыберите племя: ", d);
		STATE(d) = CON_RESET_KIN;
		return false;
	}

	//Некорректный номер рода
	if (PlayerRace::GetRaceNameByNum(GET_KIN(d->character), GET_RACE(d->character), GET_SEX(d->character))
		== RACE_NAME_UNDEFINED) {
		SEND_TO_Q("\r\nКакого роду-племени вы будете?\r\n", d);
		SEND_TO_Q(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
		SEND_TO_Q("\r\nИз чьих вы будете: ", d);
		STATE(d) = CON_RESET_RACE;
		return false;
	}

	// не корректный номер религии
	if (GET_RELIGION(d->character) > kReligionMono) {
		SEND_TO_Q(religion_menu, d);
		SEND_TO_Q("\n\rРелигия :", d);
		STATE(d) = CON_RESET_RELIGION;
		return false;
	}

	return true;
}

void DoAfterPassword(DescriptorData *d) {
	int load_result;

	// Password was correct.
	load_result = GET_BAD_PWS(d->character);
	GET_BAD_PWS(d->character) = 0;
	d->bad_pws = 0;

	if (ban->is_banned(d->host) == BanList::BAN_SELECT && !PLR_FLAGGED(d->character, PLR_SITEOK)) {
		SEND_TO_Q("Извините, вы не можете выбрать этого игрока с данного IP!\r\n", d);
		STATE(d) = CON_CLOSE;
		sprintf(buf, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, kLevelGod, SYSLOG, true);
		return;
	}
	if (GetRealLevel(d->character) < circle_restrict) {
		SEND_TO_Q("Игра временно приостановлена.. Ждем вас немного позже.\r\n", d);
		STATE(d) = CON_CLOSE;
		sprintf(buf, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, kLevelGod, SYSLOG, true);
		return;
	}
	if (new_loc_codes.count(GET_EMAIL(d->character)) != 0) {
		SEND_TO_Q("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
		STATE(d) = CON_RANDOM_NUMBER;
		return;
	}
	// нам нужен массив сетей с маской /24
	std::set<uint32_t> subnets;

	const uint32_t MASK = 16777215;
	for (const auto &logon : LOGON_LIST(d->character)) {
		uint32_t current_subnet = inet_addr(logon.ip) & MASK;
		subnets.insert(current_subnet);
	}

	if (!subnets.empty()) {
		if (subnets.count(inet_addr(d->host) & MASK) == 0) {
			sprintf(buf, "Персонаж %s вошел с необычного места!", GET_NAME(d->character));
			mudlog(buf, CMP, kLevelGod, SYSLOG, true);
			if (PRF_FLAGGED(d->character, PRF_IPCONTROL)) {
				int random_number = number(1000000, 9999999);
				new_loc_codes[GET_EMAIL(d->character)] = random_number;
				std::string cmd_line =
					str(boost::format("python3 send_code.py %s %d &") % GET_EMAIL(d->character) % random_number);
				auto result = system(cmd_line.c_str());
				UNUSED_ARG(result);
				SEND_TO_Q("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
				STATE(d) = CON_RANDOM_NUMBER;
				return;
			}
		}
	}
	// check and make sure no other copies of this player are logged in
	if (perform_dupe_check(d)) {
		Clan::SetClanData(d->character.get());
		return;
	}

	// тут несколько вариантов как это проставить и все одинаково корявые с учетом релоада, без уверенности не трогать
	Clan::SetClanData(d->character.get());

	log("%s [%s] has connected.", GET_NAME(d->character), d->host);

	if (load_result) {
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
	if (!ValidateStats(d)) {
		return;
	}

	SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	STATE(d) = CON_RMOTD;
}

void CreateChar(DescriptorData *d) {
	if (d->character) {
		return;
	}

	d->character.reset(new Player);
	d->character->player_specials = std::make_shared<player_special_data>();
	d->character->desc = d;
}

int create_unique(void) {
	int unique;

	do {
		unique = (number(0, 64) << 24) + (number(0, 255) << 16) + (number(0, 255) << 8) + (number(0, 255));
	} while (correct_unique(unique));

	return (unique);
}

// initialize a new character only if class is set
void init_char(CharData *ch, PlayerIndexElement &element) {
	int i;

#ifdef TEST_BUILD
	if (0 == player_table.size())
	{
		// При собирании через make test первый чар в маде становится иммом 34
		ch->set_level(kLevelImplementator);
	}
#endif

	GET_PORTALS(ch) = nullptr;
	CREATE(GET_LOGS(ch), 1 + LAST_LOG);
	ch->set_npc_name(0);
	ch->player_data.long_descr = "";
	ch->player_data.description = "";
	ch->player_data.time.birth = time(0);
	ch->player_data.time.played = 0;
	ch->player_data.time.logon = time(0);

	// make favors for sex
	if (ch->get_sex() == ESex::kMale) {
		ch->player_data.weight = number(120, 180);
		ch->player_data.height = number(160, 200);
	} else {
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
	element.mail = nullptr;//added by WorM mail
	element.last_ip = nullptr;//added by WorM последний айпи

	if (GetRealLevel(ch) > kLevelGod) {
		set_god_skills(ch);
		set_god_morphs(ch);
	}

	for (i = 1; i <= kSpellCount; i++) {
		if (GetRealLevel(ch) < kLevelGreatGod)
			GET_SPELL_TYPE(ch, i) = 0;
		else
			GET_SPELL_TYPE(ch, i) = kSpellKnow;
	}

	ch->char_specials.saved.affected_by = clear_flags;
	for (auto save = ESaving::kFirst; save <= ESaving::kLast; ++save) {
		SET_SAVE(ch, save, 0);
	}
	for (i = 0; i < MAX_NUMBER_RESISTANCE; i++)
		GET_RESIST(ch, i) = 0;

	if (GetRealLevel(ch) == kLevelImplementator) {
		ch->set_str(25);
		ch->set_int(25);
		ch->set_wis(25);
		ch->set_dex(25);
		ch->set_con(25);
		ch->set_cha(25);
	}
	ch->real_abils.size = 50;

	for (i = 0; i < 3; i++) {
		GET_COND(ch, i) = (GetRealLevel(ch) == kLevelImplementator ? -1 : i == DRUNK ? 0 : 24);
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
int create_entry(PlayerIndexElement &element) {
	// create new save activity
	element.activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
	element.timer = nullptr;

	return static_cast<int>(player_table.append(element));
}

void DoAfterEmailConfirm(DescriptorData *d) {
	PlayerIndexElement element(-1, GET_PC_NAME(d->character));

	// Now GET_NAME() will work properly.
	init_char(d->character.get(), element);

	if (d->character->get_pfilepos() < 0) {
		d->character->set_pfilepos(create_entry(element));
	}
	d->character->save_char();
	d->character->get_account()->set_last_login();
	d->character->get_account()->add_player(GetUniqueByName(d->character->get_name()));

	// добавляем в список ждущих одобрения
	if (!(int) NAME_FINE(d->character)) {
		sprintf(buf, "%s - новый игрок. Падежи: %s/%s/%s/%s/%s/%s Email: %s Пол: %s. ]\r\n"
					 "[ %s ждет одобрения имени.",
				GET_NAME(d->character), GET_PAD(d->character, 0),
				GET_PAD(d->character, 1), GET_PAD(d->character, 2),
				GET_PAD(d->character, 3), GET_PAD(d->character, 4),
				GET_PAD(d->character, 5), GET_EMAIL(d->character),
				genders[(int) GET_SEX(d->character)], GET_NAME(d->character));
		NewNames::add(d->character.get());
	}

	// remove from free names
	player_table.name_adviser().remove(GET_NAME(d->character));

	SEND_TO_Q(motd, d);
	SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	STATE(d) = CON_RMOTD;
	d->character->set_who_mana(0);
	d->character->set_who_last(time(0));

}

// Фраза 'Русская азбука: "абв...эюя".' в разных кодировках и для разных клиентов
#define ENC_HINT_KOI8R          "\xf2\xd5\xd3\xd3\xcb\xc1\xd1 \xc1\xda\xc2\xd5\xcb\xc1: \"\xc1\xc2\xd7...\xdc\xc0\xd1\"."
#define ENC_HINT_ALT            "\x90\xe3\xe1\xe1\xaa\xa0\xef \xa0\xa7\xa1\xe3\xaa\xa0: \"\xa0\xa1\xa2...\xed\xee\xef\"."
#define ENC_HINT_WIN            "\xd0\xf3\xf1\xf1\xea\xe0\xff\xff \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfe\xff\xff\"."
// обход ошибки с 'я' в zMUD после ver. 6.39+ и CMUD без замены 'я' на 'z'
#define ENC_HINT_WIN_ZMUD       "\xd0\xf3\xf1\xf1\xea\xe0\xff\xff? \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfe\xff\xff?\"."
// замена 'я' на 'z' в zMUD до ver. 6.39a для обхода ошибки,
// а также в zMUD после ver. 6.39a для совместимости
#define ENC_HINT_WIN_ZMUD_z     "\xd0\xf3\xf1\xf1\xea\xe0z \xe0\xe7\xe1\xf3\xea\xe0: \"\xe0\xe1\xe2...\xfd\xfez\"."
#define ENC_HINT_WIN_ZMUD_old   ENC_HINT_WIN_ZMUD_z
#define ENC_HINT_UTF8           "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb0\xd1\x8f "\
                                "\xd0\xb0\xd0\xb7\xd0\xb1\xd1\x83\xd0\xba\xd0\xb0: "\
                                "\"\xd0\xb0\xd0\xb1\xd0\xb2...\xd1\x8d\xd1\x8e\xd1\x8f\"."

static void ShowEncodingPrompt(DescriptorData *d, bool withHints = false) {
	if (withHints) {
		SEND_TO_Q(
			"\r\n"
			"Using keytable           TECT. CTPOKA\r\n"
			"  0) Koi-8               " ENC_HINT_KOI8R "\r\n"
													   "  1) Alt                 " ENC_HINT_ALT "\r\n"
																								"  2) Windows(JMC,MMC)    " ENC_HINT_WIN "\r\n"
																																		 "  3) Windows(zMUD)       " ENC_HINT_WIN_ZMUD "\r\n"
																																													   "  4) Windows(zMUD 'z')   " ENC_HINT_WIN_ZMUD_z "\r\n"
																																																									   "  5) UTF-8               " ENC_HINT_UTF8 "\r\n"
																																																																				 "  6) Windows(zMUD <6.39) " ENC_HINT_WIN_ZMUD_old "\r\n"
																																																																																   //			"Select one : ", d);
																																																																																   "\r\n"
																																																																																   "KAKOE HAnuCAHuE ECTb BEPHOE, PA3yMEEMOE HA PyCCKOM? BBEguTE HOMEP : ",
			d);
	} else {
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
void nanny(DescriptorData *d, char *arg) {
	char buf[kMaxStringLength];
	int player_i = 0, load_result;
	char tmp_name[kMaxInputLength], pwd_name[kMaxInputLength], pwd_pwd[kMaxInputLength];
	bool is_player_deleted;
	if (STATE(d) != CON_CONSOLE)
		skip_spaces(&arg);

	switch (STATE(d)) {
		case CON_INIT:
			// just connected
		{
			int online_players = 0;
			for (auto i = descriptor_list; i; i = i->next) {
				online_players++;
			}
			sprintf(buf, "Online: %d\r\n", online_players);
		}

			SEND_TO_Q(buf, d);
			ShowEncodingPrompt(d, false);
			STATE(d) = CON_GET_KEYTABLE;
			break;

			//. OLC states .
		case CON_OEDIT: oedit_parse(d, arg);
			break;

		case CON_REDIT: redit_parse(d, arg);
			break;

		case CON_ZEDIT: zedit_parse(d, arg);
			break;

		case CON_MEDIT: medit_parse(d, arg);
			break;

		case CON_TRIGEDIT: trigedit_parse(d, arg);
			break;

		case CON_MREDIT: mredit_parse(d, arg);
			break;

		case CON_CLANEDIT: d->clan_olc->clan->Manage(d, arg);
			break;

		case CON_SPEND_GLORY:
			if (!Glory::parse_spend_glory_menu(d->character.get(), arg)) {
				Glory::spend_glory_menu(d->character.get());
			}
			break;

		case CON_GLORY_CONST:
			if (!GloryConst::parse_spend_glory_menu(d->character.get(), arg)) {
				GloryConst::spend_glory_menu(d->character.get());
			}
			break;

		case CON_NAMED_STUFF:
			if (!NamedStuff::parse_nedit_menu(d->character.get(), arg)) {
				NamedStuff::nedit_menu(d->character.get());
			}
			break;

		case CON_MAP_MENU: d->map_options->parse_menu(d->character.get(), arg);
			break;

		case CON_TORC_EXCH: ExtMoney::torc_exch_parse(d->character.get(), arg);
			break;

		case CON_SEDIT: {
			try {
				obj_sets_olc::parse_input(d->character.get(), arg);
			}
			catch (const std::out_of_range &e) {
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
			if (*arg == '9') {
				ShowEncodingPrompt(d, true);
				return;
			};
			if (!*arg || *arg < '0' || *arg >= '0' + kCodePageLast) {
				SEND_TO_Q("\r\nUnknown key table. Retry, please : ", d);
				return;
			};
			d->keytable = (ubyte) *arg - (ubyte) '0';
			ip_log(d->host);
			SEND_TO_Q(GREETINGS, d);
			STATE(d) = CON_GET_NAME;
			break;

		case CON_GET_NAME:    // wait for input of name
			if (!d->character) {
				CreateChar(d);
			}

			if (!*arg) {
				STATE(d) = CON_CLOSE;
			} else if (!str_cmp("новый", arg)) {
				SEND_TO_Q(name_rules, d);

				std::stringstream ss;
				ss << "Введите имя";
				const auto free_name_list = player_table.name_adviser().get_random_name_list();
				if (!free_name_list.empty()) {
					ss << " (примеры доступных имен : ";
					ss << JoinRange(free_name_list);
					ss << ")";
				}

				ss << ": ";

				SEND_TO_Q(ss.str().c_str(), d);
				STATE(d) = CON_NEW_CHAR;
				return;
			} else {
				if (sscanf(arg, "%s %s", pwd_name, pwd_pwd) == 2) {
					if (parse_exist_name(pwd_name, tmp_name)
						|| (player_i = load_char(tmp_name, d->character.get())) < 0) {
						SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					}

					if (PLR_FLAGGED(d->character, PLR_DELETED)
						|| !Password::compare_password(d->character.get(), pwd_pwd)) {
						SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						if (!PLR_FLAGGED(d->character, PLR_DELETED)) {
							sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
							mudlog(buf, BRF, kLevelImmortal, SYSLOG, true);
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
				} else {
					if (parse_exist_name(arg, tmp_name) ||
						strlen(tmp_name) < (kMinNameLength - 1) || // дабы можно было войти чарам с 4 буквами
						strlen(tmp_name) > kMaxNameLength ||
						!Is_Valid_Name(tmp_name) || fill_word(tmp_name) || reserved_word(tmp_name)) {
						SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					} else if (!Is_Valid_Dc(tmp_name)) {
						player_i = load_char(tmp_name, d->character.get());
						d->character->set_pfilepos(player_i);
						if (IS_IMMORTAL(d->character) || PRF_FLAGGED(d->character, PRF_CODERINFO)) {
							SEND_TO_Q("Игрок с подобным именем является БЕССМЕРТНЫМ в игре.\r\n", d);
						} else {
							SEND_TO_Q("Игрок с подобным именем находится в игре.\r\n", d);
						}
						SEND_TO_Q("Во избежание недоразумений введите пару ИМЯ ПАРОЛЬ.\r\n", d);
						SEND_TO_Q("Имя и пароль через пробел : ", d);

						d->character.reset();
						return;
					}
				}

				player_i = load_char(tmp_name, d->character.get());
				if (player_i > -1) {
					d->character->set_pfilepos(player_i);
					if (PLR_FLAGGED(d->character,
									PLR_DELETED))    // We get a false positive from the original deleted character.
					{
						d->character.reset();

						// Check for multiple creations...
						if (!Valid_Name(tmp_name) || _parse_name(tmp_name, tmp_name)) {
							SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
							return;
						}

						// дополнительная проверка на длину имени чара
						if (strlen(tmp_name) < (kMinNameLength)) {
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
					} else    // undo it just in case they are set
					{
						if (IS_IMMORTAL(d->character) || PRF_FLAGGED(d->character, PRF_CODERINFO)) {
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
				} else    // player unknown -- make new character
				{
					// еще одна проверка
					if (strlen(tmp_name) < (kMinNameLength)) {
						SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					}

					// Check for multiple creations of a character.
					if (!Valid_Name(tmp_name) || _parse_name(tmp_name, tmp_name)) {
						SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					}

					if (cmp_ptable_by_name(tmp_name, kMinNameLength) >= 0) {
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

		case CON_NAME_CNFRM:    // wait for conf. of new name
			if (UPPER(*arg) == 'Y' || UPPER(*arg) == 'Д') {
				if (ban->is_banned(d->host) >= BanList::BAN_NEW) {
					sprintf(buf, "Попытка создания персонажа %s отклонена для [%s] (siteban)",
							GET_PC_NAME(d->character), d->host);
					mudlog(buf, NRM, kLevelGod, SYSLOG, true);
					SEND_TO_Q("Извините, создание нового персонажа для вашего IP !!! ЗАПРЕЩЕНО !!!\r\n", d);
					STATE(d) = CON_CLOSE;
					return;
				}

				if (circle_restrict) {
					SEND_TO_Q("Извините, вы не можете создать новый персонаж в настоящий момент.\r\n", d);
					sprintf(buf, "Попытка создания нового персонажа %s отклонена для [%s] (wizlock)",
							GET_PC_NAME(d->character), d->host);
					mudlog(buf, NRM, kLevelGod, SYSLOG, true);
					STATE(d) = CON_CLOSE;
					return;
				}

				switch (NewNames::auto_authorize(d)) {
					case NewNames::AUTO_ALLOW:
						sprintf(buf,
								"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
								GET_PAD(d->character, 1));
						SEND_TO_Q(buf, d);
						STATE(d) = CON_NEWPASSWD;
						return;

					case NewNames::AUTO_BAN: STATE(d) = CON_CLOSE;
						return;

					default: break;
				}

				SEND_TO_Q("Ваш пол [ М(M)/Ж(F) ]? ", d);
				STATE(d) = CON_QSEX;
				return;

			} else if (UPPER(*arg) == 'N' || UPPER(*arg) == 'Н') {
				SEND_TO_Q("Итак, чего изволите? Учтите, бананов нет :)\r\n" "Имя : ", d);
				d->character->set_pc_name(0);
				STATE(d) = CON_GET_NAME;
			} else {
				SEND_TO_Q("Ответьте Yes(Да) or No(Нет) : ", d);
			}
			break;

		case CON_NEW_CHAR:
			if (!*arg) {
				STATE(d) = CON_CLOSE;
				return;
			}

			if (!d->character) {
				CreateChar(d);
			}

			if (_parse_name(arg, tmp_name) ||
				strlen(tmp_name) < kMinNameLength ||
				strlen(tmp_name) > kMaxNameLength ||
				!Is_Valid_Name(tmp_name) || fill_word(tmp_name) || reserved_word(tmp_name)) {
				SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				return;
			}

			player_i = load_char(tmp_name, d->character.get());
			is_player_deleted = false;
			if (player_i > -1) {
				is_player_deleted = PLR_FLAGGED(d->character, PLR_DELETED);
				if (is_player_deleted) {
					d->character.reset();
					CreateChar(d);
				} else {
					SEND_TO_Q("Такой персонаж уже существует. Выберите другое имя : ", d);
					d->character.reset();

					return;
				}
			}

			if (!Valid_Name(tmp_name)) {
				SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				return;
			}

			// skip name check for deleted players
			if (!is_player_deleted && cmp_ptable_by_name(tmp_name, kMinNameLength) >= 0) {
				SEND_TO_Q("Первые символы вашего имени совпадают с уже существующим персонажем.\r\n"
						  "Для исключения разных недоразумений вам необходимо выбрать другое имя.\r\n"
						  "Имя  : ", d);
				return;
			}

			d->character->set_pc_name(CAP(tmp_name));
			d->character->player_data.PNames[0] = std::string(CAP(tmp_name));
			if (is_player_deleted) {
				d->character->set_pfilepos(player_i);
			}
			if (ban->is_banned(d->host) >= BanList::BAN_NEW) {
				sprintf(buf, "Попытка создания персонажа %s отклонена для [%s] (siteban)",
						GET_PC_NAME(d->character), d->host);
				mudlog(buf, NRM, kLevelGod, SYSLOG, true);
				SEND_TO_Q("Извините, создание нового персонажа для вашего IP !!!ЗАПРЕЩЕНО!!!\r\n", d);
				STATE(d) = CON_CLOSE;
				return;
			}

			if (circle_restrict) {
				SEND_TO_Q("Извините, вы не можете создать новый персонаж в настоящий момент.\r\n", d);
				sprintf(buf,
						"Попытка создания нового персонажа %s отклонена для [%s] (wizlock)",
						GET_PC_NAME(d->character), d->host);
				mudlog(buf, NRM, kLevelGod, SYSLOG, true);
				STATE(d) = CON_CLOSE;
				return;
			}

			switch (NewNames::auto_authorize(d)) {
				case NewNames::AUTO_ALLOW:
					sprintf(buf,
							"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
							GET_PAD(d->character, 1));
					SEND_TO_Q(buf, d);
					STATE(d) = CON_NEWPASSWD;
					return;

				case NewNames::AUTO_BAN: d->character.reset();
					SEND_TO_Q("Выберите другое имя : ", d);
					return;

				default: break;
			}

			SEND_TO_Q("Ваш пол [ М(M)/Ж(F) ]? ", d);
			STATE(d) = CON_QSEX;
			return;

		case CON_PASSWORD:    // get pwd for known player
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

			if (!*arg) {
				STATE(d) = CON_CLOSE;
			} else {
				if (!Password::compare_password(d->character.get(), arg)) {
					sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
					mudlog(buf, BRF, kLevelImmortal, SYSLOG, true);
					GET_BAD_PWS(d->character)++;
					d->character->save_char();
					if (++(d->bad_pws) >= max_bad_pws)    // 3 strikes and you're out.
					{
						SEND_TO_Q("Неверный пароль... Отсоединяемся.\r\n", d);
						STATE(d) = CON_CLOSE;
					} else {
						SEND_TO_Q("Неверный пароль.\r\nПароль : ", d);
					}
					return;
				}
				DoAfterPassword(d);
			}
			break;

		case CON_NEWPASSWD:
		case CON_CHPWD_GETNEW:
			if (!Password::check_password(d->character.get(), arg)) {
				sprintf(buf, "\r\n%s\r\n", Password::BAD_PASSWORD);
				SEND_TO_Q(buf, d);
				SEND_TO_Q("Пароль : ", d);
				return;
			}

			Password::set_password(d->character.get(), arg);

			SEND_TO_Q("\r\nПовторите пароль, пожалуйста : ", d);
			if (STATE(d) == CON_NEWPASSWD) {
				STATE(d) = CON_CNFPASSWD;
			} else {
				STATE(d) = CON_CHPWD_VRFY;
			}

			break;

		case CON_CNFPASSWD:
		case CON_CHPWD_VRFY:
			if (!Password::compare_password(d->character.get(), arg)) {
				SEND_TO_Q("\r\nПароли не соответствуют... повторим.\r\n", d);
				SEND_TO_Q("Пароль: ", d);
				if (STATE(d) == CON_CNFPASSWD) {
					STATE(d) = CON_NEWPASSWD;
				} else {
					STATE(d) = CON_CHPWD_GETNEW;
				}
				return;
			}

			if (STATE(d) == CON_CNFPASSWD) {
				GET_KIN(d->character) = 0; // added by WorM: Выставляем расу в Русич(коммент выше)
				SEND_TO_Q(class_menu, d);
				SEND_TO_Q("\r\nВаша профессия (Для более полной информации вы можете набрать"
						  " \r\nсправка <интересующая профессия>): ", d);
				STATE(d) = CON_QCLASS;
			} else {
				sprintf(buf, "%s заменил себе пароль.", GET_NAME(d->character));
				add_karma(d->character.get(), buf, "");
				d->character->save_char();
				SEND_TO_Q("\r\nГотово.\r\n", d);
				SEND_TO_Q(MENU, d);
				STATE(d) = CON_MENU;
			}

			break;

		case CON_QSEX:        // query sex of new user
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q("\r\nВаш пол [ М(M)/Ж(F) ]? ", d);
				STATE(d) = CON_QSEX;
				return;
			}

			switch (UPPER(*arg)) {
				case 'М':
				case 'M': d->character->set_sex(ESex::kMale);
					break;

				case 'Ж':
				case 'F': d->character->set_sex(ESex::kFemale);
					break;

				default: SEND_TO_Q("Это может быть и пол, но явно не ваш :)\r\n" "А какой у ВАС пол? ", d);
					return;
			}
			SEND_TO_Q("Проверьте правильность склонения имени. В случае ошибки введите свой вариант.\r\n", d);
			GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 1, tmp_name);
			sprintf(buf, "Имя в родительном падеже (меч КОГО?) [%s]: ", tmp_name);
			SEND_TO_Q(buf, d);
			STATE(d) = CON_NAME2;
			return;

		case CON_QKIN:        // query rass
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q("\r\nКакой народ вам ближе по духу:\r\n", d);
				SEND_TO_Q(string(PlayerRace::ShowKinsMenu()).c_str(), d);
				SEND_TO_Q("\r\nПлемя: ", d);
				STATE(d) = CON_QKIN;
				return;
			}

			load_result = PlayerRace::CheckKin(arg);
			if (load_result == KIN_UNDEFINED) {
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

		case CON_RELIGION:    // query religion of new user
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q(religion_menu, d);
				SEND_TO_Q("\n\rРелигия :", d);
				STATE(d) = CON_RELIGION;
				return;
			}

			switch (UPPER(*arg)) {
				case 'Я':
				case 'З':
				case 'P':
					if (class_religion[(int) GET_CLASS(d->character)] == kReligionMono) {
						SEND_TO_Q
						("Персонаж выбранной вами профессии не желает быть язычником!\r\n"
						 "Так каким Богам вы хотите служить? ", d);
						return;
					}
					GET_RELIGION(d->character) = kReligionPoly;
					break;

				case 'Х':
				case 'C':
					if (class_religion[(int) GET_CLASS(d->character)] == kReligionPoly) {
						SEND_TO_Q
						("Персонажу выбранной вами профессии противно христианство!\r\n"
						 "Так каким Богам вы хотите служить? ", d);
						return;
					}
					GET_RELIGION(d->character) = kReligionMono;
					break;

				default: SEND_TO_Q("Атеизм сейчас не моден :)\r\n" "Так каким Богам вы хотите служить? ", d);
					return;
			}

			SEND_TO_Q("\r\nКакой род вам ближе всего по духу:\r\n", d);
			SEND_TO_Q(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
			sprintf(buf, "Для вашей профессией больше всего подходит %s", default_race[GET_CLASS(d->character)]);
			SEND_TO_Q(buf, d);
			SEND_TO_Q("\r\nИз чьих вы будете : ", d);
			STATE(d) = CON_RACE;

			break;

		case CON_QCLASS: {
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q(class_menu, d);
				SEND_TO_Q("\r\nВаша профессия : ", d);
				STATE(d) = CON_QCLASS;
				return;
			}

			auto class_id = ParseClass(*arg);
			if (class_id == ECharClass::kUndefined) {
				SEND_TO_Q("\r\nЭто не профессия.\r\nПрофессия : ", d);
				return;
			} else {
				d->character->set_class(class_id);
			}

			SEND_TO_Q(religion_menu, d);
			SEND_TO_Q("\n\rРелигия :", d);
			STATE(d) = CON_RELIGION;
			break;
		}
		// ABYRVALG - вырезать
		case CON_QCLASSS: {
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q(class_menu_step, d);
				SEND_TO_Q("\r\nВаша профессия : ", d);
				STATE(d) = CON_QCLASSS;
				return;
			}

			auto class_id = static_cast<ECharClass>(ParseClass(*arg));
			if (class_id == ECharClass::kUndefined) {
				SEND_TO_Q("\r\nЭто не профессия.\r\nПрофессия : ", d);
				return;
			} else {
				d->character->set_class(class_id);
			}

			SEND_TO_Q(religion_menu, d);
			SEND_TO_Q("\n\rРелигия :", d);
			STATE(d) = CON_RELIGION;

			break;
		}
		case CON_QCLASSV: {
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q(class_menu_vik, d);
				SEND_TO_Q("\r\nВаша профессия : ", d);
				STATE(d) = CON_QCLASSV;
				return;
			}

			auto class_id = static_cast<ECharClass>(ParseClass(*arg));
			if (class_id == ECharClass::kUndefined) {
				SEND_TO_Q("\r\nЭто не профессия.\r\nПрофессия : ", d);
				return;
			} else {
				d->character->set_class(class_id);
			}

			SEND_TO_Q(religion_menu, d);
			SEND_TO_Q("\n\rРелигия:", d);
			STATE(d) = CON_RELIGION;

			break;
		}
		case CON_RACE:        // query race
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q("Какой род вам ближе всего по духу:\r\n", d);
				SEND_TO_Q(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
				SEND_TO_Q("\r\nРод: ", d);
				STATE(d) = CON_RACE;
				return;
			}

			load_result = PlayerRace::CheckRace(GET_KIN(d->character), arg);

			if (load_result == RACE_UNDEFINED) {
				SEND_TO_Q("Стыдно не помнить предков.\r\n" "Какой род вам ближе всего? ", d);
				return;
			}

			GET_RACE(d->character) = load_result;
			SEND_TO_Q(string(Birthplaces::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(d->character),
																				  GET_RACE(d->character)))).c_str(), d);
			SEND_TO_Q("\r\nГде вы хотите начать свои приключения: ", d);
			STATE(d) = CON_BIRTHPLACE;

			break;

		case CON_BIRTHPLACE:
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q(string(Birthplaces::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(d->character),
																					  GET_RACE(d->character)))).c_str(),
						  d);
				SEND_TO_Q("\r\nГде вы хотите начать свои приключения: ", d);
				STATE(d) = CON_BIRTHPLACE;
				return;
			}

			load_result = PlayerRace::CheckBirthPlace(GET_KIN(d->character), GET_RACE(d->character), arg);

			if (!Birthplaces::CheckId(load_result)) {
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
			if (pre_help(d->character.get(), arg)) {
				genchar_disp_menu(d->character.get());
				STATE(d) = CON_ROLL_STATS;
				return;
			}

			switch (genchar_parse(d->character.get(), arg)) {
				case GENCHAR_CONTINUE: genchar_disp_menu(d->character.get());
					break;
				default: SEND_TO_Q("\r\nВведите ваш E-mail"
								   "\r\n(ВСЕ ВАШИ ПЕРСОНАЖИ ДОЛЖНЫ ИМЕТЬ ОДИНАКОВЫЙ E-mail)."
								   "\r\nНа этот адрес вам будет отправлен код для подтверждения: ", d);
					STATE(d) = CON_GET_EMAIL;
					break;
			}
			break;

		case CON_GET_EMAIL:
			if (!*arg) {
				SEND_TO_Q("\r\nВаш E-mail : ", d);
				return;
			} else if (!IsValidEmail(arg)) {
				SEND_TO_Q("\r\nНекорректный E-mail!" "\r\nВаш E-mail :  ", d);
				return;
			}
#ifdef TEST_BUILD
			strncpy(GET_EMAIL(d->character), arg, 127);
			*(GET_EMAIL(d->character) + 127) = '\0';
			utils::ConvertToLow(GET_EMAIL(d->character));
			DoAfterEmailConfirm(d);
			break;
#endif
			{
				int random_number = number(1000000, 9999999);
				new_char_codes[d->character->get_pc_name()] = random_number;
				strncpy(GET_EMAIL(d->character), arg, 127);
				*(GET_EMAIL(d->character) + 127) = '\0';
				utils::ConvertToLow(GET_EMAIL(d->character));
				std::string cmd_line =
					str(boost::format("python3 send_code.py %s %d &") % GET_EMAIL(d->character) % random_number);
				auto result = system(cmd_line.c_str());
				UNUSED_ARG(result);
				SEND_TO_Q("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
				STATE(d) = CON_RANDOM_NUMBER;
			}
			break;

		case CON_RMOTD:    // read CR after printing motd
			if (!check_dupes_email(d)) {
				STATE(d) = CON_CLOSE;
				break;
			}

			do_entergame(d);

			break;

		case CON_RANDOM_NUMBER: {
			int code_rand = atoi(arg);

			if (new_char_codes.count(d->character->get_pc_name()) != 0) {
				if (new_char_codes[d->character->get_pc_name()] != code_rand) {
					SEND_TO_Q("\r\nВы ввели неправильный код, попробуйте еще раз.\r\n", d);
					break;
				}
				new_char_codes.erase(d->character->get_pc_name());
				DoAfterEmailConfirm(d);
				break;
			}

			if (new_loc_codes.count(GET_EMAIL(d->character)) == 0) {
				break;
			}

			if (new_loc_codes[GET_EMAIL(d->character)] != code_rand) {
				SEND_TO_Q("\r\nВы ввели неправильный код, попробуйте еще раз.\r\n", d);
				STATE(d) = CON_CLOSE;
				break;
			}

			new_loc_codes.erase(GET_EMAIL(d->character));
			add_logon_record(d);
			DoAfterPassword(d);

			break;
		}

		case CON_MENU:        // get selection from main menu
			switch (*arg) {
				case '0': SEND_TO_Q("\r\nДо встречи на земле Киевской.\r\n", d);

					if (GET_REAL_REMORT(d->character) == 0
						&& GetRealLevel(d->character) <= 25
						&& !PLR_FLAGS(d->character).get(PLR_NODELETE)) {
						int timeout = -1;
						for (int ci = 0; GetRealLevel(d->character) > pclean_criteria[ci].level; ci++) {
							//if (GetRealLevel(d->character) == pclean_criteria[ci].level)
							timeout = pclean_criteria[ci + 1].days;
						}
						if (timeout > 0) {
							time_t deltime = time(nullptr) + timeout * 60 * rent_file_timeout * 24;
							sprintf(buf, "В случае вашего отсутствия персонаж будет храниться до %s нашей эры :).\r\n",
									rustime(localtime(&deltime)));
							SEND_TO_Q(buf, d);
						}
					};

					STATE(d) = CON_CLOSE;

					break;

				case '1':
					if (!check_dupes_email(d)) {
						STATE(d) = CON_CLOSE;
						break;
					}

					do_entergame(d);

					break;

				case '2':
					if (d->character->player_data.description != "") {
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

					d->writer.reset(new utils::DelegatedStdStringWriter(d->character->player_data.description));
					d->max_str = kExdscrLength;
					STATE(d) = CON_EXDESC;

					break;

				case '3': page_string(d, background, 0);
					STATE(d) = CON_RMOTD;
					break;

				case '4': SEND_TO_Q("\r\nВведите СТАРЫЙ пароль : ", d);
					STATE(d) = CON_CHPWD_GETOLD;
					break;

				case '5':
					if (IS_IMMORTAL(d->character)) {
						SEND_TO_Q("\r\nБоги бессмертны (с) Стрибог, просите чтоб пофризили :)))\r\n", d);
						SEND_TO_Q(MENU, d);
						break;
					}

					if (PLR_FLAGGED(d->character, PLR_HELLED)
						|| PLR_FLAGGED(d->character, PLR_FROZEN)) {
						SEND_TO_Q("\r\nВы находитесь в АДУ!!! Амнистии подобным образом не будет.\r\n", d);
						SEND_TO_Q(MENU, d);
						break;
					}

					if (GET_REAL_REMORT(d->character) > 5) {
						SEND_TO_Q("\r\nНельзя удалить себя достигнув шестого перевоплощения.\r\n", d);
						SEND_TO_Q(MENU, d);
						break;
					}

					SEND_TO_Q("\r\nДля подтверждения введите свой пароль : ", d);
					STATE(d) = CON_DELCNF1;

					break;

				case '6':
					if (IS_IMMORTAL(d->character)) {
						SEND_TO_Q("\r\nВам это ни к чему...\r\n", d);
						SEND_TO_Q(MENU, d);
						STATE(d) = CON_MENU;
					} else {
						ResetStats::print_menu(d);
						STATE(d) = CON_MENU_STATS;
					}
					break;

				case '7':
					if (!PRF_FLAGGED(d->character, PRF_BLIND)) {
						PRF_FLAGS(d->character).set(PRF_BLIND);
						SEND_TO_Q("\r\nСпециальный режим слепого игрока ВКЛЮЧЕН.\r\n", d);
						SEND_TO_Q(MENU, d);
						STATE(d) = CON_MENU;
					} else {
						PRF_FLAGS(d->character).unset(PRF_BLIND);
						SEND_TO_Q("\r\nСпециальный режим слепого игрока ВЫКЛЮЧЕН.\r\n", d);
						SEND_TO_Q(MENU, d);
						STATE(d) = CON_MENU;
					}

					break;
				case '8': d->character->get_account()->show_list_players(d);
					break;

				default: SEND_TO_Q("\r\nЭто не есть правильный ответ!\r\n", d);
					SEND_TO_Q(MENU, d);

					break;
			}

			break;

		case CON_CHPWD_GETOLD:
			if (!Password::compare_password(d->character.get(), arg)) {
				SEND_TO_Q("\r\nНеверный пароль.\r\n", d);
				SEND_TO_Q(MENU, d);
				STATE(d) = CON_MENU;
			} else {
				SEND_TO_Q("\r\nВведите НОВЫЙ пароль : ", d);
				STATE(d) = CON_CHPWD_GETNEW;
			}

			return;

		case CON_DELCNF1:
			if (!Password::compare_password(d->character.get(), arg)) {
				SEND_TO_Q("\r\nНеверный пароль.\r\n", d);
				SEND_TO_Q(MENU, d);
				STATE(d) = CON_MENU;
			} else {
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
				|| !strcmp(arg, "ДА")) {
				if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
					SEND_TO_Q("Вы решились на суицид, но Боги остановили вас.\r\n", d);
					SEND_TO_Q("Персонаж не удален.\r\n", d);
					STATE(d) = CON_CLOSE;
					return;
				}
				if (GetRealLevel(d->character) >= kLevelGreatGod)
					return;
				delete_char(GET_NAME(d->character));
				sprintf(buf, "Персонаж '%s' удален!\r\n" "До свидания.\r\n", GET_NAME(d->character));
				SEND_TO_Q(buf, d);
				sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GetRealLevel(d->character));
				mudlog(buf, NRM, kLevelGod, SYSLOG, true);
				d->character->get_account()->remove_player(GetUniqueByName(GET_NAME(d->character)));
				STATE(d) = CON_CLOSE;
				return;
			} else {
				SEND_TO_Q("\r\nПерсонаж не удален.\r\n", d);
				SEND_TO_Q(MENU, d);
				STATE(d) = CON_MENU;
			}
			break;

		case CON_NAME2: skip_spaces(&arg);
			if (strlen(arg) == 0) {
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 1, arg);
			}
			if (!_parse_name(arg, tmp_name)
				&& strlen(tmp_name) >= kMinNameLength
				&& strlen(tmp_name) <= kMaxNameLength
				&& !strn_cmp(tmp_name,
							 GET_PC_NAME(d->character),
							 std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))) {
				d->character->player_data.PNames[1] = std::string(CAP(tmp_name));
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 2, tmp_name);
				sprintf(buf, "Имя в дательном падеже (отправить КОМУ?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_NAME3;
			} else {
				SEND_TO_Q("Некорректно.\r\n", d);
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 1, tmp_name);
				sprintf(buf, "Имя в родительном падеже (меч КОГО?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
			};
			break;

		case CON_NAME3: skip_spaces(&arg);

			if (strlen(arg) == 0) {
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 2, arg);
			}

			if (!_parse_name(arg, tmp_name)
				&& strlen(tmp_name) >= kMinNameLength
				&& strlen(tmp_name) <= kMaxNameLength
				&& !strn_cmp(tmp_name,
							 GET_PC_NAME(d->character),
							 std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))) {
				d->character->player_data.PNames[2] = std::string(CAP(tmp_name));
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 3, tmp_name);
				sprintf(buf, "Имя в винительном падеже (ударить КОГО?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_NAME4;
			} else {
				SEND_TO_Q("Некорректно.\r\n", d);
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 2, tmp_name);
				sprintf(buf, "Имя в дательном падеже (отправить КОМУ?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
			};

			break;

		case CON_NAME4: skip_spaces(&arg);

			if (strlen(arg) == 0) {
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 3, arg);
			}

			if (!_parse_name(arg, tmp_name)
				&& strlen(tmp_name) >= kMinNameLength
				&& strlen(tmp_name) <= kMaxNameLength
				&& !strn_cmp(tmp_name,
							 GET_PC_NAME(d->character),
							 std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))) {
				d->character->player_data.PNames[3] = std::string(CAP(tmp_name));
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 4, tmp_name);
				sprintf(buf, "Имя в творительном падеже (сражаться с КЕМ?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_NAME5;
			} else {
				SEND_TO_Q("Некорректно.\n\r", d);
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 3, tmp_name);
				sprintf(buf, "Имя в винительном падеже (ударить КОГО?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
			};

			break;

		case CON_NAME5: skip_spaces(&arg);
			if (strlen(arg) == 0)
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 4, arg);
			if (!_parse_name(arg, tmp_name) &&
				strlen(tmp_name) >= kMinNameLength && strlen(tmp_name) <= kMaxNameLength &&
				!strn_cmp(tmp_name,
						  GET_PC_NAME(d->character),
						  std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))
				) {
				d->character->player_data.PNames[4] = std::string(CAP(tmp_name));
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 5, tmp_name);
				sprintf(buf, "Имя в предложном падеже (говорить о КОМ?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_NAME6;
			} else {
				SEND_TO_Q("Некорректно.\n\r", d);
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 4, tmp_name);
				sprintf(buf, "Имя в творительном падеже (сражаться с КЕМ?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
			};
			break;
		case CON_NAME6: skip_spaces(&arg);
			if (strlen(arg) == 0)
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 5, arg);
			if (!_parse_name(arg, tmp_name) &&
				strlen(tmp_name) >= kMinNameLength && strlen(tmp_name) <= kMaxNameLength &&
				!strn_cmp(tmp_name,
						  GET_PC_NAME(d->character),
						  std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))
				) {
				d->character->player_data.PNames[5] = std::string(CAP(tmp_name));
				sprintf(buf,
						"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
						GET_PAD(d->character, 1));
				SEND_TO_Q(buf, d);
				STATE(d) = CON_NEWPASSWD;
			} else {
				SEND_TO_Q("Некорректно.\n\r", d);
				GetCase(GET_PC_NAME(d->character), GET_SEX(d->character), 5, tmp_name);
				sprintf(buf, "Имя в предложном падеже (говорить о КОМ?) [%s]: ", tmp_name);
				SEND_TO_Q(buf, d);
			};
			break;

		case CON_CLOSE: break;

		case CON_RESET_STATS:
			if (pre_help(d->character.get(), arg)) {
				return;
			}

			switch (genchar_parse(d->character.get(), arg)) {
				case GENCHAR_CONTINUE: genchar_disp_menu(d->character.get());
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
					if (!ValidateStats(d)) {
						return;
					}

					SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
					STATE(d) = CON_RMOTD;
			}

			break;

		case CON_RESET_KIN:
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q("\r\nКакой народ вам ближе по духу:\r\n", d);
				SEND_TO_Q(string(PlayerRace::ShowKinsMenu()).c_str(), d);
				SEND_TO_Q("\r\nПлемя: ", d);
				STATE(d) = CON_RESET_KIN;
				return;
			}

			load_result = PlayerRace::CheckKin(arg);

			if (load_result == KIN_UNDEFINED) {
				SEND_TO_Q("Стыдно не помнить предков.\r\n"
						  "Какое Племя вам ближе по духу? ", d);
				return;
			}

			GET_KIN(d->character) = load_result;

			if (!ValidateStats(d)) {
				return;
			}

			SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
			STATE(d) = CON_RMOTD;
			break;

		case CON_RESET_RACE:
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q("Какой род вам ближе всего по духу:\r\n", d);
				SEND_TO_Q(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
				SEND_TO_Q("\r\nРод: ", d);
				STATE(d) = CON_RESET_RACE;
				return;
			}

			load_result = PlayerRace::CheckRace(GET_KIN(d->character), arg);

			if (load_result == RACE_UNDEFINED) {
				SEND_TO_Q("Стыдно не помнить предков.\r\n" "Какой род вам ближе всего? ", d);
				return;
			}

			GET_RACE(d->character) = load_result;

			if (!ValidateStats(d)) {
				return;
			}

			// способности нового рода проставятся дальше в do_entergame
			SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
			STATE(d) = CON_RMOTD;

			break;

		case CON_MENU_STATS: ResetStats::parse_menu(d, arg);
			break;

		case CON_RESET_RELIGION:
			if (pre_help(d->character.get(), arg)) {
				SEND_TO_Q(religion_menu, d);
				SEND_TO_Q("\n\rРелигия :", d);
				return;
			}

			switch (UPPER(*arg)) {
				case 'Я':
				case 'З':
				case 'P':
					if (class_religion[(int) GET_CLASS(d->character)] == kReligionMono) {
						SEND_TO_Q
						("Персонаж выбранной вами профессии не желает быть язычником!\r\n"
						 "Так каким Богам вы хотите служить? ", d);
						return;
					}
					GET_RELIGION(d->character) = kReligionPoly;
					break;

				case 'Х':
				case 'C':
					if (class_religion[(int) GET_CLASS(d->character)] == kReligionPoly) {
						SEND_TO_Q ("Персонажу выбранной вами профессии противно христианство!\r\n"
								   "Так каким Богам вы хотите служить? ", d);
						return;
					}

					GET_RELIGION(d->character) = kReligionMono;

					break;

				default: SEND_TO_Q("Атеизм сейчас не моден :)\r\n" "Так каким Богам вы хотите служить? ", d);
					return;
			}

			if (!ValidateStats(d)) {
				return;
			}

			SEND_TO_Q("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
			STATE(d) = CON_RMOTD;

			break;

		default:
			log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
				STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
			STATE(d) = CON_DISCONNECT;    // Safest to do.

			break;
	}
}

// берем из первой строки одно слово или подстроку в кавычках, результат удаляется из buffer
void GetOneParam(std::string &in_buffer, std::string &out_buffer) {
	std::string::size_type beg_idx = 0, end_idx = 0;
	beg_idx = in_buffer.find_first_not_of(' ');

	if (beg_idx != std::string::npos) {
		// случай с кавычками
		if (in_buffer[beg_idx] == '\'') {
			if (std::string::npos != (beg_idx = in_buffer.find_first_not_of('\'', beg_idx))) {
				if (std::string::npos == (end_idx = in_buffer.find_first_of('\'', beg_idx))) {
					out_buffer = in_buffer.substr(beg_idx);
					in_buffer.clear();
				} else {
					out_buffer = in_buffer.substr(beg_idx, end_idx - beg_idx);
					in_buffer.erase(0, ++end_idx);
				}
			}
			// случай с одним параметром через пробел
		} else {
			if (std::string::npos != (beg_idx = in_buffer.find_first_not_of(' ', beg_idx))) {
				if (std::string::npos == (end_idx = in_buffer.find_first_of(' ', beg_idx))) {
					out_buffer = in_buffer.substr(beg_idx);
					in_buffer.clear();
				} else {
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
bool CompareParam(const std::string &buffer, const char *arg, bool full) {
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
bool CompareParam(const std::string &buffer, const std::string &buffer2, bool full) {
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
DescriptorData *DescByUID(int uid) {
	DescriptorData *d = 0;

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
DescriptorData *get_desc_by_id(long id, bool playing) {
	DescriptorData *d = 0;

	if (playing) {
		for (d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && GET_IDNUM(d->character) == id)
				break;
	} else {
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
long GetUniqueByName(const std::string &name, bool god) {
	for (std::size_t i = 0; i < player_table.size(); ++i) {
		if (!str_cmp(player_table[i].name(), name) && player_table[i].unique != -1) {
			if (!god)
				return player_table[i].unique;
			else {
				if (player_table[i].level < kLevelImmortal)
					return player_table[i].unique;
				else
					return -1;
			}

		}
	}
	return 0;
}

bool IsActiveUser(long unique) {
	time_t currTime = time(0);
	time_t charLogon;
	int inactivityDelay = /* day*/ (3600 * 24) * /*days count*/ 60;
	for (std::size_t i = 0; i < player_table.size(); ++i) {
		if (player_table[i].unique == unique) {
			charLogon = player_table[i].last_logon;
			return currTime - charLogon < inactivityDelay;
		}
	}
	return false;
}

// ищет имя игрока по его УИДу, второй необязательный параметр - учитывать или нет БОГОВ
std::string GetNameByUnique(long unique, bool god) {
	std::string empty;

	for (std::size_t i = 0; i < player_table.size(); ++i) {
		if (player_table[i].unique == unique) {
			if (!god) {
				return player_table[i].name();
			} else {
				if (player_table[i].level < kLevelImmortal) {
					return player_table[i].name();
				} else {
					return empty;
				}
			}
		}
	}

	return empty;
}

// замена в name русских символов на англ в нижнем регистре (для файлов)
void CreateFileName(std::string &name) {
	for (unsigned i = 0; i != name.length(); ++i)
		name[i] = LOWER(AtoL(name[i]));
}

// вывод экспы аля диабла
std::string ExpFormat(long long exp) {
	std::string prefix;
	if (exp < 0) {
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

// * Конвертация имени в нижний регистр + первый сивмол в верхний (для единообразного поиска в контейнерах)
void name_convert(std::string &text) {
	if (!text.empty()) {
		utils::ConvertToLow(text);
		*text.begin() = UPPER(*text.begin());
	}
}

// * Генерация списка неодобренных титулов и имен и вывод их имму
bool single_god_invoice(CharData *ch) {
	bool hasMessages = false;
	hasMessages |= TitleSystem::show_title_list(ch);
	hasMessages |= NewNames::show(ch);
	return hasMessages;
}

// * Поиск незанятых иммов онлайн для вывода им неодобренных титулов и имен раз в 5 минут
void god_work_invoice() {
	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character && STATE(d) == CON_PLAYING) {
			if (IS_IMMORTAL(d->character)
				|| GET_GOD_FLAG(d->character, GF_DEMIGOD)) {
				single_god_invoice(d->character.get());
			}
		}
	}
}

// * Вывод оповещений о новых сообщениях на досках, письмах, (неодобренных имен и титулов для иммов) при логине и релогине
bool login_change_invoice(CharData *ch) {
	bool hasMessages = false;

	hasMessages |= Boards::Static::LoginInfo(ch);

	if (IS_IMMORTAL(ch))
		hasMessages |= single_god_invoice(ch);

	if (mail::has_mail(ch->get_uid())) {
		hasMessages = true;
		send_to_char("&RВас ожидает письмо. ЗАЙДИТЕ НА ПОЧТУ!&n\r\n", ch);
	}
	if (Parcel::has_parcel(ch)) {
		hasMessages = true;
		send_to_char("&RВас ожидает посылка. ЗАЙДИТЕ НА ПОЧТУ!&n\r\n", ch);
	}
	hasMessages |= Depot::show_purged_message(ch);
	if (CLAN(ch)) {
		hasMessages |= CLAN(ch)->print_mod(ch);
	}

	return hasMessages;
}

// спам-контроль для команды кто и списка по дружинам
// работает аналогично восстановлению и расходованию маны у волхвов
// константы пока определены через #define в interpreter.h
// возвращает истину, если спамконтроль сработал и игроку придется подождать
bool who_spamcontrol(CharData *ch, unsigned short int mode = WHO_LISTALL) {
	int cost = 0;
	time_t ctime;

	if (IS_IMMORTAL(ch))
		return false;

	ctime = time(0);

	switch (mode) {
		case WHO_LISTALL: cost = WHO_COST;
			break;
		case WHO_LISTNAME: cost = WHO_COST_NAME;
			break;
		case WHO_LISTCLAN: cost = WHO_COST_CLAN;
			break;
	}

	int mana = ch->get_who_mana();
	int last = ch->get_who_last();

#ifdef WHO_DEBUG
	send_to_char(boost::str(boost::format("\r\nСпам-контроль:\r\n  было маны: %u, расход: %u\r\n") % ch->get_who_mana() % cost).c_str(), ch);
#endif

	// рестим ману, в БД скорость реста маны удваивается
	mana = MIN(WHO_MANA_MAX,
			   mana + (ctime - last) * WHO_MANA_REST_PER_SECOND
				   + (ctime - last) * WHO_MANA_REST_PER_SECOND * (NORENTABLE(ch) ? 1 : 0));

#ifdef WHO_DEBUG
	send_to_char(boost::str(boost::format("  прошло %u с, восстановили %u, мана после регена: %u\r\n") %
										  (ctime - last) % (mana - ch->get_who_mana()) % mana).c_str(), ch);
#endif

	ch->set_who_mana(mana);
	ch->set_who_last(ctime);

	if (mana < cost) {
		send_to_char("Запрос обрабатывается, ожидайте...\r\n", ch);
		return true;
	} else {
		mana -= cost;
		ch->set_who_mana(mana);
	}
#ifdef WHO_DEBUG
	send_to_char(boost::str(boost::format("  осталось маны: %u\r\n") % mana).c_str(), ch);
#endif
	return false;
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
