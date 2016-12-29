#include "font.h"
#include "renderer.h"
#include <check.h>
#include <stdlib.h>

START_TEST(test_load_from_file)
{
	struct Font *font = font_from_file("data/courier.ttf", 12, NULL);
	ck_assert(font != NULL);
	font_free(font);

	err_t err = 0;
	font = font_from_file("data/notfound.ttf", 12, &err);
	ck_assert(font == NULL);
	ck_assert_int_ne(err, 0);
	font_free(font);
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

static Suite*
mesh_suite(void)
{
	Suite *s = suite_create("font");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_load_from_file);

	suite_add_tcase(s, tc_core);

	return s;
}

int
main(int argc, char *argv[])
{
	Suite *s = mesh_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	int failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}


