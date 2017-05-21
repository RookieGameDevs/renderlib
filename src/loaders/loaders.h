#pragma once

#include <matlib.h>
#include <stddef.h>

struct Geometry;
struct Animation;

int
load_mesh(
	void *data,
	size_t size,
	struct Geometry **r_geometry,
	struct Animation **r_animations,
	unsigned *r_anim_count,
	Mat *r_transform
);