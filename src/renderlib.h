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
 * Set of uniform values.
 */
struct RenderPassUniformSet {
	struct RenderPassUniformSetCls *cls;
};

/**
 * Uniform value set class.
 */
struct RenderPassUniformSetCls {
	void
	(*free)(struct RenderPassUniformSet *s);

	struct ShaderUniformValue*
	(*get_value)(struct RenderPassUniformSet *s, int id);

	struct ShaderUniformValue*
	(*get_values)(struct RenderPassUniformSet *s, unsigned *r_count);
};

/**
 * Render pass.
 */
struct RenderPass {
	const struct RenderPassCls *cls;
};

struct RenderPassCls {
	const char *name;

	struct RenderPass*
	(*alloc)(void);

	void
	(*free)(struct RenderPass *pass);

	int
	(*enter)(struct RenderPass *pass);

	int
	(*exit)(struct RenderPass *pass);

	struct Shader*
	(*get_shader)(struct RenderPass *pass);

	struct RenderPassUniformSet*
	(*create_uniform_set)(struct RenderPass *pass);
};

struct RenderCommand {
	int pass;
	struct Geometry *geometry;
	int primitive_type;
	struct ShaderUniformValue *values;
	size_t values_count;

	int (*pre_exec)(void *userdata);
	int (*post_exec)(void *userdata);
	void *userdata;
};

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
renderer_add_command(const struct RenderCommand *cmd);

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

struct RenderPass*
renderer_get_pass(int pass);

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
