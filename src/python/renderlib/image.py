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

    def __init__(self, imageptr):
        self._image = imageptr

    def __del__(self):
        lib.image_free(self._image)

    @property
    def width(self):
        return self._image.width

    @property
    def height(self):
        return self._image.height

    @classmethod
    def from_file(cls, filename):
        imageptr = lib.image_from_file(filename.encode('utf8'))
        if not imageptr:
            raise RuntimeError('failed to load image from file')
        return Image(imageptr)

    @classmethod
    def from_buffer(cls, buf, codec):
        imageptr = lib.image_from_buffer(buf, len(buf), codec)
        if not imageptr:
            raise RuntimeError('failed to load image from buffer')
        return Image(imageptr)