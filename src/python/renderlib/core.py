"""Core API."""
from _renderlib import ffi
from _renderlib import lib
from matlib.mat import Mat
from matlib.vec import Vec


class Light:
    def __init__(self):
        self._ptr = ffi.new('struct Light*')
        self._transform = Mat(ptr=ffi.addressof(self._ptr, 'transform'))
        self._direction = Vec(ptr=ffi.addressof(self._ptr, 'direction'))
        self._color = Vec(ptr=ffi.addressof(self._ptr, 'color'))

        # initialize defaults
        self.transform.ident()
        self.direction = Vec(0, -1, 0)
        self.color = Vec(1, 1, 1)
        self.ambient_intensity = 0.3
        self.diffuse_intensity = 0.8

    @property
    def transform(self):
        return self._transform

    @transform.setter
    def transform(self, t):
        ffi.memmove(self._transform._ptr, t._ptr, ffi.sizeof('Mat'))

    @property
    def direction(self):
        return self._direction

    @direction.setter
    def direction(self, d):
        ffi.memmove(self._direction._ptr, d._ptr, ffi.sizeof('Vec'))

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, c):
        ffi.memmove(self._color._ptr, c._ptr, ffi.sizeof('Vec'))

    @property
    def ambient_intensity(self):
        return self._ptr.ambient_intensity

    @ambient_intensity.setter
    def ambient_intensity(self, ai):
        self._ptr.ambient_intensity = ai

    @property
    def diffuse_intensity(self):
        return self._ptr.diffuse_intensity

    @diffuse_intensity.setter
    def diffuse_intensity(self, di):
        self._ptr.diffuse_intensity = di


class Material:
    def __init__(self):
        self._ptr = ffi.new('struct Material*')
        self._texture = None
        self._color = Vec(ptr=ffi.addressof(self._ptr, 'color'))

        # defaults
        self.receive_light = False
        self.specular_power = 4
        self.specular_intensity = 0.8

    @property
    def texture(self):
        return self._texture

    @texture.setter
    def texture(self, t):
        self._texture = t
        self._ptr.texture = t._ptr

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, c):
        ffi.memmove(self._color._ptr, c._ptr, ffi.sizeof('Vec'))

    @property
    def receive_light(self):
        return bool(self._ptr.receive_light)

    @receive_light.setter
    def receive_light(self, flag):
        self._ptr.receive_light = int(bool(flag))

    @property
    def specular_intensity(self):
        return self._ptr.specular_intensity

    @specular_intensity.setter
    def specular_intensity(self, si):
        self._ptr.specular_intensity = si

    @property
    def specular_power(self):
        return self._ptr.specular_power

    @specular_power.setter
    def specular_power(self, sp):
        self._ptr.specular_power = sp


