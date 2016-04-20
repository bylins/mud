cmake_minimum_required(VERSION 2.8)

set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required

project(bylins CXX)

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
    src/bonus.cpp
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
    src/craft.cpp
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
    src/sets_drop.cpp
    src/shop_ext.cpp
    src/skills.cpp
    src/spam.cpp
    src/spec_assign.cpp
    src/spec_procs.cpp
    src/spell_parser.cpp
    src/spells.cpp
    src/stuff.cpp
    src/structs.cpp
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
    src/bonus.h
    src/cache.hpp
    src/celebrates.hpp
    src/char.hpp
	src/char_obj_utils.inl
    src/char_player.hpp
    src/comm.h
    src/conf.h
    src/config.hpp
    src/constants.h
    src/corpse.hpp
    src/craft.hpp
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
    src/sets_drop.hpp
    src/shop_ext.hpp
    src/skills.h
    src/spam.hpp
    src/spell_parser.hpp
    src/spells.h
    src/structs.h
    src/stuff.hpp
    src/sysdep.h
    src/telnet.h
    src/title.hpp
    src/top.h
    src/utils.h)

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

set(SCRIPTING_HEADERS src/scripting.hpp)
set(SCRIPTING_SOURCES src/scripting.cpp)

# Turns on/off scripting. Use the option like:
#     $ cmake -DSCRIPTING=NO ..
# to turn scripting off.
option(SCRIPTING "Turns on/off scripting engine" YES)
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

# Versioning
# Try to find hg executable, retrive revision if found and generate version.cpp from template
set(VERSIONING_FILES src/versioning.cmake src/version.cpp.in)
find_program(HG hg DOC "Mercurial executable file")
# invalidate template
add_custom_target(versioning ALL COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_SOURCE_DIR}/src/version.cpp.in
    SOURCES ${VERSIONING_FILES})
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/src/version.cpp
    COMMAND ${CMAKE_COMMAND} -DVERSION_INPUT=${CMAKE_SOURCE_DIR}/src/version.cpp.in -DVERSION_OUTPUT=${CMAKE_SOURCE_DIR}/src/version.cpp -DHG=${HG} -P ${CMAKE_SOURCE_DIR}/src/versioning.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/src/version.cpp.in)

set(CRAFT_GENERAL_FILES
    lib.template/misc/craft/index.xml)
set(CRAFT_MATERIAL_FILES
    lib.template/misc/craft/tough_leather.xml
    lib.template/misc/craft/soft_leather.xml
    lib.template/misc/craft/inwrought_flax.xml)
set(CRAFT_CRAFT_FILES
    lib.template/misc/craft/craft1.xml)
set(CRAFT_TEMPLATE_FILES
    lib.template/misc/craft/material_template.xml
    lib.template/misc/craft/prototype_template.xml)
set(CRAFT_PROTOTYPES)

source_group("Misc\\Craft" FILES ${CRAFT_GENERAL_FILES})
source_group("Misc\\Craft\\Templates" FILES ${CRAFT_TEMPLATE_FILES})
source_group("Misc\\Craft\\Materials" FILES ${CRAFT_MATERIAL_FILES})
source_group("Misc\\Craft\\Crafts" FILES ${CRAFT_CRAFT_FILES})

set(CRAFT_FILES ${CRAFT_GENERAL_FILES} ${CRAFT_MATERIAL_FILES} ${CRAFT_CRAFT_FILES} ${CRAFT_TEMPLATE_FILES})

set(MISC_FILES
    lib.template/misc/noob_help.xml
	lib.template/misc/configuration.xml)

source_group("Misc" FILES ${MISC_FILES})

set(CIRCLE_FILES ${SOURCES} ${HEADERS} readme.markdown ${CRAFT_FILES} ${MISC_FILES})

# Sort source and header files. Just to convenience.
list(SORT CIRCLE_FILES)

# Main executable file.
add_executable(circle ${CIRCLE_FILES})
add_dependencies(circle versioning)

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

