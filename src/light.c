// renderlib
#include "camera.h"
#include "light.h"

// standard library
#include <assert.h>
#include <stddef.h>

void
light_update_projection(struct Light *light, struct Camera *camera, AABB *bounding_box)
{
	assert(light != NULL);
	assert(camera != NULL);

	// compute light space matrix as a rotation along light direction vector
	// and Y axis as up vector
	Vec x, y = {{ 0, 1, 0 }}, z = light->direction;
	vec_cross(&z, &y, &x);
	vec_cross(&x, &z, &y);
	Mat light_space = {{
		x.data[0], y.data[0], z.data[0], 0,
		x.data[1], y.data[1], z.data[1], 0,
		x.data[2], y.data[2], z.data[2], 0,
		0,         0,         0,         1
	}};

	// compute the inverse projection-view matrix
	Mat view, proj, tmp, inv_view_proj, inv_light_space;
	camera_get_matrices(camera, &view, &proj);
	mat_mul(&proj, &view, &tmp);
	mat_inverse(&tmp, &inv_view_proj);
	mat_inverse(&light_space, &inv_light_space);

	// reverse-map NDC coordinates and obtain view frustum corners
	Vec corners[8] = {
		{{  1,  1,  1,  1 }}, // right-top-back
		{{  1, -1,  1,  1 }}, // right-bottom-back
		{{ -1, -1,  1,  1 }}, // left-bottom-back
		{{ -1,  1,  1,  1 }}, // left-top-back
		{{  1,  1, -1,  1 }}, // right-top-front
		{{  1, -1, -1,  1 }}, // right-bottom-front
		{{ -1, -1, -1,  1 }}, // left-bottom-front
		{{ -1,  1, -1,  1 }}  // left-top-front
	};
	for (short i = 0; i < 8; i++) {
		// find the frustum corner coordinate in world space
		Vec v;
		mat_mulv(&inv_view_proj, &corners[i], &v);
		vec_imulf(&v, 1.0f / v.data[3]);  // perspective division
		v.data[3] = 0;  // let it be a point

		// find the frustum coordinate in light space
		mat_mulv(&inv_light_space, &v, &corners[i]);
	}

	// find the coordinates of the front right top and back bottom left
	// vertices which form the AABB
	Vec v1 = corners[0], v2 = corners[0];
	for (short i = 0; i < 8; i++) {
		for (short j = 0; j < 3; j++) {
			v1.data[j] = fmax(v1.data[j], corners[i].data[j]);
			v2.data[j] = fmin(v2.data[j], corners[i].data[j]);
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