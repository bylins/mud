cmake_minimum_required(VERSION 3.0)

project(bylins)

set(SOURCES
    src/act.comm.cpp
    src/act.informative.cpp
    src/act.item.cpp
    src/act.movement.cpp
    src/act.offensive.cpp
    src/act.other.cpp
    src/act.social.cpp
    src/act.wizard.cpp
    src/AffectHandler.cpp
    src/alias.cpp
    src/auction.cpp
    src/ban.cpp
    src/birth_places.cpp
    src/boards.cpp
    src/cache.cpp
    src/celebrates.cpp
    src/char.cpp
    src/char_player.cpp
    src/class.cpp
    src/color.cpp
    src/comm.cpp
    src/config.cpp
    src/constants.cpp
    src/corpse.cpp
    src/db.cpp
    src/deathtrap.cpp
    src/depot.cpp
    src/description.cpp
    src/dg_comm.cpp
    src/dg_db_scripts.cpp
    src/dg_event.cpp
    src/dg_handler.cpp
    src/dg_misc.cpp
    src/dg_mobcmd.cpp
    src/dg_objcmd.cpp
    src/dg_olc.cpp
    src/dg_scripts.cpp
    src/dg_triggers.cpp
    src/dg_wldcmd.cpp
    src/dictionary.cpp
    src/diskio.cpp
    src/dps.cpp
    src/exchange.cpp
    src/ext_money.cpp
    src/features.cpp
    src/fight.cpp
    src/fight_hit.cpp
    src/fight_stuff.cpp
    src/file_crc.cpp
    src/genchar.cpp
    src/glory_const.cpp
    src/glory.cpp
    src/glory_misc.cpp
    src/graph.cpp
    src/handler.cpp
    src/help.cpp
    src/house.cpp
    src/house_exp.cpp
    src/im.cpp
    src/interpreter.cpp
    src/item.creation.cpp
    src/limits.cpp
    src/liquid.cpp
    src/magic.cpp
    src/mail.cpp
    src/map.cpp
    src/medit.cpp
    src/mobact.cpp
    src/mobmax.cpp
    src/mob_stat.cpp
    src/modify.cpp
    src/morph.cpp
    src/named_stuff.cpp
    src/name_list.cpp
    src/names.cpp
    src/noob.cpp
    src/obj.cpp
    src/obj_enchant.cpp
    src/objsave.cpp
    src/objsave_ext.cpp
    src/obj_sets.cpp
    src/obj_sets_olc.cpp
    src/obj.spell.cpp
    src/oedit.cpp
    src/olc.cpp
    src/parcel.cpp
    src/parse.cpp
    src/password.cpp
    src/pk.cpp
    src/player_races.cpp
    src/poison.cpp
    src/privilege.cpp
    src/pugixml.cpp
    src/quest.cpp
    src/quested.cpp
    src/random.cpp
    src/redit.cpp
    src/remember.cpp
    src/reset_stats.cpp
    src/room.cpp
    src/scripting.cpp
    src/sets_drop.cpp
    src/shop_ext.cpp
    src/skills.cpp
    src/spam.cpp
    src/spec_assign.cpp
    src/spec_procs.cpp
    src/spell_parser.cpp
    src/spells.cpp
    src/stuff.cpp
    src/title.cpp
    src/top.cpp
    src/utils.cpp
    src/version.cpp
    src/weather.cpp
    src/zedit.cpp)

set(HEADERS
    src/AffectHandler.hpp
    src/auction.h
    src/ban.hpp
    src/birth_places.hpp
    src/boards.h
    src/cache.hpp
    src/celebrates.hpp
    src/char.hpp
    src/char_player.hpp
    src/comm.h
    src/conf.h
    src/constants.h
    src/corpse.hpp
    src/db.h
    src/deathtrap.hpp
    src/depot.hpp
    src/description.h
    src/dg_event.h
    src/dg_olc.h
    src/dg_scripts.h
    src/dictionary.hpp
    src/diskio.h
    src/double_map.hpp
    src/dps.hpp
    src/exchange.h
    src/ext_money.hpp
    src/features.hpp
    src/fight.h
    src/fight_local.hpp
    src/file_crc.hpp
    src/genchar.h
    src/glory_const.hpp
    src/glory.hpp
    src/glory_misc.hpp
    src/handler.h
    src/help.hpp
    src/house_exp.hpp
    src/house.h
    src/im.h
    src/interpreter.h
    src/item.creation.hpp
    src/liquid.hpp
    src/magic.h
    src/mail.h
    src/map.hpp
    src/mobmax.hpp
    src/mob_stat.hpp
    src/modify.h
    src/morph.hpp
    src/named_stuff.hpp
    src/name_list.hpp
    src/noob.hpp
    src/obj_enchant.hpp
    src/obj.hpp
    src/objsave.h
    src/obj_sets.hpp
    src/obj_sets_stuff.hpp
    src/olc.h
    src/parcel.hpp
    src/parse.hpp
    src/password.hpp
    src/pk.h
    src/player_i.hpp
    src/player_races.hpp
    src/poison.hpp
    src/privilege.hpp
    src/pugiconfig.hpp
    src/pugixml.hpp
    src/quested.hpp
    src/quest.hpp
    src/random.hpp
    src/remember.hpp
    src/reset_stats.hpp
    src/room.hpp
    src/screen.h
    src/scripting.hpp
    src/sets_drop.hpp
    src/shop_ext.hpp
    src/skills.h
    src/spam.hpp
    src/spells.h
    src/structs.h
    src/stuff.hpp
    src/sysdep.h
    src/telnet.h
    src/title.hpp
    src/top.h
    src/utils.h)

