cmake_minimum_required(VERSION 3.10)

project(bylins CXX)

if (NOT ${CMAKE_VERSION} VERSION_LESS "3.12.0")
	cmake_policy(SET CMP0074 NEW)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCES
		src/engine/structs/blocking_queue.cpp
		src/engine/db/influxdb.cpp
		src/engine/core/heartbeat.cpp
		src/engine/core/heartbeat_commands.cpp
		src/gameplay/abilities/abilities_rollsystem.cpp
		src/administration/accounts.cpp
		src/engine/core/action_targeting.cpp
		src/engine/ui/cmd/do_sign.cpp
		src/engine/core/char_movement.cpp
		src/gameplay/communication/social.cpp
		src/gameplay/affects/affect_handler.cpp
		src/engine/ui/alias.cpp
		src/gameplay/economics/auction.cpp
		src/utils/backtrace.cpp
		src/administration/ban.cpp
		src/gameplay/mechanics/birthplaces.cpp
        src/gameplay/communication/boards/boards_changelog_loaders.cpp
        src/gameplay/communication/boards/boards_constants.cpp
        src/gameplay/communication/boards/boards.cpp
        src/gameplay/communication/boards/boards_formatters.cpp
        src/gameplay/communication/boards/boards_types.cpp
		src/gameplay/mechanics/bonus.cpp
		src/gameplay/mechanics/bonus_command_parser.cpp
		src/engine/boot/boot_constants.cpp
		src/engine/boot/boot_data_files.cpp
		src/engine/boot/boot_index.cpp
		src/utils/cache.cpp
		src/gameplay/mechanics/celebrates.cpp
		src/engine/entities/char_data.cpp
		src/engine/entities/char_player.cpp
		src/gameplay/classes/pc_classes.cpp
		src/gameplay/classes/classes_spell_slots.cpp
		src/engine/ui/color.cpp
		src/engine/core/comm.cpp
		src/engine/ui/cmd_god/do_shutdown.cpp
		src/engine/ui/commands.cpp
		src/engine/core/config.cpp
		src/gameplay/core/constants.cpp
		src/gameplay/mechanics/corpse.cpp
		src/gameplay/quests/quest.cpp
		src/gameplay/quests/daily_quest.cpp
		src/engine/db/db.cpp
		src/gameplay/mechanics/deathtrap.cpp
		src/gameplay/mechanics/depot.cpp
		src/engine/db/description.cpp
		src/engine/scripting/dg_comm.cpp
		src/engine/scripting/dg_domination_helper.cpp
		src/engine/scripting/dg_db_scripts.cpp
		src/engine/scripting/dg_event.cpp
		src/engine/scripting/dg_handler.cpp
		src/engine/scripting/dg_misc.cpp
		src/engine/scripting/dg_mobcmd.cpp
		src/engine/scripting/dg_objcmd.cpp
		src/engine/scripting/dg_olc.cpp
		src/engine/scripting/dg_scripts.cpp
		src/engine/scripting/dg_triggers.cpp
		src/engine/scripting/dg_wldcmd.cpp
		src/gameplay/economics/dictionary.cpp
		src/utils/diskio.cpp
		src/gameplay/statistics/dps.cpp
		src/engine/entities/entities_constants.cpp
		src/gameplay/economics/exchange.cpp
		src/gameplay/economics/ext_money.cpp
		src/engine/core/external_trigger.cpp
		src/gameplay/abilities/feats.cpp
		src/gameplay/fight/fight.cpp
		src/gameplay/fight/fight_hit.cpp
		src/gameplay/fight/fight_penalties.cpp
		src/gameplay/fight/fight_stuff.cpp
		src/utils/file_crc.cpp
		src/gameplay/core/genchar.cpp
		src/gameplay/mechanics/glory_const.cpp
		src/gameplay/mechanics/glory.cpp
		src/gameplay/mechanics/glory_misc.cpp
		src/gameplay/ai/graph.cpp
		src/engine/core/handler.cpp
		src/engine/db/help.cpp
		src/gameplay/clans/house.cpp
		src/gameplay/clans/house_exp.cpp
		src/gameplay/communication/ignores.cpp
		src/gameplay/communication/ignores_loader.cpp
		src/gameplay/crafting/im.cpp
		src/engine/ui/interpreter.cpp
		src/gameplay/crafting/item_creation.cpp
		src/gameplay/core/game_limits.cpp
		src/utils/levenshtein.cpp
		src/gameplay/mechanics/liquid.cpp
		src/utils/logger.cpp
		src/gameplay/magic/magic.cpp
		src/gameplay/magic/magic_items.cpp
		src/gameplay/magic/magic_rooms.cpp
		src/gameplay/magic/magic_utils.cpp
		src/gameplay/magic/spells.cpp
		src/gameplay/magic/spells_info.cpp
		src/gameplay/magic/magic_temp_spells.cpp
		src/gameplay/communication/mail.cpp
		src/engine/ui/mapsystem.cpp
		src/gameplay/mechanics/meat_maker.cpp
		src/engine/olc/medit.cpp
		src/gameplay/ai/mobact.cpp
		src/gameplay/mechanics/mobmax.cpp
		src/gameplay/statistics/mob_stat.cpp
		src/engine/ui/modify.cpp
        src/administration/name_adviser.cpp
		src/gameplay/mechanics/named_stuff.cpp
		src/administration/names.cpp
		src/gameplay/mechanics/noob.cpp
		src/engine/entities/obj_data.cpp
		src/gameplay/mechanics/obj_enchant.cpp
		src/engine/db/obj_save.cpp
		src/gameplay/mechanics/obj_save_ext.cpp
		src/gameplay/mechanics/obj_sets.cpp
		src/gameplay/mechanics/obj_sets_olc.cpp
		src/gameplay/magic/magic_objects.cpp
		src/engine/olc/oedit.cpp
		src/engine/olc/olc.cpp
		src/gameplay/communication/parcel.cpp
		src/utils/parse.cpp
		src/administration/password.cpp
		src/gameplay/fight/pk.cpp
		src/gameplay/mechanics/player_races.cpp
		src/administration/privilege.cpp
        src/third_party_libs/pugixml/pugixml.cpp
		src/gameplay/quests/quested.cpp
		src/utils/random.cpp
		src/engine/olc/redit.cpp
		src/gameplay/communication/remember.cpp
		src/gameplay/core/reset_stats.cpp
		src/engine/entities/room_data.cpp
		src/gameplay/mechanics/sets_drop.cpp
		src/gameplay/economics/shop_ext.cpp
		src/administration/shutdown_parameters.cpp
		src/gameplay/skills/skills.cpp
		src/gameplay/skills/skills_info.cpp
		src/gameplay/communication/spam.cpp
		src/gameplay/ai/spec_assign.cpp
		src/gameplay/ai/spec_procs.cpp
		src/engine/network/descriptor_data.cpp
		src/gameplay/mechanics/stuff.cpp
		src/engine/structs/flags.hpp
		src/utils/id_converter.cpp
		src/utils/utils_time.cpp
		src/gameplay/mechanics/title.cpp
		src/gameplay/statistics/top.cpp
		src/utils/utils.cpp
		src/version.cpp
		src/gameplay/mechanics/weather.cpp
		src/engine/olc/zedit.cpp
		src/utils/utils_string.cpp
		src/engine/db/world_objects.cpp
		src/engine/db/obj_prototypes.cpp
		src/engine/db/id.cpp
		src/engine/db/utils_find_obj_id_by_vnum.cpp
		src/engine/db/global_objects.cpp
		src/gameplay/economics/shops_implementation.cpp
		src/engine/db/world_characters.cpp
		src/utils/utils_debug.cpp
		src/gameplay/economics/shops_implementation.cpp
		src/engine/structs/radix_trie.cpp
		src/engine/structs/compact_trie.cpp
		src/engine/db/id.cpp
		src/engine/entities/zone.cpp
		src/gameplay/mechanics/strengthening.cpp
		src/gameplay/skills/backstab.cpp
		src/gameplay/skills/bash.cpp
		src/gameplay/skills/stun.cpp
		src/gameplay/skills/resque.cpp
		src/gameplay/skills/kick.cpp
		src/gameplay/skills/strangle.cpp
		src/gameplay/skills/chopoff.cpp
		src/gameplay/skills/disarm.cpp
		src/gameplay/skills/overhelm.cpp
		src/gameplay/skills/ironwind.cpp
		src/gameplay/skills/throw.cpp
		src/gameplay/skills/mighthit.cpp
		src/gameplay/skills/shield_block.cpp
		src/gameplay/skills/parry.cpp
		src/gameplay/skills/pick.cpp
		src/gameplay/mechanics/poison.cpp
		src/gameplay/skills/protect.cpp
		src/gameplay/skills/expendientcut.cpp
		src/gameplay/skills/turnundead.cpp
		src/gameplay/skills/townportal.cpp
		src/gameplay/skills/manadrain.cpp
		src/gameplay/skills/warcry.cpp
		src/gameplay/skills/identify.cpp
		src/gameplay/skills/relocate.cpp
		src/gameplay/mechanics/mount.cpp
		src/gameplay/fight/assist.cpp
		src/engine/ui/cmd/do_kill.cpp
		src/engine/ui/cmd/do_style.cpp
		src/gameplay/fight/common.cpp
		src/engine/ui/cmd/do_retreat.cpp
		src/engine/ui/cmd/do_order.cpp
		src/engine/ui/cmd/do_mercenary.cpp
		src/gameplay/affects/affect_data.cpp
		src/gameplay/skills/track.cpp
		src/engine/ui/cmd/do_flee.cpp
		src/engine/ui/cmd/do_hire.cpp
		src/engine/ui/cmd/do_telegram.cpp
		src/engine/ui/cmd_god/do_stat.cpp
		src/engine/ui/cmd_god/do_godtest.cpp
		src/engine/ui/cmd/do_learn.cpp
		src/engine/ui/cmd/do_forget.cpp
		src/engine/ui/cmd/do_memorize.cpp
		src/engine/ui/cmd/do_create.cpp
		src/engine/ui/cmd/do_mixture.cpp
		src/engine/ui/cmd/do_cast.cpp
		src/engine/ui/cmd/do_employ.cpp
		src/engine/ui/cmd/do_follow.cpp
		src/engine/structs/flag_data.cpp
		src/gameplay/crafting/jewelry.cpp
		src/gameplay/crafting/mining.cpp
		src/gameplay/classes/classes_constants.cpp
		src/engine/structs/extra_description.cpp
		src/gameplay/communication/insult.cpp
		src/gameplay/abilities/abilities_info.cpp
		src/gameplay/abilities/abilities_constants.cpp
		src/gameplay/classes/pc_classes_info.cpp
		src/engine/ui/objects_filter.cpp
		src/engine/structs/info_container.cpp
		src/utils/parser_wrapper.cpp
		src/engine/boot/cfg_manager.cpp
		src/gameplay/affects/affect_contants.cpp
		src/engine/ui/cmd_god/do_users.cpp
		src/engine/ui/table_wrapper.cpp
		src/engine/ui/cmd/do_score.cpp
		src/gameplay/magic/spells_constants.cpp
		src/gameplay/mechanics/mem_queue.cpp
		src/engine/ui/cmd_god/do_show.cpp
		src/engine/ui/cmd/do_drop.cpp
		src/gameplay/skills/skinning.cpp
		src/gameplay/skills/repair.cpp
		src/gameplay/skills/poisoning.cpp
		src/engine/ui/cmd/do_give.cpp
		src/engine/ui/cmd/do_trample.cpp
		src/gameplay/skills/campfire.cpp
		src/gameplay/skills/firstaid.cpp
		src/gameplay/skills/sharpening.cpp
		src/gameplay/skills/armoring.cpp
		src/engine/ui/cmd/do_get.cpp
        src/engine/ui/cmd/do_equip.cpp
		src/engine/ui/cmd/do_put.cpp
		src/engine/ui/cmd/do_remove.cpp
		src/engine/ui/cmd/do_eat.cpp
		src/engine/ui/cmd_god/do_inspect.cpp
		src/engine/ui/cmd_god/do_mark.cpp
		src/gameplay/crafting/fry.cpp
		src/engine/ui/cmd/do_refill.cpp
		src/gameplay/mechanics/guilds.cpp
		src/engine/ui/cmd/do_features.cpp
		src/engine/ui/cmd/do_spells.cpp
		src/engine/ui/cmd/do_skills.cpp
		src/gameplay/economics/currencies.cpp
		src/gameplay/skills/spell_capable.cpp
		src/gameplay/skills/lightwalk.cpp
		src/gameplay/skills/death_rage.cpp
		src/gameplay/skills/fit.cpp
		src/gameplay/abilities/talents_effects.cpp
		src/gameplay/abilities/feats_constants.cpp
		src/gameplay/abilities/abilities_items_set.cpp
		src/gameplay/abilities/talents_actions.cpp
		src/gameplay/skills/slay.cpp
		src/gameplay/skills/charge.cpp
		src/gameplay/skills/dazzle.cpp
		src/gameplay/communication/offtop.cpp
		src/gameplay/mechanics/dead_load.cpp
		src/gameplay/mechanics/city_guards.cpp
		src/engine/ui/cmd/do_remort.cpp
		src/gameplay/mechanics/dungeons.cpp
		src/engine/ui/cmd_god/do_tabulate.cpp
		src/engine/ui/cmd_god/do_reload.cpp
		src/gameplay/mechanics/mob_races.cpp
		src/engine/ui/cmd_god/do_set.cpp
		src/gameplay/mechanics/stable_objs.cpp
		src/gameplay/mechanics/treasure_cases.cpp
		src/gameplay/mechanics/cities.cpp
		src/gameplay/quests/bodrich_quests.cpp
		src/engine/ui/cmd/do_affects.cpp
		src/engine/ui/cmd/do_toggle.cpp
		src/engine/ui/cmd/do_mode.cpp
		src/engine/ui/cmd/do_levels.cpp
		src/engine/ui/cmd/do_where.cpp
		src/engine/ui/cmd/do_who.cpp
		src/engine/ui/cmd/do_enter.cpp
		src/engine/ui/cmd/do_sleep.cpp
		src/engine/ui/cmd/do_wake.cpp
        src/engine/ui/cmd/do_bandage.cpp
		src/engine/ui/cmd/do_consider.cpp
		src/engine/ui/cmd/do_exits.cpp
		src/gameplay/mechanics/sight.cpp
		src/engine/ui/cmd/do_look.cpp
		src/engine/ui/cmd/do_examine.cpp
		src/engine/ui/cmd/do_peer.cpp
		src/engine/ui/cmd/do_look_around.cpp
		src/engine/ui/cmd/do_inventory.cpp
		src/engine/ui/cmd/do_equipment.cpp
		src/engine/ui/cmd/do_listen.cpp
		src/engine/ui/cmd/do_zone.cpp
		src/gameplay/skills/dazzle.cpp
		src/gameplay/ai/mob_memory.cpp
		src/engine/network/logon.cpp
		src/engine/ui/cmd_god/do_ban.cpp
		src/engine/ui/cmd_god/do_proxy.cpp
		src/administration/proxy.cpp
		src/engine/ui/cmd_god/do_print_armor.cpp
		src/engine/ui/cmd_god/do_forcetime.cpp
		src/engine/ui/cmd_god/do_liblist.cpp
		src/engine/ui/cmd_god/do_wizutil.cpp
		src/administration/punishments.cpp
		src/engine/ui/cmd_god/do_wiznet.cpp
		src/engine/ui/cmd_god/do_force.cpp
		src/engine/ui/cmd_god/do_invisible.cpp
		src/engine/ui/cmd_god/do_set_all.cpp
        src/utils/grammar/cases.cpp
		src/engine/core/iosystem.cpp
		src/gameplay/mechanics/groups.cpp
		src/gameplay/statistics/money_drop.cpp
		src/gameplay/statistics/zone_exp.cpp
		src/gameplay/communication/talk.cpp
		src/engine/ui/cmd_god/do_echo.cpp
		src/gameplay/core/base_stats.cpp
		src/engine/ui/cmd/do_camouflage.cpp
		src/engine/ui/cmd/do_hide.cpp
		src/engine/ui/cmd/do_steal.cpp
		src/engine/ui/cmd/do_group.cpp
		src/engine/ui/cmd/do_ungroup.cpp
		src/engine/ui/cmd_god/do_beep.cpp
		src/engine/ui/cmd/do_pray.cpp
		src/engine/ui/cmd/do_recall.cpp
		src/engine/ui/cmd/do_display.cpp
		src/engine/ui/cmd/do_wimpy.cpp
		src/engine/ui/cmd/do_quit.cpp
		src/engine/ui/cmd/do_courage.cpp
		src/engine/ui/cmd/do_sneak.cpp
		src/engine/ui/cmd/do_visible.cpp
		src/engine/ui/cmd/do_save.cpp
		src/engine/ui/cmd/do_summon.cpp
		src/engine/ui/cmd/do_antigods.cpp
		src/gameplay/mechanics/illumination.cpp
		src/gameplay/mechanics/awake.cpp
		src/gameplay/statistics/char_stat.cpp
		src/engine/ui/cmd/do_mystat.cpp
		src/engine/ui/cmd/do_weather.cpp
		src/engine/ui/cmd/do_statistic.cpp
		src/engine/ui/cmd/do_time.cpp
		src/engine/ui/cmd/do_diagnose.cpp
		src/engine/ui/cmd/do_commands.cpp
		src/engine/ui/cmd/do_who_am_i.cpp
		src/engine/ui/cmd/do_generic_page.cpp
		src/engine/ui/cmd/do_gold.cpp
		src/engine/ui/cmd/do_check_invoice.cpp
		src/gameplay/communication/check_invoice.cpp
		src/engine/ui/cmd/do_quest.cpp
		src/engine/ui/cmd/do_write.cpp
		src/engine/ui/cmd/do_group_say.cpp
		src/engine/ui/cmd/do_say.cpp
		src/engine/ui/cmd/do_ignore.cpp
		src/engine/ui/cmd/do_reply.cpp
		src/engine/ui/cmd/do_tell.cpp
		src/engine/ui/cmd/do_offtop.cpp
		src/engine/ui/cmd/do_pray_gods.cpp
		src/engine/ui/cmd/do_gen_comm.cpp
		src/engine/ui/cmd/do_spec_comm.cpp
		src/engine/ui/cmd/do_mobshout.cpp
		src/engine/ui/cmd/do_page.cpp
		src/engine/ui/cmd/do_rest.cpp
		src/engine/ui/cmd/do_sit.cpp
		src/engine/ui/cmd/do_stand.cpp
		src/gameplay/mechanics/doors.cpp
		src/engine/ui/cmd/do_gen_door.cpp
		src/gameplay/skills/throwout.cpp
		src/gameplay/skills/frenzy.cpp
		src/engine/ui/cmd_god/do_zreset.cpp
		src/engine/ui/cmd_god/do_dc.cpp
		src/engine/ui/cmd_god/do_advance.cpp
		src/engine/ui/cmd_god/do_glory.cpp
		src/administration/karma.cpp
		src/engine/ui/cmd_god/do_arena_restore.cpp
		src/engine/ui/cmd_god/do_load.cpp
		src/engine/ui/cmd_god/do_overstuff.cpp
		src/engine/ui/cmd_god/do_zclear.cpp
		src/engine/ui/cmd_god/do_delete_obj.cpp
		src/engine/ui/cmd_god/do_show_zone_stat.cpp
		src/engine/ui/cmd_god/do_occupation.cpp
		src/engine/ui/cmd_god/do_send.cpp
		src/engine/ui/cmd_god/do_at_room.cpp
		src/engine/ui/cmd_god/do_unfreeze.cpp
		src/engine/ui/cmd_god/do_goto.cpp
		src/engine/ui/cmd_god/do_teleport.cpp
		src/engine/ui/cmd_god/do_snoop.cpp
		src/engine/ui/cmd_god/do_switch.cpp
		src/engine/ui/cmd_god/do_vstat.cpp
		src/engine/ui/cmd_god/do_purge.cpp
		src/engine/ui/cmd_god/do_syslog.cpp
		src/engine/ui/cmd_god/do_restore.cpp
		src/engine/ui/cmd_god/do_gecho.cpp
		src/engine/ui/cmd_god/do_poof_msg.cpp
		src/engine/ui/cmd_god/do_wizlock.cpp
		src/engine/ui/cmd_god/do_date.cpp
		src/engine/ui/cmd_god/do_last.cpp
		src/engine/ui/cmd_god/do_sdemigods.cpp
		src/engine/ui/cmd_god/do_spellstat.cpp
		src/gameplay/statistics/spell_usage.cpp
		src/engine/ui/cmd_god/do_loadstat.cpp
		src/engine/ui/cmd_god/do_sanitize.cpp
		src/engine/ui/cmd/do_alias.cpp
		src/engine/ui/cmd/do_drink.cpp
		src/engine/ui/cmd/do_drunkoff.cpp
		src/engine/ui/cmd/do_pour.cpp
		src/engine/ui/cmd/do_hidemove.cpp
		src/engine/ui/cmd/do_move.cpp
		src/gameplay/mechanics/hide.cpp
		src/gameplay/mechanics/boat.cpp
		src/gameplay/mechanics/minions.cpp
		src/gameplay/mechanics/armor.cpp
		src/gameplay/skills/leadership.cpp
		src/gameplay/skills/punctual_style.cpp
		src/gameplay/mechanics/equipment.cpp
		src/gameplay/skills/intercept.cpp
		src/gameplay/skills/deviate.cpp
		src/gameplay/skills/multyparry.cpp
		src/gameplay/mechanics/damage.cpp
		src/gameplay/mechanics/tutelar.cpp
		src/gameplay/skills/addshot.cpp
		src/engine/db/player_index.cpp
		src/gameplay/skills/addshot.cpp
		src/gameplay/classes/mob_classes_info.cpp
		src/gameplay/classes/recalc_mob_params_by_vnum.cpp)



