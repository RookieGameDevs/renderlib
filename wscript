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

    opt.add_option(
        '--with-demo',
        action='store_true',
        help='build demo app')


def configure(cfg):
    cfg.load('compiler_c')

    cfg.env.with_tests = cfg.options.with_tests
    cfg.env.with_demo = cfg.options.with_demo

    cfg.env.append_unique('CFLAGS', '-std=c99')
    cfg.env.append_unique('CFLAGS', '-Wall')
    cfg.env.append_unique('CFLAGS', '-Werror')

    if cfg.options.build_type == 'debug':
        cfg.env.append_unique('CFLAGS', '-g')
        cfg.env.append_unique('DEFINES', 'DEBUG')
    else:
        cfg.env.append_unique('CFLAGS', '-O3')
        cfg.env.append_unique('DEFINES', 'NDEBUG')

    # find GLEW
    cfg.check_cfg(
        package='glew',
        args='--libs --cflags',
        uselib_store='glew')

    # find libPNG
    cfg.check_cfg(
        package='libpng',
        args='--libs --cflags',
        uselib_store='libpng')

    # find freetype2
    cfg.check_cfg(
        package='freetype2',
        args='--libs --cflags',
        uselib_store='freetype')

    # find matlib
    cfg.check_cc(
        msg=u'Checking for matlib',
        lib='mat',
        header_file='matlib.h',
        includes=[cfg.path.find_dir('deps/matlib/build/include').abspath()],
        libpath=[cfg.path.find_dir('deps/matlib/build/lib').abspath()],
        uselib_store='matlib')

    if cfg.options.with_tests or cfg.options.with_demo:
        # find SDL2
        cfg.check_cfg(
            path='sdl2-config',
            args='--libs --cflags',
            package='',
            uselib_store='sdl')

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
    deps = ['glew', 'libpng', 'freetype', 'matlib']
    kwargs = {}

    if sys.platform.startswith('linux'):
        deps.extend([
            'libm',
            'cblas',
        ])
    elif sys.platform.startswith('darwin'):
        kwargs['framework'] = ['OpenGL', 'Accelerate']

    bld.shlib(
        target='lib/render',
        source=bld.path.ant_glob('src/**/*.c'),
        uselib=deps,
        **kwargs)

    rpath = bld.bldnode.find_or_declare('lib').abspath()
    print(rpath)

    if bld.env.with_tests:
        bld.program(
            target='bin/tests',
            source=bld.path.ant_glob('tests/**/*.c'),
            includes=['src'],
            uselib=deps + ['check', 'sdl'],
            rpath=[rpath],
            use=['lib/render'],
            **kwargs)

    if bld.env.with_demo:
        bld.program(
            target='bin/demo',
            source=bld.path.ant_glob('demo/**/*.c'),
            includes=['src'],
            uselib=deps + ['sdl'],
            rpath=[rpath],
            use=['lib/render'],
            **kwargs)
