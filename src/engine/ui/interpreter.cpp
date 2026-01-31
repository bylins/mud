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

#define INTERPRETER_CPP_

#include "interpreter.h"

#include "engine/core/char_movement.h"
#include "administration/ban.h"
#include "administration/karma.h"
#include "gameplay/communication/boards/boards.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/world_characters.h"
#include "gameplay/communication/insult.h"
#include "gameplay/communication/offtop.h"
#include "engine/ui/cmd_god/do_advance.h"
#include "engine/ui/cmd_god/do_arena_restore.h"
#include "engine/ui/cmd_god/do_at_room.h"
#include "engine/ui/cmd_god/do_occupation.h"
#include "engine/ui/cmd_god/do_date.h"
#include "engine/ui/cmd_god/do_dc.h"
#include "engine/ui/cmd_god/do_delete_obj.h"
#include "engine/ui/cmd_god/do_goto.h"
#include "engine/ui/cmd_god/do_set.h"
#include "engine/ui/cmd_god/do_invisible.h"
#include "engine/ui/cmd_god/do_echo.h"
#include "engine/ui/cmd_god/do_force.h"
#include "engine/ui/cmd_god/do_forcetime.h"
#include "engine/ui/cmd_god/do_gecho.h"
#include "engine/ui/cmd_god/do_glory.h"
#include "engine/ui/cmd_god/do_show.h"
#include "engine/ui/cmd_god/do_shutdown.h"
#include "engine/ui/cmd_god/do_reload.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "engine/ui/cmd_god/do_show.h"
#include "engine/ui/cmd_god/do_spellstat.h"
#include "engine/ui/cmd_god/do_liblist.h"
#include "engine/ui/cmd_god/do_last.h"
#include "engine/ui/cmd_god/do_load.h"
#include "engine/ui/cmd_god/do_loadstat.h"
#include "engine/ui/cmd_god/do_beep.h"
#include "engine/ui/cmd_god/do_overstuff.h"
#include "engine/ui/cmd_god/do_poof_msg.h"
#include "engine/ui/cmd_god/do_print_armor.h"
#include "engine/ui/cmd_god/do_purge.h"
#include "engine/ui/cmd_god/do_godtest.h"
#include "engine/ui/cmd_god/do_sdemigods.h"
#include "engine/ui/cmd_god/do_restore.h"
#include "engine/ui/cmd_god/do_sanitize.h"
#include "engine/ui/cmd_god/do_send.h"
#include "engine/ui/cmd_god/do_snoop.h"
#include "engine/ui/cmd_god/do_switch.h"
#include "engine/ui/cmd_god/do_tabulate.h"
#include "engine/ui/cmd_god/do_teleport.h"
#include "engine/ui/cmd_god/do_mark.h"
#include "engine/ui/cmd_god/do_unfreeze.h"
#include "engine/ui/cmd_god/do_vstat.h"
#include "engine/ui/cmd_god/do_wizlock.h"
#include "engine/ui/cmd_god/do_wiznet.h"
#include "engine/ui/cmd_god/do_wizutil.h"
#include "engine/ui/cmd_god/do_zclear.h"
#include "engine/ui/cmd_god/do_zreset.h"
#include "engine/ui/cmd/do_bandage.h"
#include "engine/ui/cmd/do_drink.h"
#include "engine/ui/cmd/do_drunkoff.h"
#include "engine/ui/cmd/do_consider.h"
#include "engine/ui/cmd/do_antigods.h"
#include "engine/ui/cmd/do_zone.h"
#include "engine/ui/cmd/do_hide.h"
#include "engine/ui/cmd/do_group.h"
#include "engine/ui/cmd/do_ungroup.h"
#include "engine/ui/cmd/do_quest.h"
#include "engine/ui/cmd/do_mystat.h"
#include "engine/ui/cmd/do_visible.h"
#include "engine/ui/cmd/do_statistic.h"
#include "engine/ui/cmd/do_pray_gods.h"
#include "engine/ui/cmd/do_mobshout.h"
#include "engine/ui/cmd/do_commands.h"
#include "engine/ui/cmd/do_gold.h"
#include "engine/ui/cmd/do_hidemove.h"
#include "engine/ui/cmd/do_generic_page.h"
#include "engine/ui/cmd/do_check_invoice.h"
#include "engine/ui/cmd/do_who_am_i.h"
#include "engine/ui/cmd/do_kill.h"
#include "engine/ui/cmd/do_move.h"
#include "engine/ui/cmd/do_diagnose.h"
#include "engine/ui/cmd/do_steal.h"
#include "engine/ui/cmd/do_camouflage.h"
#include "engine/ui/cmd/do_weather.h"
#include "engine/ui/cmd/do_offtop.h"
#include "engine/ui/cmd/do_gen_comm.h"
#include "engine/ui/cmd/do_spec_comm.h"
#include "engine/ui/cmd/do_rest.h"
#include "engine/ui/cmd/do_gen_door.h"
#include "engine/ui/cmd/do_sit.h"
#include "engine/ui/cmd/do_stand.h"
#include "engine/ui/cmd/do_style.h"
#include "engine/ui/cmd/do_tell.h"
#include "engine/ui/cmd/do_page.h"
#include "engine/ui/cmd/do_pour.h"
#include "engine/ui/cmd/do_reply.h"
#include "engine/ui/cmd/do_equip.h"
#include "engine/ui/cmd/do_sneak.h"
#include "engine/ui/cmd/do_quit.h"
#include "engine/ui/cmd/do_eat.h"
#include "engine/ui/cmd/do_enter.h"
#include "engine/ui/cmd/do_equipment.h"
#include "engine/ui/cmd/do_exits.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/cmd/do_hire.h"
#include "engine/ui/cmd/do_inventory.h"
#include "engine/ui/cmd/do_ignore.h"
#include "engine/ui/cmd/do_get.h"
#include "engine/ui/cmd/do_give.h"
#include "engine/ui/cmd/do_affects.h"
#include "engine/ui/cmd/do_display.h"
#include "engine/ui/cmd/do_mode.h"
#include "engine/ui/cmd/do_courage.h"
#include "engine/ui/cmd/do_save.h"
#include "engine/ui/cmd/do_save.h"
#include "engine/ui/cmd/do_wimpy.h"
#include "engine/ui/cmd/do_mercenary.h"
#include "engine/ui/cmd/do_levels.h"
#include "engine/ui/cmd/do_listen.h"
#include "engine/ui/cmd/do_look.h"
#include "engine/ui/cmd/do_pray.h"
#include "engine/ui/cmd/do_look_around.h"
#include "engine/ui/cmd/do_order.h"
#include "engine/ui/cmd/do_peer.h"
#include "engine/ui/cmd/do_put.h"
#include "engine/ui/cmd/do_recall.h"
#include "engine/ui/cmd/do_time.h"
#include "engine/ui/cmd/do_retreat.h"
#include "engine/ui/cmd/do_sleep.h"
#include "engine/ui/cmd/do_telegram.h"
#include "engine/ui/cmd/do_learn.h"
#include "engine/ui/cmd/do_features.h"
#include "engine/ui/cmd/do_skills.h"
#include "engine/ui/cmd/do_spells.h"
#include "engine/ui/cmd/do_forget.h"
#include "engine/ui/cmd/do_memorize.h"
#include "engine/ui/cmd/do_flee.h"
#include "engine/ui/cmd/do_create.h"
#include "engine/ui/cmd/do_mixture.h"
#include "engine/ui/cmd/do_cast.h"
#include "engine/ui/cmd/do_employ.h"
#include "engine/ui/cmd/do_remort.h"
#include "engine/ui/cmd/do_say.h"
#include "engine/ui/cmd/do_group_say.h"
#include "engine/ui/cmd/do_remove.h"
#include "engine/ui/cmd/do_refill.h"
#include "engine/ui/cmd/do_sign.h"
#include "engine/ui/cmd/do_write.h"
#include "engine/ui/cmd/do_trample.h"
#include "engine/ui/cmd/do_where.h"
#include "engine/ui/cmd/do_who.h"
#include "engine/ui/cmd/do_wake.h"
#include "engine/core/comm.h"
#include "gameplay/core/constants.h"
#include "gameplay/crafting/craft_commands.h"
#include "gameplay/crafting/fry.h"
#include "gameplay/crafting/jewelry.h"
#include "engine/db/db.h"
#include "gameplay/mechanics/depot.h"
#include "engine/scripting/dg_scripts.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/fight/assist.h"
#include "gameplay/ai/mobact.h"
#include "gameplay/fight/pk.h"
#include "engine/ui/cmd/do_kill.h"
#include "gameplay/core/genchar.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/glory_const.h"
#include "gameplay/mechanics/glory_misc.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/handler.h"
#include "engine/core/heartbeat_commands.h"
#include "gameplay/clans/house.h"
#include "gameplay/crafting/item_creation.h"
#include "utils/logger.h"
#include "gameplay/communication/mail.h"
#include "modify.h"
#include "gameplay/mechanics/named_stuff.h"
#include "administration/names.h"
#include "engine/entities/obj_data.h"
#include "engine/db/obj_prototypes.h"
#include "engine/olc/olc.h"
#include "gameplay/communication/parcel.h"
#include "administration/password.h"
#include "administration/privilege.h"
#include "engine/entities/room_data.h"
#include "engine/network/logon.h"
#include "engine/ui/color.h"
#include "gameplay/skills/armoring.h"
#include "gameplay/skills/skills.h"
#include "gameplay/skills/backstab.h"
#include "gameplay/skills/bash.h"
#include "gameplay/skills/shield_block.h"
#include "gameplay/skills/campfire.h"
#include "gameplay/skills/chopoff.h"
#include "gameplay/skills/disarm.h"
#include "gameplay/skills/deviate.h"
#include "gameplay/skills/fit.h"
#include "gameplay/skills/firstaid.h"
#include "gameplay/skills/intercept.h"
#include "gameplay/skills/ironwind.h"
#include "gameplay/skills/kick.h"
#include "gameplay/skills/lightwalk.h"
#include "gameplay/skills/manadrain.h"
#include "gameplay/skills/mighthit.h"
#include "gameplay/skills/multyparry.h"
#include "gameplay/skills/parry.h"
#include "gameplay/skills/poisoning.h"
#include "gameplay/skills/protect.h"
#include "gameplay/skills/repair.h"
#include "gameplay/skills/resque.h"
#include "gameplay/skills/sharpening.h"
#include "gameplay/skills/strangle.h"
#include "gameplay/skills/stun.h"
#include "gameplay/skills/overhelm.h"
#include "gameplay/skills/throw.h"
#include "gameplay/skills/throwout.h"
#include "gameplay/skills/track.h"
#include "gameplay/skills/turnundead.h"
#include "gameplay/skills/warcry.h"
#include "gameplay/skills/relocate.h"
#include "gameplay/skills/repair.h"
#include "gameplay/skills/skinning.h"
#include "gameplay/skills/spell_capable.h"
#include "gameplay/magic/spells.h"
#include "gameplay/mechanics/title.h"
#include "gameplay/statistics/top.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/skills/townportal.h"
#include "gameplay/mechanics/mem_queue.h"
#include "engine/db/obj_save.h"
#include "engine/core/iosystem.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/mechanics/birthplaces.h"
#include "engine/db/help.h"
#include "mapsystem.h"
#include "gameplay/economics/ext_money.h"
#include "gameplay/mechanics/noob.h"
#include "gameplay/core/reset_stats.h"
#include "gameplay/mechanics/obj_sets.h"
#include "utils/utils.h"
#include "gameplay/magic/magic_temp_spells.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"
#include "gameplay/mechanics/bonus.h"
#include "utils/utils_debug.h"
#include "engine/db/global_objects.h"
#include "administration/accounts.h"
#include "gameplay/fight/pk.h"
#include "gameplay/skills/slay.h"
#include "gameplay/skills/charge.h"
#include "gameplay/skills/dazzle.h"
#include "gameplay/mechanics/cities.h"
#include "administration/proxy.h"
#include "gameplay/communication/check_invoice.h"
#include "gameplay/mechanics/doors.h"
#include "gameplay/skills/frenzy.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/classes/recalc_mob_params_by_vnum.h"
#include "alias.h"
#include "engine/db/player_index.h"

#include <ctime>

#if defined WITH_SCRIPTING
#include "scripting.hpp"
#endif

#include <third_party_libs/fmt/include/fmt/format.h>

#include <memory>
#include <stdexcept>
#include <algorithm>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

extern RoomRnum r_frozen_start_room;
extern const char *religion_menu;
extern char *motd;
extern char *rules;
extern char *background;
extern const char *WELC_MESSG;
extern const char *START_MESSG;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern const char *default_race[];
extern struct PCCleanCriteria pclean_criteria[];
extern int rent_file_timeout;

extern char *greetings;
extern struct show_struct show_fields[];
extern char *name_rules;

void DeletePcByHimself(const char *name);

// external functions
void read_saved_vars(CharData *ch);
void oedit_parse(DescriptorData *d, char *arg);
void redit_parse(DescriptorData *d, char *arg);
void zedit_parse(DescriptorData *d, char *arg);
void medit_parse(DescriptorData *d, char *arg);
void trigedit_parse(DescriptorData *d, char *arg);
extern int CheckProxy(DescriptorData *ch);
extern void check_max_hp(CharData *ch);
// local functions
int perform_dupe_check(DescriptorData *d);
int reserved_word(const char *argument);
int _parse_name(char *argument, char *name);
int find_action(char *cmd);
int do_social(CharData *ch, char *argument);
void init_warcry(CharData *ch);

void do_alias(CharData *ch, char *argument, int cmd, int subcmd);
void do_ban(CharData *ch, char *argument, int cmd, int subcmd);
void DoExpedientCut(CharData *ch, char *argument, int, int);
void do_featset(CharData *ch, char *argument, int cmd, int subcmd);
void DoDrop(CharData *ch, char *argument, int, int);
void do_examine(CharData *ch, char *argument, int cmd, int subcmd);
void do_remember_char(CharData *ch, char *argument, int cmd, int subcmd);
void do_horseon(CharData *ch, char *argument, int cmd, int subcmd);
void do_horseoff(CharData *ch, char *argument, int cmd, int subcmd);
void do_horseput(CharData *ch, char *argument, int cmd, int subcmd);
void do_horseget(CharData *ch, char *argument, int cmd, int subcmd);
void do_horsetake(CharData *ch, char *argument, int cmd, int subcmd);
void do_givehorse(CharData *ch, char *argument, int cmd, int subcmd);
void DoStoreShop(CharData *ch, char *argument, int, int);
void do_not_here(CharData *ch, char *argument, int cmd, int subcmd);
void do_olc(CharData *ch, char *argument, int cmd, int subcmd);
void do_report(CharData *ch, char *argument, int cmd, int subcmd);
void do_stophorse(CharData *ch, char *argument, int cmd, int subcmd);
void DoScore(CharData *ch, char *argument, int, int);
void do_skillset(CharData *ch, char *argument, int cmd, int subcmd);
void DoSyslog(CharData *ch, char *argument, int, int subcmd);
void do_sense(CharData *ch, char *argument, int cmd, int subcmd);
void do_unban(CharData *ch, char *argument, int cmd, int subcmd);
void do_users(CharData *ch, char *argument, int cmd, int subcmd);
void do_transform_weapon(CharData *ch, char *argument, int cmd, int subcmd);
void do_dig(CharData *ch, char *argument, int cmd, int subcmd);
void do_insertgem(CharData *ch, char *argument, int cmd, int subcmd);
void do_proxy(CharData *ch, char *argument, int cmd, int subcmd);
void do_exchange(CharData *ch, char *argument, int cmd, int subcmd);
// DG Script ACMD's
void do_attach(CharData *ch, char *argument, int cmd, int subcmd);
void do_detach(CharData *ch, char *argument, int cmd, int subcmd);
void do_tlist(CharData *ch, char *argument, int cmd, int subcmd);
void do_tstat(CharData *ch, char *argument, int cmd, int subcmd);
void do_vdelete(CharData *ch, char *argument, int cmd, int subcmd);
void do_identify(CharData *ch, char *argument, int cmd, int subcmd);
void do_rset(CharData *ch, char *argument, int cmd, int subcmd);
void do_recipes(CharData *ch, char *argument, int cmd, int subcmd);
void do_cook(CharData *ch, char *argument, int cmd, int subcmd);
void do_forgive(CharData *ch, char *argument, int cmd, int subcmd);
void DoTownportal(CharData *ch, char *argument, int, int);
void do_dmeter(CharData *ch, char *argument, int cmd, int subcmd);
void DoShowZoneStat(CharData *ch, char *argument, int, int);
void do_show_mobmax(CharData *ch, char *, int, int);

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List).
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

	mudlog(ss.str().c_str(), DEF, kLvlGod, ERRLOG, true);
}