set(HEADERS
		src/engine/ui/cmd/do_flee.h
		src/engine/ui/cmd/do_follow.h
		src/engine/ui/cmd/do_learn.h
		src/engine/ui/cmd/do_forget.h
		src/engine/ui/cmd/do_create.h
		src/engine/ui/cmd/do_memorize.h
		src/engine/ui/cmd_god/do_stat.h
		src/engine/ui/cmd_god/do_godtest.h
		src/engine/ui/cmd/do_telegram.h
		src/engine/ui/cmd/do_hire.h
		src/engine/ui/cmd/do_mixture.h
		src/engine/ui/cmd/do_cast.h
		src/engine/ui/cmd/do_employ.h
		src/gameplay/affects/affect_data.h
		src/gameplay/skills/track.h
		src/gameplay/fight/common.h
		src/gameplay/fight/assist.h
		src/engine/ui/cmd/do_kill.h
		src/engine/ui/cmd/do_retreat.h
		src/engine/ui/cmd/do_order.h
		src/engine/ui/cmd/do_mercenary.h
		src/engine/ui/cmd/do_style.h
		src/gameplay/skills/manadrain.h
		src/gameplay/skills/townportal.h
		src/gameplay/skills/turnundead.h
		src/gameplay/skills/mighthit.h
		src/gameplay/skills/expendientcut.h
		src/gameplay/skills/shield_block.h
		src/gameplay/skills/protect.h
		src/gameplay/skills/parry.h
		src/gameplay/skills/pick.h
		src/gameplay/skills/throw.h
		src/gameplay/skills/ironwind.h
		src/gameplay/skills/disarm.h
		src/gameplay/skills/warcry.h
		src/gameplay/skills/identify.h
		src/gameplay/skills/relocate.h
		src/engine/core/char_movement.h
		src/gameplay/mechanics/strengthening.h
		src/gameplay/abilities/abilities_constants.h
		src/gameplay/abilities/abilities_rollsystem.h
		src/gameplay/mechanics/mount.h
		src/administration/accounts.h
		src/engine/core/action_targeting.h
		src/engine/structs/blocking_queue.h
		src/engine/db/influxdb.h
		src/engine/core/heartbeat_commands.h
		src/gameplay/mechanics/weather.h
		src/gameplay/core/game_limits.h
		src/engine/core/heartbeat.h
		src/engine/db/world_characters.h
		src/gameplay/economics/shops_implementation.h
		src/engine/db/global_objects.h
		src/engine/db/utils_find_obj_id_by_vnum.h
		src/utils/id_converter.h
		src/engine/db/utils_find_obj_id_by_vnum.h
		src/engine/db/id.h
		src/engine/structs/compact_trie.h
		src/engine/structs/radix_trie.h
		src/engine/db/obj_prototypes.h
		src/engine/db/world_objects.h
		src/utils/utils_string.h
		src/gameplay/affects/affect_handler.h
		src/gameplay/economics/auction.h
		src/utils/backtrace.h
		src/administration/ban.h
		src/gameplay/mechanics/birthplaces.h
        src/gameplay/communication/boards/boards_changelog_loaders.h
        src/gameplay/communication/boards/boards_constants.h
        src/gameplay/communication/boards/boards_formatters.h
        src/gameplay/communication/boards/boards.h
        src/gameplay/communication/boards/boards_message.h
        src/gameplay/communication/boards/boards_types.h
		src/gameplay/mechanics/bonus.h
		src/gameplay/mechanics/bonus_command_parser.h
		src/gameplay/mechanics/bonus_types.h
		src/engine/boot/boot_constants.h
		src/engine/boot/boot_data_files.h
		src/engine/boot/boot_index.h
		src/utils/cache.h
		src/gameplay/mechanics/celebrates.h
		src/engine/entities/char_data.h
		src/engine/core/utils_char_obj.inl
		src/engine/entities/char_player.h
		src/gameplay/classes/pc_classes.h
		src/gameplay/classes/classes_spell_slots.h
		src/engine/core/comm.h
		src/engine/ui/cmd_god/do_shutdown.h
		src/engine/ui/commands.h
		src/engine/core/conf.h
		src/engine/core/config.h
		src/gameplay/core/constants.h
		src/utils/coredump.h
		src/gameplay/mechanics/corpse.h
		src/gameplay/quests/daily_quest.h
		src/gameplay/quests/quest.h
		src/engine/db/db.h
		src/gameplay/mechanics/deathtrap.h
		src/gameplay/mechanics/depot.h
		src/engine/db/description.h
		src/engine/scripting/dg_db_scripts.h
		src/engine/scripting/dg_domination_helper.h
		src/engine/scripting/dg_event.h
		src/engine/scripting/dg_olc.h
		src/engine/scripting/dg_scripts.h
		src/gameplay/economics/dictionary.h
		src/utils/diskio.h
		src/engine/structs/structs_double_map.h
		src/gameplay/statistics/dps.h
		src/gameplay/economics/exchange.h
		src/gameplay/economics/ext_money.h
		src/engine/core/external_trigger.h
		src/gameplay/abilities/abilities_items_set.h
		src/gameplay/abilities/feats.h
		src/gameplay/fight/fight_constants.h
		src/gameplay/fight/fight.h
		src/gameplay/fight/fight_hit.h
		src/gameplay/fight/fight_stuff.h
		src/gameplay/fight/fight_penalties.h
		src/utils/file_crc.h
		src/gameplay/core/genchar.h
		src/gameplay/mechanics/glory_const.h
		src/gameplay/mechanics/glory.h
		src/gameplay/mechanics/glory_misc.h
		src/engine/core/handler.h
		src/engine/db/help.h
		src/gameplay/clans/house_exp.h
		src/gameplay/clans/house.h
		src/gameplay/communication/ignores.h
		src/gameplay/communication/ignores_loader.h
		src/gameplay/crafting/im.h
		src/engine/ui/interpreter.h
		src/gameplay/crafting/item_creation.h
		src/utils/levenshtein.h
		src/gameplay/mechanics/liquid.h
		src/utils/logger.h
		src/gameplay/magic/magic.h
		src/gameplay/magic/magic_items.h
		src/gameplay/magic/magic_rooms.h
		src/gameplay/magic/magic_utils.h
		src/gameplay/magic/spells.h
		src/gameplay/magic/spells_info.h
		src/gameplay/magic/magic_temp_spells.h
		src/gameplay/communication/mail.h
		src/engine/ui/mapsystem.h
		src/gameplay/mechanics/meat_maker.h
		src/gameplay/ai/mobact.h
		src/gameplay/mechanics/mobmax.h
		src/gameplay/statistics/mob_stat.h
		src/engine/ui/modify.h
        src/administration/name_adviser.h
		src/gameplay/mechanics/named_stuff.h
		src/gameplay/mechanics/noob.h
		src/gameplay/mechanics/obj_enchant.h
		src/engine/entities/obj_data.h
		src/engine/db/obj_save.h
		src/gameplay/mechanics/obj_sets.h
		src/gameplay/mechanics/obj_sets_stuff.h
		src/engine/olc/olc.h
		src/gameplay/communication/parcel.h
		src/utils/parse.h
		src/administration/password.h
		src/gameplay/fight/pk.h
		src/engine/entities/player_i.h
		src/gameplay/mechanics/player_races.h
		src/administration/privilege.h
        src/third_party_libs/pugixml/pugiconfig.h
        src/third_party_libs/pugixml/pugixml.h
		src/gameplay/quests/quested.h
		src/utils/random.h
		src/gameplay/communication/remember.h
		src/gameplay/core/reset_stats.h
		src/engine/entities/room_data.h
		src/engine/ui/color.h
		src/gameplay/mechanics/sets_drop.h
		src/gameplay/economics/shop_ext.h
		src/administration/shutdown_parameters.h
		src/gameplay/skills/skills.h
		src/gameplay/skills/skills_info.h
		src/gameplay/communication/spam.h
		src/engine/network/descriptor_data.h
		src/engine/structs/structs.h
		src/gameplay/mechanics/stuff.h
		src/engine/core/sysdep.h
		src/engine/network/telnet.h
		src/utils/utils_time.h
		src/gameplay/mechanics/title.h
		src/gameplay/statistics/top.h
		src/utils/utils.h
		src/utils/xml_loading_helper.h
		src/utils/utils_debug.h
		src/engine/entities/zone.h
		src/gameplay/skills/backstab.h
		src/gameplay/skills/bash.h
		src/gameplay/skills/stun.h
		src/gameplay/mechanics/poison.h
		src/gameplay/skills/resque.h
		src/gameplay/skills/kick.h
		src/gameplay/skills/strangle.h
		src/gameplay/skills/chopoff.h
		src/gameplay/skills/overhelm.h
		src/gameplay/ai/graph.h
		src/engine/structs/flag_data.h
		src/engine/entities/entities_constants.h
		src/gameplay/crafting/jewelry.h
		src/gameplay/crafting/mining.h
		src/gameplay/affects/affect_contants.h
		src/engine/structs/extra_description.h
		src/gameplay/communication/insult.h
		src/gameplay/communication/social.h
		src/gameplay/abilities/abilities_info.h
		src/gameplay/classes/pc_classes_info.h
		src/engine/ui/objects_filter.h
		src/engine/structs/info_container.h
		src/utils/parser_wrapper.h
		src/engine/boot/cfg_manager.h
		src/engine/structs/iterators.h
		src/engine/ui/cmd_god/do_users.h
		src/engine/ui/table_wrapper.h
		src/engine/ui/cmd/do_score.h
		src/gameplay/magic/spells_constants.h
		src/gameplay/mechanics/mem_queue.h
		src/engine/ui/cmd_god/do_show.h
		src/engine/ui/cmd/do_drop.h
		src/gameplay/skills/skinning.h
		src/gameplay/skills/repair.h
		src/gameplay/skills/poisoning.h
		src/engine/ui/cmd/do_give.h
		src/engine/ui/cmd/do_trample.h
		src/gameplay/skills/campfire.h
		src/gameplay/skills/firstaid.h
		src/gameplay/skills/sharpening.h
		src/gameplay/skills/armoring.h
		src/engine/ui/cmd/do_get.h
        src/engine/ui/cmd/do_equip.h
		src/engine/ui/cmd/do_put.h
		src/engine/ui/cmd/do_remove.h
		src/engine/ui/cmd/do_eat.h
		src/engine/ui/cmd_god/do_inspect.h
		src/engine/ui/cmd_god/do_mark.h
		src/gameplay/crafting/fry.h
		src/engine/ui/cmd/do_refill.h
		src/engine/ui/cmd/do_sign.h
		src/gameplay/mechanics/guilds.h
		src/engine/ui/cmd/do_features.h
		src/engine/ui/cmd/do_spells.h
		src/engine/ui/cmd/do_skills.h
		src/gameplay/economics/currencies.h
		src/gameplay/skills/spell_capable.h
		src/gameplay/skills/lightwalk.h
		src/gameplay/skills/death_rage.h
		src/gameplay/skills/fit.h
		src/gameplay/abilities/talents_effects.h
		src/gameplay/abilities/feats_constants.h
		src/gameplay/abilities/talents_actions.h
		src/gameplay/skills/slay.h
		src/gameplay/skills/charge.h
		src/gameplay/skills/dazzle.h
		src/gameplay/communication/offtop.h
		src/gameplay/mechanics/dead_load.h
		src/gameplay/mechanics/city_guards.h
		src/engine/ui/cmd/do_remort.h
		src/gameplay/mechanics/dungeons.h
		src/engine/ui/cmd_god/do_tabulate.h
		src/engine/ui/cmd_god/do_reload.h
		src/gameplay/mechanics/mob_races.h
		src/engine/ui/cmd_god/do_set.h
		src/gameplay/mechanics/stable_objs.h
		src/gameplay/mechanics/treasure_cases.h
		src/gameplay/mechanics/cities.h
		src/gameplay/quests/bodrich_quests.h
		src/engine/ui/cmd/do_affects.h
		src/engine/ui/cmd/do_toggle.h
		src/engine/ui/cmd/do_mode.h
		src/engine/ui/cmd/do_levels.h
		src/engine/ui/cmd/do_where.h
		src/engine/ui/cmd/do_who.h
		src/engine/ui/cmd/do_wake.h
		src/engine/ui/cmd/do_enter.h
		src/engine/ui/cmd/do_sleep.h
        src/engine/ui/cmd/do_bandage.h
		src/engine/ui/cmd/do_consider.h
		src/engine/ui/cmd/do_exits.h
		src/gameplay/mechanics/sight.h
		src/engine/ui/cmd/do_look.h
		src/engine/ui/cmd/do_examine.h
		src/engine/ui/cmd/do_peer.h
		src/engine/ui/cmd/do_look_around.h
		src/engine/ui/cmd/do_inventory.h
		src/engine/ui/cmd/do_equipment.h
		src/engine/ui/cmd/do_listen.h
		src/engine/ui/cmd/do_zone.h
		src/gameplay/ai/mob_memory.h
		src/gameplay/skills/dazzle.h
		src/engine/network/logon.h
		src/engine/ui/cmd_god/do_ban.h
		src/engine/ui/cmd_god/do_proxy.h
		src/administration/proxy.h
		src/engine/ui/cmd_god/do_print_armor.h
		src/engine/ui/cmd_god/do_forcetime.h
		src/engine/ui/cmd_god/do_liblist.h
		src/engine/ui/cmd_god/do_wizutil.h
		src/administration/punishments.h
		src/engine/ui/cmd_god/do_wiznet.h
		src/engine/ui/cmd_god/do_force.h
		src/engine/ui/cmd_god/do_invisible.h
		src/engine/ui/cmd_god/do_set_all.h
		src/gameplay/ai/spec_procs.h
        src/utils/grammar/cases.h
		src/engine/structs/meta_enum.h
		src/engine/core/iosystem.h
		src/gameplay/mechanics/groups.h
		src/gameplay/statistics/money_drop.h
		src/gameplay/statistics/zone_exp.h
		src/gameplay/communication/talk.h
		src/engine/ui/cmd_god/do_echo.h
		src/gameplay/core/base_stats.h
		src/engine/ui/cmd/do_camouflage.h
		src/engine/ui/cmd/do_hide.h
		src/engine/ui/cmd/do_steal.h
		src/engine/ui/cmd/do_group.h
		src/engine/ui/cmd/do_ungroup.h
		src/engine/ui/cmd_god/do_beep.h
		src/engine/ui/cmd/do_pray.h
		src/engine/ui/cmd/do_recall.h
		src/engine/ui/cmd/do_display.h
		src/engine/ui/cmd/do_wimpy.h
		src/engine/ui/cmd/do_quit.h
		src/engine/ui/cmd/do_courage.h
		src/engine/ui/cmd/do_sneak.h
		src/engine/ui/cmd/do_visible.h
		src/engine/ui/cmd/do_save.h
		src/engine/ui/alias.h
		src/engine/ui/cmd/do_summon.h
		src/engine/ui/cmd/do_antigods.h
		src/gameplay/mechanics/illumination.h
		src/gameplay/mechanics/awake.h
		src/gameplay/statistics/char_stat.h
		src/engine/ui/cmd/do_mystat.h
		src/engine/ui/cmd/do_weather.h
		src/engine/ui/cmd/do_statistic.h
		src/engine/ui/cmd/do_time.h
		src/engine/ui/cmd/do_diagnose.h
		src/engine/ui/cmd/do_commands.h
		src/engine/ui/cmd/do_who_am_i.h
		src/engine/ui/cmd/do_generic_page.h
		src/engine/ui/cmd/do_gold.h
		src/engine/ui/cmd/do_check_invoice.h
		src/gameplay/communication/check_invoice.h
		src/engine/ui/cmd/do_quest.h
		src/engine/ui/cmd/do_write.h
		src/engine/ui/cmd/do_group_say.h
		src/engine/ui/cmd/do_say.h
		src/engine/ui/cmd/do_ignore.h
		src/engine/ui/cmd/do_reply.h
		src/engine/ui/cmd/do_tell.h
		src/engine/ui/cmd/do_offtop.h
		src/engine/ui/cmd/do_pray_gods.h
		src/engine/ui/cmd/do_gen_comm.h
		src/engine/ui/cmd/do_spec_comm.h
		src/engine/ui/cmd/do_mobshout.h
		src/engine/ui/cmd/do_page.h
		src/engine/ui/cmd/do_rest.h
		src/engine/ui/cmd/do_sit.h
		src/engine/ui/cmd/do_stand.h
		src/gameplay/mechanics/doors.h
		src/engine/ui/cmd/do_gen_door.h
		src/gameplay/skills/throwout.h
		src/gameplay/skills/frenzy.h
		src/engine/ui/cmd_god/do_zreset.h
		src/engine/ui/cmd_god/do_dc.h
		src/engine/ui/cmd_god/do_advance.h
		src/engine/ui/cmd_god/do_glory.h
		src/administration/karma.h
		src/engine/ui/cmd_god/do_arena_restore.h
		src/engine/ui/cmd_god/do_load.h
		src/engine/ui/cmd_god/do_overstuff.h
		src/engine/ui/cmd_god/do_zclear.h
        src/engine/ui/cmd_god/do_delete_obj.h
		src/engine/ui/cmd_god/do_show_zone_stat.h
		src/engine/ui/cmd_god/do_occupation.h
		src/engine/ui/cmd_god/do_send.h
		src/engine/ui/cmd_god/do_at_room.h
		src/engine/ui/cmd_god/do_unfreeze.h
		src/engine/ui/cmd_god/do_goto.h
		src/engine/ui/cmd_god/do_teleport.h
		src/engine/ui/cmd_god/do_snoop.h
		src/engine/ui/cmd_god/do_switch.h
		src/engine/ui/cmd_god/do_vstat.h
		src/engine/ui/cmd_god/do_purge.h
		src/engine/ui/cmd_god/do_syslog.h
		src/engine/ui/cmd_god/do_restore.h
		src/engine/ui/cmd_god/do_gecho.h
		src/engine/ui/cmd_god/do_poof_msg.h
		src/engine/ui/cmd_god/do_wizlock.h
		src/engine/ui/cmd_god/do_date.h
		src/engine/ui/cmd_god/do_last.h
		src/engine/ui/cmd_god/do_sdemigods.h
		src/engine/ui/cmd_god/do_spellstat.h
		src/gameplay/statistics/spell_usage.h
		src/engine/ui/cmd_god/do_loadstat.h
		src/engine/ui/cmd_god/do_sanitize.h
		src/engine/ui/cmd/do_alias.h
		src/engine/ui/cmd/do_drink.h
		src/engine/ui/cmd/do_drunkoff.h
		src/engine/ui/cmd/do_pour.h
		src/engine/ui/cmd/do_hidemove.h
		src/engine/ui/cmd/do_move.h
		src/gameplay/mechanics/hide.h
		src/gameplay/mechanics/boat.h
		src/gameplay/mechanics/minions.h
		src/gameplay/mechanics/armor.h
		src/gameplay/skills/leadership.h
		src/gameplay/skills/punctual_style.h
		src/gameplay/mechanics/equipment.h
		src/gameplay/skills/intercept.h
		src/gameplay/skills/deviate.h
		src/gameplay/skills/multyparry.h
		src/gameplay/mechanics/damage.h
		src/gameplay/mechanics/tutelar.h
		src/gameplay/skills/addshot.h
		src/engine/db/player_index.h
		src/gameplay/classes/recalc_mob_params_by_vnum.h)

