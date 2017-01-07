"""Texture wrappers"""
from _renderlib import lib
from enum import IntEnum

class Texture:

    class TextureType(IntEnum):
        texture_2d = lib.GL_TEXTURE_2D
        texture_rectangle = lib.GL_TEXTURE_RECTANGLE

    def __init__(self, texptr):
        self._texture = texptr

    def __del__(self):
        lib.texture_free(self._texture)

    @classmethod
    def from_image(cls, img, tex_type):
        texptr = lib.texture_from_image(img._image, tex_type)
        if not texptr:
            raise RuntimeError('failed to create texture from image')
        return Texture(texptr)