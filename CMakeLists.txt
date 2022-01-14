cmake_minimum_required(VERSION 3.0)
cmake_minimum_required(VERSION 3.0)

project(bylins CXX)

if (NOT ${CMAKE_VERSION} VERSION_LESS "3.12.0")
	cmake_policy(SET CMP0074 NEW)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCES
		src/blocking_queue.cpp
		src/influxdb.cpp
		src/heartbeat.cpp
		src/heartbeat_commands.cpp
		src/abilities/abilities_rollsystem.cpp
		src/accounts.cpp
		src/action_targeting.cpp
		src/act_comm.cpp
		src/act_informative.cpp
		src/act_item.cpp
		src/act_movement.cpp
		src/act_other.cpp
		src/act_social.cpp
		src/act_wizard.cpp
		src/affects/affect_handler.cpp
		src/alias.cpp
		src/auction.cpp
		src/backtrace.cpp
		src/cmd_god/ban.cpp
		src/birthplaces.cpp
		src/boards/boards_changelog_loaders.cpp
		src/boards/boards_constants.cpp
		src/boards/boards.cpp
		src/boards/boards_formatters.cpp
		src/boards/boards_types.cpp
		src/game_mechanics/bonus.cpp
		src/game_mechanics/bonus_command_parser.cpp
		src/boot/boot_constants.cpp
		src/boot/boot_data_files.cpp
		src/boot/boot_index.cpp
		src/cache.cpp
		src/game_mechanics/celebrates.cpp
		src/entities/char.cpp
		src/entities/char_player.cpp
		src/classes/class.cpp
		src/classes/class_spell_slots.cpp
		src/color.cpp
		src/comm.cpp
		src/cmd_god/shutdown.cpp
		src/commands.cpp
		src/config.cpp
		src/constants.cpp
		src/corpse.cpp
		src/quests/quest.cpp
		src/quests/daily_quest.cpp
		src/db.cpp
		src/game_mechanics/deathtrap.cpp
		src/depot.cpp
		src/description.cpp
		src/dg_script/dg_comm.cpp
		src/dg_script/dg_db_scripts.cpp
		src/dg_script/dg_event.cpp
		src/dg_script/dg_handler.cpp
		src/dg_script/dg_misc.cpp
		src/dg_script/dg_mobcmd.cpp
		src/dg_script/dg_objcmd.cpp
		src/dg_script/dg_olc.cpp
		src/dg_script/dg_scripts.cpp
		src/dg_script/dg_triggers.cpp
		src/dg_script/dg_wldcmd.cpp
		src/dictionary.cpp
		src/diskio.cpp
		src/dps.cpp
		src/entities/entity_constants.cpp
		src/exchange.cpp
		src/ext_money.cpp
		src/external_trigger.cpp
		src/feats.cpp
		src/fightsystem/fight.cpp
		src/fightsystem/fight_hit.cpp
		src/fightsystem/fight_penalties.cpp
		src/fightsystem/fight_stuff.cpp
		src/utils/file_crc.cpp
		src/genchar.cpp
		src/game_mechanics/glory_const.cpp
		src/game_mechanics/glory.cpp
		src/game_mechanics/glory_misc.cpp
		src/graph.cpp
		src/handler.cpp
		src/help.cpp
		src/house.cpp
		src/house_exp.cpp
		src/ignores.cpp
		src/ignores_loader.cpp
		src/crafts/im.cpp
		src/interpreter.cpp
		src/crafts/item_creation.cpp
		src/game_limits.cpp
		src/utils/levenshtein.cpp
		src/liquid.cpp
		src/logger.cpp
		src/magic/magic.cpp
		src/magic/magic_items.cpp
		src/magic/magic_rooms.cpp
		src/magic/magic_utils.cpp
		src/magic/spells.cpp
		src/magic/spells_info.cpp
		src/magic/magic_temp_spells.cpp
		src/mail.cpp
		src/mapsystem.cpp
		src/meat_maker.cpp
		src/olc/medit.cpp
		src/fightsystem/mobact.cpp
		src/mobmax.cpp
		src/mob_stat.cpp
		src/modify.cpp
		src/skills/morph.cpp
		src/name_adviser.cpp
		src/game_mechanics/named_stuff.cpp
		src/name_list.cpp
		src/names.cpp
		src/noob.cpp
		src/entities/obj.cpp
		src/obj_enchant.cpp
		src/obj_save.cpp
		src/game_mechanics/obj_save_ext.cpp
		src/game_mechanics/obj_sets.cpp
		src/game_mechanics/obj_sets_olc.cpp
		src/magic/magic_objects.cpp
		src/olc/oedit.cpp
		src/olc/olc.cpp
		src/parcel.cpp
		src/parse.cpp
		src/password.cpp
		src/fightsystem/pk.cpp
		src/entities/player_races.cpp
		src/privilege.cpp
		src/utils/pugixml.cpp
		src/quests/quested.cpp
		src/utils/random.cpp
		src/olc/redit.cpp
		src/remember.cpp
		src/reset_stats.cpp
		src/entities/room.cpp
		src/game_mechanics/sets_drop.cpp
		src/shop_ext.cpp
		src/cmd_god/shutdown_parameters.cpp
		src/skills.cpp
		src/skills_info.cpp
		src/spam.cpp
		src/spec_assign.cpp
		src/spec_procs.cpp
		src/structs/descriptor_data.cpp
        src/structs/structs.cpp
		src/stuff.cpp
		src/utils/flags.hpp
		src/utils/id_converter.cpp
		src/utils/utils_time.cpp
		src/title.cpp
		src/top.cpp
		src/utils/utils.cpp
		src/version.cpp
		src/weather.cpp
		src/olc/zedit.cpp
		src/utils/utils_string.cpp
		src/world_objects.cpp
		src/obj_prototypes.cpp
		src/id.cpp
		src/utils/utils_find_obj_id_by_vnum.cpp
		src/global_objects.cpp
		src/shops_implementation.cpp
		src/entities/world_characters.cpp
		src/utils/utils_debug.cpp
		src/speedwalks.cpp
		src/shops_implementation.cpp
        src/structs/radix_trie.cpp
        src/structs/compact_trie.cpp
		src/id.cpp
		src/stigmas.cpp
		src/zone.table.cpp
		src/strengthening.cpp
		src/skills/backstab.cpp
		src/skills/bash.cpp
		src/skills/stun.cpp
		src/skills/resque.cpp
		src/skills/kick.cpp
		src/skills/strangle.cpp
		src/skills/chopoff.cpp
		src/skills/disarm.cpp
		src/skills/stupor.cpp
		src/skills/ironwind.cpp
		src/skills/throw.cpp
		src/skills/mighthit.cpp
		src/skills/morph.cpp
		src/skills/block.cpp
		src/skills/parry.cpp
		src/skills/pick.cpp
		src/skills/poison.cpp
		src/skills/protect.cpp
		src/skills/expendientcut.cpp
		src/skills/turnundead.cpp
		src/skills/townportal.cpp
		src/skills/manadrain.cpp
		src/skills/warcry.cpp
		src/skills/identify.cpp
		src/skills/relocate.cpp
		src/game_mechanics/mount.cpp
		src/fightsystem/assist.cpp
		src/fightsystem/fight_start.cpp
		src/skills/styles.cpp
		src/fightsystem/common.cpp
		src/cmd/retreat.cpp
		src/cmd/order.cpp
		src/cmd/mercenary.cpp
		src/affects/affect_data.cpp
		src/skills/track.cpp
		src/cmd/flee.cpp
		src/cmd/hire.cpp
		src/cmd/telegram.cpp
		src/cmd_god/stat.cpp
		src/cmd_god/godtest.cpp
		src/cmd/learn.cpp
		src/cmd/forget.cpp
		src/cmd/memorize.cpp
		src/cmd/create.cpp
		src/cmd/mixture.cpp
		src/cmd/cast.cpp
		src/cmd/employ.cpp
        src/cmd/follow.cpp
		src/structs/flag_data.cpp
		)

