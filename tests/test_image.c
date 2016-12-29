#include "image.h"
#include "renderer.h"
#include <check.h>
#include <stdlib.h>

START_TEST(test_load_from_file)
{
	struct Image *image = image_from_file("tests/data/star.png", NULL);
	ck_assert(image != NULL);
	ck_assert_uint_eq(image->width, 31);
	ck_assert_uint_eq(image->height, 30);
	ck_assert(image->data != NULL);
	image_free(image);

	err_t err = 0;
	image = image_from_file("notfound.png", &err);
	ck_assert(image == NULL);
	ck_assert_int_eq(err, ERR_NO_FILE);
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
image_suite(void)
{
	Suite *s = suite_create("image");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_load_from_file);

	suite_add_tcase(s, tc_core);

	return s;
}
