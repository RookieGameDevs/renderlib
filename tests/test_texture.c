#include "fixture.h"
#include <check.h>
#include <stdlib.h>

#include "image.h"
#include "texture.h"

START_TEST(test_create_from_image)
{
	struct Image *image = image_from_file("tests/data/star.png");
	ck_assert(image != NULL);

	struct Texture *tex = texture_from_image(image, GL_TEXTURE_RECTANGLE);
	ck_assert(tex != NULL);
	ck_assert_uint_ne(tex->id, 0);
	ck_assert_int_eq(tex->type, GL_TEXTURE_RECTANGLE);

	image_free(image);
	texture_free(tex);
}
END_TEST

Suite*
texture_suite(void)
{
	Suite *s = suite_create("texture");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_create_from_image);

	suite_add_tcase(s, tc_core);

	return s;
}