set(HEADERS
		src/cmd/flee.h
		src/cmd/follow.h
		src/cmd/learn.h
		src/cmd/forget.h
		src/cmd/create.h
		src/cmd/memorize.h
		src/cmd_god/stat.h
		src/cmd_god/godtest.h
		src/cmd/telegram.h
		src/cmd/hire.h
		src/cmd/mixture.h
		src/cmd/cast.h
		src/cmd/employ.h
		src/affects/affect_data.h
		src/skills/track.h
		src/fightsystem/common.h
		src/fightsystem/assist.h
		src/fightsystem/fight_start.h
		src/cmd/retreat.h
		src/cmd/order.h
		src/cmd/mercenary.h
		src/skills/styles.h
		src/skills/manadrain.h
		src/skills/townportal.h
		src/skills/turnundead.h
		src/skills/mighthit.h
		src/skills/expendientcut.h
		src/skills/block.h
		src/skills/protect.h
		src/skills/parry.h
		src/skills/pick.h
		src/skills/throw.h
		src/skills/ironwind.h
		src/skills/disarm.h
		src/skills/warcry.h
		src/skills/identify.h
		src/skills/relocate.h
		src/act_movement.h
		src/strengthening.h
		src/abilities/abilities_constants.h
		src/abilities/abilities_rollsystem.h
		src/game_mechanics/mount.h
		src/accounts.h
		src/action_targeting.h
		src/blocking_queue.h
		src/influxdb.h
		src/heartbeat_commands.h
		src/weather.h
		src/game_limits.h
		src/act_wizard.h
		src/heartbeat.h
		src/speedwalks.h
		src/entities/world_characters.h
		src/shops_implementation.h
		src/global_objects.h
		src/utils/id_converter.h
		src/utils/utils_find_obj_id_by_vnum.h
		src/id.h
        src/structs/compact_trie.h
        src/structs/radix_trie.h
		src/obj_prototypes.h
		src/world_objects.h
		src/utils/utils_string.h
		src/act_other.h
		src/affects/affect_handler.h
		src/auction.h
		src/backtrace.h
		src/cmd_god/ban.h
		src/birthplaces.h
		src/boards/boards_changelog_loaders.h
		src/boards/boards_constants.h
		src/boards/boards_formatters.h
		src/boards/boards.h
		src/boards/boards_message.h
		src/boards/boards_types.h
		src/game_mechanics/bonus.h
		src/game_mechanics/bonus_command_parser.h
		src/game_mechanics/bonus_types.h
		src/boot/boot_constants.h
		src/boot/boot_data_files.h
		src/boot/boot_index.h
		src/cache.h
		src/game_mechanics/celebrates.h
		src/entities/char.h
		src/utils/utils_char_obj.inl
		src/entities/char_player.h
		src/classes/class.h
		src/classes/class_spell_slots.h
		src/comm.h
		src/cmd_god/shutdown.h
		src/commands.h
		src/conf.h
		src/config.h
		src/constants.h
		src/coredump.h
		src/corpse.h
		src/quests/daily_quest.h
		src/quests/quest.h
		src/db.h
		src/game_mechanics/deathtrap.h
		src/depot.h
		src/description.h
		src/dg_script/dg_db_scripts.h
		src/dg_script/dg_event.h
		src/dg_script/dg_olc.h
		src/dg_script/dg_scripts.h
		src/dictionary.h
		src/diskio.h
		src/structs/structs_double_map.h
		src/dps.h
		src/exchange.h
		src/ext_money.h
		src/external_trigger.h
		src/abilities/abilities_items_set.h
		src/feats.h
		src/fightsystem/fight_constants.h
		src/fightsystem/fight.h
		src/fightsystem/fight_hit.h
		src/fightsystem/fight_stuff.h
		src/fightsystem/fight_penalties.h
		src/utils/file_crc.h
		src/genchar.h
		src/game_mechanics/glory_const.h
		src/game_mechanics/glory.h
		src/game_mechanics/glory_misc.h
		src/handler.h
		src/help.h
		src/house_exp.h
		src/house.h
		src/ignores.h
		src/ignores_loader.h
		src/crafts/im.h
		src/interpreter.h
		src/crafts/item_creation.h
		src/utils/levenshtein.h
		src/liquid.h
		src/logger.h
		src/magic/magic.h
		src/magic/magic_items.h
		src/magic/magic_rooms.h
		src/magic/magic_utils.h
		src/magic/spells.h
		src/magic/spells_info.h
		src/magic/magic_temp_spells.h
		src/mail.h
		src/mapsystem.h
		src/meat_maker.h
		src/fightsystem/mobact.h
		src/mobmax.h
		src/mob_stat.h
		src/modify.h
		src/name_adviser.h
		src/game_mechanics/named_stuff.h
		src/name_list.h
		src/noob.h
		src/obj_enchant.h
		src/entities/obj.h
		src/obj_save.h
		src/game_mechanics/obj_sets.h
		src/game_mechanics/obj_sets_stuff.h
		src/olc/olc.h
		src/parcel.h
		src/parse.h
		src/password.h
		src/fightsystem/pk.h
		src/entities/player_i.h
		src/entities/player_races.h
		src/privilege.h
		src/utils/pugiconfig.h
		src/utils/pugixml.h
		src/quests/quested.h
		src/utils/random.h
		src/remember.h
		src/reset_stats.h
		src/entities/room.h
		src/screen.h
		src/game_mechanics/sets_drop.h
		src/shop_ext.h
		src/cmd_god/shutdown_parameters.h
		src/skills.h
		src/skills_info.h
		src/spam.h
		src/structs/descriptor_data.h
        src/structs/structs.h
		src/stuff.h
		src/sysdep.h
		src/telnet.h
		src/utils/utils_time.h
		src/title.h
		src/top.h
		src/utils/utils.h
		src/xml_loading_helper.h
		src/utils/utils_debug.h
		src/stigmas.h
		src/zone.table.h
		src/skills/backstab.h
		src/skills/bash.h
		src/skills/stun.h
		src/skills/poison.h
		src/skills/resque.h
		src/skills/kick.h
		src/skills/strangle.h
		src/skills/chopoff.h
		src/skills/stupor.h
		src/graph.h
		src/structs/flag_data.h
		src/entities/entity_constants.h
		)

