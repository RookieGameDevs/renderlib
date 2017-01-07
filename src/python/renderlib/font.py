"""Font wrappers"""
from _renderlib import lib

class Font:
    def __init__(self, ptr):
        self._ptr = ptr

    def __del__(self):
        lib.font_free(self._ptr)

    @classmethod
    def from_file(cls, filename, ptsize):
        ptr = lib.font_from_file(filename.encode('utf8'), ptsize)
        if not ptr:
            raise RuntimeError('failed to load font from file')
        return Font(ptr)

    @classmethod
    def from_buffer(cls, buf, ptsize):
        ptr = lib.font_from_buffer(buf, len(buf), ptsize)
        if not ptr:
            raise RuntimeError('failed to load font from buffer')
        return Font(ptr)