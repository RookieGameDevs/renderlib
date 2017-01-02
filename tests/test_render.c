#include "fixture.h"
#include <check.h>
#include <stdlib.h>

#include "anim.h"
#include "image.h"
#include "mesh.h"
#include "renderer.h"
#include "texture.h"

static struct Mesh *mesh = NULL;

START_TEST(test_render_mesh_simple)
{
	Mat identity;
	mat_ident(&identity);
	struct MeshRenderProps props = {
		.view = identity,
		.model = identity,
		.projection = identity,
		.enable_animation = 0
	};
	ck_assert(render_mesh(mesh, &props));
	ck_assert(renderer_present());
}
END_TEST

START_TEST(test_render_mesh_textured)
{
	struct Image *img = image_from_file("tests/data/zombie.jpg");
	ck_assert(img != NULL);

	struct Texture *tex = texture_from_image(img, GL_TEXTURE_2D);
	ck_assert(tex != NULL);

	Mat identity;
	mat_ident(&identity);
	struct MeshRenderProps props = {
		.view = identity,
		.model = identity,
		.projection = identity,
		.texture = tex
	};
	ck_assert(render_mesh(mesh, &props));
	ck_assert(renderer_present());
}
END_TEST

START_TEST(test_render_mesh_shadowed)
{
	Mat identity;
	mat_ident(&identity);
	struct MeshRenderProps props = {
		.view = identity,
		.model = identity,
		.projection = identity,
		.cast_shadows = 1,
		.receive_shadows = 1,
		.light_space_transform = identity,
		.enable_animation = 0
	};
	ck_assert(render_mesh(mesh, &props));
	ck_assert(renderer_present());
}
END_TEST

START_TEST(test_render_mesh_animated)
{
	struct AnimationInstance *inst = animation_instance_new(
		&mesh->animations[0]
	);
	ck_assert(inst != NULL);
	animation_instance_play(inst, 1.234);

	Mat identity;
	mat_ident(&identity);
	struct MeshRenderProps props = {
		.view = identity,
		.model = identity,
		.projection = identity,
		.enable_animation = 1,
		.animation = inst
	};
	ck_assert(render_mesh(mesh, &props));
	ck_assert(renderer_present());
}
END_TEST

static void
suite_setup(void)
{
	setup();
	mesh = mesh_from_file("tests/data/zombie.mesh");
}

static void
suite_teardown(void)
{
	mesh_free(mesh);
	mesh = NULL;
	teardown();
}

Suite*
render_suite(void)
{
	Suite *s = suite_create("render");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, suite_setup, suite_teardown);
	tcase_add_test(tc_core, test_render_mesh_simple);
	tcase_add_test(tc_core, test_render_mesh_textured);
	tcase_add_test(tc_core, test_render_mesh_shadowed);
	tcase_add_test(tc_core, test_render_mesh_animated);

	suite_add_tcase(s, tc_core);

	return s;
}

