import pytest
import renderlib
from sdl2 import *


@pytest.fixture(scope='session')
def context(request):
    # create a SDL window
    SDL_Init(SDL_INIT_VIDEO)
    win = SDL_CreateWindow(
        b'renderlib test context',
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        480,
        480,
        SDL_WINDOW_OPENGL)
    if win is None:
        raise RuntimeError('failed to create SDL window')

    # create an OpenGL context
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3)
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)
    ctx = SDL_GL_CreateContext(win)

    if ctx is None:
        SDL_DestroyWindow(win)
        raise RuntimeError('failed to initialize OpenGL context')

    # initialize renderer
    renderlib.core.renderer_init()

    # add clean-up finalizer
    def shutdown():
        renderlib.core.renderer_shutdown()
        SDL_GL_DeleteContext(ctx)
        SDL_DestroyWindow(win)
        SDL_Quit()
    request.addfinalizer(shutdown)

    return True
