#include "fixture.h"
#include <check.h>
#include <stdlib.h>

#include "image.h"

START_TEST(test_load_from_file)
{
	struct Image *image = image_from_file("tests/data/star.png");
	ck_assert(image != NULL);
	ck_assert_uint_eq(image->width, 31);
	ck_assert_uint_eq(image->height, 30);
	ck_assert(image->data != NULL);
	image_free(image);

	image = image_from_file("notfound.png");
	ck_assert(image == NULL);
}
END_TEST

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