# Build types
set(CMAKE_CONFIGURATION_TYPES Release Debug Test FastTest)

# Default flags
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_TEST "")
set(CMAKE_CXX_FLAGS_FASTTEST "")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
set(CMAKE_EXE_LINKER_FLAGS_TEST "")
set(CMAKE_EXE_LINKER_FLAGS_FASTTEST "")

add_executable(circle ${SOURCES} ${HEADERS})
add_definitions("-DBOOST_PYTHON_STATIC_LIB")

## Look for required components
# Python
find_package(PythonLibs)
if (NOT PYTHONLIBS_FOUND)
    message(ERROR "Python not found.")
endif ()

# Boost
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS ON)

## In case of you have more than one Visual Studio and want to use specific version, uncomment the following three lines and specify compiler you want
if (CMAKE_HOST_WIN32)
    set(Boost_COMPILER "-vc140")
    target_compile_options(circle PUBLIC /MP /MDd)
    target_compile_options(circle PUBLIC $<$<CONFIG:Debug>:/Od>)
    target_compile_options(circle PUBLIC $<$<CONFIG:Release>:/O2>)
    target_compile_options(circle PUBLIC $<$<CONFIG:Test>:/Od>)
    target_compile_options(circle PUBLIC $<$<CONFIG:FastTest>:/O2>)
endif ()
find_package(Boost 1.59 REQUIRED COMPONENTS python locale)

# ZLib
set(ZLIB_ROOT $ENV{ZLIB_ROOT})
find_package(ZLIB REQUIRED)

include_directories(src
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS})
target_link_libraries(circle
    ${ZLIB_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES})

if (CMAKE_HOST_UNIX)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_RELEASE "-ggdb3 -O2 -Wall -Wextra -Wno-unused-parameter -DBOOST_ENABLE_ASSERT_HANDLER -DHAVE_ICONV")
        set(CMAKE_CXX_FLAGS_DEBUG "-D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC")
        set(CMAKE_CXX_FLAGS_TEST  "-ggdb3 -Wall -Wextra -Wno-unused-parameter -DNOCRYPT -DTEST_BUILD -DLOG_AUTOFLUSH")
        set(CMAKE_CXX_FLAGS_FASTTEST "-ggdb3 -O3 -Wall -DNOCRYPT -DTEST_BUILD")

        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            add_definitions("-Wno-invalid-source-encoding")
        endif ()
        target_link_libraries(circle crypt)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=gnu++0x")
    endif()

    # Suppose that UNIX hosts support epoll by default EPOLL>>>
    option(HAS_EPOLL "Does system support epoll call?" ON)
elseif (CMAKE_HOST_WIN32)
        set(CMAKE_CXX_FLAGS_RELEASE "-DBOOST_ENABLE_ASSERT_HANDLER")
        set(CMAKE_CXX_FLAGS_DEBUG "")
        set(CMAKE_CXX_FLAGS_TEST  "-DNOCRYPT -DTEST_BUILD -DLOG_AUTOFLUSH")
        set(CMAKE_CXX_FLAGS_FASTTEST "-DNOCRYPT -DTEST_BUILD")

    # EPOLL>>> ...and suppose that Windows does not
    option(HAS_EPOLL "Does system support epoll call?" OFF)
endif ()

if (HAS_EPOLL)
    add_definitions("-DHAS_EPOLL")
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

# Copy lib.template content here
add_custom_target(libTemplate)
if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    add_custom_command(TARGET libTemplate
        POST_BUILD
        DEPENDS ${CMAKEBINARY_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/lib.template ${CMAKE_BINARY_DIR}/lib)
endif ()

add_dependencies(circle libTemplate)

# vim: set ts=4 sw=4 ai tw=0 et syntax=cmake :
