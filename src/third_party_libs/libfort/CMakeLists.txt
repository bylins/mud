option(FORT_HAVE_WCHAR "Enable wchar support" OFF)
option(FORT_HAVE_UTF8 "Enable UTF8 support" OFF)
option(FORT_TEST_BUILD "Export some internal symbols for tests" OFF)

project(fort CXX)

add_library(fort
        fort.cpp)

target_include_directories(fort
    PUBLIC
        .
)

if(NOT FORT_HAVE_WCHAR)
    target_compile_definitions(fort
        PRIVATE
            -DFT_CONGIG_DISABLE_WCHAR
    )
endif()

if(NOT FORT_HAVE_UTF8)
    target_compile_definitions(fort
        PRIVATE
            -DFT_CONGIG_DISABLE_UTF8
    )
endif()

if(FORT_TEST_BUILD)
    target_compile_definitions(fort
        PRIVATE
            -DFT_TEST_BUILD=1
    )
endif()
