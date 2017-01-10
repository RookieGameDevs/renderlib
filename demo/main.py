from matlib.mat import Mat
from matlib.vec import Vec
from renderlib.animation import AnimationInstance
from renderlib.core import Light
from renderlib.core import Material
from renderlib.core import MeshRenderProps
from renderlib.core import TextRenderProps
from renderlib.core import render_mesh
from renderlib.core import render_text
from renderlib.core import renderer_clear
from renderlib.core import renderer_init
from renderlib.core import renderer_present
from renderlib.core import renderer_shutdown
from renderlib.error import error_print_traceback
from renderlib.font import Font
from renderlib.image import Image
from renderlib.mesh import Mesh
from renderlib.text import Text
from renderlib.texture import Texture
from sdl2 import *
from time import time

#: Stats update interval in seconds
UPDATE_INTERVAL = 2.0

class Demo:
    def __init__(self, width, height):
        self.win = self.ctx = None

        # create a SDL window
        self.win = SDL_CreateWindow(
            b'Demo',
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_OPENGL)
        if self.win is None:
            raise RuntimeError('failed to create SDL window')

        # create an OpenGL context
        SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK,
            SDL_GL_CONTEXT_PROFILE_CORE)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3)
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)
        self.ctx = SDL_GL_CreateContext(self.win)

        SDL_GL_SetSwapInterval(0)

        if self.ctx is None:
            raise RuntimeError('failed to initialize OpenGL context')

        # initialize renderer
        renderer_init()

        self.width = width
        self.height = height
        self.aspect = width / float(height)

        # initialize controls
        self.play_animation = False

        # initialize UI projection matrix
        self.ui_projection = Mat()
        self.ui_projection.ortho(
            -self.width / 2,
            +self.width / 2,
            +self.height / 2,
            -self.height / 2,
            0,
            1)

        # initialize camera
        self.camera_eye = Vec(5, 5, 5, 0)
        self.camera_projection = Mat()
        self.camera_projection.persp(
            30.0,
            self.aspect,
            1,
            100)
        self.camera_view = Mat()
        self.camera_view.lookatv(
            self.camera_eye,
            Vec(0, 0, 0),
            Vec(0, 1, 0))

        # initialize light
        self.light = Light()
        self.light.direction = Vec(0, -5, -5)
        self.light.direction.norm()
        self.light.color = Vec(1, 1, 1)
        self.light.ambient_intensity = 0.3
        self.light.diffuse_intensity = 1.0

        proj = Mat()
        proj.ortho(
            -5,
            +5,
            +5 * self.aspect,
            -5 * self.aspect,
            0,
            10)

        view = Mat()
        view.lookat(
            0, 5, 5,
            0, 0, 0,
            0, 1, 0)

        self.light.transform = proj * view

        self.load_resources()

    def __del__(self):
        renderer_shutdown()
        SDL_GL_DeleteContext(self.ctx)
        SDL_DestroyWindow(self.win)
        SDL_Quit()

    def load_resources(self):
        self.mesh = Mesh.from_file('tests/data/zombie.mesh')
        self.animation = AnimationInstance(self.mesh.animations[0])
        self.image = Image.from_file('tests/data/zombie.jpg')
        self.texture = Texture.from_image(
            self.image, Texture.TextureType.texture_2d)
        self.terrain_mesh = Mesh.from_file('tests/data/plane.mesh')
        self.grass_img = Image.from_file('tests/data/grass.jpg')
        self.terrain_texture = Texture.from_image(
            self.grass_img, Texture.TextureType.texture_2d)
        self.font = Font.from_file('tests/data/courier.ttf', 16)
        self.fps_text = Text(self.font)

        self.material = Material()
        self.material.texture = self.texture
        self.material.receive_light = True
        self.material.specular_intensity = 0.3
        self.material.specular_power = 4

        self.terrain_material = Material()
        self.terrain_material.texture = self.terrain_texture
        self.terrain_material.receive_light = True

    def update(self, dt):
        evt = SDL_Event()
        while SDL_PollEvent(evt):
            if evt.type == SDL_QUIT:
                return False
            elif evt.type == SDL_KEYUP:
                if evt.key.keysym.sym == SDLK_ESCAPE:
                    return False
                elif evt.key.keysym.sym == SDLK_SPACE:
                    self.play_animation = not self.play_animation
            elif evt.type == SDL_MOUSEBUTTONDOWN and \
                    evt.button.x >= self.width - 40 and \
                    evt.button.x <= self.width and \
                    evt.button.y >= 2 and \
                    evt.button.y <= 40:
                return False

        if self.play_animation:
            self.animation.play(dt)

        return True

    def update_stats(self, fps, render_time):
        self.fps_text.string = 'FPS: {fps}, render time: {time:.2f}ms'.format(
            fps=int(fps),
            time=render_time * 1000)

    def render(self):
        mesh_props = MeshRenderProps()
        mesh_props.eye = self.camera_eye
        mesh_props.model = self.mesh.transform
        mesh_props.view = self.camera_view
        mesh_props.projection = self.camera_projection
        mesh_props.cast_shadows = True
        mesh_props.receive_shadows = True
        mesh_props.light = self.light
        mesh_props.animation = self.animation
        mesh_props.material = self.material
        render_mesh(self.mesh, mesh_props)

        terrain_props = MeshRenderProps()
        terrain_props.eye = self.camera_eye
        terrain_props.model = self.terrain_mesh.transform
        terrain_props.view = self.camera_view
        terrain_props.projection = self.camera_projection
        terrain_props.cast_shadows = False
        terrain_props.receive_shadows = True
        terrain_props.light = self.light
        terrain_props.material = self.terrain_material
        terrain_props.model.scale(2, 2, 1)
        render_mesh(self.terrain_mesh, terrain_props)

        text_props = TextRenderProps()
        text_props.color = Vec(0.5, 1.0, 0.5, 1.0)
        text_props.projection = self.ui_projection
        text_props.opacity = 1.0
        text_props.model.translate(-self.width / 2 + 10, self.height / 2 - 10, 0)
        render_text(self.fps_text, text_props)

        renderer_present()
        SDL_GL_SwapWindow(self.win)

    def run(self):
        fps = render_time = time_acc = 0
        last_update = time()

        while True:
            # compute time delta
            now = time()
            dt = now - last_update
            last_update = now

            # update stats
            time_acc += dt
            if time_acc >= UPDATE_INTERVAL:
                time_acc -= UPDATE_INTERVAL
                self.update_stats(fps / UPDATE_INTERVAL, render_time)
                fps = 0
            else:
                fps += 1

            # update the scene
            if not self.update(dt):
                break

            # render and measure rendering time
            now = time()
            self.render()
            render_time = time() - now


if __name__ == '__main__':
    try:
        demo = Demo(800, 600)
        demo.run()
    except RuntimeError as err:
        print(err)
        error_print_traceback()