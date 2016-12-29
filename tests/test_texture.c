#include "image.h"
#include "renderer.h"
#include "texture.h"
#include <check.h>
#include <stdlib.h>

START_TEST(test_create_from_image)
{
	struct Image *image = image_from_file("data/star.png", NULL);
	ck_assert(image != NULL);

	struct Texture *tex = texture_from_image(image, GL_TEXTURE_RECTANGLE, NULL);
	ck_assert(tex != NULL);
	ck_assert_uint_ne(tex->id, 0);
	ck_assert_int_eq(tex->type, GL_TEXTURE_RECTANGLE);

	image_free(image);
	texture_free(tex);
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
	Suite *s = suite_create("texture");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_create_from_image);

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