set(CONFIGURATION_FILES lib.template/misc/configuration.xml)
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

set(SCRIPTING_HEADERS src/scripting.h)
set(SCRIPTING_SOURCES src/scripting.cpp)

option(TIMED_SCRIPT "Turns on/off definition TIMED_SCRIPT" NO)
set(TIMED_SCRIPT_DEFINITION "")
if (TIMED_SCRIPT)
	set(TIMED_SCRIPT_DEFINITION "-DTIMED_SCRIPT")
endif ()
add_definitions(${TIMED_SCRIPT_DEFINITION})

# Turns on/off scripting. Use the option like:
#     $ cmake -DSCRIPTING=ON..
# to turn scripting ON.
option(SCRIPTING "Turns on/off scripting engine" OFF)
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

set(MSDP_FILES
		src/msdp/msdp.h
	src/msdp/msdp.cpp
		src/msdp/msdp_parser.cpp
		src/msdp/msdp_parser.h
		src/msdp/msdp_constants.h
		src/msdp/msdp_constants.cpp
		src/msdp/msdp_reporters.cpp
		src/msdp/msdp_reporters.h
		src/msdp/msdp_senders.cpp
		src/msdp/msdp_senders.h)
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
	lib.template/misc/craft/tailoring/index.xml)
set(CRAFT_GENERAL_FILES
	lib.template/misc/craft/index.xml)
