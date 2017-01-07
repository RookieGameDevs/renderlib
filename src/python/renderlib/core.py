"""Core API."""
from _renderlib import ffi
from _renderlib import lib
from matlib import Mat
from matlib import Vec


class Light:
    def __init__(self):
        self._light = ffi.new('struct Light*')
        self._transform = Mat(matptr=ffi.addressof(self._light, 'transform'))
        self._direction = Vec(vecptr=ffi.addressof(self._light, 'direction'))
        self._color = Vec(vecptr=ffi.addressof(self._light, 'color'))

    @property
    def transform(self):
        return self._transform

    @transform.setter
    def transform(self, t):
        ffi.memmove(self._transform._mat, t._mat, ffi.sizeof('Mat'))

    @property
    def direction(self):
        return self._direction

    @direction.setter
    def direction(self, d):
        ffi.memmove(self._direction._vec, d._vec, ffi.sizeof('Vec'))

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, c):
        ffi.memmove(self._color._vec, c._vec, ffi.sizeof('Vec'))

    @property
    def ambient_intensity(self):
        return self._light.ambient_intensity

    @ambient_intensity.setter
    def ambient_intensity(self, ai):
        self._light.ambient_intensity = ai

    @property
    def diffuse_intensity(self):
        return self._light.diffuse_intensity

    @diffuse_intensity.setter
    def diffuse_intensity(self, di):
        self._light.diffuse_intensity = di


class Material:
    def __init__(self):
        self._material = ffi.new('struct Material*')


class MeshRenderProps:
    def __init__(self):
        self._props = ffi.new('struct MeshRenderProps*')


def renderer_init():
    """Initializes renderlib library."""
    if not lib.renderer_init():
        raise RuntimeError('renderer initialization failed')


def renderer_present():
    """Presents rendering results (a frame)."""
    if not lib.renderer_present():
        raise RuntimeError('rendering failed')


def renderer_shutdown():
    """Shuts down the library."""
    lib.renderer_shutdown()


def render_mesh(mesh, props):
    """Renders a mesh applying given rendering properties."""
    if not lib.render_mesh(mesh._mesh, props._props):
        raise RuntimeError('mesh rendering failed')