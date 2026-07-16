project(
    'colon-subdirs',
    'c',
    version: run_command(
        'sh',
        '-c', '''
        if [ -d "$MESON_SOURCE_ROOT/.git" ] || [ -f "$MESON_SOURCE_ROOT/.git" ]; then
            v=$(git -C "$MESON_SOURCE_ROOT" describe --tags --always --dirty 2>/dev/null || true)
            v=${v#v}
            if [ -n "$v" ]; then
                if [ "${v%.*}" = "$v" ]; then
                    v="0.1.0-$v"
                fi
                printf "%s" "$v"
                exit 0
            fi
        fi
        echo "0.1.0"
        ''',
        check: false,
    ).stdout().strip(),
    license: 'AGPL-3.0-or-later',
    default_options: [
        'c_std=c99',
        'warning_level=2',
    ],
    meson_version: '>=0.61.0',
)

project_author = 'Lenik'
project_email = 'lenik@bodz.net'
project_year = 2026

fs = import('fs')
c = meson.get_compiler('c')

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
localedir = prefix / get_option('localedir')
mandir = prefix / get_option('mandir')
pkgdatadir = datadir / meson.project_name()
pkgdocdir = datadir / 'doc' / meson.project_name()

config_h = configuration_data()
config_h.set_quoted('PROJECT_VERSION', meson.project_version())
config_h.set_quoted('PROJECT_AUTHOR', project_author)
config_h.set_quoted('PROJECT_EMAIL', project_email)
config_h.set('PROJECT_YEAR', project_year)
config_h.set_quoted('PREFIX', prefix)
config_h.set_quoted('DATADIR', datadir)
config_h.set_quoted('MANDIR', mandir)
config_h.set_quoted('PKGDATADIR', pkgdatadir)
config_h.set_quoted('LOCALEDIR', localedir)

configure_file(
    output: 'config.h',
    configuration: config_h,
)

bas_c_dep = dependency('bas-c', required: true)

bas_args = ['-DLOGGER_NAME=colon_subdirs']
c_pch = 'src/c_pch.h'
c_args = [bas_args, '-DTEXT_DOMAIN=colon-subdirs', '-D_DEFAULT_SOURCE']

lib_inc = include_directories('src')
lib_deps = [bas_c_dep]

subdir('po')

# Common sources compiled into each command (no shared library).
common_sources = [
    'src/colon-path.c',
    'src/colon-cmd.c',
]

commands = [
    'mv',
    'cp',
    'zip',
    'rm',
    'rmdir',
    'scp',
    'unzip',
    'rsync',
]

foreach cmd : commands
    exe = executable(
        'colon-' + cmd,
        ['src/colon-' + cmd + '.c'] + common_sources,
        c_args: c_args,
        c_pch: c_pch,
        include_directories: lib_inc,
        dependencies: lib_deps,
        install: true,
        install_dir: bindir,
    )

    # :mv → colon-mv, etc.
    install_symlink(
        ':' + cmd,
        pointing_to: 'colon-' + cmd,
        install_dir: bindir,
    )

    man_in = 'man/colon-' + cmd + '.1.in'
    if fs.exists(man_in)
        config_man = configuration_data()
        config_man.set('PROJECT_VERSION', meson.project_version())
        config_man.set('PROJECT_AUTHOR', project_author)
        config_man.set('PROJECT_EMAIL', project_email)
        config_man.set('PROJECT_YEAR', project_year)
        config_man.set('CMD', cmd)
        config_man.set('COLON', ':' + cmd)
        config_man.set('PROG', 'colon-' + cmd)
        configure_file(
            input: man_in,
            output: 'colon-' + cmd + '.1',
            configuration: config_man,
            install: true,
            install_dir: mandir / 'man1',
        )
    endif
endforeach

# Shared colon-path overview man page
config_man7 = configuration_data()
config_man7.set('PROJECT_VERSION', meson.project_version())
config_man7.set('PROJECT_AUTHOR', project_author)
config_man7.set('PROJECT_EMAIL', project_email)
config_man7.set('PROJECT_YEAR', project_year)
configure_file(
    input: 'man/colon-path.7.in',
    output: 'colon-path.7',
    configuration: config_man7,
    install: true,
    install_dir: mandir / 'man7',
)

install_data(
    [
        'LICENSE',
        'README.md',
        'README-zh.md',
    ],
    install_dir: pkgdocdir,
)

# Bash completion: one script registered under every colon-* / :* name.
install_data(
    'colon-subdirs.bash',
    install_dir: datadir / 'bash-completion' / 'completions',
    rename: 'colon-mv',
)
foreach cmd : commands
    if cmd != 'mv'
        install_symlink(
            'colon-' + cmd,
            pointing_to: 'colon-mv',
            install_dir: datadir / 'bash-completion' / 'completions',
        )
    endif
    install_symlink(
        ':' + cmd,
        pointing_to: 'colon-mv',
        install_dir: datadir / 'bash-completion' / 'completions',
    )
endforeach

run_target(
    'look',
    command: [
        'bash',
        '-euc', '''
            tmpdir=$(mktemp -d)
            DESTDIR=$tmpdir meson install -C "@BUILD_ROOT@"
            tree "$tmpdir"
            rm -fr "$tmpdir"
        ''',
    ],
)

install_symlinks_script = '''
bindir="@0@"
mandir="@1@"
datadir="@2@"
mkdir -p "$bindir" "$mandir/man1" "$mandir/man7" \
    "$datadir/bash-completion/completions"
'''.format(bindir, mandir, datadir)

foreach cmd : commands
    install_symlinks_script += 'sudo ln -sfn "@BUILD_ROOT@/colon-' + cmd + '" "$bindir/colon-' + cmd + '"\n'
    install_symlinks_script += 'sudo ln -sfn "@BUILD_ROOT@/colon-' + cmd + '" "$bindir/:' + cmd + '"\n'
    install_symlinks_script += 'sudo ln -sfn "@BUILD_ROOT@/colon-' + cmd + '.1" "$mandir/man1/colon-' + cmd + '.1"\n'
    install_symlinks_script += 'sudo ln -sfn "@SOURCE_ROOT@/colon-subdirs.bash" "$datadir/bash-completion/completions/colon-' + cmd + '"\n'
    install_symlinks_script += 'sudo ln -sfn "@SOURCE_ROOT@/colon-subdirs.bash" "$datadir/bash-completion/completions/:' + cmd + '"\n'
endforeach
install_symlinks_script += 'sudo ln -sfn "@BUILD_ROOT@/colon-path.7" "$mandir/man7/colon-path.7"\n'
install_symlinks_script += 'printf "Symlinks installed\\n"\n'

run_target(
    'install-symlinks',
    command: ['bash', '-euc', install_symlinks_script],
)

run_target(
    'uninstall-symlinks',
    command: [
        'bash',
        '-euc', '''
            bindir="@0@"
            mandir="@1@"
            datadir="@2@"
            for cmd in mv cp zip rm rmdir scp unzip rsync; do
                for p in \
                    "$bindir/colon-$cmd" \
                    "$bindir/:$cmd" \
                    "$mandir/man1/colon-$cmd.1" \
                    "$datadir/bash-completion/completions/colon-$cmd" \
                    "$datadir/bash-completion/completions/:$cmd"
                do
                    if [ -L "$p" ]; then
                        sudo rm -f "$p"
                        printf "Removed %s\n" "$p"
                    fi
                done
            done
            p="$mandir/man7/colon-path.7"
            if [ -L "$p" ]; then
                sudo rm -f "$p"
            fi
        '''.format(bindir, mandir, datadir),
    ],
)

subdir('tests')
