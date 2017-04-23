// renderlib
#include "anim.h"
#include "camera.h"
#include "error.h"
#include "font.h"
#include "geometry.h"
#include "image.h"
#include "light.h"
#include "mesh.h"
#include "scene.h"
#include "shader.h"
#include "text.h"
#include "texture.h"

// matlib
#include <matlib.h>

/**
 * Material.
 */
struct Material {
	struct Texture *texture;
	Vec color;
	int receive_light;
	float specular_intensity;
	float specular_power;
};

/**
 * Quad.
 */
struct Quad {
	float width;
	float height;
};

/**
 * Object transform.
 */
struct Transform {
	Mat model;
	Mat view;
	Mat projection;
};

/**
 * Mesh render properties.
 */
struct MeshProps {
	int cast_shadows;                    // should cast shadows
	int receive_shadows;                 // should receive shadows
	struct AnimationInstance *animation; // animation instance
	struct Material *material;           // material to apply
};

/**
 * Text render properties.
 */
struct TextProps {
	Vec color;                    // text color
	float opacity;                // text opacity
};

/**
 * Quad render properties.
 */
struct QuadProps {
	Vec color;                    // fill color
	struct Texture *texture;      // texture to apply
	struct {
		float left, top;
		float right, bottom;
	} borders;                    // texture borders
	float opacity;                // opacity; 0 = transparent, 1 = opaque
};

enum {
	RENDER_TARGET_FRAMEBUFFER,
	RENDER_TARGET_OVERLAY
};

enum {
	SHADOW_PASS,
	TEXT_PASS,
	QUAD_PASS,
	PHONG_PASS
};

/**
 * Initialize renderer library.
 */
int
renderer_init(void);

/**
 * Clear render buffers.
 */
void
renderer_clear(void);

/**
 * Add a geometry draw command to render queue.
 */
int
renderer_draw(
	struct Geometry *geom,
	int pass,
	struct ShaderUniformValue *values,
	size_t value_count
);

/**
 * Get a pointer to render pass shader.
 */
struct Shader*
renderer_get_shader(int pass);

/**
 * Render current render queue and flush it.
 */
int
renderer_present(void);

/**
 * Shutdown renderer library and free resources.
 */
void
renderer_shutdown(void);

/**
 * Render a mesh.
 */
int
render_mesh(
	int render_target,
	struct Mesh *mesh,
	struct MeshProps *props,
	struct Transform *t,
	struct Light *light,
	Vec *eye
);

/**
 * Render text.
 */
int
render_text(
	int render_target,
	struct Text *text,
	struct TextProps *props,
	struct Transform *t
);

/**
 * Render a colored/textured quad.
 */
int
render_quad(
	int render_target,
	struct Quad *quad,
	struct QuadProps *props,
	struct Transform *t
);
