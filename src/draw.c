#include "anim.h"
#include "mesh.h"
#include "renderer.h"
#include "shader.h"
#include <assert.h>
#include <stdlib.h>

static struct ShaderUniform u_model;
static struct ShaderUniform u_view;
static struct ShaderUniform u_projection;
static struct ShaderUniform u_enable_skinning;
static struct ShaderUniformBlock ub_skin_transforms;
static struct ShaderUniform u_skin_transforms;
static struct Shader *shader = NULL;

static GLuint skin_transforms_buffer = 0;

static void
cleanup(void)
{
	shader_free(shader);
	glDeleteBuffers(1, &skin_transforms_buffer);
}

int
init_mesh_pipeline(void)
{
	// cleanup resources at program exit
	atexit(cleanup);

	// uniform names and receiver pointers
	const char *uniform_names[] = {
		"model",
		"view",
		"projection",
		"enable_skinning",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&u_model,
		&u_view,
		&u_projection,
		&u_enable_skinning,
	};

	// uniform block names and receiver pointers
	const char *uniform_block_names[] = {
		"SkinTransforms",
		NULL
	};
	struct ShaderUniformBlock *uniform_blocks[] = {
		&ub_skin_transforms
	};

	// compile mesh pipeline shader and initialize uniforms
	shader = shader_compile(
		"src/shaders/mesh.vert",
		"src/shaders/mesh.frag",
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
		&ub_skin_transforms,
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
		ub_skin_transforms.size,
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

static int
configure_skinning(int enable_animation, struct AnimationInstance *inst)
{
	// configure uniforms
	int configured = shader_uniform_set(
		&u_enable_skinning,
		1,
		&enable_animation
	);
	if (!enable_animation) {
		return configured;
	}

	// update skin transforms buffer data
	glBindBuffer(GL_UNIFORM_BUFFER, skin_transforms_buffer);
	struct Animation *anim = inst->anim;
	Mat *dst = glMapBufferRange(
		GL_UNIFORM_BUFFER,
		u_skin_transforms.offset,
		u_skin_transforms.size,
		(
			GL_MAP_WRITE_BIT |
			GL_MAP_INVALIDATE_BUFFER_BIT |
			GL_MAP_UNSYNCHRONIZED_BIT
		)
	);
	if (!dst || glGetError() != GL_NO_ERROR) {
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		err(ERR_OPENGL);
		return 0;
	}
	Mat tmp;
	for (int j = 0; j < anim->skeleton->joint_count; j++) {
		mat_mul(
			&inst->joint_transforms[j],
			&anim->skeleton->joints[j].inv_bind_pose,
			&tmp
		);
		mat_transpose(&tmp, &dst[j]);
	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// perform indexed buffer binding
	GLuint binding_index = 1;
	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		binding_index,
		skin_transforms_buffer
	);
	glUniformBlockBinding(
		shader->prog,
		ub_skin_transforms.index,
		binding_index
	);

	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}

	return 1;
}

int
draw_mesh(struct Mesh *mesh, struct MeshRenderProps *props) {
	assert(mesh != NULL);
	assert(props != NULL);

	int configured = (
		shader_bind(shader) &&
		shader_uniform_set(&u_model, 1, &props->model) &&
		shader_uniform_set(&u_view, 1, &props->view) &&
		shader_uniform_set(&u_projection, 1, &props->projection) &&
		configure_skinning(props->enable_animation, props->animation)
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
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}
#endif
	return 1;
}
