from _renderlib import lib

class Animation:
    def __init__(self, animptr):
        self._ptr = animptr


class AnimationInstance:
    def __init__(self, animation):
        self._ptr = lib.animation_instance_new(animation._ptr)
        if not self._ptr:
            raise RuntimeError('failed to create animation instance')

    def __del__(self):
        lib.animation_instance_free(self._ptr)

    def play(self, dt):
        if not lib.animation_instance_play(self._ptr, dt):
            raise RuntimeError('animation instance playback failed')