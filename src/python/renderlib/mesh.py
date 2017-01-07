"""Mesh wrappers"""
from _renderlib import ffi
from _renderlib import lib
from renderlib.animation import Animation

class Mesh:
    def __init__(self, meshptr):
        self._mesh = meshptr
        self._animations = [
            Animation(animptr=ffi.addressof(self._mesh.animations, i))
            for i in range(self._mesh.anim_count)
        ]

    def __del__(self):
        lib.mesh_free(self._mesh)

    @classmethod
    def from_file(cls, filename):
        meshptr = lib.mesh_from_file(filename.encode('utf8'))
        if not meshptr:
            raise RuntimeError('failed to load mesh from file')
        return Mesh(meshptr)

    @classmethod
    def from_buffer(cls, buf):
        meshptr = lib.mesh_from_buffer(buf, len(buf))
        if not meshptr:
            raise RuntimeError('failed to load mesh from buffer')
        return Mesh(meshptr)

    @property
    def animations(self):
        return self._animations