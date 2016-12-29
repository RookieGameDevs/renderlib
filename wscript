# -*- coding: utf-8 -*-
import sys


def options(opt):
    opt.load('compiler_c')

    opt.add_option(
        '--build-type',
        action='store',
        choices=['release', 'debug'],
        default='debug',
        help='build type (release or debug)')

    opt.add_option(
        '--with-tests',
        action='store_true',
        help='build tests')


def configure(cfg):
    cfg.load('compiler_c')

    cfg.env.append_unique('CFLAGS', '-std=c99')
    cfg.env.append_unique('CFLAGS', '-Wall')
    cfg.env.append_unique('CFLAGS', '-Werror')
    cfg.env.append_unique(
        'INCLUDES',
        cfg.path.find_dir('deps/matlib/build/include').abspath())
    cfg.env.append_unique(
        'LIBPATH',
        cfg.path.find_dir('deps/matlib/build/lib').abspath())

    if cfg.options.build_type == 'debug':
        cfg.env.append_unique('CFLAGS', '-g')
        cfg.env.append_unique('DEFINES', 'DEBUG')
    else:
        cfg.env.append_unique('CFLAGS', '-O3')
        cfg.env.append_unique('DEFINES', 'NDEBUG')

    # find SDL2
    cfg.check_cfg(
        path='sdl2-config',
        args='--libs --cflags',
        package='',
        uselib_store='sdl')

    # find GLEW
    cfg.check_cfg(
        package='glew',
        args='--libs --cflags',
        uselib_store='glew')

    cfg.env.with_tests = cfg.options.with_tests
    if cfg.options.with_tests:
        cfg.check_cfg(
            package='check',
            args='--libs --cflags',
            uselib_store='check')

    if sys.platform.startswith('linux'):
        # find libm (standard C math library)
        cfg.check_cc(
            msg=u'Checking for libm',
            lib='m',
            cflags='-Wall',
            uselib_store='libm')

        # find CBLAS library (of any implementation)
        cfg.check_cc(
            msg=u'Checking for BLAS library',
            lib='blas',
            header_name='cblas.h',
            uselib_store='cblas')


def build(bld):
    deps = ['sdl', 'glew']
    libs = ['mat']
    kwargs = {}

    if sys.platform.startswith('linux'):
        deps.extend([
            'libm',
            'cblas',
        ])
    elif sys.platform.startswith('darwin'):
        kwargs['framework'] = ['OpenGL', 'Accelerate']

    bld.shlib(
        target='render',
        source=bld.path.ant_glob('src/**/*.c'),
        uselib=deps,
        lib=libs,
        **kwargs)

    if bld.env.with_tests:
        for src in bld.path.ant_glob('tests/test_*.c'):
            name = src.name.split('.')[0]
            bld.program(
                target=name,
                source=[src],
                includes=['src'],
                uselib=deps + ['check'],
                use=['render'],
                lib=libs,
                **kwargs)

