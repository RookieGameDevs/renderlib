#include "fixture.h"
#include <renderlib.h>
#include <check.h>
#include <stdlib.h>


START_TEST(test_create_and_initialize)
{
	struct Scene *scene = scene_new();
	ck_assert(scene);

	struct Object *mesh_obj = scene_add_mesh(scene, NULL);
	ck_assert(mesh_obj);

	struct Object *text_obj = scene_add_text(scene, NULL);
	ck_assert(text_obj && text_obj != mesh_obj);

	struct Object *quad_obj = scene_add_quad(scene, NULL);
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

	suite_add_tcase(s, tc_core);

	return s;
}

