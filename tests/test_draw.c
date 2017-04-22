#include "fixture.h"
#include <check.h>

#include "renderlib.h"

static void
make_test_geometry(
	struct Geometry **r_geom,
	struct Buffer **r_positions,
	struct Buffer **r_normals
) {
	ck_assert(r_geom != NULL);
	ck_assert(r_positions != NULL);
	ck_assert(r_normals != NULL);
	*r_geom = NULL;
	*r_positions = NULL;
	*r_normals = NULL;

	float positions[] = {
		-0.5, 0.0, 0.0,
		 0.5, 0.0, 0.0,
		 0.0, 0.7, 0.0
	};
	struct Buffer *pos_buf = buffer_new(sizeof(positions), positions);

	float normals[] = {
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};
	struct Buffer *norm_buf = buffer_new(sizeof(normals), normals);

	struct Geometry *geom = geometry_new();
	geometry_add_attribute(
		geom,
		pos_buf,
		"position",
		GL_FLOAT,
		3,
		0,
		0
	);
	geometry_add_attribute(
		geom,
		norm_buf,
		"normal",
		GL_FLOAT,
		3,
		0,
		0
	);

	*r_geom = geom;
	*r_positions = pos_buf;
	*r_normals = norm_buf;
}

START_TEST(test_simple_draw)
{
	struct Geometry *geom;
	struct Buffer *pos_buf, *norm_buf;
	make_test_geometry(&geom, &pos_buf, &norm_buf);

	// vertex shader uniform values
	Mat model, view, projection;
	mat_ident(&model);
	mat_ident(&view);
	mat_ident(&projection);
	int enable_skinning = 0;
	int enable_shadow_mapping = 0;

	// fragment shader uniform values
	int enable_lighting = 0;
	int enable_texture_mapping = 0;
	Vec color = vec(1, 1, 1, 1);

	// retrieve the Phong pass shader
	struct Shader *shader = renderer_get_shader(PHONG_PASS);
	ck_assert(shader);

	// check that the shader accepts the attributes specified in geometry
	for (unsigned i = 0; i < geom->attribute_count; i++) {
		int attr_found = 0;
		for (unsigned j = 0; j < shader->attribute_count; j++) {
			if (strcmp(geom->attributes[i].name, shader->attributes[j].name) == 0) {
				attr_found = 1;
				break;
			}
		}
		ck_assert(attr_found);
	}

	// populate a NUL-terminated array of ShadowUniformValue structs
	const char *names[] = {
		"model",
		"view",
		"projection",
		"enable_skinning",
		"enable_shadow_mapping",
		"enable_lighting",
		"enable_texture_mapping",
		"material.color",
	};
	struct ShaderUniformValue values[] = {
		{ .count = 1, .data = &model },
		{ .count = 1, .data = &view },
		{ .count = 1, .data = &projection },
		{ .count = 1, .data = &enable_skinning },
		{ .count = 1, .data = &enable_shadow_mapping },
		{ .count = 1, .data = &enable_lighting },
		{ .count = 1, .data = &enable_texture_mapping },
		{ .count = 1, .data = &color },
		{ .count = 0 }
	};
	for (unsigned i = 0; i < sizeof(names) / sizeof(char*); i++) {
		struct ShaderUniform *uniform = NULL;
		for (unsigned j = 0; j < shader->uniform_count; j++) {
			if (strcmp(shader->uniforms[j].name, names[i]) == 0) {
				uniform = &shader->uniforms[j];
				break;
			}
		}
		// check that a uniform with given name was found
		ck_assert(uniform);
		values[i].uniform = uniform;
	}

	// perform the draw
	ck_assert(renderer_draw(geom, PHONG_PASS, values));

	buffer_free(norm_buf);
	buffer_free(pos_buf);
	geometry_free(geom);
}
END_TEST

Suite*
draw_suite(void)
{
	Suite *s = suite_create("draw");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_simple_draw);
	suite_add_tcase(s, tc_core);

	return s;
}