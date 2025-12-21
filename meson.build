project('bylins', 'cpp',
    version: '0.1',
    default_options: ['cpp_std=c++20', 'buildtype=release']
)

cpp = meson.get_compiler('cpp')
host_system = host_machine.system()
project_args = []
dependencies = []

if get_option('zlib')
    zlib_dep = dependency('zlib', required: true)
    project_args += '-DHAVE_ZLIB'
    dependencies += zlib_dep
endif

if get_option('iconv')
    iconv_dep = cpp.find_library('iconv', required: false)
    if iconv_dep.found()
        project_args += '-DHAVE_ICONV'
        dependencies += iconv_dep
    endif
endif

if get_option('telegram')
    curl_dep = dependency('libcurl', required: false)
    ssl_dep = dependency('openssl', required: false)
    if curl_dep.found() and ssl_dep.found()
        project_args += '-DHAVE_TG'
        dependencies += [curl_dep, ssl_dep]
    endif
endif

boost_modules = ['system', 'filesystem']
if host_system == 'windows'
    boost_modules += 'locale'
endif

if get_option('scripting')
    python_dep = dependency('python3', version: '>=3.4')
    project_args += '-DWITH_SCRIPTING'
    dependencies += python_dep
    boost_modules += 'python3'
endif

if get_option('boost')
    dependencies += dependency('boost', modules: boost_modules)
    if host_system != 'cygwin'
        project_args += '-DBOOST_PYTHON_STATIC_LIB'
    endif
endif

if get_option('nocrypt') or get_option('testbuild') or host_system == 'darwin'
    project_args += '-DNOCRYPT'
else
    libcrypt = cpp.find_library('crypt', required: false)
    if libcrypt.found()
        dependencies += libcrypt
    endif
endif

if get_option('has_epoll') and host_system == 'linux'
    project_args += '-DHAS_EPOLL'
endif

if host_system == 'windows'
    project_args += '-D_SCL_SECURE_NO_WARNINGS'
    dependencies += cpp.find_library('dbghelp')
    if cpp.get_id() != 'msvc'
        dependencies += cpp.find_library('ws2_32')
    endif
endif

if get_option('with_asan')
    project_args += ['-fsanitize=address']
    add_project_link_arguments('-fsanitize=address', language: 'cpp')
endif

cpu = host_machine.cpu_family()
if cpp.get_id() != 'msvc'
    if cpu == 'x86_64'
        project_args += '-m64'
    elif cpu == 'x86'
        project_args += '-m32'
    endif
endif

buildtype = ''
if get_option('testbuild')
    project_args += '-DTEST_BUILD'
    buildtype = 'Test'
    message('Test build turning on')
elif get_option('buildtype') == 'debug'
    project_args += ['-D_GLIBCXX_DEBUG', '-D_GLIBXX_DEBUG_PEDANTIC']
    buildtype = 'Debug'
else
    buildtype = 'Release'
endif

add_project_arguments(project_args, language: 'cpp')

cmake = import('cmake')
fmt_opts = cmake.subproject_options()
fmt_opts.add_cmake_defines({
    'FMT_TEST': 'OFF', 
    'FMT_INSTALL': 'OFF',
    'BUILD_SHARED_LIBS': 'OFF',
    'CMAKE_POSITION_INDEPENDENT_CODE': 'ON' 
})

fmt_sub = cmake.subproject('fmt', options: fmt_opts)
fmt_dep = fmt_sub.dependency('fmt')

subdir('src/third_party_libs/libfort')

dependencies += [fmt_dep, fort_dep]

git = find_program('git', required: false)
git_rev = 'unknown'
if git.found()
    git_rev = run_command(git, 'rev-parse', '--short', 'HEAD', check: false).stdout().strip()
endif

conf_data = configuration_data()
conf_data.set('REVISION', git_rev)
conf_data.set('CMAKE_BINARY_DIR', meson.project_build_root())

py3 = import('python').find_installation()

