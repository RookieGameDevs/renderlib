#include "renderlib.h"
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

static struct ShaderUniform u_model;
static struct ShaderUniform u_view;
static struct ShaderUniform u_projection;
static struct ShaderUniform u_enable_skinning;
static struct ShaderUniformBlock ub_animation;
static struct ShaderUniform u_skin_transforms;
static struct ShaderUniform u_enable_texture_mapping;
static struct ShaderUniform u_texture_map_sampler;
static struct ShaderUniform u_enable_shadow_mapping;
static struct ShaderUniform u_shadow_map_sampler;
static struct ShaderUniform u_light_space_transform;
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
		"enable_texture_mapping",
		"texture_map_sampler",
		"enable_shadow_mapping",
		"shadow_map_sampler",
		"light_space_transform",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&u_model,
		&u_view,
		&u_projection,
		&u_enable_skinning,
		&u_enable_texture_mapping,
		&u_texture_map_sampler,
		&u_enable_shadow_mapping,
		&u_shadow_map_sampler,
		&u_light_space_transform
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

static int
configure_texture_mapping(struct Texture *texture)
{
	int enable_texture_mapping = texture != NULL;
	int ok = shader_uniform_set(
		&u_enable_texture_mapping,
		1,
		&enable_texture_mapping
	);

	if (enable_texture_mapping) {
		GLint tex_unit = 0;
		ok &= shader_uniform_set(
			&u_texture_map_sampler,
			1,
			&tex_unit
		);
		glActiveTexture(GL_TEXTURE0 + tex_unit);
		glBindTexture(texture->type, texture->id);
		if (glGetError() != GL_NO_ERROR) {
			err(ERR_OPENGL);
			return 0;
		}
	}

	return ok;
}

static int
configure_shadow_mapping(
	struct MeshRenderProps *props,
	int shadow_map
) {
	int enable_shadow_mapping = (
		props->receive_shadows &&
		shadow_map > 0
	);
	int configured = shader_uniform_set(
		&u_enable_shadow_mapping,
		1,
		&enable_shadow_mapping
	);
	if (enable_shadow_mapping) {
		configured &= shader_uniform_set(
			&u_shadow_map_sampler,
			1,
			&shadow_map
		);
		configured &= shader_uniform_set(
			&u_light_space_transform,
			1,
			&props->light_space_transform
		);
	}
	return configured;
}

int
draw_mesh(
	struct Mesh *mesh,
	struct MeshRenderProps *props,
	int shadow_map
) {
	assert(mesh != NULL);
	assert(props != NULL);

	int configured = (
		shader_bind(shader) &&
		shader_uniform_set(&u_model, 1, &props->model) &&
		shader_uniform_set(&u_view, 1, &props->view) &&
		shader_uniform_set(&u_projection, 1, &props->projection) &&
		configure_skinning(
			props->animation,
			shader,
			&u_enable_skinning,
			&u_skin_transforms,
			&ub_animation,
			skin_transforms_buffer
		) &&
		configure_texture_mapping(props->texture) &&
		configure_shadow_mapping(props, shadow_map)
	);
	if (!configured) {
		errf(ERR_GENERIC, "failed to configure mesh pipeline", 0);
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
