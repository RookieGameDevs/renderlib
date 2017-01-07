from _renderlib import lib

class Text:
    def __init__(self, font):
        self._ptr = lib.text_new(font._ptr)
        if not self._ptr:
            raise RuntimeError('failed to create text')
        self._string = ''

    def __del__(self):
        lib.text_free(self._ptr)

    @property
    def string(self):
        return self._string

    @string.setter
    def string(self, s):
        self._string = s
        if not lib.text_set_string(self._ptr, s.encode('utf8')):
            raise RuntimeError('failed to set text string')