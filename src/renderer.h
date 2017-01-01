#pragma once

#include <matlib.h>

struct Mesh;

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
 * Mesh render properties.
 */
struct MeshRenderProps {
	Mat model, view, projection;         // transforms
	int cast_shadows;                    // should cast shadows
	int receive_shadows;                 // should receive shadows
	int enable_animation;                // should apply animation
	struct AnimationInstance *animation; // animation instance
	struct Texture *texture;             // texture to apply
};

/**
 * Add a mesh to render queue.
 */
int
render_mesh(struct Mesh *mesh, struct MeshRenderProps *props);