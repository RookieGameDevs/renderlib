"""Mesh wrappers"""
from _renderlib import ffi
from _renderlib import lib
from renderlib.animation import Animation

class Mesh:
    def __init__(self, ptr):
        self._ptr = ptr
        self._animations = [
            Animation(animptr=ffi.addressof(self._ptr.animations, i))
            for i in range(self._ptr.anim_count)
        ]

    def __del__(self):
        lib.mesh_free(self._ptr)

    @classmethod
    def from_file(cls, filename):
        ptr = lib.mesh_from_file(filename.encode('utf8'))
        if not ptr:
            raise RuntimeError('failed to load mesh from file')
        return Mesh(ptr)

    @classmethod
    def from_buffer(cls, buf):
        ptr = lib.mesh_from_buffer(buf, len(buf))
        if not ptr:
            raise RuntimeError('failed to load mesh from buffer')
        return Mesh(ptr)

    @property
    def animations(self):
        return self._animations