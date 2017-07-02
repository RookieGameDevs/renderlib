#include "fixture.h"
#include <renderlib.h>
#include <check.h>
#include <stdlib.h>

static struct Font *font = NULL;

START_TEST(test_render_text)
{
	// create and initialize a Text
	struct Text *text = text_new(font);
	ck_assert(text != NULL);
	int ok = text_set_string(text, "The quick brown fox jumps over the lazy dog!");
	ck_assert(ok);
	text->color = vec(26 / 255.0, 146 / 255.0, 186 / 255.0, 1);

	// create a renderable Object from it
	struct Object *obj = text_to_object(text);
	ck_assert(obj != NULL);

	// initialize a render context
	Mat model, view, projection;
	struct ObjectRenderContext ctx = {
		.camera = NULL,
		.light = NULL,
		.model = &model,
		.view = &view,
		.projection = &projection
	};
	mat_ident(&model);
	mat_translate(&model, -(int)text->width / 2, (int)text->height / 2, 0);
	mat_ident(&view);
	mat_ident(&projection);
	mat_ortho(
		&projection,
		-TEST_WINDOW_WIDTH / 2,
		+TEST_WINDOW_WIDTH / 2,
		+TEST_WINDOW_HEIGHT / 2,
		-TEST_WINDOW_HEIGHT / 2,
		0,
		1
	);

	// render the object
	ok = obj->cls->render(obj, &ctx);
	ck_assert(ok);

	// ask the renderer to perform an actual draw
	renderer_clear();
	ok = renderer_present();
	ck_assert(ok);
	test_render_frame(__func__);

	obj->cls->free(obj);
	text_free(text);
}
END_TEST

static void
suite_setup(void)
{
	setup();
	font = font_from_file("tests/data/courier.ttf", 24);
	ck_assert(font != NULL);
}

static void
suite_teardown(void)
{
	font_free(font);
	font = NULL;
	teardown();
}

Suite*
text_suite(void)
{
	Suite *s = suite_create("text");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, suite_setup, suite_teardown);
	tcase_add_test(tc_core, test_render_text);
	suite_add_tcase(s, tc_core);

	return s;
}