# Build types
set(CMAKE_CONFIGURATION_TYPES Release Debug Test FastTest)
if (NOT MSVC)
	# Default build type will be Release.
	if (NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
		set(DEFAULT_CMAKE_BUILD_TYPE "Release")
		message(STATUS "Setting build type by default: ${DEFAULT_CMAKE_BUILD_TYPE}")
		set(CMAKE_BUILD_TYPE ${DEFAULT_CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build: Debug, Release, Test, FastTest" FORCE)
	else ()
		message(STATUS "Using build type \"${CMAKE_BUILD_TYPE}\"")
	endif()
endif ()

# Default flags
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_TEST "")
set(CMAKE_CXX_FLAGS_FASTTEST "")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
set(CMAKE_EXE_LINKER_FLAGS_TEST "")
set(CMAKE_EXE_LINKER_FLAGS_FASTTEST "")

## Look for required components
# Python
# You can specify preferred Python version using cmake command line parameter. Like this:
#     $ cmake -DPY_VERSION=3.4 ..
if (UNIX)
	set(DEFAULT_PYTHON_VERSION "3.4" CACHE STRING "Default value for PY_VERSION")
else ()
	set(DEFAULT_PYTHON_VERSION NO CACHE STRING "Default value for PY_VERSION")
endif ()
set(PY_VERSION ${DEFAULT_PYTHON_VERSION} CACHE STRING "Preferred Python version")
if (PY_VERSION)
	set(Python_ADDITIONAL_VERSIONS ${PY_VERSION})
endif ()

set(SCRIPTING_HEADERS src/engine/scripting/scripting.h)
set(SCRIPTING_SOURCES src/engine/scripting/scripting.cpp)

option(TIMED_SCRIPT "Turns on/off definition TIMED_SCRIPT" NO)
set(TIMED_SCRIPT_DEFINITION "")
if (TIMED_SCRIPT)
	set(TIMED_SCRIPT_DEFINITION "-DTIMED_SCRIPT")
endif ()
add_definitions(${TIMED_SCRIPT_DEFINITION})

# Turns on/off scripting. Use the option like:
#     $ cmake -DSCRIPTING=ON..
# to turn scripting ON.
option(SCRIPTING "Turns on/off scripting engine" NO)
if (SCRIPTING)
	message(STATUS "Turning scripting ON")
	list(APPEND HEADERS ${SCRIPTING_HEADERS})
	list(APPEND SOURCES ${SCRIPTING_SOURCES})
	source_group("Scripting\\Headers" FILES ${SCRIPTING_HEADERS})
	source_group("Scripting\\Sources" FILES ${SCRIPTING_SOURCES})
	add_definitions(-DWITH_SCRIPTING)

	find_package(PythonLibs)
	if (NOT PYTHONLIBS_FOUND)
		message(FATAL_ERROR "Python not found.")
	endif ()

	if (PYTHONLIBS_VERSION_STRING STRLESS "3.0.0")
		message(FATAL_ERROR "Python of version 3 or later has not been found (found ${PYTHONLIBS_VERSION_STRING})")
	endif ()
	message(STATUS "Python libs version string is '${PYTHONLIBS_VERSION_STRING}'")

	if (NOT CYGWIN)
		if (UNIX)
			set(DEFAULT_EXPLICIT_BP_VERSION YES)
		else ()
			set(DEFAULT_EXPLICIT_BP_VERSION NO)
		endif ()
		# Should we specify version of Boost python explicitly or not. Yes by default on Unix platforms.
		option(EXPLICIT_BP_VERSION "Try to find boost python library with explicit version suffix" ${DEFAULT_EXPLICIT_BP_VERSION})
		if (EXPLICIT_BP_VERSION AND PYTHONLIBS_VERSION_STRING)
			string(REGEX REPLACE "^([0-9])\\.([0-9]).*$" "-py\\1\\2" BOOSTPYTHON_LIBS_SUFFIX ${PYTHONLIBS_VERSION_STRING})
			message(STATUS "Using explicit boostpython library suffix '${BOOSTPYTHON_LIBS_SUFFIX}'")
		else ()
			set(BOOSTPYTHON_LIBS_SUFFIX "")
		endif ()
	else ()
		set(BOOSTPYTHON_LIBS_SUFFIX "3")
	endif ()

	set(BOOST_PYTHON_MODULE python${BOOSTPYTHON_LIBS_SUFFIX})
else()
	message(STATUS "Turning scripting OFF")
	set(BOOST_PYTHON_MODULE)
	set(PYTHON_LIBRARIES)
endif ()

# MSDP
set(MSDP_FILES
		src/engine/network/msdp/msdp.h
		src/engine/network/msdp/msdp.cpp
		src/engine/network/msdp/msdp_parser.cpp
		src/engine/network/msdp/msdp_parser.h
		src/engine/network/msdp/msdp_constants.h
		src/engine/network/msdp/msdp_constants.cpp
		src/engine/network/msdp/msdp_reporters.cpp
		src/engine/network/msdp/msdp_reporters.h
		src/engine/network/msdp/msdp_senders.cpp
		src/engine/network/msdp/msdp_senders.h)
list(APPEND SOURCES ${MSDP_FILES})

# Versioning
# Try to find git executable, retrive revision if found and generate version.cpp from template
set(VERSIONING_FILES src/versioning.cmake src/version.cpp.in)
find_program(GIT git DOC "Git executable file")
# invalidate template
add_custom_target(versioning ALL COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_SOURCE_DIR}/src/version.cpp.in
	SOURCES ${VERSIONING_FILES})
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/src/version.cpp
	COMMAND ${CMAKE_COMMAND} -DBUILD_TYPE=${CMAKE_BUILD_TYPE} -DVERSION_INPUT=${CMAKE_SOURCE_DIR}/src/version.cpp.in -DVERSION_OUTPUT=${CMAKE_SOURCE_DIR}/src/version.cpp -DGIT=${GIT} -P ${CMAKE_SOURCE_DIR}/src/versioning.cmake
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS ${CMAKE_SOURCE_DIR}/src/version.cpp.in)
set_target_properties(versioning PROPERTIES FOLDER "Utility targets")

