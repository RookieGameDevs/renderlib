"""Texture wrappers"""
from _renderlib import lib
from enum import IntEnum

class Texture:

    class TextureType(IntEnum):
        texture_2d = lib.GL_TEXTURE_2D
        texture_rectangle = lib.GL_TEXTURE_RECTANGLE

    def __init__(self, ptr):
        self._ptr = ptr

    def __del__(self):
        lib.texture_free(self._ptr)

    @classmethod
    def from_image(cls, img, tex_type):
        ptr = lib.texture_from_image(img._ptr, tex_type)
        if not ptr:
            raise RuntimeError('failed to create texture from image')
        return Texture(ptr)