cmake_minimum_required(VERSION 2.8)

enable_testing()

find_path(GTEST_ROOT include/gtest/gtest.h PATHS $ENV{GTEST_ROOT})
set(GTEST_INCLUDE ${GTEST_ROOT}/include)
find_library(GTEST_LIB gtest PATHS ${GTEST_ROOT}/lib)

include_directories(${GTEST_INCLUDE})

add_executable(msdp.tests msdp.tests.cpp)
target_link_libraries(msdp.tests circle.library ${GTEST_LIB})
add_test(msdp msdp.tests)

add_custom_target(checks COMMAND ${CMAKE_CTEST_COMMAND} -V DEPENDS msdp.tests)