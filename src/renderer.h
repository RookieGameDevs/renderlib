#pragma once

#include "errors.h"
#include <matlib.h>

struct Mesh;

/**
 * Initialize renderer library.
 */
int
renderer_init(err_t *r_err);

/**
 * Render current render queue and flush it.
 */
int
renderer_present(err_t *r_err);

/**
 * Shutdown renderer library and free resources.
 */
void
renderer_shutdown(void);

/**
 * Mesh render properties.
 */
struct MeshRenderProps {
	Mat model, view, projection;  // transforms
	int cast_shadows;             // should cast shadows
	int receive_shadows;          // should receive shadows
};

/**
 * Add a mesh to render queue.
 */
int
render_mesh(struct Mesh *mesh, struct MeshRenderProps *props, err_t *r_err);