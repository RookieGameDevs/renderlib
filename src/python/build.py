"""CFFI wrapper entry point."""

from cffi import FFI

ffi = FFI()
ffi.set_source(
    '_renderlib',
    """
    #include "renderlib.h"
    #include <GL/glew.h>
    """,
    include_dirs=[
        '../../build/include/renderlib',
        '../../deps/matlib/build/include'
    ],
    library_dirs=['../../build/lib'],
    libraries=['render'])

# OpenGL types
ffi.cdef(
    """
    typedef int... GLenum;
    typedef unsigned int... GLuint;
    enum {
        GL_TEXTURE_2D,
        GL_TEXTURE_RECTANGLE,
        ...
    };
    """)

# Core API
ffi.cdef(
    """
    int
    renderer_init();

    int
    renderer_present(void);

    void
    renderer_shutdown(void);
    """)

# Mesh API
ffi.cdef(
    """
    struct Mesh;

    struct Mesh*
    mesh_from_file(const char *filename);

    struct Mesh*
    mesh_from_buffer(const void *data, size_t data_size);
    """)

if __name__ == '__main__':
    ffi.compile(verbose=True)
