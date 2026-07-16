check_dep = dependency('check', required: true)

test_c_pch = '../' + c_pch
test_inc = include_directories('../src', '..')

path_test = executable(
    'colon-path',
    [
        'colon-path_test.c',
        '../src/colon-path.c',
    ],
    c_args: c_args,
    c_pch: test_c_pch,
    include_directories: test_inc,
    dependencies: [lib_deps, check_dep],
    install: false,
)
test('colon-path', path_test)

cmd_test = executable(
    'colon-cmd',
    [
        'colon-cmd_test.c',
        '../src/colon-path.c',
        '../src/colon-cmd.c',
    ],
    c_args: c_args,
    c_pch: test_c_pch,
    include_directories: test_inc,
    dependencies: [lib_deps, check_dep],
    install: false,
)
test('colon-cmd', cmd_test)

test(
    'completion',
    find_program('bash'),
    args: [meson.current_source_dir() / 'completion_smoke.sh'],
)