set(CRAFT_TAILORING_FILES
	lib/misc/craft/tailoring/index.xml)
set(CRAFT_GENERAL_FILES
	lib/misc/craft/index.xml)
set(CRAFT_MATERIAL_FILES
	lib/misc/craft/tough_leather.xml
	lib/misc/craft/soft_leather.xml
	lib/misc/craft/inwrought_flax.xml)
set(CRAFT_RECIPE_FILES
	lib/misc/craft/recipe1.xml)
set(CRAFT_CRAFT_FILES
	lib/misc/craft/craft1.xml)
set(CRAFT_TEMPLATE_FILES
	lib/misc/craft/material_template.xml
	lib/misc/craft/prototype_template.xml
	lib/misc/craft/recipe_template.xml)
set(CRAFT_PROTOTYPES)
set(CRAFT_SOURCE_FILES
		src/gameplay/crafting/craft_commands.cpp
		src/gameplay/crafting/craft.cpp
		src/gameplay/crafting/craft_logger.cpp
		src/gameplay/crafting/craft_static.cpp)
set(CRAFT_HEADER_FILES
		src/gameplay/crafting/craft_commands.h
		src/gameplay/crafting/craft.h
		src/gameplay/crafting/craft_logger.h
		src/gameplay/crafting/craft_static.h)

