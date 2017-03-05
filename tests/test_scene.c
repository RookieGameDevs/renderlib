#include "fixture.h"
#include <renderlib.h>
#include <check.h>
#include <stdlib.h>

START_TEST(test_create_and_initialize)
{
	struct Scene *scene = scene_new();
	ck_assert(scene);

	struct Object *mesh_obj = scene_add_mesh(scene, NULL, NULL);
	ck_assert(mesh_obj);

	struct Object *text_obj = scene_add_text(scene, NULL, NULL);
	ck_assert(text_obj && text_obj != mesh_obj);

	struct Object *quad_obj = scene_add_quad(scene, NULL, NULL);
	ck_assert(quad_obj && quad_obj != text_obj && quad_obj != mesh_obj);

	ck_assert_int_eq(scene_object_count(scene), 3);
	scene_remove_object(scene, text_obj);
	ck_assert_int_eq(scene_object_count(scene), 2);
	scene_remove_object(scene, mesh_obj);
	ck_assert_int_eq(scene_object_count(scene), 1);
	scene_remove_object(scene, quad_obj);
	ck_assert_int_eq(scene_object_count(scene), 0);

	scene_free(scene);
}
END_TEST

START_TEST(test_render)
{
	struct Mesh *mesh = mesh_from_file("tests/data/zombie.mesh");
	ck_assert(mesh);

	struct MeshProps props = {
		.cast_shadows = 1,
		.receive_shadows = 1,
		.animation = NULL,
		.material = NULL
	};

	Mat identity;
	mat_ident(&identity);

	struct Camera camera = {
		.position = vec(0, 0, 0, 0),
		.view = identity,
		.projection = identity
	};

	struct Light light = {
		.projection = identity,
		.direction = vec(0, -1, 0, 0),
		.color = vec(1, 1, 1, 1),
		.ambient_intensity = 0.3,
		.diffuse_intensity = 0.8
	};

	struct Scene *scene = scene_new();
	struct Object *obj = scene_add_mesh(scene, mesh, &props);
	ck_assert(obj);

	int ok = scene_render(scene, &camera, &light);
	ck_assert(ok);

	scene_free(scene);
	mesh_free(mesh);
}
END_TEST

static void
suite_setup(void)
{
	setup();
}

static void
suite_teardown(void)
{
	teardown();
}

Suite*
scene_suite(void)
{
	Suite *s = suite_create("scene");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, suite_setup, suite_teardown);
	tcase_add_test(tc_core, test_create_and_initialize);
	tcase_add_test(tc_core, test_render);

	suite_add_tcase(s, tc_core);

	return s;
}

