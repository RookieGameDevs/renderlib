"""CFFI wrapper entry point."""

from cffi import FFI
from matlib.ffi import ffi as matlib_ffi

ffi = FFI()

# matlib FFI dependency
ffi.include(matlib_ffi)

ffi.set_source(
    '_renderlib',
    """
    #include "renderlib.h"
    #include <GL/glew.h>
    #include <matlib.h>
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
    struct Light {
        struct Mat transform;
        struct Vec direction;
        struct Vec color;
        float ambient_intensity;
        float diffuse_intensity;
    };

    struct Material {
        struct Texture *texture;
        struct Vec color;
        int receive_light;
        float specular_intensity;
        float specular_power;
    };

    struct MeshRenderProps {
        struct Vec eye;
        struct Mat model, view, projection;
        int cast_shadows;
        int receive_shadows;
        struct Light *light;
        struct AnimationInstance *animation;
        struct Material *material;
    };

    int
    renderer_init();

    int
    renderer_present(void);

    void
    renderer_shutdown(void);

    int
    render_mesh(struct Mesh *mesh, struct MeshRenderProps *props);
    """)

# Mesh API
ffi.cdef(
    """
    struct Mesh;

    struct Mesh*
    mesh_from_file(const char *filename);

    struct Mesh*
    mesh_from_buffer(const void *data, size_t data_size);

    void
    mesh_free(struct Mesh *mesh);
    """)

# Font API
ffi.cdef(
    """
    struct Font;

    struct Font*
    font_from_buffer(const void *data, size_t size, unsigned pt);

    struct Font*
    font_from_file(const char *filename, unsigned pt);

    void
    font_free(struct Font *font);
    """)

# Image API
ffi.cdef(
    """
    enum {
        IMAGE_FORMAT_RGBA,
        IMAGE_FORMAT_RGB,
        IMAGE_CODEC_PNG,
        IMAGE_CODEC_JPEG,
        ...
    };

    struct Image {
        unsigned width, height;
        int format;
        ...;
    };

    struct Image*
    image_from_buffer(const void *data, size_t size, int codec);

    struct Image*
    image_from_file(const char *filename);

    void
    image_free(struct Image *image);
    """)
