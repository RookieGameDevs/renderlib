#include "fixture.h"
#include <check.h>
#include <stdlib.h>

#include "image.h"

START_TEST(test_load_png_from_file)
{
	struct Image *image = image_from_file("tests/data/star.png");
	ck_assert(image != NULL);
	ck_assert_uint_eq(image->width, 31);
	ck_assert_uint_eq(image->height, 30);
	ck_assert(image->data != NULL);
	ck_assert_int_eq(image->format, IMAGE_FORMAT_RGBA);
	image_free(image);

	image = image_from_file("notfound.png");
	ck_assert(image == NULL);
}
END_TEST

START_TEST(test_load_jpeg_from_file)
{
	struct Image *image = image_from_file("tests/data/zombie.jpg");
	ck_assert(image != NULL);
	ck_assert_uint_eq(image->width, 2048);
	ck_assert_uint_eq(image->height, 2048);
	ck_assert(image->data != NULL);
	ck_assert_int_eq(image->format, IMAGE_FORMAT_RGB);
	image_free(image);

	image = image_from_file("notfound.jpg");
	ck_assert(image == NULL);
}
END_TEST

Suite*
image_suite(void)
{
	Suite *s = suite_create("image");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_load_png_from_file);
	tcase_add_test(tc_core, test_load_jpeg_from_file);
	suite_add_tcase(s, tc_core);

	return s;
}
