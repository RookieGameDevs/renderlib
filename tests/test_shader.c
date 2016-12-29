#include "shader.h"
#include "renderer.h"
#include <check.h>
#include <stdlib.h>

START_TEST(test_compile_string)
{
	const char *vert = (
		"#version 330 core\n"
		"void main() {\n"
		"	gl_Position = vec4(0, 0, 0, 1);\n"
		"}"
	);

	struct ShaderSource *vert_source = shader_source_from_string(
		vert,
		GL_VERTEX_SHADER,
		NULL
	);
	ck_assert(vert_source != NULL);
	ck_assert_int_ne(vert_source->src, 0);
	shader_source_free(vert_source);

	const char *frag = (
		"#version 330 core\n"
		"out vec4 color;\n"
		"void main() {\n"
		"	color = vec4(1);\n"
		"}"
	);

	struct ShaderSource *frag_source = shader_source_from_string(
		frag,
		GL_FRAGMENT_SHADER,
		NULL
	);
	ck_assert(frag_source != NULL);
	ck_assert_int_ne(frag_source->src, 0);
	shader_source_free(frag_source);
}
END_TEST

START_TEST(test_compile_glsl_files)
{
	struct Shader *shader = shader_compile(
		"tests/data/test.vert",
		"tests/data/test.frag",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);
	ck_assert(shader != NULL);
	ck_assert_int_ne(shader->prog, 0);
	shader_free(shader);
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
shader_suite(void)
{
	Suite *s = suite_create("shader");

	TCase *tc_core = tcase_create("core");
	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_compile_string);
	tcase_add_test(tc_core, test_compile_glsl_files);

	suite_add_tcase(s, tc_core);

	return s;
}

