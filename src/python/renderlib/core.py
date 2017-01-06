"""Core API."""
from _renderlib import ffi
from _renderlib import lib


class Light:
    def __init__(self):
        self._light = ffi.new('struct Light*')


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