set(Boost_NO_SYSTEM_PATHS ON)
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
    set(Boost_COMPILER "-vc140")
    target_compile_options(circle PUBLIC /MP /MDd)
    target_compile_options(circle PUBLIC $<$<CONFIG:Debug>:/Zi /Od>)
    target_compile_options(circle PUBLIC $<$<CONFIG:Release>:/O2>)
    target_compile_options(circle PUBLIC $<$<CONFIG:Test>:/Zi /Od>)
    target_compile_options(circle PUBLIC $<$<CONFIG:FastTest>:/O2>)
    set_target_properties(circle PROPERTIES LINK_FLAGS_DEBUG "/DEBUG")
    set_target_properties(circle PROPERTIES LINK_FLAGS_TEST "/DEBUG")
endif ()
find_package(Boost 1.54 REQUIRED COMPONENTS ${BOOST_MODULES})

# ZLib
set(ZLIB_ROOT $ENV{ZLIB_ROOT})
find_package(ZLIB REQUIRED)

# Iconv
find_path(ICONV_INCLUDE_DIR iconv.h) 
find_library(ICONV_LIBRARIES NAMES iconv libiconv libiconv-2 c)
if (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES) 
    message(STATUS "Iconv found. Turning it ON")
    set(ICONV_FOUND TRUE)
else ()
    message(STATUS "Iconv not found. Turning it OFF")
endif () 

include_directories(src
    system ${Boost_INCLUDE_DIRS}
    system ${ZLIB_INCLUDE_DIRS}
    system ${PYTHON_INCLUDE_DIRS})
target_link_libraries(circle
    ${ZLIB_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES})

if (ICONV_FOUND)
    add_definitions("-DHAVE_ICONV")
    include_directories(system ${ICONV_INCLUDE_DIR})
    target_link_libraries(circle ${ICONV_LIBRARIES})
endif ()

# Prepare options for test build
set(TESTBUILD_DEFINITIONS "-DNOCRYPT -DTEST_BUILD")

if (CMAKE_HOST_UNIX)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_RELEASE  "-ggdb3 -Og    -Wall -Wextra -DBOOST_ENABLE_ASSERT_HANDLER")
        set(CMAKE_CXX_FLAGS_DEBUG    "-ggdb3 -O0    -Wall -Wextra -D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC")
        set(CMAKE_CXX_FLAGS_TEST     "       -O0    -Wall -Wextra ${TESTBUILD_DEFINITIONS} -DLOG_AUTOFLUSH")
        set(CMAKE_CXX_FLAGS_FASTTEST "       -Ofast -Wall -Wextra ${TESTBUILD_DEFINITIONS}")

        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            add_definitions("-Wno-invalid-source-encoding")
        endif ()
        target_link_libraries(circle crypt)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=gnu++14")
    endif()

    # Suppose that UNIX hosts support epoll by default EPOLL>>>
    if (NOT CYGWIN)
        option(HAS_EPOLL "Does system support epoll call?" ON)
    else ()
        # EPOLL>>> ... if it is not Cygwin ... EPOLL>>>
        option(HAS_EPOLL "Does system support epoll call?" OFF)
    endif ()

    # On Unix systems will make changelog file.
    add_custom_target(changelog ALL)
    add_custom_command(TARGET changelog
        COMMAND LANGUAGE=en_US.UTF-8 HGENCODING=utf-8 hg log -v -M > changelog)
elseif (CMAKE_HOST_WIN32)
    add_definitions("-D_SCL_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS_RELEASE  "-DBOOST_ENABLE_ASSERT_HANDLER")
    set(CMAKE_CXX_FLAGS_DEBUG    "")
    set(CMAKE_CXX_FLAGS_TEST     "${TESTBUILD_DEFINITIONS} -DLOG_AUTOFLUSH")
    set(CMAKE_CXX_FLAGS_FASTTEST "${TESTBUILD_DEFINITIONS}")

    # EPOLL>>> ...and suppose that Windows does not
    option(HAS_EPOLL "Does system support epoll call?" OFF)
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

# vim: set ts=4 sw=4 ai tw=0 et syntax=cmake :