cpp_extern const struct command_info cmd_info[] =
	{
		{"RESERVED", EPosition::kDead, nullptr, 0, 0, 0},    // this must be first -- for specprocs

		// directions must come before other commands but after RESERVED
		{"север", EPosition::kStand, DoMove, 0, EDirection::kNorth, -2},
		{"восток", EPosition::kStand, DoMove, 0, EDirection::kEast, -2},
		{"юг", EPosition::kStand, DoMove, 0, EDirection::kSouth, -2},
		{"запад", EPosition::kStand, DoMove, 0, EDirection::kWest, -2},
		{"вверх", EPosition::kStand, DoMove, 0, EDirection::kUp, -2},
		{"вниз", EPosition::kStand, DoMove, 0, EDirection::kDown, -2},
		{"north", EPosition::kStand, DoMove, 0, EDirection::kNorth, -2},
		{"east", EPosition::kStand, DoMove, 0, EDirection::kEast, -2},
		{"south", EPosition::kStand, DoMove, 0, EDirection::kSouth, -2},
		{"west", EPosition::kStand, DoMove, 0, EDirection::kWest, -2},
		{"up", EPosition::kStand, DoMove, 0, EDirection::kUp, -2},
		{"down", EPosition::kStand, DoMove, 0, EDirection::kDown, -2},

		{"аффекты", EPosition::kDead, do_affects, 0, kScmdAuction, 0},
		{"авторы", EPosition::kDead, DoGenericPage, 0, kScmdCredits, 0},
		{"атаковать", EPosition::kFight, DoHit, 0, kScmdMurder, -1},
		{"аукцион", EPosition::kRest, do_gen_comm, 0, kScmdAuction, 100},
		{"анонсы", EPosition::kDead, Boards::DoBoard, 1, Boards::NOTICE_BOARD, -1},

		{"базар", EPosition::kRest, do_exchange, 1, 0, -1},
		{"баланс", EPosition::kStand, do_not_here, 1, 0, 0},
		{"баги", EPosition::kDead, Boards::DoBoard, 1, Boards::ERROR_BOARD, 0},
		{"бежать", EPosition::kFight, DoFlee, 1, 0, -1},
		{"бинтовать", EPosition::kRest, DoBandage, 0, 0, 0},
		{"билдер", EPosition::kDead, Boards::DoBoard, 1, Boards::GODBUILD_BOARD, -1},
		{"блок", EPosition::kFight, do_block, 0, 0, -1},
		{"блокнот", EPosition::kDead, Boards::DoBoard, 1, Boards::PERS_BOARD, -1},
		{"боги", EPosition::kDead, DoGenericPage, 0, kScmdImmlist, 0},
		{"божества", EPosition::kDead, Boards::DoBoard, 1, Boards::GODGENERAL_BOARD, -1},
		{"болтать", EPosition::kRest, do_gen_comm, 0, kScmdGossip, -1},
		{"бонус", EPosition::kDead, Bonus::do_bonus_by_character, kLvlImplementator, 0, 0},
		{"бонусинфо", EPosition::kDead, Bonus::do_bonus_info, kLvlImplementator, 0, 0},
		{"бросить", EPosition::kRest, DoDrop, 0, 0, -1},
		{"варить", EPosition::kRest, do_cook, 0, 0, 200},
		{"версия", EPosition::kDead, DoGenericPage, 0, kScmdVersion, 0},
		{"вече", EPosition::kDead, Boards::DoBoard, 1, Boards::GENERAL_BOARD, -1},
		{"взять", EPosition::kRest, do_get, 0, 0, 200},
		{"взглянуть", EPosition::kRest, do_diagnose, 0, 0, 100},
		{"взломать", EPosition::kStand, do_gen_door, 1, EDoorScmd::kScmdPick, -1},
		{"вихрь", EPosition::kFight, do_iron_wind, 0, 0, -1},
		{"вложить", EPosition::kStand, do_not_here, 1, 0, -1},
		{"вернуть", EPosition::kStand, do_not_here, 0, 0, -1},
		{"вернуться", EPosition::kDead, DoReturn, 0, 0, -1},
		{"войти", EPosition::kStand, DoEnter, 0, 0, -2},
		{"война", EPosition::kRest, ClanSystem::DoShowWars, 0, 0, 0},
		{"вооружиться", EPosition::kRest, do_wield, 0, 0, 200},
		{"возврат", EPosition::kRest, do_recall, 0, 0, -1},
		{"воззвать", EPosition::kDead, do_pray_gods, 0, 0, -1},
		{"вплавить", EPosition::kStand, do_insertgem, 0, 0, -1},
		{"время", EPosition::kDead, do_time, 0, 0, 0},
		{"врата", EPosition::kSit, DoTownportal, 1, 0, -1},
		{"вскочить", EPosition::kFight, do_horseon, 0, 0, 500},
		{"встать", EPosition::kRest, do_stand, 0, 0, 500},
		{"вспомнить", EPosition::kDead, do_remember_char, 0, 0, 0},
		{"выбросить", EPosition::kRest, DoDrop, 0, 0 /*SCMD_DONATE */ , 300},
		{"выследить", EPosition::kStand, do_track, 0, 0, 500},
		{"вылить", EPosition::kStand, do_pour, 0, kScmdPour, 500},
		{"выходы", EPosition::kRest, DoExits, 0, 0, 0},
		{"вышвырнуть", EPosition::kFight, DoThrowout, 0, 0, 0},

		{"говорить", EPosition::kRest, do_say, 0, 0, -1},
		{"ггруппа", EPosition::kSleep, do_gsay, 0, 0, 500},
		{"гговорить", EPosition::kSleep, do_gsay, 0, 0, 500},
		{"гдругам", EPosition::kSleep, ClanSystem::DoClanChannel, 0, kScmdChannel, 0},
		{"где", EPosition::kRest, DoWhere, kLvlImmortal, 0, 0},
		{"гдея", EPosition::kRest, DoZone, 0, 0, 0},
		{"глоток", EPosition::kRest, DoDrink, 0, kScmdSip, 200},
		{"города", EPosition::kDead, cities::DoCities, 0, 0, 0},
		{"группа", EPosition::kSleep, do_group, 1, 0, -1},
		{"гсоюзникам", EPosition::kSleep, ClanSystem::DoClanChannel, 0, kScmdAchannel, 0},
		{"гэхо", EPosition::kDead, DoGlobalEcho, kLvlGod, 0, 0},
		{"гбогам", EPosition::kDead, do_wiznet, kLvlImmortal, 0, 0},

		{"дать", EPosition::kRest, do_give, 0, 0, 500},
		{"дата", EPosition::kDead, DoPageDateTime, 0, kScmdDate, 0},
		{"делить", EPosition::kRest, group::do_split, 1, 0, 200},
		{"держать", EPosition::kRest, do_grab, 0, 0, 300},
		{"дметр", EPosition::kDead, do_dmeter, 0, 0, 0},
		{"доложить", EPosition::kRest, do_report, 0, 0, 500},
		{"доски", EPosition::kDead, Boards::DoBoardList, 0, 0, 0},
		{"дружины", EPosition::kDead, ClanSystem::DoClanList, 0, 0, 0},
		{"дрновости", EPosition::kDead, Boards::DoBoard, 1, Boards::CLANNEWS_BOARD, -1},
		{"дрвече", EPosition::kDead, Boards::DoBoard, 1, Boards::CLAN_BOARD, -1},
		{"дрлист", EPosition::kDead, ClanSystem::DoClanPkList, 0, 1, 0},
		{"есть", EPosition::kRest, do_eat, 0, kScmdEat, 500},

		{"жертвовать", EPosition::kStand, do_pray, 1, SCMD_DONATE, -1},

		{"заколоть", EPosition::kStand, DoBackstab, 1, 0, 1},
		{"забыть", EPosition::kRest, do_forget, 0, 0, 0},
		{"задержать", EPosition::kStand, do_not_here, 1, 0, -1},
		{"заклинания", EPosition::kSleep, DoSpells, 0, 0, 0},
		{"заклстат", EPosition::kDead, DoPageSpellStat, kLvlGreatGod, 0, 0},
		{"закрыть", EPosition::kSit, do_gen_door, 0, kScmdClose, 500},
		{"замакс", EPosition::kRest, do_show_mobmax, 1, 0, -1},
		{"замести", EPosition::kStand, do_hidetrack, 1, 0, -1},
		{"замолчать", EPosition::kDead, DoWizutil, kLvlGod, kScmdMute, 0},
		{"заморозить", EPosition::kDead, DoWizutil, kLvlFreeze, kScmdFreeze, 0},
		{"занятость", EPosition::kDead, DoCheckZoneOccupation, kLvlGod, 0, 0},
		{"запомнить", EPosition::kRest, do_memorize, 0, 0, 0},
		{"запереть", EPosition::kSit, do_gen_door, 0, kScmdLock, 500},
		{"запрет", EPosition::kDead, do_ban, kLvlGreatGod, 0, 0},
		{"заснуть", EPosition::kSleep, do_sleep, 0, 0, -1},
		{"заставка", EPosition::kDead, DoGenericPage, 0, kScmdMotd, 0},
		{"заставить", EPosition::kSleep, do_force, kLvlGreatGod, 0, 0},
		{"затоптать", EPosition::kStand, DoTrample, 0, 0, 0},
		{"заточить", EPosition::kRest, DoSharpening, 0, 0, 500},
		{"заучить", EPosition::kRest, do_memorize, 0, 0, 0},
		{"зачитать", EPosition::kRest, do_employ, 0, SCMD_RECITE, 500},
		{"зачаровать", EPosition::kStand, DoSpellCapable, 1, 0, 0},
		{"зачистить", EPosition::kDead, DoSanitize, kLvlGreatGod, 0, 0},
		{"золото", EPosition::kRest, do_gold, 0, 0, 0},
		{"зона", EPosition::kRest, DoZone, 0, 0, 0},
		{"зоныстат", EPosition::kDead, DoShowZoneStat, kLvlImmortal, 0, 0},
		{"инвентарь", EPosition::kSleep, DoInventory, 0, 0, 0},
		{"игнорировать", EPosition::kDead, do_ignore, 0, 0, 0},
		{"идеи", EPosition::kDead, Boards::DoBoard, 1, Boards::IDEA_BOARD, 0},
		{"изгнать нежить", EPosition::kRest, do_turn_undead, 0, 0, -1},
		{"изучить", EPosition::kSit, DoLearn, 0, 0, 0},
		{"информация", EPosition::kSleep, DoGenericPage, 0, kScmdInfo, 0},
		{"испить", EPosition::kRest, do_employ, 0, SCMD_QUAFF, 500},
		{"использовать", EPosition::kRest, DoStyle, 0, 0, 0},
		{"исступление", EPosition::kFight, do_frenzy, 0, 0, -1},
		{"имя", EPosition::kSleep, do_name, kLvlImmortal, 0, 0},

		{"колдовать", EPosition::kSit, DoCast, 1, 0, -1},
		{"казна", EPosition::kRest, do_not_here, 1, 0, 0},
		{"карта", EPosition::kRest, do_map, 0, 0, 0},
		{"клан", EPosition::kRest, ClanSystem::DoHouse, 0, 0, 0},
		{"клич", EPosition::kFight, do_warcry, 1, 0, -1},
		{"кодер", EPosition::kDead, Boards::DoBoard, 1, Boards::CODER_BOARD, -1},
		{"команды", EPosition::kDead, do_commands, 0, kScmdCommands, 0},
		{"коне", EPosition::kSleep, do_quit, 0, 0, 0},
		{"конец", EPosition::kSleep, do_quit, 0, kScmdQuit, 0},
		{"копать", EPosition::kStand, do_dig, 0, 0, -1},
		{"красться", EPosition::kStand, DoHidemove, 1, 0, -2},
		{"копироватьзону", EPosition::kStand, dungeons::DoZoneCopy, kLvlImplementator, 0, 0},
		{"кричать", EPosition::kRest, do_gen_comm, 0, kScmdShout, -1},
		{"кто", EPosition::kRest, DoWho, 0, 0, 0},
		{"ктодружина", EPosition::kRest, ClanSystem::DoWhoClan, 0, 0, 0},
		{"ктоя", EPosition::kDead, DoWhoAmI, 0, 0, 0},
		{"купить", EPosition::kStand, do_not_here, 0, 0, -1},

		{"леваярука", EPosition::kRest, do_grab, 1, 0, 300},
		{"лечить", EPosition::kStand, DoFirstaid, 0, 0, -1},
		{"лить", EPosition::kStand, do_pour, 0, kScmdPour, 500},
		{"лошадь", EPosition::kStand, do_not_here, 1, 0, -1},
		{"лучшие", EPosition::kDead, Rating::DoBest, 0, 0, 0},

		{"маскировка", EPosition::kRest, do_camouflage, 0, 0, 500},
		{"магазины", EPosition::kDead, DoStoreShop, kLvlImmortal, 0, 0},
		{"метнуть", EPosition::kFight, DoThrow, 0, kScmdPhysicalThrow, -1},
		{"менять", EPosition::kStand, do_not_here, 0, 0, -1},
		{"месть", EPosition::kRest, do_revenge, 0, 0, 0},
		{"молот", EPosition::kFight, DoMighthit, 0, 0, -1},
		{"молиться", EPosition::kStand, do_pray, 1, SCMD_PRAY, -1},
		{"моястатистика", EPosition::kDead, do_mystat, 0, 0, 0},
		{"мысл", EPosition::kDead, do_quit, 0, 0, 0},
		{"мысль", EPosition::kDead, Boards::report_on_board, 0, Boards::SUGGEST_BOARD, 0},

		{"наемник", EPosition::kStand, do_not_here, 1, 0, -1},
		{"наказания", EPosition::kDead, Boards::DoBoard, 1, Boards::GODPUNISH_BOARD, -1},
		{"налить", EPosition::kStand, do_pour, 0, kScmdFill, 500},
		{"наполнить", EPosition::kStand, do_pour, 0, kScmdFill, 500},
		{"натиск", EPosition::kStand, DoCharge, 0, 0, 0},
		{"найти", EPosition::kStand, do_sense, 0, 0, 500},
		{"нанять", EPosition::kStand, DoFindhelpee, 0, 0, -1},
		{"новичок", EPosition::kSleep, DoGenericPage, 0, kScmdInfo, 0},
		{"новости", EPosition::kDead, Boards::DoBoard, 1, Boards::NEWS_BOARD, -1},
		{"надеть", EPosition::kRest, do_wear, 0, 0, 500},
		{"нацарапать", EPosition::kRest, DoSign, 0, 0, 0},

		{"обезоружить", EPosition::kFight, do_disarm, 0, 0, -1},
		{"облачить", EPosition::kRest, do_wear, 0, 0, 500},
		{"обмен", EPosition::kStand, do_not_here, 0, 0, 0},
		{"обменять", EPosition::kStand, do_not_here, 0, 0, 0},
		{"оглядеться", EPosition::kRest, DoLookAround, 0, 0, 0},
		{"оглушить", EPosition::kFight, DoOverhelm, 0, 0, -1},
		{"одеть", EPosition::kRest, do_wear, 0, 0, 500},
		{"опознать", EPosition::kRest, do_identify, 0, 0, 500},
		{"опохмелиться", EPosition::kRest, do_drunkoff, 0, 0, -1},
		{"опечатк", EPosition::kDead, do_quit, 0, 0, 0},
		{"опечатка", EPosition::kDead, Boards::report_on_board, 0, Boards::MISPRINT_BOARD, 0},
		{"опустить", EPosition::kRest, do_put, 0, 0, 500},
		{"орать", EPosition::kRest, do_gen_comm, 1, kScmdHoller, -1},
		{"осмотреть", EPosition::kRest, do_examine, 0, 0, 0},
		{"оседлать", EPosition::kStand, do_horsetake, 1, 0, -1},
		{"оскорбить", EPosition::kRest, do_insult, 0, 0, -1},
		{"ослепить", EPosition::kFight, DoDazzle, 1, 0, -1},
		{"осушить", EPosition::kRest, do_employ, 0, SCMD_QUAFF, 300},
		{"освежевать", EPosition::kStand, DoSkinning, 0, 0, -1},
		{"ответить", EPosition::kRest, do_reply, 0, 0, -1},
		{"отразить", EPosition::kFight, do_multyparry, 0, 0, -1},
		{"отвязать", EPosition::kDead, do_horseget, 0, 0, -1},
		{"отдохнуть", EPosition::kRest, do_rest, 0, 0, -1},
		{"открыть", EPosition::kSit, do_gen_door, 0, EDoorScmd::kScmdOpen, 500},
		{"отпереть", EPosition::kSit, do_gen_door, 0, kScmdUnlock, 500},
		{"отпустить", EPosition::kSit, do_stophorse, 0, 0, -1},
		{"отравить", EPosition::kFight, DoPoisoning, 0, 0, -1},
		{"отринуть", EPosition::kRest, do_antigods, 1, 0, -1},
		{"отступить", EPosition::kFight, do_retreat, 1, 0, -1},
		{"отправить", EPosition::kStand, do_not_here, 1, 0, -1},
		{"оффтоп", EPosition::kDead, do_offtop, 0, 0, -1},
		{"ошеломить", EPosition::kStand, do_stun, 1, 0, -1},
		{"оценить", EPosition::kStand, do_not_here, 0, 0, 500},
		{"очки", EPosition::kDead, DoScore, 0, 0, 0},
		{"очепятки", EPosition::kDead, Boards::DoBoard, 1, Boards::MISPRINT_BOARD, 0},
		{"очистить", EPosition::kDead, do_not_here, 0, kScmdClear, -1},
		{"ошибк", EPosition::kDead, do_quit, 0, 0, 0},
		{"ошибка", EPosition::kDead, Boards::report_on_board, 0, Boards::ERROR_BOARD, 0},

		{"парировать", EPosition::kFight, DoParry, 0, 0, -1},
		{"перехватить", EPosition::kFight, DoIntercept, 0, 0, -1},
		{"перековать", EPosition::kStand, do_transform_weapon, 0, SCMD_TRANSFORMWEAPON, -1},
		{"передать", EPosition::kStand, do_givehorse, 0, 0, -1},
		{"перевести", EPosition::kStand, do_not_here, 1, 0, -1},
		{"переместиться", EPosition::kStand, do_relocate, 1, 0, 0},
		{"перевоплотитьс", EPosition::kStand, DoRemort, 0, 0, -1},
		{"перевоплотиться", EPosition::kStand, DoRemort, 0, 1, -1},
		{"перелить", EPosition::kStand, do_pour, 0, kScmdPour, 500},
		{"пересчитатьзону", EPosition::kDead, do_recalc_zone, kLvlImmortal, 0, 0},
		{"перешить", EPosition::kRest, DoFit, 0, kScmdMakeOver, 500},
		{"пить", EPosition::kRest, DoDrink, 0, kScmdDrink, 400},
		{"писать", EPosition::kStand, do_write, 1, 0, -1},
		{"пклист", EPosition::kSleep, ClanSystem::DoClanPkList, 0, 0, 0},
		{"пнуть", EPosition::kFight, do_kick, 1, 0, -1},
		{"погода", EPosition::kRest, do_weather, 0, 0, 0},
		{"подкрасться", EPosition::kStand, do_sneak, 1, 0, 100},
		{"подножка", EPosition::kFight, do_chopoff, 0, 0, 500},
		{"подняться", EPosition::kRest, do_stand, 0, 0, -1},
		{"поджарить", EPosition::kRest, do_fry, 0, 0, -1},
		{"перевязать", EPosition::kRest, DoBandage, 0, 0, 0},
		{"переделать", EPosition::kRest, DoFit, 0, kScmdDoAdapt, 500},
		{"подсмотреть", EPosition::kRest, DoLook, 0, kScmdLookHide, 0},
		{"положить", EPosition::kRest, do_put, 0, 0, 400},
		{"получить", EPosition::kStand, do_not_here, 1, 0, -1},
		{"политика", EPosition::kSleep, ClanSystem::DoShowPolitics, 0, 0, 0},
		{"помочь", EPosition::kFight, do_assist, 1, 0, -1},
		{"помощь", EPosition::kDead, do_help, 0, 0, 0},
		{"пометить", EPosition::kDead, do_mark, kLvlImplementator, 0, 0},
		{"порез", EPosition::kFight, DoExpedientCut, 0, 0, -1},
		{"поселиться", EPosition::kStand, do_not_here, 1, 0, -1},
		{"постой", EPosition::kStand, do_not_here, 1, 0, -1},
		{"почта", EPosition::kStand, do_not_here, 1, 0, -1},
		{"пополнить", EPosition::kStand, do_refill, 0, 0, 300},
		{"поручения", EPosition::kRest, do_quest, 1, 0, -1},
		{"появиться", EPosition::kRest, do_visible, 1, 0, -1},
		{"правила", EPosition::kDead, DoGenericPage, 0, kScmdPolicies, 0},
		{"предложение", EPosition::kStand, do_not_here, 1, 0, 500},
		//{"призвать", EPosition::kStand, do_summon, 1, 0, -1},
		{"приказ", EPosition::kRest, do_order, 1, 0, -1},
		{"привязать", EPosition::kRest, do_horseput, 0, 0, 500},
		{"приглядеться", EPosition::kRest, DoPeer, 0, 0, 250},
		{"прикрыть", EPosition::kFight, do_protect, 0, 0, -1},
		{"применить", EPosition::kSit, do_employ, 1, SCMD_USE, 400},
		//{"прикоснуться", EPosition::kStand, do_touch_stigma, 0, 0, -1},
		{"присесть", EPosition::kRest, do_sit, 0, 0, -1},
		{"прислушаться", EPosition::kRest, DoListen, 0, 0, 300},
		{"присмотреться", EPosition::kRest, DoPeer, 0, 0, 250},
		{"придумки", EPosition::kDead, Boards::DoBoard, 0, Boards::SUGGEST_BOARD, 0},
		{"проверить", EPosition::kDead, DoCheckInvoice, 0, 0, 0},
		{"проснуться", EPosition::kSleep, do_wake, 0, kScmdWake, -1},
		{"простить", EPosition::kRest, do_forgive, 0, 0, 0},
		{"пробовать", EPosition::kRest, do_eat, 0, kScmdTaste, 300},
		{"сожрать", EPosition::kRest, do_eat, 0, kScmdDevour, 300},
		{"продать", EPosition::kStand, do_not_here, 0, 0, -1},
		{"фильтровать", EPosition::kStand, do_not_here, 0, 0, -1},
		{"прыжок", EPosition::kSleep, DoGoto, kLvlGod, 0, 0},

		{"разбудить", EPosition::kRest, do_wake, 0, kScmdWakeUp, -1},
		{"разгруппировать", EPosition::kDead, do_ungroup, 0, 0, 500},
		{"разделить", EPosition::kRest, group::do_split, 1, 0, 500},
		{"разделы", EPosition::kRest, do_help, 1, 0, 500},
		{"разжечь", EPosition::kStand, DoCampfire, 0, 0, -1},
		{"распустить", EPosition::kDead, do_ungroup, 0, 0, 500},
		{"рассмотреть", EPosition::kStand, do_not_here, 0, 0, -1},
		{"рассчитать", EPosition::kRest, DoFreehelpee, 0, 0, -1},
		{"режим", EPosition::kDead, DoMode, 0, 0, 0},
		{"ремонт", EPosition::kRest, DoRepair, 0, 0, -1},
		{"рецепты", EPosition::kRest, do_recipes, 0, 0, 0},
		{"рекорды", EPosition::kDead, Rating::DoBest, 0, 0, 0},
		{"руны", EPosition::kFight, do_mixture, 0, SCMD_RUNES, -1},

		{"сбить", EPosition::kFight, do_bash, 1, 0, -1},
		{"свойства", EPosition::kStand, do_not_here, 0, 0, -1},
		{"сгруппа", EPosition::kSleep, do_gsay, 0, 0, -1},
		{"сглазить", EPosition::kFight, do_manadrain, 0, 0, -1},
		{"сесть", EPosition::kRest, do_sit, 0, 0, -1},
		{"синоним", EPosition::kDead, do_alias, 0, 0, 0},
		{"сдемигодам", EPosition::kDead, DoSendMsgToDemigods, kLvlImmortal, 0, 0},
		{"сказать", EPosition::kRest, do_tell, 0, 0, -1},
		{"скользить", EPosition::kStand, DoLightwalk, 0, 0, 0},
		{"следовать", EPosition::kRest, do_follow, 0, 0, 500},
		{"сложить", EPosition::kFight, do_mixture, 0, SCMD_RUNES, -1},
		{"слава", EPosition::kStand, Glory::do_spend_glory, 0, 0, 0},
		{"слава2", EPosition::kStand, GloryConst::do_spend_glory, 0, 0, 0},
		{"смотреть", EPosition::kRest, DoLook, 0, kScmdLook, 0},
		{"смешать", EPosition::kStand, do_mixture, 0, SCMD_ITEMS, -1},
//  { "смастерить",     EPosition::kStand, do_transform_weapon, 0, SCMD_CREATEBOW, -1 },
		{"снять", EPosition::kRest, do_remove, 0, 0, 500},
		{"создать", EPosition::kSit, do_create, 0, 0, -1},
		{"сон", EPosition::kSleep, do_sleep, 0, 0, -1},
		{"соскочить", EPosition::kFight, do_horseoff, 0, 0, -1},
		{"состав", EPosition::kRest, do_create, 0, SCMD_RECIPE, 0},
		{"сохранить", EPosition::kSleep, do_save, 0, 0, 0},
		{"союзы", EPosition::kRest, ClanSystem::do_show_alliance, 0, 0, 0},
		{"социалы", EPosition::kDead, do_commands, 0, kScmdSocials, 0},
		{"спать", EPosition::kSleep, do_sleep, 0, 0, -1},
		{"спасти", EPosition::kFight, do_rescue, 1, 0, -1},
		{"способности", EPosition::kSleep, DoFeatures, 0, 0, 0},
		{"список", EPosition::kStand, do_not_here, 0, 0, -1},
		{"справка", EPosition::kDead, do_help, 0, 0, 0},
		{"спросить", EPosition::kRest, do_spec_comm, 0, kScmdAsk, -1},
		{"сбросить", EPosition::kRest, dungeons::DoDungeonReset, kLvlImplementator, 0, -1},
		{"спрятаться", EPosition::kStand, do_hide, 1, 0, 500},
		{"сравнить", EPosition::kRest, DoConsider, 0, 0, 500},
		{"сразить", EPosition::kFight, do_slay, 1, 0, -1},
		{"ставка", EPosition::kStand, do_not_here, 0, 0, -1},
		{"статус", EPosition::kDead, do_display, 0, 0, 0},
		{"статистика", EPosition::kDead, do_statistic, 0, 0, 0},
		{"стереть", EPosition::kDead, DoGenericPage, 0, kScmdClear, 0},
		{"стиль", EPosition::kRest, DoStyle, 0, 0, 0},
		{"строка", EPosition::kDead, do_display, 0, 0, 0},
		{"счет", EPosition::kDead, DoScore, 0, 0, 0},
		{"телега", EPosition::kDead, do_telegram, 0, 0, -1},
		{"тень", EPosition::kFight, DoThrow, 0, kScmdShadowThrow, -1},
		{"титул", EPosition::kDead, TitleSystem::do_title, 0, 0, 0},
		{"трусость", EPosition::kDead, do_wimpy, 0, 0, 0},
		{"убить", EPosition::kFight, DoKill, 0, 0, -1},
		{"убрать", EPosition::kRest, do_remove, 0, 0, 400},
		{"ударить", EPosition::kFight, DoHit, 0, kScmdHit, -1},
		{"удавить", EPosition::kFight, do_strangle, 0, 0, -1},
		{"удалитьпредмет", EPosition::kStand, DoDeleteObj, kLvlImplementator, 0, 0},
		{"уклониться", EPosition::kFight, DoDeviate, 1, 0, -1},
		{"украсть", EPosition::kStand, do_steal, 1, 0, 0},
		{"укрепить", EPosition::kRest, DoArmoring, 0, 0, -1},
		{"умения", EPosition::kSleep, DoSkills, 0, 0, 0},
		{"уровень", EPosition::kDead, DoScore, 0, 0, 0},
		{"уровни", EPosition::kDead, do_levels, 0, 0, 0},
		{"учить", EPosition::kStand, do_not_here, 0, 0, -1},
		{"хранилище", EPosition::kDead, ClanSystem::DoStoreHouse, 0, 0, 0},
		{"характеристики", EPosition::kStand, do_not_here, 0, 0, -1},
		{"кланстаф", EPosition::kStand, ClanSystem::do_clanstuff, 0, 0, 0},
		{"чинить", EPosition::kStand, do_not_here, 0, 0, -1},
		{"читать", EPosition::kRest, DoLook, 0, kScmdRead, 200},
		{"шептать", EPosition::kRest, do_spec_comm, 0, kScmdWhisper, -1},
		{"экипировка", EPosition::kSleep, DoEquipment, 0, 0, 0},
		{"эмоция", EPosition::kRest, do_echo, 1, kScmdEmote, -1},
		{"эхо", EPosition::kSleep, do_echo, kLvlImmortal, kScmdEcho, -1},
		{"ярость", EPosition::kRest, do_courage, 0, 0, -1},

		// God commands for listing
		{"мсписок", EPosition::kDead, do_liblist, 0, kScmdMlist, 0},
		{"осписок", EPosition::kDead, do_liblist, 0, kScmdOlist, 0},
		{"ксписок", EPosition::kDead, do_liblist, 0, kScmdRlist, 0},
		{"зсписок", EPosition::kDead, do_liblist, 0, kScmdZlist, 0},

		{"'", EPosition::kRest, do_say, 0, 0, -1},
		{":", EPosition::kRest, do_echo, 1, kScmdEmote, -1},
		{";", EPosition::kDead, do_wiznet, kLvlImmortal, 0, -1},
		{"advance", EPosition::kDead, DoAdvance, kLvlImplementator, 0, 0},
		{"alias", EPosition::kDead, do_alias, 0, 0, 0},
		{"alter", EPosition::kRest, DoFit, 0, kScmdMakeOver, 500},
		{"ask", EPosition::kRest, do_spec_comm, 0, kScmdAsk, -1},
		{"assist", EPosition::kFight, do_assist, 1, 0, -1},
		{"attack", EPosition::kFight, DoHit, 0, kScmdMurder, -1},
		{"auction", EPosition::kRest, do_gen_comm, 0, kScmdAuction, -1},
		{"arenarestore", EPosition::kSleep, DoArenaRestore, kLvlGod, 0, 0},
		{"backstab", EPosition::kStand, DoBackstab, 1, 0, 1},
		{"balance", EPosition::kStand, do_not_here, 1, 0, -1},
		{"ban", EPosition::kDead, do_ban, kLvlGreatGod, 0, 0},
		{"bash", EPosition::kFight, do_bash, 1, 0, -1},
		{"beep", EPosition::kDead, do_beep, kLvlImmortal, 0, 0},
		{"block", EPosition::kFight, do_block, 0, 0, -1},
		{"bug", EPosition::kDead, Boards::report_on_board, 0, Boards::ERROR_BOARD, 0},
		{"buy", EPosition::kStand, do_not_here, 0, 0, -1},
		{"best", EPosition::kDead, Rating::DoBest, 0, 0, 0},
		{"cast", EPosition::kSit, DoCast, 1, 0, -1},
		{"charge", EPosition::kStand, DoCharge, 0, 0, 0},
		{"check", EPosition::kStand, do_not_here, 1, 0, -1},
		{"chopoff", EPosition::kFight, do_chopoff, 0, 0, 500},
		{"clear", EPosition::kDead, DoGenericPage, 0, kScmdClear, 0},
		{"close", EPosition::kSit, do_gen_door, 0, kScmdClose, 500},
		{"cls", EPosition::kDead, DoGenericPage, 0, kScmdClear, 0},
		{"commands", EPosition::kDead, do_commands, 0, kScmdCommands, 0},
		{"consider", EPosition::kRest, DoConsider, 0, 0, 500},
		{"credits", EPosition::kDead, DoGenericPage, 0, kScmdCredits, 0},
		{"date", EPosition::kDead, DoPageDateTime, kLvlImmortal, kScmdDate, 0},
		{"dazzle", EPosition::kFight, DoDazzle, 1, 0, -1},
		{"dc", EPosition::kDead, DoDropConnect, kLvlGreatGod, 0, 0},
		{"deposit", EPosition::kStand, do_not_here, 1, 0, 500},
		{"deviate", EPosition::kFight, DoDeviate, 0, 0, -1},
		{"diagnose", EPosition::kRest, do_diagnose, 0, 0, 500},
		{"dig", EPosition::kStand, do_dig, 0, 0, -1},
		{"disarm", EPosition::kFight, do_disarm, 0, 0, -1},
		{"display", EPosition::kDead, do_display, 0, 0, 0},
		{"drink", EPosition::kRest, DoDrink, 0, kScmdDrink, 500},
		{"drop", EPosition::kRest, DoDrop, 0, 0, 500},
		{"dumb", EPosition::kDead, DoWizutil, kLvlImmortal, kScmdDumb, 0},
		{"eat", EPosition::kRest, do_eat, 0, kScmdEat, 500},
		{"devour", EPosition::kRest, do_eat, 0, kScmdDevour, 300},
		{"echo", EPosition::kSleep, do_echo, kLvlImmortal, kScmdEcho, 0},
		{"emote", EPosition::kRest, do_echo, 1, kScmdEmote, -1},
		{"enter", EPosition::kStand, DoEnter, 0, 0, -2},
		{"equipment", EPosition::kSleep, DoEquipment, 0, 0, 0},
		{"examine", EPosition::kRest, do_examine, 0, 0, 500},
		{"exchange", EPosition::kRest, do_exchange, 1, 0, -1},
		{"exits", EPosition::kRest, DoExits, 0, 0, 500},
		{"featset", EPosition::kSleep, do_featset, kLvlImplementator, 0, 0},
		{"features", EPosition::kSleep, DoFeatures, 0, 0, 0},
		{"fill", EPosition::kStand, do_pour, 0, kScmdFill, 500},
		{"fit", EPosition::kRest, DoFit, 0, kScmdDoAdapt, 500},
		{"flee", EPosition::kFight, DoFlee, 1, 0, -1},
		{"follow", EPosition::kRest, do_follow, 0, 0, -1},
		{"force", EPosition::kSleep, do_force, kLvlGreatGod, 0, 0},
		{"forcetime", EPosition::kDead, DoForcetime, kLvlImplementator, 0, 0},
		{"freeze", EPosition::kDead, DoWizutil, kLvlFreeze, kScmdFreeze, 0},
		{"frenzy", EPosition::kFight, do_frenzy, 0, 0, -1},
		{"gecho", EPosition::kDead, DoGlobalEcho, kLvlGod, 0, 0},
		{"get", EPosition::kRest, do_get, 0, 0, 500},
		{"give", EPosition::kRest, do_give, 0, 0, 500},
		{"godnews", EPosition::kDead, Boards::DoBoard, 1, Boards::GODNEWS_BOARD, -1},
		{"gold", EPosition::kRest, do_gold, 0, 0, 0},
		{"glide", EPosition::kStand, DoLightwalk, 0, 0, 0},
		{"glory", EPosition::kRest, GloryConst::do_glory, kLvlImplementator, 0, 0},
		{"glorytemp", EPosition::kRest, DoGlory, kLvlBuilder, 0, 0},
		{"gossip", EPosition::kRest, do_gen_comm, 0, kScmdGossip, -1},
		{"goto", EPosition::kSleep, DoGoto, kLvlGod, 0, 0},
		{"grab", EPosition::kRest, do_grab, 0, 0, 500},
		{"group", EPosition::kRest, do_group, 1, 0, 500},
		{"gsay", EPosition::kSleep, do_gsay, 0, 0, -1},
		{"gtell", EPosition::kSleep, do_gsay, 0, 0, -1},
		{"handbook", EPosition::kDead, DoGenericPage, kLvlImmortal, kScmdHandbook, 0},
		{"hcontrol", EPosition::kDead, ClanSystem::DoHcontrol, kLvlGreatGod, 0, 0},
		{"help", EPosition::kDead, do_help, 0, 0, 0},
		{"hell", EPosition::kDead, DoWizutil, kLvlGod, kScmdHell, 0},
		{"hide", EPosition::kStand, do_hide, 1, 0, 0},
		{"hit", EPosition::kFight, DoHit, 0, kScmdHit, -1},
		{"hold", EPosition::kRest, do_grab, 1, 0, 500},
		{"holler", EPosition::kRest, do_gen_comm, 1, kScmdHoller, -1},
		{"horse", EPosition::kStand, do_not_here, 0, 0, -1},
		{"house", EPosition::kRest, ClanSystem::DoHouse, 0, 0, 0},
		{"huk", EPosition::kFight, DoMighthit, 0, 0, -1},
		{"idea", EPosition::kDead, Boards::DoBoard, 1, Boards::IDEA_BOARD, 0},
		{"ignore", EPosition::kDead, do_ignore, 0, 0, 0},
		{"immlist", EPosition::kDead, DoGenericPage, 0, kScmdImmlist, 0},
		{"index", EPosition::kRest, do_help, 1, 0, 500},
		{"info", EPosition::kSleep, DoGenericPage, 0, kScmdInfo, 0},
		{"insert", EPosition::kStand, do_insertgem, 0, 0, -1},
		{"inspect", EPosition::kDead, DoInspect, kLvlBuilder, 0, 0},
		{"insult", EPosition::kRest, do_insult, 0, 0, -1},
		{"inventory", EPosition::kSleep, DoInventory, 0, 0, 0},
		{"invis", EPosition::kDead, do_invis, kLvlGod, 0, -1},
		{"kick", EPosition::kFight, do_kick, 1, 0, -1},
		{"kill", EPosition::kFight, DoKill, 0, 0, -1},
		{"last", EPosition::kDead, DoPageLastLogins, kLvlGod, 0, 0},
		{"levels", EPosition::kDead, do_levels, 0, 0, 0},
		{"list", EPosition::kStand, do_not_here, 0, 0, -1},
		{"load", EPosition::kDead, DoLoad, 0, 0, 0},
		{"loadstat", EPosition::kDead, DoLoadstat, kLvlImplementator, 0, 0},
		{"look", EPosition::kRest, DoLook, 0, kScmdLook, 200},
		{"lock", EPosition::kSit, do_gen_door, 0, kScmdLock, 500},
		{"map", EPosition::kRest, do_map, 0, 0, 0},
		{"mail", EPosition::kStand, do_not_here, 1, 0, -1},
		{"mercenary", EPosition::kStand, do_not_here, 1, 0, -1},
		{"mode", EPosition::kDead, DoMode, 0, 0, 0},
		{"mshout", EPosition::kRest, do_mobshout, 0, 0, -1},
		{"motd", EPosition::kDead, DoGenericPage, 0, kScmdMotd, 0},
		{"murder", EPosition::kFight, DoHit, 0, kScmdMurder, -1},
		{"mute", EPosition::kDead, DoWizutil, kLvlImmortal, kScmdMute, 0},
		{"medit", EPosition::kDead, do_olc, 0, kScmdOlcMedit, 0},
		{"name", EPosition::kDead, DoWizutil, kLvlGod, kScmdName, 0},
		{"nedit", EPosition::kRest, NamedStuff::do_named, kLvlBuilder, SCMD_NAMED_EDIT,
		 0}, //Именной стаф редактирование
		{"news", EPosition::kDead, Boards::DoBoard, 1, Boards::NEWS_BOARD, -1},
		{"nlist", EPosition::kRest, NamedStuff::do_named, kLvlBuilder, SCMD_NAMED_LIST, 0},
		{"notitle", EPosition::kDead, DoWizutil, kLvlGreatGod, kScmdNotitle, 0},
		{"objfind", EPosition::kStand, DoFindObjByRnum, kLvlImplementator, 0, 0},
		{"odelete", EPosition::kStand, DoDeleteObj, kLvlImplementator, 0, 0},
		{"oedit", EPosition::kDead, do_olc, 0, kScmdOlcOedit, 0},
		{"offer", EPosition::kStand, do_not_here, 1, 0, 0},
		{"olc", EPosition::kDead, do_olc, kLvlGod, kScmdOlcSaveinfo, 0},
		{"open", EPosition::kSit, do_gen_door, 0, kScmdOpen, 500},
		{"order", EPosition::kRest, do_order, 1, 0, -1},
		{"overstuff", EPosition::kDead, DoPageClanOverstuff, kLvlGreatGod, 0, 0},
		{"page", EPosition::kDead, do_page, kLvlGod, 0, 0},
		{"parry", EPosition::kFight, DoParry, 0, 0, -1},
		{"pick", EPosition::kStand, do_gen_door, 1, kScmdPick, -1},
		{"poisoned", EPosition::kFight, DoPoisoning, 0, 0, -1},
		{"policy", EPosition::kDead, DoGenericPage, 0, kScmdPolicies, 0},
		{"poofin", EPosition::kDead, DoSetPoofMsg, kLvlGod, kScmdPoofin, 0},
		{"poofout", EPosition::kDead, DoSetPoofMsg, kLvlGod, kScmdPoofout, 0},
		{"pour", EPosition::kStand, do_pour, 0, kScmdPour, -1},
		{"practice", EPosition::kStand, do_not_here, 0, 0, -1},
		{"prompt", EPosition::kDead, do_display, 0, 0, 0},
		{"proxy", EPosition::kDead, do_proxy, kLvlGreatGod, 0, 0},
		{"purge", EPosition::kDead, DoPurge, kLvlGod, 0, 0},
		{"put", EPosition::kRest, do_put, 0, 0, 500},
//	{"python", EPosition::kDead, do_console, kLevelGod, 0, 0},
		{"quaff", EPosition::kRest, do_employ, 0, SCMD_QUAFF, 500},
		{"qui", EPosition::kSleep, do_quit, 0, 0, 0},
		{"quit", EPosition::kSleep, do_quit, 0, kScmdQuit, -1},
		{"read", EPosition::kRest, DoLook, 0, kScmdRead, 200},
		{"receive", EPosition::kStand, do_not_here, 1, 0, -1},
		{"recipes", EPosition::kRest, do_recipes, 0, 0, 0},
		{"recite", EPosition::kRest, do_employ, 0, SCMD_RECITE, 500},
		{"redit", EPosition::kDead, do_olc, 0, kScmdOlcRedit, 0},
		{"register", EPosition::kDead, DoWizutil, kLvlImmortal, kScmdRegister, 0},
		{"unregister", EPosition::kDead, DoWizutil, kLvlImmortal, kScmdUnregister, 0},
		{"reload", EPosition::kDead, DoReload, kLvlImplementator, 0, 0},
		{"remove", EPosition::kRest, do_remove, 0, 0, 500},
		{"rent", EPosition::kStand, do_not_here, 1, 0, -1},
		{"reply", EPosition::kRest, do_reply, 0, 0, -1},
		{"report", EPosition::kRest, do_report, 0, 0, -1},
		{"reroll", EPosition::kDead, DoWizutil, kLvlGreatGod, kScmdReroll, 0},
		{"rescue", EPosition::kFight, do_rescue, 1, 0, -1},
		{"rest", EPosition::kRest, do_rest, 0, 0, -1},
		{"restore", EPosition::kDead, DoRestore, kLvlGreatGod, kScmdRestoreGod, 0},
		{"return", EPosition::kDead, DoReturn, 0, 0, -1},
		{"rset", EPosition::kSleep, do_rset, kLvlBuilder, 0, 0},
		{"rules", EPosition::kDead, DoGenericPage, kLvlImmortal, kScmdRules, 0},
		{"runes", EPosition::kFight, do_mixture, 0, SCMD_RUNES, -1},
		{"save", EPosition::kSleep, do_save, 0, 0, 0},
		{"say", EPosition::kRest, do_say, 0, 0, -1},
		{"scan", EPosition::kRest, DoLookAround, 0, 0, 500},
		{"score", EPosition::kDead, DoScore, 0, 0, 0},
		{"sell", EPosition::kStand, do_not_here, 0, 0, -1},
		{"send", EPosition::kSleep, DoSendMsgToChar, kLvlGreatGod, 0, 0},
		{"sense", EPosition::kStand, do_sense, 0, 0, 500},
		{"set", EPosition::kDead, DoSet, kLvlImmortal, 0, 0},
		{"settle", EPosition::kStand, do_not_here, 1, 0, -1},
		{"shout", EPosition::kRest, do_gen_comm, 0, kScmdShout, -1},
		{"show", EPosition::kDead, do_show, kLvlImmortal, 0, 0},
		{"shutdown", EPosition::kDead, DoShutdown, kLvlImplementator, 0, 0},
		{"sip", EPosition::kRest, DoDrink, 0, kScmdSip, 500},
		{"sit", EPosition::kRest, do_sit, 0, 0, -1},
		{"skills", EPosition::kRest, DoSkills, 0, 0, 0},
		{"skillset", EPosition::kSleep, do_skillset, kLvlImplementator, 0, 0},
		{"slay", EPosition::kFight, do_slay, 1, 0, -1},
		{"setall", EPosition::kDead, do_setall, kLvlImplementator, 0, 0},
		{"sleep", EPosition::kSleep, do_sleep, 0, 0, -1},
		{"sneak", EPosition::kStand, do_sneak, 1, 0, -2},
		{"snoop", EPosition::kDead, DoSnoop, kLvlGreatGod, 0, 0},
		{"socials", EPosition::kDead, do_commands, 0, kScmdSocials, 0},
		{"spells", EPosition::kRest, DoSpells, 0, 0, 0},
		{"split", EPosition::kRest, group::do_split, 1, 0, 0},
		{"stand", EPosition::kRest, do_stand, 0, 0, -1},
		{"stat", EPosition::kDead, do_stat, 0, 0, 0},
		{"steal", EPosition::kStand, do_steal, 1, 0, 300},
		{"strangle", EPosition::kFight, do_strangle, 0, 0, -1},
		{"stupor", EPosition::kFight, DoOverhelm, 0, 0, -1},
		{"switch", EPosition::kDead, DoSwitch, kLvlGreatGod, 0, 0},
		{"syslog", EPosition::kDead, DoSyslog, kLvlImmortal, SYSLOG, 0},
		{"suggest", EPosition::kDead, Boards::report_on_board, 0, Boards::SUGGEST_BOARD, 0},
		{"slist", EPosition::kDead, do_slist, kLvlImplementator, 0, 0},
		{"sedit", EPosition::kDead, do_sedit, kLvlImplementator, 0, 0},
		{"errlog", EPosition::kDead, DoSyslog, kLvlBuilder, ERRLOG, 0},
		{"imlog", EPosition::kDead, DoSyslog, kLvlBuilder, IMLOG, 0},
		{"take", EPosition::kRest, do_get, 0, 0, 500},
		{"taste", EPosition::kRest, do_eat, 0, kScmdTaste, 500},
		{"telegram", EPosition::kDead, do_telegram, kLvlImmortal, 0, -1},
		{"teleport", EPosition::kDead, DoTeleport, kLvlGreatGod, 0, -1},
		{"tell", EPosition::kRest, do_tell, 0, 0, -1},
		{"throwout", EPosition::kFight, DoThrowout, 0, 0, 0},
		{"time", EPosition::kDead, do_time, 0, 0, 0},
		{"title", EPosition::kDead, TitleSystem::do_title, 0, 0, 0},
		{"touch", EPosition::kFight, DoIntercept, 0, 0, -1},
		{"track", EPosition::kStand, do_track, 0, 0, -1},
		{"transfer", EPosition::kStand, do_not_here, 1, 0, -1},
		{"trigedit", EPosition::kDead, do_olc, 0, kScmdOlcTrigedit, 0},
		{"turn undead", EPosition::kRest, do_turn_undead, 0, 0, -1},
		{"typo", EPosition::kDead, Boards::report_on_board, 0, Boards::MISPRINT_BOARD, 0},
		{"unaffect", EPosition::kDead, DoWizutil, kLvlGreatGod, kScmdUnaffect, 0},
		{"unban", EPosition::kDead, do_unban, kLvlGreatGod, 0, 0},
		{"unfreeze", EPosition::kDead, DoUnfreeze, kLvlImplementator, 0, 0},
		{"ungroup", EPosition::kDead, do_ungroup, 0, 0, -1},
		{"unlock", EPosition::kSit, do_gen_door, 0, kScmdUnlock, 500},
		{"uptime", EPosition::kDead, DoPageDateTime, kLvlImmortal, kScmdUptime, 0},
		{"use", EPosition::kSit, do_employ, 1, SCMD_USE, 500},
		{"users", EPosition::kDead, do_users, kLvlImmortal, 0, 0},
		{"value", EPosition::kStand, do_not_here, 0, 0, -1},
		{"version", EPosition::kDead, DoGenericPage, 0, kScmdVersion, 0},
		{"visible", EPosition::kRest, do_visible, 1, 0, -1},
		{"vnum", EPosition::kDead, DoTabulate, kLvlGreatGod, 0, 0},
		{"вномер", EPosition::kDead, DoTabulate, kLvlGreatGod, 0, 0},  //тупой копипаст для использования русского синтаксиса
		{"vstat", EPosition::kDead, DoVstat, 0, 0, 0},
		{"wake", EPosition::kSleep, do_wake, 0, 0, -1},
		{"warcry", EPosition::kFight, do_warcry, 1, 0, -1},
		{"wear", EPosition::kRest, do_wear, 0, 0, 500},
		{"weather", EPosition::kRest, do_weather, 0, 0, 0},
		{"where", EPosition::kRest, DoWhere, kLvlImmortal, 0, 0},
		{"whirl", EPosition::kFight, do_iron_wind, 0, 0, -1},
		{"whisper", EPosition::kRest, do_spec_comm, 0, kScmdWhisper, -1},
		{"who", EPosition::kRest, DoWho, 0, 0, 0},
		{"whoami", EPosition::kDead, DoWhoAmI, 0, 0, 0},
		{"wield", EPosition::kRest, do_wield, 0, 0, 500},
		{"wimpy", EPosition::kDead, do_wimpy, 0, 0, 0},
		{"withdraw", EPosition::kStand, do_not_here, 1, 0, -1},
		{"wizhelp", EPosition::kSleep, do_commands, kLvlImmortal, kScmdWizhelp, 0},
		{"wizlock", EPosition::kDead, DoWizlock, kLvlImplementator, 0, 0},
		{"wiznet", EPosition::kDead, do_wiznet, kLvlImmortal, 0, 0},
		{"wizat", EPosition::kDead, DoAtRoom, kLvlGreatGod, 0, 0},
		{"write", EPosition::kStand, do_write, 1, 0, -1},
		{"zclear", EPosition::kDead, DoClearZone, kLvlImplementator, 0, 0},
		{"zedit", EPosition::kDead, do_olc, 0, kScmdOlcZedit, 0},
		{"zone", EPosition::kRest, DoZone, 0, 0, 0},
		{"zreset", EPosition::kDead, DoZreset, 0, 0, 0},

		// test command for gods
		{"godtest", EPosition::kDead, do_godtest, kLvlGreatGod, 0, 0},
		{"armor", EPosition::kDead, DoPrintArmor, kLvlImplementator, 0, 0},

		// Команды крафтинга - для тестига пока уровня имма
		{"mrlist", EPosition::kDead, do_list_make, kLvlBuilder, 0, 0},
		{"mredit", EPosition::kDead, do_edit_make, kLvlBuilder, 0, 0},
		{"сшить", EPosition::kStand, do_make_item, 0, MAKE_WEAR, 0},
		{"выковать", EPosition::kStand, do_make_item, 0, MAKE_METALL, 0},
		{"смастерить", EPosition::kStand, do_make_item, 0, MAKE_CRAFT, 0},

		// God commands for listing
		{"mlist", EPosition::kDead, do_liblist, 0, kScmdMlist, 0},
		{"olist", EPosition::kDead, do_liblist, 0, kScmdOlist, 0},
		{"rlist", EPosition::kDead, do_liblist, 0, kScmdRlist, 0},
		{"zlist", EPosition::kDead, do_liblist, 0, kScmdZlist, 0},
		{"clist", EPosition::kDead, do_liblist, kLvlGod, kScmdClist, 0},

		{"attach", EPosition::kDead, do_attach, kLvlImplementator, 0, 0},
		{"detach", EPosition::kDead, do_detach, kLvlImplementator, 0, 0},
		{"tlist", EPosition::kDead, do_tlist, 0, 0, 0},
		{"tstat", EPosition::kDead, do_tstat, 0, 0, 0},
		{"vdelete", EPosition::kDead, do_vdelete, kLvlImplementator, 0, 0},
		{"debug_queues", EPosition::kDead, do_debug_queues, kLvlImplementator, 0, 0},
		{heartbeat::cmd::HEARTBEAT_COMMAND, heartbeat::cmd::MINIMAL_POSITION, heartbeat::cmd::do_heartbeat,
		 heartbeat::cmd::MINIMAL_LEVEL, heartbeat::SCMD_NOTHING, heartbeat::cmd::UNHIDE_PROBABILITY},
		//{crafts::cmd::CRAFT_COMMAND, crafts::cmd::MINIMAL_POSITION, crafts::cmd::do_craft, crafts::cmd::MINIMAL_LEVEL, crafts::SCMD_NOTHING, crafts::cmd::UNHIDE_PROBABILITY},
		{"\n", EPosition::kDead, nullptr, 0, 0, 0}
	};

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
	if (IsAffectedBySpell(ch, ESpell::kHide)) {
		if (percent == -2) {
			if (AFF_FLAGGED(ch, EAffect::kSneak)) {
				remove_hide = number(1, MUD::Skill(ESkill::kSneak).difficulty) >
					ch->GetSkill(ESkill::kHide);
			} else {
				percent = 500;
			}
		}
		if (percent == -1) {
			remove_hide = true;
		} else if (percent > 0) {
			remove_hide = number(1, percent) > ch->GetSkill(ESkill::kHide);
		}
		if (remove_hide) {
			RemoveAffectFromChar(ch, ESpell::kHide);
			AFF_FLAGS(ch).unset(EAffect::kHide);
			SendMsgToChar("Вы прекратили прятаться.\r\n", ch);
			act("$n прекратил$g прятаться.", false, ch, nullptr, nullptr, kToRoom);
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
	ch->check_aggressive = false;
	skip_spaces(&argument);

	if (!*argument)
		return;

	if (!ch->IsNpc()) {
		log("<%s> {%5d} [%s]",
			GET_NAME(ch),
			GET_ROOM_VNUM(ch->in_room),
			argument);
		if (GetRealLevel(ch) >= kLvlImmortal || GET_GOD_FLAG(ch, EGf::kPerslog) || GET_GOD_FLAG(ch, EGf::kDemigod))
			pers_log(ch, "<%s> {%5d} [%s]", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), argument);
	}

	int fnum = get_number(&argument);

	/*
	   * special case to handle one-character, non-alphanumeric commands;
	   * requested by many people so "hi" or ";godnet test" is possible.
	   * Patch sent by Eric Green and Stefan Wasilewski.
	   */
	if (!a_isalpha(*argument)) {
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
	} else {
		line = any_one_arg(argument, arg);
	}
// все тримат теперь any_one_arg
//	std::string line2 = line;
//	utils::Trim(line2);
//	line = strdup(line2.c_str());
//	utils::Trim(line);
	const size_t length = strlen(arg);
	if (1 < length && *(arg + length - 1) == '!') {
		hardcopy = true;
		*(arg + length - 1) = '\0';
		*(argument + length - 1) = ' ';
	}
	if (!ch->IsNpc()
		&& !GET_INVIS_LEV(ch)
		&& !AFF_FLAGGED(ch, EAffect::kHold)
		&& !AFF_FLAGGED(ch, EAffect::kStopFight)
		&& !AFF_FLAGGED(ch, EAffect::kMagicStopFight)
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
				if (privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), cmd, 0))
					break;
		} else {
			if (!strncmp(cmd_info[cmd].command, arg, length))
				if (privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), cmd, 0))
					break;
		}
	}

	if (*cmd_info[cmd].command == '\n') {
		if (find_action(arg) >= 0)
			social = true;
		else {
			SendMsgToChar("Чаво?\r\n", ch);
			return;
		}
	}
	if (!is_head(ch->get_name())
		&& ((!ch->IsNpc() && (GET_FREEZE_LEV(ch) > GetRealLevel(ch))
			&& (ch->IsFlagged(EPlrFlag::kFrozen)))
			|| AFF_FLAGGED(ch, EAffect::kHold)
			|| AFF_FLAGGED(ch, EAffect::kStopFight)
			|| AFF_FLAGGED(ch, EAffect::kMagicStopFight))
		&& !check_frozen_cmd(ch, cmd)) {
		SendMsgToChar("Вы попытались, но не смогли сдвинуться с места...\r\n", ch);
		return;
	}

	if (!social && cmd_info[cmd].command_pointer == nullptr) {
		SendMsgToChar("Извините, не смог разобрать команду.\r\n", ch);
		return;
	}

	if (!social && ch->IsNpc() && cmd_info[cmd].minimum_level >= kLvlImmortal) {
		SendMsgToChar("Вы еще не БОГ, чтобы делать это.\r\n", ch);
		return;
	}

	if (!social && ch->GetPosition() < cmd_info[cmd].minimum_position) {
		switch (ch->GetPosition()) {
			case EPosition::kDead: SendMsgToChar("Очень жаль - ВЫ МЕРТВЫ!!! :-(\r\n", ch);
				break;
			case EPosition::kIncap:
			case EPosition::kPerish: SendMsgToChar("Вы в критическом состоянии и не можете ничего делать!\r\n", ch);
				break;
			case EPosition::kStun: SendMsgToChar("Вы слишком слабы, чтобы сделать это!\r\n", ch);
				break;
			case EPosition::kSleep: SendMsgToChar("Сделать это в ваших снах?\r\n", ch);
				break;
			case EPosition::kRest: SendMsgToChar("Нет... Вы слишком расслаблены...\r\n", ch);
				break;
			case EPosition::kSit: SendMsgToChar("Пожалуй, вам лучше встать на ноги.\r\n", ch);
				break;
			case EPosition::kFight: SendMsgToChar("Ни за что! Вы сражаетесь за свою жизнь!\r\n", ch);
				break;
			default: SendMsgToChar("Вы не в том состоянии, чтобы это сделать...\r\n", ch);
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
		if (!ch->IsNpc() && ch->in_room != kNowhere && ch->check_aggressive) {
			ch->check_aggressive = false;
			mob_ai::do_aggressive_room(ch, false);
			if (ch->purged()) {
				return;
			}
		}
	}
}

