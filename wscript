# -*- coding: utf-8 -*-
import sys
import os


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

    opt.add_option(
        '--jpeg-path',
        default='/usr',
        action='store',
        help='path to libjpeg installation root')

    opt.add_option(
        '--matlib-path',
        default='/usr',
        action='store',
        help='path to matlib installation root')

    opt.add_option(
        '--datalib-path',
        default='/usr',
        action='store',
        help='path to datalib installation root')


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

    # find libpng
    cfg.check_cfg(
        package='libpng',
        args='--libs --cflags',
        uselib_store='libpng')

    # find libjpeg
    cfg.check_cc(
        msg=u'Checking for \'libjpeg\'',
        lib='jpeg',
        header_file='jpeglib.h',
        includes=os.path.join(cfg.options.jpeg_path, 'include'),
        libpath=os.path.join(cfg.options.jpeg_path, 'lib'),
        uselib_store='libjpeg')

    # find freetype2
    cfg.check_cfg(
        package='freetype2',
        args='--libs --cflags',
        uselib_store='freetype')

    # find matlib
    cfg.check_cc(
        msg=u'Checking for \'matlib\'',
        lib='mat',
        header_file='matlib.h',
        includes=os.path.join(cfg.options.matlib_path, 'include'),
        libpath=os.path.join(cfg.options.matlib_path, 'lib'),
        uselib_store='matlib')

    # find datalib
    cfg.check_cc(
        msg=u'Checking for \'datalib\'',
        lib='data',
        header_file='datalib.h',
        includes=os.path.join(cfg.options.datalib_path, 'include'),
        libpath=os.path.join(cfg.options.datalib_path, 'lib'),
        uselib_store='datalib')

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


def stringify_shader(task):
    with open(task.inputs[0].abspath()) as in_fp:
        with open(task.outputs[0].abspath(), 'w') as out_fp:
            out_fp.writelines(
                '"{}\\n"\n'.format(line.replace('\n', '')) for line in in_fp.readlines())


def build(bld):
    deps = ['glew', 'libpng', 'libjpeg', 'freetype', 'matlib', 'datalib']
    kwargs = {}

    if sys.platform.startswith('linux'):
        deps.extend([
            'libm',
        ])
    elif sys.platform.startswith('darwin'):
        kwargs['framework'] = ['OpenGL', 'Accelerate']

    # stringify shaders in order to embed them into library at compile time
    shaders = []
    for shader_file in bld.path.ant_glob('src/shaders/*'):
        target = 'shaders/{}.h'.format(os.path.basename(shader_file.name))
        bld(rule=stringify_shader, source=shader_file, target=target)
        shaders.append(target)

    bld.shlib(
        target='render',
        source=bld.path.ant_glob('src/**/*.c'),
        uselib=deps,
        install_path='${PREFIX}/lib',
        includes=bld.bldnode.find_or_declare('shaders').abspath(),
        use=shaders,
        **kwargs)

    bld.install_files(
        '${PREFIX}/include/renderlib',
        [
            'src/anim.h',
            'src/camera.h',
            'src/error.h',
            'src/font.h',
            'src/image.h',
            'src/light.h',
            'src/mesh.h',
            'src/renderlib.h',
            'src/scene.h',
            'src/shader.h',
            'src/string_utils.h',
            'src/text.h',
            'src/texture.h',
        ])

    rpath = [
        bld.bldnode.abspath(),
    ]

    if bld.env.with_tests:
        bld.program(
            target='test-suite',
            source=bld.path.ant_glob('tests/**/*.c'),
            includes=['src'],
            uselib=deps + ['check', 'sdl'],
            rpath=rpath,
            use=['render'],
            install_path=None,
            **kwargs)

    if bld.env.with_demo:
        bld.program(
            target='renderlib-demo',
            source=bld.path.ant_glob('demo/**/*.c'),
            includes=['src'],
            uselib=deps + ['sdl'],
            rpath=rpath,
            use=['render'],
            install_path=None,
            **kwargs)
