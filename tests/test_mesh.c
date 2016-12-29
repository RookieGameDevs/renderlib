#include "anim.h"
#include "mesh.h"
#include "renderer.h"
#include <check.h>
#include <stdlib.h>

START_TEST(test_create_simple)
{
	float vertices[][3] = {
		{ -0.3f, -0.3f,  0.0f },
		{ 0.3f, -0.3f,  0.0f },
		{ 0.0f,  0.3f,  0.0f }
	};

	float normals[][3] = {
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f }
	};

	uint32_t indices[] = { 0, 1, 2 };

	struct Mesh *mesh = mesh_new(
		vertices,
		normals,
		NULL,
		NULL,
		NULL,
		3,
		indices,
		3,
		NULL
	);
	ck_assert(mesh != NULL);
	ck_assert_int_eq(mesh->vertex_count, 3);
	ck_assert_int_eq(mesh->index_count, 3);
	ck_assert_int_eq(mesh->anim_count, 0);
	mesh_free(mesh);
}
END_TEST

START_TEST(test_create_from_file)
{
	struct Mesh *mesh = mesh_from_file("tests/data/zombie.mesh", NULL);
	ck_assert(mesh != NULL);
	ck_assert_int_eq(mesh->vertex_count, 37368);
	ck_assert_int_eq(mesh->index_count, 37368);
	ck_assert_int_eq(mesh->anim_count, 1);
	ck_assert_int_eq(mesh->skeleton->joint_count, 27);
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
mesh_suite(void)
{
	Suite *s = suite_create("mesh");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_create_simple);
	tcase_add_test(tc_core, test_create_from_file);

	suite_add_tcase(s, tc_core);

	return s;
}