// ***************************************************************************
// * Various other parsing utilities                                         *
// ***************************************************************************

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether the match must be exact for
 * it to be returned.  Returns -1 if not found; 0...n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(const char *target_string, const char **list, int exact) {
	int i;
	size_t l = strlen(target_string);

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!str_cmp(target_string, *(list + i))) {
				return i;
			}
		}
	} else {
		if (0 == l) {
			l = 1;    // Avoid "" to match the first available string
		}
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!strn_cmp(target_string, *(list + i), l)) {
				return i;
			}
		}
	}

	return -1;
}

int search_block(const std::string &block, const char **list, int exact) {
	int i;
	std::string::size_type l = block.length();

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++)
			if (!str_cmp(block, *(list + i)))
				return (i);
	} else {
		if (!l)
			l = 1;    // Avoid "" to match the first available string
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strn_cmp(block, *(list + i), l))
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
	skip_spaces(&argument);
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
	skip_spaces(&argument);
	return argument;
}

char *one_argument(char *argument, char *first_arg) { return one_argument_template(argument, first_arg); }
const char *one_argument(const char *argument, char *first_arg) { return one_argument_template(argument, first_arg); }
char *any_one_arg(char *argument, char *first_arg) { return any_one_arg_template(argument, first_arg); }
const char *any_one_arg(const char *argument, char *first_arg) { return any_one_arg_template(argument, first_arg); }

