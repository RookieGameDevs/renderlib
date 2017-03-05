#include "anim.h"
#include "error.h"
#include "font.h"
#include "image.h"
#include "mesh.h"
#include "scene.h"
#include "shader.h"
#include "text.h"
#include "texture.h"
#include <matlib.h>

/**
 * Camera.
 */
struct Camera {
	Vec position;
	Mat view;
	Mat projection;
};

/**
 * Light.
 */
struct Light {
	Mat projection;
	Vec direction;
	Vec color;
	float ambient_intensity;
	float diffuse_intensity;
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
render_text(struct Text *text, struct TextProps *props, struct Transform *t);

/**
 * Render a colored/textured quad.
 */
int
render_quad(struct Quad *quad, struct QuadProps *props, struct Transform *t);
