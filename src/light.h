#pragma once

#include <matlib.h>

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

void
light_update_projection(struct Light *light, struct Camera *camera);