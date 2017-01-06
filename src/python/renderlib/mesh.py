"""Mesh wrappers"""
from _renderlib import lib

class Mesh:
    def __init__(self, meshptr):
        self._mesh = meshptr

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