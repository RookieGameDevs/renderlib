#pragma once

#include <matlib.h>

struct Mesh;

int
draw_mesh(
	struct Mesh *mesh,
	Mat *model,
	Mat *view,
	Mat *proj
);