source_group("Craft\\Configuration" FILES ${CRAFT_GENERAL_FILES})
source_group("Craft\\Configuration\\Tailoring" FILES ${CRAFT_TAILORING_FILES})
source_group("Craft\\Configuration\\Templates" FILES ${CRAFT_TEMPLATE_FILES})
source_group("Craft\\Configuration\\Materials" FILES ${CRAFT_MATERIAL_FILES})
source_group("Craft\\Configuration\\Recipes" FILES ${CRAFT_RECIPE_FILES})
source_group("Craft\\Configuration\\Crafts" FILES ${CRAFT_CRAFT_FILES})
source_group("Craft\\Source files" FILES ${CRAFT_SOURCE_FILES})
source_group("Craft\\Header files" FILES ${CRAFT_HEADER_FILES})

set(CRAFT_FILES
	${CRAFT_GENERAL_FILES}
	${CRAFT_TAILORING_FILES}
	${CRAFT_MATERIAL_FILES}
	${CRAFT_RECIPE_FILES}
	${CRAFT_CRAFT_FILES}
	${CRAFT_TEMPLATE_FILES}
	${CRAFT_SOURCE_FILES}
	${CRAFT_HEADER_FILES})

set(MISC_FILES
	lib/misc/noob_help.xml
	lib/misc/configuration.xml
	lib/misc/grouping)

