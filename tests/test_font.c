#include "fixture.h"
#include <check.h>
#include <stdlib.h>

#include "font.h"

START_TEST(test_load_from_file)
{
	struct Font *font = font_from_file("tests/data/courier.ttf", 12);
	ck_assert(font != NULL);
	font_free(font);

	font = font_from_file("notfound.ttf", 12);
	ck_assert(font == NULL);
}
END_TEST

Suite*
font_suite(void)
{
	Suite *s = suite_create("font");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_load_from_file);

	suite_add_tcase(s, tc_core);

	return s;
}
