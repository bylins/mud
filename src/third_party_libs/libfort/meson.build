fort_args = []
if not get_option('fort_wchar')
    fort_args += '-DFT_CONGIG_DISABLE_WCHAR'
endif

if not get_option('fort_utf8')
    fort_args += '-DFT_CONGIG_DISABLE_UTF8'
endif

if get_option('fort_test')
    fort_args += '-DFT_TEST_BUILD=1'
endif

fort_lib = static_library('fort',
    'fort.cpp',
    cpp_args: fort_args,
    include_directories: include_directories('.')
)

fort_dep = declare_dependency(
    link_with: fort_lib,
    include_directories: include_directories('.')
)
