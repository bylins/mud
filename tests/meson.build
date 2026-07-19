message('Seting up tests...')
if gtest_opt == 'builtin'
    gtest_sub = subproject('gtest', default_options: ['default_library=static'])
    gtest_dep = gtest_sub.get_variable('gtest_dep')
elif gtest_opt == 'system'
    gtest_dep = dependency('gtest', required: true, allow_fallback: false)
else
    gtest_dep = dependency('gtest', required: true, fallback: ['gtest', 'gtest_dep'], default_options: ['default_library=static'])
endif

test_sources = files(
    'main.cpp',
    'vedun.enum_names.cpp',
    'vedun.dom.cpp',
    'vedun.enum_registry.cpp',
    'vedun.scheme.cpp',
    'vedun.datanode_mutation.cpp',
    'vedun.element_find.cpp',
    'msg_container.cdata.cpp',
    'guilds.loader.cpp',
    'socials.loader.cpp',
    'subcmd_resolver.cpp',
    'spell_wear_off_msg.wrapper.cpp',
    'blocking.queue.cpp',
    'boards.changelog.cpp',
    'boards.news.cpp',
    'msdp.parser.cpp',
    'msdp.builder.cpp',
    'char.affects.cpp',
    'dot.death.credit.cpp',
    'affects.loader.cpp',
    'random.noise.cpp',
    'potion.duration.cpp',
    'char.leaders.cpp',
    'char.morph.copy.cpp',
    'obj.copy.cpp',
    'spell_item_convert.cpp',
    'act.makefood.cpp',
    'utils.editor.cpp',
    'utils.string.cpp',
    'utils.encoding.cpp',
    'fight.penalties.cpp',
    'bonus.command.parser.cpp',
    'quested.cpp',
    'sprintbitwd.cpp',
    'triggers.list.cpp',
    'compact.trie.cpp',
    'compact.trie.iterators.cpp',
    'yaml.save.encoding.cpp',
    'koi8r.yaml.emitter.cpp',
    'compact.trie.prefixes.cpp',
    'thread_pool.cpp',
    'world_data_source_manager.cpp',
    'flag_data.cpp',
    'bitset_flags.cpp',
    'extra_description.cpp',
    'trigger_indenter.cpp',
    'trigger_script_parser.cpp',
    'trigger_obj_load_index.cpp',
    'yaml_loader_flag_packing.cpp',
    'yaml_mob_saves_resistances.cpp',
    'world_checksum_serialization.cpp',
    'dg_string_matching.cpp',
    'dg_var_context.cpp',
    'mob_classes.loader.cpp',
    'world_load_order.cpp',
    'obj_decay_manager.cpp',
    # char.utilities.cpp removed: test_utils::CharacterBuilder is now an
    # alias for simulator::CharacterBuilder (char.utilities.hpp is a shim);
    # the implementation is compiled below as
    # ../src/simulator/character_builder.cpp.
    'exit_is_empty.cpp',
    'msg_container.cpp',
    'spell_messages.cpp',
    'skill_messages.cpp',
    'fight_messages.cpp',
    'file_crc.cpp',
    'buffered_file_writer.cpp',
    'where.format.cpp',
    # Balance simulator (issue #2967)
    'random.seed.cpp',
    'file_event_sink.cpp',
    'event_sink_global.cpp',
    'character_builder.cpp',
    # CharacterBuilder simulator source compiled straight into the test
    # binary (it is not in circle_lib -- it is part of mud-sim).
    '../src/simulator/character_builder.cpp',
)

# scenario_loader parses YAML -- test + source only when yaml is enabled.
if yaml_opt != 'disabled'
    test_sources += files(
        'scenario_loader.cpp',
        '../src/simulator/scenario_loader.cpp',
    )
endif

fs = import('fs')

files_to_copy = [
    ['../lib/cfg/mechanics/group_exp_handicap.xml', 'cfg/mechanics/group_exp_handicap.xml'],
    ['data/boards/changelog.sample',  'data/boards/changelog.sample'],
    ['data/boards/news.sample',       'data/boards/news.sample'],
    ['data/mob_classes/test_initial.xml', 'data/mob_classes/test_initial.xml'],
    ['data/mob_classes/test_updated.xml', 'data/mob_classes/test_updated.xml'],
    ['data/msg_container/test_messages.xml', 'data/msg_container/test_messages.xml'],
]

copied_targets = []
foreach pair : files_to_copy
    src_rel = pair[0]
    dst_rel = pair[1]    
    src_path = meson.current_source_dir() / src_rel
    
    full_dst = join_paths(meson.project_build_root(), dst_rel)
    stamp_name = dst_rel.underscorify() + '.copy_stamp'
    copied = custom_target(
        'copy_test_data_' + dst_rel.underscorify(),
        input: src_path,
        output: stamp_name,
        command: [py3, files('../tools/meson/copy_test_data.py'), '@INPUT@', full_dst, '@OUTPUT@'],
        build_by_default: true
    )
    copied_targets += copied
endforeach

test_compile_args = []
if host_machine.system() == 'windows'
    test_compile_args += '-DFMT_UNICODE=0'
endif
if get_option('b_coverage')
    test_compile_args += '-DENABLE_GCOV_FLUSH'
endif

test_exe = executable(
    'tests',
    test_sources,
    include_directories: include_directories('.'),
    dependencies: [circle_dep, gtest_dep, threads_dep],
    cpp_args: test_compile_args + (host_machine.system() != 'windows' ? ['-Wno-conversion-null', '-Wno-sign-compare'] : []),
    override_options: profile_target_opts
)

test('bylins_tests_suite',
     test_exe,
     protocol: 'gtest',
     timeout: 180,
     verbose: true,
     depends: copied_targets,
     workdir: meson.project_build_root()
)
