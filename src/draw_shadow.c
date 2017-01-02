#include "anim.h"
#include "error.h"
#include "mesh.h"
#include "renderer.h"
#include "shader.h"
#include <GL/glew.h>
#include <assert.h>
#include <stdlib.h>

// defined in draw_common.c
int
update_skin_transforms_buffer(
	struct AnimationInstance *inst,
	GLuint buffer,
	size_t offset,
	size_t size
);

int
configure_skinning(
	struct AnimationInstance *inst,
	struct Shader *shader,
	struct ShaderUniform *u_enable_skinning,
	struct ShaderUniform *u_skin_transforms,
	struct ShaderUniformBlock *ub_animation,
	GLuint buffer
);

static struct Shader *shader = NULL;
static struct ShaderUniform u_mvp;
static struct ShaderUniform u_enable_skinning;
static struct ShaderUniform u_skin_transforms;
static struct ShaderUniformBlock ub_animation;

static GLuint skin_transforms_buffer = 0;

static void
cleanup(void)
{
	shader_free(shader);
	glDeleteBuffers(1, &skin_transforms_buffer);
}

int
init_shadow_pipeline(void)
{
	// cleanup resources at program exit
	atexit(cleanup);

	// uniform names and receiver pointers
	const char *uniform_names[] = {
		"mvp",
		"enable_skinning",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&u_mvp,
		&u_enable_skinning
	};

	// uniform block names and receiver pointers
	const char *uniform_block_names[] = {
		"Animation",
		NULL
	};
	struct ShaderUniformBlock *uniform_blocks[] = {
		&ub_animation
	};

	// compile mesh pipeline shader and initialize uniforms
	shader = shader_compile(
		"src/shaders/shadow.vert",
		"src/shaders/shadow.frag",
		uniform_names,
		uniforms,
		uniform_block_names,
		uniform_blocks
	);
	if (!shader) {
		return 0;
	}

	// lookup skin transforms array uniform within the uniform block
	const struct ShaderUniform *u = shader_uniform_block_get_uniform(
		&ub_animation,
		"skin_transforms[0]"
	);
	if (!u) {
		return 0;
	}
	u_skin_transforms = *u;

	// allocate an OpenGL buffer for animation uniform block
	glGenBuffers(1, &skin_transforms_buffer);
	if (!skin_transforms_buffer) {
		err(ERR_OPENGL);
		return 0;
	}

	// initialize buffer storage
	glBindBuffer(GL_UNIFORM_BUFFER, skin_transforms_buffer);
	glBufferData(
		GL_UNIFORM_BUFFER,
		ub_animation.size,
		NULL,
		GL_DYNAMIC_DRAW
	);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// check for any OpenGL-related errors
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}

	return 1;
}

int
draw_mesh_shadow(struct Mesh *mesh, struct MeshRenderProps *props)
{
	assert(mesh != NULL);
	assert(props != NULL);

	// compute final model-view-projection transform in light-space
	Mat mvp;
	mat_mul(&props->light_space_transform, &props->model, &mvp);

	int configured = (
		shader_bind(shader) &&
		shader_uniform_set(&u_mvp, 1, &mvp) &&
		configure_skinning(
			props->animation,
			shader,
			&u_enable_skinning,
			&u_skin_transforms,
			&ub_animation,
			skin_transforms_buffer
		)
	);
	if (!configured) {
		errf(ERR_GENERIC, "failed to configure shadow pipeline", 0);
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
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}
#endif
	return 1;
}