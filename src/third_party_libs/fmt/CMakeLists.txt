cmake_minimum_required(VERSION 3.1...3.18)

project(FMT CXX)

#args.h chrono.h color.h compile.h os.h ostream.h printf.h std.h xchar.h
set(FMT_HEADERS core.h format.h ranges.h format-inl.h)
set(FMT_SOURCES format.cc)

add_library(fmt ${FMT_SOURCES} ${FMT_HEADERS})
add_library(fmt::fmt ALIAS fmt)

target_include_directories(fmt PUBLIC .)

