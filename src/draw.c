#include "draw.h"
#include "mesh.h"
#include "shader.h"
#include <assert.h>

static struct ShaderUniform u_model;
static struct ShaderUniform u_view;
static struct ShaderUniform u_projection;
static struct Shader *shader = NULL;

int
init_mesh_pipeline(err_t *r_err)
{
	const char *uniform_names[] = {
		"model",
		"view",
		"projection",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&u_model,
		&u_view,
		&u_projection
	};
	shader = shader_compile(
		"src/shaders/mesh.vert",
		"src/shaders/mesh.frag",
		uniform_names,
		uniforms,
		NULL,
		NULL,
		r_err
	);
	return shader != NULL;
}

int
draw_mesh(
	struct Mesh *mesh,
	Mat *model,
	Mat *view,
	Mat *proj
) {
	assert(mesh != NULL);
	assert(model != NULL);
	assert(view != NULL);
	assert(proj != NULL);

	int configured = (
		shader_bind(shader) &&
		shader_uniform_set(&u_model, 1, model) &&
		shader_uniform_set(&u_view, 1, view) &&
		shader_uniform_set(&u_projection, 1, proj)
	);

	if (!configured) {
		return 0;
	}

	glBindVertexArray(mesh->vao);
	glDrawElements(
		GL_TRIANGLES,
		mesh->index_count,
		GL_UNSIGNED_INT,
		(void*)(0)
	);

#ifdef DEBUG
	return glGetError() == GL_NO_ERROR;
#endif
	return 1;
}