source_group("Misc" FILES ${MISC_FILES})

set(CIRCLE_FILES ${SOURCES} ${HEADERS} readme.markdown CONTRIBUTING.md ${CRAFT_FILES} ${MISC_FILES} )

# Sort source and header files. Just to convenience.
list(SORT CIRCLE_FILES)

# Create changelog file
add_custom_target(changelog ALL
	COMMAND ${GIT} log --date=local --reverse > changelog
	COMMENT "Writing changelog...")
set_target_properties(changelog PROPERTIES FOLDER "Utility targets")

# Main library file.
add_library(circle.library STATIC ${CIRCLE_FILES})
if (UNIX AND NOT CYGWIN)
	add_definitions("-std=c++20")
elseif (CYGWIN)
	add_definitions("-std=gnu++2a")
else ()
	set_target_properties(circle.library PROPERTIES CXX_STANDARD 20)
endif ()
add_dependencies(circle.library versioning changelog)

# Linfort integration
# Disable building tests and examples in libfort project
set(FORT_ENABLE_TESTING OFF CACHE INTERNAL "")
add_subdirectory(src/third_party_libs/libfort)
target_link_libraries(circle.library fort)

#fmt inegration
add_subdirectory(src/third_party_libs/fmt EXCLUDE_FROM_ALL)
target_link_libraries(circle.library fmt::fmt)

