cmake_minimum_required(VERSION 2.8)

enable_testing()

option(MANUAL_GTEST_LOCATION "Ifset then you have to provide proper value of environment value GTEST_ROOT" ON)

if (NOT GTEST_INCLUDE OR NOT GTEST_LIB)
	if (MANUAL_GTEST_LOCATION)
		set(GTEST_LIBRARY_NAME "gtest")
		set(GTEST_LIBRARY_SUBDIR_NAME "lib")
		set(GTEST_INCLUDE_FILE "include/gtest/gtest.h")

		find_path(GTEST_ROOT ${GTEST_INCLUDE_FILE} PATHS $ENV{GTEST_ROOT} /usr/ NO_DEFAULT_PATH)
		set(GTEST_INCLUDE ${GTEST_ROOT}/include)
		find_library(GTEST_LIB ${GTEST_LIBRARY_NAME} PATHS ${GTEST_ROOT}/${GTEST_LIBRARY_SUBDIR_NAME} NO_DEFAULT_PATH)

		if (NOT GTEST_LIB OR NOT GTEST_INCLUDE)
			set(MISSING_FILES "")
			if (NOT GTEST_LIB)
				set(MISSING_FILES "\n  - ${GTEST_LIBRARY_NAME} library in subdirectory '${GTEST_LIBRARY_SUBDIR_NAME}'")
			endif ()

			if (NOT GTEST_INCLUDE)
				set(MISSING_FILES "${MISSING_FILES}\n  - ${GTEST_INCLUDE_FILE}")
			endif ()

			message(WARNING "Value of GTEST_ROOT is incorrect. At least the following files weren't found "
				"in the GTest root directory ${GTEST_ROOT}:${MISSING_FILES}\nTrying to find package automatically.")

			find_package(GTest)
			if (NOT GTEST_FOUND)
				message(FATAL_ERROR "Automatic search for GTest package failed as well.")
			else ()
				set(GTEST_LIB "${GTEST_LIBRARIES}")
				set(GTEST_INCLUDE "${GTEST_INCLUDE_DIRS}")
			endif ()
		endif ()
	else ()
		find_package(GTest)
		if (NOT GTEST_FOUND)
			message(FATAL_ERROR "Automatic search for GTest package failed as well.")
		else ()
			set(GTEST_LIB "${GTEST_LIBRARIES}")
			set(GTEST_INCLUDE "${GTEST_INCLUDE_DIRS}")
		endif ()
	endif ()
endif ()

include_directories(system ${GTEST_INCLUDE})

if (UNIX)
	add_definitions("-Wno-conversion-null -Wno-sign-compare")
endif ()

set(TESTS
		spell_wear_off_msg.wrapper.cpp
		blocking.queue.cpp
		boards.changelog.cpp
		boards.news.cpp
		msdp.parser.cpp
		msdp.builder.cpp
		char.affects.cpp
		char.leaders.cpp
		obj.copy.cpp
		act.makefood.cpp
		utils.editor.cpp
		utils.string.cpp
		fight.penalties.cpp
		bonus.command.parser.cpp
		quested.cpp
		sprintbitwd.cpp
		triggers.list.cpp
		sprintbitwd.cpp
		bonus.command.parser.cpp
		radix.trie.cpp
		compact.trie.cpp
		compact.trie.iterators.cpp
		compact.trie.prefixes.cpp)
set(UTILITIES
		char.utilities.hpp
		char.utilities.cpp)
set(ADDITIONAL_FILES
		data/boards/changelog.sample
		data/boards/news.sample)
source_group("Tests data" FILES ${ADDITIONAL_FILES})
source_group("Tests" FILES ${TESTS})
source_group("Tests helpers" FILES ${UTILITIES})
add_executable(tests main.cpp ${TESTS} ${UTILITIES} ${ADDITIONAL_FILES})

set_target_properties(tests PROPERTIES CXX_STANDARD 17)
target_link_libraries(tests circle.library ${GTEST_LIB})
if (NOT WIN32)
	target_link_libraries(tests pthread)
endif ()

add_test(all tests)

add_custom_target(checks COMMAND ${CMAKE_CTEST_COMMAND} -V DEPENDS tests)

add_custom_target(tests.misc.grouping
		DEPENDS ${CMAKE_SOURCE_DIR}/lib.template/misc/grouping
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib.template/misc/grouping ${CMAKE_CURRENT_BINARY_DIR}/misc/grouping
		COMMENT "Copy lib/misc/grouping for tests.")
set_target_properties(tests.misc.grouping PROPERTIES FOLDER "Utility targets/Tests data")
add_custom_target(tests.data.boards.changelog_sample
		DEPENDS ${CMAKE_SOURCE_DIR}/tests/data/boards/changelog.sample
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tests/data/boards/changelog.sample ${CMAKE_CURRENT_BINARY_DIR}/data/boards/changelog.sample
		COMMENT "Copy tests/data/boards/changelog.sample for tests.")
set_target_properties(tests.data.boards.changelog_sample PROPERTIES FOLDER "Utility targets/Tests data")
add_custom_target(tests.data.boards.news_sample
		DEPENDS ${CMAKE_SOURCE_DIR}/tests/data/boards/news.sample
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tests/data/boards/news.sample ${CMAKE_CURRENT_BINARY_DIR}/data/boards/news.sample
		COMMENT "Copy tests/data/boards/news.sample for tests.")
set_target_properties(tests.data.boards.news_sample PROPERTIES FOLDER "Utility targets/Tests data")

add_dependencies(tests tests.misc.grouping tests.data.boards.changelog_sample tests.data.boards.news_sample)
add_dependencies(checks tests)

# vim: set ts=4 sw=4 ai tw=0 noet syntax=cmake :