set(CRAFT_MATERIAL_FILES
	lib.template/misc/craft/tough_leather.xml
	lib.template/misc/craft/soft_leather.xml
	lib.template/misc/craft/inwrought_flax.xml)
set(CRAFT_RECIPE_FILES
	lib.template/misc/craft/recipe1.xml)
set(CRAFT_CRAFT_FILES
	lib.template/misc/craft/craft1.xml)
set(CRAFT_TEMPLATE_FILES
	lib.template/misc/craft/material_template.xml
	lib.template/misc/craft/prototype_template.xml
	lib.template/misc/craft/recipe_template.xml)
set(CRAFT_PROTOTYPES)
set(CRAFT_SOURCE_FILES
		src/crafts/craft_commands.cpp
		src/crafts/craft.cpp
		src/crafts/craft_logger.cpp
		src/crafts/craft_static.cpp)
set(CRAFT_HEADER_FILES
		src/crafts/craft_commands.h
		src/crafts/craft.h
		src/crafts/craft_logger.h
		src/crafts/craft_static.h)

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
	lib.template/misc/noob_help.xml
	lib.template/misc/configuration.xml
	lib.template/misc/grouping)

source_group("Misc" FILES ${MISC_FILES})

set(CIRCLE_FILES ${SOURCES} ${HEADERS} readme.markdown CONTRIBUTING.md ${CRAFT_FILES} ${MISC_FILES})

