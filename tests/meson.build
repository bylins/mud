gtest_dep = dependency('gtest', main: false, required: true)
threads_dep = dependency('threads')

test_sources = files(
    'main.cpp',
    'spell_wear_off_msg.wrapper.cpp',
    'blocking.queue.cpp',
    'boards.changelog.cpp',
    'boards.news.cpp',
    'msdp.parser.cpp',
    'msdp.builder.cpp',
    'char.affects.cpp',
    'char.leaders.cpp',
    'char.morph.copy.cpp',
    'obj.copy.cpp',
    'act.makefood.cpp',
    'utils.editor.cpp',
    'utils.string.cpp',
    'fight.penalties.cpp',
    'bonus.command.parser.cpp',
    'quested.cpp',
    'sprintbitwd.cpp',
    'triggers.list.cpp',
    'radix.trie.cpp',
    'compact.trie.cpp',
    'compact.trie.iterators.cpp',
    'compact.trie.prefixes.cpp',
    'char.utilities.cpp'
)

fs = import('fs')

files_to_copy = [
    ['../lib.template/misc/grouping', 'misc/grouping'],
    ['data/boards/changelog.sample',  'data/boards/changelog.sample'],
    ['data/boards/news.sample',       'data/boards/news.sample'],
]

copied_targets = []

foreach pair : files_to_copy
    src_rel = pair[0]
    dst_rel = pair[1]

    src_file = files(src_rel)
    dst_dir = fs.parent(dst_rel)
    dst_name = fs.name(dst_rel)

    full_dst_dir = join_paths(meson.project_source_root(), dst_dir)
    full_dst = join_paths(meson.project_source_root(), dst_rel)

    target_name = 'copy_test_data_' + dst_name.replace('.', '_').replace('-', '_')

    copied = custom_target(
        target_name,
        input: src_file,
        output: dst_name,
        command: [
            'sh', '-c',
            'mkdir -p "@0@" && cp -r "@INPUT@" "@1@"'.format(full_dst_dir, full_dst)
        ],
        build_by_default: true,
        build_always_stale: true
    )

    copied_targets += copied
endforeach

test_exe = executable(
    'tests',
    test_sources,
    include_directories: include_directories('.'),
    dependencies: [circle_dep, gtest_dep, threads_dep],
    cpp_args: host_machine.system() != 'windows' ? ['-Wno-conversion-null', '-Wno-sign-compare'] : []
)

alias_target('tests', [test_exe] + copied_targets)

test('bylins_tests_suite',
     test_exe,
     verbose: true,
     depends: copied_targets)