class MeshRenderProps:
    def __init__(self):
        self._ptr = ffi.new('struct MeshRenderProps*')
        self._eye = Vec(ptr=ffi.addressof(self._ptr, 'eye'))
        self._model = Mat(ptr=ffi.addressof(self._ptr, 'model'))
        self._view = Mat(ptr=ffi.addressof(self._ptr, 'view'))
        self._projection = Mat(ptr=ffi.addressof(self._ptr, 'projection'))
        self._light = None
        self._animation = None
        self._material = None

        # defaults
        self.eye = Vec(0, 0, 1)
        self.model.ident()
        self.view.ident()
        self.projection.ident()
        self.cast_shadows = False
        self.receive_shadows = True

    @property
    def eye(self):
        return self._eye

    @eye.setter
    def eye(self, pos):
        ffi.memmove(self._eye._ptr, pos._ptr, ffi.sizeof('Vec'))

    @property
    def model(self):
        return self._model

    @model.setter
    def model(self, mat):
        ffi.memmove(self._model._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def view(self):
        return self._view

    @view.setter
    def view(self, mat):
        ffi.memmove(self._view._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def projection(self):
        return self._projection

    @projection.setter
    def projection(self, mat):
        ffi.memmove(self._projection._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def cast_shadows(self):
        return bool(self._ptr.cast_shadows)

    @cast_shadows.setter
    def cast_shadows(self, flag):
        self._ptr.cast_shadows = int(bool(flag))

    @property
    def receive_shadows(self):
        return bool(self._ptr.receive_shadows)

    @receive_shadows.setter
    def receive_shadows(self, flag):
        self._ptr.receive_shadows = int(bool(flag))

    @property
    def light(self):
        return self._light

    @light.setter
    def light(self, l):
        self._light = l
        self._ptr.light = l._ptr

    @property
    def animation(self):
        return self._animation

    @animation.setter
    def animation(self, a):
        self._animation = a
        self._ptr.animation = a._ptr

    @property
    def material(self):
        return self._material

    @material.setter
    def material(self, m):
        self._material = m
        self._ptr.material = m._ptr


class TextRenderProps:
    def __init__(self):
        self._ptr = ffi.new('struct TextRenderProps*')
        self._model = Mat(ptr=ffi.addressof(self._ptr, 'model'))
        self._view = Mat(ptr=ffi.addressof(self._ptr, 'view'))
        self._projection = Mat(ptr=ffi.addressof(self._ptr, 'projection'))
        self._color = Vec(ptr=ffi.addressof(self._ptr, 'color'))

        self.model.ident()
        self.view.ident()
        self.projection.ident()
        self.color = Vec(1, 1, 1, 1)
        self.opacity = 1.0

    @property
    def model(self):
        return self._model

    @model.setter
    def model(self, mat):
        ffi.memmove(self._model._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def view(self):
        return self._view

    @view.setter
    def view(self, mat):
        ffi.memmove(self._view._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def projection(self):
        return self._projection

    @projection.setter
    def projection(self, mat):
        ffi.memmove(self._projection._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, c):
        ffi.memmove(self._color._ptr, c._ptr, ffi.sizeof('Vec'))

    @property
    def opacity(self):
        return self._ptr.opacity

    @opacity.setter
    def opacity(self, value):
        self._ptr.opacity = value


class QuadRenderProps:
    class Borders:
        def __init__(self, container):
            self._borders = container._ptr.borders
            self.left = self.right = self.top = self.bottom = 0

        @property
        def left(self):
            return self._borders.left

        @left.setter
        def left(self, value):
            self._borders.left = value

        @property
        def top(self):
            return self._borders.top

        @top.setter
        def top(self, value):
            self._borders.top = value

        @property
        def right(self):
            return self._borders.right

        @right.setter
        def right(self, value):
            self._borders.right = value

        @property
        def right(self):
            return self._borders.right

        @right.setter
        def right(self, value):
            self._borders.right = value

    def __init__(self):
        self._ptr = ffi.new('struct QuadRenderProps*')
        self._model = Mat(ptr=ffi.addressof(self._ptr, 'model'))
        self._view = Mat(ptr=ffi.addressof(self._ptr, 'view'))
        self._projection = Mat(ptr=ffi.addressof(self._ptr, 'projection'))
        self._color = Vec(ptr=ffi.addressof(self._ptr, 'color'))
        self._texture = None

        self.borders = QuadRenderProps.Borders(self)

        self.model.ident()
        self.view.ident()
        self.projection.ident()
        self.color = Vec(1, 1, 1, 1)
        self.opacity = 1.0

    @property
    def model(self):
        return self._model

    @model.setter
    def model(self, mat):
        ffi.memmove(self._model._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def view(self):
        return self._view

    @view.setter
    def view(self, mat):
        ffi.memmove(self._view._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def projection(self):
        return self._projection

    @projection.setter
    def projection(self, mat):
        ffi.memmove(self._projection._ptr, mat._ptr, ffi.sizeof('Mat'))

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, c):
        ffi.memmove(self._color._ptr, c._ptr, ffi.sizeof('Vec'))

    @property
    def opacity(self):
        return self._ptr.opacity

    @opacity.setter
    def opacity(self, value):
        self._ptr.opacity = value

    @property
    def texture(self):
        return self._texture

    @texture.setter
    def texture(self, t):
        self._texture = t
        self._ptr.texture = t._ptr


def renderer_init():
    """Initializes renderlib library."""
    if not lib.renderer_init():
        raise RuntimeError('renderer initialization failed')


def renderer_clear():
    """Clear render buffers."""
    lib.renderer_clear()


def renderer_present():
    """Presents rendering results (a frame)."""
    if not lib.renderer_present():
        raise RuntimeError('rendering failed')


def renderer_shutdown():
    """Shuts down the library."""
    lib.renderer_shutdown()


def render_mesh(mesh, props):
    """Renders a mesh applying given rendering properties."""
    if not lib.render_mesh(mesh._ptr, props._ptr):
        raise RuntimeError('mesh rendering failed')


def render_text(text, props):
    """Renders a text applying given rendering properties."""
    if not lib.render_text(text._ptr, props._ptr):
        raise RuntimeError('text rendering failed')


def render_quad(w, h, props):
    """Renders a rectangle of given dimensions with given rendering properties."""
    if not lib.render_quad(w, h, props._ptr):
        raise RuntimeError('quad rendering failed')