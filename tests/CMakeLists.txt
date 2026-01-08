cmake_minimum_required(VERSION 3.10)


# Поиск GTest: сначала пробуем find_package (работает на большинстве дистрибутивов),
# затем ручной поиск для нестандартных установок
if (NOT GTEST_INCLUDE OR NOT GTEST_LIB)
	# Сначала пробуем стандартный поиск через find_package
	find_package(GTest QUIET)
	if (GTest_FOUND OR GTEST_FOUND)
		message(STATUS "GTest found via find_package")
		# Современный CMake (3.20+) использует GTest::gtest target
		if (TARGET GTest::gtest)
			set(GTEST_LIB GTest::gtest)
			get_target_property(GTEST_INCLUDE GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
		else ()
			set(GTEST_LIB "${GTEST_LIBRARIES}")
			set(GTEST_INCLUDE "${GTEST_INCLUDE_DIRS}")
		endif ()
	else ()
		# Ручной поиск для нестандартных установок
		message(STATUS "GTest not found via find_package, trying manual search...")

		# Ищем заголовочные файлы
		find_path(GTEST_INCLUDE_DIR gtest/gtest.h
			PATHS
				$ENV{GTEST_ROOT}/include
				/usr/include
				/usr/local/include
		)

		# Ищем библиотеку в стандартных путях для разных дистрибутивов
		find_library(GTEST_LIB gtest
			PATHS
				$ENV{GTEST_ROOT}/lib
				/usr/lib
				/usr/lib64
				/usr/local/lib
				/usr/local/lib64
		)

		if (GTEST_INCLUDE_DIR AND GTEST_LIB)
			set(GTEST_INCLUDE "${GTEST_INCLUDE_DIR}")
			message(STATUS "GTest found manually: ${GTEST_LIB}")
		else ()
			message(FATAL_ERROR "GTest not found. Install libgtest-dev (Debian/Ubuntu), gtest-devel (Fedora/RHEL), or gtest (Arch)")
		endif ()
	endif ()
endif ()

if (GTEST_INCLUDE)
	include_directories(SYSTEM ${GTEST_INCLUDE})
endif ()

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

add_test(NAME all COMMAND tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Configure LeakSanitizer to use suppression file and set working directory
set_tests_properties(all PROPERTIES
	ENVIRONMENT "LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/lsan.supp"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

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
