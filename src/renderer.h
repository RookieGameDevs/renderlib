#pragma once

#include "errors.h"
#include <matlib.h>

struct Mesh;

int
renderer_init(err_t *r_err);

int
renderer_present(err_t *r_err);

void
renderer_shutdown(void);

struct MeshRenderProps {
	Mat model, view, projection;  // transforms
	int cast_shadows;             // should cast shadows
	int receive_shadows;          // should receive shadows
};

int
render_mesh(
	struct Mesh *mesh,
	struct MeshRenderProps *props,
	err_t *r_err
);