# Sort source and header files. Just to convenience.
list(SORT CIRCLE_FILES)

# Create changelog file
add_custom_target(changelog ALL
	COMMAND ${GIT} log --date=local --reverse > changelog
	COMMENT "Writing changelog...")
set_target_properties(changelog PROPERTIES FOLDER "Utility targets")

# Main library file.
add_library(circle.library STATIC ${CIRCLE_FILES})
if (UNIX OR CYGWIN)
	add_definitions("--std=gnu++17")
else ()
	set_target_properties(circle.library PROPERTIES CXX_STANDARD 14)
endif ()
add_dependencies(circle.library versioning changelog)

# Boost
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
else ()
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
if (NOCRYPT)
	set(DEBUG_CRYPT "-DNOCRYPT")
endif ()

if (CMAKE_HOST_UNIX)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS_RELEASE  "-ggdb3 -O0    -Wall -Wextra -Wno-format-truncation")
		set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0    -Wall -Wextra -Wno-format-truncation -D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC ${ASAN_FLAGS} ${DEBUG_CRYPT}")
		set(CMAKE_CXX_FLAGS_TEST     "       -Og    -Wall -Wextra -Wno-format-truncation ${TESTBUILD_DEFINITIONS} -DLOG_AUTOFLUSH")
		set(CMAKE_CXX_FLAGS_FASTTEST "       -Ofast -Wall -Wextra -Wno-format-truncation ${TESTBUILD_DEFINITIONS}")

		if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
			add_definitions("-Wno-invalid-source-encoding")
		endif ()
		target_link_libraries(circle.library crypt)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()

	# Suppose that UNIX hosts support epoll by default EPOLL>>>
	if (NOT CYGWIN)
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
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG")
	set(CMAKE_EXE_LINKER_FLAGS_TEST "/DEBUG")

	# EPOLL>>> ...and suppose that Windows does not
	option(HAS_EPOLL "Does system support epoll call?" OFF)
	target_link_libraries(circle.library DbgHelp)
endif ()

if (HAS_EPOLL)
	message(STATUS "Turning ON epoll")
	add_definitions("-DHAS_EPOLL")
else ()
	message(STATUS "Turning OFF epoll")
endif ()

message(STATUS "CXX compiler ID is ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "System processor is ${CMAKE_SYSTEM_PROCESSOR}")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	endif ()
endif ()

# Main executable file.
set(MAIN_EXECUTABLE src/main.cpp)

add_executable(circle ${MAIN_EXECUTABLE})
if (NOT UNIX)
	set_target_properties(circle PROPERTIES CXX_STANDARD 14)
endif ()
target_link_libraries(circle circle.library)

# Testing part
option(BUILD_TESTS "Build and run tests." YES)
if (BUILD_TESTS)
	add_subdirectory(tests)
endif ()

# vim: set ts=4 sw=4 ai tw=0 noet syntax=cmake :

