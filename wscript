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


def configure(cfg):
    cfg.load('compiler_c')

    cfg.env.append_unique('CFLAGS', '-std=c99')
    cfg.env.append_unique('CFLAGS', '-Wall')
    cfg.env.append_unique('CFLAGS', '-Werror')

    if cfg.options.build_type == 'debug':
        cfg.env.append_unique('CFLAGS', '-g')
        cfg.env.append_unique('DEFINES', 'DEBUG')
    else:
        cfg.env.append_unique('CFLAGS', '-O3')
        cfg.env.append_unique('DEFINES', 'NDEBUG')


def build(bld):
    pass