version_cpp = custom_target('version_gen',
  input: 'src/version.cpp.in',
  output: 'version.cpp',
  command: [
    py3, '-c',
    '''import sys
content = open(sys.argv[1], encoding='koi8-r').read()
content = content.replace('${REVISION}', sys.argv[3] + ' (' + sys.argv[5] + ')')
content = content.replace('${CMAKE_BINARY_DIR}', sys.argv[4])
open(sys.argv[2], 'w', encoding='koi8-r').write(content)
    ''',
    '@INPUT@', '@OUTPUT@', git_rev, meson.project_build_root(), buildtype
  ]
)

main_sources = files(
'src/engine/structs/blocking_queue.cpp', 'src/engine/db/influxdb.cpp', 'src/engine/core/heartbeat.cpp', 'src/engine/core/heartbeat_commands.cpp', 'src/gameplay/abilities/abilities_rollsystem.cpp', 'src/administration/accounts.cpp', 'src/engine/core/action_targeting.cpp', 'src/engine/ui/cmd/do_sign.cpp', 'src/engine/core/char_movement.cpp', 'src/gameplay/communication/social.cpp', 'src/gameplay/affects/affect_handler.cpp', 'src/engine/ui/alias.cpp', 'src/gameplay/economics/auction.cpp', 'src/utils/backtrace.cpp', 'src/administration/ban.cpp', 'src/gameplay/mechanics/birthplaces.cpp', 'src/gameplay/communication/boards/boards_changelog_loaders.cpp', 'src/gameplay/communication/boards/boards_constants.cpp', 'src/gameplay/communication/boards/boards.cpp', 'src/gameplay/communication/boards/boards_formatters.cpp', 'src/gameplay/communication/boards/boards_types.cpp', 'src/gameplay/mechanics/bonus.cpp', 'src/gameplay/mechanics/bonus_command_parser.cpp', 'src/engine/boot/boot_constants.cpp', 'src/engine/boot/boot_data_files.cpp', 'src/engine/boot/boot_index.cpp', 'src/utils/cache.cpp', 'src/gameplay/mechanics/celebrates.cpp', 'src/engine/entities/char_data.cpp', 'src/engine/entities/char_player.cpp', 'src/gameplay/classes/pc_classes.cpp', 'src/gameplay/classes/classes_spell_slots.cpp', 'src/engine/ui/color.cpp', 'src/engine/core/comm.cpp', 'src/engine/ui/cmd_god/do_shutdown.cpp', 'src/engine/ui/commands.cpp', 'src/engine/core/config.cpp', 'src/gameplay/core/constants.cpp', 'src/gameplay/mechanics/corpse.cpp', 'src/gameplay/quests/quest.cpp', 'src/gameplay/quests/daily_quest.cpp', 'src/engine/db/db.cpp', 'src/gameplay/mechanics/deathtrap.cpp', 'src/gameplay/mechanics/depot.cpp', 'src/engine/db/description.cpp', 'src/engine/scripting/dg_comm.cpp', 'src/engine/scripting/dg_domination_helper.cpp', 'src/engine/scripting/dg_db_scripts.cpp', 'src/engine/scripting/dg_event.cpp', 'src/engine/scripting/dg_handler.cpp', 'src/engine/scripting/dg_misc.cpp', 'src/engine/scripting/dg_mobcmd.cpp', 'src/engine/scripting/dg_objcmd.cpp', 'src/engine/scripting/dg_olc.cpp', 'src/engine/scripting/dg_scripts.cpp', 'src/engine/scripting/dg_triggers.cpp', 'src/engine/scripting/dg_wldcmd.cpp', 'src/gameplay/economics/dictionary.cpp', 'src/utils/diskio.cpp', 'src/gameplay/statistics/dps.cpp', 'src/engine/entities/entities_constants.cpp', 'src/gameplay/economics/exchange.cpp', 'src/gameplay/economics/ext_money.cpp', 'src/engine/core/external_trigger.cpp', 'src/gameplay/abilities/feats.cpp', 'src/gameplay/fight/fight.cpp', 'src/gameplay/fight/fight_hit.cpp', 'src/gameplay/fight/fight_penalties.cpp', 'src/gameplay/fight/fight_stuff.cpp', 'src/utils/file_crc.cpp', 'src/gameplay/core/genchar.cpp', 'src/gameplay/mechanics/glory_const.cpp', 'src/gameplay/mechanics/glory.cpp', 'src/gameplay/mechanics/glory_misc.cpp', 'src/gameplay/ai/graph.cpp', 'src/engine/core/handler.cpp', 'src/engine/db/help.cpp', 'src/gameplay/clans/house.cpp', 'src/gameplay/clans/house_exp.cpp', 'src/gameplay/communication/ignores.cpp', 'src/gameplay/communication/ignores_loader.cpp', 'src/gameplay/crafting/im.cpp', 'src/engine/ui/interpreter.cpp', 'src/gameplay/crafting/item_creation.cpp', 'src/gameplay/core/game_limits.cpp', 'src/utils/levenshtein.cpp', 'src/gameplay/mechanics/liquid.cpp', 'src/utils/logger.cpp', 'src/gameplay/magic/magic.cpp', 'src/gameplay/magic/magic_items.cpp', 'src/gameplay/magic/magic_rooms.cpp', 'src/gameplay/magic/magic_utils.cpp', 'src/gameplay/magic/spells.cpp', 'src/gameplay/magic/spells_info.cpp', 'src/gameplay/magic/magic_temp_spells.cpp', 'src/gameplay/communication/mail.cpp', 'src/engine/ui/mapsystem.cpp', 'src/gameplay/mechanics/meat_maker.cpp', 'src/engine/olc/medit.cpp', 'src/gameplay/ai/mobact.cpp', 'src/gameplay/mechanics/mobmax.cpp', 'src/gameplay/statistics/mob_stat.cpp', 'src/engine/ui/modify.cpp', 'src/gameplay/skills/morph.cpp', 'src/administration/name_adviser.cpp', 'src/gameplay/mechanics/named_stuff.cpp', 'src/administration/names.cpp', 'src/gameplay/mechanics/noob.cpp', 'src/engine/entities/obj_data.cpp', 'src/gameplay/mechanics/obj_enchant.cpp', 'src/engine/db/obj_save.cpp', 'src/gameplay/mechanics/obj_save_ext.cpp', 'src/gameplay/mechanics/obj_sets.cpp', 'src/gameplay/mechanics/obj_sets_olc.cpp', 'src/gameplay/magic/magic_objects.cpp', 'src/engine/olc/oedit.cpp', 'src/engine/olc/olc.cpp', 'src/gameplay/communication/parcel.cpp', 'src/utils/parse.cpp', 'src/administration/password.cpp', 'src/gameplay/fight/pk.cpp', 'src/gameplay/mechanics/player_races.cpp', 'src/administration/privilege.cpp', 'src/third_party_libs/pugixml/pugixml.cpp', 'src/gameplay/quests/quested.cpp', 'src/utils/random.cpp', 'src/engine/olc/redit.cpp', 'src/gameplay/communication/remember.cpp', 'src/gameplay/core/reset_stats.cpp', 'src/engine/entities/room_data.cpp', 'src/gameplay/mechanics/sets_drop.cpp', 'src/gameplay/economics/shop_ext.cpp', 'src/administration/shutdown_parameters.cpp', 'src/gameplay/skills/skills.cpp', 'src/gameplay/skills/skills_info.cpp', 'src/gameplay/communication/spam.cpp', 'src/gameplay/ai/spec_assign.cpp', 'src/gameplay/ai/spec_procs.cpp', 'src/engine/network/descriptor_data.cpp', 'src/gameplay/mechanics/stuff.cpp', 'src/engine/structs/flags.hpp', 'src/utils/id_converter.cpp', 'src/utils/utils_time.cpp', 'src/gameplay/mechanics/title.cpp', 'src/gameplay/statistics/top.cpp', 'src/utils/utils.cpp', 'src/version.cpp', 'src/gameplay/mechanics/weather.cpp', 'src/engine/olc/zedit.cpp', 'src/utils/utils_string.cpp', 'src/engine/db/world_objects.cpp', 'src/engine/db/obj_prototypes.cpp', 'src/engine/db/id.cpp', 'src/engine/db/utils_find_obj_id_by_vnum.cpp', 'src/engine/db/global_objects.cpp', 'src/gameplay/economics/shops_implementation.cpp', 'src/engine/db/world_characters.cpp', 'src/utils/utils_debug.cpp', 'src/gameplay/economics/shops_implementation.cpp', 'src/engine/structs/radix_trie.cpp', 'src/engine/structs/compact_trie.cpp', 'src/engine/db/id.cpp', 'src/engine/entities/zone.cpp', 'src/gameplay/mechanics/strengthening.cpp', 'src/gameplay/skills/backstab.cpp', 'src/gameplay/skills/bash.cpp', 'src/gameplay/skills/stun.cpp', 'src/gameplay/skills/resque.cpp', 'src/gameplay/skills/kick.cpp', 'src/gameplay/skills/strangle.cpp', 'src/gameplay/skills/chopoff.cpp', 'src/gameplay/skills/disarm.cpp', 'src/gameplay/skills/overhelm.cpp', 'src/gameplay/skills/ironwind.cpp', 'src/gameplay/skills/throw.cpp', 'src/gameplay/skills/mighthit.cpp', 'src/gameplay/skills/morph.cpp', 'src/gameplay/skills/shield_block.cpp', 'src/gameplay/skills/parry.cpp', 'src/gameplay/skills/pick.cpp', 'src/gameplay/mechanics/poison.cpp', 'src/gameplay/skills/protect.cpp', 'src/gameplay/skills/expendientcut.cpp', 'src/gameplay/skills/turnundead.cpp', 'src/gameplay/skills/townportal.cpp', 'src/gameplay/skills/manadrain.cpp', 'src/gameplay/skills/warcry.cpp', 'src/gameplay/skills/identify.cpp', 'src/gameplay/skills/relocate.cpp', 'src/gameplay/mechanics/mount.cpp', 'src/gameplay/fight/assist.cpp', 'src/engine/ui/cmd/do_kill.cpp', 'src/engine/ui/cmd/do_style.cpp', 'src/gameplay/fight/common.cpp', 'src/engine/ui/cmd/do_retreat.cpp', 'src/engine/ui/cmd/do_order.cpp', 'src/engine/ui/cmd/do_mercenary.cpp', 'src/gameplay/affects/affect_data.cpp', 'src/gameplay/skills/track.cpp', 'src/engine/ui/cmd/do_flee.cpp', 'src/engine/ui/cmd/do_hire.cpp', 'src/engine/ui/cmd/do_telegram.cpp', 'src/engine/ui/cmd_god/do_stat.cpp', 'src/engine/ui/cmd_god/do_godtest.cpp', 'src/engine/ui/cmd/do_learn.cpp', 'src/engine/ui/cmd/do_forget.cpp', 'src/engine/ui/cmd/do_memorize.cpp', 'src/engine/ui/cmd/do_create.cpp', 'src/engine/ui/cmd/do_mixture.cpp', 'src/engine/ui/cmd/do_cast.cpp', 'src/engine/ui/cmd/do_employ.cpp', 'src/engine/ui/cmd/do_follow.cpp', 'src/engine/structs/flag_data.cpp', 'src/gameplay/crafting/jewelry.cpp', 'src/gameplay/crafting/mining.cpp', 'src/gameplay/classes/classes_constants.cpp', 'src/engine/structs/extra_description.cpp', 'src/gameplay/communication/insult.cpp', 'src/gameplay/abilities/abilities_info.cpp', 'src/gameplay/abilities/abilities_constants.cpp', 'src/gameplay/classes/pc_classes_info.cpp', 'src/engine/ui/objects_filter.cpp', 'src/engine/structs/info_container.cpp', 'src/utils/parser_wrapper.cpp', 'src/engine/boot/cfg_manager.cpp', 'src/gameplay/affects/affect_contants.cpp', 'src/engine/ui/cmd_god/do_users.cpp', 'src/engine/ui/table_wrapper.cpp', 'src/engine/ui/cmd/do_score.cpp', 'src/gameplay/magic/spells_constants.cpp', 'src/gameplay/mechanics/mem_queue.cpp', 'src/engine/ui/cmd_god/do_show.cpp', 'src/engine/ui/cmd/do_drop.cpp', 'src/gameplay/skills/skinning.cpp', 'src/gameplay/skills/repair.cpp', 'src/gameplay/skills/poisoning.cpp', 'src/engine/ui/cmd/do_give.cpp', 'src/engine/ui/cmd/do_trample.cpp', 'src/gameplay/skills/campfire.cpp', 'src/gameplay/skills/firstaid.cpp', 'src/gameplay/skills/sharpening.cpp', 'src/gameplay/skills/armoring.cpp', 'src/engine/ui/cmd/do_get.cpp', 'src/engine/ui/cmd/do_equip.cpp', 'src/engine/ui/cmd/do_put.cpp', 'src/engine/ui/cmd/do_remove.cpp', 'src/engine/ui/cmd/do_eat.cpp', 'src/engine/ui/cmd_god/do_inspect.cpp', 'src/engine/ui/cmd_god/do_mark.cpp', 'src/gameplay/crafting/fry.cpp', 'src/engine/ui/cmd/do_refill.cpp', 'src/gameplay/mechanics/guilds.cpp', 'src/engine/ui/cmd/do_features.cpp', 'src/engine/ui/cmd/do_spells.cpp', 'src/engine/ui/cmd/do_skills.cpp', 'src/gameplay/economics/currencies.cpp', 'src/gameplay/skills/spell_capable.cpp', 'src/gameplay/skills/lightwalk.cpp', 'src/gameplay/skills/death_rage.cpp', 'src/gameplay/skills/fit.cpp', 'src/gameplay/abilities/talents_effects.cpp', 'src/gameplay/abilities/feats_constants.cpp', 'src/gameplay/abilities/abilities_items_set.cpp', 'src/gameplay/abilities/talents_actions.cpp', 'src/gameplay/skills/slay.cpp', 'src/gameplay/skills/charge.cpp', 'src/gameplay/skills/dazzle.cpp', 'src/gameplay/communication/offtop.cpp', 'src/gameplay/mechanics/dead_load.cpp', 'src/gameplay/mechanics/city_guards.cpp', 'src/engine/ui/cmd/do_remort.cpp', 'src/gameplay/mechanics/dungeons.cpp', 'src/engine/ui/cmd_god/do_tabulate.cpp', 'src/engine/ui/cmd_god/do_reload.cpp', 'src/gameplay/mechanics/mob_races.cpp', 'src/engine/ui/cmd_god/do_set.cpp', 'src/gameplay/mechanics/stable_objs.cpp', 'src/gameplay/mechanics/treasure_cases.cpp', 'src/gameplay/mechanics/cities.cpp', 'src/gameplay/quests/bodrich_quests.cpp', 'src/engine/ui/cmd/do_affects.cpp', 'src/engine/ui/cmd/do_toggle.cpp', 'src/engine/ui/cmd/do_mode.cpp', 'src/engine/ui/cmd/do_levels.cpp', 'src/engine/ui/cmd/do_where.cpp', 'src/engine/ui/cmd/do_who.cpp', 'src/engine/ui/cmd/do_enter.cpp', 'src/engine/ui/cmd/do_sleep.cpp', 'src/engine/ui/cmd/do_wake.cpp', 'src/engine/ui/cmd/do_bandage.cpp', 'src/engine/ui/cmd/do_consider.cpp', 'src/engine/ui/cmd/do_exits.cpp', 'src/gameplay/mechanics/sight.cpp', 'src/engine/ui/cmd/do_look.cpp', 'src/engine/ui/cmd/do_examine.cpp', 'src/engine/ui/cmd/do_peer.cpp', 'src/engine/ui/cmd/do_look_around.cpp', 'src/engine/ui/cmd/do_inventory.cpp', 'src/engine/ui/cmd/do_equipment.cpp', 'src/engine/ui/cmd/do_listen.cpp', 'src/engine/ui/cmd/do_zone.cpp', 'src/gameplay/skills/dazzle.cpp', 'src/gameplay/ai/mob_memory.cpp', 'src/engine/network/logon.cpp', 'src/engine/ui/cmd_god/do_ban.cpp', 'src/engine/ui/cmd_god/do_proxy.cpp', 'src/administration/proxy.cpp', 'src/engine/ui/cmd_god/do_print_armor.cpp', 'src/engine/ui/cmd_god/do_forcetime.cpp', 'src/engine/ui/cmd_god/do_liblist.cpp', 'src/engine/ui/cmd_god/do_wizutil.cpp', 'src/administration/punishments.cpp', 'src/engine/ui/cmd_god/do_wiznet.cpp', 'src/engine/ui/cmd_god/do_force.cpp', 'src/engine/ui/cmd_god/do_invisible.cpp', 'src/engine/ui/cmd_god/do_set_all.cpp', 'src/utils/grammar/cases.cpp', 'src/engine/core/iosystem.cpp', 'src/gameplay/mechanics/groups.cpp', 'src/gameplay/statistics/money_drop.cpp', 'src/gameplay/statistics/zone_exp.cpp', 'src/gameplay/communication/talk.cpp', 'src/engine/ui/cmd_god/do_echo.cpp', 'src/gameplay/core/base_stats.cpp', 'src/engine/ui/cmd/do_camouflage.cpp', 'src/engine/ui/cmd/do_hide.cpp', 'src/engine/ui/cmd/do_steal.cpp', 'src/engine/ui/cmd/do_group.cpp', 'src/engine/ui/cmd/do_ungroup.cpp', 'src/engine/ui/cmd_god/do_beep.cpp', 'src/engine/ui/cmd/do_pray.cpp', 'src/engine/ui/cmd/do_recall.cpp', 'src/engine/ui/cmd/do_display.cpp', 'src/engine/ui/cmd/do_wimpy.cpp', 'src/engine/ui/cmd/do_quit.cpp', 'src/engine/ui/cmd/do_courage.cpp', 'src/engine/ui/cmd/do_sneak.cpp', 'src/engine/ui/cmd/do_visible.cpp', 'src/engine/ui/cmd/do_save.cpp', 'src/engine/ui/cmd/do_summon.cpp', 'src/engine/ui/cmd/do_antigods.cpp', 'src/gameplay/mechanics/illumination.cpp', 'src/gameplay/mechanics/awake.cpp', 'src/gameplay/statistics/char_stat.cpp', 'src/engine/ui/cmd/do_mystat.cpp', 'src/engine/ui/cmd/do_weather.cpp', 'src/engine/ui/cmd/do_statistic.cpp', 'src/engine/ui/cmd/do_time.cpp', 'src/engine/ui/cmd/do_diagnose.cpp', 'src/engine/ui/cmd/do_commands.cpp', 'src/engine/ui/cmd/do_who_am_i.cpp', 'src/engine/ui/cmd/do_generic_page.cpp', 'src/engine/ui/cmd/do_gold.cpp', 'src/engine/ui/cmd/do_check_invoice.cpp', 'src/gameplay/communication/check_invoice.cpp', 'src/engine/ui/cmd/do_quest.cpp', 'src/engine/ui/cmd/do_write.cpp', 'src/engine/ui/cmd/do_group_say.cpp', 'src/engine/ui/cmd/do_say.cpp', 'src/engine/ui/cmd/do_ignore.cpp', 'src/engine/ui/cmd/do_reply.cpp', 'src/engine/ui/cmd/do_tell.cpp', 'src/engine/ui/cmd/do_offtop.cpp', 'src/engine/ui/cmd/do_pray_gods.cpp', 'src/engine/ui/cmd/do_gen_comm.cpp', 'src/engine/ui/cmd/do_spec_comm.cpp', 'src/engine/ui/cmd/do_mobshout.cpp', 'src/engine/ui/cmd/do_page.cpp', 'src/engine/ui/cmd/do_rest.cpp', 'src/engine/ui/cmd/do_sit.cpp', 'src/engine/ui/cmd/do_stand.cpp', 'src/gameplay/mechanics/doors.cpp', 'src/engine/ui/cmd/do_gen_door.cpp', 'src/gameplay/skills/throwout.cpp', 'src/gameplay/skills/frenzy.cpp', 'src/engine/ui/cmd_god/do_zreset.cpp', 'src/engine/ui/cmd_god/do_dc.cpp', 'src/engine/ui/cmd_god/do_advance.cpp', 'src/engine/ui/cmd_god/do_glory.cpp', 'src/administration/karma.cpp', 'src/engine/ui/cmd_god/do_arena_restore.cpp', 'src/engine/ui/cmd_god/do_load.cpp', 'src/engine/ui/cmd_god/do_overstuff.cpp', 'src/engine/ui/cmd_god/do_zclear.cpp', 'src/engine/ui/cmd_god/do_delete_obj.cpp', 'src/engine/ui/cmd_god/do_show_zone_stat.cpp', 'src/engine/ui/cmd_god/do_occupation.cpp', 'src/engine/ui/cmd_god/do_send.cpp', 'src/engine/ui/cmd_god/do_at_room.cpp', 'src/engine/ui/cmd_god/do_unfreeze.cpp', 'src/engine/ui/cmd_god/do_goto.cpp', 'src/engine/ui/cmd_god/do_teleport.cpp', 'src/engine/ui/cmd_god/do_snoop.cpp', 'src/engine/ui/cmd_god/do_switch.cpp', 'src/engine/ui/cmd_god/do_vstat.cpp', 'src/engine/ui/cmd_god/do_purge.cpp', 'src/engine/ui/cmd_god/do_syslog.cpp', 'src/engine/ui/cmd_god/do_restore.cpp', 'src/engine/ui/cmd_god/do_gecho.cpp', 'src/engine/ui/cmd_god/do_poof_msg.cpp', 'src/engine/ui/cmd_god/do_wizlock.cpp', 'src/engine/ui/cmd_god/do_date.cpp', 'src/engine/ui/cmd_god/do_last.cpp', 'src/engine/ui/cmd_god/do_sdemigods.cpp', 'src/engine/ui/cmd_god/do_spellstat.cpp', 'src/gameplay/statistics/spell_usage.cpp', 'src/engine/ui/cmd_god/do_loadstat.cpp', 'src/engine/ui/cmd_god/do_sanitize.cpp', 'src/engine/ui/cmd/do_alias.cpp', 'src/engine/ui/cmd/do_drink.cpp', 'src/engine/ui/cmd/do_drunkoff.cpp', 'src/engine/ui/cmd/do_pour.cpp', 'src/engine/ui/cmd/do_hidemove.cpp', 'src/engine/ui/cmd/do_move.cpp', 'src/gameplay/mechanics/hide.cpp', 'src/gameplay/mechanics/boat.cpp', 'src/gameplay/mechanics/minions.cpp', 'src/gameplay/mechanics/armor.cpp', 'src/gameplay/skills/leadership.cpp', 'src/gameplay/skills/punctual_style.cpp', 'src/gameplay/mechanics/equipment.cpp', 'src/gameplay/skills/intercept.cpp', 'src/gameplay/skills/deviate.cpp', 'src/gameplay/skills/multyparry.cpp', 'src/gameplay/mechanics/damage.cpp', 'src/gameplay/mechanics/tutelar.cpp', 'src/gameplay/skills/addshot.cpp', 'src/engine/db/player_index.cpp', 'src/gameplay/skills/addshot.cpp', 'src/gameplay/classes/mob_classes_info.cpp', 'src/gameplay/classes/recalc_mob_params_by_vnum.cpp'
)

msdp_sources = files(
    'src/engine/network/msdp/msdp.cpp',
    'src/engine/network/msdp/msdp_parser.cpp',
    'src/engine/network/msdp/msdp_constants.cpp',
    'src/engine/network/msdp/msdp_reporters.cpp',
    'src/engine/network/msdp/msdp_senders.cpp'
)

if get_option('scripting')
    main_sources += files('src/engine/scripting/scripting.cpp')
endif

circle_lib = static_library('circle',
    [main_sources, msdp_sources, version_cpp],
    include_directories: include_directories('src'),
    dependencies: dependencies
)

circle_dep = declare_dependency(
    link_with: circle_lib,
    include_directories: include_directories('src')
)

subdir('tests')

executable('circle',
    'src/main.cpp',
    dependencies: [dependencies, circle_dep],
    link_with: circle_lib,
    install: true
)