# Boost
option(BOOST "Turns on/off scripting engine" NO)
if (BOOST)
	message(STATUS "Turning BOOST ON")
	set(BOOST_MODULES ${BOOST_PYTHON_MODULE} system filesystem)
	if (WIN32)
		list(APPEND BOOST_MODULES locale)
	elseif (UNIX)
		if (NOT DEFINED ENV{BOOST_ROOT} AND NOT DEFINED ENV{BOOST_INCLUDEDIR})
			set(BOOST_INCLUDEDIR "/usr/include")
			message(STATUS "Trying to use BOOST_INCLUDEDIR with value \"${BOOST_INCLUDEDIR}\"")
		endif()
		if (NOT DEFINED ENV{BOOST_ROOT} AND NOT DEFINED ENV{BOOST_LIBRARYDIR})
			set(BOOST_LIBRARYDIR "/usr/lib/x86_64-linux-gnu")
			message(STATUS "Trying to use BOOST_LIBRARYDIR with value \"${BOOST_LIBRARYDIR}\"")
		endif()
	endif ()
	if (BOOST_NO_SYSTEM_PATHS)
		set(Boost_NO_SYSTEM_PATHS ON)
	endif ()
	if (NOT CYGWIN)
		add_definitions("-DBOOST_PYTHON_STATIC_LIB")
		set(Boost_USE_STATIC_LIBS ON)
	endif ()
	if (DEFINED ENV{DEBUG_BOOST_FIND})
		set(Boost_DEBUG ON)
	endif ()
	# In case of you have more than one Visual Studio and want to use specific version, uncomment the following three lines and specify compiler you want
	if (CMAKE_HOST_WIN32)
		set(BOOST_REQUIRED_COMPILER "" CACHE STRING "Compiler which must be used to compile Boost libraries (for example: -vc141 for Visual Studio 15 (2017))")
		if (BOOST_REQUIRED_COMPILER)
			set(Boost_COMPILER "${BOOST_REQUIRED_COMPILER}")
		else ()
			set(Boost_COMPILER "-vc141")
		endif ()
		target_compile_options(circle.library PUBLIC /MP /MDd)
		target_compile_options(circle.library PUBLIC $<$<CONFIG:Debug>:/Zi /Od>)
		target_compile_options(circle.library PUBLIC $<$<CONFIG:Release>:/O2>)
		target_compile_options(circle.library PUBLIC $<$<CONFIG:Test>:/Zi /Od>)
		target_compile_options(circle.library PUBLIC $<$<CONFIG:FastTest>:/O2>)
	endif ()

	set(BOOST_REQUIRED_VERSION OFF CACHE STRING "Optional version for Boost (for example, 1.54)")
	set(BOOST_REQUIRED_VERSION_VALUE "")
	if (BOOST_REQUIRED_VERSION)
		set(BOOST_REQUIRED_VERSION_VALUE "${BOOST_REQUIRED_VERSION}")
	endif ()
	find_package(Boost ${BOOST_REQUIRED_VERSION_VALUE} REQUIRED COMPONENTS ${BOOST_MODULES})
