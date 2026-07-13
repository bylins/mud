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
#include "gameplay/mechanics/hide.h"
#include "gameplay/mechanics/condition.h"
#include "utils/utils_encoding.h"

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
#ifdef ENABLE_ADMIN_API
#include "engine/network/admin_api.h"
#endif
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
#include "engine/ui/cmd_god/do_spellstat.h"
#include "engine/ui/cmd_god/do_liblist.h"
#include "engine/ui/cmd_god/do_last.h"
#include "engine/ui/cmd_god/do_load.h"
#include "engine/ui/cmd_god/do_loadstat.h"
#include "engine/ui/cmd_god/do_beep.h"
#include "engine/ui/cmd_god/do_overstuff.h"
#include "engine/ui/cmd_god/do_poof_msg.h"
#include "engine/ui/cmd_god/do_profile.h"
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
#include "engine/ui/cmd/do_money.h"
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
#include "engine/ui/cmd/do_wimpy.h"
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
#include "gameplay/crafting/fry.h"
#include "engine/db/db.h"
#include "engine/scripting/dg_scripts.h"
#include "gameplay/fight/assist.h"
#include "gameplay/ai/mobact.h"
#include "gameplay/fight/pk.h"
#include "gameplay/core/genchar.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/glory_const.h"
#include "utils/utils_parse.h"
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
#include "engine/olc/vedun/vedun.h"
#include "engine/ui/cmd_god/do_debug_queues.h"
#include "administration/privilege.h"
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
#include "gameplay/skills/skinning.h"
#include "gameplay/skills/spell_capable.h"
#include "gameplay/mechanics/title.h"
#include "gameplay/statistics/top.h"
#include "gameplay/ai/spec_procs.h"
#include "engine/db/help.h"
#include "mapsystem.h"
#include "gameplay/mechanics/obj_sets.h"
#include "utils/utils.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "gameplay/mechanics/bonus.h"
#include "engine/db/global_objects.h"
#include "administration/accounts.h"
#include "gameplay/skills/slay.h"
#include "gameplay/skills/charge.h"
#include "gameplay/skills/dazzle.h"
#include "gameplay/mechanics/cities.h"
#include "gameplay/mechanics/doors.h"
#include "gameplay/skills/frenzy.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/classes/recalc_mob_params_by_vnum.h"
#include "alias.h"
#include "engine/db/player_index.h"

#if defined WITH_SCRIPTING
#include "scripting.hpp"
#endif

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

extern RoomRnum r_frozen_start_room;
extern const char *religion_menu;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern const char *default_race[];
extern struct PCCleanCriteria pclean_criteria[];
extern int rent_file_timeout;

extern struct show_struct show_fields[];


