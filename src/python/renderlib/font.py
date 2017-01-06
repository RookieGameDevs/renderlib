"""Font wrappers"""
from _renderlib import lib

class Font:
    def __init__(self, fontptr):
        self._font = fontptr

    def __del__(self):
        lib.font_free(self._font)

    @classmethod
    def from_file(cls, filename, ptsize):
        fontptr = lib.font_from_file(filename.encode('utf8'), ptsize)
        if not fontptr:
            raise RuntimeError('failed to load font from file')
        return Font(fontptr)

    @classmethod
    def from_buffer(cls, buf, ptsize):
        fontptr = lib.font_from_buffer(buf, len(buf), ptsize)
        if not fontptr:
            raise RuntimeError('failed to load font from buffer')
        return Font(fontptr)