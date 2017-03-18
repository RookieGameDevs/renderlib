// renderlib
#include "camera.h"
#include "light.h"

// standard library
#include <assert.h>
#include <stddef.h>

void
light_update_projection(struct Light *light, struct Camera *camera)
{
	assert(light != NULL);
	assert(camera != NULL);

	// compute light space matrix as a rotation along light direcition
	// vector and Y axis as up vector
	Vec x, y = {{ 0, 1, 0 }}, z = light->direction;
	vec_cross(&z, &y, &x);
	vec_cross(&x, &z, &y);
	Mat light_space = {{
		x.data[0], y.data[0], z.data[0], 0,
		x.data[1], y.data[1], z.data[1], 0,
		x.data[2], y.data[2], z.data[2], 0,
		0,         0,         0,         1
	}};

	// combine the inverse of camera view-projection and the inverse of
	// light space transform
	Mat inv_proj_view_light, inv_proj_view, inv_light, tmp;
	mat_mul(&camera->projection, &camera->view, &tmp);
	mat_inverse(&tmp, &inv_proj_view);
	mat_inverse(&light_space, &inv_light);
	mat_mul(&inv_proj_view, &inv_light, &inv_proj_view_light);

	// compute the position of the eight view frustum corners in light space
	Vec view_corners[8];
	Vec ndc_corners[8] = {
		{{  1,  1,  1 }}, // front-right-top
		{{  1, -1,  1 }}, // front-right-bottom
		{{ -1, -1,  1 }}, // front-left-bottom
		{{ -1,  1,  1 }}, // front-left-top
		{{  1,  1, -1 }}, // back-right-top
		{{  1, -1, -1 }}, // back-right-bottom
		{{ -1, -1, -1 }}, // back-left-bottom
		{{ -1,  1, -1 }}  // back-left-top
	};
	for (short i = 0; i < 8; i++) {
		mat_mulv(&inv_proj_view_light, &ndc_corners[i], &view_corners[i]);
	}

	// find the coordinates of the front right top and back bottom left
	// vertices which form the AABB
	Vec v1 = view_corners[0], v2 = view_corners[7];
	for (short i = 0; i < 8; i++) {
		for (short j = 0; j < 3; j++) {
			v1.data[j] = fmax(v1.data[j], view_corners[i].data[j]);
			v2.data[j] = fmin(v2.data[j], view_corners[i].data[j]);
		}
	}

	// compute the orthographic projection matrix using bounding box extents
	Mat light_proj;
	mat_ortho(
		&light_proj,
		v2.data[0],
		v1.data[0],
		v1.data[1],
		v2.data[1],
		v1.data[2],
		v2.data[2]
	);
	mat_mul(&light_proj, &light_space, &light->projection);
}