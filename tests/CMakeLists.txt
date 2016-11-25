cmake_minimum_required(VERSION 2.8)

enable_testing()

find_path(GTEST_ROOT include/gtest/gtest.h PATHS $ENV{GTEST_ROOT} NO_DEFAULT_PATH)
set(GTEST_INCLUDE ${GTEST_ROOT}/include)
find_library(GTEST_LIB gtest PATHS ${GTEST_ROOT}/lib)

include_directories(system ${GTEST_INCLUDE})

set(TESTS
	msdp.parser.cpp
	char.affects.cpp
	obj.copy.cpp
	act.makefood.cpp
	utils.editor.cpp
	fight.penalties.cpp)
set(UTILITIES
	char.utilities.hpp
	char.utilities.cpp)
add_executable(tests main.cpp ${TESTS} ${UTILITIES})
set_target_properties(tests PROPERTIES FOLDER "Utility targets")
set_target_properties(tests PROPERTIES CXX_STANDARD 14)
target_link_libraries(tests circle.library ${GTEST_LIB})
if (NOT WIN32)
	target_link_libraries(tests pthread)
endif ()

add_test(all tests)

add_custom_target(checks COMMAND ${CMAKE_CTEST_COMMAND} -V DEPENDS tests)

add_custom_target(tests.file.grouping
	DEPENDS ${CMAKE_SOURCE_DIR}/lib.template/misc/grouping
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib.template/misc/grouping ${CMAKE_CURRENT_BINARY_DIR}/misc/grouping
	COMMENT "Copy lib/misc/grouping for tests.")
set_target_properties(tests.file.grouping PROPERTIES FOLDER "Utility targets")

add_dependencies(checks tests tests.file.grouping)

# vim: set ts=4 sw=4 ai tw=0 noet syntax=cmake :
