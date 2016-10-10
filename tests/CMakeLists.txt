cmake_minimum_required(VERSION 2.8)

enable_testing()

find_path(GTEST_ROOT include/gtest/gtest.h PATHS $ENV{GTEST_ROOT} NO_DEFAULT_PATH)
set(GTEST_INCLUDE ${GTEST_ROOT}/include)
find_library(GTEST_LIB gtest PATHS ${GTEST_ROOT}/lib)

include_directories(system ${GTEST_INCLUDE})

add_executable(mud.tests main.cpp msdp.parser.cpp char.affects.cpp)
set_property(TARGET mud.tests PROPERTY CXX_STANDARD 14)
target_link_libraries(mud.tests circle.library ${GTEST_LIB})
if (NOT WIN32)
    target_link_libraries(mud.tests pthread)
endif ()

add_test(all mud.tests)

add_custom_target(checks COMMAND ${CMAKE_CTEST_COMMAND} -V DEPENDS mud.tests)

# vim: set ts=4 sw=4 ai tw=0 et syntax=cmake :