void SplitArgument(const char *arguments, std::vector<std::string> &out) {
	char local_buf[kMaxTrglineLength];
	const char *current_arg = arguments;
	out.clear();
	do {
		current_arg = one_argument(current_arg, local_buf);
		if (!*local_buf) {
			break;
		}
		out.emplace_back(local_buf);
	} while (*current_arg);
}

void SplitArgument(const char *arguments, std::vector<short> &out) {
	std::vector<std::string> tmp;
	SplitArgument(arguments, tmp);
	for (const auto &value : tmp) {
		out.push_back(atoi(value.c_str()));
	}
}

void SplitArgument(const char *arguments, std::vector<int> &out) {
	std::vector<std::string> tmp;
	SplitArgument(arguments, tmp);
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
int special(CharData *ch, int cmd, char *argument, int fnum) {
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kHouse)) {
		const auto clan = Clan::GetClanByRoom(ch->in_room);
		if (!clan) {
			return 0;
		}
	}

	ObjData *i;
	int j;

	// special in room? //
	if (GET_ROOM_SPEC(ch->in_room) != nullptr) {
		if (GET_ROOM_SPEC(ch->in_room)(ch, world[ch->in_room], cmd, argument)) {
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in equipment list? //
	for (j = 0; j < EEquipPos::kNumEquipPos; j++) {
		if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != nullptr) {
			if (GET_OBJ_SPEC(GET_EQ(ch, j))(ch, GET_EQ(ch, j), cmd, argument)) {
				check_hiding_cmd(ch, -1);
				return (1);
			}
		}
	}

	// special in inventory? //
	for (i = ch->carrying; i; i = i->get_next_content()) {
		if (GET_OBJ_SPEC(i) != nullptr && GET_OBJ_SPEC(i)(ch, i, cmd, argument)) {
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in mobile present? //
	int specialNum = 1; //если номер не указан - по умолчанию берется первый
	for (const auto k : world[ch->in_room]->people) {
		if (GET_MOB_SPEC(k) != nullptr && (fnum == 1 || fnum == specialNum++)
			&& GET_MOB_SPEC(k)(ch, k, cmd, argument)) {
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	// special in object present? //
	for (i = world[ch->in_room]->contents; i; i = i->get_next_content()) {
		auto spec = GET_OBJ_SPEC(i);
		if (spec != nullptr && spec(ch, i, cmd, argument)) {
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	return (0);
}

// **************************************************************************
// *  Stuff for controlling the non-playing sockets (get name, pwd etc.)     *
// **************************************************************************

// locate entry in p_table with entry->name == name. -1 mrks failed search
int find_name(const char *name) {
	const auto index = player_table.GetIndexByName(name);
	return PlayersIndex::NOT_FOUND == index ? -1 : static_cast<int>(index);
}

int _parse_name(char *argument, char *name) {
	int i;

	// skip whitespaces
	for (i = 0; (*name = (i ? LOWER(*argument) : UPPER(*argument))); argument++, i++, name++) {
		if (*argument == 'ё'
			|| *argument == 'Ё'
			|| !a_isalpha(*argument)
			|| *argument > 0) {
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
int parse_exist_name(char *argument, char *name) {
	int i;

	// skip whitespaces
	for (i = 0; (*name = (i ? LOWER(*argument) : UPPER(*argument))); argument++, i++, name++)
		if (!a_isalpha(*argument) || *argument > 0)
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

	int id = d->character->get_uid();

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

		if (k->original && (k->original->get_uid() == id))    // switched char
		{
			if (str_cmp(d->host, k->host)) {
				sprintf(buf, "ПОВТОРНЫЙ ВХОД! Id = %ld Персонаж = %s Хост = %s(был %s)",
						d->character->get_uid(), GET_NAME(d->character), k->host, d->host);
				mudlog(buf, BRF, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
				//send_to_gods(buf);
			}

			iosystem::write_to_output("\r\nПопытка второго входа - отключаемся.\r\n", k);
			k->state = EConState::kClose;

			if (!target) {
				target = k->original;
				mode = UNSWITCH;
			}

			if (k->character) {
				k->character->desc = nullptr;
			}

			k->character = nullptr;
			k->original = nullptr;
		} else if (k->character && (k->character->get_uid() == id)) {
			if (str_cmp(d->host, k->host)) {
				sprintf(buf, "ПОВТОРНЫЙ ВХОД! Id = %ld Name = %s Host = %s(был %s)",
						d->character->get_uid(), GET_NAME(d->character), k->host, d->host);
				mudlog(buf, BRF, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
				//send_to_gods(buf);
			}

			if (!target &&  k->state == EConState::kPlaying) {
				iosystem::write_to_output("\r\nВаше тело уже кем-то занято!\r\n", k);
				target = k->character;
				mode = USURP;
			}
			k->character->desc = nullptr;
			k->character = nullptr;
			k->original = nullptr;
			iosystem::write_to_output("\r\nПопытка второго входа - отключаемся.\r\n", k);
			k->state = EConState::kClose;
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

	character_list.foreach([&target, &mode, id](const CharData::shared_ptr &ch) {
	  if (ch->IsNpc()) {
		  return;
	  }

	  if (ch->get_uid() != id) {
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
		char_to_room(ch, kStrangeRoom);
		mudlog(fmt::format("Обнаружен дубликат игрока %s, перемещен в ад и очищен.", ch->get_name().c_str()));
		character_list.AddToExtractedList(ch.get());
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
	d->character->UnsetFlag(EPlrFlag::kMailing);
	d->character->UnsetFlag(EPlrFlag::kWriting);
	d->state = EConState::kPlaying;

	switch (mode) {
		case RECON: iosystem::write_to_output("Пересоединяемся.\r\n", d);
			CheckLight(d->character.get(), kLightNo, kLightNo, kLightNo, kLightNo, 1);
			act("$n восстановил$g связь.",
				true, d->character.get(), nullptr, nullptr, kToRoom);
			sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
			mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			login_change_invoice(d->character.get());
			break;

		case USURP: iosystem::write_to_output("Ваша душа вновь вернулась в тело, которое так ждало ее возвращения!\r\n", d);
			act("$n надломил$u от боли, окруженн$w белой аурой...\r\n"
				"Тело $s было захвачено новым духом!",
				true, d->character.get(), nullptr, nullptr, kToRoom);
			sprintf(buf, "%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
			mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			break;

		case UNSWITCH: iosystem::write_to_output("Пересоединяемся для перевключения игрока.", d);
			sprintf(buf, "%s [%s] has reconnected (UNSWITCH).", GET_NAME(d->character), d->host);
			mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			break;

		default:
			// ??? what does this case mean ???
			break;
	}

	network::add_logon_record(d);
	return 1;
}

int pre_help(CharData *ch, char *argument) {
	char command[kMaxInputLength], topic[kMaxInputLength];

	half_chop(argument, command, topic);

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
int check_dupes_host(DescriptorData *d, bool autocheck = false) {
	if (!d->character || IS_IMMORTAL(d->character) || d->character->desc->original)
		return 1;

	// в случае авточекалки нужная проверка уже выполнена до входа в функцию
	if (!autocheck) {
		if (RegisterSystem::IsRegistered(d->character.get())) {
			return 1;
		}

		if (RegisterSystem::IsRegisteredEmail(GET_EMAIL(d->character))) {
			d->registered_email = true;
			return 1;
		}
	}

	for (DescriptorData *i = descriptor_list; i; i = i->next) {
		if (i != d
			&& i->ip == d->ip
			&& i->character
			&& !IS_IMMORTAL(i->character)
			&&  (i->state == EConState::kPlaying
				||  i->state == EConState::kMenu)) {
			switch (CheckProxy(d)) {
				case 0:
					// если уже сидим в проксе, то смысла спамить никакого
					if (d->character->in_room == r_unreg_start_room
						|| d->character->get_was_in_room() == r_unreg_start_room) {
						return 0;
					}
					SendMsgToChar(d->character.get(),
								  "&RВы вошли с игроком %s с одного IP(%s)!\r\n"
								  "Вам необходимо обратиться к Богам для регистрации.\r\n"
								  "Пока вы будете помещены в комнату для незарегистрированных игроков.&n\r\n",
								  GET_PAD(i->character, 4), i->host);
					sprintf(buf,
							"! ВХОД С ОДНОГО IP ! незарегистрированного игрока.\r\n"
							"Вошел - %s, в игре - %s, IP - %s.\r\n"
							"Игрок помещен в комнату незарегистрированных игроков.",
							GET_NAME(d->character), GET_NAME(i->character), d->host);
					mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
					return 0;

				case 1:
					if (autocheck) {
						return 1;
					}
					SendMsgToChar("&RС вашего IP адреса находится максимально допустимое количество игроков.\r\n"
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
			|| ch->IsNpc()) {
			continue;
		}

		if (!IS_IMMORTAL(ch)
			&& (!str_cmp(GET_EMAIL(ch), GET_EMAIL(d->character)))) {
			sprintf(buf, "Персонаж с таким email уже находится в игре, вы не можете войти одновременно с ним!");
			SendMsgToChar(buf, d->character.get());
			return (0);
		}
	}

	return 1;
}

// * Проверка на доступные религии конкретной профе (из текущей генерации чара).
void check_religion(CharData *ch) {
	if (class_religion[to_underlying(ch->GetClass())] == kReligionPoly && GET_RELIGION(ch) != kReligionPoly) {
		GET_RELIGION(ch) = kReligionPoly;
		log("Change religion to poly: %s", ch->get_name().c_str());
	} else if (class_religion[to_underlying(ch->GetClass())] == kReligionMono && GET_RELIGION(ch) != kReligionMono) {
		GET_RELIGION(ch) = kReligionMono;
		log("Change religion to mono: %s", ch->get_name().c_str());
	}
}

void do_entergame(DescriptorData *d) {
	int load_room, cmd, flag = 0;

	d->character->reset();
	ReadAliases(d->character.get());

	if (GetRealLevel(d->character) == kLvlImmortal) {
		d->character->set_level(kLvlGod);
	}

	if (GetRealLevel(d->character) > kLvlImplementator) {
		d->character->set_level(1);
	}

	if (GET_INVIS_LEV(d->character) > kLvlImplementator
		|| GET_INVIS_LEV(d->character) < 0) {
		SET_INVIS_LEV(d->character, 0);
	}

	if (GetRealLevel(d->character) > kLvlImmortal
		&& GetRealLevel(d->character) < kLvlBuilder
		&& (d->character->get_gold() > 0 || d->character->get_bank() > 0)) {
		d->character->set_gold(0);
		d->character->set_bank(0);
	}

	if (GetRealLevel(d->character) >= kLvlImmortal && GetRealLevel(d->character) < kLvlImplementator) {
		for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
			if (!strcmp(cmd_info[cmd].command, "syslog")) {
				if (privilege::HasPrivilege(d->character.get(), std::string(cmd_info[cmd].command), cmd, 0)) {
					flag = 1;
					break;
				}
			}
		}

		if (!flag) {
			GET_LOGS(d->character)[0] = 0;
		}
	}

	if (GetRealLevel(d->character) < kLvlImplementator) {
		if (d->character->IsFlagged(EPlrFlag::kInvStart)) {
			SET_INVIS_LEV(d->character, kLvlImmortal);
		}
		if (GET_INVIS_LEV(d->character) > GetRealLevel(d->character)) {
			SET_INVIS_LEV(d->character, GetRealLevel(d->character));
		}

		if (d->character->IsFlagged(EPrf::kCoderinfo)) {
			d->character->UnsetFlag(EPrf::kCoderinfo);
		}
		if (GetRealLevel(d->character) < kLvlGod) {
			if (d->character->IsFlagged(EPrf::kHolylight)) {
				d->character->UnsetFlag(EPrf::kHolylight);
			}
		}
		if (GetRealLevel(d->character) < kLvlGod) {
			if (d->character->IsFlagged(EPrf::kNohassle)) {
				d->character->UnsetFlag(EPrf::kNohassle);
			}
			if (d->character->IsFlagged(EPrf::kRoomFlags)) {
				d->character->UnsetFlag(EPrf::kRoomFlags);
			}
		}

		if (GET_INVIS_LEV(d->character) > 0
			&& GetRealLevel(d->character) < kLvlImmortal) {
			SET_INVIS_LEV(d->character, 0);
		}
	}

	offtop_system::SetStopOfftopFlag(d->character.get());
	// пересчет максимального хп, если нужно
	check_max_hp(d->character.get());
	// проверка и сет религии
	check_religion(d->character.get());

	/*
	   * We have to place the character in a room before equipping them
	   * or equip_char() will gripe about the person in kNowhere.
	   */
	if (d->character->IsFlagged(EPlrFlag::kHelled))
		load_room = r_helled_start_room;
	else if (d->character->IsFlagged(EPlrFlag::kNameDenied))
		load_room = r_named_start_room;
	else if (d->character->IsFlagged(EPlrFlag::kFrozen))
		load_room = r_frozen_start_room;
	else if (!check_dupes_host(d))
		load_room = r_unreg_start_room;
	else {
		if ((load_room = GET_LOADROOM(d->character)) == kNowhere) {
			load_room = calc_loadroom(d->character.get());
		}
		load_room = GetRoomRnum(load_room);

		if (!Clan::MayEnter(d->character.get(), load_room, kHousePortal)) {
			load_room = Clan::CloseRent(load_room);
		}

		if (!is_rent(load_room)) {
			load_room = kNowhere;
		}
	}

	if (load_room == kNowhere) {
		if (GetRealLevel(d->character) >= kLvlImmortal)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}

	SendMsgToChar(WELC_MESSG, d->character.get());

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
		character->UnsetFlag(EMobFlag::kMobDeleted);
		character->UnsetFlag(EMobFlag::kMobFreed);
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
	if (d->character->IsFlagged(EPrf::kShadowThrow)) {
		d->character->UnsetFlag(EPrf::kShadowThrow);
	}

	if (d->character->IsFlagged(EPrf::kPunctual)
		&& !d->character->GetSkill(ESkill::kPunctual)) {
		d->character->UnsetFlag(EPrf::kPunctual);
	}

	if (d->character->IsFlagged(EPrf::kAwake)
		&& !d->character->GetSkill(ESkill::kAwake)) {
		d->character->UnsetFlag(EPrf::kAwake);
	}

	if (d->character->IsFlagged(EPrf::kPerformPowerAttack) &&
		!CanUseFeat(d->character.get(), EFeat::kPowerAttack)) {
		d->character->UnsetFlag(EPrf::kPerformPowerAttack);
	}
	if (d->character->IsFlagged(EPrf::kPerformGreatPowerAttack) &&
		!CanUseFeat(d->character.get(), EFeat::kGreatPowerAttack)) {
		d->character->UnsetFlag(EPrf::kPerformGreatPowerAttack);
	}
	if (d->character->IsFlagged(EPrf::kPerformAimingAttack) &&
		!CanUseFeat(d->character.get(), EFeat::kAimingAttack)) {
		d->character->UnsetFlag(EPrf::kPerformAimingAttack);
	}
	if (d->character->IsFlagged(EPrf::kPerformGreatAimingAttack) &&
		!CanUseFeat(d->character.get(), EFeat::kGreatAimingAttack)) {
		d->character->UnsetFlag(EPrf::kPerformGreatAimingAttack);
	}
	if (d->character->IsFlagged(EPrf::kDoubleThrow) &&
		!CanUseFeat(d->character.get(), EFeat::kDoubleThrower)) {
		d->character->UnsetFlag(EPrf::kDoubleThrow);
	}
	if (d->character->IsFlagged(EPrf::kTripleThrow) &&
		!CanUseFeat(d->character.get(), EFeat::kTripleThrower)) {
		d->character->UnsetFlag(EPrf::kTripleThrow);
	}
	if (d->character->IsFlagged(EPrf::kPerformSerratedBlade) &&
		!CanUseFeat(d->character.get(), EFeat::kSerratedBlade)) {
		d->character->UnsetFlag(EPrf::kPerformSerratedBlade);
	}
	if (d->character->IsFlagged(EPrf::kSkirmisher)) {
		d->character->UnsetFlag(EPrf::kSkirmisher);
	}
	if (d->character->IsFlagged(EPrf::kIronWind)) {
		d->character->UnsetFlag(EPrf::kIronWind);
	}

	// Check & remove/add natural, race & unavailable features
	UnsetInaccessibleFeats(d->character.get());
	SetInbornAndRaceFeats(d->character.get());

	if (!IS_IMMORTAL(d->character)) {
		for (const auto &skill : MUD::Skills()) {
			if (MUD::Class((d->character)->GetClass()).skills[skill.GetId()].IsInvalid()) {
				d->character->set_skill(skill.GetId(), 0);
			}
		}

		for (const auto &spell : MUD::Spells()) {
			if (IS_SPELL_SET(d->character, spell.GetId(), ESpellType::kKnow)) {
				if (MUD::Class((d->character)->GetClass()).spells[spell.GetId()].IsInvalid()) {
					UNSET_SPELL_TYPE(d->character, spell.GetId(), ESpellType::kKnow);
				}
			}
		}
	}

	temporary_spells::update_char_times(d->character.get(), time(nullptr));

	// Сбрасываем явно не нужные аффекты.
	d->character->remove_affect(EAffect::kGroup);
	d->character->remove_affect(EAffect::kHorse);

	d->character->DeleteIrrelevantRunestones();

	// with the copyover patch, this next line goes in enter_player_game()
	chardata_by_uid[d->character->get_uid()] = d->character.get();
	GET_ACTIVITY(d->character) = number(0, kPlayerSaveActivity - 1);
	d->character->set_last_logon(time(nullptr));
//	player_table[GetPtableByUnique(d->character->get_uid())].last_logon = LAST_LOGON(d->character);
	player_table[d->character->get_pfilepos()].last_logon = LAST_LOGON(d->character);
	network::add_logon_record(d);
	// чтобы восстановление маны спам-контроля "кто" не шло, когда чар заходит после
	// того, как повисел на менюшке; важно, чтобы этот вызов шел раньше save_char()
	d->character->set_who_last(time(nullptr));
	d->character->save_char();
	act("$n вступил$g в игру.", true, d->character.get(), nullptr, nullptr, kToRoom);
	// with the copyover patch, this next line goes in enter_player_game()
	read_saved_vars(d->character.get());
	enter_wtrigger(world[d->character->in_room], d->character.get(), -1);
	greet_mtrigger(d->character.get(), -1);
	greet_otrigger(d->character.get(), -1);
	d->state = EConState::kPlaying;
	d->character->SetFlag(EPrf::kColor2); // цвет всегда полный
// режимы по дефолту у нового чара
	const bool new_char = d->character->GetLevel() <= 0;
	if (new_char) {
		d->character->SetFlag(EPrf::kDrawMap);
		d->character->SetFlag(EPrf::kGoAhead); //IAC GA
		d->character->SetFlag(EPrf::kAutomem);
		d->character->SetFlag(EPrf::kAutoloot);
		d->character->SetFlag(EPrf::kPklMode);
		d->character->SetFlag(EPrf::kClanmembersMode); // соклан
		d->character->map_set_option(MapSystem::MAP_MODE_MOB_SPEC_SHOP);
		d->character->map_set_option(MapSystem::MAP_MODE_MOB_SPEC_RENT);
		d->character->map_set_option(MapSystem::MAP_MODE_MOB_SPEC_BANK);
		d->character->map_set_option(MapSystem::MAP_MODE_MOB_SPEC_TEACH);
		d->character->map_set_option(MapSystem::MAP_MODE_BIG);
		d->character->SetFlag(EPrf::kShowZoneNameOnEnter);
		d->character->SetFlag(EPrf::kBoardMode);
		d->character->set_last_exchange(time(nullptr));
		DoPcInit(d->character.get(), true);
		d->character->mem_queue.stored = 0;
		SendMsgToChar(START_MESSG, d->character.get());
	}

	init_warcry(d->character.get());

	// На входе в игру вешаем флаг (странно, что он до этого нигде не вешался
	if (privilege::IsContainedInGodsList(GET_NAME(d->character), d->character->get_uid())
		&& (GetRealLevel(d->character) < kLvlGod)) {
		SET_GOD_FLAG(d->character, EGf::kDemigod);
	}
	// Насильственно забираем этот флаг у иммов (если он, конечно же, есть
	if ((GET_GOD_FLAG(d->character, EGf::kDemigod) && GetRealLevel(d->character) >= kLvlGod)) {
		CLR_GOD_FLAG(d->character, EGf::kDemigod);
	}

	switch (d->character->get_sex()) {
		case EGender::kLast: [[fallthrough]];
		case EGender::kNeutral: sprintf(buf, "%s вошло в игру.", GET_NAME(d->character));
			break;
		case EGender::kMale: sprintf(buf, "%s вошел в игру.", GET_NAME(d->character));
			break;
		case EGender::kFemale: sprintf(buf, "%s вошла в игру.", GET_NAME(d->character));
			break;
		case EGender::kPoly: sprintf(buf, "%s вошли в игру.", GET_NAME(d->character));
			break;
	}

	mudlog(buf, NRM, std::max(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
	d->has_prompt = 0;
	login_change_invoice(d->character.get());
	affect_total(d->character.get());
	CheckLight(d->character.get(), kLightNo, kLightNo, kLightNo, kLightNo, 0);
	look_at_room(d->character.get(), false);

	if (new_char) {
		SendMsgToChar("\r\nВоспользуйтесь командой НОВИЧОК для получения вводной информации игроку.\r\n",
					  d->character.get());
		SendMsgToChar(
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
	if (PlayerRace::GetKinNameByNum(GET_KIN(d->character), d->character->get_sex()) == KIN_NAME_UNDEFINED) {
		iosystem::write_to_output("\r\nЧто-то заплутал ты, путник. Откуда бредешь?\r\nВыберите народ:\r\n", d);
		iosystem::write_to_output(string(PlayerRace::ShowKinsMenu()).c_str(), d);
		iosystem::write_to_output("\r\nВыберите племя: ", d);
		d->state = EConState::kResetKin;
		return false;
	}

	//Некорректный номер рода
	if (PlayerRace::GetRaceNameByNum(GET_KIN(d->character), GET_RACE(d->character), d->character->get_sex())
		== RACE_NAME_UNDEFINED) {
		iosystem::write_to_output("\r\nКакого роду-племени вы будете?\r\n", d);
		iosystem::write_to_output(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
		iosystem::write_to_output("\r\nИз чьих вы будете: ", d);
		d->state = EConState::kResetRace;
		return false;
	}

	// не корректный номер религии
	if (GET_RELIGION(d->character) > kReligionMono) {
		iosystem::write_to_output(religion_menu, d);
		iosystem::write_to_output("\n\rРелигия :", d);
		d->state = EConState::kResetReligion;
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

	if (ban->IsBanned(d->host) == BanList::BAN_SELECT && !d->character->IsFlagged(EPlrFlag::kSiteOk)) {
		iosystem::write_to_output("Извините, вы не можете выбрать этого игрока с данного IP!\r\n", d);
		d->state = EConState::kClose;
		sprintf(buf, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, kLvlGod, SYSLOG, true);
		return;
	}
	if (GetRealLevel(d->character) < circle_restrict) {
		iosystem::write_to_output("Игра временно приостановлена.. Ждем вас немного позже.\r\n", d);
		d->state = EConState::kClose;
		sprintf(buf, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, kLvlGod, SYSLOG, true);
		return;
	}
	if (new_loc_codes.count(GET_EMAIL(d->character)) != 0) {
		iosystem::write_to_output("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
		d->state = EConState::kRandomNumber;
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
			mudlog(buf, CMP, kLvlGod, SYSLOG, true);
			if (d->character->IsFlagged(EPrf::kIpControl)) {
				int random_number = number(1000000, 9999999);
				new_loc_codes[GET_EMAIL(d->character)] = random_number;
				std::string cmd_line =
					fmt::format("python3 send_code.py {} {} &", GET_EMAIL(d->character), random_number);
				auto result = system(cmd_line.c_str());
				UNUSED_ARG(result);
				iosystem::write_to_output("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
				d->state = EConState::kRandomNumber;
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
				kColorRed, load_result, (load_result > 1) ? "S" : "", kColorNrm);
		iosystem::write_to_output(buf, d);
		GET_BAD_PWS(d->character) = 0;
	}
	time_t tmp_time = LAST_LOGON(d->character);
	sprintf(buf, "\r\nПоследний раз вы заходили к нам в %s с адреса (%s).\r\n",
			rustime(localtime(&tmp_time)), GET_LASTIP(d->character));
	iosystem::write_to_output(buf, d);

	//if (!GloryMisc::check_stats(d->character))
	if (!ValidateStats(d)) {
		return;
	}

	iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	d->state = EConState::kRmotd;
}

void CreateChar(DescriptorData *d) {
	if (d->character) {
		return;
	}

	d->character = std::make_shared<Player>();
	d->character->player_specials = std::make_shared<player_special_data>();
	d->character->desc = d;
}

// initialize a new character only if class is set
void init_char(CharData *ch, PlayerIndexElement &element) {
	int i;

#ifdef TEST_BUILD
	if (0 == player_table.size())
  {
  // При собирании через make test первый чар в маде становится иммом 34
  ch->set_level(kLvlImplementator);
  }
#endif

	CREATE(GET_LOGS(ch), 1 + LAST_LOG);
	ch->set_npc_name(nullptr);
	ch->player_data.long_descr = "";
	ch->player_data.description = "";
	ch->player_data.time.birth = time(nullptr);
	ch->player_data.time.played = 0;
	ch->player_data.time.logon = time(nullptr);

	// make favors for sex
	if (ch->get_sex() == EGender::kMale) {
		ch->player_data.weight = number(120, 180);
		ch->player_data.height = number(160, 200);
	} else {
		ch->player_data.weight = number(100, 160);
		ch->player_data.height = number(150, 180);
	}

	ch->set_hit(ch->get_max_hit());
	ch->set_max_move(82);
	ch->set_move(ch->get_max_move());
	ch->real_abils.armor = 100;

	ch->set_uid(++top_idnum);
	element.level = 0;
	element.remorts = 0;
	element.last_logon = -1;
	element.mail = nullptr;//added by WorM mail
	element.last_ip = nullptr;//added by WorM последний айпи

	if (GetRealLevel(ch) > kLvlGod) {
		SetGodSkills(ch);
	}

	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		if (GetRealLevel(ch) < kLvlGreatGod) {
			GET_SPELL_TYPE(ch, spell_id) = ESpellType::kUnknowm;
		} else {
			GET_SPELL_TYPE(ch, spell_id) = ESpellType::kKnow;
		}
	}

	ch->char_specials.saved.affected_by = clear_flags;
	for (auto save = ESaving::kFirst; save <= ESaving::kLast; ++save) {
		SetSave(ch, save, 0);
	}
	for (i = EResist::kFirstResist; i <= EResist::kLastResist; ++i) {
		GET_RESIST(ch, i) = 0;
	}

	if (GetRealLevel(ch) == kLvlImplementator) {
		ch->set_str(25);
		ch->set_int(25);
		ch->set_wis(25);
		ch->set_dex(25);
		ch->set_con(25);
		ch->set_cha(25);
	}
	ch->real_abils.size = 50;

	for (i = 0; i < 3; i++) {
		GET_COND(ch, i) = (GetRealLevel(ch) == kLvlImplementator ? -1 : i == DRUNK ? 0 : 24);
	}
	GET_LASTIP(ch)[0] = 0;
	//	GET_LOADROOM(ch) = start_room;
	ch->SetFlag(EPrf::kDispHp);
	ch->SetFlag(EPrf::kDispMana);
	ch->SetFlag(EPrf::kDispExits);
	ch->SetFlag(EPrf::kDispMove);
	ch->SetFlag(EPrf::kDispExp);
	ch->SetFlag(EPrf::kDispFight);
	ch->UnsetFlag(EPrf::KSummonable);
	ch->SetFlag(EPrf::kColor2);
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
	element.activity = number(0, kObjectSaveActivity - 1);
	element.timer = nullptr;

	return static_cast<int>(player_table.Append(element));
}

void DoAfterEmailConfirm(DescriptorData *d) {
	PlayerIndexElement element(GET_PC_NAME(d->character));

	// Now GET_NAME() will work properly.
	init_char(d->character.get(), element);

	if (d->character->get_pfilepos() < 0) {
		d->character->set_pfilepos(create_entry(element));
	}
	d->character->save_char();
	d->character->get_account()->set_last_login();
	d->character->get_account()->add_player(d->character->get_uid());

	// добавляем в список ждущих одобрения
	if (!(int) NAME_FINE(d->character)) {
		sprintf(buf, "%s - новый игрок. Падежи: %s/%s/%s/%s/%s/%s Email: %s Пол: %s. ]\r\n"
					 "[ %s ждет одобрения имени.",
				GET_NAME(d->character), GET_PAD(d->character, 0),
				GET_PAD(d->character, 1), GET_PAD(d->character, 2),
				GET_PAD(d->character, 3), GET_PAD(d->character, 4),
				GET_PAD(d->character, 5), GET_EMAIL(d->character),
				genders[(int) d->character->get_sex()], GET_NAME(d->character));
		NewNames::add(d->character.get());
	}

	// remove from free names
	player_table.GetNameAdviser().remove(GET_NAME(d->character));

	iosystem::write_to_output(motd, d);
	iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
	d->state = EConState::kRmotd;
	d->character->set_who_mana(0);
	d->character->set_who_last(time(nullptr));

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
		iosystem::write_to_output(
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
		iosystem::write_to_output(
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

void DisplaySelectCharClassMenu(DescriptorData *d) {
	std::ostringstream out;
	out << "\r\n" << "Выберите профессию:" << "\r\n";
	std::vector<ECharClass> char_classes;
	char_classes.reserve(kNumPlayerClasses);
	for (const auto &it : MUD::Classes()) {
		if (it.IsAvailable()) {
			char_classes.push_back(it.GetId());
		}
	}
	std::sort(char_classes.begin(), char_classes.end());
	for (const auto &it : char_classes) {
		out << "  " << kColorCyn << std::right << std::setw(3) << to_underlying(it) + 1 << kColorNrm << ") "
			<< kColorGrn << std::left << MUD::Class(it).GetName() << "\r\n" << kColorNrm;
	}
	iosystem::write_to_output(out.str().c_str(), d);
}

// deal with newcomers and other non-playing sockets
void nanny(DescriptorData *d, char *argument) {
	char buffer[kMaxStringLength];
	int player_i = 0, load_result;
	char tmp_name[kMaxInputLength], pwd_name[kMaxInputLength], pwd_pwd[kMaxInputLength];
	bool is_player_deleted;
	if (d->state != EConState::kConsole)
		skip_spaces(&argument);

	switch (d->state) {
		case EConState::kInit:
			// just connected
		{
			int online_players = 0;
			for (auto i = descriptor_list; i; i = i->next) {
				online_players++;
			}
			sprintf(buffer, "Online: %d\r\n", online_players);
		}

			iosystem::write_to_output(buffer, d);
			ShowEncodingPrompt(d, false);
			d->state = EConState::kGetKeytable;
			break;

			//. OLC states .
		case EConState::kOedit: oedit_parse(d, argument);
			break;

		case EConState::kRedit: redit_parse(d, argument);
			break;

		case EConState::kZedit: zedit_parse(d, argument);
			break;

		case EConState::kMedit: medit_parse(d, argument);
			break;

		case EConState::kTrigedit: trigedit_parse(d, argument);
			break;

		case EConState::kMredit: mredit_parse(d, argument);
			break;

		case EConState::kClanedit: d->clan_olc->clan->Manage(d, argument);
			break;

		case EConState::kSpendGlory:
			if (!Glory::parse_spend_glory_menu(d->character.get(), argument)) {
				Glory::spend_glory_menu(d->character.get());
			}
			break;

		case EConState::kGloryConst:
			if (!GloryConst::parse_spend_glory_menu(d->character.get(), argument)) {
				GloryConst::spend_glory_menu(d->character.get());
			}
			break;

		case EConState::kNamedStuff:
			if (!NamedStuff::parse_nedit_menu(d->character.get(), argument)) {
				NamedStuff::nedit_menu(d->character.get());
			}
			break;

		case EConState::kMapMenu: d->map_options->parse_menu(d->character.get(), argument);
			break;

		case EConState::kTorcExch: ExtMoney::torc_exch_parse(d->character.get(), argument);
			break;

		case EConState::kSedit: {
			try {
				obj_sets_olc::parse_input(d->character.get(), argument);
			}
			catch (const std::out_of_range &e) {
				SendMsgToChar(d->character.get(), "Редактирование прервано: %s", e.what());
				d->sedit.reset();
				d->state = EConState::kPlaying;
			}
			break;
		}
			//. End of OLC states .*/

		case EConState::kGetKeytable:
			if (strlen(argument) > 0)
				argument[0] = argument[strlen(argument) - 1];
			if (*argument == '9') {
				ShowEncodingPrompt(d, true);
				return;
			}
			if (!*argument || *argument < '0' || *argument >= '0' + kCodePageLast) {
				iosystem::write_to_output("\r\nUnknown key table. Retry, please : ", d);
				return;
			}
			d->keytable = (ubyte) *argument - (ubyte) '0';
			ip_log(d->host);
			iosystem::write_to_output(greetings, d);
			d->state = EConState::kGetName;
			break;

		case EConState::kGetName:    // wait for input of name
			if (!d->character) {
				CreateChar(d);
			}

			if (!*argument) {
				d->state = EConState::kClose;
			} else if (!str_cmp("новый", argument)) {
				iosystem::write_to_output(name_rules, d);

				std::stringstream ss;
				ss << "Введите имя";
				const auto free_name_list = player_table.GetNameAdviser().get_random_name_list();
				if (!free_name_list.empty()) {
					ss << " (примеры доступных имен : ";
					ss << JoinRange(free_name_list);
					ss << ")";
				}

				ss << ": ";

				iosystem::write_to_output(ss.str().c_str(), d);
				d->state = EConState::kNewChar;
				return;
			} else {
				if (sscanf(argument, "%s %s", pwd_name, pwd_pwd) == 2) {
					if (parse_exist_name(pwd_name, tmp_name)
						|| (player_i = LoadPlayerCharacter(tmp_name, d->character.get(), ELoadCharFlags::kFindId))
							< 0) {
						iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					}

					if (d->character->IsFlagged(EPlrFlag::kDeleted)
						|| !Password::compare_password(d->character.get(), pwd_pwd)) {
						iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						if (!d->character->IsFlagged(EPlrFlag::kDeleted)) {
							sprintf(buffer, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
							mudlog(buffer, BRF, kLvlImmortal, SYSLOG, true);
						}

						d->character.reset();
						return;
					}

					d->character->UnsetFlag(EPlrFlag::kMailing);
					d->character->UnsetFlag(EPlrFlag::kWriting);
					d->character->UnsetFlag(EPlrFlag::kCryo);
					d->character->set_pfilepos(player_i);
					DoAfterPassword(d);

					return;
				} else {
					if (parse_exist_name(argument, tmp_name) ||
						strlen(tmp_name) < (kMinNameLength - 1) || // дабы можно было войти чарам с 4 буквами
						strlen(tmp_name) > kMaxNameLength ||
						!IsValidName(tmp_name) || fill_word(tmp_name) || reserved_word(tmp_name)) {
						iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					} else if (!IsNameOffline(tmp_name)) {
						player_i = LoadPlayerCharacter(tmp_name, d->character.get(), ELoadCharFlags::kFindId);
						d->character->set_pfilepos(player_i);
						if (IS_IMMORTAL(d->character) || d->character->IsFlagged(EPrf::kCoderinfo)) {
							iosystem::write_to_output("Игрок с подобным именем является БЕССМЕРТНЫМ в игре.\r\n", d);
						} else {
							iosystem::write_to_output("Игрок с подобным именем находится в игре.\r\n", d);
						}
						iosystem::write_to_output("Во избежание недоразумений введите пару ИМЯ ПАРОЛЬ.\r\n", d);
						iosystem::write_to_output("Имя и пароль через пробел : ", d);

						d->character.reset();
						return;
					}
				}

				player_i = LoadPlayerCharacter(tmp_name, d->character.get(), ELoadCharFlags::kFindId);
				if (player_i > -1) {
					d->character->set_pfilepos(player_i);
					if (d->character->IsFlagged(EPlrFlag::kDeleted)) {
						d->character.reset();

						if (!IsNameAvailable(tmp_name) || _parse_name(tmp_name, tmp_name)) {
							iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
							return;
						}

						if (strlen(tmp_name) < (kMinNameLength)) {
							iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
							return;
						}

						CreateChar(d);
						d->character->SetCharAliases(utils::CAP(tmp_name));
						d->character->player_data.PNames[ECase::kNom] = std::string(utils::CAP(tmp_name));
						d->character->set_pfilepos(player_i);
						sprintf(buffer, "Вы действительно выбрали имя %s [ Y(Д) / N(Н) ]? ", tmp_name);
						log("New player %s ip %s", d->character->player_data.PNames[ECase::kNom].c_str(), d->host);
						iosystem::write_to_output(buffer, d);
						d->state = EConState::kNameConfirm;
					} else    // undo it just in case they are set
					{
						if (IS_IMMORTAL(d->character) || d->character->IsFlagged(EPrf::kCoderinfo)) {
							iosystem::write_to_output("Игрок с подобным именем является БЕССМЕРТНЫМ в игре.\r\n", d);
							iosystem::write_to_output("Во избежание недоразумений введите пару ИМЯ ПАРОЛЬ.\r\n", d);
							iosystem::write_to_output("Имя и пароль через пробел : ", d);
							d->character.reset();

							return;
						}

						d->character->UnsetFlag(EPlrFlag::kMailing);
						d->character->UnsetFlag(EPlrFlag::kWriting);
						d->character->UnsetFlag(EPlrFlag::kCryo);
						iosystem::write_to_output("Персонаж с таким именем уже существует. Введите пароль : ", d);
						d->idle_tics = 0;
						d->state = EConState::kPassword;
					}
				} else    // player unknown -- make new character
				{
					// еще одна проверка
					if (strlen(tmp_name) < (kMinNameLength)) {
						iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					}

					// Check for multiple creations of a character.
					if (!IsNameAvailable(tmp_name) || _parse_name(tmp_name, tmp_name)) {
						iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
						return;
					}

					if (CmpPtableByName(tmp_name, kMinNameLength) >= 0) {
						iosystem::write_to_output("Первые символы вашего имени совпадают с уже существующим персонажем.\r\n"
						 "Для исключения разных недоразумений вам необходимо выбрать другое имя.\r\n"
						 "Имя  : ", d);
						return;
					}

					d->character->SetCharAliases(utils::CAP(tmp_name));
					d->character->player_data.PNames[ECase::kNom] = std::string(utils::CAP(tmp_name));
					iosystem::write_to_output(name_rules, d);
					sprintf(buffer, "Вы действительно выбрали имя  %s [ Y(Д) / N(Н) ]? ", tmp_name);
					log("New player %s ip %s", d->character->player_data.PNames[ECase::kNom].c_str(), d->host);
					iosystem::write_to_output(buffer, d);
					d->state = EConState::kNameConfirm;
				}
			}
			break;

		case EConState::kNameConfirm:    // wait for conf. of new name
			if (UPPER(*argument) == 'Y' || UPPER(*argument) == 'Д') {
				if (ban->IsBanned(d->host) >= BanList::BAN_NEW) {
					sprintf(buffer, "Попытка создания персонажа %s отклонена для [%s] (siteban)",
							GET_PC_NAME(d->character), d->host);
					mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
					iosystem::write_to_output("Извините, создание нового персонажа для вашего IP !!! ЗАПРЕЩЕНО !!!\r\n", d);
					d->state = EConState::kClose;
					return;
				}

				if (circle_restrict) {
					iosystem::write_to_output("Извините, вы не можете создать новый персонаж в настоящий момент.\r\n", d);
					sprintf(buffer, "Попытка создания нового персонажа %s отклонена для [%s] (wizlock)",
							GET_PC_NAME(d->character), d->host);
					mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
					d->state = EConState::kClose;
					return;
				}

				switch (NewNames::auto_authorize(d)) {
					case NewNames::AUTO_ALLOW:
						sprintf(buffer,
								"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
								GET_PAD(d->character, 1));
						iosystem::write_to_output(buffer, d);
						d->state = EConState::kNewpasswd;
						return;

					case NewNames::AUTO_BAN: d->state = EConState::kClose;
						return;

					default: break;
				}

				iosystem::write_to_output("Ваш пол [ М(M)/Ж(F) ]? ", d);
				d->state = EConState::kQsex;
				return;

			} else if (UPPER(*argument) == 'N' || UPPER(*argument) == 'Н') {
				iosystem::write_to_output("Итак, чего изволите? Учтите, бананов нет :)\r\n" "Имя : ", d);
				d->character->SetCharAliases(nullptr);
				d->state = EConState::kGetName;
			} else {
				iosystem::write_to_output("Ответьте Yes(Да) or No(Нет) : ", d);
			}
			break;

		case EConState::kNewChar:
			if (!*argument) {
				d->state = EConState::kClose;
				return;
			}

			if (!d->character) {
				CreateChar(d);
			}

			if (_parse_name(argument, tmp_name) ||
				strlen(tmp_name) < kMinNameLength ||
				strlen(tmp_name) > kMaxNameLength ||
				!IsValidName(tmp_name) || fill_word(tmp_name) || reserved_word(tmp_name)) {
				iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				return;
			}

			player_i = LoadPlayerCharacter(tmp_name, d->character.get(), ELoadCharFlags::kFindId);
			is_player_deleted = false;
			if (player_i > -1) {
				is_player_deleted = d->character->IsFlagged(EPlrFlag::kDeleted);
				if (is_player_deleted) {
					d->character.reset();
					CreateChar(d);
				} else {
					iosystem::write_to_output("Такой персонаж уже существует. Выберите другое имя : ", d);
					d->character.reset();

					return;
				}
			}

			if (!IsNameAvailable(tmp_name)) {
				iosystem::write_to_output("Некорректное имя. Повторите, пожалуйста.\r\n" "Имя : ", d);
				return;
			}

			// skip name check for deleted players
			if (!is_player_deleted && CmpPtableByName(tmp_name, kMinNameLength) >= 0) {
				iosystem::write_to_output("Первые символы вашего имени совпадают с уже существующим персонажем.\r\n"
						  "Для исключения разных недоразумений вам необходимо выбрать другое имя.\r\n"
						  "Имя  : ", d);
				return;
			}

			d->character->SetCharAliases(utils::CAP(tmp_name));
			d->character->player_data.PNames[ECase::kNom] = std::string(utils::CAP(tmp_name));
			if (is_player_deleted) {
				d->character->set_pfilepos(player_i);
			}
			if (ban->IsBanned(d->host) >= BanList::BAN_NEW) {
				sprintf(buffer, "Попытка создания персонажа %s отклонена для [%s] (siteban)",
						GET_PC_NAME(d->character), d->host);
				mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
				iosystem::write_to_output("Извините, создание нового персонажа для вашего IP !!!ЗАПРЕЩЕНО!!!\r\n", d);
				d->state = EConState::kClose;
				return;
			}

			if (circle_restrict) {
				iosystem::write_to_output("Извините, вы не можете создать новый персонаж в настоящий момент.\r\n", d);
				sprintf(buffer,
						"Попытка создания нового персонажа %s отклонена для [%s] (wizlock)",
						GET_PC_NAME(d->character), d->host);
				mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
				d->state = EConState::kClose;
				return;
			}

			switch (NewNames::auto_authorize(d)) {
				case NewNames::AUTO_ALLOW:
					sprintf(buffer,
							"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
							GET_PAD(d->character, 1));
					iosystem::write_to_output(buffer, d);
					d->state = EConState::kNewpasswd;
					return;

				case NewNames::AUTO_BAN: d->character.reset();
					iosystem::write_to_output("Выберите другое имя : ", d);
					return;

				default: break;
			}

			iosystem::write_to_output("Ваш пол [ М(M)/Ж(F) ]? ", d);
			d->state = EConState::kQsex;
			return;

		case EConState::kPassword:    // get pwd for known player
			/*
				   * To really prevent duping correctly, the player's record should
				   * be reloaded from disk at this point (after the password has been
				   * typed). However, I'm afraid that trying to load a character over
				   * an already loaded character is going to cause some problem down the
				   * road that I can't see at the moment.  So to compensate, I'm going to
				   * (1) add a 15 or 20-second time limit for entering a password, and (2)
				   * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
				   */

			iosystem::write_to_output("\r\n", d);

			if (!*argument) {
				d->state = EConState::kClose;
			} else {
				if (!Password::compare_password(d->character.get(), argument)) {
					sprintf(buffer, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
					mudlog(buffer, BRF, kLvlImmortal, SYSLOG, true);
					GET_BAD_PWS(d->character)++;
					d->character->save_char();
					if (++(d->bad_pws) >= max_bad_pws)    // 3 strikes and you're out.
					{
						iosystem::write_to_output("Неверный пароль... Отсоединяемся.\r\n", d);
						d->state = EConState::kClose;
					} else {
						iosystem::write_to_output("Неверный пароль.\r\nПароль : ", d);
					}
					return;
				}
				DoAfterPassword(d);
			}
			break;

		case EConState::kNewpasswd:
		case EConState::kChpwdGetNew:
			if (!Password::check_password(d->character.get(), argument)) {
				sprintf(buffer, "\r\n%s\r\n", Password::BAD_PASSWORD);
				iosystem::write_to_output(buffer, d);
				iosystem::write_to_output("Пароль : ", d);
				return;
			}

			Password::set_password(d->character.get(), argument);

			iosystem::write_to_output("\r\nПовторите пароль, пожалуйста : ", d);
			if (d->state == EConState::kNewpasswd) {
				d->state = EConState::kCnfpasswd;
			} else {
				d->state = EConState::kChpwdVrfy;
			}

			break;

		case EConState::kCnfpasswd:
		case EConState::kChpwdVrfy:
			if (!Password::compare_password(d->character.get(), argument)) {
				iosystem::write_to_output("\r\nПароли не соответствуют... повторим.\r\n", d);
				iosystem::write_to_output("Пароль: ", d);
				if (d->state == EConState::kCnfpasswd) {
					d->state = EConState::kNewpasswd;
				} else {
					d->state = EConState::kChpwdGetNew;
				}
				return;
			}

			if (d->state == EConState::kCnfpasswd) {
				GET_KIN(d->character) = 0;
				DisplaySelectCharClassMenu(d);
				iosystem::write_to_output(
					"\r\nВаша профессия? (Для более полной информации вы можете набрать 'справка <интересующая профессия>'): ",
					d);
				d->state = EConState::kQclass;
			} else {
				sprintf(buffer, "%s заменил себе пароль.", GET_NAME(d->character));
				AddKarma(d->character.get(), buffer, "");
				d->character->save_char();
				iosystem::write_to_output("\r\nГотово.\r\n", d);
				iosystem::write_to_output(MENU, d);
				d->state = EConState::kMenu;
			}

			break;

		case EConState::kQsex:        // query sex of new user
			if (pre_help(d->character.get(), argument)) {
				iosystem::write_to_output("\r\nВаш пол [ М(M)/Ж(F) ]? ", d);
				d->state = EConState::kQsex;
				return;
			}

			switch (UPPER(*argument)) {
				case 'М':
				case 'M': d->character->set_sex(EGender::kMale);
					break;

				case 'Ж':
				case 'F': d->character->set_sex(EGender::kFemale);
					break;

				default: iosystem::write_to_output("Это может быть и пол, но явно не ваш :)\r\n" "А какой у ВАС пол? ", d);
					return;
			}
			iosystem::write_to_output("Проверьте правильность склонения имени. В случае ошибки введите свой вариант.\r\n", d);
			GetCase(d->character->GetCharAliases(), d->character->get_sex(), 1, tmp_name);
			sprintf(buffer, "Имя в родительном падеже (меч КОГО?) [%s]: ", tmp_name);
			iosystem::write_to_output(buffer, d);
			d->state = EConState::kName2;
			return;

		case EConState::kQkin:        // query rass
			if (pre_help(d->character.get(), argument)) {
				iosystem::write_to_output("\r\nКакой народ вам ближе по духу:\r\n", d);
				iosystem::write_to_output(string(PlayerRace::ShowKinsMenu()).c_str(), d);
				iosystem::write_to_output("\r\nПлемя: ", d);
				d->state = EConState::kQkin;
				return;
			}

			load_result = PlayerRace::CheckKin(argument);
			if (load_result == KIN_UNDEFINED) {
				iosystem::write_to_output("Стыдно не помнить предков.\r\n"
						  "Какое Племя вам ближе по духу? ", d);
				return;
			}

			GET_KIN(d->character) = load_result;
			DisplaySelectCharClassMenu(d);
			iosystem::write_to_output(
				"\r\nВаша профессия? (Для более полной информации вы можете набрать 'справка <интересующая профессия>'): ",
				d);
			d->state = EConState::kQclass;
			break;

		case EConState::kQreligion:    // query religion of new user
			if (pre_help(d->character.get(), argument)) {
				iosystem::write_to_output(religion_menu, d);
				iosystem::write_to_output("\n\rРелигия :", d);
				d->state = EConState::kQreligion;
				return;
			}

			switch (UPPER(*argument)) {
				case 'Я':
				case 'З':
				case 'P':
					if (class_religion[to_underlying(d->character->GetClass())] == kReligionMono) {
						iosystem::write_to_output("Персонаж выбранной вами профессии не желает быть язычником!\r\n"
						 "Так каким Богам вы хотите служить? ", d);
						return;
					}
					GET_RELIGION(d->character) = kReligionPoly;
					break;

				case 'Х':
				case 'C':
					if (class_religion[to_underlying(d->character->GetClass())] == kReligionPoly) {
						iosystem::write_to_output("Персонажу выбранной вами профессии противно христианство!\r\n"
						 "Так каким Богам вы хотите служить? ", d);
						return;
					}
					GET_RELIGION(d->character) = kReligionMono;
					break;

				default: iosystem::write_to_output("Атеизм сейчас не моден :)\r\n" "Так каким Богам вы хотите служить? ", d);
					return;
			}

			iosystem::write_to_output("\r\nКакой род вам ближе всего по духу:\r\n", d);
			iosystem::write_to_output(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
			sprintf(buffer, "Для вашей профессией больше всего подходит %s",
					default_race[to_underlying(d->character->GetClass())]);
			iosystem::write_to_output(buffer, d);
			iosystem::write_to_output("\r\nИз чьих вы будете : ", d);
			d->state = EConState::kRace;

			break;

		case EConState::kQclass: {
			if (pre_help(d->character.get(), argument)) {
				DisplaySelectCharClassMenu(d);
				iosystem::write_to_output("\r\nВаша профессия : ", d);
				d->state = EConState::kQclass;
				return;
			}

			int class_num{-1};
			ECharClass class_id{ECharClass::kUndefined};
			try {
				class_num = std::stoi(argument);
			} catch (std::exception &) {
				class_id = FindAvailableCharClassId(argument);
			}
			if (class_num != -1) {
				class_id = MUD::Classes().FindAvailableItem(class_num - 1).GetId();
			}

			if (class_id == ECharClass::kUndefined) {
				iosystem::write_to_output("\r\nЭто не профессия.\r\nПрофессия : ", d);
				return;
			} else {
				d->character->set_class(class_id);
			}

			iosystem::write_to_output(religion_menu, d);
			iosystem::write_to_output("\n\rРелигия :", d);
			d->state = EConState::kQreligion;
			break;
		}

		case EConState::kRace:        // query race
			if (pre_help(d->character.get(), argument)) {
				iosystem::write_to_output("Какой род вам ближе всего по духу:\r\n", d);
				iosystem::write_to_output(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
				iosystem::write_to_output("\r\nРод: ", d);
				d->state = EConState::kRace;
				return;
			}

			load_result = PlayerRace::CheckRace(GET_KIN(d->character), argument);

			if (load_result == RACE_UNDEFINED) {
				iosystem::write_to_output("Стыдно не помнить предков.\r\n" "Какой род вам ближе всего? ", d);
				return;
			}

			GET_RACE(d->character) = load_result;
			iosystem::write_to_output(string(Birthplaces::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(d->character),
																				  GET_RACE(d->character)))).c_str(), d);
			iosystem::write_to_output("\r\nГде вы хотите начать свои приключения: ", d);
			d->state = EConState::kBirthplace;

			break;

		case EConState::kBirthplace:
			if (pre_help(d->character.get(), argument)) {
				iosystem::write_to_output(string(Birthplaces::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(d->character),
																					  GET_RACE(d->character)))).c_str(),
						  d);
				iosystem::write_to_output("\r\nГде вы хотите начать свои приключения: ", d);
				d->state = EConState::kBirthplace;
				return;
			}

			load_result = PlayerRace::CheckBirthPlace(GET_KIN(d->character), GET_RACE(d->character), argument);

			if (!Birthplaces::CheckId(load_result)) {
				iosystem::write_to_output("Не уверены? Бывает.\r\n"
						  "Подумайте еще разок, и выберите:", d);
				return;
			}
			GET_LOADROOM(d->character) = calc_loadroom(d->character.get(), load_result);
			iosystem::write_to_output(genchar_help, d);
			iosystem::write_to_output("\r\n\r\nНажмите любую клавишу.\r\n", d);
			d->state = EConState::kRollStats;
			SetStartAbils(d->character.get());
			break;

		case EConState::kRollStats:
			if (pre_help(d->character.get(), argument)) {
				genchar_disp_menu(d->character.get());
				d->state = EConState::kRollStats;
				return;
			}

			switch (genchar_parse(d->character.get(), argument)) {
				case kGencharContinue: genchar_disp_menu(d->character.get());
					break;
				default: iosystem::write_to_output("\r\nВведите ваш E-mail"
								   "\r\n(ВСЕ ВАШИ ПЕРСОНАЖИ ДОЛЖНЫ ИМЕТЬ ОДИНАКОВЫЙ E-mail)."
								   "\r\nНа этот адрес вам будет отправлен код для подтверждения: ", d);
					d->state = EConState::kGetEmail;
					break;
			}
			break;

		case EConState::kGetEmail:
			if (!*argument) {
				iosystem::write_to_output("\r\nВаш E-mail : ", d);
				return;
			} else if (!IsValidEmail(argument)) {
				iosystem::write_to_output("\r\nНекорректный E-mail!" "\r\nВаш E-mail :  ", d);
				return;
			}
#ifdef TEST_BUILD
			strncpy(GET_EMAIL(d->character), argument, 127);
	  *(GET_EMAIL(d->character) + 127) = '\0';
	  utils::ConvertToLow(GET_EMAIL(d->character));
	  DoAfterEmailConfirm(d);
	  break;
#endif
			{
				int random_number = number(1000000, 9999999);
				new_char_codes[d->character->GetCharAliases()] = random_number;
				strncpy(GET_EMAIL(d->character), argument, 127);
				*(GET_EMAIL(d->character) + 127) = '\0';
				utils::ConvertToLow(GET_EMAIL(d->character));
				std::string cmd_line =
					fmt::format("python3 send_code.py {} {} &", GET_EMAIL(d->character), random_number);
				auto result = system(cmd_line.c_str());
				UNUSED_ARG(result);
				iosystem::write_to_output("\r\nВам на электронную почту был выслан код. Введите его, пожалуйста: \r\n", d);
				d->state = EConState::kRandomNumber;
			}
			break;

		case EConState::kRmotd:    // read CR after printing motd
			if (!check_dupes_email(d)) {
				d->state = EConState::kClose;
				break;
			}

			do_entergame(d);

			break;

		case EConState::kRandomNumber: {
			int code_rand = atoi(argument);

			if (new_char_codes.count(d->character->GetCharAliases()) != 0) {
				if (new_char_codes[d->character->GetCharAliases()] != code_rand) {
					iosystem::write_to_output("\r\nВы ввели неправильный код, попробуйте еще раз.\r\n", d);
					break;
				}
				new_char_codes.erase(d->character->GetCharAliases());
				DoAfterEmailConfirm(d);
				break;
			}

			if (new_loc_codes.count(GET_EMAIL(d->character)) == 0) {
				break;
			}

			if (new_loc_codes[GET_EMAIL(d->character)] != code_rand) {
				iosystem::write_to_output("\r\nВы ввели неправильный код, попробуйте еще раз.\r\n", d);
				d->state = EConState::kClose;
				break;
			}

			new_loc_codes.erase(GET_EMAIL(d->character));
			network::add_logon_record(d);
			DoAfterPassword(d);

			break;
		}

		case EConState::kMenu:        // get selection from main menu
			switch (*argument) {
				case '0': iosystem::write_to_output("\r\nДо встречи на земле Киевской.\r\n", d);

					if (GetRealRemort(d->character) == 0
						&& GetRealLevel(d->character) <= 25
						&& !d->character->IsFlagged(EPlrFlag::kNoDelete)) {
						int timeout = -1;
						for (int ci = 0; GetRealLevel(d->character) > pclean_criteria[ci].level; ci++) {
							//if (GetRealLevel(d->character) == pclean_criteria[ci].level)
							timeout = pclean_criteria[ci + 1].days;
						}
						if (timeout > 0) {
							time_t deltime = time(nullptr) + timeout * 60 * rent_file_timeout * 24;
							sprintf(buffer,
									"В случае вашего отсутствия персонаж будет храниться до %s нашей эры :).\r\n",
									rustime(localtime(&deltime)));
							iosystem::write_to_output(buffer, d);
						}
					}

					d->state = EConState::kClose;

					break;

				case '1':
					if (!check_dupes_email(d)) {
						d->state = EConState::kClose;
						break;
					}

					do_entergame(d);

					break;

				case '2':
					if (!d->character->player_data.description.empty()) {
						iosystem::write_to_output("Ваше ТЕКУЩЕЕ описание:\r\n", d);
						iosystem::write_to_output(d->character->player_data.description.c_str(), d);
						/*
									 * Don't free this now... so that the old description gets loaded
									 * as the current buffer in the editor.  Do set up the ABORT buffer
									 * here, however.
									 *
									 * free(d->character->player_data.description);
									 * d->character->player_data.description = NULL;
									 */
						d->backstr = str_dup(d->character->player_data.description.c_str());
					}

					iosystem::write_to_output("Введите описание вашего героя, которое будет выводиться по команде <осмотреть>.\r\n", d);
					iosystem::write_to_output("(/s сохранить /h помощь)\r\n", d);

					d->writer =
						std::make_shared<utils::DelegatedStdStringWriter>(d->character->player_data.description);
					d->max_str = kExdscrLength;
					d->state = EConState::kExdesc;

					break;

				case '3': page_string(d, background, 0);
					d->state = EConState::kRmotd;
					break;

				case '4': iosystem::write_to_output("\r\nВведите СТАРЫЙ пароль : ", d);
					d->state = EConState::kChpwdGetOld;
					break;

				case '5':
					if (IS_IMMORTAL(d->character)) {
						iosystem::write_to_output("\r\nБоги бессмертны (с) Стрибог, просите чтоб пофризили :)))\r\n", d);
						iosystem::write_to_output(MENU, d);
						break;
					}

					if (d->character->IsFlagged(EPlrFlag::kHelled)
						|| d->character->IsFlagged(EPlrFlag::kFrozen)) {
						iosystem::write_to_output("\r\nВы находитесь в АДУ!!! Амнистии подобным образом не будет.\r\n", d);
						iosystem::write_to_output(MENU, d);
						break;
					}

					if (GetRealRemort(d->character) > 5) {
						iosystem::write_to_output("\r\nНельзя удалить себя достигнув шестого перевоплощения.\r\n", d);
						iosystem::write_to_output(MENU, d);
						break;
					}

					iosystem::write_to_output("\r\nДля подтверждения введите свой пароль : ", d);
					d->state = EConState::kDelcnf1;

					break;

				case '6':
					if (IS_IMMORTAL(d->character)) {
						iosystem::write_to_output("\r\nВам это ни к чему...\r\n", d);
						iosystem::write_to_output(MENU, d);
						d->state = EConState::kMenu;
					} else {
						stats_reset::print_menu(d);
						d->state = EConState::kMenuStats;
					}
					break;

				case '7':
					if (!d->character->IsFlagged(EPrf::kBlindMode)) {
						d->character->SetFlag(EPrf::kBlindMode);
						iosystem::write_to_output("\r\nСпециальный режим слепого игрока ВКЛЮЧЕН.\r\n", d);
						iosystem::write_to_output(MENU, d);
						d->state = EConState::kMenu;
					} else {
						d->character->UnsetFlag(EPrf::kBlindMode);
						iosystem::write_to_output("\r\nСпециальный режим слепого игрока ВЫКЛЮЧЕН.\r\n", d);
						iosystem::write_to_output(MENU, d);
						d->state = EConState::kMenu;
					}

					break;
				case '8': d->character->get_account()->list_players(d);
					break;

				default: iosystem::write_to_output("\r\nЭто не есть правильный ответ!\r\n", d);
					iosystem::write_to_output(MENU, d);

					break;
			}

			break;

		case EConState::kChpwdGetOld:
			if (!Password::compare_password(d->character.get(), argument)) {
				iosystem::write_to_output("\r\nНеверный пароль.\r\n", d);
				iosystem::write_to_output(MENU, d);
				d->state = EConState::kMenu;
			} else {
				iosystem::write_to_output("\r\nВведите НОВЫЙ пароль : ", d);
				d->state = EConState::kChpwdGetNew;
			}

			return;

		case EConState::kDelcnf1:
			if (!Password::compare_password(d->character.get(), argument)) {
				iosystem::write_to_output("\r\nНеверный пароль.\r\n", d);
				iosystem::write_to_output(MENU, d);
				d->state = EConState::kMenu;
			} else {
				iosystem::write_to_output("\r\n!!! ВАШ ПЕРСОНАЖ БУДЕТ УДАЛЕН !!!\r\n"
						  "Вы АБСОЛЮТНО В ЭТОМ УВЕРЕНЫ?\r\n\r\n"
						  "Наберите \"YES / ДА\" для подтверждения: ", d);
				d->state = EConState::kDelcnf2;
			}

			break;

		case EConState::kDelcnf2:
			if (!strcmp(argument, "yes")
				|| !strcmp(argument, "YES")
				|| !strcmp(argument, "да")
				|| !strcmp(argument, "ДА")) {
				if (d->character->IsFlagged(EPlrFlag::kFrozen)) {
					iosystem::write_to_output("Вы решились на суицид, но Боги остановили вас.\r\n", d);
					iosystem::write_to_output("Персонаж не удален.\r\n", d);
					d->state = EConState::kClose;
					return;
				}
				if (GetRealLevel(d->character) >= kLvlGreatGod) {
					return;
				}
				DeletePcByHimself(GET_NAME(d->character));
				sprintf(buffer, "Персонаж '%s' удален!\r\n" "До свидания.\r\n", GET_NAME(d->character));
				iosystem::write_to_output(buffer, d);
				sprintf(buffer, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GetRealLevel(d->character));
				mudlog(buffer, NRM, kLvlGod, SYSLOG, true);
				d->character->get_account()->remove_player(d->character->get_uid());
				d->state = EConState::kClose;
				return;
			} else {
				iosystem::write_to_output("\r\nПерсонаж не удален.\r\n", d);
				iosystem::write_to_output(MENU, d);
				d->state = EConState::kMenu;
			}
			break;

		case EConState::kName2: skip_spaces(&argument);
			if (strlen(argument) == 0) {
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 1, argument);
			}
			if (!_parse_name(argument, tmp_name)
				&& strlen(tmp_name) >= kMinNameLength
				&& strlen(tmp_name) <= kMaxNameLength
				&& !strn_cmp(tmp_name,
							 GET_PC_NAME(d->character),
							 std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))) {
				d->character->player_data.PNames[ECase::kGen] = std::string(utils::CAP(tmp_name));
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 2, tmp_name);
				sprintf(buffer, "Имя в дательном падеже (отправить КОМУ?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
				d->state = EConState::kName3;
			} else {
				iosystem::write_to_output("Некорректно.\r\n", d);
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 1, tmp_name);
				sprintf(buffer, "Имя в родительном падеже (меч КОГО?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
			}
			break;

		case EConState::kName3: skip_spaces(&argument);

			if (strlen(argument) == 0) {
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 2, argument);
			}

			if (!_parse_name(argument, tmp_name)
				&& strlen(tmp_name) >= kMinNameLength
				&& strlen(tmp_name) <= kMaxNameLength
				&& !strn_cmp(tmp_name,
							 GET_PC_NAME(d->character),
							 std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))) {
				d->character->player_data.PNames[ECase::kDat] = std::string(utils::CAP(tmp_name));
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 3, tmp_name);
				sprintf(buffer, "Имя в винительном падеже (ударить КОГО?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
				d->state = EConState::kName4;
			} else {
				iosystem::write_to_output("Некорректно.\r\n", d);
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 2, tmp_name);
				sprintf(buffer, "Имя в дательном падеже (отправить КОМУ?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
			}
			break;

		case EConState::kName4: skip_spaces(&argument);

			if (strlen(argument) == 0) {
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 3, argument);
			}

			if (!_parse_name(argument, tmp_name)
				&& strlen(tmp_name) >= kMinNameLength
				&& strlen(tmp_name) <= kMaxNameLength
				&& !strn_cmp(tmp_name,
							 GET_PC_NAME(d->character),
							 std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))) {
				d->character->player_data.PNames[ECase::kAcc] = std::string(utils::CAP(tmp_name));
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 4, tmp_name);
				sprintf(buffer, "Имя в творительном падеже (сражаться с КЕМ?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
				d->state = EConState::kName5;
			} else {
				iosystem::write_to_output("Некорректно.\n\r", d);
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 3, tmp_name);
				sprintf(buffer, "Имя в винительном падеже (ударить КОГО?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
			}
			break;

		case EConState::kName5: skip_spaces(&argument);
			if (strlen(argument) == 0)
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 4, argument);
			if (!_parse_name(argument, tmp_name) &&
				strlen(tmp_name) >= kMinNameLength && strlen(tmp_name) <= kMaxNameLength &&
				!strn_cmp(tmp_name,
						  GET_PC_NAME(d->character),
						  std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))
				) {
				d->character->player_data.PNames[ECase::kIns] = std::string(utils::CAP(tmp_name));
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 5, tmp_name);
				sprintf(buffer, "Имя в предложном падеже (говорить о КОМ?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
				d->state = EConState::kName6;
			} else {
				iosystem::write_to_output("Некорректно.\n\r", d);
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 4, tmp_name);
				sprintf(buffer, "Имя в творительном падеже (сражаться с КЕМ?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
			}
			break;

		case EConState::kName6: skip_spaces(&argument);
			if (strlen(argument) == 0)
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 5, argument);
			if (!_parse_name(argument, tmp_name) &&
				strlen(tmp_name) >= kMinNameLength && strlen(tmp_name) <= kMaxNameLength &&
				!strn_cmp(tmp_name,
						  GET_PC_NAME(d->character),
						  std::min<size_t>(kMinNameLength, strlen(GET_PC_NAME(d->character)) - 1))
				) {
				d->character->player_data.PNames[ECase::kPre] = std::string(utils::CAP(tmp_name));
				sprintf(buffer,
						"Введите пароль для %s (не вводите пароли типа '123' или 'qwe', иначе ваших персонажев могут украсть) : ",
						GET_PAD(d->character, 1));
				iosystem::write_to_output(buffer, d);
				d->state = EConState::kNewpasswd;
			} else {
				iosystem::write_to_output("Некорректно.\n\r", d);
				GetCase(GET_PC_NAME(d->character), d->character->get_sex(), 5, tmp_name);
				sprintf(buffer, "Имя в предложном падеже (говорить о КОМ?) [%s]: ", tmp_name);
				iosystem::write_to_output(buffer, d);
			}
			break;

		case EConState::kClose: break;

		case EConState::kResetStats:
			if (pre_help(d->character.get(), argument)) {
				return;
			}

			switch (genchar_parse(d->character.get(), argument)) {
				case kGencharContinue: genchar_disp_menu(d->character.get());
					break;

				default:
					// после перераспределения и сейва в genchar_parse стартовых статов надо учесть морты и славу
					GloryMisc::recalculate_stats(d->character.get());
					// статы срезетили и новые выбрали
					sprintf(buffer, "\r\n%sБлагодарим за сотрудничество. Ж)%s\r\n", kColorBoldGrn, kColorNrm);
					iosystem::write_to_output(buffer, d);

					// Проверяем корректность статов
					// Если что-то некорректно, функция проверки сама вернет чара на доработку.
					if (!ValidateStats(d)) {
						return;
					}

					iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
					d->state = EConState::kRmotd;
			}

			break;

		case EConState::kResetKin:
			if (pre_help(d->character.get(), argument)) {
				iosystem::write_to_output("\r\nКакой народ вам ближе по духу:\r\n", d);
				iosystem::write_to_output(string(PlayerRace::ShowKinsMenu()).c_str(), d);
				iosystem::write_to_output("\r\nПлемя: ", d);
				d->state = EConState::kResetKin;
				return;
			}

			load_result = PlayerRace::CheckKin(argument);

			if (load_result == KIN_UNDEFINED) {
				iosystem::write_to_output("Стыдно не помнить предков.\r\n"
						  "Какое Племя вам ближе по духу? ", d);
				return;
			}

			GET_KIN(d->character) = load_result;

			if (!ValidateStats(d)) {
				return;
			}

			iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
			d->state = EConState::kRmotd;
			break;

		case EConState::kResetRace:
			if (pre_help(d->character.get(), argument)) {
				iosystem::write_to_output("Какой род вам ближе всего по духу:\r\n", d);
				iosystem::write_to_output(string(PlayerRace::ShowRacesMenu(GET_KIN(d->character))).c_str(), d);
				iosystem::write_to_output("\r\nРод: ", d);
				d->state = EConState::kResetRace;
				return;
			}

			load_result = PlayerRace::CheckRace(GET_KIN(d->character), argument);

			if (load_result == RACE_UNDEFINED) {
				iosystem::write_to_output("Стыдно не помнить предков.\r\n" "Какой род вам ближе всего? ", d);
				return;
			}

			GET_RACE(d->character) = load_result;

			if (!ValidateStats(d)) {
				return;
			}

			// способности нового рода проставятся дальше в do_entergame
			iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
			d->state = EConState::kRmotd;

			break;

		case EConState::kMenuStats: stats_reset::parse_menu(d, argument);
			break;

		case EConState::kResetReligion:
			if (pre_help(d->character.get(), argument)) {
				iosystem::write_to_output(religion_menu, d);
				iosystem::write_to_output("\n\rРелигия :", d);
				return;
			}

			switch (UPPER(*argument)) {
				case 'Я':
				case 'З':
				case 'P':
					if (class_religion[to_underlying(d->character->GetClass())] == kReligionMono) {
						iosystem::write_to_output("Персонаж выбранной вами профессии не желает быть язычником!\r\n"
						 "Так каким Богам вы хотите служить? ", d);
						return;
					}
					GET_RELIGION(d->character) = kReligionPoly;
					break;

				case 'Х':
				case 'C':
					if (class_religion[to_underlying(d->character->GetClass())] == kReligionPoly) {
						iosystem::write_to_output("Персонажу выбранной вами профессии противно христианство!\r\n"
								   "Так каким Богам вы хотите служить? ", d);
						return;
					}

					GET_RELIGION(d->character) = kReligionMono;

					break;

				default: iosystem::write_to_output("Атеизм сейчас не моден :)\r\n" "Так каким Богам вы хотите служить? ", d);
					return;
			}

			if (!ValidateStats(d)) {
				return;
			}

			iosystem::write_to_output("\r\n* В связи с проблемами перевода фразы ANYKEY нажмите ENTER *", d);
			d->state = EConState::kRmotd;

			break;

		default:
			log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
				static_cast<int>(d->state), d->character ? GET_NAME(d->character) : "<unknown>");
			d->state = EConState::kDisconnect;    // Safest to do.

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
bool CompareParam(const std::string &buffer, const char *str, bool full) {
	if (!str || !*str || buffer.empty() || (full && buffer.length() != strlen(str))) {
		return false;
	}

	std::string::size_type i;
	for (i = 0; i != buffer.length() && *str; ++i, ++str) {
		if (LOWER(buffer[i]) != LOWER(*str)) {
			return false;
		}
	}

	if (i == buffer.length()) {
		return true;
	} else {
		return false;
	}
}

// тоже самое с обоими аргументами стринг
bool CompareParam(const std::string &buffer, const std::string &buffer2, bool full) {
	if (buffer.empty() || buffer2.empty()
		|| (full && buffer.length() != buffer2.length())) {
		return false;
	}

	std::string::size_type i;
	for (i = 0; i != buffer.length() && i != buffer2.length(); ++i) {
		if (LOWER(buffer[i]) != LOWER(buffer2[i])) {
			return false;
		}
	}

	if (i == buffer.length()) {
		return true;
	} else {
		return false;
	}
}

// ищет дескриптор игрока(онлайн состояние) по его УИДу
DescriptorData *DescriptorByUid(long uid) {
	DescriptorData *d = nullptr;

	for (d = descriptor_list; d; d = d->next) {
		if (d->character && d->character->get_uid() == uid) {
			break;
		}
	}
	return (d);
}

/**
* ищет УИД игрока по его имени, второй необязательный параметр - учитывать или нет БОГОВ
* проверка уида на -1 нужна потому, что при делете чара (через меню например), его имя
* останется в плеер-листе, но все остальные параметры будут -1
* TODO: т.к. за все это время понадобилось только при добавлении в пкл - встает вопрос нафига было это городить...
* \param god по умолчанию = 0
* \return >0 - уид чара, 0 - не нашли, -1 - нашли, но это оказался бог (только при god = true)
*/

int GetUniqueByName(std::string name, bool god) {
	for (auto &i : player_table) {
		if (i.uid() != -1 && CompareParam(name, i.name(), true)) {
			if (!god) {
				return i.uid();
			} else {
				if (i.level < kLvlImmortal) {
					return i.uid();
				} else {
					return -1;
				}
			}
		}
	}
	return 0;
}

bool IsActiveUser(long unique) {
	time_t currTime = time(nullptr);
	time_t charLogon;
	int inactivityDelay = /* day*/ (3600 * 24) * /*days count*/ 60;
	for (auto &i : player_table) {
		if (i.uid() == unique) {
			charLogon = i.last_logon;
			return currTime - charLogon < inactivityDelay;
		}
	}
	return false;
}

// ищет имя игрока по его УИДу, второй необязательный параметр - учитывать или нет БОГОВ
std::string GetNameByUnique(long unique, bool god) {
	std::string empty;

	for (auto &i : player_table) {
		if (i.uid() == unique) {
			if (!god) {
				return i.name();
			} else {
				if (i.level < kLvlImmortal) {
					return i.name();
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
		return (prefix + fmt::format("{}", exp));
	else if (exp < 1000000000)
		return (prefix + fmt::format("{}", exp / 1000) + " тыс");
	else if (exp < 1000000000000LL)
		return (prefix + fmt::format("{}", exp / 1000000) + " млн");
	else
		return (prefix + fmt::format("{}", exp / 1000000000LL) + " млрд");
}

// * Конвертация имени в нижний регистр + первый сивмол в верхний (для единообразного поиска в контейнерах)
void name_convert(std::string &text) {
	if (!text.empty()) {
		utils::ConvertToLow(text);
		*text.begin() = UPPER(*text.begin());
	}
}

// * Добровольное удаление персонажа через игровое меню.
void DeletePcByHimself(const char *name) {
	Player t_st;
	Player *st = &t_st;
	int id = LoadPlayerCharacter(name, st, ELoadCharFlags::kFindId);

	if (id >= 0) {
		st->SetFlag(EPlrFlag::kDeleted);
		NewNames::remove(st);
		if (NAME_FINE(st)) {
			player_table.GetNameAdviser().add(GET_NAME(st));
		}
		Clan::remove_from_clan(st->get_uid());
		st->save_char();

		ClearCrashSavedObjects(id);
		player_table[id].set_uid(0);
		player_table[id].level = -1;
		player_table[id].remorts = -1;
		player_table[id].last_logon = -1;
		player_table[id].activity = -1;
		if (player_table[id].mail)
			free(player_table[id].mail);
		player_table[id].mail = nullptr;
		if (player_table[id].last_ip)
			free(player_table[id].last_ip);
		player_table[id].last_ip = nullptr;
	}
}

// generic function for commands which are normally overridden by
// special procedures - i.e., shop commands, mail commands, etc.
void do_not_here(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	SendMsgToChar("Эта команда недоступна в этом месте!\r\n", ch);
}

SortStruct *cmd_sort_info{nullptr};
int num_of_cmds{0};

void SortCommands() {
	int a, b, tmp;

	num_of_cmds = 0;

	// first, count commands (num_of_commands is actually one greater than the
	// number of commands; it inclues the '\n'.

	while (*cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	// create data array
	CREATE(cmd_sort_info, num_of_cmds);

	// initialize it
	for (a = 1; a < num_of_cmds; a++) {
		cmd_sort_info[a].sort_pos = a;
		cmd_sort_info[a].is_social = false;
	}

	// the infernal special case
	cmd_sort_info[find_command("insult")].is_social = true;

	// Sort.  'a' starts at 1, not 0, to remove 'RESERVED'
	for (a = 1; a < num_of_cmds - 1; a++)
		for (b = a + 1; b < num_of_cmds; b++)
			if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
					   cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
