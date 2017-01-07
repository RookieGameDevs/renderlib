"""Image wrappers"""
from enum import IntEnum
from _renderlib import lib

class Image:

    class Format(IntEnum):
        RGBA = lib.IMAGE_FORMAT_RGBA
        RGB = lib.IMAGE_FORMAT_RGB

    class Codec(IntEnum):
        PNG = lib.IMAGE_CODEC_PNG
        JPEG = lib.IMAGE_CODEC_JPEG

    def __init__(self, ptr):
        self._ptr = ptr

    def __del__(self):
        lib.image_free(self._ptr)

    @property
    def width(self):
        return self._ptr.width

    @property
    def height(self):
        return self._ptr.height

    @classmethod
    def from_file(cls, filename):
        ptr = lib.image_from_file(filename.encode('utf8'))
        if not ptr:
            raise RuntimeError('failed to load image from file')
        return Image(ptr)

    @classmethod
    def from_buffer(cls, buf, codec):
        ptr = lib.image_from_buffer(buf, len(buf), codec)
        if not ptr:
            raise RuntimeError('failed to load image from buffer')
        return Image(ptr)