else()
	message(STATUS "Turning BOOST OFF")
endif ()

# ZLib
if (CMAKE_HOST_WIN32)
	set(DEFAULT_HAVE_ZLIB "NO")
else ()
	set(DEFAULT_HAVE_ZLIB "YES")
endif ()
option(HAVE_ZLIB "Should ZLib be compiled in. It is required to support MCCP." ${DEFAULT_HAVE_ZLIB})

if (HAVE_ZLIB)
	set(ZLIB_ROOT $ENV{ZLIB_ROOT})
	find_package(ZLIB REQUIRED)
	add_definitions(-DHAVE_ZLIB)
	include_directories(system ${ZLIB_INCLUDE_DIRS})
	target_link_libraries(circle.library ${ZLIB_LIBRARIES})
	message(STATUS "ZLib is turned ON. Circle will be compiled with MCCP support.")
else ()
	message(STATUS "ZLib is turned off. Circle will NOT support MCCP.")
endif ()

# Iconv
option(HAVE_ICONV "Allows to enable search of iconv." OFF)
if (HAVE_ICONV)
	find_path(ICONV_INCLUDE_DIR iconv.h)
	find_library(ICONV_LIBRARIES NAMES iconv libiconv libiconv-2 c)
	if (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
		message(STATUS "Iconv found. Turning it ON")
		set(ICONV_FOUND TRUE)
	else ()
		message(STATUS "Iconv not found. Turning it OFF")
	endif ()

	if (ICONV_FOUND)
		add_definitions("-DHAVE_ICONV")
		include_directories(system ${ICONV_INCLUDE_DIR})
		target_link_libraries(circle.library ${ICONV_LIBRARIES})
	endif ()
endif ()

# Telegram integration
if (CMAKE_HOST_WIN32)
	set(DEFAULT_TG "NO")
else ()
	set(DEFAULT_TG "YES")
endif ()
option(HAVE_TG "SSL and CURL should be compiled, only LINUX. It is required to support Telegram." ${DEFAULT_TG})
if (HAVE_TG)
#	find_package(Threads REQUIRED)
#	if (NOT Threads_FOUND)
#		set(HAVE_TG "NO")
#	endif()
	find_package(OpenSSL REQUIRED)
	if (NOT OPENSSL_FOUND)
		set(HAVE_TG "NO")
	endif()
	find_package(CURL)
	if (NOT CURL_FOUND)
		set(HAVE_TG "NO")
	endif()
	if (HAVE_TG)
		add_definitions("-DHAVE_TG")
#		set(Boost_USE_MULTITHREADED ON)
#		${CMAKE_THREAD_LIBS_INIT}
		target_link_libraries(circle.library ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARIES})
		message(STATUS "SSL IS ON, Telegram support enabled")
	else ()
		message(STATUS "Telegram support disabled, no dependant libs found")
	endif ()
endif ()

include_directories(src
	system ${Boost_INCLUDE_DIRS}
	system ${PYTHON_INCLUDE_DIRS})
target_link_libraries(circle.library
	${Boost_LIBRARIES}
	${PYTHON_LIBRARIES})

# Prepare options for test build
set(TESTBUILD_DEFINITIONS "-DNOCRYPT -DTEST_BUILD")

set(ASAN_FLAGS)
if (UNIX AND NOT CYGWIN)
	set(DEFAULT_WITH_ASAN YES)
elseif ()
	set(DEFAULT_WITH_ASAN NO)
endif ()
option(WITH_ASAN "Compile with ASAN" ${DEFAULT_WITH_ASAN})

if (WITH_ASAN)
	message(STATUS "Turning compiling with ASAN in debug mode ON")
	set(ASAN_FLAGS "-fsanitize=address -static-libasan")
else ()
	message(STATUS "Turning compiling with ASAN in debug mode OFF")
endif ()

option(NOCRYPT "Allows to turn off usage of crypt() function." OFF)
set(DEBUG_CRYPT "")
if (APPLE)
  set(DEBUG_CRYPT "-DNOCRYPT")
  message(STATUS "Turning OFF crypt")
elseif (NOCRYPT)
  set(DEBUG_CRYPT "-DNOCRYPT")
  message(STATUS "Turning OFF crypt")
else ()
  message(STATUS "Turning ON crypt")
endif ()

if (CMAKE_HOST_UNIX)
	if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
		set(CMAKE_CXX_FLAGS_RELEASE  "-ggdb3 -O0 -rdynamic -Wall -Wextra -Wno-format-truncation ${DEBUG_CRYPT}")
		set(CMAKE_CXX_FLAGS_DEBUG    "-ggdb3 -O0 -rdynamic -Wall -Wextra -Wno-format-truncation -D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC ${ASAN_FLAGS} ${DEBUG_CRYPT}")
		set(CMAKE_CXX_FLAGS_TEST     "-O3 -rdynamic -Wall -Wextra -Wno-format-truncation ${TESTBUILD_DEFINITIONS} -DLOG_AUTOFLUSH")
		set(CMAKE_CXX_FLAGS_FASTTEST "-Ofast -Wall -Wextra -Wno-format-truncation ${TESTBUILD_DEFINITIONS}")

		if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
			add_definitions("-Wno-invalid-source-encoding")
		endif ()
		if(NOT DEBUG_CRYPT)
			target_link_libraries(circle.library crypt)
		endif()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()

	# Suppose that UNIX hosts support epoll by default EPOLL>>>
	if (NOT CYGWIN AND NOT APPLE)
		option(HAS_EPOLL "Does system support epoll call?" ON)
	else ()
		# EPOLL>>> ... if it is not Cygwin ... EPOLL>>>
		option(HAS_EPOLL "Does system support epoll call?" OFF)
	endif ()
	target_link_libraries(circle.library pthread)
elseif (CMAKE_HOST_WIN32)
	add_definitions("-D_SCL_SECURE_NO_WARNINGS")
	set(CMAKE_CXX_FLAGS_RELEASE  "")
	set(CMAKE_CXX_FLAGS_DEBUG    "")
	set(CMAKE_CXX_FLAGS_TEST     "${TESTBUILD_DEFINITIONS} -DLOG_AUTOFLUSH")
	set(CMAKE_CXX_FLAGS_FASTTEST "${TESTBUILD_DEFINITIONS}")

	# EPOLL>>> ...and suppose that Windows does not
	option(HAS_EPOLL "Does system support epoll call?" OFF)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		target_link_libraries(circle.library DbgHelp)
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG")
		set(CMAKE_EXE_LINKER_FLAGS_TEST "/DEBUG")
	else ()
		target_link_libraries(circle.library DbgHelp ws2_32)
	endif()
endif ()

if (HAS_EPOLL)
	message(STATUS "Turning ON epoll")
	add_definitions("-DHAS_EPOLL")
else ()
	message(STATUS "Turning OFF epoll")
endif ()

message(STATUS "CXX compiler ID is ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "System processor is ${CMAKE_SYSTEM_PROCESSOR}")
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    endif ()
endif ()

# Main executable file.
set(MAIN_EXECUTABLE src/main.cpp)

add_executable(circle ${MAIN_EXECUTABLE})
if (NOT UNIX)
	set_target_properties(circle PROPERTIES CXX_STANDARD 20)
endif ()
target_link_libraries(circle circle.library)

# Testing part
option(BUILD_TESTS "Build and run tests." YES)
if (BUILD_TESTS)
	add_subdirectory(tests)
endif ()

# vim: set ts=4 sw=4 ai tw=0 noet syntax=cmake :

