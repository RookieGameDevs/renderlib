#include "fixture.h"
#include <check.h>

#include "geometry.h"

START_TEST(test_simple)
{
	// create empty geometry object
	struct Geometry *geom = geometry_new();
	ck_assert(geom != NULL);

	// create an empty buffer large enough to contain 3 vertex coordinates
	struct Buffer *position_buffer = buffer_new(sizeof(float) * 9, NULL);
	ck_assert(position_buffer != NULL);

	// initialize the buffer with triangle vertices positions
	float positions[] = {
		-0.5, 0.0, 0.0,
		 0.5, 0.0, 0.0,
		 0.0, 0.7, 0.0
	};
	int updated = buffer_update(position_buffer, positions);
	ck_assert(updated);

	// map the buffer to `position` attribute
	int added = geometry_add_attribute(
		geom,
		position_buffer,
		"position",
		GL_FLOAT,
		3,
		0,
		0
	);
	ck_assert(added);

	// create another buffer for per-vertex normals attribute
	float normals[] = {
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};
	struct Buffer *normal_buffer = buffer_new(sizeof(normals), normals);
	ck_assert(normal_buffer);

	// map the buffer to `normal` attribute
	added = geometry_add_attribute(
		geom,
		normal_buffer,
		"normal",
		GL_FLOAT,
		3,
		0,
		0
	);
	ck_assert(added);

	// checks
	ck_assert_uint_eq(geom->attribute_count, 2);

	// cleanup
	buffer_free(position_buffer);
	buffer_free(normal_buffer);
	geometry_free(geom);
}
END_TEST

Suite*
geometry_suite(void)
{
	Suite *s = suite_create("geometry");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_simple);
	suite_add_tcase(s, tc_core);

	return s;
}