#include "renderlib.h"
#include <GL/glew.h>
#include <assert.h>
#include <stdlib.h>

static const char *vertex_shader = (
# include "shadow.vert.h"
);

static const char *fragment_shader = (
# include "shadow.frag.h"
);

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
static struct ShaderSource *shader_sources[2] = { NULL, NULL };
static struct ShaderUniform u_mvp;
static struct ShaderUniform u_enable_skinning;
static struct ShaderUniform u_skin_transforms;
static struct ShaderUniformBlock ub_animation;

static GLuint skin_transforms_buffer = 0;

static void
cleanup(void)
{
	shader_free(shader);
	shader_source_free(shader_sources[0]);
	shader_source_free(shader_sources[1]);
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

	// compile shadow pipeline shader and initialize uniforms
	shader_sources[0] = shader_source_from_string(
		vertex_shader,
		GL_VERTEX_SHADER
	);
	shader_sources[1] = shader_source_from_string(
		fragment_shader,
		GL_FRAGMENT_SHADER
	);
	if (!shader_sources[0] ||
	    !shader_sources[1] ||
	    !(shader = shader_new(shader_sources, 2))) {
		errf(ERR_GENERIC, "shadow pipeline shader compile failed");
		return 0;
	} else if (!shader_get_uniforms(shader, uniform_names, uniforms) ||
	           !shader_get_uniform_blocks(shader, uniform_block_names, uniform_blocks)) {
		errf(ERR_GENERIC, "bad shadow pipeline shader");
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
draw_mesh_shadow(struct Mesh *mesh, struct MeshProps *props)
{
	assert(mesh != NULL);
	assert(props != NULL);

	// compute final model-view-projection transform in light-space
	Mat mvp;
	mat_mul(&props->light->transform, &props->model, &mvp);

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