"""Core API."""
from _renderlib import lib


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
