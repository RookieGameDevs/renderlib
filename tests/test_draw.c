#include "draw.h"
#include "mesh.h"
#include "renderer.h"
#include <check.h>
#include <stdlib.h>

START_TEST(test_draw_mesh)
{
	struct Mesh *mesh = mesh_from_file("tests/data/zombie.mesh", NULL);
	Mat identity;
	mat_ident(&identity);
	ck_assert(draw_mesh(mesh, &mesh->transform, &identity, &identity));
	ck_assert(renderer_present());
	mesh_free(mesh);
}
END_TEST

static void
setup(void)
{
	ck_assert(renderer_init(800, 800, NULL));
}

static void
teardown(void)
{
	renderer_shutdown();
}

Suite*
draw_suite(void)
{
	Suite *s = suite_create("draw");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_draw_mesh);

	suite_add_tcase(s, tc_core);

	return s;
}

