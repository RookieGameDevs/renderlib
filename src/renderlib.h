#include <matlib.h>

#include "anim.h"
#include "error.h"
#include "font.h"
#include "image.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

/**
 * Light.
 */
struct Light {
	Mat transform;
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
	float specular_intensity;
	float specular_power;
};


/**
 * Mesh render properties.
 */
struct MeshRenderProps {
	Mat model, view, projection;         // transforms
	int cast_shadows;                    // should cast shadows
	int receive_shadows;                 // should receive shadows
	struct Light *light;                 // light to use
	struct AnimationInstance *animation; // animation instance
	struct Material *material;           // material to apply
};

/**
 * Initialize renderer library.
 */
int
renderer_init(void);

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
render_mesh(struct Mesh *mesh, struct MeshRenderProps *props);