// external functions
void read_saved_vars(CharData *ch);
void oedit_parse(DescriptorData *d, char *arg);
void redit_parse(DescriptorData *d, char *arg);
void zedit_parse(DescriptorData *d, char *arg);
void medit_parse(DescriptorData *d, char *arg);
void trigedit_parse(DescriptorData *d, char *arg);
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
void DoRunestone(CharData *ch, char *argument, int, int);
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
		{"банк", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kBank), -1},
		{"баланс", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), 0},
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
		{"вклад", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), -1},
		{"вложить", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), -1},
		{"вернуть", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kMail), -1},
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
		{"деньги", EPosition::kRest, do_money, 0, 0, 0},
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
		{"казна", EPosition::kRest, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), 0},
		{"карта", EPosition::kRest, do_map, 0, 0, 0},
		{"камень", EPosition::kSit, DoRunestone, 1, 0, -1},
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
		{"купить", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},

		{"леваярука", EPosition::kRest, do_grab, 1, 0, 300},
		{"лечить", EPosition::kStand, DoFirstaid, 0, 0, -1},
		{"лить", EPosition::kStand, do_pour, 0, kScmdPour, 500},
		{"лошадь", EPosition::kStand, do_specproc, 1, static_cast<int>(specials::ESpecial::kHorse), -1},
		{"лучшие", EPosition::kDead, Rating::DoBest, 0, 0, 0},

		{"маскировка", EPosition::kRest, do_camouflage, 0, 0, 500},
		{"метнуть", EPosition::kFight, DoThrow, 0, kScmdPhysicalThrow, -1},
		{"магазин", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"менять", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kTorc), -1},
		{"месть", EPosition::kRest, do_revenge, 0, 0, 0},
		{"молот", EPosition::kFight, DoMighthit, 0, 0, -1},
		{"молиться", EPosition::kStand, do_pray, 1, SCMD_PRAY, -1},
		{"моястатистика", EPosition::kDead, do_mystat, 0, 0, 0},
		{"мысл", EPosition::kDead, do_quit, 0, 0, 0},
		{"мысль", EPosition::kDead, Boards::report_on_board, 0, Boards::SUGGEST_BOARD, 0},

		{"наемник", EPosition::kStand, do_specproc, 1, static_cast<int>(specials::ESpecial::kMercenary), -1},
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
		{"обмен", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kTorc), 0},
		{"обменять", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kTorc), 0},
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
		{"отправить", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kMail), -1},
		{"оффтоп", EPosition::kDead, do_offtop, 0, 0, -1},
		{"ошеломить", EPosition::kStand, do_stun, 1, 0, -1},
		{"оценить", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), 500},
		{"очки", EPosition::kDead, DoScore, 0, 0, 0},
		{"очепятки", EPosition::kDead, Boards::DoBoard, 1, Boards::MISPRINT_BOARD, 0},
		{"очистить", EPosition::kDead, do_not_here, 0, kScmdClear, -1},
		{"ошибк", EPosition::kDead, do_quit, 0, 0, 0},
		{"ошибка", EPosition::kDead, Boards::report_on_board, 0, Boards::ERROR_BOARD, 0},

		{"парировать", EPosition::kFight, DoParry, 0, 0, -1},
		{"перехватить", EPosition::kFight, DoIntercept, 0, 0, -1},
		{"перековать", EPosition::kStand, do_transform_weapon, 0, SCMD_TRANSFORMWEAPON, -1},
		{"передать", EPosition::kStand, do_givehorse, 0, 0, -1},
		{"перевести", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), -1},
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
		{"получить", EPosition::kStand, do_receive, 1, 0, -1},
		{"политика", EPosition::kSleep, ClanSystem::DoShowPolitics, 0, 0, 0},
		{"помочь", EPosition::kFight, do_assist, 1, 0, -1},
		{"помощь", EPosition::kDead, do_help, 0, 0, 0},
		{"пометить", EPosition::kDead, do_mark, kLvlImplementator, 0, 0},
		{"порез", EPosition::kFight, DoExpedientCut, 0, 0, -1},
		{"поселиться", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kRent), -1},
		{"постой", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kRent), -1},
		{"почта", EPosition::kStand, do_specproc, 1, static_cast<int>(specials::ESpecial::kMail), -1},
		{"пополнить", EPosition::kStand, do_refill, 0, 0, 300},
		{"поручения", EPosition::kRest, do_quest, 1, 0, -1},
		{"появиться", EPosition::kRest, do_visible, 1, 0, -1},
		{"правила", EPosition::kDead, DoGenericPage, 0, kScmdPolicies, 0},
		{"предложение", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kRent), 500},
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
		{"продать", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"фильтровать", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"прыжок", EPosition::kSleep, DoGoto, kLvlGod, 0, 0},

		{"разбудить", EPosition::kRest, do_wake, 0, kScmdWakeUp, -1},
		{"разгруппировать", EPosition::kDead, do_ungroup, 0, 0, 500},
		{"разделить", EPosition::kRest, group::do_split, 1, 0, 500},
		{"разделы", EPosition::kRest, do_help, 1, 0, 500},
		{"разжечь", EPosition::kStand, DoCampfire, 0, 0, -1},
		{"распустить", EPosition::kDead, do_ungroup, 0, 0, 500},
		{"рассмотреть", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"рассчитать", EPosition::kRest, DoFreehelpee, 0, 0, -1},
		{"режим", EPosition::kDead, DoMode, 0, 0, 0},
		{"ремонт", EPosition::kRest, DoRepair, 0, 0, -1},
		{"рецепты", EPosition::kRest, do_recipes, 0, 0, 0},
		{"рекорды", EPosition::kDead, Rating::DoBest, 0, 0, 0},
		{"руны", EPosition::kFight, do_mixture, 0, SCMD_RUNES, -1},

		{"сбить", EPosition::kFight, do_bash, 1, 0, -1},
		{"сальдо", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), -1},
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
		{"список", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
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
		{"учить", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kGuild), -1},
		{"хранилище", EPosition::kDead, ClanSystem::DoStoreHouse, 0, 0, 0},
		{"характеристики", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"кланстаф", EPosition::kStand, ClanSystem::do_clanstuff, 0, 0, 0},
		{"чинить", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
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
		{"bank", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kBank), -1},
		{"balance", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), -1},
		{"ban", EPosition::kDead, do_ban, kLvlGreatGod, 0, 0},
		{"bash", EPosition::kFight, do_bash, 1, 0, -1},
		{"beep", EPosition::kDead, do_beep, kLvlImmortal, 0, 0},
		{"block", EPosition::kFight, do_block, 0, 0, -1},
		{"bug", EPosition::kDead, Boards::report_on_board, 0, Boards::ERROR_BOARD, 0},
		{"buy", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"best", EPosition::kDead, Rating::DoBest, 0, 0, 0},
		{"cast", EPosition::kSit, DoCast, 1, 0, -1},
		{"charge", EPosition::kStand, DoCharge, 0, 0, 0},
		{"check", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kMail), -1},
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
		{"deposit", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), 500},
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
		{"money", EPosition::kRest, do_money, 0, 0, 0},
		{"glide", EPosition::kStand, DoLightwalk, 0, 0, 0},
		{"glory", EPosition::kRest, GloryConst::do_glory, kLvlImplementator, 0, 0},
		{"glorytemp", EPosition::kRest, DoGlory, kLvlBuilder, 0, 0},
		{"gossip", EPosition::kRest, do_gen_comm, 0, kScmdGossip, -1},
		{"goto", EPosition::kSleep, DoGoto, kLvlGod, 0, 0},
		{"grab", EPosition::kRest, do_grab, 0, 0, 500},
		{"group", EPosition::kRest, do_group, 1, 0, 500},
		{"gsay", EPosition::kSleep, do_gsay, 0, 0, -1},
		{"gtell", EPosition::kSleep, do_gsay, 0, 0, -1},
		{"hcontrol", EPosition::kDead, ClanSystem::DoHcontrol, kLvlGreatGod, 0, 0},
		{"help", EPosition::kDead, do_help, 0, 0, 0},
		{"hell", EPosition::kDead, DoWizutil, kLvlGod, kScmdHell, 0},
		{"hide", EPosition::kStand, do_hide, 1, 0, 0},
		{"hit", EPosition::kFight, DoHit, 0, kScmdHit, -1},
		{"hold", EPosition::kRest, do_grab, 1, 0, 500},
		{"holler", EPosition::kRest, do_gen_comm, 1, kScmdHoller, -1},
		{"horse", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kHorse), -1},
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
		{"list", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"load", EPosition::kDead, DoLoad, 0, 0, 0},
		{"loadstat", EPosition::kDead, DoLoadstat, kLvlImplementator, 0, 0},
		{"look", EPosition::kRest, DoLook, 0, kScmdLook, 200},
		{"lock", EPosition::kSit, do_gen_door, 0, kScmdLock, 500},
		{"map", EPosition::kRest, do_map, 0, 0, 0},
		{"mail", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kMail), -1},
		{"mercenary", EPosition::kStand, do_specproc, 1, static_cast<int>(specials::ESpecial::kMercenary), -1},
		{"mode", EPosition::kDead, DoMode, 0, 0, 0},
		{"mshout", EPosition::kRest, do_mobshout, 0, 0, -1},
		{"motd", EPosition::kDead, DoGenericPage, 0, kScmdMotd, 0},
		{"murder", EPosition::kFight, DoHit, 0, kScmdMurder, -1},
		{"mute", EPosition::kDead, DoWizutil, kLvlImmortal, kScmdMute, 0},
		{"medit", EPosition::kDead, do_olc, 0, kScmdOlcMedit, 0},
		{"name", EPosition::kDead, DoWizutil, kLvlGod, kScmdName, 0},
		{"nedit", EPosition::kRest, NamedStuff::do_named, kLvlBuilder, SCMD_NAMED_EDIT, 0},
		{"news", EPosition::kDead, Boards::DoBoard, 1, Boards::NEWS_BOARD, -1},
		{"nlist", EPosition::kRest, NamedStuff::do_named, kLvlBuilder, SCMD_NAMED_LIST, 0},
		{"notitle", EPosition::kDead, DoWizutil, kLvlGreatGod, kScmdNotitle, 0},
		{"objfind", EPosition::kStand, DoFindObjByRnum, kLvlImplementator, 0, 0},
		{"odelete", EPosition::kStand, DoDeleteObj, kLvlImplementator, 0, 0},
		{"oedit", EPosition::kDead, do_olc, 0, kScmdOlcOedit, 0},
		{"offer", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kRent), 0},
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
		{"practice", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kGuild), -1},
		{"prompt", EPosition::kDead, do_display, 0, 0, 0},
		{"profile", EPosition::kDead, do_profile, kLvlImmortal, 0, 0},
		{"proxy", EPosition::kDead, do_proxy, kLvlGreatGod, 0, 0},
		{"purge", EPosition::kDead, DoPurge, kLvlGod, 0, 0},
		{"put", EPosition::kRest, do_put, 0, 0, 500},
		{"quaff", EPosition::kRest, do_employ, 0, SCMD_QUAFF, 500},
		{"qui", EPosition::kSleep, do_quit, 0, 0, 0},
		{"quit", EPosition::kSleep, do_quit, 0, kScmdQuit, -1},
		{"read", EPosition::kRest, DoLook, 0, kScmdRead, 200},
		{"receive", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kMail), -1},
		{"recipes", EPosition::kRest, do_recipes, 0, 0, 0},
		{"recite", EPosition::kRest, do_employ, 0, SCMD_RECITE, 500},
		{"redit", EPosition::kDead, do_olc, 0, kScmdOlcRedit, 0},
		{"register", EPosition::kDead, DoWizutil, kLvlImmortal, kScmdRegister, 0},
		{"unregister", EPosition::kDead, DoWizutil, kLvlImmortal, kScmdUnregister, 0},
		{"reload", EPosition::kDead, DoReload, kLvlImplementator, 0, 0},
		{"remove", EPosition::kRest, do_remove, 0, 0, 500},
		{"rent", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kRent), -1},
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
		{"shop", EPosition::kStand, do_specproc, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"sell", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"send", EPosition::kSleep, DoSendMsgToChar, kLvlGreatGod, 0, 0},
		{"sense", EPosition::kStand, do_sense, 0, 0, 500},
		{"set", EPosition::kDead, DoSet, kLvlImmortal, 0, 0},
		{"settle", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kRent), -1},
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
		{"stone", EPosition::kSit, DoRunestone, 1, 0, -1},
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
		{"transfer", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), -1},
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
		{"value", EPosition::kStand, do_specword, 0, static_cast<int>(specials::ESpecial::kShop), -1},
		{"vedun", EPosition::kDead, vedun::do_vedun, kLvlImplementator, 0, 0},
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
		{"withdraw", EPosition::kStand, do_specword, 1, static_cast<int>(specials::ESpecial::kBank), -1},
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

