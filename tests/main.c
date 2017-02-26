#include <check.h>
#include <stdlib.h>

Suite*
font_suite(void);

Suite*
image_suite(void);

Suite*
mesh_suite(void);

Suite*
render_suite(void);

Suite*
scene_suite(void);

Suite*
shader_suite(void);

Suite*
texture_suite(void);

int
main(int argc, char *argv[])
{
	// initialize a master suite and a suite runner
	Suite *s = suite_create("suite");
	SRunner *sr = srunner_create(s);

	// add external suites
	srunner_add_suite(sr, font_suite());
	srunner_add_suite(sr, image_suite());
	srunner_add_suite(sr, mesh_suite());
	srunner_add_suite(sr, render_suite());
	srunner_add_suite(sr, scene_suite());
	srunner_add_suite(sr, shader_suite());
	srunner_add_suite(sr, texture_suite());

	// execute all suites
	srunner_run_all(sr, CK_NORMAL);
	int failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
