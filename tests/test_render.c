#include "fixture.h"
#include <check.h>
#include <stdlib.h>

#include "mesh.h"
#include "renderer.h"

START_TEST(test_render_mesh_simple)
{
	struct Mesh *mesh = mesh_from_file("tests/data/zombie.mesh", NULL);

	Mat identity;
	mat_ident(&identity);

	struct MeshRenderProps props = {
		.view = identity,
		.model = identity,
		.projection = identity
	};
	ck_assert(render_mesh(mesh, &props, NULL));
	ck_assert(renderer_present(NULL));
	mesh_free(mesh);
}
END_TEST

Suite*
render_suite(void)
{
	Suite *s = suite_create("render");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_render_mesh_simple);

	suite_add_tcase(s, tc_core);

	return s;
}