// dir_fill, reserved moved to mud_string.cpp

void check_hiding_cmd(CharData *ch, int percent) {
	int remove_hide = false;
	if (IsAffectedFlagOnly(ch, EAffect::kHide)) {
		if (percent == -2) {
			if (AFF_FLAGGED(ch, EAffect::kSneak)) {
				remove_hide = number(1, MUD::Skill(ESkill::kSneak).difficulty) >
					GetSkill(ch, ESkill::kHide);
			} else {
				percent = 500;
			}
		}
		if (percent == -1) {
			remove_hide = true;
		} else if (percent > 0) {
			remove_hide = number(1, percent) > GetSkill(ch, ESkill::kHide);
		}
		if (remove_hide) {
			RemoveAffectFromChar(ch, EAffect::kHide);
			MakeVisible(ch, EAffect::kHide);
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
namespace { int s_specproc_fnum = 1; }
int GetSpecprocFnum() { return s_specproc_fnum; }

// Bare spec-proc action words (список/продать/...) route here: the command word IS the action, so we
// prepend it to the argument and run it through the room carrier -- a player at a shop can just type
// "список" instead of "магазин список", and gets the carrier's reply, not "command not available".
void do_specword(CharData *ch, char *argument, int cmd, int subcmd) {
	char line[kMaxInputLength];
	snprintf(line, sizeof(line), "%s %s", cmd_info[cmd].command, argument);
	specials::RunSpecCommand(ch, static_cast<specials::ESpecial>(subcmd), line);
}

// "получить" is claimed by both the bank (withdraw) and the post office (receive); when both could
// apply we prefer the bank (банк получить / почта получить are always unambiguous).
void do_receive(CharData *ch, char *argument, int cmd, int /*subcmd*/) {
	char line[kMaxInputLength];
	snprintf(line, sizeof(line), "%s %s", cmd_info[cmd].command, argument);
	const auto target = specials::IsMobSpecialInRoom(ch->in_room, specials::ESpecial::kBank)
		? specials::ESpecial::kBank
		: specials::ESpecial::kMail;
	specials::RunSpecCommand(ch, target, line);
}

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
		&& !(privilege::IsGod(ch) && !strcmp(arg, "invis")))  // let immortals switch to wizinvis to avoid broken command triggers
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
	if (!privilege::IsOwner(ch)
		&& ((!ch->IsNpc() && (punishments::Get(ch, punishments::EType::kFreeze).level > GetRealLevel(ch))
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
		s_specproc_fnum = fnum;
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

// Used in specprocs, mostly.  (Exactly) matches "command" to cmd number //
int find_command(const char *command) {
	int cmd;

	for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
		if (!strcmp(cmd_info[cmd].command, command))
			return (cmd);

	return (-1);
}

// int fnum - номер найденного в комнате спешиал-моба, для обработки нескольких спешиал-мобов в одной комнате //
int special(CharData *ch, int cmd, char *argument, int /*fnum*/) {
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kHouse)) {
		const auto clan = Clan::GetClanByRoom(ch->in_room);
		if (!clan) {
			return 0;
		}
	}

	ObjData *i;
	int j;

	// special in room? //
	if ((ValidRnum(ch->in_room) ? world[ch->in_room]->func : nullptr) != nullptr) {
		if ((ValidRnum(ch->in_room) ? world[ch->in_room]->func : nullptr)(ch, world[ch->in_room], cmd, argument)) {
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

	// (mob spec procs are data-driven now -- dispatched via do_specproc, not special(); see spec_assign)

	// special in object present? //
	for (auto i : world[ch->in_room]->contents) {
		auto spec = GET_OBJ_SPEC(i);
		if (spec != nullptr && spec(ch, i, cmd, argument)) {
			check_hiding_cmd(ch, -1);
			return (1);
		}
	}

	return (0);
}

// locate entry in p_table with entry->name == name. -1 mrks failed search
int find_name(const char *name) {
	const auto index = player_table.GetIndexByName(name);
	return PlayersIndex::NOT_FOUND == index ? -1 : static_cast<int>(index);
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
