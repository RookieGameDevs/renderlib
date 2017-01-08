"""Mesh wrappers"""
from _renderlib import ffi
from _renderlib import lib
from renderlib.animation import Animation
from matlib.mat import Mat

class Mesh:
    def __init__(self, vertices=None, indices=None, normals=None, uvs=None,
            joint_ids=None, joint_weights=None, ptr=None):
        self._ptr = None
        if not ptr:
            # check constraints
            if not vertices or not indices:
                raise RuntimeError('vertex and index data is required')
            if len(vertices) < 3 or len(indices) % 3:
                raise RuntimeError('invalid vertex and/or index data')
            if normals and len(normals) != len(vertices):
                raise RuntimeError('invalid index data')
            if uvs and len(uvs) != len(vertices):
                raise RuntimeError('invalid UV data')
            if any([joint_ids, joint_weights]):
                if not all([joint_ids, joint_weights]) or \
                        len(joint_ids) != len(joint_weights):
                    raise RuntimeError('inconsistent joint data')

            vdata = ffi.new('float[][3]', vertices)
            idata = ffi.new('uint32_t[]', indices)
            udata = ffi.new('float[][2]', uvs) if uvs else ffi.NULL
            ndata = ffi.new('float[][3]', normals) if normals else ffi.NULL
            jidata = ffi.new('uint8_t[][4]', joint_ids) if joint_ids else ffi.NULL
            jwdata = ffi.new('uint8_t[][4]', joint_weights) if joint_weights else ffi.NULL

            ptr = lib.mesh_new(
                vdata,
                ndata,
                udata,
                jidata,
                jwdata,
                len(vertices),
                idata,
                len(indices))
            if not ptr:
                raise RuntimeError('failed to create mesh')

        self._ptr = ptr
        self._animations = [
            Animation(animptr=ffi.addressof(self._ptr.animations, i))
            for i in range(self._ptr.anim_count)
        ]
        self._transform = Mat(ptr=ffi.addressof(self._ptr, 'transform'))

    def __del__(self):
        lib.mesh_free(self._ptr or ffi.NULL)

    @classmethod
    def from_file(cls, filename):
        ptr = lib.mesh_from_file(filename.encode('utf8'))
        if not ptr:
            raise RuntimeError('failed to load mesh from file')
        return Mesh(ptr=ptr)

    @classmethod
    def from_buffer(cls, buf):
        ptr = lib.mesh_from_buffer(buf, len(buf))
        if not ptr:
            raise RuntimeError('failed to load mesh from buffer')
        return Mesh(ptr=ptr)

    @property
    def animations(self):
        return self._animations

    @property
    def transform(self):
        return self._transform

    @transform.setter
    def transform(self, t):
        ffi.memmove(self._transform._ptr, t._ptr, ffi.sizeof('Mat'))

    @property
    def vertex_count(self):
        return self._ptr.vertex_count

    @property
    def index_count(self):
        return self._ptr.index_count